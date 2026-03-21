# 快速入门指南

## 5分钟快速上手

### 1️⃣ 准备工作

确保你的系统已安装：
- MySQL 5.7+ 或 MariaDB
- C++17 编译器
- CMake 3.15+
- Git

### 2️⃣ 获取代码

```bash
cd E:\研究生\资料
git clone <your-repo-url> PaperCrawler
cd PaperCrawler
```

### 3️⃣ 安装依赖

#### Windows
```bash
# 使用 vcpkg 安装依赖
vcpkg install curl:x64-windows openssl:x64-windows mysql-connector-cpp:x64-windows
```

#### Linux
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libcurl4-openssl-dev libssl-dev libmysqlclient-dev
```

### 4️⃣ 配置数据库

```bash
# 登录 MySQL
mysql -u root -p

# 执行以下 SQL
CREATE DATABASE IF NOT EXISTS csdatabs CHARACTER SET utf8mb4;
USE csdatabs;

CREATE TABLE IF NOT EXISTS cspaper (
    id INT AUTO_INCREMENT PRIMARY KEY,
    kid INT NOT NULL DEFAULT 0,
    type VARCHAR(50),
    title TEXT,
    qikanfull VARCHAR(255),
    qikanjc VARCHAR(100),
    year VARCHAR(10),
    author VARCHAR(255),
    qikanurl VARCHAR(512),
    doiurl VARCHAR(512),
    info TEXT,
    qkid INT DEFAULT 0,
    level VARCHAR(10),
    INDEX idx_type (type),
    INDEX idx_qkid (qkid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS qikantb (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) UNIQUE,
    fullname VARCHAR(255),
    level VARCHAR(10),
    flevel VARCHAR(10),
    info TEXT,
    url VARCHAR(512),
    INDEX idx_name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### 5️⃣ 配置应用

```bash
# 复制示例配置
cp config/config.json.example config/config.json

# 编辑配置文件
# Windows: notepad config\config.json
# Linux: vim config/config.json
```

修改数据库配置：
```json
{
    "database": {
        "host": "localhost",
        "port": 3306,
        "user": "root",
        "password": "你的MySQL密码",
        "database": "csdatabs"
    }
}
```

### 6️⃣ 编译项目

#### Windows
```bash
# 使用构建脚本
build.bat

# 或手动编译
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=[vcpkg路径]/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --config Release
```

#### Linux
```bash
chmod +x build.sh
./build.sh

# 或手动编译
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 7️⃣ 运行程序

```bash
# Windows
.\build\bin\Release\PaperCrawler.exe --keyword "dma"

# Linux
./build/bin/PaperCrawler --keyword "dma"
```

### 📊 查看结果

```bash
# 登录 MySQL 查看结果
mysql -u root -p
USE csdatabs;

# 查看论文数量
SELECT COUNT(*) FROM cspaper;

# 查看期刊数量
SELECT COUNT(*) FROM qikantb;

# 查看论文详情
SELECT id, title, qikanjc, year FROM cspaper LIMIT 10;
```

## 🎯 常用命令

```bash
# 指定搜索关键词
./PaperCrawler --keyword "machine learning"

# 指定配置文件
./PaperCrawler --config /path/to/config.json

# 查看帮助
./PaperCrawler --help
```

## 🐛 常见问题

### Q: 编译时找不到 MySQL 头文件？
A: 设置 MySQL 路径：
```bash
cmake -DMYSQL_INCLUDE_DIR=/path/to/mysql/include \
      -DMYSQL_LIBRARY=/path/to/mysql/lib/libmysqlclient.so ..
```

### Q: 网络请求超时？
A: 在配置文件中增加超时时间：
```json
{
    "crawler": {
        "timeout": 30
    }
}
```

### Q: 数据库连接失败？
A: 检查 MySQL 服务是否运行：
```bash
# Windows
net start mysql

# Linux
sudo systemctl status mysql
```

## 📚 下一步

- 📖 阅读 [README.md](README.md) 了解更多功能
- 🎨 自定义 `config/config.json` 配置
- 🔍 查看 `paper_crawler.log` 日志文件
- 💻 参考源码进行二次开发

## 💡 提示

- 首次运行建议从少量数据开始测试
- 定期检查日志文件了解运行状态
- 数据库密码不要提交到版本控制系统
- 建议使用数据库备份保护数据安全
