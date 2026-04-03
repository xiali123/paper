# 🚀 快速启动指南 - PaperCrawler端到端测试

**当前状态**: ✅ 完整MVP已交付，准备测试  
**测试时间**: 约15-20分钟  
**难度等级**: ⭐ 简单

---

## 📋 快速启动步骤

### 方式1：一键启动（推荐）

```bash
# 运行自动启动脚本
bash start_e2e_test.sh
```

脚本将自动：
1. ✅ 启动后端服务器（端口8080）
2. ✅ 启动前端开发服务器（端口5173）
3. ✅ 运行API测试
4. ✅ 打开浏览器到AI审稿页面

### 方式2：手动启动

#### Step 1: 启动后端

```bash
# 进入后端目录
cd backend

# 启动服务器
./build/Release/PaperCrawlerServer.exe
```

**预期输出**:
```
PaperCrawler Backend Server
============================
Listening on port 8080...
API Base URL: http://localhost:8080
```

#### Step 2: 启动前端（新终端）

```bash
# 进入前端目录
cd frontend

# 启动开发服务器
npm run dev
```

**预期输出**:
```
VITE v5.x.x  ready in xxx ms
➜  Local:   http://localhost:5173/
```

#### Step 3: 运行API测试（新终端）

```bash
# 运行测试脚本
cd backend
bash test_api_e2e.sh
```

**预期输出**:
```
================================================
Test Summary
================================================
Total Tests: 7
Passed: 7
Failed: 0
Status: ALL TESTS PASSED ✓
```

#### Step 4: 打开浏览器测试

访问以下URL进行手动测试：

1. **AI审稿人**: http://localhost:5173/ai/review
2. **文献综述**: http://localhost:5173/ai/literature-review
3. **研究计划**: http://localhost:5173/ai/research-plan

---

## ✅ 测试检查清单

### 后端测试

- [ ] 服务器启动成功（http://localhost:8080）
- [ ] 健康检查通过（http://localhost:8080/api/health）
- [ ] API端点可访问（7个测试全部通过）
- [ ] WebSocket服务运行（ws://localhost:8080/ws）

### 前端测试

- [ ] 开发服务器启动（http://localhost:5173）
- [ ] AI审稿页面加载正常
- [ ] 文献综述页面加载正常
- [ ] 研究计划页面加载正常

### 功能测试

- [ ] AI审稿表单填写成功
- [ ] 文献综述表单填写成功
- [ ] 研究计划表单填写成功
- [ ] 进度条正常显示
- [ ] 结果展示正常

---

## 🧪 快速功能测试

### 测试1：AI审稿人（2分钟）

```
1. 访问：http://localhost:5173/ai/review
2. 填写表单：
   - 论文：选择任意论文
   - 期刊：输入 "Nature"
   - 领域：选择 "Computer Science"
   - 风格：选择 "Balanced"
3. 点击"生成审稿报告"
4. 观察：进度条更新（~15秒）
5. 验证：显示审稿结果卡片
```

### 测试2：文献综述（2分钟）

```
1. 访问：http://localhost:5173/ai/literature-review
2. 填写表单：
   - 标题：输入 "Deep Learning in NLP"
   - 领域：选择 "Computer Science"
   - 论文数：输入 "50"
   - 关键词：输入 "AI, machine learning"
3. 点击"生成综述"
4. 观察：进度条更新（~20秒）
5. 验证：显示主题聚类和研究空白
```

### 测试3：研究计划（2分钟）

```
1. 访问：http://localhost:5173/ai/research-plan
2. 填写表单：
   - 标题：输入 "AI Medical Imaging"
   - 领域：选择 "Medicine"
   - 周期：输入 "12"
   - 预算：输入 "50000"
3. 点击"生成研究计划"
4. 观察：进度条更新（~18秒）
5. 验证：显示可行性分析和时间线
```

---

## 🐛 常见问题排查

### 问题1：后端启动失败

**症状**: `Cannot find ./build/Release/PaperCrawlerServer.exe`

**解决方案**:
```bash
cd backend
mkdir -p build && cd build
cmake ..
cmake --build . --config Release
./Release/PaperCrawlerServer.exe
```

### 问题2：端口被占用

**症状**: `Port 8080/5173 is already in use`

**解决方案**:
```bash
# 查找并杀死占用端口的进程
netstat -ano | findstr ":8080"
taskkill /PID <PID> /F
```

### 问题3：API测试失败

**症状**: `Backend server is not accessible`

**解决方案**:
```bash
# 检查后端是否运行
curl http://localhost:8080/api/health

# 查看后端日志
# Windows: Type 'backend.log' in terminal
# Linux/Mac: tail -f backend.log
```

### 问题4：前端页面空白

**症状**: 浏览器显示空白页面

**解决方案**:
```bash
# 检查前端开发服务器是否运行
curl http://localhost:5173

# 清除浏览器缓存
# 按 Ctrl+Shift+Delete

# 检查浏览器控制台（F12）
# 查看是否有错误信息
```

---

## 📊 测试成功标准

### 后端测试成功

```
✓ 所有7个API测试通过
✓ 健康检查返回200
✓ 响应时间<30s
✓ 无错误日志
```

### 前端测试成功

```
✓ 所有页面正常加载
✓ 表单可以正常填写
✓ 进度条正常更新
✓ 结果正常显示
```

### 集成测试成功

```
✓ 前端可以调用后端API
✓ 数据格式一致
✓ 错误正确处理
✓ 性能指标达标
```

---

## 🎉 测试完成后

### 如果测试通过 ✅

恭喜！系统已经完全就绪，可以：

1. ✅ 进行更深入的功能测试
2. ✅ 开始Beta用户测试准备
3. ✅ 规划公开发布

### 如果测试失败 ❌

1. 查看错误日志
2. 参考问题排查部分
3. 重新运行测试
4. 如需要帮助，查看详细测试指南：`E2E_API_TEST_GUIDE.md`

---

## 📞 需要帮助？

**详细文档**: 
- 测试指南：`E2E_API_TEST_GUIDE.md`
- 集成报告：`FRONTEND_BACKEND_INTEGRATION_REPORT.md`
- 最终报告：`FINAL_DELIVERY_REPORT.md`

**快速命令**:
```bash
# 查看后端日志
cat backend.log

# 查看前端日志
cat frontend.log

# 检查端口占用
netstat -ano | findstr ":8080"
netstat -ano | findstr ":5173"
```

---

**祝你测试顺利！🚀**

**PaperCrawler Team** - 让AI成为研究者的第二大脑
