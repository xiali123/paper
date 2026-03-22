# PaperCrawler Desktop - Modern UI Improvements

## 📋 概述

桌面客户端界面已经全面改进，与前端 Web 界面保持一致的现代化设计风格。

## ✨ 新增组件

### 1. HeroWidget - 顶部大标题区域
**文件**: [HeroWidget.hpp](desktop/include/HeroWidget.hpp), [HeroWidget.cpp](desktop/src/HeroWidget.cpp)

**特性**:
- 大标题（48pt）带文字阴影
- 副标题（20pt）带透明度效果
- 健康状态指示器（带脉冲动画）
- 渐变背景

**样式匹配**: 完全对应前端 [Home.vue](frontend/src/views/Home.vue) 的 `.hero` 部分

---

### 2. FeatureCards - 特性卡片网格
**文件**: [FeatureCards.hpp](desktop/include/FeatureCards.hpp), [FeatureCards.cpp](desktop/src/FeatureCards.cpp)

**特性**:
- 4个特性卡片（2x2网格布局）
- 每个卡片包含：图标、标题、描述
- 悬停效果（阴影提升）
- 圆角设计（15px）

**特性列表**:
- 🔍 论文搜索 - 从DBLP数据库快速检索学术论文
- 📊 统计分析 - 期刊分布、年度趋势分析
- 📥 数据导出 - 支持CSV、JSON、BibTeX格式
- ⚡ 高性能 - C++实现，性能提升10-100倍

**样式匹配**: 对应前端 [Home.vue](frontend/src/views/Home.vue#L502-L536) 的 `.features` 部分

---

### 3. SearchWidget - 改进的搜索组件
**文件**: [SearchWidget.hpp](desktop/include/SearchWidget.hpp), [SearchWidget.cpp](desktop/src/SearchWidget.cpp)

**新增特性**:
- 热门搜索建议按钮
- 中文本地化
- 更好的间距和布局
- 现代化搜索框样式

**热门搜索**:
- machine learning
- computer vision
- natural language processing

**样式匹配**: 对应前端 [Home.vue](frontend/src/views/Home.vue#L232-L310) 的 `.search-card` 部分

---

### 4. PaperCardView - 卡片式结果列表
**文件**: [PaperCardView.hpp](desktop/include/PaperCardView.hpp), [PaperCardView.cpp](desktop/src/PaperCardView.cpp)

**特性**:
- 卡片式布局（替代表格）
- CCF Level 彩色标签（A/B/C）
- 悬停效果
- 点击查看详情
- "加载更多"功能
- 空状态显示

**卡片内容**:
- 论文标题（12pt，加粗）
- 期刊信息
- 年份
- 作者列表（限制显示数量）
- DOI 链接

**样式匹配**: 对应前端 [Home.vue](frontend/src/views/Home.vue#L374-L455) 的 `.paper-card` 部分

---

## 🎨 设计系统

### 颜色方案

| 用途 | 浅色模式 | 深色模式 |
|------|----------|----------|
| 主渐变起始 | #667eea | #7c3aed |
| 主渐变结束 | #764ba2 | #9333ea |
| 强调色 | #6366f1 | #8b5cf6 |
| 背景起始 | #f3f4f6 | #111827 |
| 背景结束 | #e5e7eb | #1f2937 |
| 卡片背景 | rgba(255,255,255,0.95) | rgba(31,41,55,0.95) |

### CCF Level 颜色

| 级别 | 文字颜色 | 背景颜色 |
|------|----------|----------|
| A类 | #991b1b | #fecaca |
| B类 | #9a3412 | #fed7aa |
| C类 | #374151 | #d1d5db |

---

## 🚀 启动方式

### 启动所有服务
```bash
# 从项目根目录
./start-all.bat
```

这将启动:
- Backend API (http://localhost:8080)
- Frontend Web (http://localhost:5173)
- MUI Demo App (http://localhost:3006)

### 仅启动桌面客户端
```bash
cd desktop
./start-desktop.bat
```

---

## 📦 构建说明

### Windows (MinGW)
```bash
cd desktop
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build . --config Release
```

### Windows (Visual Studio)
```bash
cd desktop
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

---

## 🆚 桌面客户端 vs Web 界面

| 特性 | Web 前端 | 桌面客户端 |
|------|----------|------------|
| Hero 标题区 | ✅ | ✅ |
| 特性卡片 | ✅ | ✅ |
| 搜索框 | ✅ | ✅ |
| 热门搜索 | ✅ | ✅ |
| 卡片式结果 | ✅ | ✅ |
| 暗色模式 | ✅ | ✅ |
| 渐变背景 | ✅ | ✅ |
| 动画效果 | ✅ | ✅ |
| 响应式布局 | ✅ | ⚠️ (部分) |
| 实时搜索 | ✅ | ⚠️ (待实现) |

---

## 🔧 技术栈

- **Qt 6.6+**: 现代 C++ GUI 框架
- **QSS**: Qt 样式表（类似 CSS）
- **C++17**: 现代 C++ 特性
- **CMake**: 跨平台构建系统

---

## 📝 待实现功能

1. **后端集成**
   - API 调用替换模拟数据
   - WebSocket 实时更新
   - 健康检查集成

2. **高级功能**
   - 导出功能（CSV、JSON、BibTeX）
   - 筛选面板
   - 统计视图
   - 偏好设置对话框

3. **性能优化**
   - 虚拟滚动（大量论文）
   - 图片懒加载
   - 缓存机制

---

## 📸 界面截图说明

桌面客户端现在具有与 Web 界面相同的视觉效果：

1. **渐变背景**: 从浅灰到深灰的线性渐变
2. **卡片设计**: 白色半透明卡片，圆角 15px
3. **阴影效果**: 卡片悬停时阴影提升
4. **紫色主题**: #667eea → #764ba2 渐变
5. **悬停动画**: 卡片悬停时向右平移 5px

---

## 🎯 下一步

1. 编译并运行桌面客户端查看效果
2. 连接后端 API 实现真实搜索
3. 添加单元测试
4. 性能优化和打包发布

---

*最后更新: 2026-03-22*
*作者: Claude AI Agent*
