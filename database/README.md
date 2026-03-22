# PaperCrawler 数据库初始化指南

本指南帮助你快速设置 PaperCrawler 项目的数据库环境。

---

## 快速开始

### 前置要求

- **MySQL**: 8.0 或更高版本
- **SQLite**: 3.35 或更高版本
- **权限**: MySQL 数据库创建权限

### 1. 创建 MySQL 数据库

```bash
# 登录 MySQL
mysql -u root -p

# 创建数据库
CREATE DATABASE papercrawler CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

# 创建用户（可选，推荐）
CREATE USER 'papercrawler'@'localhost' IDENTIFIED BY 'your_secure_password';
GRANT ALL PRIVILEGES ON papercrawler.* TO 'papercrawler'@'localhost';
FLUSH PRIVILEGES;

# 退出
EXIT;
```

### 2. 初始化数据库架构

```bash
# 进入数据库目录
cd E:\PaperCrawler\database

# 导入 MySQL 架构
mysql -u root -p papercrawler < complete-schema-mysql.sql

# 或者使用特定用户
mysql -u papercrawler -p papercrawler < complete-schema-mysql.sql
```

### 3. 初始化 SQLite 数据库（本地）

```bash
# 创建 SQLite 数据库
sqlite3 papercrawler.db < complete-schema-sqlite.sql

# 或者从应用代码初始化
# 应用会自动创建数据库文件
```

### 4. 应用索引优化

```bash
# 应用 MySQL 索引优化
mysql -u root -p papercrawler < indexes-optimization-guide.sql

# SQLite 索引已包含在主架构中
```

### 5. 验证安装

```bash
# MySQL 验证
mysql -u root -p papercrawler -e "SHOW TABLES;"

# SQLite 验证
sqlite3 papercrawler.db ".tables"

# 应该看到以下表：
# - users
# - papers
# - journals
# - user_bookmarks
# - user_collections
# - crawler_sources
# - ai_conversations
# - sync_logs
# - 等等...
```

---

## 数据迁移

### 从旧版本迁移

如果你有现有的 PaperCrawler 数据库，请运行迁移脚本：

```bash
# 备份现有数据库
mysqldump -u root -p papercrawler > backup_$(date +%Y%m%d).sql

# 运行迁移脚本
mysql -u root -p papercrawler < migration-script.sql

# 验证迁移结果
mysql -u root -p papercrawler -e "
SELECT
    'Total papers' as metric,
    COUNT(*) as count
FROM papers
UNION ALL
SELECT 'Total users', COUNT(*) FROM users;
"
```

---

## 配置应用连接

### MySQL 连接配置

编辑 `backend/config.json`:

```json
{
  "database": {
    "type": "mysql",
    "host": "localhost",
    "port": 3306,
    "database": "papercrawler",
    "user": "papercrawler",
    "password": "your_secure_password",
    "charset": "utf8mb4",
    "pool_size": 10
  }
}
```

### SQLite 连接配置

```json
{
  "database": {
    "type": "sqlite",
    "path": "E:/PaperCrawler/data/papercrawler.db",
    "journal_mode": "WAL",
    "synchronous": "NORMAL"
  }
}
```

---

## 初始化数据

### 创建默认管理员

```sql
-- 创建超级管理员
INSERT INTO users (
    username,
    email,
    password_hash,
    salt,
    full_name,
    role,
    is_active,
    is_verified
) VALUES (
    'admin',
    'admin@papercrawler.local',
    'pbkdf2:sha256:260000$...',  -- 使用应用生成
    'salt...',
    'System Administrator',
    'superadmin',
    TRUE,
    TRUE
);
```

### 创建默认爬虫源

```sql
-- DBLP 爬虫源
INSERT INTO crawler_sources (
    name,
    display_name,
    source_type,
    base_url,
    is_official,
    priority
) VALUES
(
    'dblp',
    'DBLP Computer Science Bibliography',
    'api',
    'https://dblp.org',
    TRUE,
    10
),
(
    'arxiv',
    'arXiv Preprint Server',
    'api',
    'https://export.arxiv.org',
    TRUE,
    10
),
(
    'semanticscholar',
    'Semantic Scholar',
    'api',
    'https://api.semanticscholar.org',
    TRUE,
    20
);
```

---

## 性能优化

### MySQL 配置优化

编辑 `my.cnf` (Linux) 或 `my.ini` (Windows):

```ini
[mysqld]
# InnoDB 缓冲池（设置为可用内存的 70-80%）
innodb_buffer_pool_size = 2G

# 查询缓存
query_cache_size = 256M
query_cache_type = 1

# 临时表
tmp_table_size = 256M
max_heap_table_size = 256M

# 连接数
max_connections = 200

# 慢查询日志
slow_query_log = 1
long_query_time = 1
```

重启 MySQL:
```bash
# Linux
sudo systemctl restart mysql

# Windows
net stop mysql && net start mysql
```

### SQLite 配置优化

在应用启动时执行：

```sql
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;
PRAGMA cache_size = -50000;  -- 50MB
PRAGMA temp_store = MEMORY;
PRAGMA mmap_size = 30000000000;  -- ~30GB
```

---

## 维护任务

### 日常维护

```bash
#!/bin/bash
# daily-maintenance.sh

# MySQL 维护
mysql -u root -p papercrawler -e "OPTIMIZE TABLE papers, users, user_bookmarks;"
mysql -u root -p papercrawler -e "ANALYZE TABLE papers, users, user_bookmarks;"

# SQLite 维护
sqlite3 /path/to/papercrawler.db "VACUUM;"
sqlite3 /path/to/papercrawler.db "ANALYZE;"

# 清理过期数据
mysql -u root -p papercrawler -e "
DELETE FROM user_sessions WHERE expires_at < NOW();
DELETE FROM crawler_logs WHERE logged_at < DATE_SUB(NOW(), INTERVAL 30 DAY);
DELETE FROM search_history WHERE created_at < DATE_SUB(NOW(), INTERVAL 90 DAY);
"
```

### 备份计划

```bash
#!/bin/bash
# backup.sh

DATE=$(date +%Y%m%d_%H%M%S)

# MySQL 备份
mysqldump -u root -p papercrawler | gzip > backups/mysql_backup_${DATE}.sql.gz

# SQLite 备份
cp /path/to/papercrawler.db backups/sqlite_backup_${DATE}.db

# 保留最近 30 天的备份
find backups/ -name "mysql_backup_*.sql.gz" -mtime +30 -delete
find backups/ -name "sqlite_backup_*.db" -mtime +30 -delete
```

### 监控脚本

```bash
#!/bin/bash
# health-check.sh

# 检查数据库连接
mysql -u root -p papercrawler -e "SELECT 1;" > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "✓ MySQL is running"
else
    echo "✗ MySQL is down!"
    # 发送告警
fi

# 检查表大小
mysql -u root -p papercrawler -e "
SELECT
    table_name,
    ROUND((data_length + index_length) / 1024 / 1024, 2) AS size_mb
FROM information_schema.TABLES
WHERE table_schema = 'papercrawler'
  AND (data_length + index_length) > 100000000
ORDER BY size_mb DESC;
"

# 检查慢查询
mysql -u root -p papercrawler -e "
SELECT COUNT(*) as slow_queries
FROM mysql.slow_log
WHERE start_time > DATE_SUB(NOW(), INTERVAL 1 HOUR);
"
```

---

## 故障排查

### 常见问题

#### 1. 连接失败

```
Error: Can't connect to MySQL server on 'localhost'
```

**解决方案**:
```bash
# 检查 MySQL 状态
sudo systemctl status mysql

# 检查端口
netstat -tlnp | grep 3306

# 检查防火墙
sudo ufw allow 3306
```

#### 2. 字符集问题

```
Error: Incorrect string value
```

**解决方案**:
```sql
-- 检查字符集
SHOW VARIABLES LIKE 'character%';

-- 修改表字符集
ALTER DATABASE papercrawler CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
ALTER TABLE papers CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
```

#### 3. 慢查询

```
Query took 5.2 seconds
```

**解决方案**:
```sql
-- 检查慢查询日志
SELECT * FROM mysql.slow_log
ORDER BY query_time DESC
LIMIT 10;

-- 分析查询
EXPLAIN SELECT * FROM papers WHERE ...;

-- 添加索引
CREATE INDEX idx_field ON papers(field);
```

---

## 开发环境设置

### Docker Compose 设置

```yaml
# docker-compose.yml
version: '3.8'

services:
  mysql:
    image: mysql:8.0
    environment:
      MYSQL_ROOT_PASSWORD: rootpassword
      MYSQL_DATABASE: papercrawler
      MYSQL_USER: papercrawler
      MYSQL_PASSWORD: password
    ports:
      - "3306:3306"
    volumes:
      - mysql_data:/var/lib/mysql
      - ./database:/docker-entrypoint-initdb.d
    command: --character-set-server=utf8mb4 --collation-server=utf8mb4_unicode_ci

  adminer:
    image: adminer
    ports:
      - "8080:8080"

volumes:
  mysql_data:
```

启动开发环境:
```bash
docker-compose up -d
```

---

## 生产环境部署

### 安全配置

```sql
-- 移除测试数据库
DROP DATABASE IF EXISTS test;

-- 移除匿名用户
DELETE FROM mysql.user WHERE User = '';

-- 禁止远程 root 登录
DELETE FROM mysql.user
WHERE User = 'root' AND Host NOT IN ('localhost', '127.0.0.1', '::1');

-- 刷新权限
FLUSH PRIVILEGES;
```

### 性能调优

```sql
-- 启用查询缓存（如果使用 MySQL 5.7 或更早）
SET GLOBAL query_cache_size = 256M;
SET GLOBAL query_cache_type = ON;

-- 调整 InnoDB 缓冲池
SET GLOBAL innodb_buffer_pool_size = 2147483648;  -- 2GB

-- 增加连接数
SET GLOBAL max_connections = 200;
```

### 监控设置

```bash
# 安装监控工具
sudo apt-get install percona-toolkit

# 慢查询分析
pt-query-digest /var/log/mysql/slow-query.log

# 表大小监控
pt-duplicate-key-checker --host=localhost --user=root --pass=password
```

---

## 支持和帮助

### 文档资源

- [完整数据库文档](./DATABASE_DOCUMENTATION.md)
- [索引优化指南](./indexes-optimization-guide.sql)
- [迁移脚本](./migration-script.sql)

### 获取帮助

- GitHub Issues: https://github.com/papercrawler/papercrawler/issues
- 文档: https://docs.papercrawler.com
- 论坛: https://forum.papercrawler.com

---

## 许可证

MIT License - 详见 LICENSE 文件

---

**版本**: 2.0.0
**最后更新**: 2026-03-22
**维护者**: PaperCrawler 开发团队
