#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <string>
#include <map>
#include <vector>
#include <cstdint>

namespace PaperCrawler {

/**
 * @brief JWT声明
 */
using JWTClaims = std::map<std::string, std::string>;

/**
 * @brief JWT验证结果
 */
struct JWTVerifyResult {
    bool valid{false};
    JWTClaims claims;
    std::string errorMessage;
};

/**
 * @brief 密码哈希结果
 */
struct PasswordHashResult {
    bool success{false};
    std::string hash;
    std::string errorMessage;
};

/**
 * @brief 加密结果
 */
struct EncryptionResult {
    bool success{false};
    std::vector<uint8_t> encryptedData;
    std::string errorMessage;
};

/**
 * @brief 解密结果
 */
struct DecryptionResult {
    bool success{false};
    std::vector<uint8_t> decryptedData;
    std::string errorMessage;
};

/**
 * @brief 安全模块
 *
 * 功能：
 * 1. JWT令牌生成和验证
 * 2. 密码哈希（bcrypt）
 * 3. SSL/TLS管理
 * 4. 加密/解密（AES-256-GCM）
 * 5. 签名验证（HMAC-SHA256）
 *
 * 安全特性：
 * - 密码哈希：bcrypt（自适应成本因子）
 * - JWT：HS256/RS256算法支持
 * - 加密：AES-256-GCM（认证加密）
 * - 密钥管理：内存加密存储
 * - 防重放：时间戳 + nonce
 */
class SecurityModule : public IModule {
public:
    SecurityModule();
    ~SecurityModule() override;

    std::string getName() const override { return "Security"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Security module: JWT, password hashing, encryption";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 生成JWT令牌
     * @param claims JWT声明（如：sub, exp, iat）
     * @param expirySeconds 过期时间（秒）
     * @return JWT令牌字符串
     */
    std::string generateJWT(const JWTClaims& claims,
                           std::chrono::seconds expiry = std::chrono::seconds(3600));

    /**
     * @brief 验证JWT令牌
     */
    JWTVerifyResult verifyJWT(const std::string& token);

    /**
     * @brief 刷新JWT令牌
     */
    std::string refreshJWT(const std::string& token);

    /**
     * @brief 哈希密码
     * @param cost bcrypt成本因子（4-31，默认12）
     */
    PasswordHashResult hashPassword(const std::string& password, int cost = 12);

    /**
     * @brief 验证密码
     */
    bool verifyPassword(const std::string& password, const std::string& hash);

    /**
     * @brief 加密数据（AES-256-GCM）
     * @param key 加密密钥（32字节）
     * @param nonce nonce（12字节）
     */
    EncryptionResult encrypt(const std::vector<uint8_t>& data,
                            const std::vector<uint8_t>& key,
                            const std::vector<uint8_t>& nonce);

    /**
     * @brief 解密数据
     */
    DecryptionResult decrypt(const std::vector<uint8_t>& encryptedData,
                            const std::vector<uint8_t>& key,
                            const std::vector<uint8_t>& nonce);

    /**
     * @brief 生成随机密钥
     */
    std::vector<uint8_t> generateKey(size_t length = 32);

    /**
     * @brief 生成随机nonce
     */
    std::vector<uint8_t> generateNonce(size_t length = 12);

    /**
     * @brief HMAC签名
     */
    std::vector<uint8_t> hmacSign(const std::string& message,
                                  const std::vector<uint8_t>& key);

    /**
     * @brief HMAC验证
     */
    bool hmacVerify(const std::string& message,
                   const std::vector<uint8_t>& signature,
                   const std::vector<uint8_t>& key);

    /**
     * @brief 生成哈希（SHA-256）
     */
    std::vector<uint8_t> sha256(const std::string& data);

    /**
     * @brief 生成哈希（SHA-512）
     */
    std::vector<uint8_t> sha512(const std::string& data);

    /**
     * @brief Base64编码
     */
    std::string base64Encode(const std::vector<uint8_t>& data);

    /**
     * @brief Base64解码
     */
    std::vector<uint8_t> base64Decode(const std::string& encoded);

    /**
     * @brief 设置JWT密钥
     */
    void setJWTSecret(const std::string& secret);

    /**
     * @brief 设置默认过期时间
     */
    voidsetDefaultJWTExpiry(std::chrono::seconds expiry);

    /**
     * @brief 安全统计
     */
    struct SecurityStats {
        uint64_t totalJWTGenerated;
        uint64_t totalJWTVerified;
        uint64_t totalJWTVerifyFailures;
        uint64_t totalPasswordsHashed;
        uint64_t totalPasswordsVerified;
        uint64_t totalEncryptions;
        uint64_t totalDecryptions;
    };
    SecurityStats getStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    std::string jwtSecret_;
    std::chrono::seconds defaultJWTExpiry_{3600};
    int bcryptCost_{12};

    SecurityStats stats_;
};

} // namespace PaperCrawler
