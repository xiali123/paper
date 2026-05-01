#include "features/security/SecurityModule.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <cstdlib>

#include "../../core/external/nlohmann/json.hpp"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/err.h>

// Base64辅助函数（静态函数避免链接冲突）
static std::string base64_encode(const unsigned char* data, size_t len) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    result.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3) {
        unsigned char b0 = data[i];
        unsigned char b1 = (i + 1 < len) ? data[i + 1] : 0;
        unsigned char b2 = (i + 2 < len) ? data[i + 2] : 0;

        result.push_back(base64_chars[b0 >> 2]);
        result.push_back(base64_chars[((b0 & 0x03) << 4) | (b1 >> 4)]);
        result.push_back((i + 1 < len) ? base64_chars[((b1 & 0x0F) << 2) | (b2 >> 6)] : '=');
        result.push_back((i + 2 < len) ? base64_chars[b2 & 0x3F] : '=');
    }

    return result;
}

static std::vector<unsigned char> base64_decode(const std::string& encoded_string) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<unsigned char> result;
    result.reserve((encoded_string.size() * 3) / 4);

    int val = 0, valb = -8;
    for (unsigned char c : encoded_string) {
        if (c == '=') break;

        std::string::size_type pos = base64_chars.find(c);
        if (pos == std::string::npos) continue;

        val = (val << 6) + pos;
        valb += 6;

        if (valb >= 0) {
            result.push_back((val >> (valb - 8)) & 0xFF);
            valb -= 8;
        }
    }

    return result;
}

// URL-safe Base64 (no padding) for JWT
static std::string base64url_encode(const unsigned char* data, size_t len) {
    std::string b64 = base64_encode(data, len);
    // Replace +/ with -_ and strip padding
    for (auto& c : b64) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!b64.empty() && b64.back() == '=') b64.pop_back();
    return b64;
}

static std::string base64url_encode(const std::string& str) {
    return base64url_encode(reinterpret_cast<const unsigned char*>(str.data()), str.size());
}

static std::vector<unsigned char> base64url_decode(const std::string& encoded) {
    std::string b64 = encoded;
    // Restore standard base64 chars
    for (auto& c : b64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    // Add padding
    while (b64.size() % 4 != 0) b64.push_back('=');
    return base64_decode(b64);
}

namespace PaperCrawler {

// ============================================================================
// SecurityModule::Impl — Real OpenSSL implementations
// ============================================================================

class SecurityModule::Impl {
public:
    std::string jwtSecret_;
    std::chrono::seconds defaultExpiry_{3600};
    int bcryptCost_{12};

    Impl() {
        const char* envSecret = std::getenv("JWT_SECRET");
        if (envSecret && std::string(envSecret).length() >= 16) {
            jwtSecret_ = envSecret;
        } else {
            spdlog::warn("[Security] JWT_SECRET env var not set or too short (<16 chars). "
                         "Generating ephemeral secret — tokens will not survive restart.");
            std::array<unsigned char, 32> buf{};
            if (RAND_bytes(buf.data(), buf.size()) == 1) {
                std::ostringstream hex;
                for (auto c : buf) hex << std::hex << std::setfill('0') << std::setw(2) << (int)c;
                jwtSecret_ = hex.str();
            } else {
                throw std::runtime_error("Failed to generate JWT secret");
            }
        }
    }

    SecurityStats stats_{};

    // Real JWT generation with HMAC-SHA256
    std::string generateJWT(const JWTClaims& claims, std::chrono::seconds expiry) {
        // Header: {"alg":"HS256","typ":"JWT"}
        std::string headerJson = R"({"alg":"HS256","typ":"JWT"})";
        std::string headerB64 = base64url_encode(headerJson);

        // Payload
        auto now = std::chrono::system_clock::now();
        auto iat = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        auto exp = iat + expiry.count();

        std::ostringstream payload;
        payload << "{\"iat\":" << iat << ",\"exp\":" << exp;
        for (const auto& [key, value] : claims) {
            payload << ",\"" << key << "\":\"" << value << "\"";
        }
        payload << "}";

        std::string payloadB64 = base64url_encode(payload.str());

        // Sign: HMAC-SHA256(header.payload, secret)
        std::string signingInput = headerB64 + "." + payloadB64;
        auto sig = hmacSHA256(signingInput, jwtSecret_);
        std::string signatureB64 = base64url_encode(sig.data(), sig.size());

        stats_.totalJWTGenerated++;
        spdlog::debug("[Security] JWT generated (exp: {})", exp);

        return signingInput + "." + signatureB64;
    }

    JWTVerifyResult verifyJWT(const std::string& token) {
        JWTVerifyResult result;

        if (token.empty()) {
            result.valid = false;
            result.errorMessage = "Token is empty";
            stats_.totalJWTVerifyFailures++;
            return result;
        }

        // Split by '.'
        size_t dot1 = token.find('.');
        size_t dot2 = (dot1 != std::string::npos) ? token.find('.', dot1 + 1) : std::string::npos;

        if (dot1 == std::string::npos || dot2 == std::string::npos || dot2 >= token.size() - 1) {
            result.valid = false;
            result.errorMessage = "Invalid token format";
            stats_.totalJWTVerifyFailures++;
            return result;
        }

        std::string headerB64 = token.substr(0, dot1);
        std::string payloadB64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
        std::string signatureB64 = token.substr(dot2 + 1);

        // Verify signature
        std::string signingInput = headerB64 + "." + payloadB64;
        auto expectedSig = hmacSHA256(signingInput, jwtSecret_);
        std::string expectedB64 = base64url_encode(expectedSig.data(), expectedSig.size());

        if (signatureB64 != expectedB64) {
            result.valid = false;
            result.errorMessage = "Invalid signature";
            stats_.totalJWTVerifyFailures++;
            return result;
        }

        // Decode payload
        auto payloadBytes = base64url_decode(payloadB64);
        std::string payloadStr(payloadBytes.begin(), payloadBytes.end());

        // Simple JSON parsing for claims
        auto parsed = nlohmann::json::parse(payloadStr, nullptr, false);
        if (parsed.is_discarded()) {
            result.valid = false;
            result.errorMessage = "Invalid payload JSON";
            stats_.totalJWTVerifyFailures++;
            return result;
        }

        // Check expiry
        if (parsed.contains("exp")) {
            auto exp = parsed["exp"].get<int64_t>();
            auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            if (now > exp) {
                result.valid = false;
                result.errorMessage = "Token expired";
                stats_.totalJWTVerifyFailures++;
                return result;
            }
        }

        // Extract claims
        for (auto& [key, value] : parsed.items()) {
            if (key != "iat" && key != "exp" && value.is_string()) {
                result.claims[key] = value.get<std::string>();
            }
        }

        result.valid = true;
        stats_.totalJWTVerified++;
        spdlog::debug("[Security] JWT verified");
        return result;
    }

    // PBKDF2-based password hashing (bcrypt requires separate lib; PBKDF2 is OpenSSL-native)
    PasswordHashResult hashPassword(const std::string& password, int cost) {
        PasswordHashResult result;

        // Generate random salt (16 bytes)
        unsigned char salt[16];
        if (RAND_bytes(salt, sizeof(salt)) != 1) {
            result.success = false;
            result.errorMessage = "Failed to generate salt";
            return result;
        }

        // Derive key using PBKDF2-HMAC-SHA256
        // iterations = 2^cost, capped to reasonable range
        int iterations = 1 << std::min(std::max(cost, 4), 20);
        std::vector<unsigned char> derived(32);

        if (PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                               salt, sizeof(salt),
                               iterations, EVP_sha256(),
                               derived.size(), derived.data()) != 1) {
            result.success = false;
            result.errorMessage = "PBKDF2 derivation failed";
            return result;
        }

        // Format: $pbkdf2-sha256$iterations$salt_base64$hash_base64
        std::ostringstream hash;
        hash << "$pbkdf2-sha256$" << iterations << "$"
             << base64_encode(salt, sizeof(salt)) << "$"
             << base64_encode(derived.data(), derived.size());

        result.success = true;
        result.hash = hash.str();
        stats_.totalPasswordsHashed++;
        spdlog::debug("[Security] Password hashed (iterations: {})", iterations);
        return result;
    }

    bool verifyPassword(const std::string& password, const std::string& storedHash) {
        // Parse stored hash: $pbkdf2-sha256$iterations$salt$hash
        if (storedHash.find("$pbkdf2-sha256$") != 0) {
            spdlog::error("[Security] Unknown hash format");
            return false;
        }

        std::istringstream ss(storedHash.substr(15)); // skip "$pbkdf2-sha256$"
        std::string iterStr, saltB64, hashB64;
        if (!std::getline(ss, iterStr, '$') ||
            !std::getline(ss, saltB64, '$') ||
            !std::getline(ss, hashB64, '$')) {
            spdlog::error("[Security] Malformed hash");
            return false;
        }

        int iterations = std::stoi(iterStr);
        auto salt = base64_decode(saltB64);
        auto storedHashBytes = base64_decode(hashB64);

        if (salt.empty() || storedHashBytes.empty()) {
            spdlog::error("[Security] Failed to decode salt or hash: saltB64='{}' hashB64='{}'", saltB64, hashB64);
            spdlog::error("[Security] Failed to decode salt or hash");
            return false;
        }

        // Derive with same parameters
        std::vector<unsigned char> derived(storedHashBytes.size());
        if (PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                               salt.data(), salt.size(),
                               iterations, EVP_sha256(),
                               derived.size(), derived.data()) != 1) {
            spdlog::error("[Security] PBKDF2 derivation failed");
            return false;
        }

        // Constant-time comparison
        bool match = (derived.size() == storedHashBytes.size()) &&
                     CRYPTO_memcmp(derived.data(), storedHashBytes.data(), derived.size()) == 0;

        if (!match) {
            spdlog::error("[Security] Hash mismatch for input hash='{}' saltB64='{}' hashB64='{}' iter={} derived_size={} stored_size={}",
                          storedHash, saltB64, hashB64, iterations, derived.size(), storedHashBytes.size());
        }

        if (match) {
            stats_.totalPasswordsVerified++;
        }
        return match;
    }

    // Real AES-256-GCM encryption
    EncryptionResult encrypt(const std::vector<uint8_t>& data,
                             const std::vector<uint8_t>& key,
                             const std::vector<uint8_t>& nonce) {
        EncryptionResult result;

        if (key.size() != 32) {
            result.errorMessage = "Key must be 32 bytes for AES-256-GCM";
            return result;
        }
        if (nonce.size() != 12) {
            result.errorMessage = "Nonce must be 12 bytes for AES-256-GCM";
            return result;
        }

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            result.errorMessage = "Failed to create cipher context";
            return result;
        }

        std::vector<uint8_t> ciphertext(data.size());
        std::vector<uint8_t> tag(16);
        int len = 0, ciphertextLen = 0;

        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1 ||
            EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.errorMessage = "AES-256-GCM init failed";
            return result;
        }

        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                              data.data(), data.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.errorMessage = "AES-256-GCM encrypt failed";
            return result;
        }
        ciphertextLen = len;

        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.errorMessage = "AES-256-GCM final failed";
            return result;
        }
        ciphertextLen += len;

        // Get tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.errorMessage = "Failed to get GCM tag";
            return result;
        }

        EVP_CIPHER_CTX_free(ctx);

        // Output: ciphertext || tag (16 bytes appended)
        result.encryptedData.resize(ciphertextLen + 16);
        std::memcpy(result.encryptedData.data(), ciphertext.data(), ciphertextLen);
        std::memcpy(result.encryptedData.data() + ciphertextLen, tag.data(), 16);

        result.success = true;
        stats_.totalEncryptions++;
        return result;
    }

    DecryptionResult decrypt(const std::vector<uint8_t>& encryptedData,
                             const std::vector<uint8_t>& key,
                             const std::vector<uint8_t>& nonce) {
        DecryptionResult result;

        if (key.size() != 32) {
            result.errorMessage = "Key must be 32 bytes for AES-256-GCM";
            return result;
        }
        if (nonce.size() != 12) {
            result.errorMessage = "Nonce must be 12 bytes for AES-256-GCM";
            return result;
        }
        if (encryptedData.size() < 16) {
            result.errorMessage = "Encrypted data too short (missing GCM tag)";
            return result;
        }

        // Split ciphertext and tag
        size_t ciphertextLen = encryptedData.size() - 16;
        const uint8_t* ciphertext = encryptedData.data();
        const uint8_t* tag = encryptedData.data() + ciphertextLen;

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            result.errorMessage = "Failed to create cipher context";
            return result;
        }

        std::vector<uint8_t> plaintext(ciphertextLen);
        int len = 0, plaintextLen = 0;

        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1 ||
            EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.errorMessage = "AES-256-GCM decrypt init failed";
            return result;
        }

        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                              ciphertext, ciphertextLen) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.errorMessage = "AES-256-GCM decrypt update failed";
            return result;
        }
        plaintextLen = len;

        // Set expected tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16,
                                const_cast<uint8_t*>(tag)) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.errorMessage = "Failed to set GCM tag";
            return result;
        }

        // Verify tag (this is the authentication step)
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            result.errorMessage = "AES-256-GCM authentication failed (tampered data?)";
            return result;
        }
        plaintextLen += len;

        EVP_CIPHER_CTX_free(ctx);

        result.decryptedData.assign(plaintext.begin(), plaintext.begin() + plaintextLen);
        result.success = true;
        stats_.totalDecryptions++;
        return result;
    }

    std::vector<uint8_t> generateKey(size_t length) {
        std::vector<uint8_t> key(length);
        if (RAND_bytes(key.data(), length) != 1) {
            spdlog::error("[Security] RAND_bytes failed, fallback to std::random_device");
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);
            for (auto& byte : key) byte = static_cast<uint8_t>(dis(gen));
        }
        return key;
    }

    std::vector<uint8_t> generateNonce(size_t length) {
        return generateKey(length);
    }

    // Real HMAC-SHA256
    std::vector<uint8_t> hmacSign(const std::string& message,
                                   const std::vector<uint8_t>& key) {
        return hmacSHA256(message, std::string(key.begin(), key.end()));
    }

    bool hmacVerify(const std::string& message,
                    const std::vector<uint8_t>& signature,
                    const std::vector<uint8_t>& key) {
        auto computed = hmacSign(message, key);
        if (computed.size() != signature.size()) return false;
        return CRYPTO_memcmp(computed.data(), signature.data(), computed.size()) == 0;
    }

    // Real SHA-256
    std::vector<uint8_t> sha256(const std::string& data) {
        std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
        SHA256(reinterpret_cast<const unsigned char*>(data.c_str()),
               data.size(), hash.data());
        return hash;
    }

    // Real SHA-512
    std::vector<uint8_t> sha512(const std::string& data) {
        std::vector<uint8_t> hash(SHA512_DIGEST_LENGTH);
        SHA512(reinterpret_cast<const unsigned char*>(data.c_str()),
               data.size(), hash.data());
        return hash;
    }

    std::string base64Encode(const std::vector<uint8_t>& data) {
        return base64_encode(data.data(), data.size());
    }

    std::vector<uint8_t> base64Decode(const std::string& encoded) {
        return base64_decode(encoded);
    }

    SecurityStats getStats() const {
        return stats_;
    }

private:
    // Internal HMAC-SHA256 with string key
    std::vector<uint8_t> hmacSHA256(const std::string& data, const std::string& key) {
        std::vector<uint8_t> result(SHA256_DIGEST_LENGTH);
        unsigned int hmacLen = 0;

        HMAC(EVP_sha256(),
             key.c_str(), key.size(),
             reinterpret_cast<const unsigned char*>(data.c_str()), data.size(),
             result.data(), &hmacLen);

        return result;
    }
};

// ============================================================================
// SecurityModule
// ============================================================================

SecurityModule::SecurityModule()
    : impl_(std::make_unique<Impl>()) {}

SecurityModule::~SecurityModule() = default;

bool SecurityModule::initialize() {
    spdlog::info("[Security] Initializing SecurityModule");
    spdlog::info("  JWT secret: {}", jwtSecret_.empty() ? "default (WARNING: change in production)" : "configured");
    spdlog::info("  JWT expiry: {}s", defaultJWTExpiry_.count());
    spdlog::info("  bcrypt cost: {}", bcryptCost_);
    return true;
}

bool SecurityModule::start() {
    spdlog::info("[Security] SecurityModule started (OpenSSL crypto)");
    return true;
}

bool SecurityModule::stop() {
    spdlog::info("[Security] SecurityModule stopped");

    auto stats = getStats();
    spdlog::info("  JWT generated: {}", stats.totalJWTGenerated);
    spdlog::info("  JWT verified: {}", stats.totalJWTVerified);
    spdlog::info("  JWT verify failures: {}", stats.totalJWTVerifyFailures);
    spdlog::info("  Passwords hashed: {}", stats.totalPasswordsHashed);
    spdlog::info("  Passwords verified: {}", stats.totalPasswordsVerified);

    return true;
}

void SecurityModule::cleanup() {
    // Securely wipe JWT secret from memory
    std::fill(jwtSecret_.begin(), jwtSecret_.end(), '\0');
}

std::string SecurityModule::generateJWT(const JWTClaims& claims,
                                       std::chrono::seconds expiry) {
    return impl_->generateJWT(claims, expiry);
}

JWTVerifyResult SecurityModule::verifyJWT(const std::string& token) {
    return impl_->verifyJWT(token);
}

std::string SecurityModule::refreshJWT(const std::string& token) {
    auto result = verifyJWT(token);
    if (!result.valid) {
        return "";
    }

    JWTClaims newClaims = result.claims;
    newClaims.erase("iat");
    newClaims.erase("exp");

    return generateJWT(newClaims, defaultJWTExpiry_);
}

PasswordHashResult SecurityModule::hashPassword(const std::string& password, int cost) {
    return impl_->hashPassword(password, cost);
}

bool SecurityModule::verifyPassword(const std::string& password, const std::string& hash) {
    return impl_->verifyPassword(password, hash);
}

EncryptionResult SecurityModule::encrypt(const std::vector<uint8_t>& data,
                                             const std::vector<uint8_t>& key,
                                             const std::vector<uint8_t>& nonce) {
    return impl_->encrypt(data, key, nonce);
}

DecryptionResult SecurityModule::decrypt(const std::vector<uint8_t>& encryptedData,
                                           const std::vector<uint8_t>& key,
                                           const std::vector<uint8_t>& nonce) {
    return impl_->decrypt(encryptedData, key, nonce);
}

std::vector<uint8_t> SecurityModule::generateKey(size_t length) {
    return impl_->generateKey(length);
}

std::vector<uint8_t> SecurityModule::generateNonce(size_t length) {
    return impl_->generateNonce(length);
}

std::vector<uint8_t> SecurityModule::hmacSign(const std::string& message,
                                               const std::vector<uint8_t>& key) {
    return impl_->hmacSign(message, key);
}

bool SecurityModule::hmacVerify(const std::string& message,
                               const std::vector<uint8_t>& signature,
                               const std::vector<uint8_t>& key) {
    return impl_->hmacVerify(message, signature, key);
}

std::vector<uint8_t> SecurityModule::sha256(const std::string& data) {
    return impl_->sha256(data);
}

std::vector<uint8_t> SecurityModule::sha512(const std::string& data) {
    return impl_->sha512(data);
}

std::string SecurityModule::base64Encode(const std::vector<uint8_t>& data) {
    return impl_->base64Encode(data);
}

std::vector<uint8_t> SecurityModule::base64Decode(const std::string& encoded) {
    return impl_->base64Decode(encoded);
}

void SecurityModule::setJWTSecret(const std::string& secret) {
    jwtSecret_ = secret;
}

void SecurityModule::setDefaultJWTExpiry(std::chrono::seconds expiry) {
    defaultJWTExpiry_ = expiry;
}

SecurityModule::SecurityStats SecurityModule::getStats() const {
    return impl_->getStats();
}

} // namespace PaperCrawler
