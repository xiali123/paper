# PaperCrawler 前端布局系统使用指南

## 概述

PaperCrawler布局系统是一套完整的响应式布局解决方案，提供高性能、可访问、易用的布局组件。

## 核心组件

### 1. PageLayout - 页面布局组件

完整的页面布局框架，包含头部、内容、尾部等部分。

#### 基础用法

```vue
<PageLayout
  title="页面标题"
  subtitle="页面描述"
  :loading="isLoading"
  :error="errorMessage"
  layout="wide"
>
  <template #headerActions>
    <button>操作按钮</button>
  </template>

  <div>页面内容</div>
</PageLayout>
```

#### Props

| 属性 | 类型 | 默认值 | 描述 |
|------|------|--------|------|
| title | string | '' | 页面标题 |
| subtitle | string | '' | 页面副标题 |
| loading | boolean | false | 加载状态 |
| loadingText | string | '加载中...' | 加载文本 |
| error | string | '' | 错误信息 |
| empty | boolean | false | 空状态 |
| layout | 'default' \| 'narrow' \| 'wide' \| 'full' | 'default' | 布局宽度 |
| contentWidth | 'narrow' \| 'default' \| 'wide' | 'default' | 内容宽度 |
| padding | 'none' \| 'sm' \| 'md' \| 'lg' | 'md' | 内边距 |
| showHeader | boolean | true | 显示头部 |
| showFooter | boolean | false | 显示尾部 |
| showBreadcrumb | boolean | false | 显示面包屑 |
| breadcrumbs | Breadcrumb[] | [] | 面包屑数据 |

#### 插槽 (Slots)

| 插槽名 | 描述 |
|--------|------|
| header | 自定义头部 |
| title | 自定义标题 |
| subtitle | 自定义副标题 |
| headerActions | 头部操作按钮 |
| loading | 自定义加载状态 |
| error | 自定义错误状态 |
| empty | 自定义空状态 |
| default | 主要内容 |
| footer | 自定义尾部 |

#### 事件 (Events)

| 事件名 | 描述 |
|--------|------|
| retry | 重试加载 |
| emptyAction | 空状态操作 |

#### 示例：完整用法

```vue
<PageLayout
  title="论文搜索"
  subtitle="在DBLP数据库中搜索学术论文"
  :loading="search.loading"
  :error="search.error"
  :empty="search.isEmpty"
  layout="wide"
  content-width="wide"
  :breadcrumbs="[
    { title: '首页', path: '/' },
    { title: '搜索' }
  ]"
  @retry="search.performSearch"
  @empty-action="search.resetSearch"
>
  <template #headerActions>
    <button @click="exportResults" class="btn btn-secondary">
      导出结果
    </button>
  </template>

  <template #empty>
    <EmptyState
      icon="🔍"
      title="未找到相关论文"
      description="请尝试其他关键词或调整搜索条件"
      action-text="重新搜索"
      @action="search.resetSearch"
    />
  </template>

  <div class="search-results">
    <!-- 搜索结果内容 -->
  </div>
</PageLayout>
```

### 2. ResponsiveGrid - 响应式网格组件

智能的响应式网格系统，自动适配不同屏幕尺寸。

#### 基础用法

```vue
<ResponsiveGrid
  :columns="3"
  gap="lg"
  align="center"
  justify="between"
>
  <div v-for="item in items" :key="item.id" class="card">
    {{ item.content }}
  </div>
</ResponsiveGrid>
```

#### Props

| 属性 | 类型 | 默认值 | 描述 |
|------|------|--------|------|
| columns | 'auto' \| 'minmax' \| number \| string[] | 'auto' | 网格列数 |
| gap | 'none' \| 'sm' \| 'md' \| 'lg' \| 'xl' | 'md' | 间距 |
| align | 'start' \| 'end' \| 'center' \| 'stretch' | 'stretch' | 垂直对齐 |
| justify | 'start' \| 'end' \| 'center' \| 'between' \| 'around' \| 'evenly' | 'start' | 水平对齐 |
| responsive | boolean | true | 启用响应式 |
| mobileColumns | number | 1 | 移动端列数 |
| tabletColumns | number | 2 | 平板列数 |
| desktopColumns | number | 3 | 桌面列数 |

#### 示例：自适应卡片网格

```vue
<ResponsiveGrid
  columns="minmax"
  gap="lg"
  responsive
>
  <div v-for="paper in papers" :key="paper.id" class="paper-card">
    <h3>{{ paper.title }}</h3>
    <p>{{ paper.abstract }}</p>
  </div>
</ResponsiveGrid>
```

### 3. ResponsiveContainer - 响应式容器

智能的容器组件，自动管理宽度和内边距。

#### 基础用法

```vue
<ResponsiveContainer
  size="wide"
  padding="lg"
  :max-width="1400"
>
  <div>容器内容</div>
</ResponsiveContainer>
```

#### Props

| 属性 | 类型 | 默认值 | 描述 |
|------|------|--------|------|
| tag | string | 'div' | 容器标签 |
| size | 'fluid' \| 'narrow' \| 'wide' \| 'custom' | 'custom' | 容器尺寸 |
| maxWidth | string | undefined | 最大宽度 |
| padding | 'none' \| 'sm' \| 'md' \| 'lg' \| 'xl' | 'md' | 内边距 |
| centered | boolean | true | 居中对齐 |
| fullWidth | boolean | false | 全宽模式 |

#### 示例：自定义容器

```vue
<ResponsiveContainer
  size="custom"
  :max-width="1200"
  padding="xl"
  centered
>
  <div class="custom-content">
    自定义宽度和内边距的容器
  </div>
</ResponsiveContainer>
```

### 4. MobileNav - 移动优先导航

响应式导航组件，在移动端显示汉堡菜单。

#### 基础用法

```vue
<MobileNav
  :menu-items="menuItems"
  brand="PaperCrawler"
  position="sticky"
  theme="light"
>
  <template #actions>
    <LanguageSwitcher />
    <button @click="toggleTheme">切换主题</button>
  </template>
</MobileNav>
```

#### Props

| 属性 | 类型 | 默认值 | 描述 |
|------|------|--------|------|
| brand | string | 'PaperCrawler' | 品牌名称 |
| menuItems | MenuItem[] | [] | 菜单项 |
| position | 'fixed' \| 'sticky' \| 'relative' | 'sticky' | 定位方式 |
| theme | 'light' \| 'dark' \| 'colored' | 'light' | 主题样式 |
| ariaLabel | string | '主导航' | ARIA标签 |

#### MenuItem接口

```typescript
interface MenuItem {
  path: string
  title: string
  icon?: string
  badge?: string | number
}
```

#### 示例：完整导航

```vue
<MobileNav
  brand="PaperCrawler"
  :menu-items="[
    { path: '/', title: '首页', icon: '🏠' },
    { path: '/search', title: '搜索', icon: '🔍' },
    { path: '/stats', title: '统计', icon: '📊' }
  ]"
  position="sticky"
  theme="colored"
>
  <template #actions>
    <LanguageSwitcher />
    <button @click="toggleTheme" class="theme-toggle">
      {{ isDark ? '☀️' : '🌙' }}
    </button>
  </template>
</MobileNav>
```

## 布局工具类

### Grid系统

```html
<!-- 基础网格 -->
<div class="grid grid-cols-3 gap-6">
  <div>项目 1</div>
  <div>项目 2</div>
  <div>项目 3</div>
</div>

<!-- 自适应网格 -->
<div class="grid grid-cols-auto gap-6">
  <div>自适应列</div>
</div>

<!-- 响应式网格 -->
<div class="grid mobile:grid-cols-1 tablet:grid-cols-2 desktop:grid-cols-3">
  <div>响应式项目</div>
</div>
```

### Flexbox系统

```html
<!-- 基础flex -->
<div class="flex items-center justify-between">
  <div>左侧内容</div>
  <div>右侧内容</div>
</div>

<!-- 垂直居中 -->
<div class="flex items-center justify-center h-screen">
  居中内容
</div>

<!-- Flex列 -->
<div class="flex flex-col gap-4">
  <div>项目 1</div>
  <div>项目 2</div>
</div>
```

### 容器系统

```html
<!-- 标准容器 -->
<div class="container">
  标准宽度容器
</div>

<!-- 全宽容器 -->
<div class="container-fluid">
  全宽容器
</div>

<!-- 窄容器 -->
<div class="container-narrow">
  窄容器
</div>

<!-- 宽容器 -->
<div class="container-wide">
  宽容器
</div>
```

## 响应式设计

### 断点系统

```css
/* 移动设备 */
@media (max-width: 640px) { }

/* 平板设备 */
@media (min-width: 641px) and (max-width: 1024px) { }

/* 桌面设备 */
@media (min-width: 1025px) and (max-width: 1536px) { }

/* 大屏幕 */
@media (min-width: 1537px) { }
```

### 响应式工具类

```html
<!-- 移动端隐藏 -->
<div class="hidden-mobile">
  只在桌面显示
</div>

<!-- 桌面端隐藏 -->
<div class="hidden-desktop">
  只在移动端显示
</div>

<!-- 响应式网格 -->
<div class="grid grid-cols-1 mobile:grid-cols-2 desktop:grid-cols-4">
  响应式网格
</div>
```

## 性能优化

### CSS Containment

```html
<!-- 减少重排和重绘 -->
<div class="contain-layout">
  内容隔离
</div>

<!-- 减少绘制 -->
<div class="contain-paint">
  绘制隔离
</div>
```

### GPU加速

```html
<!-- GPU加速 -->
<div class="gpu-accelerated">
  硬件加速元素
</div>
```

### 内容可见性

```html
<!-- 延迟渲染 -->
<div class="in-view-auto">
  长内容延迟加载
</div>
```

## 可访问性

### 焦点管理

```html
<!-- 焦点环 -->
<button class="focus-ring">
  带焦点环的按钮
</button>
```

### 屏幕阅读器

```html
<!-- 仅屏幕阅读器可见 -->
<span class="sr-only">隐藏文本</span>

<!-- 屏幕阅读器忽略 -->
<span class="not-sr-only">可见文本</span>
```

### 跳过链接

```html
<!-- 跳转到主内容 -->
<a href="#main-content" class="skip-link">
  跳转到主要内容
</a>
```

## 最佳实践

### 1. 移动优先设计

```vue
<!-- 推荐：从小屏幕开始设计 -->
<div class="grid grid-cols-1 tablet:grid-cols-2 desktop:grid-cols-3">
  <!-- 内容 -->
</div>

<!-- 避免：从大屏幕开始设计 -->
<div class="grid grid-cols-3 mobile:grid-cols-1">
  <!-- 内容 -->
</div>
```

### 2. 语义化HTML

```vue
<!-- 推荐：使用语义化标签 -->
<header>
  <nav>
    <ul>
      <li><a href="/">首页</a></li>
    </ul>
  </nav>
</header>

<main>
  <article>
    <h1>文章标题</h1>
    <p>文章内容</p>
  </article>
</main>

<footer>
  <p>页脚信息</p>
</footer>

<!-- 避免：过度使用div -->
<div class="header">
  <div class="nav">
    <div class="item">首页</div>
  </div>
</div>
```

### 3. 性能优化

```vue
<!-- 推荐：使用CSS containment -->
<div class="contain-layout-style">
  <!-- 复杂内容 -->
</div>

<!-- 推荐：使用content visibility -->
<div class="in-view-auto">
  <!-- 长列表 -->
</div>

<!-- 避免：不必要的DOM操作 -->
<div v-for="item in largeList" :key="item.id">
  {{ item }}
</div>
```

### 4. 可访问性

```vue
<!-- 推荐：添加ARIA属性 -->
<button
  aria-label="关闭对话框"
  aria-pressed="false"
>
  关闭
</button>

<!-- 推荐：键盘导航支持 -->
<div
  @keydown.enter="handleClick"
  @keydown.space="handleClick"
  tabindex="0"
  role="button"
>
  可点击的div
</div>
```

## 浏览器兼容性

| 浏览器 | 支持版本 | 备注 |
|--------|----------|------|
| Chrome | 最新版本 | 完全支持 |
| Firefox | 最新版本 | 完全支持 |
| Safari | 14+ | 完全支持 |
| Edge | 最新版本 | 完全支持 |
| Opera | 最新版本 | 完全支持 |
| IE | 不支持 | 请使用现代浏览器 |

## 故障排除

### 常见问题

1. **布局不响应**
   - 确保使用了正确的断点类名
   - 检查viewport meta标签是否正确

2. **性能问题**
   - 使用CSS containment减少重排
   - 避免在滚动容器中使用固定定位

3. **可访问性问题**
   - 确保所有交互元素有适当的ARIA属性
   - 测试键盘导航和屏幕阅读器支持

### 调试技巧

```css
/* 显示布局边界 */
* {
  outline: 1px solid red;
}

/* 显示网格布局 */
.grid {
  background-image:
    linear-gradient(to right, rgba(0,0,0,0.1) 1px, transparent 1px),
    linear-gradient(to bottom, rgba(0,0,0,0.1) 1px, transparent 1px);
  background-size: 20px 20px;
}
```

## 参考资源

- [CSS Grid Layout](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Grid_Layout)
- [Flexbox Layout](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Flexible_Box_Layout)
- [CSS Containment](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Containment)
- [Web Accessibility](https://www.w3.org/WAI/WCAG21/quickref/)
