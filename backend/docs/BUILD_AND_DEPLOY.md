# PaperCrawler 编译和部署指南

**超级功能套件 v2.0**

---

## 📋 前置要求

### 系统要求
- **操作系统**: Windows 10+, Linux (Ubuntu 20.04+), macOS 10.15+
- **编译器**: MSVC 2019+, GCC 9+, Clang 10+
- **CMake**: 3.15+
- **Python**: 3.8+ (用于某些构建脚本)

### 依赖库

#### 核心依赖
```bash
# OpenSSL（加密）
# Windows: vcpkg install openssl
# Linux: sudo apt-get install libssl-dev
# macOS: brew install openssl

# libcurl（HTTP客户端）
# Windows: vcpkg install curl
# Linux: sudo apt-get install libcurl4-openssl-dev
# macOS: brew install curl

# spdlog（日志）
# Windows: vcpkg install spdlog
# Linux: sudo apt-get install libspdlog-dev
# macOS: brew install spdlog

# nlohmann/json（JSON库）
# Windows: vcpkg install nlohmann-json
# Linux: sudo apt-get install nlohmann-json3-dev
# macOS: brew install nlohmann-json
```

#### 数据库依赖
```bash
# MySQL
# Windows: 下载MySQL Installer
# Linux: sudo apt-get install libmysqlclient-dev
# macOS: brew install mysql-client

# Redis（可选，用于L2缓存）
# Windows: 下载Redis for Windows
# Linux: sudo apt-get install redis-server
# macOS: brew install redis
```

#### AI依赖（可选）
```bash
# 本地LLM（如llama.cpp）
# git clone https://github.com/ggerganov/llama.cpp
# cd llama.cpp && make
```

---

## 🔨 编译步骤

### 1. 克隆仓库
```bash
git clone https://github.com/your-org/PaperCrawler.git
cd PaperCrawler/backend
```

### 2. 安装依赖（vcpkg）
```bash
# Windows
vcpkg install openssl curl spdlog nlohmann-json

# Linux/macOS
./vcpkg install openssl curl spdlog nlohmann-json
```

### 3. 配置CMake
```bash
mkdir build
cd build

# Windows (Visual Studio)
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake

# Linux/macOS
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### 4. 编译
```bash
# Windows
cmake --build . --config Release

# Linux/macOS
make -j$(nproc)
```

### 5. 运行测试
```bash
# Windows
ctest --config Release

# Linux/macOS
make test
```

---

## 🗄️ 数据库迁移

### MySQL迁移
```bash
# 连接到MySQL
mysql -u root -p papercrawler

# 执行迁移文件
source backend/migrations/005_add_ai_co_pilot_mysql.sql;

# 验证表创建
SHOW TABLES LIKE 'ai_%';
```

### 验证迁移
```sql
-- 检查表结构
DESCRIBE ai_review_feedback;
DESCRIBE literature_reviews;
DESCRIBE research_plans;

-- 检查索引
SHOW INDEX FROM ai_review_feedback;
```

---

## ⚙️ 配置

### 1. config.json配置
```json
{
  "database": {
    "type": "mysql",
    "host": "localhost",
    "port": 3306,
    "name": "papercrawler",
    "user": "root",
    "password": "your_password"
  },
  "redis": {
    "host": "localhost",
    "port": 6379,
    "database": 0
  },
  "ai": {
    "openai_api_key": "your-openai-api-key",
    "claude_api_key": "your-claude-api-key",
    "default_model": "gpt-4.1-mini"
  },
  "modules": {
    "event_driven_enabled": true,
    "ai_workflow_enabled": true,
    "ai_co_pilot_enabled": true
  }
}
```

### 2. modules.json配置
已自动更新，包含AiCoPilotModule配置。

---

## 🚀 部署

### 开发环境
```bash
# 启动MySQL
sudo systemctl start mysql

# 启动Redis（可选）
sudo systemctl start redis

# 运行服务器
cd build
./PaperCrawlerServer
```

### 生产环境（Docker）
```dockerfile
FROM ubuntu:22.04

# 安装依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libcurl4-openssl-dev \
    libmysqlclient-dev \
    redis-server

# 复制编译好的二进制文件
COPY build/PaperCrawlerServer /app/
COPY config /app/config

# 暴露端口
EXPOSE 8080

# 启动命令
CMD ["/app/PaperCrawlerServer"]
```

### 构建和运行Docker
```bash
# 构建镜像
docker build -t papercrawler:latest .

# 运行容器
docker run -d \
  -p 8080:8080 \
  -e MYSQL_HOST=mysql \
  -e MYSQL_PASSWORD=secret \
  --name papercrawler \
  papercrawler:latest
```

---

## 🧪 测试

### 1. 单元测试
```bash
cd build
ctest --output-on-failure
```

### 2. API测试（使用curl）
```bash
# AI审稿测试
curl -X POST http://localhost:8080/api/ai-co-pilot/review \
  -H "Content-Type: application/json" \
  -d '{
    "paperId": 1,
    "userId": 1,
    "targetJournal": "Nature",
    "includeComparison": true
  }'

# 文献综述生成测试
curl -X POST http://localhost:8080/api/ai-co-pilot/literature-review/generate \
  -H "Content-Type: application/json" \
  -d '{
    "userId": 1,
    "title": "Deep Learning in NLP",
    "researchField": "Computer Science",
    "paperIds": [1, 2, 3],
    "maxLength": 5000,
    "includeGaps": true
  }'

# AI对话测试
curl -X POST http://localhost:8080/api/ai-co-pilot/chat \
  -H "Content-Type: application/json" \
  -d '{
    "userId": 1,
    "message": "What are the latest trends in deep learning?",
    "sessionId": ""
  }'
```

### 3. 性能测试
```bash
# 使用Apache Bench
ab -n 1000 -c 10 http://localhost:8080/api/ai-co-pilot/stats

# 使用wrk
wrk -t4 -c100 -d30s http://localhost:8080/api/ai-co-pilot/recommendations
```

---

## 📊 监控

### Prometheus指标
```
http://localhost:9090/metrics
```

### 日志
```
tail -f logs/papercrawler.log
```

### 健康检查
```
curl http://localhost:8080/health
```

---

## 🐛 故障排查

### 常见问题

**1. 编译错误：找不到头文件**
```bash
# 确保CMAKE_PREFIX_PATH正确设置
export CMAKE_PREFIX_PATH=/path/to/vcpkg/installed/x64-linux
```

**2. 运行时错误：无法连接数据库**
```bash
# 检查MySQL是否运行
sudo systemctl status mysql

# 检查连接配置
mysql -u root -p -h localhost
```

**3. AI API调用失败**
```bash
# 检查API密钥
echo $OPENAI_API_KEY

# 测试API连接
curl https://api.openai.com/v1/models \
  -H "Authorization: Bearer $OPENAI_API_KEY"
```

**4. 模块加载失败**
```bash
# 检查模块文件是否存在
ls -l build/modules/business/libaicopilot.so

# 检查modules.json配置
cat config/modules.json | grep AiCoPilot
```

---

## 📚 相关文档

- [超级功能套件方案](./SUPER_FEATURES_PLAN.md)
- [实施进度](./IMPLEMENTATION_PROGRESS.md)
- [架构分析](./architecture-analysis/FINAL_ARCHITECTURE_REPORT.md)
- [API文档](./API_DOCUMENTATION.md)

---

**最后更新**: 2026-04-02
**版本**: 2.0.0
