#!/bin/bash
# ============================================================================
# PaperCrawler Backend - 一键启动脚本 (Linux)
# ============================================================================
# 用法: ./start-server.sh [端口号]
#   默认端口: 8080
# ============================================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PORT="${1:-8080}"

# --stop 参数支持
if [ "$1" = "--stop" ] || [ "$1" = "-s" ]; then
    echo -e "${YELLOW}停止 PaperCrawler 服务器...${NC}"
    if pgrep -f "PaperCrawlerServerHotPlug" > /dev/null 2>&1; then
        pkill -f "PaperCrawlerServerHotPlug" 2>/dev/null || true
        sleep 1
        echo -e "${GREEN}已停止${NC}"
    else
        echo -e "${GREEN}未运行${NC}"
    fi
    exit 0
fi
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/.."

# 如果从 build/ 目录运行
if [ -f "${SCRIPT_DIR}/PaperCrawlerServerHotPlug" ]; then
    BUILD_DIR="${SCRIPT_DIR}"
fi

SERVER_BIN="${BUILD_DIR}/PaperCrawlerServerHotPlug"
MODULES_DIR="${BUILD_DIR}/modules"
CONFIG_DIR="${BUILD_DIR}/config"
LOG_FILE="${BUILD_DIR}/server.log"

# ============================================================================
# 1. 停止旧进程
# ============================================================================
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  PaperCrawler Backend v2.0.0${NC}"
echo -e "${BLUE}========================================${NC}"

if pgrep -f "PaperCrawlerServerHotPlug" > /dev/null 2>&1; then
    echo -e "${YELLOW}[1/4] 停止旧进程...${NC}"
    # 用 PID 文件方式避免杀掉自身
    OLD_PIDS=$(pgrep -f "PaperCrawlerServerHotPlug" 2>/dev/null || true)
    for pid in $OLD_PIDS; do
        # 不杀当前脚本的进程组
        kill -9 "$pid" 2>/dev/null || true
    done
    sleep 2

    # 等待端口释放
    for i in $(seq 1 10); do
        if ! ss -tlnp 2>/dev/null | grep -q ":${PORT} "; then
            break
        fi
        sleep 1
    done
    echo -e "${GREEN}  旧进程已停止${NC}"
else
    echo -e "${GREEN}[1/4] 无运行中的进程${NC}"
fi

# 检查端口是否被占用
if ss -tlnp 2>/dev/null | grep -q ":${PORT} "; then
    echo -e "${RED}错误: 端口 ${PORT} 已被占用${NC}"
    ss -tlnp 2>/dev/null | grep ":${PORT} "
    exit 1
fi

# ============================================================================
# 2. 检查环境
# ============================================================================
echo -e "${YELLOW}[2/4] 检查环境...${NC}"

if [ ! -f "${SERVER_BIN}" ]; then
    echo -e "${RED}错误: 找不到可执行文件 ${SERVER_BIN}${NC}"
    echo -e "${YELLOW}请先编译: cd backend && mkdir -p build && cd build && cmake .. && make -j\$(nproc)${NC}"
    exit 1
fi
echo -e "${GREEN}  可执行文件: ${SERVER_BIN}${NC}"

if [ ! -d "${MODULES_DIR}" ]; then
    echo -e "${RED}错误: 找不到模块目录 ${MODULES_DIR}${NC}"
    exit 1
fi
MODULE_COUNT=$(ls "${MODULES_DIR}"/lib*.so 2>/dev/null | wc -l)
echo -e "${GREEN}  模块数量: ${MODULE_COUNT}${NC}"

if [ ! -f "${CONFIG_DIR}/modules.json" ]; then
    echo -e "${YELLOW}  警告: 找不到 config/modules.json，模块将不会自动加载${NC}"
fi

# ============================================================================
# 3. 启动服务器
# ============================================================================
echo -e "${YELLOW}[3/4] 启动服务器 (端口: ${PORT})...${NC}"

export LD_LIBRARY_PATH="${MODULES_DIR}:${LD_LIBRARY_PATH:-}"

# 启动并获取PID
cd "${BUILD_DIR}"
nohup "${SERVER_BIN}" > "${LOG_FILE}" 2>&1 &
SERVER_PID=$!

# 等待启动
sleep 3

# 检查进程是否存活
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${RED}错误: 服务器启动失败${NC}"
    echo -e "${YELLOW}查看日志: tail -20 ${LOG_FILE}${NC}"
    tail -20 "${LOG_FILE}"
    exit 1
fi

# 检查端口是否在监听
for i in $(seq 1 5); do
    if ss -tlnp 2>/dev/null | grep -q ":${PORT} "; then
        break
    fi
    sleep 1
done

if ! ss -tlnp 2>/dev/null | grep -q ":${PORT} "; then
    echo -e "${YELLOW}  警告: 端口 ${PORT} 未监听，检查日志...${NC}"
    tail -10 "${LOG_FILE}"
    exit 1
fi

echo -e "${GREEN}  服务器已启动 (PID: ${SERVER_PID})${NC}"

# ============================================================================
# 4. 验证
# ============================================================================
echo -e "${YELLOW}[4/4] 验证服务...${NC}"

HEALTH=$(curl -s --max-time 3 "http://localhost:${PORT}/api/health" 2>/dev/null || echo "")
if [ -z "$HEALTH" ]; then
    echo -e "${RED}错误: 健康检查失败${NC}"
    echo -e "${YELLOW}查看日志: tail -50 ${LOG_FILE}${NC}"
    exit 1
fi

# 统计模块状态
HEALTHY=$(echo "$HEALTH" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('summary',{}).get('healthy',0))" 2>/dev/null || echo "?")
TOTAL=$(echo "$HEALTH" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('summary',{}).get('total',0))" 2>/dev/null || echo "?")

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  启动成功!${NC}"
echo -e "${GREEN}========================================${NC}"
echo -e "  PID:        ${SERVER_PID}"
echo -e "  端口:        ${PORT}"
echo -e "  模块:        ${HEALTHY}/${TOTAL} healthy"
echo -e "  健康检查:    http://localhost:${PORT}/api/health"
echo -e "  模块列表:    http://localhost:${PORT}/api/modules"
echo -e "  系统信息:    http://localhost:${PORT}/api/system/info"
echo -e "  日志文件:    ${LOG_FILE}"
echo ""
echo -e "  停止命令:    ./start-server.sh --stop  或  pkill -f PaperCrawlerServerHotPlug"
echo -e "  查看日志:    tail -f ${LOG_FILE}"
echo ""
