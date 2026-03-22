#!/bin/bash

echo "=========================================="
echo "PaperCrawler Mock API + Frontend"
echo "=========================================="
echo ""

# 检查 Node.js 是否安装
if ! command -v node &> /dev/null; then
    echo "错误: Node.js 未安装"
    echo "请先安装 Node.js: https://nodejs.org/"
    exit 1
fi

# 检查依赖
if [ ! -d "node_modules" ] || [ ! -d "node_modules/express" ]; then
    echo "安装依赖..."
    npm install express cors
fi

echo "启动 Mock API 服务器..."
echo "API 地址: http://127.0.0.1:8080"
echo ""

# 启动 Mock API
node mock-auth-api.js &

API_PID=$!
echo "Mock API PID: $API_PID"
echo ""

# 等待 API 启动
sleep 2

echo "=========================================="
echo "Mock API 已启动"
echo "=========================================="
echo ""
echo "现在启动前端..."
echo ""

cd frontend
npm run dev

# 当前端停止时，也停止 API
kill $API_PID
