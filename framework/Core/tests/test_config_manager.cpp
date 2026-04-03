/**
 * @file test_config_manager.cpp
 * @brief ConfigManager 单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>
#include <fstream>
#include <cstdio>

using namespace PaperCrawler::Core;

/**
 * @test 配置设置和获取测试
 */
TEST(ConfigManagerTest, SetAndGet) {
    auto& config = ConfigManager::getInstance();
    config.clear();

    config.set("string.key", "value");
    config.setInt("int.key", 42);
    config.setDouble("double.key", 3.14);
    config.setBool("bool.key", true);

    EXPECT_EQ(config.getString("string.key"), "value");
    EXPECT_EQ(config.getInt("int.key"), 42);
    EXPECT_DOUBLE_EQ(config.getDouble("double.key"), 3.14);
    EXPECT_TRUE(config.getBool("bool.key"));
}

/**
 * @test 配置默认值测试
 */
TEST(ConfigManagerTest, DefaultValues) {
    auto& config = ConfigManager::getInstance();
    config.clear();

    EXPECT_EQ(config.getString("nonexistent"), "");
    EXPECT_EQ(config.getString("nonexistent", "default"), "default");

    EXPECT_EQ(config.getInt("nonexistent"), 0);
    EXPECT_EQ(config.getInt("nonexistent", 100), 100);

    EXPECT_DOUBLE_EQ(config.getDouble("nonexistent"), 0.0);
    EXPECT_NEAR(config.getDouble("nonexistent", 2.5), 2.5, 0.001);

    EXPECT_FALSE(config.getBool("nonexistent"));
    EXPECT_TRUE(config.getBool("nonexistent", true));
}

/**
 * @test 配置存在检查测试
 */
TEST(ConfigManagerTest, HasKey) {
    auto& config = ConfigManager::getInstance();
    config.clear();

    config.set("existing.key", "value");

    EXPECT_TRUE(config.has("existing.key"));
    EXPECT_FALSE(config.has("nonexistent.key"));
}

/**
 * @test 配置更新测试
 */
TEST(ConfigManagerTest, UpdateValue) {
    auto& config = ConfigManager::getInstance();
    config.clear();

    config.set("key", "value1");
    EXPECT_EQ(config.getString("key"), "value1");

    config.set("key", "value2");
    EXPECT_EQ(config.getString("key"), "value2");
}

/**
 * @test 嵌套键测试
 */
TEST(ConfigManagerTest, NestedKeys) {
    auto& config = ConfigManager::getInstance();
    config.clear();

    config.set("database.host", "localhost");
    config.set("database.port", "3306");
    config.set("database.user", "root");

    EXPECT_EQ(config.getString("database.host"), "localhost");
    EXPECT_EQ(config.getInt("database.port"), 3306);
    EXPECT_EQ(config.getString("database.user"), "root");
}

/**
 * @test 配置验证测试
 */
TEST(ConfigManagerTest, Validation) {
    auto& config = ConfigManager::getInstance();
    config.clear();

    // 设置必需的配置
    config.set("database.host", "localhost");
    config.set("database.port", "3306");

    // 注册验证器
    config.registerValidator([](const auto& cfg) {
        ValidationResult result;
        if (!cfg.count("database.host")) {
            result.addError("database.host is required");
        }
        if (cfg.count("database.port")) {
            int port = std::stoi(cfg.at("database.port"));
            if (port < 1 || port > 65535) {
                result.addError("database.port must be 1-65535");
            }
        }
        return result;
    });

    // 验证配置
    auto result = config.validate();
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.errors.size(), 0);
}

/**
 * @test 配置验证失败测试
 */
TEST(ConfigManagerTest, ValidationFailure) {
    auto& config = ConfigManager::getInstance();
    config.clear();

    // 设置无效的端口
    config.set("database.port", "99999");

    // 注册验证器
    config.registerValidator([](const auto& cfg) {
        ValidationResult result;
        if (cfg.count("database.port")) {
            int port = std::stoi(cfg.at("database.port"));
            if (port < 1 || port > 65535) {
                result.addError("database.port must be 1-65535");
            }
        }
        return result;
    });

    // 验证配置
    auto result = config.validate();
    EXPECT_FALSE(result.valid);
    EXPECT_GT(result.errors.size(), 0);
}

/**
 * @test 配置监听测试
 */
TEST(ConfigManagerTest, WatchConfig) {
    auto& config = ConfigManager::getInstance();
    config.clear();

    bool callbackCalled = false;
    std::string oldValue;
    std::string newValue;

    // 监听配置变更
    std::string watchId = config.watch("test.key",
        [&](const std::string& key, const std::string& oldVal, const std::string& newVal) {
            callbackCalled = true;
            oldValue = oldVal;
            newValue = newVal;
        }
    );

    // 修改配置
    config.set("test.key", "value1");
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(newValue, "value1");

    // 取消监听
    callbackCalled = false;
    config.unwatch(watchId);
    config.set("test.key", "value2");
    EXPECT_FALSE(callbackCalled);
}

/**
 * @test 配置清空测试
 */
TEST(ConfigManagerTest, Clear) {
    auto& config = ConfigManager::getInstance();

    config.set("key1", "value1");
    config.set("key2", "value2");

    EXPECT_TRUE(config.has("key1"));
    EXPECT_TRUE(config.has("key2"));

    config.clear();

    EXPECT_FALSE(config.has("key1"));
    EXPECT_FALSE(config.has("key2"));
}

/**
 * @test 单例模式测试
 */
TEST(ConfigManagerTest, Singleton) {
    auto& config1 = ConfigManager::getInstance();
    auto& config2 = ConfigManager::getInstance();

    EXPECT_EQ(&config1, &config2);
}

/**
 * @test 环境变量加载测试
 */
TEST(ConfigManagerTest, LoadFromEnvironment) {
    auto& config = ConfigManager::getInstance();
    config.clear();

    // 设置测试环境变量
#ifdef _WIN32
    _putenv("TEST_VAR=test_value");
#else
    setenv("TEST_VAR", "test_value", 1);
#endif

    // 加载环境变量
    config.loadFromEnvironment("TEST_");

    // 验证（注意：环境变量名会被转换）
    EXPECT_TRUE(config.has("TEST_VAR") || config.has("TEST_VAR"));
}

/**
 * @test 线程安全测试
 */
TEST(ConfigManagerTest, ThreadSafety) {
    auto& config = ConfigManager::getInstance();
    config.clear();

    std::vector<std::thread> threads;

    // 多线程同时读写配置
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 100; ++j) {
                config.set("key." + std::to_string(i) + "." + std::to_string(j), "value");
                config.getString("key." + std::to_string(i) + "." + std::to_string(j));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 验证没有崩溃或死锁
    SUCCEED();
}

/**
 * @test 全局配置访问器测试
 */
TEST(ConfigManagerTest, GlobalConfigAccessor) {
    auto& config = ConfigManager::getInstance();
    config.clear();

    config.set("test.key", "test.value");

    // 使用全局Config访问器
    EXPECT_EQ(Config::getString("test.key"), "test.value");
    EXPECT_EQ(Config::getInt("nonexistent", 42), 42);
    EXPECT_TRUE(Config::has("test.key"));
}
