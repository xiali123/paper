# PaperCrawler 前端-后端集成测试指南

## 测试前准备

### 1. 确保后端服务运行
```bash
# 确保后端服务在 http://localhost:8080 运行
curl http://localhost:8080/health
```

### 2. 安装前端依赖
```bash
cd e:/PaperCrawler/frontend
npm install
```

### 3. 启动前端开发服务器
```bash
npm run dev
```

## 功能测试清单

### 基础功能测试

#### 1. 健康检查测试
- [ ] 访问首页 `http://localhost:5173/`
- [ ] 检查右上角健康状态指示器
- [ ] 应显示绿色 "服务正常" 状态
- [ ] 每60秒自动更新一次

#### 2. 搜索功能测试
**首页快速搜索**
- [ ] 在搜索框输入 "machine learning"
- [ ] 点击 "搜索" 按钮或按 Enter 键
- [ ] 验证搜索结果正确显示
- [ ] 检查论文数量和耗时显示
- [ ] 点击热门搜索建议验证功能

**搜索页面**
- [ ] 访问 `http://localhost:5173/search`
- [ ] 测试实时搜索（输入时自动搜索）
- [ ] 测试年份过滤（2024, 2023, 2022）
- [ ] 测试等级过滤（CCF-A, CCF-B, CCF-C）
- [ ] 验证 "加载更多" 功能
- [ ] 点击论文项跳转到详情页

#### 3. 论文详情测试
- [ ] 从搜索结果点击任意论文
- [ ] 验证详情页正确显示论文信息
- [ ] 测试 "返回" 按钮
- [ ] 测试 "复制 BibTeX" 功能
- [ ] 测试相关论文链接

#### 4. 统计功能测试
- [ ] 访问 `http://localhost:5173/stats`
- [ ] 验证统计卡片显示正确数据
- [ ] 测试 "刷新数据" 按钮
- [ ] 测试 "导出 CSV" 按钮
- [ ] 测试 "导出 JSON" 按钮

### 错误处理测试

#### 1. 网络错误测试
- [ ] 停止后端服务
- [ ] 尝试搜索论文
- [ ] 验证显示友好的错误提示
- [ ] 重启后端服务
- [ ] 点击 "重试" 按钮验证恢复

#### 2. 无结果测试
- [ ] 搜索不可能存在的关键词（如 "xyzabc123"）
- [ ] 验证显示 "未找到相关论文" 空状态
- [ ] 验证提示用户尝试其他关键词

#### 3. 无效输入测试
- [ ] 尝试搜索空字符串
- [ ] 验证显示 "请输入搜索关键词" 提示
- [ ] 尝试访问不存在的论文 ID
- [ ] 验证显示友好的错误页面

### 性能测试

#### 1. 搜索性能
- [ ] 搜索热门关键词（大量结果）
- [ ] 记录搜索耗时（应 < 2秒）
- [ ] 验证分页加载流畅

#### 2. 实时搜索性能
- [ ] 快速输入多个关键词
- [ ] 验证防抖功能正常（500ms）
- [ ] 确认不会频繁发送请求

#### 3. 加载状态
- [ ] 验证所有加载操作显示加载指示器
- [ ] 验证加载期间按钮禁用
- [ ] 验证加载完成后状态正确更新

## API 端点测试

### 手动测试 API 端点

```bash
# 1. 健康检查
curl http://localhost:8080/health

# 2. 搜索论文
curl "http://localhost:8080/api/search?q=machine%20learning&limit=10"

# 3. 获取论文详情
curl http://localhost:8080/api/papers/1

# 4. 获取统计信息
curl http://localhost:8080/api/stats/overview

# 5. 导出 CSV
curl http://localhost:8080/api/export/csv -o test.csv

# 6. 导出 JSON
curl http://localhost:8080/api/export/json -o test.json
```

## 浏览器控制台测试

### 1. 打开浏览器开发者工具
- 按 F12 或右键 -> 检查
- 切换到 Console 标签

### 2. 测试 Composables
```javascript
// 测试搜索 composable
import { useSearch } from '/src/composables/useSearch.ts'
const search = useSearch()
await search.performSearch('deep learning')
console.log(search.results.value)

// 测试统计 composable
import { useStats } from '/src/composables/useStats.ts'
const stats = useStats()
await stats.fetchOverview()
console.log(stats.overview.value)
```

### 3. 测试 API 调用
```javascript
// 测试 paperApi
import { paperApi } from '/src/api/index.ts'
const result = await paperApi.search({ q: 'AI' })
console.log(result)

// 测试 statsApi
import { statsApi } from '/src/api/index.ts'
const stats = await statsApi.getOverview()
console.log(stats)
```

## 响应式设计测试

### 1. 桌面视图 (1920x1080)
- [ ] 验证布局正常
- [ ] 验证卡片网格显示正确
- [ ] 验证搜索功能正常

### 2. 平板视图 (768x1024)
- [ ] 验证布局自适应
- [ ] 验证卡片网格调整为2列
- [ ] 验证所有功能可访问

### 3. 移动视图 (375x667)
- [ ] 验证单列布局
- [ ] 验证触摸操作正常
- [ ] 验证按钮大小合适

## 可访问性测试

### 1. 键盘导航
- [ ] 使用 Tab 键导航所有交互元素
- [ ] 使用 Enter 键激活按钮和链接
- [ ] 验证焦点指示器可见

### 2. 屏幕阅读器
- [ ] 验证所有图片有 alt 文本
- [ ] 验证表单元素有标签
- [ ] 验证语义化 HTML 结构

## 性能指标测试

### 1. Core Web Vitals
- [ ] LCP (Largest Contentful Paint) < 2.5s
- [ ] FID (First Input Delay) < 100ms
- [ ] CLS (Cumulative Layout Shift) < 0.1

### 2. Lighthouse 测试
- [ ] Performance > 90
- [ ] Accessibility > 90
- [ ] Best Practices > 90
- [ ] SEO > 90

## 兼容性测试

### 浏览器兼容性
- [ ] Chrome (最新版本)
- [ ] Firefox (最新版本)
- [ ] Safari (最新版本)
- [ ] Edge (最新版本)

### 测试步骤
1. 在每个浏览器中运行所有功能测试
2. 记录任何兼容性问题
3. 验证 polyfills 正常工作

## 已知问题和限制

### 1. 当前限制
- 实时搜索默认禁用（可通过配置启用）
- 导出功能依赖后端实现
- BibTeX 导出格式可能需要调整

### 2. 待优化项
- 添加请求缓存机制
- 实现离线功能
- 添加更多导出格式

## 测试报告模板

```
测试日期: ___________
测试人员: ___________
浏览器版本: ___________

功能测试通过率: _____ / _____

发现问题:
1. ___________________
2. ___________________

性能测试结果:
- 首次加载时间: _____ ms
- 搜索平均耗时: _____ ms
- Lighthouse 分数: _____

建议改进:
1. ___________________
2. ___________________
```

## 后续步骤

完成测试后：
1. 记录所有发现的问题
2. 优先修复阻塞性问题
3. 创建 GitHub Issues
4. 计划下一轮测试
