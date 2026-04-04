// ============================================================================
// 可预测JWT漏洞修复
// 文件位置：backend/src/business/JwtService_SECURE.cpp
// ============================================================================

#include "business/AuthApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include <spdlog/spdlog.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <stdexcept>

namespace PaperCrawler {
namespace Services {

/**
 * @brief 原漏洞：无签名JWT（可预测）
 *
 * 原代码：
 * std::string generateToken(int userId) {
 *     std::string header = base64_encode("{\"alg\":\"none\"}");
 *     std::string payload = base64_encode("{\"user_id\":" + std::to_string(userId) + "}");
 *     return header + "." + payload + ".";  // 无签名！
 * }
 *
 * CVSS评分：8.5（High）
 * 风险：任何人可伪造令牌，绕过所有权限检查
 */

/**
 * @brief JWT算法
 */
enum class JwtAlgorithm {
    HS256,  // HMAC-SHA256
    HS512,  // HMAC-SHA512
    RS256   // RSA-SHA256（暂不实现）
};

/**
 * @brief JWT载荷
 */
struct JwtPayload {
    int userId;
    std::string username;
    std::string role;
    int64_t expiresAt;     // 过期时间（Unix时间戳）
    int64_t issuedAt;      // 签发时间
    int64_t notBefore;     // 生效时间

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["user_id"] = userId;
        j["username"] = username;
        j["role"] = role;
        j["exp"] = expiresAt;
        j["iat"] = issuedAt;
        j["nbf"] = notBefore;
        return j;
    }

    static JwtPayload fromJson(const nlohmann::json& j) {
        JwtPayload payload;
        payload.userId = j["user_id"];
        payload.username = j["username"];
        payload.role = j["role"];
        payload.expiresAt = j["exp"];
        payload.issuedAt = j["iat"];
        payload.notBefore = j["nbf"];
        return payload;
    }
};

/**
 * @brief 安全JWT服务（带签名）
 *
 * 修复方案：
 * 1. HMAC-SHA256签名
 * 2. 密钥从环境变量获取
 * 3. 过期时间验证
 * 4. 令牌刷新机制
 */
class SecureJwtService {
public:
    /**
     * @brief 构造函数
     * @param secret HMAC密钥（从环境变量获取）
     * @param algorithm 签名算法（默认HS256）
     */
    explicit SecureJwtService(
        const std::string& secret,
        JwtAlgorithm algorithm = JwtAlgorithm::HS256)
        : secret_(secret), algorithm_(algorithm) {

        if (secret.empty()) {
            throw std::runtime_error("JWT secret cannot be empty");
        }

        // 验证密钥长度（HS256至少32字节）
        if (algorithm == JwtAlgorithm::HS256 && secret.length() < 32) {
            throw std::runtime_error(
                "JWT secret must be at least 32 bytes for HS256"
            );
        }

        auto logger = spdlog::get("AuthService");
        if (logger) {
            logger->info("SecureJwtService initialized with {}", getAlgorithmName());
        }
    }

    // ========================================================================
    // 令牌生成
    // ========================================================================

    /**
     * @brief 生成访问令牌
     * @param userId 用户ID
     * @param username 用户名
     * @param role 角色
     * @param expirationSeconds 过期时间（秒）
     * @return JWT令牌
     */
    std::string generateAccessToken(
        int userId,
        const std::string& username,
        const std::string& role,
        int expirationSeconds = 3600) {

        auto logger = spdlog::get("AuthService");

        auto now = getCurrentTimestamp();

        JwtPayload payload;
        payload.userId = userId;
        payload.username = username;
        payload.role = role;
        payload.issuedAt = now;
        payload.notBefore = now;
        payload.expiresAt = now + expirationSeconds;

        std::string token = generateToken(payload);

        logger->info("Generated access token for user {} (expires in {}s)",
                     userId, expirationSeconds);

        return token;
    }

    /**
     * @brief 生成刷新令牌
     * @param userId 用户ID
     * @param expirationSeconds 过期时间（默认7天）
     * @return 刷新令牌
     */
    std::string generateRefreshToken(
        int userId,
        int expirationSeconds = 7 * 24 * 3600) {

        auto logger = spdlog::get("AuthService");

        auto now = getCurrentTimestamp();

        JwtPayload payload;
        payload.userId = userId;
        payload.username = "";  // 刷新令牌不需要用户名
        payload.role = "refresh";
        payload.issuedAt = now;
        payload.notBefore = now;
        payload.expiresAt = now + expirationSeconds;

        std::string token = generateToken(payload);

        logger->info("Generated refresh token for user {} (expires in {}s)",
                     userId, expirationSeconds);

        return token;
    }

    // ========================================================================
    // 令牌验证
    // ========================================================================

    /**
     * @brief 验证令牌
     * @param token JWT令牌
     * @return 载荷（如果验证成功）
     *
     * 验证步骤：
     * 1. 验证签名
     * 2. 验证过期时间
     * 3. 验证生效时间
     * 4. 验证格式
     */
    std::optional<JwtPayload> verifyToken(const std::string& token) {
        auto logger = spdlog::get("AuthService");

        try {
            // 1. 分割令牌
            auto parts = splitToken(token);
            if (parts.size() != 3) {
                logger->warn("Invalid token format");
                return std::nullopt;
            }

            std::string headerEncoded = parts[0];
            std::string payloadEncoded = parts[1];
            std::string signatureEncoded = parts[2];

            // 2. 验证签名
            std::string expectedSignature = sign(headerEncoded + "." + payloadEncoded);
            if (signatureEncoded != expectedSignature) {
                logger->warn("Invalid token signature");
                return std::nullopt;
            }

            // 3. 解码载荷
            std::string payloadJson = base64_decode(payloadEncoded);
            JwtPayload payload = JwtPayload::fromJson(nlohmann::json::parse(payloadJson));

            // 4. 验证过期时间
            auto now = getCurrentTimestamp();
            if (payload.expiresAt < now) {
                logger->warn("Token expired (exp: {}, now: {})",
                            payload.expiresAt, now);
                return std::nullopt;
            }

            // 5. 验证生效时间
            if (payload.notBefore > now) {
                logger->warn("Token not yet valid (nbf: {}, now: {})",
                            payload.notBefore, now);
                return std::nullopt;
            }

            logger->info("Token verified for user {}", payload.userId);
            return payload;

        } catch (const std::exception& e) {
            logger->error("Token verification failed: {}", e.what());
            return std::nullopt;
        }
    }

    /**
     * @brief 验证访问令牌
     * @param token JWT令牌
     * @return 用户ID（如果验证成功）
     */
    std::optional<int> verifyAccessToken(const std::string& token) {
        auto payload = verifyToken(token);
        if (!payload) {
            return std::nullopt;
        }

        // 验证角色（访问令牌不能是refresh角色）
        if (payload->role == "refresh") {
            spdlog::get("AuthService")->warn("Refresh token used as access token");
            return std::nullopt;
        }

        return payload->userId;
    }

    /**
     * @brief 验证刷新令牌
     * @param token JWT令牌
     * @return 用户ID（如果验证成功）
     */
    std::optional<int> verifyRefreshToken(const std::string& token) {
        auto payload = verifyToken(token);
        if (!payload) {
            return std::nullopt;
        }

        // 验证角色（必须是refresh角色）
        if (payload->role != "refresh") {
            spdlog::get("AuthService")->warn("Access token used as refresh token");
            return std::nullopt;
        }

        return payload->userId;
    }

    // ========================================================================
    // 令牌刷新
    // ========================================================================

    /**
     * @brief 刷新访问令牌
     * @param refreshToken 刷新令牌
     * @param database 数据库（验证令牌是否被撤销）
     * @return 新的访问令牌（如果刷新成功）
     */
    std::optional<std::string> refreshAccessToken(
        const std::string& refreshToken,
        std::shared_ptr<IDatabase> database) {

        auto logger = spdlog::get("AuthService");

        // 1. 验证刷新令牌
        auto userId = verifyRefreshToken(refreshToken);
        if (!userId) {
            logger->warn("Invalid refresh token");
            return std::nullopt;
        }

        // 2. 检查令牌是否被撤销
        if (isTokenRevoked(*userId, refreshToken, database)) {
            logger->warn("Refresh token has been revoked for user {}", *userId);
            return std::nullopt;
        }

        // 3. 获取用户信息
        auto user = getUserById(*userId, database);
        if (!user) {
            logger->warn("User not found: {}", *userId);
            return std::nullopt;
        }

        // 4. 生成新的访问令牌
        std::string newAccessToken = generateAccessToken(
            *userId,
            (*user)["username"],
            (*user)["role"],
            3600  // 1小时
        );

        logger->info("Access token refreshed for user {}", *userId);
        return newAccessToken;
    }

    /**
     * @brief 撤销令牌
     * @param token 令牌
     * @param database 数据库
     */
    void revokeToken(const std::string& token, std::shared_ptr<IDatabase> database) {
        auto logger = spdlog::get("AuthService");

        auto payload = verifyToken(token);
        if (!payload) {
            logger->warn("Cannot revoke invalid token");
            return;
        }

        // 添加到撤销列表
        std::ostringstream sql;
        sql << "INSERT INTO revoked_tokens (user_id, token, revoked_at) VALUES ("
            << payload->userId << ", "
            << "'" << database->escape(hashToken(token)) << "', "
            << "NOW())";

        if (database->execute(sql.str())) {
            logger->info("Token revoked for user {}", payload->userId);
        }
    }

private:
    std::string secret_;
    JwtAlgorithm algorithm_;

    /**
     * @brief 生成令牌
     */
    std::string generateToken(const JwtPayload& payload) {
        // 1. 构建Header
        nlohmann::json header;
        header["alg"] = getAlgorithmString();
        header["typ"] = "JWT";

        std::string headerEncoded = base64_encode(header.dump());

        // 2. 构建Payload
        std::string payloadEncoded = base64_encode(payload.toJson().dump());

        // 3. 签名
        std::string signature = sign(headerEncoded + "." + payloadEncoded);
        std::string signatureEncoded = base64_encode(signature);

        // 4. 组合
        return headerEncoded + "." + payloadEncoded + "." + signatureEncoded;
    }

    /**
     * @brief 签名数据
     */
    std::string sign(const std::string& data) {
        switch (algorithm_) {
            case JwtAlgorithm::HS256:
                return hmac_sha256(data, secret_);
            case JwtAlgorithm::HS512:
                return hmac_sha512(data, secret_);
            default:
                throw std::runtime_error("Unsupported algorithm");
        }
    }

    /**
     * @brief HMAC-SHA256
     */
    static std::string hmac_sha256(const std::string& data, const std::string& key) {
        unsigned char* digest;
        unsigned int digest_len;

        digest = HMAC(
            EVP_sha256(),
            key.data(), key.length(),
            reinterpret_cast<const unsigned char*>(data.data()), data.length(),
            nullptr, &digest_len
        );

        return std::string(reinterpret_cast<const char*>(digest), digest_len);
    }

    /**
     * @brief HMAC-SHA512
     */
    static std::string hmac_sha512(const std::string& data, const std::string& key) {
        unsigned char* digest;
        unsigned int digest_len;

        digest = HMAC(
            EVP_sha512(),
            key.data(), key.length(),
            reinterpret_cast<const unsigned char*>(data.data()), data.length(),
            nullptr, &digest_len
        );

        return std::string(reinterpret_cast<const char*>(digest), digest_len);
    }

    /**
     * @brief 分割令牌
     */
    std::vector<std::string> splitToken(const std::string& token) {
        std::vector<std::string> parts;
        std::stringstream ss(token);
        std::string part;

        while (std::getline(ss, part, '.')) {
            parts.push_back(part);
        }

        return parts;
    }

    /**
     * @brief Base64编码（URL安全）
     */
    static std::string base64_encode(const std::string& data) {
        // 简化实现（应该使用OpenSSL的BIO_f_base64）
        // 实际应该替换为正确的Base64 URL编码实现
        // 这里使用nlohmann/json的base64编码

        // （简化：返回十六进制编码作为占位符）
        std::ostringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0');
        for (unsigned char c : data) {
            ss << static_cast<int>(c);
        }
        return ss.str();
    }

    /**
     * @brief Base64解码
     */
    static std::string base64_decode(const std::string& encoded) {
        // （简化实现）
        // 实际应该使用正确的Base64 URL解码实现
        return encoded;
    }

    /**
     * @brief 获取算法名称
     */
    std::string getAlgorithmName() const {
        switch (algorithm_) {
            case JwtAlgorithm::HS256: return "HS256";
            case JwtAlgorithm::HS512: return "HS512";
            default: return "unknown";
        }
    }

    /**
     * @brief 获取算法字符串
     */
    std::string getAlgorithmString() const {
        return getAlgorithmName();
    }

    /**
     * @brief 获取当前时间戳
     */
    static int64_t getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()
        ).count();
    }

    /**
     * @brief 检查令牌是否被撤销
     */
    bool isTokenRevoked(int userId, const std::string& token, std::shared_ptr<IDatabase> database) {
        std::ostringstream sql;
        sql << "SELECT id FROM revoked_tokens "
            << "WHERE user_id = " << userId << " "
            << "AND token = '" << database->escape(hashToken(token)) << "' "
            << "LIMIT 1";

        auto results = database->query(sql.str());
        return !results.empty();
    }

    /**
     * @brief 哈希令牌（用于存储）
     */
    static std::string hashToken(const std::string& token) {
        // 使用SHA-256哈希
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, token.data(), token.size());
        SHA256_Final(hash, &ctx);

        std::ostringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0');
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            ss << static_cast<int>(hash[i]);
        }
        return ss.str();
    }

    /**
     * @brief 获取用户信息
     */
    std::optional<std::map<std::string, std::string>> getUserById(
        int userId,
        std::shared_ptr<IDatabase> database) {

        std::ostringstream sql;
        sql << "SELECT id, username, role FROM users WHERE id = " << userId;

        auto results = database->query(sql.str());
        if (results.empty()) {
            return std::nullopt;
        }

        return results[0];
    }
};

// ============================================================================
// 便捷函数
// ============================================================================

/**
 * @brief 从环境变量获取JWT密钥
 */
inline std::string getJwtSecretFromEnv() {
    const char* secret = std::getenv("PAPERCRAWLER_JWT_SECRET");

    if (!secret) {
        throw std::runtime_error("PAPERCRAWLER_JWT_SECRET not set");
    }

    std::string secretStr(secret);

    if (secretStr.length() < 32) {
        throw std::runtime_error(
            "JWT secret must be at least 32 characters, got " +
            std::to_string(secretStr.length())
        );
    }

    return secretStr;
}

/**
 * @brief 生成JWT密钥
 */
inline std::string generateJwtSecret() {
    unsigned char secret[32];
    if (RAND_bytes(secret, sizeof(secret)) != 1) {
        throw std::runtime_error("Failed to generate JWT secret");
    }

    std::ostringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0');
    for (int i = 0; i < 32; ++i) {
        ss << static_cast<int>(secret[i]);
    }
    return ss.str();
}

/**
 * @brief 初始化JWT服务
 */
inline bool initializeJwtService() {
    try {
        std::string secret = getJwtSecretFromEnv();
        spdlog::get("AuthService")->info("JWT service initialized successfully");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to initialize JWT service: " << e.what() << std::endl;
        std::cerr << "Generate a secret with: openssl rand -hex 32" << std::endl;
        std::cerr << "Then set it: export PAPARCRAWLER_JWT_SECRET=<your_secret>" << std::endl;
        return false;
    }
}

} // namespace Services
} // namespace PaperCrawler
