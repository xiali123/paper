#!/bin/bash
# MySQL连接测试程序编译脚本

echo "=== 编译MySQL连接测试程序 ==="

# 查找MySQL库
MYSQL_CONFIG=$(which mysql_config 2>/dev/null)
if [ -z "$MYSQL_CONFIG" ]; then
    echo "⚠️  未找到mysql_config，尝试使用默认路径"
    MYSQL_CFLAGS=""
    MYSQL_LIBS="-lmysqlclient"
else
    MYSQL_CFLAGS=$($MYSQL_CONFIG --cflags)
    MYSQL_LIBS=$($MYSQL_CONFIG --libs)
fi

echo "MySQL CFLAGS: $MYSQL_CFLAGS"
echo "MySQL LIBS: $MYSQL_LIBS"

# 编译测试程序
g++ -std=c++17 \
    $MYSQL_CFLAGS \
    -I./include \
    -I./core/external \
    test_mysql_connection.cpp \
    $MYSQL_LIBS \
    -o test_mysql_connection

if [ $? -eq 0 ]; then
    echo "✅ 编译成功！"
    echo "运行测试程序："
    ./test_mysql_connection
else
    echo "❌ 编译失败"
    exit 1
fi
