#!/bin/bash
# ============================================================================
# PaperCrawler Frontend - 一键启动脚本 (Linux)
# ============================================================================
# 用法: ./start-frontend.sh [选项]
#   默认: 启动开发服务器
#   --stop  停止前端服务
#   --build 构建生产版本
# ============================================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FRONTEND_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PORT=4200

# --stop 参数
if [ "$1" = "--stop" ] || [ "$1" = "-s" ]; then
    echo -e "${YELLOW}停止 PaperCrawler 前端服务...${NC}"
    if pgrep -f "vite.*${FRONTEND_DIR}" > /dev/null 2>&1; then
        pkill -f "vite.*${FRONTEND_DIR}" 2>/dev/null || true
        sleep 1
        echo -e "${GREEN}已停止${NC}"
    else
        # 尝试通过端口查找
        VITE_PID=$(lsof -ti :${PORT} 2>/dev/null || true)
        if [ -n "$VITE_PID" ]; then
            kill $VITE_PID 2>/dev/null || true
            sleep 1
            echo -e "${GREEN}已停止 (PID: ${VITE_PID})${NC}"
        else
            echo -e "${GREEN}未运行${NC}"
        fi
    fi
    exit 0
fi

# --build 参数
if [ "$1" = "--build" ] || [ "$1" = "-b" ]; then
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  PaperCrawler Frontend - 生产构建${NC}"
    echo -e "${BLUE}========================================${NC}"

    cd "${FRONTEND_DIR}"

    # 检查 node_modules
    if [ ! -d "node_modules" ]; then
        echo -e "${YELLOW}[1/2] 安装依赖...${NC}"
        npm install
    else
        echo -e "${GREEN}[1/2] 依赖已安装${NC}"
    fi

    echo -e "${YELLOW}[2/2] 构建生产版本...${NC}"
    npm run build

    if [ -d "dist" ]; then
        echo ""
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}  构建成功!${NC}"
        echo -e "${GREEN}========================================${NC}"
        echo -e "  输出目录: ${FRONTEND_DIR}/dist"
        echo ""
        echo -e "  预览命令: npm run preview"
        echo -e "  部署: 将 dist/ 目录部署到静态服务器"
        echo ""
    fi
    exit 0
fi

# ============================================================================
# 开发模式启动
# ============================================================================
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  PaperCrawler Frontend v1.0.0${NC}"
echo -e "${BLUE}========================================${NC}"

# 停止旧进程
if pgrep -f "vite" > /dev/null 2>&1; then
    echo -e "${YELLOW}[1/3] 停止旧进程...${NC}"
    pkill -f "vite" 2>/dev/null || true
    sleep 1
    echo -e "${GREEN}  旧进程已停止${NC}"
else
    echo -e "${GREEN}[1/3] 无运行中的进程${NC}"
fi

cd "${FRONTEND_DIR}"

# 检查 node_modules
echo -e "${YELLOW}[2/3] 检查环境...${NC}"
if [ ! -d "node_modules" ]; then
    echo -e "${YELLOW}  首次运行，安装依赖...${NC}"
    npm install
    echo -e "${GREEN}  依赖安装完成${NC}"
else
    echo -e "${GREEN}  依赖已就绪${NC}"
fi

# 检查后端是否运行
if ! ss -tlnp 2>/dev/null | grep -q ":8080 "; then
    echo -e "${YELLOW}  警告: 后端未运行 (端口 8080)，API 代理将不可用"
    echo -e "${YELLOW}  启动后端: cd backend/build && ../scripts/start-server.sh"
fi

# 清理 Vite 缓存
rm -rf node_modules/.vite 2>/dev/null

# 启动开发服务器
echo -e "${YELLOW}[3/3] 启动开发服务器...${NC}"
echo ""

nohup npx vite --host > /tmp/vite-frontend.log 2>&1 &
VITE_PID=$!
sleep 3

# 检查是否启动成功
if ! kill -0 $VITE_PID 2>/dev/null; then
    echo -e "${RED}错误: 启动失败${NC}"
    echo -e "${YELLOW}查看日志: tail -20 /tmp/vite-frontend.log${NC}"
    cat /tmp/vite-frontend.log
    exit 1
fi

if ! ss -tlnp 2>/dev/null | grep -q ":${PORT} "; then
    echo -e "${RED}错误: 端口 ${PORT} 未监听${NC}"
    cat /tmp/vite-frontend.log
    exit 1
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  启动成功!${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "  PID:        ${VITE_PID}"
echo -e "  本地访问:   http://localhost:${PORT}"
echo -e "  网络访问:   http://$(hostname -I | awk '{print $1}'):${PORT}"
echo -e "  后端代理:   /api -> http://localhost:8080"
echo -e "  日志文件:   /tmp/vite-frontend.log"
echo ""
echo -e "  停止命令:   ./start-frontend.sh --stop"
echo -e "  查看日志:   tail -f /tmp/vite-frontend.log"
echo ""
