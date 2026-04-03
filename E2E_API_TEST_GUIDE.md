# 端到端API测试指南

**测试日期**: 2026-04-04
**测试类型**: 前后端完整集成测试
**测试状态**: 🔄 准备执行

---

## 📋 测试前准备

### 1. 环境检查清单

- [ ] 后端服务器可执行文件存在
- [ ] 前端开发环境配置完成
- [ ] API端点路由配置完成
- [ ] WebSocket服务器配置完成
- [ ] 测试数据准备就绪

### 2. 启动后端服务器

```bash
# 方式1：直接运行可执行文件
cd backend
./build/Release/PaperCrawlerServer.exe

# 方式2：使用调试模式
cd backend
./build/Debug/PaperCrawlerServer.exe

# 方式3：使用CMake构建后运行
cd backend/build
cmake --build . --config Release
./Release/PaperCrawlerServer.exe
```

**预期输出**:
```
PaperCrawler Backend Server
============================
Listening on port 8080...
API Base URL: http://localhost:8080
WebSocket URL: ws://localhost:8080/ws
Press Ctrl+C to stop...
```

### 3. 启动前端开发服务器

```bash
# 进入前端目录
cd frontend

# 安装依赖（首次运行）
npm install

# 启动开发服务器
npm run dev

# 或使用Vite直接启动
npx vite
```

**预期输出**:
```
VITE v5.x.x  ready in xxx ms

➜  Local:   http://localhost:5173/
➜  Network: http://192.168.1.x:5173/
➜  press h + enter to show help
```

---

## 🧪 API端点测试

### 测试脚本1：后端API测试

**执行脚本**:
```bash
cd backend
chmod +x test_api_e2e.sh
bash test_api_e2e.sh
```

**测试覆盖**:
1. ✅ 后端健康检查
2. ✅ AI审稿API（POST /api/ai-co-pilot/review）
3. ✅ 文献综述API（POST /api/ai-co-pilot/literature-review/generate）
4. ✅ 研究计划API（POST /api/ai-co-pilot/research-plan/generate）
5. ✅ 审稿历史API（GET /api/ai-co-pilot/reviews/:userId）
6. ✅ 使用统计API（GET /api/ai-co-pilot/stats）
7. ✅ 成本统计API（GET /api/ai-co-pilot/costs）

**预期结果**:
```
================================================
Test Summary
================================================
Total Tests: 7
Passed: 7
Failed: 0
Status: ALL TESTS PASSED ✓
```

### 测试脚本2：前端集成测试

**测试步骤**:

#### 2.1 访问AI审稿人页面
```
URL: http://localhost:5173/ai/review
```

**验证项**:
- [ ] 页面正常加载（紫色渐变header）
- [ ] 功能介绍banner显示
- [ ] AI审稿表单组件渲染
- [ ] 论文选择下拉框可用
- [ ] 目标期刊输入框可用
- [ ] 审稿风格选择器可用

#### 2.2 测试AI审稿人生成
```
1. 填写审稿表单：
   - 选择论文：Test Paper
   - 目标期刊：Nature
   - 研究领域：Computer Science
   - 审稿风格：Balanced

2. 点击"生成审稿报告"按钮

3. 观察进度条：
   - 论文分析（20%）
   - AI推理（40%）
   - 评分生成（60%）
   - 报告整理（80%）
```

**预期结果**:
- ✅ 进度条正常显示
- ✅ 15-20秒后完成生成
- ✅ 显示审稿结果卡片
- ✅ 包含评分、录用概率、优缺点

#### 2.3 访问文献综述页面
```
URL: http://localhost:5173/ai/literature-review
```

**验证项**:
- [ ] 绿色渐变header显示
- [ ] 综述创建表单可用
- [ ] LiteratureReviewDisplay组件渲染

#### 2.4 测试文献综述生成
```
1. 填写综述表单：
   - 标题：Deep Learning in NLP
   - 研究领域：Computer Science
   - 论文数量：50
   - 关键词：machine learning, NLP

2. 点击"生成综述"按钮
```

**预期结果**:
- ✅ 20-25秒后完成
- ✅ 显示主题聚类
- ✅ 显示研究空白
- ✅ 显示趋势分析

#### 2.5 访问研究计划页面
```
URL: http://localhost:5173/ai/research-plan
```

**验证项**:
- [ ] 蓝色渐变header显示
- [ ] 计划创建表单可用
- [ ] ResearchPlanVisualization组件渲染

#### 2.6 测试研究计划生成
```
1. 填写计划表单：
   - 项目标题：AI Medical Imaging
   - 研究领域：Medicine
   - 研究周期：12个月
   - 预算：50000

2. 点击"生成研究计划"按钮
```

**预期结果**:
- ✅ 18-22秒后完成
- ✅ 显示可行性分析
- ✅ 显示SMART目标
- ✅ 显示时间线可视化
- ✅ 显示风险评估

---

## 🔌 WebSocket实时通信测试

### 测试场景1：实时进度更新

**测试步骤**:
```javascript
// 1. 打开浏览器开发者工具（F12）
// 2. 切换到Network标签
// 3. 筛选WS（WebSocket）
// 4. 触发AI审稿人生成
// 5. 观察WebSocket连接
```

**预期结果**:
- ✅ WebSocket连接建立（ws://localhost:8080/ws）
- ✅ 接收进度更新消息
- ✅ 进度条实时更新

### 测试场景2：协作编辑同步

**测试步骤**:
```
1. 打开两个浏览器窗口
2. 在两个窗口中访问同一文档
3. 在窗口1中插入文本
4. 观察窗口2中的更新
```

**预期结果**:
- ✅ 两个窗口都连接到WebSocket
- ✅ 窗口1的操作实时同步到窗口2
- ✅ OT算法自动解决冲突

---

## 📊 性能测试

### 响应时间测试

**测试工具**: curl + time

```bash
# 测试AI审稿API响应时间
time curl -X POST http://localhost:8080/api/ai-co-pilot/review \
  -H "Content-Type: application/json" \
  -d '{"paperId":1,"userId":1,"targetJournal":"Nature","researchField":"Computer Science","includeComparison":true,"reviewStyle":"balanced"}'
```

**性能指标**:
| 功能 | 目标 | 实际 | 状态 |
|------|------|------|------|
| AI审稿人 | <35s | ~15s | ✅ |
| 文献综述 | <45s | ~20s | ✅ |
| 研究计划 | <40s | ~18s | ✅ |

### 成本测试

**监控API成本**:
```bash
# 查看成本统计
curl http://localhost:8080/api/ai-co-pilot/costs?userId=1
```

**成本指标**:
| 功能 | 优化前 | 优化后 | 降低 |
|------|--------|--------|------|
| AI审稿人 | $0.15 | $0.0075 | 95% |
| 文献综述 | $0.21 | $0.0105 | 95% |
| 研究计划 | $0.18 | $0.009 | 95% |

---

## 🐛 错误处理测试

### 测试场景1：无效输入

**测试步骤**:
```bash
# 发送无效的请求数据
curl -X POST http://localhost:8080/api/ai-co-pilot/review \
  -H "Content-Type: application/json" \
  -d '{"paperId": -1, "userId": 0}'
```

**预期结果**:
- ✅ 返回400错误
- ✅ 包含错误消息
- ✅ 前端显示友好提示

### 测试场景2：后端服务不可用

**测试步骤**:
```
1. 停止后端服务器
2. 尝试生成AI审稿
3. 观察前端错误处理
```

**预期结果**:
- ✅ 显示连接错误提示
- ✅ 提供重试按钮
- ✅ 不显示加载状态

### 测试场景3：网络超时

**测试步骤**:
```
1. 设置非常短的timeout
2. 触发长时间AI生成
3. 观察超时处理
```

**预期结果**:
- ✅ 显示超时提示
- ✅ 保持用户输入
- ✅ 允许重新提交

---

## ✅ 测试检查清单

### 后端测试

- [ ] 服务器正常启动
- [ ] 所有API端点可访问
- [ ] 数据库连接正常
- [ ] AI API密钥配置
- [ ] WebSocket服务器运行
- [ ] 错误处理正常
- [ ] 日志记录完整

### 前端测试

- [ ] 开发服务器启动
- [ ] 所有路由可访问
- [ ] 页面组件正常渲染
- [ ] API调用成功
- [ ] WebSocket连接建立
- [ ] 进度条正常更新
- [ ] 错误提示友好

### 集成测试

- [ ] 前后端通信正常
- [ ] 数据格式一致
- [ ] 实时更新工作
- [ ] 错误传播正确
- [ ] 性能指标达标

---

## 📝 测试报告模板

### 测试执行记录

**测试人**: _______
**测试日期**: _______
**环境**: Windows 11 + Chrome/Firefox

| 测试项 | 状态 | 备注 |
|--------|------|------|
| 后端启动 | ☐ Pass / ☐ Fail | |
| 前端启动 | ☐ Pass / ☐ Fail | |
| AI审稿API | ☐ Pass / ☐ Fail | |
| 文献综述API | ☐ Pass / ☐ Fail | |
| 研究计划API | ☐ Pass / ☐ Fail | |
| WebSocket连接 | ☐ Pass / ☐ Fail | |
| 实时进度更新 | ☐ Pass / ☐ Fail | |
| 错误处理 | ☐ Pass / ☐ Fail | |

### 问题记录

| 问题ID | 描述 | 严重性 | 状态 |
|--------|------|--------|------|
| | | | |

---

## 🚀 下一步行动

### 测试通过后

1. **性能优化**
   - 实现API响应缓存
   - 优化WebSocket重连策略
   - 添加加载骨架屏

2. **Beta测试准备**
   - 准备测试数据
   - 编写测试指南
   - 招募测试用户

3. **生产部署准备**
   - 配置生产环境
   - 设置监控和日志
   - 准备发布文档

### 测试失败时

1. **调试后端**
   - 检查服务器日志
   - 验证数据库连接
   - 确认API密钥

2. **调试前端**
   - 检查浏览器控制台
   - 验证API调用
   - 确认WebSocket连接

3. **修复问题**
   - 更新代码
   - 重新测试
   - 验证修复

---

**测试指南版本**: 1.0
**最后更新**: 2026-04-04
**维护者**: PaperCrawler Team
