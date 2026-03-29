#!/bin/bash
# PaperCrawler 启动脚本

echo "Starting PaperCrawler servers..."

# 启动后端
cd E:/PaperCrawler/backend/build/Release
if [ ! -f "libmysql.dll" ]; then
    cp "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.dll" .
fi
./PaperCrawlerServer.exe &
BACKEND_PID=$!
echo "Backend started (PID: $BACKEND_PID)"

# 启动前端
cd E:/PaperCrawler/frontend
npm run dev &
FRONTEND_PID=$!
echo "Frontend started (PID: $FRONTEND_PID)"

echo ""
echo "✅ Both servers started!"
echo "Frontend: http://localhost:5173"
echo "Backend:  http://localhost:8080"
echo ""
echo "Press Ctrl+C to stop both servers"

# 等待任一进程退出
wait $BACKEND_PID $FRONTEND_PID
