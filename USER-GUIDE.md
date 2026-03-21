# PaperCrawler 使用指南

## 🎉 恭喜！项目已完成

您现在拥有一个完整的学术论文检索和分析平台，包括：

- ✅ **C++核心库** - 4700+行高质量C++代码
- ✅ **Qt 6桌面客户端** - 现代化GUI应用
- ✅ **C++ REST API** - 高性能Web服务
- ✅ **Vue 3前端** - 优美Web界面
- ✅ **Docker部署** - 一键启动
- ✅ **完整文档** - 详细使用说明

---

## 🚀 快速开始（3种方式）

### 方式1️⃣: 使用启动脚本（最简单）

**Windows:**
```bash
双击运行 START.bat

然后选择：
1 - 启动后端API
2 - 启动前端界面
3 - 启动桌面应用
4 - Docker一键启动（推荐）
```

**Linux:**
```bash
chmod +x build.sh
./build.sh
```

### 方式2️⃣: Docker部署（推荐）

```bash
cd PaperCrawler

# 1. 配置数据库密码
notepad config\config.json  # Windows
vim config/config.json       # Linux

# 2. 启动所有服务
docker-compose up -d

# 3. 访问应用
# Web前端: http://localhost
# API文档: http://localhost:8080/health

# 4. 查看日志
docker-compose logs -f
```

### 方式3️⃣: 手动编译和运行

#### A. 初始化数据库
```bash
mysql -u root -p < sql/init.sql
```

#### B. 启动后端API
```bash
cd backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./PaperCrawlerServer 8080
```

#### C. 启动前端
```bash
cd frontend
npm install
npm run dev
# 访问 http://localhost:5173
```

#### D. 运行桌面应用
```bash
cd desktop
mkdir build && cd build
cmake ..
make -j$(nproc)
./PaperCrawlerDesktop
```

---

## 📖 使用示例

### Web界面使用

1. **打开浏览器访问**: http://localhost:5173

2. **搜索论文**:
   - 在搜索框输入关键词（如 "deep learning"）
   - 点击搜索按钮
   - 等待结果展示

3. **查看结果**:
   - 浏览论文列表
   - 查看期刊等级
   - 点击论文查看详情

### 桌面应用使用

1. **启动应用**: 运行 `PaperCrawlerDesktop`

2. **搜索**:
   - 顶部搜索框输入关键词
   - 点击Search按钮
   - 查看实时进度

3. **查看结果**:
   - 右侧表格显示搜索结果
   - 点击行查看详情
   - 使用左侧过滤器

4. **导出数据**:
   - 菜单: File → Export Results
   - 选择格式（CSV/JSON/BibTeX）
   - 保存文件

### API使用

```bash
# 健康检查
curl http://localhost:8080/health

# 搜索论文
curl "http://localhost:8080/api/search?q=dma&max=100"

# 获取统计
curl http://localhost:8080/api/stats/overview

# 导出CSV
curl "http://localhost:8080/api/export/csv?type=dma" -o papers.csv

# 导出JSON
curl "http://localhost:8080/api/export/json?type=dma" -o papers.json
```

---

## 🎯 核心功能

### 1. 论文搜索
- ✅ 从DBLP数据库搜索
- ✅ 实时进度显示
- ✅ 自动获取期刊等级
- ✅ 支持大量结果

### 2. 数据管理
- ✅ MySQL数据库存储
- ✅ 批量导入导出
- ✅ 多种格式支持
- ✅ 数据持久化

### 3. 可视化
- ✅ 搜索结果列表
- ✅ 统计图表
- ✅ 实时进度条
- ✅ 状态提示

### 4. 导出功能
- ✅ CSV格式
- ✅ JSON格式
- ✅ BibTeX格式
- ✅ 自定义字段

---

## 🔧 配置说明

### 数据库配置

编辑 `config/config.json`:
```json
{
    "database": {
        "host": "localhost",
        "port": 3306,
        "user": "root",
        "password": "你的密码",
        "database": "csdatabs"
    }
}
```

### 爬虫配置

```json
{
    "crawler": {
        "timeout": 10,
        "retryTimes": 3,
        "delayBetweenRequests": 1000,
        "maxResults": 10000,
        "userAgent": "PaperCrawler/1.0"
    }
}
```

### 日志配置

```json
{
    "logging": {
        "level": "info",
        "file": "paper_crawler.log"
    }
}
```

---

## 📊 项目文件说明

### 核心文件
- `core/include/core/PaperCrawlerAPI.hpp` - 统一API接口
- `core/src/core/PaperCrawlerAPI.cpp` - API实现

### 桌面客户端
- `desktop/src/MainWindow.cpp` - 主窗口
- `desktop/src/SearchWidget.cpp` - 搜索组件
- `desktop/src/ResultView.cpp` - 结果展示

### 后端API
- `backend/src/main.cpp` - API服务器
- `backend/CMakeLists.txt` - 构建配置

### Web前端
- `frontend/src/App.vue` - 根组件
- `frontend/src/views/Home.vue` - 首页
- `frontend/src/api/paper.ts` - API封装

### 部署文件
- `docker-compose.yml` - Docker编排
- `sql/init.sql` - 数据库初始化

---

## 🧪 测试验证

### 运行测试脚本

**Windows:**
```bash
quick-test.bat
```

**Linux:**
```bash
chmod +x quick-test.sh
./quick-test.sh
```

### 手动测试

1. **检查文件结构**
   ```bash
   ls -la core/ desktop/ backend/ frontend/
   ```

2. **测试API**
   ```bash
   curl http://localhost:8080/health
   ```

3. **测试前端**
   - 浏览器访问 http://localhost:5173
   - 搜索功能
   - 界面显示

4. **测试桌面**
   - 运行应用
   - 搜索论文
   - 导出功能

---

## 🐛 常见问题

### Q1: 编译错误 - 找不到Qt
```bash
# Windows
set CMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64

# Linux
export CMAKE_PREFIX_PATH=/opt/Qt/6.5.0/gcc_64
```

### Q2: 数据库连接失败
```bash
# 检查MySQL服务
sudo systemctl status mysql

# 测试连接
mysql -u root -p -h localhost

# 检查端口
sudo netstat -tlnp | grep 3306
```

### Q3: Docker启动失败
```bash
# 检查Docker服务
docker ps

# 查看日志
docker-compose logs

# 重启服务
docker-compose down
docker-compose up -d
```

### Q4: 前端无法连接后端
```bash
# 检查后端是否运行
curl http://localhost:8080/health

# 检查proxy配置
# frontend/vite.config.ts
```

---

## 📈 性能优化建议

### 数据库优化
```sql
-- 添加索引
ALTER TABLE cspaper ADD INDEX idx_type (type);
ALTER TABLE cspaper ADD INDEX idx_year (year);

-- 配置优化
SET GLOBAL innodb_buffer_pool_size = 2G;
```

### 后端优化
- 增加工作线程
- 启用连接池
- 实现缓存机制

### 前端优化
- 代码分割
- 懒加载
- CDN加速

---

## 🔒 安全建议

1. **修改默认密码**
   - 数据库密码
   - API密钥

2. **启用HTTPS**
   - 配置SSL证书
   - 强制HTTPS

3. **限制访问**
   - 防火墙规则
   - IP白名单

4. **定期备份**
   - 数据库备份
   - 配置文件备份

---

## 📚 更多文档

- `README.md` - 项目概述
- `README.EXPANDED.md` - 详细文档
- `DEPLOYMENT.md` - 部署指南
- `TEST-GUIDE.md` - 测试指南
- `PROJECT-SUMMARY.md` - 项目总结

---

## 🎓 学习资源

### C++相关
- C++17特性
- Qt 6文档
- CMake教程

### Web开发
- Vue 3官方文档
- TypeScript手册
- Element Plus组件库

### 部署运维
- Docker官方文档
- MySQL优化指南
- Nginx配置教程

---

## 💡 提示

1. **首次使用建议**:
   - 先用Docker快速体验
   - 再手动编译学习
   - 最后自定义扩展

2. **开发建议**:
   - 遵循现有代码风格
   - 添加注释和文档
   - 编写单元测试

3. **部署建议**:
   - 测试环境先行
   - 备份重要数据
   - 监控运行状态

---

## 🎊 享受使用！

PaperCrawler平台已完成，祝您使用愉快！

如有问题，请参考文档或提交Issue。

**Happy Coding! 🚀**
