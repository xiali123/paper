# PaperCrawler 测试指南

本文档提供详细的测试步骤，帮助您验证PaperCrawler平台的各项功能。

---

## 🧪 测试环境准备

### 1. 数据库准备

```bash
# 启动MySQL
sudo systemctl start mysql

# 创建数据库
mysql -u root -p < sql/init.sql
```

### 2. 配置文件

编辑 `config/config.json`:
```json
{
    "database": {
        "host": "localhost",
        "port": 3306,
        "user": "root",
        "password": "your_password",
        "database": "csdatabs"
    }
}
```

---

## 📋 测试清单

### Phase 1: 核心库测试

#### ✅ 编译核心库
```bash
cd core
mkdir build && cd build
cmake ..
make -j$(nproc)
```

**预期结果**: 编译成功，生成 `libPaperCrawlerCore.so` (Linux) 或 `PaperCrawlerCore.dll` (Windows)

#### ✅ 功能测试
创建测试文件 `test_core.cpp`:
```cpp
#include "core/PaperCrawlerAPI.hpp"
#include <iostream>

int main() {
    try {
        auto& api = PaperCrawler::PaperCrawlerAPI::getInstance();
        api.initialize("config/config.json");

        std::cout << "✅ Core library initialized successfully" << std::endl;

        auto stats = api.getStatistics();
        std::cout << "Total papers: " << stats.totalPapers << std::endl;
        std::cout << "Total journals: " << stats.totalJournals << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
```

---

### Phase 2: REST API后端测试

#### ✅ 编译后端
```bash
cd backend
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### ✅ 启动服务
```bash
./PaperCrawlerServer 8080
```

**预期输出**:
```
========================================
  PaperCrawler REST API Server v1.0.0
========================================
PaperCrawler initialized successfully
Server starting on port 8080...
```

#### ✅ API端点测试

**1. 健康检查**
```bash
curl http://localhost:8080/health
```

**预期响应**:
```json
{
  "status": "ok",
  "service": "PaperCrawler API",
  "version": "1.0.0"
}
```

**2. 搜索论文**
```bash
curl "http://localhost:8080/api/search?q=dma&max=100"
```

**预期响应**:
```json
{
  "papers": [...],
  "total": 1234,
  "keyword": "dma",
  "duration": 2.45
}
```

**3. 获取统计**
```bash
curl http://localhost:8080/api/stats/overview
```

**4. 导出CSV**
```bash
curl "http://localhost:8080/api/export/csv?type=dma" -o papers.csv
```

---

### Phase 3: Vue前端测试

#### ✅ 安装依赖
```bash
cd frontend
npm install
```

#### ✅ 启动开发服务器
```bash
npm run dev
```

**预期输出**:
```
VITE v5.0.0  ready in 500 ms

➜  Local:   http://localhost:5173/
➜  Network: use --host to expose
```

#### ✅ 功能测试

**1. 访问首页**
打开浏览器访问 `http://localhost:5173`

**检查项**:
- [ ] 页面正常显示
- [ ] 搜索框可见
- [ ] 样式加载正确

**2. 搜索测试**
- 输入关键词 "dma"
- 点击搜索按钮

**预期结果**:
- 显示loading状态
- 显示搜索结果
- 显示论文数量

**3. 响应式测试**
- 调整浏览器窗口大小
- 在移动设备视图下测试

---

### Phase 4: Qt桌面应用测试

#### ✅ 编译桌面应用
```bash
cd desktop
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### ✅ 运行应用
```bash
./PaperCrawlerDesktop
```

#### ✅ 功能测试

**1. 界面测试**
- [ ] 主窗口正常显示
- [ ] 搜索框可用
- [ ] 结果表格可见
- [ ] 进度条初始隐藏

**2. 搜索测试**
- 输入关键词 "dma"
- 点击搜索按钮

**检查项**:
- [ ] 进度条显示
- [ ] 搜索结果填充表格
- [ ] 状态栏更新
- [ ] 无崩溃

**3. 交互测试**
- 点击结果行
- 测试导出功能
- 切换主题

---

### Phase 5: Docker部署测试

#### ✅ 构建镜像
```bash
docker-compose build
```

**预期结果**: 所有服务镜像构建成功

#### ✅ 启动服务
```bash
docker-compose up -d
```

**检查容器状态**:
```bash
docker-compose ps
```

**预期输出**:
```
NAME                      STATUS
papercrawler-backend      Up
papercrawler-frontend     Up
papercrawler-mysql        Up
```

#### ✅ 端到端测试
```bash
# 测试后端
curl http://localhost:8080/health

# 测试前端
curl http://localhost/
```

#### ✅ 查看日志
```bash
docker-compose logs -f backend
docker-compose logs -f frontend
```

---

## 🔍 详细测试用例

### 测试用例1: 基本搜索流程

**前置条件**: API服务器运行中

**步骤**:
1. 发送搜索请求: `curl "http://localhost:8080/api/search?q=test"`
2. 检查响应状态码为200
3. 验证返回的JSON格式正确
4. 检查papers数组不为空
5. 验证total字段正确

**预期结果**: ✅ 所有检查通过

### 测试用例2: 数据库连接

**步骤**:
1. 检查MySQL服务运行
2. 测试数据库连接
3. 检查表结构
4. 验证数据完整性

```bash
mysql -u root -p -e "USE csdatabs; SHOW TABLES;"
```

### 测试用例3: 错误处理

**步骤**:
1. 测试无效关键词
2. 测试数据库连接失败
3. 测试网络超时
4. 验证错误消息

**测试**:
```bash
curl "http://localhost:8080/api/search?q="  # 空关键词
```

**预期**: 返回400错误和错误消息

---

## 📊 性能测试

### 响应时间测试
```bash
# 安装Apache Bench
sudo apt-get install apache2-utils

# 测试API性能
ab -n 1000 -c 10 http://localhost:8080/health
```

**预期结果**:
- Requests per second: > 100
- Time per request: < 100ms (95%)

### 并发测试
```bash
# 10个并发用户
ab -n 1000 -c 10 http://localhost:8080/api/search?q=test
```

---

## 🐛 已知问题与解决

### 问题1: 编译错误 - 找不到Qt
**解决**:
```bash
export CMAKE_PREFIX_PATH="/path/to/Qt/6.5.0/gcc_64"
```

### 问题2: 数据库连接失败
**解决**:
```bash
# 检查MySQL服务
sudo systemctl status mysql

# 检查端口
sudo netstat -tlnp | grep 3306
```

### 问题3: 前端API调用失败
**解决**:
检查 `vite.config.ts` 中的proxy配置

---

## ✅ 测试报告模板

```markdown
## 测试报告 - [日期]

### 测试环境
- OS: [操作系统]
- Compiler: [编译器版本]
- Qt: [Qt版本]
- Node.js: [Node版本]

### 测试结果
- 核心库: ✅/❌
- REST API: ✅/❌
- Vue前端: ✅/❌
- Qt桌面: ✅/❌
- Docker部署: ✅/❌

### 发现的问题
1. [问题描述]
   - 重现步骤:
   - 期望结果:
   - 实际结果:

### 建议改进
- [改进建议]
```

---

## 🎯 测试完成标准

当以下所有项目都通过时，可以认为测试完成：

- [ ] 所有模块编译成功
- [ ] API健康检查通过
- [ ] 搜索功能正常工作
- [ ] 数据正确存储到数据库
- [ ] 前端界面正常显示
- [ ] 桌面应用可正常运行
- [ ] Docker部署成功
- [ ] 性能指标达标
- [ ] 无严重bug
- [ ] 文档完整

---

**测试完成后，请填写测试报告并提交！**
