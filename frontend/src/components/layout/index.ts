/**
 * PaperCrawler Layout Components
 * 响应式布局组件库
 *
 * @version 2.0.0
 * @update 2026-04-04
 */

// Core Layout Components
export { default as MainLayout } from './MainLayout.vue'
export { default as TopNavigation } from './TopNavigation.vue'
export { default as SidebarNavigation } from './SidebarNavigation.vue'
export { default as BreadcrumbBar } from './BreadcrumbBar.vue'
export { default as Footer } from './Footer.vue'

// Legacy Layout Components
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
 * // 主布局系统 (推荐)
 * import { MainLayout, TopNavigation, SidebarNavigation } from '@/components/layout'
 *
 * // 在路由配置中使用
 * {
 *   path: '/',
 *   component: MainLayout,
 *   children: [
 *     {
 *       path: 'dashboard',
 *       component: () => import('@/views/Dashboard.vue')
 *     }
 *   ]
 * }
 *
 * // 单独使用组件
 * import { BreadcrumbBar, Footer } from '@/components/layout'
 *
 * <template>
 *   <BreadcrumbBar />
 *   <router-view />
 *   <Footer />
 * </template>
 *
 * // 响应式容器
 * import { ResponsiveContainer, ResponsiveGrid } from '@/components/layout'
 *
 * <ResponsiveContainer maxWidth="1200px">
 *   <ResponsiveGrid :columns="3" gap="lg">
 *     <div v-for="item in items" :key="item.id">
 *       {{ item.content }}
 *     </div>
 *   </ResponsiveGrid>
 * </ResponsiveContainer>
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
 * 组件功能说明:
 *
 * MainLayout (主布局)
 * - 集成所有布局组件的主容器
 * - 响应式侧边栏和顶部导航
 * - 自动处理移动端适配
 * - 页面切换动画
 *
 * TopNavigation (顶部导航)
 * - Logo和品牌展示
 * - 主导航菜单
 * - 搜索、通知、用户菜单
 * - 主题切换和语言切换
 * - 移动端汉堡菜单
 *
 * SidebarNavigation (侧边导航)
 * - 可折叠多级菜单
 * - 自动记忆折叠状态
 * - 图标和文字标签
 * - 激活状态高亮
 * - 版本信息和快捷操作
 *
 * BreadcrumbBar (面包屑导航)
 * - 动态生成面包屑
 * - 自动解析路由元信息
 * - 图标支持
 * - 可选操作按钮插槽
 *
 * Footer (页脚)
 * - 品牌信息和描述
 * - 快速链接和资源
 * - 社交媒体链接
 * - 版权和版本信息
 */

/**
 * 性能优化特性:
 *
 * 1. CSS Containment - 减少浏览器重排和重绘
 * 2. Content Visibility - 延迟渲染不可见内容
 * 3. Will Change - GPU加速动画和过渡
 * 4. 防抖和节流 - 优化滚动和调整大小事件
 * 5. 懒加载 - 按需加载组件和资源
 * 6. 虚拟滚动 - 大列表性能优化
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
 * 7. 色彩对比 - 符合WCAG AA标准
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

/**
 * 主题支持:
 *
 * - Light Mode (浅色主题)
 * - Dark Mode (深色主题)
 * - System Theme (跟随系统)
 * - 自动切换和记忆功能
 */

/**
 * 国际化支持:
 *
 * - English (en)
 * - 中文 (zh)
 * - 易于扩展新语言
 * - 自动语言切换
 */

/**
 * Store集成:
 *
 * - useUIStore: 主题、语言、侧边栏状态
 * - useUserStore: 用户信息、认证状态
 * - useI18n: 国际化翻译
 */

/**
 * 设计系统遵循:
 *
 * - 颜色系统: 使用设计token
 * - 字体系统: 统一字体栈
 * - 间距系统: 4px基础网格
 * - 圆角系统: 统一圆角规范
 * - 阴影系统: 层级阴影规范
 * - 动画系统: 统一过渡时长
 *
 * 参考: /docs/UI-DESIGN-SYSTEM.md
 */
