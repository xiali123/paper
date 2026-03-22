# PaperCrawler 布局系统快速参考

## 🚀 快速开始

### 1. 引入布局组件
```vue
<script setup>
import { PageLayout, ResponsiveGrid, ResponsiveContainer } from '@/components/layout'
</script>
```

### 2. 基础页面布局
```vue
<PageLayout title="页面标题">
  <div>页面内容</div>
</PageLayout>
```

## 📦 核心组件速查

### PageLayout - 页面布局
```vue
<!-- 简单用法 -->
<PageLayout title="标题" subtitle="描述">
  内容
</PageLayout>

<!-- 完整用法 -->
<PageLayout
  title="论文搜索"
  subtitle="搜索学术论文"
  :loading="isLoading"
  :error="errorMessage"
  :empty="isEmpty"
  layout="wide"
  content-width="wide"
  :breadcrumbs="breadcrumbs"
>
  <template #headerActions>
    <button>操作</button>
  </template>

  <template #default>
    主要内容
  </template>

  <template #empty>
    <EmptyState title="无数据" />
  </template>
</PageLayout>
```

**关键属性：**
- `layout`: 'default' | 'narrow' | 'wide' | 'full'
- `contentWidth`: 'narrow' | 'default' | 'wide'
- `padding`: 'none' | 'sm' | 'md' | 'lg'

### ResponsiveGrid - 响应式网格
```vue
<!-- 自动适配 -->
<ResponsiveGrid gap="lg">
  <div v-for="item in items">项目</div>
</ResponsiveGrid>

<!-- 固定列数 -->
<ResponsiveGrid :columns="3" gap="md">
  <div>项目</div>
</ResponsiveGrid>

<!-- 自适应 -->
<ResponsiveGrid columns="minmax" gap="lg">
  <div>自适应卡片</div>
</ResponsiveGrid>
```

**关键属性：**
- `columns`: 'auto' | 'minmax' | 数字
- `gap`: 'none' | 'sm' | 'md' | 'lg' | 'xl'
- `responsive`: boolean

### ResponsiveContainer - 响应式容器
```vue
<!-- 标准容器 -->
<ResponsiveContainer>
  内容
</ResponsiveContainer>

<!-- 自定义宽度 -->
<ResponsiveContainer size="wide" padding="lg" :max-width="1400">
  内容
</ResponsiveContainer>
```

**关键属性：**
- `size`: 'fluid' | 'narrow' | 'wide' | 'custom'
- `padding`: 'none' | 'sm' | 'md' | 'lg' | 'xl'
- `maxWidth`: string (自定义宽度)

## 🎨 常用工具类

### Grid系统
```html
<div class="grid grid-cols-3 gap-6">...</div>
<div class="grid grid-cols-1 tablet:grid-cols-2 desktop:grid-cols-3">...</div>
<div class="grid grid-cols-auto gap-4">...</div>
```

### Flexbox系统
```html
<div class="flex justify-center items-center">...</div>
<div class="flex justify-between items-center">...</div>
<div class="flex flex-col gap-4">...</div>
```

### 容器工具
```html
<div class="container">标准容器</div>
<div class="container-fluid">全宽容器</div>
<div class="container-narrow">窄容器</div>
```

### 间距工具
```html
<div class="m-4">外边距16px</div>
<div class="p-4">内边距16px</div>
<div class="mx-auto">水平居中</div>
```

## ⚡ 性能优化

### CSS Containment
```html
<div class="contain-layout">优化重排的内容</div>
```

### Content Visibility
```html
<div class="in-view-auto">长列表延迟渲染</div>
```

### GPU加速
```html
<div class="gpu-accelerated">硬件加速动画</div>
```

## 📱 移动端技巧

```html
<!-- 触摸友好按钮 -->
<button class="min-h-44 min-w-44">44x44px触摸目标</button>

<!-- 安全区域适配 -->
<div class="safe-area-top">适配刘海屏</div>
```

## 🎯 最佳实践

### DO (推荐)
```vue
<!-- ✅ 使用语义化组件 -->
<PageLayout title="标题">
  <ResponsiveGrid gap="lg">
    <div class="card">内容</div>
  </ResponsiveGrid>
</PageLayout>

<!-- ✅ 移动优先 -->
<div class="grid grid-cols-1 tablet:grid-cols-2 desktop:grid-cols-3">
</div>
```

### DON'T (避免)
```vue
<!-- ❌ 过度使用div -->
<div class="header"><div class="nav">...</div></div>

<!-- ❌ 从大屏幕开始 -->
<div class="grid grid-cols-4 mobile:grid-cols-1"></div>
```

## 📊 断点参考

| 设备类型 | 屏幕宽度 | 断点类名 |
|---------|---------|----------|
| 小屏手机 | < 640px | `mobile:` |
| 大屏手机 | 641px - 1024px | `tablet:` |
| 桌面 | 1025px - 1536px | `desktop:` |
| 大屏 | > 1536px | `wide:` |

## 🚨 常见问题

**Q: 布局不响应？**
A: 检查viewport meta标签和CSS引入顺序

**Q: 性能问题？**
A: 使用CSS containment和content visibility

**Q: 移动端显示异常？**
A: 确保使用移动优先的设计方法

## 📚 更多资源

- 完整文档: `LAYOUT_SYSTEM_GUIDE.md`
- 实现指南: `LAYOUT_IMPLEMENTATION_GUIDE.md`
- 设计系统: `src/assets/styles/design-system.css`
- 布局工具: `src/assets/styles/layout-utilities.css`

---

**快速提示**: 所有组件都支持TypeScript，获得完整的类型提示！