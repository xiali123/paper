#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <map>
#include <nlohmann/json.hpp>

namespace PaperCrawler {

enum class UserRole {
    ADMIN,
    USER,
    GUEST
};

enum class UserStatus {
    ACTIVE,
    INACTIVE,
    SUSPENDED,
    PENDING
};

inline std::string userRoleToString(UserRole role) {
    switch (role) {
        case UserRole::ADMIN: return "admin";
        case UserRole::USER: return "user";
        case UserRole::GUEST: return "guest";
    }
    return "user";
}

inline UserRole stringToUserRole(const std::string& s) {
    if (s == "admin") return UserRole::ADMIN;
    if (s == "guest") return UserRole::GUEST;
    return UserRole::USER;
}

struct User {
    int id = 0;
    std::string username;
    std::string email;
    std::string fullName;
    std::string passwordHash;
    std::string avatar;
    std::string bio;
    UserRole role{UserRole::USER};
    UserStatus status{UserStatus::ACTIVE};
    std::string createdAt;
    std::string updatedAt;
    std::string lastLoginAt;
    std::vector<std::string> preferences;

    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"username", username},
            {"email", email},
            {"fullName", fullName},
            {"avatar", avatar},
            {"bio", bio},
            {"role", userRoleToString(role)},
            {"status", status == UserStatus::ACTIVE ? "active" :
                       status == UserStatus::INACTIVE ? "inactive" :
                       status == UserStatus::SUSPENDED ? "suspended" : "pending"},
            {"createdAt", createdAt},
            {"lastLoginAt", lastLoginAt}
        };
    }

    static User fromDbRow(const std::map<std::string, std::string>& row) {
        User user;
        user.id = std::stoi(row.at("id"));
        user.username = row.count("username") ? row.at("username") : "";
        user.email = row.count("email") ? row.at("email") : "";
        user.fullName = row.count("full_name") ? row.at("full_name") :
                        row.count("fullName") ? row.at("fullName") : "";
        user.passwordHash = row.count("password_hash") ? row.at("password_hash") : "";
        user.avatar = row.count("avatar") ? row.at("avatar") : "";
        user.bio = row.count("bio") ? row.at("bio") : "";
        user.role = row.count("role") ? stringToUserRole(row.at("role")) : UserRole::USER;
        user.status = row.count("status") ? (row.at("status") == "active" ? UserStatus::ACTIVE :
                      row.at("status") == "suspended" ? UserStatus::SUSPENDED :
                      row.at("status") == "pending" ? UserStatus::PENDING :
                      UserStatus::INACTIVE) : UserStatus::ACTIVE;
        user.createdAt = row.count("created_at") ? row.at("created_at") : "";
        user.updatedAt = row.count("updated_at") ? row.at("updated_at") : "";
        user.lastLoginAt = row.count("last_login_at") ? row.at("last_login_at") : "";
        return user;
    }
};

struct UserStats {
    uint64_t totalUsers{0};
    uint64_t activeUsers{0};
    uint64_t inactiveUsers{0};
    uint64_t suspendedUsers{0};
    uint64_t adminCount{0};
    uint64_t userCount{0};
    uint64_t guestCount{0};
};

} // namespace PaperCrawler
