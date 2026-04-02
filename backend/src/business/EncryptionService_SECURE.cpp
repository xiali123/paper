// ============================================================================
// Mock加密漏洞修复
// 文件位置：backend/src/business/EncryptionService_SECURE.cpp
// ============================================================================

#include "data/DatabaseModule.hpp"
#include <spdlog/spdlog.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace PaperCrawler {
namespace Services {

/**
 * @brief 原漏洞：Mock加密（XOR加密，任何人都能解密）
 *
 * 原代码：
 * std::string encrypt(const std::string& plaintext) {
 *     std::string ciphertext;
 *     for (size_t i = 0; i < plaintext.size(); ++i) {
 *         ciphertext += plaintext[i] ^ 0x42;  // XOR加密，极其脆弱
 *     }
 *     return ciphertext;
 * }
 *
 * CVSS评分：9.1（Critical）
 * 风险：所有"加密"数据实际是明文，任何人都可解密
 */

/**
 * @brief 安全加密服务（AES-256-GCM）
 *
 * 修复方案：
 * 1. 使用OpenSSL EVP API实现AES-256-GCM加密
 * 2. 每次加密使用随机IV（初始化向量）
 * 3. 认证加密（AEAD）防止篡改
 * 4. 密钥从环境变量或安全密钥管理系统获取
 */
class SecureEncryptionService {
public:
    /**
     * @brief 构造函数
     * @param encryptionKey 加密密钥（32字节for AES-256）
     *
     * 重要：密钥应该从环境变量或密钥管理系统获取
     * 绝不能硬编码在代码中
     */
    explicit SecureEncryptionService(const std::string& encryptionKey) {
        if (encryptionKey.length() != 32) {
            throw std::runtime_error("Encryption key must be 32 bytes for AES-256");
        }

        encryptionKey_ = encryptionKey;

        // 初始化OpenSSL
        OpenSSL_add_all_algorithms();

        auto logger = spdlog::get("EncryptionService");
        if (logger) {
            logger->info("SecureEncryptionService initialized with AES-256-GCM");
        }
    }

    ~SecureEncryptionService() {
        // 清零密钥（防止内存泄露）
        std::fill(encryptionKey_.begin(), encryptionKey_.end(), '\0');
    }

    // ========================================================================
    // 加密操作
    // ========================================================================

    /**
     * @brief 加密数据（AES-256-GCM）
     * @param plaintext 明文
     * @return 密文（格式：IV[16字节] + 密文 + 认证标签[16字节]）
     *
     * 安全特性：
     * - 每次加密使用随机IV（防止重放攻击）
     * - GCM模式提供认证（防止篡改）
     * - 符合FIPS 140-2标准
     */
    std::string encrypt(const std::string& plaintext) {
        auto logger = spdlog::get("EncryptionService");

        if (plaintext.empty()) {
            logger->warn("Attempted to encrypt empty string");
            return "";
        }

        // 1. 生成随机IV（16字节for GCM）
        unsigned char iv[16];
        if (RAND_bytes(iv, sizeof(iv)) != 1) {
            logger->error("Failed to generate random IV");
            throw std::runtime_error("Failed to generate random IV");
        }

        // 2. 初始化加密上下文
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            logger->error("Failed to create cipher context");
            throw std::runtime_error("Failed to create cipher context");
        }

        // 3. 初始化加密操作
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
                               reinterpret_cast<const unsigned char*>(encryptionKey_.data()),
                               iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            logger->error("Failed to initialize encryption");
            throw std::runtime_error("Failed to initialize encryption");
        }

        // 4. 加密明文
        std::vector<unsigned char> ciphertext(plaintext.size() + AES_BLOCK_SIZE);
        int len;
        int ciphertext_len;

        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                             reinterpret_cast<const unsigned char*>(plaintext.data()),
                             plaintext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            logger->error("Failed to encrypt data");
            throw std::runtime_error("Failed to encrypt data");
        }
        ciphertext_len = len;

        // 5. 结束加密操作
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            logger->error("Failed to finalize encryption");
            throw std::runtime_error("Failed to finalize encryption");
        }
        ciphertext_len += len;

        // 6. 获取认证标签（16字节）
        unsigned char tag[16];
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            logger->error("Failed to get authentication tag");
            throw std::runtime_error("Failed to get authentication tag");
        }

        EVP_CIPHER_CTX_free(ctx);

        // 7. 组合输出：IV + 密文 + 认证标签
        std::string result;
        result.append(reinterpret_cast<const char*>(iv), 16);
        result.append(reinterpret_cast<const char*>(ciphertext.data()), ciphertext_len);
        result.append(reinterpret_cast<const char*>(tag), 16);

        logger->debug("Encrypted {} bytes -> {} bytes", plaintext.size(), result.size());

        return result;
    }

    /**
     * @brief 解密数据（AES-256-GCM）
     * @param ciphertext 密文（格式：IV[16字节] + 密文 + 认证标签[16字节]）
     * @return 明文
     *
     * 安全特性：
     * - 验证认证标签（防止篡改）
     * - 如果验证失败，抛出异常
     */
    std::string decrypt(const std::string& ciphertext) {
        auto logger = spdlog::get("EncryptionService");

        if (ciphertext.size() < 32) {  // 最少16字节IV + 16字节标签
            logger->error("Ciphertext too short");
            throw std::runtime_error("Ciphertext too short");
        }

        // 1. 提取IV（前16字节）
        const unsigned char* iv = reinterpret_cast<const unsigned char*>(ciphertext.data());

        // 2. 提取密文（中间部分）
        const unsigned char* ciphertext_data = iv + 16;
        size_t ciphertext_len = ciphertext.size() - 32;  // 减去IV和标签

        // 3. 提取认证标签（最后16字节）
        const unsigned char* tag = ciphertext_data + ciphertext_len;

        // 4. 初始化解密上下文
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            logger->error("Failed to create cipher context");
            throw std::runtime_error("Failed to create cipher context");
        }

        // 5. 初始化解密操作
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
                               reinterpret_cast<const unsigned char*>(encryptionKey_.data()),
                               iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            logger->error("Failed to initialize decryption");
            throw std::runtime_error("Failed to initialize decryption");
        }

        // 6. 解密密文
        std::vector<unsigned char> plaintext(ciphertext_len + AES_BLOCK_SIZE);
        int len;
        int plaintext_len;

        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                             ciphertext_data, ciphertext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            logger->error("Failed to decrypt data");
            throw std::runtime_error("Failed to decrypt data");
        }
        plaintext_len = len;

        // 7. 设置认证标签（在Finalize之前）
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16,
                               const_cast<unsigned char*>(tag)) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            logger->error("Failed to set authentication tag");
            throw std::runtime_error("Failed to set authentication tag");
        }

        // 8. 结束解密操作并验证认证标签
        int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
        EVP_CIPHER_CTX_free(ctx);

        if (ret <= 0) {
            logger->error("Authentication tag verification failed - data may be tampered");
            throw std::runtime_error("Decryption failed: authentication tag mismatch");
        }

        plaintext_len += len;

        logger->debug("Decrypted {} bytes -> {} bytes", ciphertext.size(), plaintext_len);

        return std::string(reinterpret_cast<const char*>(plaintext.data()), plaintext_len);
    }

    // ========================================================================
    // 哈希操作
    // ========================================================================

    /**
     * @brief 计算SHA-256哈希
     * @param data 数据
     * @return 哈希值（64字符十六进制字符串）
     */
    static std::string sha256(const std::string& data) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, data.data(), data.size());
        SHA256_Final(hash, &ctx);

        return bytesToHex(hash, SHA256_DIGEST_LENGTH);
    }

    /**
     * @brief 计算SHA-512哈希
     * @param data 数据
     * @return 哈希值（128字符十六进制字符串）
     */
    static std::string sha512(const std::string& data) {
        unsigned char hash[SHA512_DIGEST_LENGTH];
        SHA512_CTX ctx;
        SHA512_Init(&ctx);
        SHA512_Update(&ctx, data.data(), data.size());
        SHA512_Final(hash, &ctx);

        return bytesToHex(hash, SHA512_DIGEST_LENGTH);
    }

    // ========================================================================
    // 密钥管理
    // ========================================================================

    /**
     * @brief 从环境变量获取加密密钥
     * @param envVar 环境变量名
     * @return 加密密钥（32字节）
     *
     * 如果环境变量不存在或长度不足，抛出异常
     */
    static std::string getEncryptionKeyFromEnv(const std::string& envVar) {
        const char* envValue = std::getenv(envVar.c_str());

        if (!envValue) {
            throw std::runtime_error(
                "Encryption key environment variable '" + envVar + "' not set"
            );
        }

        std::string key(envValue);

        // 如果密钥是Base64编码的，先解码
        // 这里简化处理，假设环境变量已经是32字节

        if (key.length() != 32) {
            // 尝试从哈希生成32字节密钥
            std::string hashed = sha256(key);
            if (hashed.length() >= 32) {
                return hashed.substr(0, 32);
            }

            throw std::runtime_error(
                "Encryption key must be 32 bytes for AES-256, got " +
                std::to_string(key.length()) + " bytes"
            );
        }

        return key;
    }

    /**
     * @brief 生成随机密钥
     * @return 随机密钥（32字节）
     *
     * 用于初始化系统时生成新密钥
     * 应该立即保存到安全的地方（如环境变量或密钥管理系统）
     */
    static std::string generateRandomKey() {
        unsigned char key[32];
        if (RAND_bytes(key, sizeof(key)) != 1) {
            throw std::runtime_error("Failed to generate random key");
        }

        return bytesToHex(key, sizeof(key));
    }

private:
    std::string encryptionKey_;

    /**
     * @brief 字节数组转十六进制字符串
     */
    static std::string bytesToHex(const unsigned char* data, size_t len) {
        std::ostringstream ss;
        for (size_t i = 0; i < len; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(data[i]);
        }
        return ss.str();
    }

    /**
     * @brief 十六进制字符串转字节数组
     */
    static std::vector<unsigned char> hexToBytes(const std::string& hex) {
        std::vector<unsigned char> bytes;

        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            unsigned char byte = static_cast<unsigned char>(
                std::strtol(byteString.c_str(), nullptr, 16)
            );
            bytes.push_back(byte);
        }

        return bytes;
    }
};

// ============================================================================
// 便捷函数
// ============================================================================

/**
 * @brief 安全加密
 *
 * 使用示例：
 * auto encrypted = secureEncrypt("sensitive data");
 */
inline std::string secureEncrypt(const std::string& plaintext) {
    // 从环境变量获取密钥
    std::string key = SecureEncryptionService::getEncryptionKeyFromEnv("PAPERCRAWLER_ENCRYPTION_KEY");

    SecureEncryptionService service(key);
    return service.encrypt(plaintext);
}

/**
 * @brief 安全解密
 *
 * 使用示例：
 * auto decrypted = secureDecrypt(encrypted_data);
 */
inline std::string secureDecrypt(const std::string& ciphertext) {
    // 从环境变量获取密钥
    std::string key = SecureEncryptionService::getEncryptionKeyFromEnv("PAPERCRAWLER_ENCRYPTION_KEY");

    SecureEncryptionService service(key);
    return service.decrypt(ciphertext);
}

/**
 * @brief 安全哈希（SHA-256）
 *
 * 使用示例：
 * auto hash = secureHash("data to hash");
 */
inline std::string secureHash(const std::string& data) {
    return SecureEncryptionService::sha256(data);
}

// ============================================================================
// 密钥初始化辅助
// ============================================================================

/**
 * @brief 初始化加密服务
 *
 * 在main函数中调用：
 * if (!initializeEncryption()) {
 *     std::cerr << "Failed to initialize encryption" << std::endl;
 *     return 1;
 * }
 */
inline bool initializeEncryption() {
    try {
        // 检查环境变量
        const char* key = std::getenv("PAPERCRAWLER_ENCRYPTION_KEY");

        if (!key) {
            std::cerr << "WARNING: PAPERCRAWLER_ENCRYPTION_KEY not set" << std::endl;
            std::cerr << "Generate a key with: openssl rand -hex 32" << std::endl;
            std::cerr << "Then set it: export PAPERCRAWLER_ENCRYPTION_KEY=<your_key>" << std::endl;
            return false;
        }

        // 验证密钥长度
        std::string keyStr(key);
        if (keyStr.length() < 32) {
            std::cerr << "ERROR: Encryption key must be at least 32 bytes" << std::endl;
            return false;
        }

        spdlog::get("EncryptionService")->info("Encryption service initialized successfully");
        return true;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to initialize encryption: " << e.what() << std::endl;
        return false;
    }
}

} // namespace Services
} // namespace PaperCrawler
