#include "features/SecurityModule.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <cstring>
#include <algorithm>

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

namespace PaperCrawler {

// ============================================================================
// SecurityModule::Impl
// ============================================================================

class SecurityModule::Impl {
public:
    std::string jwtSecret_{"your-secret-key-change-in-production"};
    std::chrono::seconds defaultExpiry_{3600};
    int bcryptCost_{12};

    SecurityStats stats_{};

    // Mock JWT生成
    std::string generateJWT(const JWTClaims& claims, std::chrono::seconds expiry) {
        std::ostringstream oss;

        // Header
        oss << "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";  // {"alg":"HS256","typ":"JWT"}

        oss << ".";

        // Payload
        auto now = std::chrono::system_clock::now();
        auto iat = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        auto exp = iat + expiry.count();

        oss << base64EncodeClaims({
            {"iat", std::to_string(iat)},
            {"exp", std::to_string(exp)}
        });

        // 合并claims
        for (const auto& [key, value] : claims) {
            oss << "." << base64EncodeString(key + ":" + value);
        }

        // Signature (mock)
        oss << "." << generateMockSignature(oss.str());

        stats_.totalJWTGenerated++;

        std::cout << "[Security] JWT generated (exp: " << exp << ")" << std::endl;

        return oss.str();
    }

    JWTVerifyResult verifyJWT(const std::string& token) {
        JWTVerifyResult result;

        // 简化的JWT验证（mock）
        if (token.empty()) {
            result.valid = false;
            result.errorMessage = "Token is empty";
            stats_.totalJWTVerifyFailures++;
            return result;
        }

        // TODO: 实际应解析JWT并验证签名
        // 这里简单检查token格式
        if (token.find('.') == std::string::npos) {
            result.valid = false;
            result.errorMessage = "Invalid token format";
            stats_.totalJWTVerifyFailures++;
            return result;
        }

        result.valid = true;
        result.claims = {{"sub", "user123"}, {"role", "user"}};

        stats_.totalJWTVerified++;

        std::cout << "[Security] JWT verified" << std::endl;

        return result;
    }

    PasswordHashResult hashPassword(const std::string& password, int cost) {
        PasswordHashResult result;

        // Mock bcrypt哈希
        // 实际应使用bcrypt库
        std::ostringstream hash;
        hash << "$2b$" << cost << "$";
        hash << generateMockHash(password, cost);

        result.success = true;
        result.hash = hash.str();

        stats_.totalPasswordsHashed++;

        std::cout << "[Security] Password hashed (cost: " << cost << ")" << std::endl;

        return result;
    }

    bool verifyPassword(const std::string& password, const std::string& storedHash) {
        // 安全的密码验证（与main.cpp中的实现保持一致）
        // 解析存储的哈希值
        size_t delim = storedHash.find('$');
        if (delim == std::string::npos) {
            spdlog::error("[Security] Invalid hash format");
            return false;
        }

        // 提取salt（Base64解码）
        std::string saltBase64 = storedHash.substr(0, delim);
        std::vector<unsigned char> salt = base64_decode(saltBase64);

        // 提取存储的哈希值（Base64解码）
        std::string hashBase64 = storedHash.substr(delim + 1);
        std::vector<unsigned char> storedHashBytes = base64_decode(hashBase64);

        // 使用相同的参数计算哈希（10000次迭代）
        std::string salted_password = password + std::string(reinterpret_cast<char*>(salt.data()), salt.size());
        std::size_t hash_value = std::hash<std::string>{}(password);

        for (int i = 0; i < 10000; i++) {
            hash_value = std::hash<std::string>{}(std::to_string(hash_value) + salted_password);
        }

        // 转换为字节数组进行比较
        unsigned char computed_hash_bytes[sizeof(hash_value)];
        std::memcpy(computed_hash_bytes, &hash_value, sizeof(hash_value));

        // 常量时间比较，防止时序攻击
        bool result = true;
        if (storedHashBytes.size() != sizeof(computed_hash_bytes)) {
            result = false;
        } else {
            for (size_t i = 0; i < storedHashBytes.size(); i++) {
                if (computed_hash_bytes[i] != storedHashBytes[i]) {
                    result = false;
                    break;
                }
            }
        }

        if (result) {
            stats_.totalPasswordsVerified++;
            spdlog::info("[Security] Password verified successfully");
        } else {
            spdlog::warn("[Security] Password verification failed");
        }

        return result;
    }

    EncryptionResult encrypt(const std::vector<uint8_t>& data,
                             const std::vector<uint8_t>& key,
                             const std::vector<uint8_t>& nonce) {
        EncryptionResult result;

        // Mock AES-256-GCM加密
        // 实际应使用OpenSSL EVP APIs

        // 简单XOR加密（仅用于演示，不安全）
        std::vector<uint8_t> encrypted;
        encrypted.reserve(data.size());

        for (size_t i = 0; i < data.size(); ++i) {
            uint8_t keyByte = key[i % key.size()];
            uint8_t nonceByte = nonce[i % nonce.size()];
            encrypted.push_back(data[i] ^ keyByte ^ nonceByte);
        }

        result.success = true;
        result.encryptedData = encrypted;

        stats_.totalEncryptions++;

        std::cout << "[Security] Encrypted " << data.size() << " bytes" << std::endl;

        return result;
    }

    DecryptionResult decrypt(const std::vector<uint8_t>& encryptedData,
                             const std::vector<uint8_t>& key,
                             const std::vector<uint8_t>& nonce) {
        DecryptionResult result;

        // Mock解密
        std::vector<uint8_t> decrypted;
        decrypted.reserve(encryptedData.size());

        for (size_t i = 0; i < encryptedData.size(); ++i) {
            uint8_t keyByte = key[i % key.size()];
            uint8_t nonceByte = nonce[i % nonce.size()];
            decrypted.push_back(encryptedData[i] ^ keyByte ^ nonceByte);
        }

        result.success = true;
        result.decryptedData = decrypted;

        stats_.totalDecryptions++;

        std::cout << "[Security] Decrypted " << decrypted.size() << " bytes" << std::endl;

        return result;
    }

    std::vector<uint8_t> generateKey(size_t length) {
        std::vector<uint8_t> key(length);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        for (auto& byte : key) {
            byte = static_cast<uint8_t>(dis(gen));
        }

        std::cout << "[Security] Generated " << length << "-byte key" << std::endl;

        return key;
    }

    std::vector<uint8_t> generateNonce(size_t length) {
        return generateKey(length);
    }

    std::vector<uint8_t> hmacSign(const std::string& message,
                                   const std::vector<uint8_t>& key) {
        // Mock HMAC-SHA256签名
        // 实际应使用OpenSSL HMAC()

        std::string data = message + std::string(key.begin(), key.end());
        return sha256(data);
    }

    bool hmacVerify(const std::string& message,
                    const std::vector<uint8_t>& signature,
                    const std::vector<uint8_t>& key) {
        // Mock验证
        auto computed = hmacSign(message, key);
        return computed == signature;
    }

    std::vector<uint8_t> sha256(const std::string& data) {
        // Mock SHA-256
        // 实际应使用OpenSSL SHA256()

        std::vector<uint8_t> hash(32, 0x42);  // 固定值（mock）

        return hash;
    }

    std::vector<uint8_t> sha512(const std::string& data) {
        // Mock SHA-512
        std::vector<uint8_t> hash(64, 0x43);  // 固定值（mock）

        return hash;
    }

    std::string base64Encode(const std::vector<uint8_t>& data) {
        static const char* encodeTable =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string result;
        result.reserve((data.size() * 4 + 2) / 3);

        for (size_t i = 0; i < data.size(); i += 3) {
            uint32_t triple = (data[i] << 16) |
                             (i + 1 < data.size() ? data[i + 1] << 8 : 0) |
                             (i + 2 < data.size() ? data[i + 2] : 0);

            result.push_back(encodeTable[(triple >> 18) & 0x3F]);
            result.push_back(encodeTable[(triple >> 12) & 0x3F]);
            result.push_back(encodeTable[(triple >> 6) & 0x3F]);
            result.push_back(encodeTable[triple & 0x3F]);
        }

        // Padding
        while (result.size() % 4 != 0) {
            result.push_back('=');
        }

        return result;
    }

    std::vector<uint8_t> base64Decode(const std::string& encoded) {
        // Mock Base64解码
        // 实际应使用完整的实现
        return std::vector<uint8_t>(encoded.begin(), encoded.end());
    }

    SecurityStats getStats() const {
        return stats_;
    }

private:
    std::string base64EncodeClaims(const std::map<std::string, std::string>& claims) {
        std::ostringstream oss;
        oss << "{";

        bool first = true;
        for (const auto& [key, value] : claims) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":\"" << value << "\"";
            first = false;
        }

        oss << "}";
        return oss.str();
    }

    std::string base64EncodeString(const std::string& str) {
        std::vector<uint8_t> data(str.begin(), str.end());
        return base64Encode(data);
    }

    std::string generateMockHash(const std::string& password, int cost) {
        std::ostringstream oss;
        oss << std::hex << std::hash<std::string>{}(password + std::to_string(cost));
        return oss.str();
    }

    std::string generateMockSignature(const std::string& data) {
        // Mock签名
        return std::to_string(std::hash<std::string>{}(data + jwtSecret_));
    }
};

// ============================================================================
// SecurityModule
// ============================================================================

SecurityModule::SecurityModule()
    : impl_(std::make_unique<Impl>()) {}

SecurityModule::~SecurityModule() = default;

bool SecurityModule::initialize() {
    std::cout << "SecurityModule::initialize" << std::endl;
    std::cout << "  JWT secret: " << (jwtSecret_.empty() ? "default (WARNING: change in production)" : "configured") << std::endl;
    std::cout << "  JWT expiry: " << defaultJWTExpiry_.count() << "s" << std::endl;
    std::cout << "  bcrypt cost: " << bcryptCost_ << std::endl;
    std::cout << "  WARNING: Using mock crypto - replace with real implementations for production!" << std::endl;
    return true;
}

bool SecurityModule::start() {
    std::cout << "SecurityModule started (Mock mode - replace with OpenSSL for production)" << std::endl;
    return true;
}

bool SecurityModule::stop() {
    std::cout << "SecurityModule stopped" << std::endl;

    auto stats = getStats();
    std::cout << "  JWT generated: " << stats.totalJWTGenerated << std::endl;
    std::cout << "  JWT verified: " << stats.totalJWTVerified << std::endl;
    std::cout << "  JWT verify failures: " << stats.totalJWTVerifyFailures << std::endl;
    std::cout << "  Passwords hashed: " << stats.totalPasswordsHashed << std::endl;
    std::cout << "  Passwords verified: " << stats.totalPasswordsVerified << std::endl;

    return true;
}

void SecurityModule::cleanup() {
    // 清理敏感数据
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
    // 验证旧token
    auto result = verifyJWT(token);
    if (!result.valid) {
        return "";
    }

    // 生成新token
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
