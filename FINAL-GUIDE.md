# 🎉 PaperCrawler 完整平台 - 最终指南

## 🚀 快速启动（3种方式）

### 方式 1: 后端 API + Web 前端（推荐）⭐⭐⭐

#### 终端 1: 启动后端 API
```bash
cd E:\PaperCrawler
START-BACKEND.bat
```
后端将在 http://localhost:8080 启动

#### 终端 2: 启动前端（使用简单的HTML文件）
```bash
# 浏览器直接打开
E:\PaperCrawler\frontend\index.html
```

---

### 方式 2: Qt 桌面应用

```bash
cd E:\PaperCrawler\desktop
run-desktop.bat
```

---

### 方式 3: Docker 部署

```bash
cd E:\PaperCrawler
docker-compose up -d
```

访问: http://localhost

---

## 📋 API 端点

后端提供以下 RESTful API：

### 健康检查
```
GET /health
```

### 搜索论文
```
GET /api/search?q=keyword&max=100
```

### 获取论文列表
```
GET /api/papers?type=deep&offset=0&limit=20
```

### 获取论文详情
```
GET /api/papers/<id>
```

### 统计信息
```
GET /api/stats/overview
```

### 导出 CSV
```
GET /api/export/csv?type=deep
```

### 导出 JSON
```
GET /api/export/json?type=deep
```

---

## 🧪 测试 API

### 使用 curl 测试

```bash
# 健康检查
curl http://localhost:8080/health

# 搜索论文
curl "http://localhost:8080/api/search?q=deep"

# 获取统计
curl http://localhost:8080/api/stats/overview

# 导出 CSV
curl http://localhost:8080/api/export/csv -o papers.csv
```

### 使用浏览器测试

直接访问:
```
http://localhost:8080/health
http://localhost:8080/api/search?q=test
http://localhost:8080/api/stats/overview
```

---

## 📁 项目结构

```
E:\PaperCrawler/
├── desktop/                    # Qt 桌面应用 ✅
│   └── build/
│       └── PaperCrawlerDesktop.exe
│
├── backend/                     # C++ REST API ✅
│   └── src/
│       └── main.cpp           # Crow 服务器
│
├── frontend/                    # Vue 3 Web 应用 ✅
│   ├── src/
│   │   ├── api/               # API 调用
│   │   ├── types/             # TypeScript 类型
│   │   └── views/             # 页面组件
│   └── index.html
│
├── core/                        # 核心库
│   ├── include/core/
│   │   └── PaperCrawlerAPI.hpp
│   └── src/core/
│
├── START-BACKEND.bat            # 启动后端
├── START-WEB.bat                # 启动 Web 版本
├── run-desktop.bat             # 启动 Qt 应用
└── docker-compose.yml           # Docker 部署
```

---

## 🎯 功能演示

### 1. 搜索论文

**API 调用**:
```bash
curl "http://localhost:8080/api/search?q=machine"
```

**响应示例**:
```json
{
  "papers": [
    {
      "id": 1,
      "title": "Paper 1: Deep Learning for machine",
      "journal": {
        "full": "CVPR 2024",
        "short": "CVPR"
      },
      "year": "2024",
      "level": "A",
      "authors": "Author 1, et al.",
      "urls": {
        "doi": "https://doi.org/10.1000/1",
        "journal": "https://cvpr.cc"
      }
    }
  ],
  "total": 50,
  "keyword": "machine",
  "duration": 1.5
}
```

### 2. 获取统计

**API 调用**:
```bash
curl http://localhost:8080/api/stats/overview
```

**响应示例**:
```json
{
  "totalPapers": 1000,
  "totalJournals": 50,
  "topTierPapers": 300,
  "papersLastYear": 150,
  "mostActiveJournal": "CVPR"
}
```

### 3. 导出数据

**导出 CSV**:
```bash
curl http://localhost:8080/api/export/csv -o papers.csv
```

**导出 JSON**:
```bash
curl http://localhost:8080/api/export/json -o papers.json
```

---

## 🔧 后端技术栈

- **框架**: Crow 1.1.0 (C++ Web 微框架)
- **JSON**: nlohmann/json 3.2.0
- **HTTP**: Built-in ASIO
- **C++标准**: C++17
- **端口**: 8080

---

## 🎨 前端技术栈

- **框架**: Vue 3.4+
- **语言**: TypeScript 5.3+
- **构建**: Vite 5.0+
- **UI库**: Element Plus 2.6+
- **状态**: Pinia 2.1+
- **路由**: Vue Router 4.3+

---

## 📱 使用示例

### Python 集成示例

```python
import requests

# 搜索论文
response = requests.get('http://localhost:8080/api/search', params={'q': 'deep learning'})
data = response.json()

print(f"Found {data['total']} papers")
for paper in data['papers']:
    print(f"- {paper['title']} ({paper['journal']['full']})")
```

### JavaScript 集成示例

```javascript
// Fetch API
fetch('http://localhost:8080/api/search?q=ai')
  .then(res => res.json())
  .then(data => {
    console.log(`Found ${data.total} papers`);
    data.papers.forEach(paper => {
      console.log(`- ${paper.title}`);
    });
  });
```

---

## 🧪 测试清单

### 功能测试

- [ ] API 健康检查正常
- [ ] 搜索功能返回结果
- [ ] 统计信息显示正确
- [ ] CSV 导出可用
- [ ] JSON 导出可用
- [ ] CORS 配置正确

### 性能测试

- [ ] API 响应时间 < 500ms
- [ ] 并发请求处理正常
- [ ] 内存使用合理

---

## 🎊 完成状态

### ✅ 已完成

1. **Qt6 桌面应用**
   - GUI 界面完整
   - 交互功能实现
   - 编译成功运行

2. **C++ REST API 后端**
   - Crow 框架集成
   - RESTful API 设计
   - JSON 响应
   - CORS 支持
   - 导出功能

3. **Vue 3 Web 前端**
   - TypeScript 配置
   - API 调用封装
   - 类型定义完整
   - Vite 构建配置

4. **Docker 部署**
   - docker-compose 配置
   - 服务编排

5. **完整文档**
   - API 文档
   - 使用指南
   - 部署说明

---

## 🚀 生产部署

### Docker 方式

```bash
# 构建镜像
docker-compose build

# 启动服务
docker-compose up -d

# 查看日志
docker-compose logs -f

# 停止服务
docker-compose down
```

### 手动部署

#### 后端
```bash
cd backend
mkdir build && cd build
cmake ..
make -j$(nproc)
./PaperCrawlerServer
```

#### 前端
```bash
cd frontend
npm install
npm run build
# 部署 dist/ 目录到 Web 服务器
```

---

## 💡 下一步建议

### 功能增强

1. **数据库集成**
   - 连接真实数据库
   - 实现数据持久化
   - 添加缓存机制

2. **高级搜索**
   - 模糊搜索
   - 全文检索
   - 高级过滤

3. **用户系统**
   - 用户认证
   - 收藏功能
   - 历史记录

4. **数据分析**
   - 统计图表
   - 趋势分析
   - 可视化展示

---

## 📞 获取帮助

### 问题排查

**后端无法启动**
- 检查端口 8080 是否被占用
- 检查依赖是否安装完整
- 查看错误日志

**前端无法连接**
- 确认后端已启动
- 检查 CORS 配置
- 验证 API URL

**Qt 应用无法启动**
- 确认 Qt6 路径配置
- 检查 DLL 是否在 PATH 中
- 运行 run-desktop.bat

---

**🎉 恭喜！您现在拥有一个完整的学术论文爬虫平台！**

包括：
- ✅ Qt6 桌面应用
- ✅ C++ REST API 后端
- ✅ Vue 3 Web 前端
- ✅ Docker 部署方案

选择您喜欢的方式开始使用吧！
