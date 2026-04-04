/**
 * @file test_type_helper.cpp
 * @brief TypeHelper 单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>
#include <memory>
#include <vector>

using namespace PaperCrawler::Core;

/**
 * @test 类型名称获取测试
 */
TEST(TypeHelperTest, GetTypeName) {
    // 基本类型
    EXPECT_EQ(TypeHelper::getTypeName<int>(), "int");
    EXPECT_EQ(TypeHelper::getTypeName<double>(), "double");
    EXPECT_EQ(TypeHelper::getTypeName<std::string>(), "std::string");

    // 指针类型
    EXPECT_EQ(TypeHelper::getTypeName<int*>(), "int*");
    EXPECT_EQ(TypeHelper::getTypeName<std::string*>(), "std::string*");

    // 容器类型
    using IntVector = std::vector<int>;
    std::string vectorName = TypeHelper::getTypeName<IntVector>();
    EXPECT_TRUE(vectorName.find("vector") != std::string::npos);
}

/**
 * @test 从对象获取类型名测试
 */
TEST(TypeHelperTest, GetTypeNameFromObject) {
    int i = 42;
    EXPECT_EQ(TypeHelper::getTypeName(i), "int");

    std::string s = "hello";
    EXPECT_EQ(TypeHelper::getTypeName(s), "std::string");

    std::vector<int> v;
    std::string vectorName = TypeHelper::getTypeName(v);
    EXPECT_TRUE(vectorName.find("vector") != std::string::npos);
}

/**
 * @test 类型大小测试
 */
TEST(TypeHelperTest, GetTypeSize) {
    EXPECT_EQ(TypeHelper::getSize<int>(), sizeof(int));
    EXPECT_EQ(TypeHelper::getSize<double>(), sizeof(double));
    EXPECT_EQ(TypeHelper::getSize<char>(), sizeof(char));

    // 指针大小（平台相关）
    EXPECT_EQ(TypeHelper::getSize<int*>(), sizeof(int*));
}

/**
 * @test 类型对齐测试
 */
TEST(TypeHelperTest, GetTypeAlignment) {
    EXPECT_EQ(TypeHelper::getAlignment<int>(), alignof(int));
    EXPECT_EQ(TypeHelper::getAlignment<double>(), alignof(double));

    // 对齐应该大于0
    EXPECT_GT(TypeHelper::getAlignment<int>(), 0);
}

/**
 * @test 类型转字符串测试
 */
TEST(TypeHelperTest, ToString) {
    EXPECT_EQ(TypeHelper::toString(42), "42");
    EXPECT_EQ(TypeHelper::toString(3.14), "3.14");  // 或 "3.140000"取决于实现
    EXPECT_EQ(TypeHelper::toString(true), "true");
    EXPECT_EQ(TypeHelper::toString(false), "false");

    EXPECT_EQ(TypeHelper::toString(std::string("hello")), "hello");
}

/**
 * @test 字符串转类型测试
 */
TEST(TypeHelperTest, FromString) {
    EXPECT_EQ(TypeHelper::fromString<int>("123"), 123);
    EXPECT_FLOAT_EQ(TypeHelper::fromString<double>("3.14"), 3.14);
    EXPECT_TRUE(TypeHelper::fromString<bool>("true"));
    EXPECT_FALSE(TypeHelper::fromString<bool>("false"));

    EXPECT_EQ(TypeHelper::fromString<std::string>("hello"), "hello");
}

/**
 * @test isInteger类型特征测试
 */
TEST(TypeHelperTest, TypeTraitsIntegral) {
    EXPECT_TRUE(TypeHelper::isIntegral<int>::value);
    EXPECT_TRUE(TypeHelper::isIntegral<char>::value);
    EXPECT_TRUE(TypeHelper::isIntegral<bool>::value);

    EXPECT_FALSE(TypeHelper::isIntegral<float>::value);
    EXPECT_FALSE(TypeHelper::isIntegral<double>::value);
}

/**
 * @test isFloatingPoint类型特征测试
 */
TEST(TypeHelperTest, TypeTraitsFloatingPoint) {
    EXPECT_TRUE(TypeHelper::isFloatingPoint<float>::value);
    EXPECT_TRUE(TypeHelper::isFloatingPoint<double>::value);

    EXPECT_FALSE(TypeHelper::isFloatingPoint<int>::value);
}

/**
 * @test isPointer类型特征测试
 */
TEST(TypeHelperTest, TypeTraitsPointer) {
    EXPECT_TRUE(TypeHelper::isPointer<int*>::value);
    EXPECT_TRUE(TypeHelper::isPointer<std::string*>::value);
    EXPECT_TRUE(TypeHelper::isPointer<int**>::value);

    EXPECT_FALSE(TypeHelper::isPointer<int>::value);
    EXPECT_FALSE(TypeHelper::isPointer<int&>::value);
}

/**
 * @test 类型移除修饰符测试
 */
TEST(TypeHelperTest, TypeModifiers) {
    // 移除const
    EXPECT_TRUE((std::is_same<
        TypeHelper::removeConst<const int>,
        int
    >::value));

    // 移除引用
    EXPECT_TRUE((std::is_same<
        TypeHelper::removeReference<int&>,
        int
    >::value));

    // 移除指针
    EXPECT_TRUE((std::is_same<
        TypeHelper::removePointer<int*>,
        int
    >::value));
}

/**
 * @test 智能指针创建测试
 */
TEST(TypeHelperTest, MakeSharedPtr) {
    auto ptr = TypeHelper::makeShared<std::vector<int>>(5, 10);

    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->size(), 5);
    EXPECT_EQ((*ptr)[0], 10);
}

/**
 * @test MakeUnique测试
 */
TEST(TypeHelperTest, MakeUniquePtr) {
    auto ptr = TypeHelper::makeUnique<std::vector<int>>(3, 20);

    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->size(), 3);
    EXPECT_EQ((*ptr)[0], 20);
}

/**
 * @test TypeList基础测试
 */
TEST(TypeHelperTest, TypeListSize) {
    using MyTypes = TypeHelper::TypeList<int, double, std::string>;

    EXPECT_EQ(MyTypes::size, 3);
}

/**
 * @test TypeList获取类型测试
 */
TEST(TypeHelperTest, TypeListGetType) {
    using MyTypes = TypeHelper::TypeList<int, double, std::string>;

    using First = MyTypes::get<0>;
    using Second = MyTypes::get<1>;
    using Third = MyTypes::get<2>;

    EXPECT_TRUE((std::is_same<First, int>::value));
    EXPECT_TRUE((std::is_same<Second, double>::value));
    EXPECT_TRUE((std::is_same<Third, std::string>::value));
}

/**
 * @test TypeList包含测试
 */
TEST(TypeHelperTest, TypeListContains) {
    using MyTypes = TypeHelper::TypeList<int, double, std::string>;

    EXPECT_TRUE(MyTypes::contains<int>::value);
    EXPECT_TRUE(MyTypes::contains<double>::value);
    EXPECT_TRUE(MyTypes::contains<std::string>::value);
    EXPECT_FALSE(MyTypes::contains<float>::value);
}

/**
 * @test TypeList追加测试
 */
TEST(TypeHelperTest, TypeListAppend) {
    using MyTypes = TypeHelper::TypeList<int, double>;
    using Appended = MyTypes::append<float, char>;

    EXPECT_EQ(Appended::size, 4);
    EXPECT_TRUE(Appended::contains<float>::value);
    EXPECT_TRUE(Appended::contains<char>::value);
}

/**
 * @test 容器类型检测测试
 */
TEST(TypeHelperTest, ContainerDetection) {
    EXPECT_TRUE(TypeHelper::isVector<std::vector<int>>::value);
    EXPECT_TRUE(TypeHelper::isMap<std::map<int, std::string>>::value);
    EXPECT_TRUE(TypeHelper::isSet<std::set<int>>::value);

    EXPECT_FALSE(TypeHelper::isVector<std::list<int>>::value);
    EXPECT_FALSE(TypeHelper::isMap<std::unordered_map<int, std::string>>::value);
}

/**
 * @test 智能指针检测测试
 */
TEST(TypeHelperTest, SmartPointerDetection) {
    EXPECT_TRUE(TypeHelper::isSharedPtr<std::shared_ptr<int>>::value);
    EXPECT_TRUE(TypeHelper::isUniquePtr<std::unique_ptr<int>>::value);
    EXPECT_TRUE(TypeHelper::isWeakPtr<std::weak_ptr<int>>::value);

    EXPECT_FALSE(TypeHelper::isSharedPtr<int*>::value);
}

/**
 * @test Pair检测测试
 */
TEST(TypeHelperTest, PairDetection) {
    EXPECT_TRUE(TypeHelper::isPair<std::pair<int, double>>::value);
    EXPECT_FALSE(TypeHelper::isPair<std::tuple<int, double>>::value);
}

/**
 * @test Tuple检测测试
 */
TEST(TypeHelperTest, TupleDetection) {
    EXPECT_TRUE(TypeHelper::isTuple<std::tuple<int, double>>::value);
    EXPECT_FALSE(TypeHelper::isTuple<std::pair<int, double>>::value);
}

/**
 * @test 类型比较测试
 */
TEST(TypeHelperTest, TypeComparison) {
    EXPECT_TRUE(TypeHelper::isSame<int, int>::value);
    EXPECT_FALSE(TypeHelper::isSame<int, double>::value);

    EXPECT_TRUE(TypeHelper::isConvertible<int, double>::value);
    EXPECT_FALSE(TypeHelper::isConvertible<double, int>::value);
}

/**
 * @test Any类型擦除测试
 */
TEST(TypeHelperTest, AnyType) {
    TypeHelper::Any any1 = 42;
    TypeHelper::Any any2 = std::string("hello");
    TypeHelper::Any any3 = 3.14;

    EXPECT_TRUE(any1.isType<int>());
    EXPECT_TRUE(any2.isType<std::string>());
    EXPECT_TRUE(any3.isType<double>());

    EXPECT_EQ(any1.cast<int>(), 42);
    EXPECT_EQ(any2.cast<std::string>(), "hello");
    EXPECT_DOUBLE_EQ(any3.cast<double>(), 3.14);
}

/**
 * @test Any类型擦除异常测试
 */
TEST(TypeHelperTest, AnyTypeErrors) {
    TypeHelper::Any any = 42;

    EXPECT_FALSE(any.isType<std::string>());
    EXPECT_THROW(any.cast<std::string>(), std::bad_cast);
}

/**
 * @test 条件类型选择测试
 */
TEST(TypeHelperTest, ConditionalType) {
    using TrueCase = TypeHelper::conditional<true, int, double>;
    using FalseCase = TypeHelper::conditional<false, int, double>;

    EXPECT_TRUE((std::is_same<TrueCase, int>::value));
    EXPECT_TRUE((std::is_same<FalseCase, double>::value));
}
