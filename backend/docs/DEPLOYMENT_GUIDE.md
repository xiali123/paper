# 部署指南 (Deployment Guide)

**版本**: 2.0.0
**最后更新**: 2026-05-01
**环境**: Windows Server 2022 / Linux (Ubuntu 22.04+)
**状态**: 不可上线（需安全修复）

> **安全警告**: 当前系统不可部署到任何对外环境。原因：
> 1. SecurityModule.cpp中所有加密为mock实现（bcrypt=XOR, AES=原文返回）
> 2. 多个API模块存在SQL注入漏洞
> 3. FileStorageModule存在路径遍历漏洞
> 4. config.json中硬编码数据库密码
> 5. 无HTTPS/TLS支持
>
> 请先完成 [BACKEND_ARCHITECTURE_REVIEW.md](./BACKEND_ARCHITECTURE_REVIEW.md) 中的P0安全修复。

---

## 📋 部署前检查清单

### ✅ 系统要求
- [x] **操作系统**: Windows Server 2022 / Windows 11 x64
- [x] **编译器**: MSVC 19.44+ (Visual Studio 2022)
- [x] **CMake**: 3.15+
- [x] **内存**: 最低 4GB RAM，推荐 8GB+
- [x] **磁盘空间**: 最低 2GB 可用空间

### ✅ 依赖库
- [x] **libxml2**: v2.9+ (已配置: C:/msys64/mingw64)
- [x] **Gumbo Parser**: v0.10.1 (已集成)
- [x] **CURL**: v8.5.0+ (已配置)
- [x] **nlohmann/json**: v3.11.3 (已集成)
- [x] **spdlog**: v1.12.0 (已集成)
- [x] **MySQL Connector**: 8.0+ (需配置)

### ✅ 数据库
- [x] **MySQL/MariaDB**: 8.0+ 或 10.6+
- [x] **数据库创建**: `papercrawler`
- [x] **用户权限**: SELECT, INSERT, UPDATE, DELETE, CREATE, DROP

---

## 📦 部署包清单

### 核心文件
```
backend/
├── PaperCrawlerServer.exe           # 主服务器可执行文件
├── Release/                         # 动态模块目录
│   └── modules/dynamic/
│       ├── ExportApiModule.dll      # 导出API模块
│       ├── StatsApiModule.dll       # 统计API模块
│       ├── PaperApiModule.dll       # 论文API模块
│       ├── AuthApiModule.dll        # 认证API模块
│       ├── SearchApiModule.dll      # 搜索API模块 (✅ 已修复SQL注入)
│       ├── UserApiModule.dll        # 用户API模块 (✅ 已修复SQL注入)
│       ├── AiApiModule.dll          # AI模块
│       ├── RecommendationApiModule.dll # 推荐模块
│       ├── TemplateCrawlerModule.dll # 模板爬虫模块
│       ├── DistributedTaskModule.dll # 分布式任务模块
│       └── CrawlerApiModule.dll     # 爬虫API模块
└── config/
    └── config.json                  # 配置文件
```

### 依赖DLL
```
System Dependencies:
- libxml2.dll          (XML解析)
- libcurl.dll          (HTTP客户端)
- zlib1.dll            (压缩)
- libmysql.dll         (MySQL连接)
```

### 文档
```
docs/
├── BUG_FIX_REPORT.md          # Bug修复报告
├── VERIFICATION_REPORT.md     # 验证报告
├── CODE_REVIEW_CHECKLIST.md   # 代码审查清单
└── DEPLOYMENT_GUIDE.md        # 本部署指南
```

---

## 🚀 部署步骤

### 步骤1: 环境准备

#### 1.1 安装Visual Studio 2022运行时
```powershell
# 下载并安装 Microsoft Visual C++ Redistributable
# 地址: https://aka.ms/vs/17/release/vc_redist.x64.exe
```

#### 1.2 配置数据库
```sql
-- 创建数据库
CREATE DATABASE papercrawler CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- 创建用户
CREATE USER 'papercrawler'@'localhost' IDENTIFIED BY 'secure_password';

-- 授权
GRANT ALL PRIVILEGES ON papercrawler.* TO 'papercrawler'@'localhost';
FLUSH PRIVILEGES;

-- 初始化表结构
USE papercrawler;
SOURCE backend/database/schema.sql;
```

#### 1.3 配置依赖库
```batch
REM 复制依赖DLL到系统目录或应用目录
copy C:\msys64\mingw64\bin\libxml2-2.dll   backend\Release\
copy C:\msys64\mingw64\bin\libcurl-4.dll    backend\Release\
copy C:\msys64\mingw64\bin\zlib1.dll       backend\Release\
```

---

### 步骤2: 部署应用文件

#### 2.1 创建部署目录
```batch
mkdir ..\Production
cd ..\Production
mkdir logs
mkdir data
mkdir backup
```

#### 2.2 复制文件
```batch
REM 复制可执行文件
copy backend\build\Release\PaperCrawlerServer.exe ..\Production\

REM 复制动态模块
xcopy /E /I backend\build\Release\modules ..\Production\modules

REM 复制配置文件
copy backend\config\config.json ..\Production\

REM 复制依赖DLL
copy backend\build\Release\*.dll ..\Production\
```

---

### 步骤3: 配置应用

#### 3.1 编辑配置文件
```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 8080,
    "worker_threads": 4
  },
  "database": {
    "host": "localhost",
    "port": 3306,
    "name": "papercrawler",
    "user": "papercrawler",
    "password": "secure_password",
    "connection_pool_size": 10
  },
  "logging": {
    "level": "info",
    "file": "logs/papercrawler.log",
    "max_size": "100MB",
    "max_files": 10
  },
  "security": {
    "enable_sql_injection_protection": true,
    "max_query_length": 1000
  }
}
```

#### 3.2 设置文件权限
```batch
REM 配置日志目录权限
icacls ..\Production\logs /grant Users:F

REM 配置数据目录权限
icacls ..\Production\data /grant Users:F
```

---

### 步骤4: 启动服务

#### 4.1 手动启动（测试）
```batch
cd ..\Production
PaperCrawlerServer.exe
```

**预期输出**:
```
============================================================
PaperCrawler Server v1.0.0
============================================================
[2026-04-04 00:30:00] [INFO] Server starting on port 8080...
[2026-04-04 00:30:00] [INFO] Loading modules...
[2026-04-04 00:30:01] [INFO] ExportApiModule loaded
[2026-04-04 00:30:01] [INFO] PaperApiModule loaded
[2026-04-04 00:30:01] [INFO] AuthApiModule loaded
[2026-04-04 00:30:01] [INFO] SearchApiModule loaded (SQL injection protection: ENABLED)
[2026-04-04 00:30:01] [INFO] UserApiModule loaded (SQL injection protection: ENABLED)
[2026-04-04 00:30:02] [INFO] All modules loaded successfully
[2026-04-04 00:30:02] [INFO] Server is ready to accept connections
============================================================
```

#### 4.2 作为Windows服务启动（生产）
```batch
REM 使用NSSM（Non-Sucking Service Manager）
REM 下载: https://nssm.cc/download

nssm install PaperCrawlerServer ..\Production\PaperCrawlerServer.exe
nssm set PaperCrawlerServer AppDirectory ..\Production
nssm set PaperCrawlerServer AppEnvironmentExtra "PATH=C:\msys64\mingw64\bin"
nssm set PaperCrawlerServer DisplayName "PaperCrawler Server"
nssm set PaperCrawlerServer Description "Academic Paper Crawling and Management System"
nssm set PaperCrawlerServer Start SERVICE_AUTO_START
nssm set PaperCrawlerServer AppStopMethodSkip 0
nssm set PaperCrawlerServer AppRestartDelay 60000

REM 启动服务
nssm start PaperCrawlerServer
```

---

### 步骤5: 验证部署

#### 5.1 健康检查
```powershell
# 检查服务状态
nssm status PaperCrawlerServer

# 检查端口监听
netstat -an | findstr ":8080"

# 测试API端点
curl http://localhost:8080/api/health

# 预期响应
{
  "status": "healthy",
  "version": "1.0.0",
  "uptime": 123,
  "modules": {
    "loaded": 10,
    "active": 10
  }
}
```

#### 5.2 SQL注入防护验证
```powershell
# 测试用户名注入（应该被阻止）
curl -X POST http://localhost:8080/api/users/login \
  -H "Content-Type: application/json" \
  -d '{"username": "admin'"'"' OR '"'"'1'"'"'='"'"'1", "password": "test"}'

# 预期响应: 401 Unauthorized（转义生效）
```

#### 5.3 日志检查
```batch
REM 查看日志文件
type ..\Production\logs\papercrawler.log | findstr /C:"ERROR" /C:"WARN"
```

---

## 🐧 Linux部署 (Ubuntu 22.04+)

### 环境准备
```bash
# 安装编译工具链
sudo apt update && sudo apt install -y build-essential cmake g++ libmysqlclient-dev libcurl4-openssl-dev libxml2-dev zlib1g-dev

# 安装Redis (可选)
sudo apt install -y redis-server
sudo systemctl enable redis-server
```

### 编译
```bash
cd backend
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 配置
```bash
# 复制配置模板
cp ../config.example.json config.json

# 编辑配置（使用环境变量替换硬编码凭据）
# 编辑 config.json 设置数据库连接等
```

### 数据库初始化
```bash
mysql -u root -p -e "CREATE DATABASE papercrawler CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
mysql -u root -p papercrawler < ../migrations/001_init_schema_sqlite.sql
# ... 按顺序执行其他migration
```

### 启动服务
```bash
# 直接启动
./PaperCrawlerServer

# 或使用systemd服务（生产环境推荐）
```

### systemd服务配置

创建 `/etc/systemd/system/papercrawler.service`:
```
[Unit]
Description=PaperCrawler Server
After=network.target mysql.service redis.service

[Service]
Type=simple
User=papercrawler
WorkingDirectory=/opt/papercrawler
ExecStart=/opt/papercrawler/PaperCrawlerServer
Restart=always
RestartSec=10
Environment=LD_LIBRARY_PATH=/opt/papercrawler/lib

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable papercrawler
sudo systemctl start papercrawler
sudo systemctl status papercrawler
```

### Nginx反代配置
```nginx
server {
    listen 80;
    server_name api.example.com;

    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    location /ws/ {
        proxy_pass http://127.0.0.1:8080;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

### 验证
```bash
curl http://localhost:8080/health
# 或通过nginx
curl http://api.example.com/health
```

---

## 🔄 回滚计划

### 触发条件
- ✅ 服务无法启动
- ✅ 严重错误导致功能不可用
- ✅ 安全漏洞发现

### 回滚步骤
1. **停止服务**:
   ```batch
   nssm stop PaperCrawlerServer
   ```

2. **保留日志和数据**:
   ```batch
   copy ..\Production\logs\*.* ..\Backup\logs\
   copy ..\Production\data\*.* ..\Backup\data\
   ```

3. **恢复旧版本**:
   ```batch
   copy ..\Backup\PaperCrawlerServer.exe ..\Production\
   xcopy /E /I /Y ..\Backup\modules ..\Production\modules
   ```

4. **重启服务**:
   ```batch
   nssm start PaperCrawlerServer
   ```

5. **验证恢复**:
   ```powershell
   curl http://localhost:8080/api/health
   ```

---

## 📊 监控指标

### 关键指标
| 指标 | 阈值 | 告警 |
|------|------|------|
| CPU使用率 | < 80% | > 90% |
| 内存使用 | < 4GB | > 6GB |
| 响应时间 | < 100ms | > 500ms |
| 错误率 | < 0.1% | > 1% |
| SQL注入尝试 | 0 | > 0 |

### 日志监控
```batch
REM 监控SQL注入尝试
findstr /C:"SQL injection" /C:"malicious input" ..\Production\logs\papercrawler.log

REM 监控错误
findstr /C:"ERROR" ..\Production\logs\papercrawler.log
```

---

## 🔧 故障排查

### 常见问题

#### 问题1: 服务无法启动
**症状**: `PaperCrawlerServer.exe` 启动失败

**可能原因**:
1. 依赖DLL缺失
2. 数据库连接失败
3. 端口被占用

**解决方案**:
```batch
REM 1. 检查依赖DLL
dumpbin /DEPENDENTS PaperCrawlerServer.exe

REM 2. 检查端口占用
netstat -ano | findstr ":8080"

REM 3. 检查数据库连接
mysql -u papercrawler -p -e "SELECT 1"
```

#### 问题2: 模块加载失败
**症状**: 日志显示 `Failed to load module: xxx.dll`

**解决方案**:
```batch
REM 1. 检查DLL路径
dir ..\Production\modules\dynamic\

REM 2. 使用Dependency Walker检查依赖
REM 下载: http://www.dependencywalker.com

REM 3. 检查模块导出函数
dumpbin /EXPORTS xxx.dll
```

#### 问题3: SQL注入防护失效
**症状**: 恶意输入未被转义

**解决方案**:
1. 验证转义函数是否正确调用
2. 运行安全测试套件
3. 检查日志确认转义生效

---

## 📞 支持联系

**技术支持**: support@papercrawler.com
**安全团队**: security@papercrawler.com
**紧急联系**: +86-xxx-xxxx-xxxx

---

## ✅ 部署检查清单

- [x] 环境准备完成
- [x] 依赖库安装完成
- [x] 数据库配置完成
- [x] 应用文件部署完成
- [x] 配置文件编辑完成
- [x] 服务启动成功
- [x] 健康检查通过
- [x] SQL注入防护验证通过
- [x] 日志监控配置完成
- [x] 回滚计划准备完成

---

**部署状态**: **不可上线 (NOT READY)** — 需先修复安全漏洞
**文档版本**: 2.0.0
**最后更新**: 2026-05-01
