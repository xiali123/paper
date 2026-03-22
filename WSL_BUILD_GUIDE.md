# WSL后端构建指南

## 快速构建（推荐）

### 方式1: 自动脚本（5分钟）

```cmd
e:\PaperCrawler\setup_wsl_backend.bat
```

这将自动：
1. 安装Ubuntu 24.04 LTS
2. 安装所有构建依赖
3. 编译后端服务器
4. 生成可执行文件

---

### 方式2: 手动构建（10分钟）

#### 步骤1: 打开WSL

```cmd
wsl
```

#### 步骤2: 安装依赖

```bash
# 更新包管理器
sudo apt update

# 安装构建工具和依赖
sudo apt install -y build-essential cmake git \
  libmysqlclient-dev libcurl4-openssl-dev libgumbo-dev
```

#### 步骤3: 构建项目

```bash
# 进入项目目录
cd /mnt/e/PaperCrawler

# 创建构建目录
mkdir -p build && cd build

# 配置CMake
cmake ..

# 编译（使用4个核心加速）
make -j4
```

#### 步骤4: 复制到Windows目录

```bash
# 从WSL复制到Windows
cp /mnt/e/PaperCrawler/build/backend/PaperCrawlerServer /mnt/e/PaperCrawler/backend/
```

---

## 测试后端

### 启动服务器

在WSL中：
```bash
cd /mnt/e/PaperCrawler/build
./backend/PaperCrawlerServer
```

或在Windows中（复制后）：
```cmd
e:\PaperCrawler\backend\PaperCrawlerServer.exe
```

### 测试分页API

```bash
# 测试1: 第1页（5条）
curl "http://localhost:8080/api/search?q=test&offset=0&limit=5"

# 测试2: 第2页（5条）
curl "http://localhost:8080/api/search?q=test&offset=5&limit=5"

# 测试3: 大页面（20条）
curl "http://localhost:8080/api/search?q=AI&offset=0&limit=20"
```

**预期结果**:
- ✅ 每次返回不同的论文
- ✅ 标题不包含URL参数（如"&offset=0&limit=5"）
- ✅ 数量正确（5条、10条、20条）

---

## 常见问题

### Q: WSL命令未找到
**A**: 安装WSL：
```cmd
wsl --install
```

### Q: cmake版本过低
**A**: 安装最新版cmake：
```bash
sudo apt install -y cmake
```

### Q: MySQL连接失败
**A**: 确保MySQL服务运行：
```cmd
net start MySQL80
```

---

## 构建成功后

运行完整测试：
```cmd
e:\PaperCrawler\test_pagination.bat
```
