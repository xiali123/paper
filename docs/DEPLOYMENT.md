# PaperCrawler 部署指南

本文档详细介绍如何部署PaperCrawler平台的三种客户端：桌面应用、Web服务和完整平台。

---

## 📦 部署方式概览

### 1. 桌面应用部署
- **适用场景**: 个人使用、单机部署
- **特点**: 无需服务器，直接安装运行

### 2. Web服务部署
- **适用场景**: 团队协作、多用户访问
- **特点**: 客户端-服务器架构

### 3. 完整平台部署
- **适用场景**: 公共服务、生产环境
- **特点**: 包含数据库、后端、前端的完整系统

---

## 🖥️ 方式1: 桌面应用部署

### Windows

#### 编译发布版本
```bash
cd desktop
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64" ..
cmake --build . --config Release
```

#### 打包应用
```bash
# 使用windeployqt
cd build/Release
windeployqt PaperCrawlerDesktop.exe

# 创建安装包
# 使用NSIS或Inno Setup创建安装程序
```

#### 用户安装
1. 下载 `PaperCrawler-Setup.exe`
2. 运行安装程序
3. 启动应用，配置数据库连接

### Linux

#### 编译
```bash
cd desktop
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

#### 打包
```bash
# 创建AppImage
mkdir -p AppDir/usr/bin
cp build/PaperCrawlerDesktop AppDir/usr/bin/
wget -c "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
wget -c "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
chmod +x linuxdeploy-x86_64.AppImage linuxdeploy-plugin-qt-x86_64.AppImage
./linuxdeploy-x86_64.AppImage --appdir AppDir --plugin qt --output appimage
```

#### 安装
```bash
# Debian/Ubuntu
sudo dpkg -i PaperCrawlerDesktop_1.0.0_amd64.deb

# CentOS/RHEL
sudo rpm -i PaperCrawlerDesktop-1.0.0-1.x86_64.rpm
```

### macOS

#### 编译
```bash
cd desktop
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
```

#### 打包
```bash
# 创建DMG
macdeployqt build/PaperCrawlerDesktop.app -dmg
```

---

## 🌐 方式2: Web服务部署

### 系统要求

- **操作系统**: Linux (Ubuntu 20.04+, CentOS 7+)
- **CPU**: 2核心及以上
- **内存**: 4GB及以上
- **磁盘**: 20GB及以上
- **网络**: 公网IP或内网访问

### 手动部署

#### 1. 安装依赖

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libssl-dev \
    libmysqlclient-dev \
    mysql-server
```

**CentOS/RHEL:**
```bash
sudo yum groupinstall -y "Development Tools"
sudo yum install -y \
    cmake \
    git \
    libcurl-devel \
    openssl-devel \
    mysql-devel \
    mysql-server
```

#### 2. 编译后端

```bash
git clone https://github.com/your-repo/PaperCrawler.git
cd PaperCrawler/backend
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

#### 3. 配置MySQL

```bash
sudo mysql_secure_installation

sudo mysql -u root -p
```

```sql
CREATE DATABASE csdatabs CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'papercrawler'@'localhost' IDENTIFIED BY 'your_password';
GRANT ALL PRIVILEGES ON csdatabs.* TO 'papercrawler'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

#### 4. 配置应用

编辑 `config/config.json`:
```json
{
    "database": {
        "host": "localhost",
        "port": 3306,
        "user": "papercrawler",
        "password": "your_password",
        "database": "csdatabs"
    },
    "crawler": {
        "timeout": 10,
        "userAgent": "PaperCrawler/1.0"
    },
    "logging": {
        "level": "info",
        "file": "/var/log/papercrawler/api.log"
    }
}
```

#### 5. 创建系统服务

创建 `/etc/systemd/system/papercrawler.service`:

```ini
[Unit]
Description=PaperCrawler API Server
After=network.target mysql.service

[Service]
Type=simple
User=papercrawler
WorkingDirectory=/opt/PaperCrawler
ExecStart=/opt/PaperCrawler/PaperCrawlerServer
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```

启动服务:
```bash
sudo systemctl daemon-reload
sudo systemctl enable papercrawler
sudo systemctl start papercrawler
sudo systemctl status papercrawler
```

#### 6. 配置Nginx反向代理

创建 `/etc/nginx/sites-available/papercrawler`:

```nginx
server {
    listen 80;
    server_name your-domain.com;

    location / {
        proxy_pass http://localhost:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        # WebSocket support
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

启用配置:
```bash
sudo ln -s /etc/nginx/sites-available/papercrawler /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

---

## 🐳 方式3: Docker部署 (推荐)

### 快速启动

```bash
# 克隆项目
git clone https://github.com/your-repo/PaperCrawler.git
cd PaperCrawler

# 创建环境变量
cat > .env << EOF
DB_PASSWORD=your_secure_password
EOF

# 启动所有服务
docker-compose up -d

# 查看日志
docker-compose logs -f

# 停止服务
docker-compose down
```

### 生产环境配置

#### 1. 使用外部数据库

修改 `docker-compose.yml`:
```yaml
services:
  backend:
    environment:
      DB_HOST: your-external-db-host
      DB_PORT: 3306
```

#### 2. 配置HTTPS

使用Nginx反向代理:

```nginx
server {
    listen 443 ssl http2;
    server_name your-domain.com;

    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;

    location / {
        proxy_pass http://localhost:8080;
        # ... other proxy settings
    }
}
```

#### 3. 配置自动重启

```yaml
services:
  backend:
    restart: always
    deploy:
      resources:
        limits:
          cpus: '2.0'
          memory: 2G
```

### 备份与恢复

#### 备份
```bash
# 备份数据库
docker-compose exec mysql mysqldump -u root -p csdatabs > backup_$(date +%Y%m%d).sql

# 备份配置
tar -czf config_backup_$(date +%Y%m%d).tar.gz config/
```

#### 恢复
```bash
# 恢复数据库
docker-compose exec -T mysql mysql -u root -p csdatabs < backup_20241201.sql

# 恢复配置
tar -xzf config_backup_20241201.tar.gz
```

---

## 🚀 性能优化

### 数据库优化

```sql
-- 添加索引
ALTER TABLE cspaper ADD INDEX idx_type (type);
ALTER TABLE cspaper ADD INDEX idx_year (year);
ALTER TABLE cspaper ADD INDEX idx_qkid (qkid);
ALTER TABLE qikantb ADD INDEX idx_name (name);

-- 配置优化
SET GLOBAL max_connections = 200;
SET GLOBAL innodb_buffer_pool_size = 2G;
```

### 后端优化

```cpp
// 使用连接池
// 启用缓存
// 异步处理
```

### 前端优化

```bash
# 构建生产版本
npm run build

# 启用Gzip压缩
# CDN加速
```

---

## 🔒 安全配置

### 1. 数据库安全

```bash
# 修改root密码
sudo mysql -u root -p
ALTER USER 'root'@'localhost' IDENTIFIED BY 'strong_password';

# 删除测试数据库
DROP DATABASE IF EXISTS test;
DELETE FROM mysql.user WHERE User='';

# 限制远程访问
CREATE USER 'papercrawler'@'localhost' IDENTIFIED BY 'password';
GRANT ALL PRIVILEGES ON csdatabs.* TO 'papercrawler'@'localhost';
```

### 2. API安全

- 启用HTTPS
- 实现认证授权
- 限流保护
- CORS配置

### 3. 防火墙配置

```bash
# Ubuntu UFW
sudo ufw allow 22/tcp
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw enable

# CentOS firewalld
sudo firewall-cmd --permanent --add-service=http
sudo firewall-cmd --permanent --add-service=https
sudo firewall-cmd --reload
```

---

## 📊 监控与日志

### 日志管理

```bash
# 查看API日志
tail -f /var/log/papercrawler/api.log

# 查看Nginx日志
tail -f /var/log/nginx/access.log
tail -f /var/log/nginx/error.log

# 查看MySQL日志
tail -f /var/log/mysql/error.log
```

### 性能监控

```bash
# 系统资源
htop

# 数据库性能
docker-compose exec mysql mysqladmin -u root -p processlist

# API性能
curl -w "@curl-format.txt" http://localhost:8080/health
```

---

## 🐛 故障排除

### 常见问题

**1. API无法启动**
```bash
# 检查日志
journalctl -u papercrawler -n 50

# 检查端口
sudo netstat -tlnp | grep 8080

# 检查数据库连接
mysql -u papercrawler -p -h localhost csdatabs
```

**2. 前端无法连接后端**
```bash
# 检查CORS配置
# 检查代理配置
# 检查防火墙
```

**3. 数据库性能问题**
```bash
# 慢查询日志
SET GLOBAL slow_query_log = 'ON';
SET GLOBAL long_query_time = 2;

# 查看慢查询
tail -f /var/log/mysql/slow.log
```

---

## 📞 支持

如有问题，请提交Issue或联系维护团队。
