/**
 * @file test_time_tools.cpp
 * @brief TimeTools 单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>
#include <thread>
#include <chrono>

using namespace PaperCrawler::Core;

/**
 * @test 获取当前时间测试
 */
TEST(TimeToolsTest, GetCurrentTime) {
    int64_t now = TimeTools::now();
    int64_t nowSec = TimeTools::nowSeconds();
    int64_t nowMicro = TimeTools::nowMicroseconds();
    int64_t nowNano = TimeTools::nowNanoseconds();

    // 验证时间值合理
    EXPECT_GT(now, 0);
    EXPECT_GT(nowSec, 0);
    EXPECT_GT(nowMicro, 0);
    EXPECT_GT(nowNano, 0);

    // 验证时间单位转换正确
    EXPECT_NEAR(now / 1000, nowSec, 1);
    EXPECT_NEAR(nowMicro / 1000, now, 1000);
    EXPECT_NEAR(nowNano / 1000, nowMicro, 1000);
}

/**
 * @test 时间格式化测试
 */
TEST(TimeToolsTest, FormatTime) {
    int64_t timestamp = 1712123456789;  // 2024-04-03 15:30:45

    std::string formatted = TimeTools::format("%Y-%m-%d %H:%M:%S", timestamp);

    // 验证格式化包含正确的部分
    EXPECT_TRUE(formatted.find("2024") != std::string::npos ||
                formatted.find("04") != std::string::npos);
}

/**
 * @test ISO 8601格式测试
 */
TEST(TimeToolsTest, ISO8601Format) {
    int64_t timestamp = 1712123456789;

    std::string iso = TimeTools::toISO8601(timestamp);

    // ISO 8601格式应该有特定的结构
    EXPECT_EQ(iso.length(), 28);  // "2024-04-03T15:30:45.789Z"
    EXPECT_EQ(iso[10], 'T');
    EXPECT_EQ(iso[23], '.');
    EXPECT_EQ(iso[27], 'Z');
}

/**
 * @test ISO 8601解析测试
 */
TEST(TimeToolsTest, ISO8601Parsing) {
    std::string iso = "2024-04-03T15:30:45.123Z";

    int64_t timestamp = TimeTools::fromISO8601(iso);

    EXPECT_GT(timestamp, 0);

    // 转换回ISO应该相同（忽略毫秒可能的小差异）
    std::string iso2 = TimeTools::toISO8601(timestamp);
    EXPECT_EQ(iso.substr(0, 19), iso2.substr(0, 19));
}

/**
 * @test 性能计时测试
 */
TEST(TimeToolsTest, PerformanceTiming) {
    auto timer = TimeTools::startTimer();

    // 睡眠50ms
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    int64_t elapsedMs = TimeTools::elapsedMilliseconds(timer);
    int64_t elapsedMicro = TimeTools::elapsedMicroseconds(timer);

    // 验证时间精度
    EXPECT_GE(elapsedMs, 45);  // 至少45ms（允许误差）
    EXPECT_LE(elapsedMs, 100); // 最多100ms

    EXPECT_GE(elapsedMicro, 45000);  // 微秒应该更大
}

/**
 * @test 纳秒级计时测试
 */
TEST(TimeToolsTest, NanosecondTiming) {
    auto timer = TimeTools::startTimer();

    // 做一些工作
    volatile int sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += i;
    }

    int64_t elapsedNs = TimeTools::elapsedNanoseconds(timer);

    // 应该有一些纳秒级的时间
    EXPECT_GT(elapsedNs, 0);
}

/**
 * @test 时间单位转换测试
 */
TEST(TimeToolsTest, TimeUnitConversion) {
    // 秒转毫秒
    int64_t ms = TimeTools::convert(5, TimeUnit::SECONDS, TimeUnit::MILLISECONDS);
    EXPECT_EQ(ms, 5000);

    // 分钟转秒
    int64_t sec = TimeTools::convert(3, TimeUnit::MINUTES, TimeUnit::SECONDS);
    EXPECT_EQ(sec, 180);

    // 小时转分钟
    int64_t min = TimeTools::convert(2, TimeUnit::HOURS, TimeUnit::MINUTES);
    EXPECT_EQ(min, 120);

    // 天转小时
    int64_t hours = TimeTools::convert(1, TimeUnit::DAYS, TimeUnit::HOURS);
    EXPECT_EQ(hours, 24);
}

/**
 * @test 人类可读时间测试
 */
TEST(TimeToolsTest, HumanReadableTime) {
    // 2天3小时45分钟30秒 = 183450000毫秒
    int64_t time1 = 183450000;
    std::string human1 = TimeTools::toHumanReadable(time1);

    EXPECT_TRUE(human1.find("2 day") != std::string::npos ||
                human1.find("2 days") != std::string::npos);
    EXPECT_TRUE(human1.find("3 hour") != std::string::npos ||
                human1.find("3 hours") != std::string::npos);

    // 测试短格式
    std::string short1 = TimeTools::toShortHumanReadable(time1);
    EXPECT_TRUE(short1.find("2d") != std::string::npos);
    EXPECT_TRUE(short1.find("3h") != std::string::npos);
}

/**
 * @test 时间比较测试
 */
TEST(TimeToolsTest, TimeComparison) {
    int64_t past = TimeTools::now() - 1000;  // 1秒前
    int64_t future = TimeTools::now() + 1000;  // 1秒后

    EXPECT_TRUE(TimeTools::isPast(past));
    EXPECT_FALSE(TimeTools::isPast(future));

    EXPECT_TRUE(TimeTools::isFuture(future));
    EXPECT_FALSE(TimeTools::isFuture(past));

    // 范围检查
    int64_t start = TimeTools::now();
    int64_t end = start + 1000;
    int64_t inRange = start + 500;
    int64_t outRange = end + 500;

    EXPECT_TRUE(TimeTools::isInRange(inRange, start, end));
    EXPECT_FALSE(TimeTools::isInRange(outRange, start, end));
}

/**
 * @test 时间加法测试
 */
TEST(TimeToolsTest, TimeAddition) {
    int64_t now = TimeTools::now();

    // 加1天
    int64_t tomorrow = TimeTools::add(now, 1, TimeUnit::DAYS);
    EXPECT_NEAR(tomorrow - now, 86400000, 1000);  // 允许1秒误差

    // 加1小时
    int64_t nextHour = TimeTools::add(now, 1, TimeUnit::HOURS);
    EXPECT_NEAR(nextHour - now, 3600000, 100);  // 允许100ms误差

    // 加1分钟
    int64_t nextMinute = TimeTools::add(now, 1, TimeUnit::MINUTES);
    EXPECT_NEAR(nextMinute - now, 60000, 50);
}

/**
 * @test 时间差值测试
 */
TEST(TimeToolsTest, TimeDifference) {
    int64_t ts1 = TimeTools::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    int64_t ts2 = TimeTools::now();

    // 计算差值（秒）
    int64_t diffSec = TimeTools::diff(ts1, ts2, TimeUnit::SECONDS);

    EXPECT_GE(diffSec, 0);
    EXPECT_LE(diffSec, 1);  // 应该小于1秒
}

/**
 * @test TimeStamp转换测试
 */
TEST(TimeToolsTest, TimestampConversion) {
    int64_t timestamp = 1712123456789;

    // 转换为time_t
    std::time_t tt = TimeTools::toTimeT(timestamp);
    EXPECT_EQ(tt, 1712123456);

    // 转换回来
    int64_t converted = TimeTools::fromTimeT(tt);
    EXPECT_EQ(converted, 1712123456000);  // 毫秒精度
}

/**
 * @test 时间格式化本地时间测试
 */
TEST(TimeToolsTest, FormatLocalTime) {
    // 只是验证不会崩溃
    std::string local = TimeTools::formatLocal("%Y-%m-%d %H:%M:%S");
    EXPECT_FALSE(local.empty());
}

/**
 * @test 高精度时间获取测试
 */
TEST(TimeToolsTest, HighPrecisionTime) {
    int64_t now = TimeTools::now();
    int64_t nowMicro = TimeTools::nowMicroseconds();
    int64_t nowNano = TimeTools::nowNanoseconds();

    // 验证精度递增
    EXPECT_LE(now * 1000, nowMicro + 1000);  // 允许1ms误差
    EXPECT_LE(nowMicro * 1000, nowNano + 1000000);  // 允许1ms误差
}

/**
 * @test 时间常量测试
 */
TEST(TimeToolsTest, TimeConstants) {
    EXPECT_EQ(TimeTools::ONE_DAY_MS, 86400000);
    EXPECT_EQ(TimeTools::ONE_HOUR_MS, 3600000);
    EXPECT_EQ(TimeTools::ONE_MINUTE_MS, 60000);
    EXPECT_EQ(TimeTools::ONE_SECOND_MS, 1000);
}

/**
 * @test 时间大小测试
 */
TEST(TimeToolsTest, TimeSize) {
    // Timestamp类型大小
    EXPECT_EQ(sizeof(TimeTools::Timestamp), sizeof(int64_t));

    // TimePoint大小
    EXPECT_GT(sizeof(TimeTools::TimePoint), 0);
}

/**
 * @test 周期任务取消测试
 */
TEST(TimeToolsTest, PeriodicTask) {
    ThreadPoolConfig config;
    config.initialThreads = 2;

    ThreadPool pool(config);

    std::atomic<int> counter{0};

    // 创建周期任务（虽然无法直接取消，但验证功能）
    size_t taskId = pool.scheduleAtFixedRate([&]() {
        counter++;
    }, std::chrono::milliseconds(50));

    EXPECT_GT(taskId, 0);

    // 让它运行一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_GT(counter.load(), 2);
}

/**
 * @test 大小字节转换测试
 */
TEST(TimeToolsTest, BytesToHuman) {
    EXPECT_EQ(TimeTools::bytesToHuman(1024), "1.00 KB");
    EXPECT_EQ(TimeTools::bytesToHuman(1048576), "1.00 MB");
    EXPECT_EQ(TimeTools::bytesToHuman(1073741824), "1.00 GB");
    EXPECT_EQ(TimeTools::bytesToHuman(1099511627776), "1.00 TB");
}

/**
 * @test 定时执行测试
 */
TEST(TimeToolsTest, ScheduleAt) {
    ThreadPoolConfig config;
    config.initialThreads = 2;

    ThreadPool pool(config);

    std::atomic<bool> executed{false};

    auto now = std::chrono::system_clock::now();
    auto after = now + std::chrono::milliseconds(100);

    pool.scheduleAt([&]() {
        executed = true;
    }, after);

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_TRUE(executed);
}
