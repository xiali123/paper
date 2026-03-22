#!/bin/bash

# Quick Authentication System Verification

echo "🔍 PaperCrawler 认证系统验证"
echo "================================"
echo ""

PROJECT_ROOT="e:/PaperCrawler"
PASS=0
FAIL=0

check_file() {
    if [ -f "$1" ]; then
        echo "✓ $2"
        ((PASS++))
    else
        echo "✗ $2 (缺失)"
        ((FAIL++))
    fi
}

check_content() {
    if grep -q "$2" "$1" 2>/dev/null; then
        echo "✓ $3"
        ((PASS++))
    else
        echo "✗ $3 (缺失)"
        ((FAIL++))
    fi
}

echo "📁 数据库层"
check_file "$PROJECT_ROOT/backend/migrations/002_add_authentication.sql" "MySQL schema"
check_file "$PROJECT_ROOT/backend/migrations/002_add_authentication_sqlite.sql" "SQLite schema"
echo ""

echo "🔧 后端头文件"
check_file "$PROJECT_ROOT/include/auth/AuthManager.hpp" "AuthManager.hpp"
check_file "$PROJECT_ROOT/include/auth/JwtUtils.hpp" "JwtUtils.hpp"
check_file "$PROJECT_ROOT/include/auth/PasswordHasher.hpp" "PasswordHasher.hpp"
check_file "$PROJECT_ROOT/include/auth/RateLimiter.hpp" "RateLimiter.hpp"
echo ""

echo "⚙️ 后端实现"
check_file "$PROJECT_ROOT/backend/src/auth_handlers.cpp" "Auth handlers"
check_file "$PROJECT_ROOT/backend/src/auth_middleware.cpp" "Auth middleware"
echo ""

echo "🌐 前端层"
check_file "$PROJECT_ROOT/frontend/src/api/modules/auth.ts" "Auth API"
check_file "$PROJECT_ROOT/frontend/src/stores/auth.ts" "Auth store"
check_file "$PROJECT_ROOT/frontend/src/utils/request.ts" "Request (已更新)"
check_file "$PROJECT_ROOT/frontend/src/router/guards.ts" "Route guards"
check_file "$PROJECT_ROOT/frontend/src/views/Login.vue" "Login page"
check_file "$PROJECT_ROOT/frontend/src/views/Register.vue" "Register page"
echo ""

echo "🖥️ 桌面客户端"
check_file "$PROJECT_ROOT/desktop/include/AuthManager.hpp" "Desktop AuthManager"
check_file "$PROJECT_ROOT/desktop/include/LoginWindow.hpp" "LoginWindow"
echo ""

echo "🧪 测试文件"
check_file "$PROJECT_ROOT/backend/tests/test_auth.cpp" "Backend tests"
check_file "$PROJECT_ROOT/frontend/src/stores/__tests__/auth.test.ts" "Frontend tests"
echo ""

echo "📖 文档"
check_file "$PROJECT_ROOT/AUTHENTICATION_GUIDE.md" "使用指南"
check_file "$PROJECT_ROOT/AUTHENTICATION_IMPLEMENTATION_SUMMARY.md" "实现总结"
echo ""

echo "🔍 内容验证"
check_content "$PROJECT_ROOT/include/auth/AuthManager.hpp" "registerUser" "注册方法"
check_content "$PROJECT_ROOT/include/auth/AuthManager.hpp" "AuthResult login" "登录方法"
check_content "$PROJECT_ROOT/backend/migrations/002_add_authentication.sql" "CREATE TABLE users" "用户表"
check_content "$PROJECT_ROOT/backend/migrations/002_add_authentication.sql" "CREATE TABLE user_sessions" "会话表"
check_content "$PROJECT_ROOT/frontend/src/stores/auth.ts" "async function login" "登录 action"
check_content "$PROJECT_ROOT/frontend/src/views/Login.vue" "template" "登录模板"
echo ""

echo "================================"
echo "✅ 通过: $PASS"
echo "❌ 失败: $FAIL"
echo "总计: $((PASS + FAIL))"
echo "成功率: $((PASS * 100 / (PASS + FAIL)))%"
echo ""

if [ $FAIL -eq 0 ]; then
    echo "🎉 所有组件验证通过！"
    echo ""
    echo "📋 下一步操作："
    echo ""
    echo "1️⃣  创建测试数据库："
    echo "   cd $PROJECT_ROOT/backend"
    echo "   sqlite3 papercrawler.db < migrations/002_add_authentication_sqlite.sql"
    echo ""
    echo "2️⃣  检查前端："
    echo "   cd $PROJECT_ROOT/frontend"
    echo "   npm run type-check  # 检查 TypeScript"
    echo ""
    echo "3️⃣  查看文档："
    echo "   cat $PROJECT_ROOT/AUTHENTICATION_GUIDE.md"
else
    echo "⚠️  部分组件缺失，请检查上述列表"
fi
