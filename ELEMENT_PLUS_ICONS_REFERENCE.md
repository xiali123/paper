# Element Plus Icons 快速参考

> 更新时间：2026-03-22
> 官方文档：https://element-plus.org/zh-CN/component/icon.html

---

## ✅ 常用图标正确名称

### 操作图标

| 图标 | 名称 | 使用场景 |
|------|------|----------|
| ➕ | `Plus` | 添加、新建 |
| ✏️ | `Edit` | 编辑 |
| 🗑️ | `Delete` | 删除 |
| 🔍 | `Search` | 搜索 |
| 🔄 | `Refresh` | 刷新 |
| ↩️ | `RefreshLeft` | 撤销、重置 |
| ↪️ | `RefreshRight` | 重做 |
| ✔️ | `Check` | 确认、勾选 |
| ✖️ | `Close` | 关闭、取消 |
| ⬆️ | `ArrowUp` | 向上 |
| ⬇️ | `ArrowDown` | 向下 |
| ⬅️ | `ArrowLeft` | 返回、向左 |
| ➡️ | `ArrowRight` | 前进、向右 |

### 收藏和评分

| 图标 | 名称 | 使用场景 |
|------|------|----------|
| ⭐ | `Star` | 收藏（空心）|
| ⭐ | `StarFilled` | 收藏（实心）|
| ❤️ | `Heart` | 点赞（空心）|
| ❤️ | `HeartFilled` | 点赞（实心）|
| 🔖 | `Collection` | 收藏夹 |
| 🔖 | `CollectionTag` | 标签收藏 |

### 用户和身份

| 图标 | 名称 | 使用场景 |
|------|------|----------|
| 👤 | `User` | 用户 |
| 👥 | `UserFilled` | 用户（实心）|
| 🏠 | `House` | 首页 |
| 🔐 | `Lock` | 锁定、密码 |
| 🔓 | `Unlock` | 解锁 |
| 🛡️ | `Shield` | 安全、管理员 |
| 👔 | `Avatar` | 头像 |

### 文档和内容

| 图标 | 名称 | 使用场景 |
|------|------|----------|
| 📄 | `Document` | 文档 |
| 📝 | `DocumentCopy` | 复制文档 |
| 📚 | `Reading` | 阅读 |
| 📖 | `Tickets` | 票据、笔记 |
| 📎 | `Paperclip` | 附件 |
| 🔗 | `Link` | 链接 |
| 🖼️ | `Picture` | 图片 |
| 🎥 | `VideoPlay` | 视频 |
| 🎵 | `Headset` | 音频 |

### 状态和提示

| 图标 | 名称 | 使用场景 |
|------|------|----------|
| ℹ️ | `InfoFilled` | 信息提示 |
| ⚠️ | `Warning` | 警告 |
| ❌ | `CircleClose` | 错误、关闭 |
| ✅ | `CircleCheck` | 成功、确认 |
| ❓ | `QuestionFilled` | 问题 |
| 🔔 | `Bell` | 通知 |
| 💬 | `ChatDotRound` | 消息 |
| 📧 | `Message` | 邮件、消息 |

### 编辑操作

| 图标 | 名称 | 使用场景 |
|------|------|----------|
| 📝 | `EditPen` | 编辑笔 |
| ✂️ | `Scissor` | 剪切 |
| 📋 | `CopyDocument` | 复制 |
| 📑 | `Folder` | 文件夹 |
| 📁 | `FolderOpened` | 打开的文件夹 |
| 📤 | `Upload` | 上传 |
| 📥 | `Download` | 下载 |

### 视图控制

| 图标 | 名称 | 使用场景 |
|------|------|----------|
| 👁️ | `View` | 查看 |
| 👁️‍🗨️ | `Hide` | 隐藏 |
| 🔍 | `ZoomIn` | 放大 |
| 🔍 | `ZoomOut` | 缩小 |
| ⛶ | `FullScreen` | 全屏 |
| ⛶ | `Close` | 退出全屏 |

### 数据和图表

| 图标 | 名称 | 使用场景 |
|------|------|----------|
| 📊 | `Histogram` | 柱状图 |
| 📈 | `TrendCharts` | 趋势图 |
| 🍩 | `PieChart` | 饼图 |
| 📅 | `Calendar` | 日历 |
| ⏰ | `Clock` | 时间 |
| ⏱️ | `Timer` | 计时器 |

---

## ❌ 常见错误

### 不存在的图标名称

| ❌ 错误名称 | ✅ 正确名称 |
|------------|------------|
| `Bookmark` | `Star` 或 `Collection` |
| `Save` | `Check` 或 `DocumentCopy` |
| `Cancel` | `Close` |
| `Home` | `House` |
| `Password` | `Lock` |

---

## 💡 使用方法

### 方式1：全局导入（推荐）

**main.ts** 已全局注册所有图标：

```vue
<template>
  <el-icon :size="20">
    <Star />
  </el-icon>
</template>

<script setup lang="ts">
// 无需导入，直接使用
</script>
```

### 方式2：按需导入

```vue
<template>
  <el-icon :size="20">
    <Star />
  </el-icon>
</template>

<script setup lang="ts">
import { Star } from '@element-plus/icons-vue'
</script>
```

### 方式3：在组件中使用

```vue
<template>
  <!-- 按钮图标 -->
  <el-button :icon="Star">收藏</el-button>

  <!-- 输入框图标 -->
  <el-input :prefix-icon="Search" />

  <!-- 独立图标 -->
  <el-icon :size="24" color="#409eff">
    <Star />
  </el-icon>
</template>
```

---

## 🎨 图标样式

### 大小

```vue
<!-- 使用 size 属性 -->
<el-icon :size="20">
  <Star />
</el-icon>

<!-- 使用 CSS -->
<el-icon style="font-size: 20px">
  <Star />
</el-icon>
```

### 颜色

```vue
<el-icon color="#409eff">
  <Star />
</el-icon>

<el-icon style="color: #f56c6c">
  <Star />
</el-icon>
```

### 旋转

```vue
<el-icon class="is-loading">
  <Loading />
</el-icon>
```

---

## 🔍 查看所有可用图标

### 方法1：访问官方文档

https://element-plus.org/zh-CN/component/icon.html

### 方法2：查看源码

```bash
# 查看 icon 属性
node_modules/@element-plus/icons-vue/dist/types/index.d.ts
```

### 方法3：使用 TypeScript 智能提示

```typescript
import { /* 在这里输入看所有图标 */ } from '@element-plus/icons-vue'
```

---

## 📝 项目中的使用示例

### Papers.vue

```typescript
import {
  Plus,        // 添加论文
  Search,      // 搜索
  RefreshLeft, // 清除筛选
  Star,        // 收藏
  Delete       // 删除
} from '@element-plus/icons-vue'
```

### PaperCard.vue

```typescript
import {
  Star,        // 收藏（空心）
  StarFilled,  // 收藏（实心）
  User,        // 作者
  Document,    // 论文
  Edit,        // 编辑
  Delete,      // 删除
  View,        // 查看详情
  Hide         // 隐藏
} from '@element-plus/icons-vue'
```

### PaperManageDetail.vue

```typescript
import {
  ArrowLeft,   // 返回
  Star,        // 收藏
  StarFilled,  // 已收藏
  Edit,        // 编辑
  Delete,      // 删除
  User,        // 用户
  Document,    // 文档
  EditPen      // 编辑笔记
} from '@element-plus/icons-vue'
```

---

## ⚡ 性能优化建议

1. **全局注册 vs 按需导入**
   - 小项目：全局注册（简化开发）
   - 大项目：按需导入（减小体积）

2. **图标统一性**
   - 收藏功能统一使用 `Star`/`StarFilled`
   - 编辑统一使用 `Edit`/`EditPen`
   - 删除统一使用 `Delete`

3. **大小规范**
   - 按钮：16-18px
   - 列表：20px
   - 标题：24px

---

**最后更新**: 2026-03-22
**维护者**: PaperCrawler Development Team
