/**
 * PaperCrawler Layout Components
 * 响应式布局组件库
 */

export { default as ResponsiveContainer } from './ResponsiveContainer.vue'
export { default as ResponsiveGrid } from './ResponsiveGrid.vue'
export { default as MobileNav } from './MobileNav.vue'
export { default as PageLayout } from './PageLayout.vue'

// TypeScript类型导出
export type { MenuItem } from './MobileNav.vue'
export type { Breadcrumb } from './PageLayout.vue'

/**
 * 使用示例:
 *
 * import { PageLayout, ResponsiveGrid, MobileNav } from '@/components/layout'
 *
 * // 在模板中使用
 * <PageLayout
 *   title="页面标题"
 *   subtitle="页面描述"
 *   :loading="isLoading"
 *   :error="errorMessage"
 *   layout="wide"
 * >
 *   <template #headerActions>
 *     <button>操作按钮</button>
 *   </template>
 *
 *   <ResponsiveGrid
 *     :columns="3"
 *     gap="lg"
 *   >
 *     <div v-for="item in items" :key="item.id">
 *       {{ item.content }}
 *     </div>
 *   </ResponsiveGrid>
 * </PageLayout>
 */

/**
 * 布局系统设计原则:
 *
 * 1. 移动优先 - 所有组件都从小屏幕开始设计
 * 2. 响应式 - 使用断点系统适配不同设备
 * 3. 性能优化 - 使用GPU加速和内容可见性API
 * 4. 可访问性 - 支持键盘导航和屏幕阅读器
 * 5. 浏览器兼容 - 支持现代浏览器和降级方案
 *
 * 断点系统:
 * - Mobile: < 640px
 * - Tablet: 641px - 1024px
 * - Desktop: 1025px - 1536px
 * - Wide: > 1536px
 */

/**
 * 性能优化特性:
 *
 * 1. CSS Containment - 减少浏览器重排和重绘
 * 2. Content Visibility - 延迟渲染不可见内容
 * 3. Will Change - GPU加速动画和过渡
 * 4. 防抖和节流 - 优化滚动和调整大小事件
 * 5. 懒加载 - 按需加载组件和资源
 */

/**
 * 可访问性特性:
 *
 * 1. 语义化HTML - 使用正确的HTML5元素
 * 2. ARIA属性 - 增强屏幕阅读器支持
 * 3. 键盘导航 - 完整的键盘操作支持
 * 4. 焦点管理 - 清晰的焦点指示器
 * 5. 跳过链接 - 允许跳过导航内容
 * 6. 减少动画 - 尊重用户的运动偏好
 */

/**
 * 浏览器兼容性:
 *
 * Chrome/Edge: 完全支持 (最新版本)
 * Firefox: 完全支持 (最新版本)
 * Safari: 完全支持 (14+)
 * Opera: 完全支持 (最新版本)
 * IE: 不支持 (使用现代浏览器)
 */
