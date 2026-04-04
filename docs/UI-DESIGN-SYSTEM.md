# PaperCrawler UI Design System
## 学术论文管理系统 - 完整UI组件系统与视觉规范

**版本**: 2.0.0
**更新日期**: 2026-04-04
**设计师**: UI Designer Agent
**状态**: 生产就绪

---

## 目录

1. [设计理念](#设计理念)
2. [视觉基础系统](#视觉基础系统)
3. [核心组件库](#核心组件库)
4. [布局系统](#布局系统)
5. [响应式设计](#响应式设计)
6. [交互设计规范](#交互设计规范)
7. [可访问性标准](#可访问性标准)
8. [动画与过渡](#动画与过渡)
9. [组件实现指南](#组件实现指南)
10. [设计资源](#设计资源)

---

## 设计理念

### 核心原则

**1. 数据优先 (Data-First)**
- 专为学术数据展示优化
- 清晰的信息层级，快速浏览
- 支持大量数据的高效渲染

**2. 专业可信 (Professional & Trustworthy)**
- 学术严谨性的视觉表达
- 一致性和可预测性
- 高质量的设计细节

**3. 高效交互 (Efficient Interaction)**
- 减少操作步骤
- 智能默认值
- 键盘友好

**4. 可扩展性 (Scalability)**
- 模块化组件架构
- 主题系统支持
- 国际化准备

### 设计语言特征

- **现代学术风格**: 清爽、专业、不干扰内容
- **色彩克制**: 使用色彩引导而非分散注意力
- **信息密度**: 平衡的信息密度，避免过度拥挤
- **视觉层级**: 清晰的视觉层级引导用户注意力

---

## 视觉基础系统

### 色彩系统

#### 主色调 (Primary Colors)
```css
/* 学术蓝 - 传达专业、可信 */
--primary-50: #e3f2fd;    /* 浅背景 */
--primary-100: #bbdefb;   /* 悬停背景 */
--primary-200: #90caf9;   /* 边框 */
--primary-300: #64b5f6;   /* 禁用状态 */
--primary-400: #42a5f5;   /* 次要按钮 */
--primary-500: #2196f3;   /* 主按钮、链接 */
--primary-600: #1e88e5;   /* 按钮悬停 */
--primary-700: #1976d2;   /* 按钮激活 */
--primary-800: #1565c0;   /* 深色文本 */
--primary-900: #0d47a1;   /* 标题强调 */
```

#### 辅助色 (Secondary Colors)
```css
/* 学术紫 - 辅助强调 */
--secondary-50: #f3e5f5;
--secondary-100: #e1bee7;
--secondary-200: #ce93d8;
--secondary-300: #ba68c8;
--secondary-400: #ab47bc;
--secondary-500: #9c27b0;  /* 辅助按钮 */
--secondary-600: #8e24aa;
--secondary-700: #7b1fa2;
--secondary-800: #6a1b9a;
--secondary-900: #4a148c;
```

#### 语义色 (Semantic Colors)
```css
/* 成功状态 */
--success-50: #e8f5e9;
--success-500: #4caf50;
--success-700: #388e3c;

/* 警告状态 */
--warning-50: #fff8e1;
--warning-500: #ff9800;
--warning-700: #f57c00;

/* 错误状态 */
--error-50: #ffebee;
--error-500: #f44336;
--error-700: #d32f2f;

/* 信息状态 */
--info-50: #e3f2fd;
--info-500: #2196f3;
--info-700: #1976d2;
```

#### 中性色 (Neutral Colors)
```css
/* 灰度系统 */
--gray-50: #fafafa;     /* 页面背景 */
--gray-100: #f5f5f5;    /* 卡片背景 */
--gray-200: #eeeeee;    /* 分割线 */
--gray-300: #e0e0e0;    /* 边框 */
--gray-400: #bdbdbd;    /* 占位符 */
--gray-500: #9e9e9e;    /* 次要文本 */
--gray-600: #757575;    /* 辅助文本 */
--gray-700: #616161;    /* 常规文本 */
--gray-800: #424242;    /* 深色文本 */
--gray-900: #212121;    /* 标题 */
```

#### 学术指标色 (Academic Metric Colors)
```css
/* CCF等级配色 */
--ccf-a-bg: #fee2e2;
--ccf-a-text: #991b1b;
--ccf-a-border: #fca5a5;

--ccf-b-bg: #ffedd5;
--ccf-b-text: #9a3412;
--ccf-b-border: #fdba74;

--ccf-c-bg: #f3f4f6;
--ccf-c-text: #374151;
--ccf-c-border: #d1d5db;

/* 引用数量配色 */
--citation-low: #9ca3af;      /* < 10 */
--citation-medium: #f59e0b;   /* 10-49 */
--citation-high: #3b82f6;     /* 50-99 */
--citation-very-high: #10b981; /* 100+ */
```

#### 色彩使用规则

**主色使用场景**:
- 主要操作按钮（搜索、导出、爬虫启动）
- 链接和可点击元素
- 选中状态指示
- 进度指示器

**辅助色使用场景**:
- 次要操作按钮
- 标签和徽章
- 图表配色

**语义色使用场景**:
- 成功：任务完成、爬虫成功
- 警告：需要注意事项、配额限制
- 错误：失败状态、验证错误
- 信息：提示信息、帮助文档

### 字体系统

#### 字体家族 (Font Families)
```css
/* 系统字体栈 - 优先使用系统默认字体 */
--font-family-base: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto,
                   'Helvetica Neue', Arial, sans-serif;

/* 标题字体 - 需要时可引入Inter字体 */
--font-family-heading: 'Inter', -apple-system, BlinkMacSystemFont,
                      'Segoe UI', Roboto, sans-serif;

/* 等宽字体 - 代码、DOI、URL */
--font-family-mono: 'Fira Code', 'Consolas', 'Monaco',
                   'Courier New', monospace;
```

#### 字体大小 (Font Sizes)
```css
/* 主三比例 (Major Third) - 1.250 */
--font-xs: 0.75rem;     /* 12px - 标签、注释 */
--font-sm: 0.875rem;    /* 14px - 正文、按钮 */
--font-base: 1rem;      /* 16px - 默认文本 */
--font-md: 1.125rem;    /* 18px - 大段正文 */
--font-lg: 1.25rem;     /* 20px - 小标题 */
--font-xl: 1.5rem;      /* 24px - 标题 */
--font-2xl: 1.875rem;   /* 30px - 大标题 */
--font-3xl: 2.25rem;    /* 36px - 特大标题 */
--font-4xl: 3rem;       /* 48px - Hero标题 */
```

#### 字体重 (Font Weights)
```css
--font-light: 300;      /* 轻体 - 少用 */
--font-normal: 400;     /* 常规 - 正文 */
--font-medium: 500;     /* 中等 - 强调 */
--font-semibold: 600;   /* 半粗 - 小标题 */
--font-bold: 700;       /* 粗体 - 标题 */
```

#### 行高 (Line Heights)
```css
--leading-tight: 1.25;   /* 紧凑 - 标题 */
--leading-normal: 1.5;   /* 常规 - 正文 */
--leading-relaxed: 1.75; /* 宽松 - 长段落 */
```

#### 字母间距 (Letter Spacing)
```css
--tracking-tight: -0.025em;  /* 紧凑 */
--tracking-normal: 0;        /* 常规 */
--tracking-wide: 0.025em;    /* 宽松 - 大标题 */
```

#### 排版使用规范

**标题层级**:
```
H1: --font-3xl, --font-bold, --leading-tight
H2: --font-2xl, --font-semibold, --leading-tight
H3: --font-xl, --font-semibold, --leading-normal
H4: --font-lg, --font-medium, --leading-normal
```

**正文文本**:
```
段落: --font-base, --font-normal, --leading-relaxed
辅助: --font-sm, --font-normal, --leading-normal
标签: --font-xs, --font-medium, --leading-normal
```

### 间距系统

#### 基础间距 (Base Spacing - 4px Grid)
```css
--space-0: 0;           /* 0px */
--space-1: 0.25rem;     /* 4px - 最小间距 */
--space-2: 0.5rem;      /* 8px - 紧凑间距 */
--space-3: 0.75rem;     /* 12px - 小间距 */
--space-4: 1rem;        /* 16px - 标准间距 */
--space-5: 1.25rem;     /* 20px - 中等间距 */
--space-6: 1.5rem;      /* 24px - 大间距 */
--space-8: 2rem;        /* 32px - 超大间距 */
--space-10: 2.5rem;     /* 40px - 特大间距 */
--space-12: 3rem;       /* 48px - 巨大间距 */
--space-16: 4rem;       /* 64px - 分区间距 */
--space-20: 5rem;       /* 80px - 页面级间距 */
```

#### 间距使用场景

**组件内部间距**:
- 元素间隙: `--space-2` 到 `--space-4`
- 卡片内边距: `--space-4` 到 `--space-6`
- 按钮内边距: `--space-3` 垂直, `--space-5` 水平

**布局间距**:
- 节间距: `--space-6` 到 `--space-8`
- 区块间距: `--space-12` 到 `--space-16`
- 页面边距: `--space-4` (移动), `--space-6` (平板), `--space-8` (桌面)

### 圆角系统

```css
--radius-none: 0;          /* 直角 */
--radius-sm: 0.125rem;     /* 2px - 小标签 */
--radius-base: 0.25rem;    /* 4px - 输入框 */
--radius-md: 0.375rem;     /* 6px - 按钮 */
--radius-lg: 0.5rem;       /* 8px - 卡片 */
--radius-xl: 0.75rem;      /* 12px - 大卡片 */
--radius-2xl: 1rem;        /* 16px - 面板 */
--radius-3xl: 1.5rem;      /* 24px - 模态框 */
--radius-full: 9999px;     /* 圆形 - 头像、徽章 */
```

### 阴影系统

```css
/* 层级阴影 - 用于建立视觉层级 */
--shadow-xs: 0 1px 2px 0 rgba(0, 0, 0, 0.05);
              /* 极淡 - 装饰线 */

--shadow-sm: 0 1px 3px 0 rgba(0, 0, 0, 0.1),
             0 1px 2px 0 rgba(0, 0, 0, 0.06);
              /* 轻微 - 卡片默认 */

--shadow-md: 0 4px 6px -1px rgba(0, 0, 0, 0.1),
             0 2px 4px -1px rgba(0, 0, 0, 0.06);
              /* 中等 - 悬浮卡片 */

--shadow-lg: 0 10px 15px -3px rgba(0, 0, 0, 0.1),
             0 4px 6px -2px rgba(0, 0, 0, 0.05);
              /* 明显 - 下拉菜单 */

--shadow-xl: 0 20px 25px -5px rgba(0, 0, 0, 0.1),
             0 10px 10px -5px rgba(0, 0, 0, 0.04);
              /* 强烈 - 模态框 */

--shadow-2xl: 0 25px 50px -12px rgba(0, 0, 0, 0.25);
               /* 最强 - 全屏遮罩 */

--shadow-inner: inset 0 2px 4px 0 rgba(0, 0, 0, 0.06);
                /* 内阴影 - 输入框聚焦 */
```

### 层级系统

```css
--z-dropdown: 1000;        /* 下拉菜单 */
--z-sticky: 1020;          /* 粘性头部 */
--z-fixed: 1030;           /* 固定元素 */
--z-modal-backdrop: 1040;  /* 模态框背景 */
--z-modal: 1050;           /* 模态框 */
--z-popover: 1060;         /* 弹出框 */
--z-tooltip: 1070;         /* 工具提示 */
--z-notification: 1080;    /* 通知 */
```

---

## 核心组件库

### 1. 导航组件

#### 顶部导航栏 (Top Navigation)

**功能**:
- Logo和品牌展示
- 主导航菜单
- 搜索入口
- 用户信息和操作

**布局规范**:
```
┌─────────────────────────────────────────────────────────────┐
│ [Logo] PaperCrawler    [首页][论文][统计][设置]     [🔍] [👤] │
└─────────────────────────────────────────────────────────────┘
高度: 64px
背景: var(--gray-50) + blur(20px)
边框: 底部 1px solid var(--gray-200)
阴影: var(--shadow-sm)
```

**状态**:
- 默认: 半透明背景
- 滚动: 完全不透明
- 悬浮: 菜单项显示下划线

**响应式**:
- 桌面 (>1024px): 完整导航
- 平板 (768-1024px): 收起次要菜单
- 移动 (<768px): 汉堡菜单

#### 侧边导航栏 (Sidebar Navigation)

**功能**:
- 次级导航
- 过滤器和分类
- 快捷操作

**布局规范**:
```
┌──────────────┐
│ [≡] 论文管理 │
│              │
│ 📚 全部论文  │
│ ⭐ 收藏夹    │
│ 📁 分类      │
│              │
│ 🔧 爬虫任务  │
│ 📊 统计分析  │
│              │
│ ⚙️ 设置      │
└──────────────┘
宽度: 240px (可折叠至 64px)
背景: var(--gray-50)
边框: 右侧 1px solid var(--gray-200)
```

**状态**:
- 展开: 显示完整文本
- 折叠: 仅显示图标
- 激活: 左侧 3px primary 色条

### 2. 论文卡片组件

#### 标准论文卡片 (Standard Paper Card)

**布局**:
```
┌─────────────────────────────────────────────────────┐
│ [标题 - 粗体, 最多2行]                    [⋮] [❤️] │
│                                                     │
│ 👤 作者1, 作者2, 作者3...                          │
│                                                     │
│ [📅 2024] [🏢 CVPR] [📊 156 引用]                  │
│                                                     │
│ #深度学习 #计算机视觉 #NLP                          │
│                                                     │
│ [查看详情 →]                                        │
└─────────────────────────────────────────────────────┘
圆角: var(--radius-lg)
内边距: var(--space-5)
阴影: var(--shadow-sm) → var(--shadow-md) (hover)
```

**状态变体**:
- 默认: 白色背景
- 悬浮: 阴影加深, 顶部渐变边框
- 选中: Primary 色边框, 浅蓝背景
- 收藏: 心形图标填充

#### 紧凑论文卡片 (Compact Paper Card)

**布局**:
```
┌──────────────────────────────────────────────┐
│ 标题...                     [CVPR] [156 引用] │
│ 作者等...                         [❤️] [⋮]    │
└──────────────────────────────────────────────┘
高度: 自适应
内边距: var(--space-3) var(--space-4)
```

**使用场景**:
- 列表视图
- 搜索结果
- 移动端展示

#### 论文卡片必选信息

**核心信息** (始终显示):
- 论文标题
- 第一作者（或前三位作者）
- 发表年份
- 引用数量

**次要信息** (空间允许时显示):
- 全部作者
- 发表 venue
- CCF 等级
- 摘要预览

**操作** (悬停或点击显示):
- 查看详情
- 添加到收藏
- 导出引用
- 分享

### 3. 搜索栏组件

#### 主搜索栏 (Main Search Bar)

**布局**:
```
┌─────────────────────────────────────────────────────┐
│ [🔍] 搜索论文标题、作者、DOI...          [搜索]    │
└─────────────────────────────────────────────────────┘
高度: 48px
圆角: var(--radius-full)
背景: white
边框: 2px solid var(--gray-300)
焦点: 边框变为 var(--primary-500)
```

**功能**:
- 自动完成建议
- 搜索历史
- 高级搜索开关

#### 高级搜索面板 (Advanced Search Panel)

**布局**:
```
┌─────────────────────────────────────────────────────┐
│ 关键词: [输入框]                                     │
│ 作者: [输入框]                                       │
│ 发表年份: [从] [到]                                  │
│ Venue: [输入框]                                      │
│ CCF等级: [A☑] [B☑] [C☑]                            │
│                                                     │
│ [重置] [应用筛选]                                    │
└─────────────────────────────────────────────────────┘
```

**状态**:
- 默认: 折叠
- 展开: 显示所有筛选选项
- 应用: 显示激活的筛选器数量

### 4. 表格组件

#### 数据表格 (Data Table)

**布局**:
```
┌─────────────────────────────────────────────────────────────────┐
│ 标题        ▾  │ 作者          │ 年份  │ Venue    │ 引用   │ 操作│
├─────────────────────────────────────────────────────────────────┤
│ Paper 1      │ Author等      │ 2024  │ CVPR     │ 156    │ [⋮] │
├─────────────────────────────────────────────────────────────────┤
│ Paper 2      │ Author等      │ 2023  │ ICCV     │ 89     │ [⋮] │
├─────────────────────────────────────────────────────────────────┤
│ Paper 3      │ Author等      │ 2024  │ NeurIPS  │ 234    │ [⋮] │
└─────────────────────────────────────────────────────────────────┘
```

**功能**:
- 列排序
- 列过滤
- 列宽调整
- 行选择
- 虚拟滚动（大数据集）

**状态**:
- 悬浮行: 浅色背景
- 选中行: Primary 色边框
- 加载中: 骨架屏

#### 分页组件 (Pagination)

**布局**:
```
┌─────────────────────────────────────────────────────┐
│ 显示 1-20 / 共 1,234 篇论文                        │
│                                                     │
│ [< 上一页] [1] [2] [3] ... [10] [下一页 >]         │
└─────────────────────────────────────────────────────┘
```

**功能**:
- 页码跳转
- 每页数量选择
- 上一页/下一页
- 快速跳转（首页/末页）

### 5. 过滤器组件

#### 侧边过滤器 (Sidebar Filters)

**布局**:
```
┌─────────────────────┐
│ 🔍 筛选结果         │
│                     │
│ ▼ 发表年份          │
│ ☑ 2024 (156)       │
│ ☑ 2023 (89)        │
│ ☐ 2022 (234)       │
│                     │
│ ▼ CCF 等级          │
│ ☑ A 类 (45)        │
│ ☑ B 类 (78)        │
│ ☐ C 类 (112)       │
│                     │
│ ▼ Venue             │
│ ☐ CVPR (23)        │
│ ☐ ICCV (15)        │
│                     │
│ [清除全部筛选]      │
└─────────────────────┘
宽度: 280px
```

**功能**:
- 多选
- 数量显示
- 展开/折叠
- 清除筛选

#### 标签过滤器 (Tag Filters)

**布局**:
```
┌─────────────────────────────────────────────────────┐
│ 激活的筛选:                                         │
│ [2024 ×] [CCF-A ×] [CVPR ×] [清除全部]             │
└─────────────────────────────────────────────────────┘
```

### 6. 导出组件

#### 导出选项面板 (Export Options)

**布局**:
```
┌─────────────────────────────┐
│ 📤 导出选项                 │
│                             │
│ 格式:                       │
│ ○ BibTeX                   │
│ ○ EndNote                  │
│ ○ CSV                      │
│ ○ PDF                      │
│                             │
│ 内容:                       │
│ ☑ 标题                     │
│ ☑ 作者                     │
│ ☑ 摘要                     │
│ ☐ 全文                     │
│                             │
│ [取消] [导出 23 篇论文]     │
└─────────────────────────────┘
```

**功能**:
- 格式选择
- 内容选项
- 批量导出
- 导出进度

### 7. 爬虫任务监控组件

#### 任务列表 (Task List)

**布局**:
```
┌─────────────────────────────────────────────────────────────┐
│ 🔄 爬虫任务                              [+ 新建任务]      │
├─────────────────────────────────────────────────────────────┤
│ CVPR 2024 论文爬取                           [运行中]       │
│ 进度: ████████░░ 80% (800/1000)                          [⏸] │
│ 开始时间: 2026-04-04 14:30  │  预计完成: 14:45             │
├─────────────────────────────────────────────────────────────┤
│ ICCV 2023 论文补全                             [已完成]   [✓] │
│ 完成: 100% (234/234)                                         │
│ 完成时间: 2026-04-04 13:15                                   │
└─────────────────────────────────────────────────────────────┘
```

**状态**:
- 运行中: 蓝色动画进度条
- 暂停: 黄色暂停图标
- 完成: 绿色对勾
- 失败: 红色错误图标

#### 任务详情面板 (Task Details)

**布局**:
```
┌─────────────────────────────┐
│ 任务详情                    │
│                             │
│ CVPR 2024 论文爬取          │
│                             │
│ 状态: 运行中 🔄             │
│ 进度: 800/1000 (80%)        │
│                             │
│ 📊 统计:                    │
│ • 成功: 785                 │
│ • 失败: 15                  │
│ • 跳过: 0                   │
│                             │
│ ⚙️ 配置:                    │
│ • 并发: 10                  │
│ • 超时: 30s                 │
│ • 重试: 3次                 │
│                             │
│ [暂停] [停止] [查看日志]     │
└─────────────────────────────┘
```

### 8. 统计图表组件

#### 影响力图表 (Impact Chart)

**类型**: 散点图 / 气泡图
**维度**:
- X轴: 发表年份
- Y轴: 引用数量
- 气泡大小: 影响因子

#### 兴趣雷达图 (Interest Radar)

**类型**: 雷达图
**维度**:
- 机器学习
- 计算机视觉
- 自然语言处理
- 软件工程
- 数据库
- 系统架构

#### 时间线图表 (Timeline Chart)

**类型**: 折线图 / 面积图
**维度**:
- X轴: 时间（月/年）
- Y轴: 论文数量 / 引用数量

---

## 布局系统

### 页面布局规范

#### 标准页面布局 (Standard Page Layout)

```
┌─────────────────────────────────────────────────────────┐
│                    顶部导航栏 (64px)                    │
├─────────────────────────────────────────────────────────┤
│  面包屑导航 (40px)                                      │
├──────────┬──────────────────────────────────────────────┤
│          │  页面标题 (80px)                             │
│  侧边栏  │                                              │
│ (240px)  │  主内容区域                                   │
│          │                                              │
│          │                                              │
└──────────┴──────────────────────────────────────────────┘
```

#### 宽页面布局 (Wide Page Layout)

```
┌─────────────────────────────────────────────────────────┐
│                    顶部导航栏 (64px)                    │
├─────────────────────────────────────────────────────────┤
│  页面标题 + 操作 (80px)                                 │
├─────────────────────────────────────────────────────────┤
│                                                         │
│                   主内容区域 (全宽)                      │
│                                                         │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 容器宽度规范

```css
/* 紧凑布局 */
--container-narrow: 800px;
/* 用于: 表单、详情页、文章阅读 */

/* 标准布局 */
--container-default: 1200px;
/* 用于: 大多数页面、列表视图 */

/* 宽布局 */
--container-wide: 1400px;
/* 用于: 数据表格、图表、统计页面 */

/* 超宽布局 */
--container-full: 1600px;
/* 用于: 画廊、网格视图 */
```

### 网格系统

#### 12列网格 (12-Column Grid)

```css
.grid {
  display: grid;
  grid-template-columns: repeat(12, 1fr);
  gap: var(--space-6);
}

/* 使用示例 */
.col-3 { grid-column: span 3; }  /* 1/4 宽度 */
.col-4 { grid-column: span 4; }  /* 1/3 宽度 */
.col-6 { grid-column: span 6; }  /* 1/2 宽度 */
.col-8 { grid-column: span 8; }  /* 2/3 宽度 */
.col-12 { grid-column: span 12; } /* 全宽 */
```

#### 论文卡片网格

```
桌面 (>1280px):  ┌────┐┌────┐┌────┐┌────┐
                 │    ││    ││    ││    │
                 └────┘└────┘└────┘└────┘

平板 (768-1280px): ┌─────┐┌─────┐┌─────┐
                   │     ││     ││     │
                   └─────┘└─────┘└─────┘

移动 (<768px):     ┌──────────┐
                   │          │
                   └──────────┘
```

---

## 响应式设计

### 断点系统

```css
/* 断点定义 */
--breakpoint-xs: 0px;      /* 超小屏 - 手机竖屏 */
--breakpoint-sm: 640px;    /* 小屏 - 手机横屏 */
--breakpoint-md: 768px;    /* 中屏 - 平板竖屏 */
--breakpoint-lg: 1024px;   /* 大屏 - 平板横屏 */
--breakpoint-xl: 1280px;   /* 超大屏 - 笔记本 */
--breakpoint-2xl: 1536px;  /* 2K屏 - 桌面显示器 */
```

### 响应式布局策略

#### 移动优先 (Mobile First)

```css
/* 默认样式 - 移动端 */
.container {
  padding: var(--space-4);
  grid-template-columns: 1fr;
}

/* 平板及以上 */
@media (min-width: 768px) {
  .container {
    padding: var(--space-6);
    grid-template-columns: repeat(2, 1fr);
  }
}

/* 桌面及以上 */
@media (min-width: 1024px) {
  .container {
    padding: var(--space-8);
    grid-template-columns: repeat(3, 1fr);
  }
}
```

#### 关键组件响应式行为

**导航栏**:
- <768px: 汉堡菜单 + 全屏抽屉
- 768-1024px: 简化导航
- >1024px: 完整导航

**侧边栏**:
- <1024px: 抽屉式侧边栏
- ≥1024px: 固定侧边栏

**论文卡片**:
- <640px: 单列
- 640-1024px: 2列
- 1024-1280px: 3列
- ≥1280px: 4列

**表格**:
- <768px: 卡片视图
- ≥768px: 表格视图

### 触摸优化

**最小触摸目标**:
```css
/* 按钮、链接等可点击元素 */
.touch-target {
  min-width: 44px;
  min-height: 44px;
}
```

**手势支持**:
- 下拉刷新
- 侧滑删除
- 双击缩放
- 长按操作

---

## 交互设计规范

### 鼠标交互

#### 悬浮状态 (Hover States)

**标准悬浮**:
- 背景色变化: `--gray-50` → `--gray-100`
- 阴影加深: `--shadow-sm` → `--shadow-md`
- 过渡时间: 150ms
- 缓动函数: ease-out

**按钮悬浮**:
- 背景色加深 10-15%
- 轻微上移: `translateY(-1px)`
- 阴影增强

**卡片悬浮**:
- 阴影加深: `--shadow-sm` → `--shadow-lg`
- 轻微上移: `translateY(-2px)`
- 顶部渐变边框显示

#### 点击状态 (Active States)

- 轻微缩小: `scale(0.98)`
- 阴影减弱
- 过渡时间: 100ms

#### 焦点状态 (Focus States)

**可见焦点环**:
```css
:focus-visible {
  outline: 2px solid var(--primary-500);
  outline-offset: 2px;
  border-radius: var(--radius-sm);
}
```

### 键盘交互

#### 快捷键

**全局快捷键**:
- `Ctrl/Cmd + K`: 打开搜索
- `Ctrl/Cmd + /`: 打开帮助
- `Esc`: 关闭模态框/抽屉
- `Ctrl/Cmd + ,`: 打开设置

**导航快捷键**:
- `Ctrl/Cmd + 1-9`: 切换标签页
- `Alt + ←/→`: 前进/后退
- `Ctrl/Cmd + F`: 聚焦搜索框

**列表快捷键**:
- `↑/↓`: 上/下移动
- `Home/End`: 首/末项
- `Space`: 选择/取消选择
- `Enter`: 打开详情

#### 键盘导航

**Tab 顺序**:
1. 跳过导航链接
2. 主导航菜单
3. 搜索框
4. 主内容区域
5. 侧边栏过滤器
6. 页脚链接

**焦点陷阱**:
- 模态框打开时，焦点限制在模态框内
- 按下 `Esc` 关闭模态框
- 关闭后，焦点返回触发元素

### 加载状态

#### 骨架屏 (Skeleton Screens)

```css
.skeleton {
  background: linear-gradient(
    90deg,
    var(--gray-200) 0%,
    var(--gray-100) 50%,
    var(--gray-200) 100%
  );
  background-size: 200% 100%;
  animation: skeleton-loading 1.5s ease-in-out infinite;
}

@keyframes skeleton-loading {
  0% { background-position: 200% 0; }
  100% { background-position: -200% 0; }
}
```

#### 进度指示器

**线性进度条**:
- 高度: 4px
- 圆角: var(--radius-full)
- 颜色: var(--primary-500)
- 动画: 渐变条纹移动

**环形进度条**:
- 直径: 40px
- 线宽: 4px
- 颜色: var(--primary-500)
- 轨道: var(--gray-200)

### 空状态

#### 无数据状态 (Empty State)

```
┌─────────────────────────────┐
│                             │
│          [图标]             │
│                             │
│      暂无论文数据           │
│   开始添加您的第一篇论文     │
│                             │
│     [添加论文] [导入数据]    │
│                             │
└─────────────────────────────┘
```

#### 搜索无结果状态

```
┌─────────────────────────────┐
│                             │
│          [🔍]              │
│                             │
│      未找到相关论文         │
│   试试调整搜索关键词         │
│                             │
│     [清除筛选] [高级搜索]    │
│                             │
└─────────────────────────────┘
```

### 错误状态

#### 错误提示

```
┌─────────────────────────────┐
│ [⚠️] 加载失败               │
│                             │
│ 无法加载论文数据             │
│ 请检查网络连接后重试         │
│                             │
│ 错误代码: 500               │
│                             │
│     [重试] [返回首页]       │
└─────────────────────────────┘
```

#### 表单验证错误

- 输入框红框
- 错误图标
- 错误提示文字（红色）
- 摇晃动画

---

## 可访问性标准

### WCAG 2.1 AA 级别合规

#### 色彩对比度

**正文文本 (≥16px)**:
- 对比度 ≥ 4.5:1
- 示例: `--gray-900` on `--white`

**大文本 (≥24px 或 ≥18px 粗体)**:
- 对比度 ≥ 3:1

**交互元素**:
- 对比度 ≥ 3:1
- 图标和背景

#### 语义化 HTML

```html
<!-- 正确示例 -->
<header>
  <nav aria-label="主导航">
    <ul>
      <li><a href="/papers">论文</a></li>
    </ul>
  </nav>
</header>

<main id="main-content">
  <article>
    <h1>论文标题</h1>
    <p>论文内容...</p>
  </article>
</main>

<aside aria-label="侧边栏">
  <!-- 过滤器 -->
</aside>

<footer>
  <!-- 页脚内容 -->
</footer>
```

#### ARIA 属性

**导航标记**:
```html
<nav aria-label="主导航">
<nav aria-label="页脚导航">
```

**按钮标签**:
```html
<button aria-label="关闭对话框">×</button>
<button aria-label="添加到收藏">
  <svg><!-- 爱心图标 --></svg>
</button>
```

**实时区域**:
```html
<div aria-live="polite" aria-atomic="true">
  <!-- 加载状态、错误消息 -->
</div>
```

**展开/折叠**:
```html
<button aria-expanded="false" aria-controls="filters-panel">
  显示筛选器
</button>
<div id="filters-panel" hidden>
  <!-- 筛选器内容 -->
</div>
```

### 屏幕阅读器支持

#### 跳过导航链接

```html
<a href="#main-content" class="skip-link">
  跳转到主要内容
</a>
```

```css
.skip-link {
  position: absolute;
  top: -40px;
  left: 0;
  background: var(--primary-600);
  color: white;
  padding: var(--space-2) var(--space-4);
  transition: top 0.3s;
}

.skip-link:focus {
  top: 0;
}
```

#### 图标辅助文本

```html
<button>
  <svg aria-hidden="true"><!-- 图标 --></svg>
  <span class="sr-only">添加到收藏</span>
</button>
```

```css
.sr-only {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border: 0;
}
```

### 键盘可访问性

#### 焦点管理

**所有交互元素必须可聚焦**:
- 按钮
- 链接
- 表单控件
- 自定义组件（使用 `tabindex`）

**焦点顺序**:
- 符合视觉顺序
- 逻辑性分组
- 跳过隐藏元素

**焦点样式**:
```css
:focus-visible {
  outline: 2px solid var(--primary-500);
  outline-offset: 2px;
}
```

#### 模态框焦点陷阱

```javascript
// 打开模态框时
const focusableElements = modal.querySelectorAll(
  'a[href], button, textarea, input, select'
);
const firstElement = focusableElements[0];
const lastElement = focusableElements[focusableElements.length - 1];

firstElement.focus();

// Tab 键循环
modal.addEventListener('keydown', (e) => {
  if (e.key === 'Tab') {
    if (e.shiftKey && document.activeElement === firstElement) {
      e.preventDefault();
      lastElement.focus();
    } else if (!e.shiftKey && document.activeElement === lastElement) {
      e.preventDefault();
      firstElement.focus();
    }
  }
});
```

### 辅助技术兼容性

#### 测试工具

- **NVDA** (Windows, 免费)
- **JAWS** (Windows, 商业)
- **VoiceOver** (macOS/iOS, 内置)
- **TalkBack** (Android, 内置)

#### 测试检查清单

- [ ] 使用屏幕阅读器导航所有页面
- [ ] 验证所有交互元素可访问
- [ ] 检查焦点顺序合理
- [ ] 确认表单错误提示可读
- [ ] 测试键盘快捷键功能
- [ ] 验证动态内容更新通知

---

## 动画与过渡

### 过渡时长

```css
--duration-instant: 100ms;   /* 即时反馈 - 悬浮 */
--duration-fast: 150ms;      /* 快速 - 按钮点击 */
--duration-base: 200ms;      /* 标准 - 大多数交互 */
--duration-slow: 300ms;      /* 慢速 - 模态框展开 */
--duration-slower: 500ms;    /* 更慢 - 复杂动画 */
```

### 缓动函数

```css
--ease-linear: linear;              /* 线性 - 进度条 */
--ease-in: cubic-bezier(0.4, 0, 1, 1);     /* 加速 */
--ease-out: cubic-bezier(0, 0, 0.2, 1);    /* 减速 - 大多数动画 */
--ease-in-out: cubic-bezier(0.4, 0, 0.2, 1); /* 加减速 */
--ease-bounce: cubic-bezier(0.68, -0.55, 0.265, 1.55); /* 弹跳 - 特殊效果 */
```

### 常用动画

#### 淡入淡出 (Fade In/Out)

```css
@keyframes fade-in {
  from { opacity: 0; }
  to { opacity: 1; }
}

@keyframes fade-out {
  from { opacity: 1; }
  to { opacity: 0; }
}
```

#### 滑入滑出 (Slide In/Out)

```css
@keyframes slide-in-right {
  from {
    transform: translateX(100%);
    opacity: 0;
  }
  to {
    transform: translateX(0);
    opacity: 1;
  }
}

@keyframes slide-in-up {
  from {
    transform: translateY(100%);
    opacity: 0;
  }
  to {
    transform: translateY(0);
    opacity: 1;
  }
}
```

#### 缩放 (Scale)

```css
@keyframes scale-in {
  from {
    transform: scale(0.9);
    opacity: 0;
  }
  to {
    transform: scale(1);
    opacity: 1;
  }
}

@keyframes scale-out {
  from {
    transform: scale(1);
    opacity: 1;
  }
  to {
    transform: scale(0.9);
    opacity: 0;
  }
}
```

#### 旋转 (Rotate)

```css
@keyframes spin {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

/* 使用示例 */
.loading-spinner {
  animation: spin 1s linear infinite;
}
```

### 性能优化

#### GPU 加速

```css
/* 使用 transform 和 opacity */
.animated-element {
  will-change: transform, opacity;
  transform: translateZ(0);
}

/* 避免动画的属性 */
/* 避免动画 width, height, margin, padding */
/* 这些会触发 layout/reflow */
```

#### 减少动画 (Reduced Motion)

```css
@media (prefers-reduced-motion: reduce) {
  *,
  *::before,
  *::after {
    animation-duration: 0.01ms !important;
    animation-iteration-count: 1 !important;
    transition-duration: 0.01ms !important;
    scroll-behavior: auto !important;
  }
}
```

---

## 组件实现指南

### 组件模板

```vue
<template>
  <component
    :is="tag"
    :class="classes"
    :style="styles"
    v-bind="$attrs"
    @click="handleClick"
  >
    <slot />

    <!-- 装饰性元素 -->
    <div v-if="showDecoration" class="decoration" />
  </component>
</template>

<script setup lang="ts">
import { computed } from 'vue'

interface Props {
  variant?: 'primary' | 'secondary' | 'tertiary'
  size?: 'sm' | 'md' | 'lg'
  disabled?: boolean
  loading?: boolean
  tag?: string
}

const props = withDefaults(defineProps<Props>(), {
  variant: 'primary',
  size: 'md',
  disabled: false,
  loading: false,
  tag: 'button'
})

const emit = defineEmits<{
  click: [event: MouseEvent]
}>()

const classes = computed(() => {
  return [
    'component',
    `component--${props.variant}`,
    `component--${props.size}`,
    {
      'component--disabled': props.disabled,
      'component--loading': props.loading
    }
  ]
})

const styles = computed(() => {
  return {
    // 动态样式
  }
})

const showDecoration = computed(() => {
  return props.variant === 'primary' && !props.disabled
})

const handleClick = (event: MouseEvent) => {
  if (props.disabled || props.loading) {
    event.preventDefault()
    return
  }
  emit('click', event)
}
</script>

<style scoped lang="scss">
@import '@/styles/design-system.scss';

.component {
  // 基础样式
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border-radius: var(--radius-md);
  font-weight: var(--font-medium);
  transition: all var(--duration-base) var(--ease-out);
  cursor: pointer;

  // 变体样式
  &--primary {
    background: var(--primary-500);
    color: white;

    &:hover:not(:disabled) {
      background: var(--primary-600);
      transform: translateY(-1px);
      box-shadow: var(--shadow-md);
    }
  }

  // 尺寸变体
  &--sm {
    padding: var(--space-2) var(--space-3);
    font-size: var(--font-sm);
  }

  &--md {
    padding: var(--space-3) var(--space-5);
    font-size: var(--font-base);
  }

  &--lg {
    padding: var(--space-4) var(--space-6);
    font-size: var(--font-lg);
  }

  // 状态样式
  &--disabled {
    opacity: 0.6;
    cursor: not-allowed;
    pointer-events: none;
  }

  &--loading {
    position: relative;
    pointer-events: none;
  }

  // 焦点样式
  &:focus-visible {
    outline: 2px solid var(--primary-500);
    outline-offset: 2px;
  }

  // 激活样式
  &:active:not(:disabled) {
    transform: scale(0.98);
  }
}
</style>
```

### 组件文档模板

```markdown
# ComponentName

简短描述组件的功能和用途。

## 基础用法

\`\`\`vue
<ComponentName />
\`\`\`

## Props

| 参数 | 说明 | 类型 | 默认值 |
|------|------|------|--------|
| variant | 组件变体 | `'primary' \| 'secondary' \| 'tertiary'` | `'primary'` |
| size | 组件尺寸 | `'sm' \| 'md' \| 'lg'` | `'md'` |
| disabled | 是否禁用 | `boolean` | `false` |

## Events

| 事件名 | 说明 | 参数 |
|--------|------|------|
| click | 点击事件 | `(event: MouseEvent) => void` |

## Slots

| 插槽名 | 说明 |
|--------|------|
| default | 默认内容 |

## 可访问性

- 支持键盘导航
- 符合 WCAG AA 标准
- 屏幕阅读器友好

## 示例

### 基础示例

\`\`\`vue
<ComponentName variant="primary" size="md">
  内容
</ComponentName>
\`\`\`

### 禁用状态

\`\`\`vue
<ComponentName disabled>
  禁用的组件
</ComponentName>
\`\`\`
```

---

## 设计资源

### 设计工具配置

#### Figma 变量配置

```json
{
  "color": {
    "primary": {
      "50": "#e3f2fd",
      "500": "#2196f3",
      "900": "#0d47a1"
    }
  },
  "spacing": {
    "1": "4px",
    "2": "8px",
    "4": "16px"
  },
  "typography": {
    "fontSizes": {
      "xs": "12px",
      "sm": "14px",
      "base": "16px"
    }
  }
}
```

#### Sketch 样式库

```
PaperCrawler Design System.sketch
├── Colors
│   ├── Primary
│   ├── Secondary
│   └── Semantic
├── Typography
│   ├── Headings
│   └── Body
├── Spacing
└── Components
    ├── Buttons
    ├── Cards
    └── Forms
```

### 图标库

**推荐图标库**:
- Material Design Icons (已经使用)
- Feather Icons
- Heroicons

**图标使用规范**:
- 大小: 16px, 20px, 24px, 32px
- 颜色: 继承文本颜色或使用语义色
- 间距: 图标与文本间距 `--space-2`

### 插画库

**用于空状态和错误页面**:
- undraw.co
- Storyset
- Humaaans

### 字体资源

**Google Fonts**:
```html
<!-- Inter - 标题字体 -->
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">

<!-- Fira Code - 等宽字体 -->
<link href="https://fonts.googleapis.com/css2?family=Fira+Code:wght@400;500&display=swap" rel="stylesheet">
```

### 开发工具

**浏览器扩展**:
- Vue.js devtools
- ColorZilla (取色器)
- WhatFont (字体识别)

**VS Code 扩展**:
- Vetur (Vue 语法支持)
- ESLint (代码检查)
- Prettier (代码格式化)

### 测试工具

**视觉回归测试**:
- Percy
- Chromatic

**可访问性测试**:
- axe DevTools
- WAVE
- Lighthouse

### 参考资源

**设计系统参考**:
- [Material Design](https://material.io/design)
- [Ant Design](https://ant.design/)
- [Chakra UI](https://chakra-ui.com/)
- [Tailwind CSS](https://tailwindcss.com/)

**可访问性参考**:
- [WCAG 2.1 Guidelines](https://www.w3.org/WAI/WCAG21/quickref/)
- [A11y Project](https://www.a11yproject.com/)
- [WebAIM](https://webaim.org/)

---

## 版本历史

### v2.0.0 (2026-04-04)

**重大更新**:
- 完整重构设计系统
- 新增论文卡片组件规范
- 新增爬虫监控组件规范
- 优化响应式布局
- 完善可访问性标准

**新增内容**:
- 学术指标配色系统
- 动画和过渡规范
- 组件实现指南
- 设计资源链接

### v1.0.0 (2025-03-22)

**初始版本**:
- 基础色彩系统
- 字体和间距规范
- 核心组件库
- 响应式断点

---

## 维护者

**UI Designer Agent**
**Email**: design@papercrawler.com
**GitHub**: @papercrawler/ui

---

## 许可证

MIT License - Copyright (c) 2026 PaperCrawler Project

---

**文档结束**

本文档是 PaperCrawler 项目的官方 UI 设计系统规范。所有组件和样式必须遵循此规范以确保整个应用的一致性和专业性。
