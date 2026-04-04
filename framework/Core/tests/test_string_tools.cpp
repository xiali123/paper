/**
 * @file test_string_tools.cpp
 * @brief StringTools 单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>

using namespace PaperCrawler::Core;

/**
 * @test 字符串修剪测试
 */
TEST(StringToolsTest, Trim) {
    EXPECT_EQ(StringTools::trim("  hello  "), "hello");
    EXPECT_EQ(StringTools::trim("\t\nhello\r\n"), "hello");
    EXPECT_EQ(StringTools::trim("hello"), "hello");
    EXPECT_EQ(StringTools::trim(""), "");
    EXPECT_EQ(StringTools::trim("   "), "");
}

/**
 * @test 字符串分割测试
 */
TEST(StringToolsTest, Split) {
    auto parts = StringTools::split("a,b,c", ",");

    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

/**
 * @test 字符串连接测试
 */
TEST(StringToolsTest, Join) {
    std::vector<std::string> parts = {"a", "b", "c"};

    EXPECT_EQ(StringTools::join(parts, ","), "a,b,c");
    EXPECT_EQ(StringTools::join(parts, "-"), "a-b-c");
    EXPECT_EQ(StringTools::join(parts, ""), "abc");
}

/**
 * @test 大小写转换测试
 */
TEST(StringToolsTest, CaseConversion) {
    EXPECT_EQ(StringTools::toUpper("hello"), "HELLO");
    EXPECT_EQ(StringTools::toUpper("HELLO"), "HELLO");
    EXPECT_EQ(StringTools::toLower("HELLO"), "hello");
    EXPECT_EQ(StringTools::toLower("hello"), "hello");
    EXPECT_EQ(StringTools::capitalize("hello"), "Hello");
}

/**
 * @test 命名格式转换测试
 */
TEST(StringToolsTest, NamingConversion) {
    EXPECT_EQ(StringTools::toCamelCase("hello_world"), "helloWorld");
    EXPECT_EQ(StringTools::toCamelCase("hello-world"), "helloWorld");
    EXPECT_EQ(StringTools::toPascalCase("hello_world"), "HelloWorld");
    EXPECT_EQ(StringTools::toSnakeCase("helloWorld"), "hello_world");
    EXPECT_EQ(StringTools::toKebabCase("helloWorld"), "hello-world");
}

/**
 * @test 字符串替换测试
 */
TEST(StringToolsTest, Replace) {
    EXPECT_EQ(StringTools::replaceAll("hello world", "world", "there"), "hello there");
    EXPECT_EQ(StringTools::replaceAll("aaa", "a", "b"), "bbb");
    EXPECT_EQ(StringTools::replaceAll("hello", "x", "y"), "hello");
}

/**
 * @test 字符串验证测试
 */
TEST(StringToolsTest, Validation) {
    EXPECT_TRUE(StringTools::isEmpty(""));
    EXPECT_TRUE(StringTools::isEmpty("   "));
    EXPECT_FALSE(StringTools::isEmpty("hello"));

    EXPECT_TRUE(StringTools::isNumeric("123"));
    EXPECT_TRUE(StringTools::isNumeric("-123"));
    EXPECT_TRUE(StringTools::isNumeric("3.14"));
    EXPECT_FALSE(StringTools::isNumeric("abc"));
    EXPECT_FALSE(StringTools::isNumeric(""));

    EXPECT_TRUE(StringTools::isAlpha("hello"));
    EXPECT_FALSE(StringTools::isAlpha("hello123"));

    EXPECT_TRUE(StringTools::isAlphanumeric("hello123"));
    EXPECT_FALSE(StringTools::isAlphanumeric("hello_123"));
}

/**
 * @test 字符串前缀后缀测试
 */
TEST(StringToolsTest, PrefixSuffix) {
    EXPECT_TRUE(StringTools::startsWith("hello world", "hello"));
    EXPECT_FALSE(StringTools::startsWith("hello world", "world"));

    EXPECT_TRUE(StringTools::endsWith("hello world", "world"));
    EXPECT_FALSE(StringTools::endsWith("hello world", "hello"));

    EXPECT_TRUE(StringTools::contains("hello world", "lo wo"));
    EXPECT_FALSE(StringTools::contains("hello world", "xyz"));
}

/**
 * @test 字符串格式化测试
 */
TEST(StringToolsTest, Format) {
    EXPECT_EQ(StringTools::format("User: {}, Age: {}", "Alice", 25), "User: Alice, Age: 25");
    EXPECT_EQ(StringTools::format("Pi: {:.2f}", 3.14159), "Pi: 3.14");
}

/**
 * @test 随机字符串生成测试
 */
TEST(StringToolsTest, RandomGeneration) {
    auto random = StringTools::generateRandom(16);
    EXPECT_EQ(random.length(), 16);

    auto uuid = StringTools::generateUUID();
    EXPECT_EQ(uuid.length(), 36);  // 格式: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    EXPECT_EQ(uuid[8], '-');
    EXPECT_EQ(uuid[13], '-');
    EXPECT_EQ(uuid[18], '-');
    EXPECT_EQ(uuid[23], '-');
}

/**
 * @test Base64编码测试
 */
TEST(StringToolsTest, Base64Encoding) {
    std::string original = "Hello, World!";
    std::string encoded = StringTools::base64Encode(original);
    std::string decoded = StringTools::base64Decode(encoded);

    EXPECT_EQ(original, decoded);
}

/**
 * @test URL编码测试
 */
TEST(StringToolsTest, URLEncoding) {
    std::string original = "hello world!";
    std::string encoded = StringTools::urlEncode(original);
    std::string decoded = StringTools::urlDecode(encoded);

    EXPECT_EQ(original, decoded);
}

/**
 * @test 字符串反转测试
 */
TEST(StringToolsTest, Reverse) {
    EXPECT_EQ(StringTools::reverse("hello"), "olleh");
    EXPECT_EQ(StringTools::reverse("a"), "a");
    EXPECT_EQ(StringTools::reverse(""), "");
}

/**
 * @test 子串操作测试
 */
TEST(StringToolsTest, SubstringOperations) {
    EXPECT_EQ(StringTools::left("hello", 2), "he");
    EXPECT_EQ(StringTools::left("hello", 10), "hello");

    EXPECT_EQ(StringTools::right("hello", 2), "lo");
    EXPECT_EQ(StringTools::right("hello", 10), "hello");
}

/**
 * @test 正则表达式测试
 */
TEST(StringToolsTest, Regex) {
    EXPECT_TRUE(StringTools::matches("hello123", "[a-z]+[0-9]+"));
    EXPECT_FALSE(StringTools::matches("hello", "[0-9]+"));

    auto emails = StringTools::extract("test@example.com and admin@test.org", R"([\w\.]+@[\w\.]+)");
    EXPECT_EQ(emails.size(), 2);
    EXPECT_EQ(emails[0], "test@example.com");
    EXPECT_EQ(emails[1], "admin@test.org");
}
