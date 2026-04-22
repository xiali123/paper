// ==========================================
// LaTeX Editor Optimized Styles
// 全套优化：视觉 + 布局 + 交互 + 无障碍
// ==========================================

// 导入设计系统变量
@import '@/styles/variables.scss';

// ==========================================
// 全局优化变量和动画
// ==========================================

// 改进的过渡时间和缓动函数
$transition-fast: 150ms;
$transition-base: 200ms;
$transition-slow: 300ms;
$easing-out: cubic-bezier(0.4, 0, 0.2, 1);
$easing-in-out: cubic-bezier(0.4, 0, 0.6, 1);

// 改进的阴影系统
$shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.05);
$shadow-md: 0 4px 8px rgba(0, 0, 0, 0.1);
$shadow-lg: 0 8px 16px rgba(0, 0, 0, 0.15);
$shadow-focus: 0 0 0 3px rgba(99, 102, 241, 0.1), 0 0 0 1px var(--el-color-primary);
$shadow-premium: 0 8px 32px rgba(99, 102, 241, 0.15);

// ==========================================
// 1. 按钮状态优化
// ==========================================

// 主按钮增强样式
.el-button {
  transition: all $transition-base $easing-out;
  position: relative;
  overflow: hidden;

  // Hover 状态 - 轻微上移
  &:hover:not(:disabled) {
    transform: translateY(-1px);
    box-shadow: $shadow-md;
  }

  // Active 状态 - 按下效果
  &:active:not(:disabled) {
    transform: translateY(0) scale(0.98);
    box-shadow: $shadow-sm;
  }

  // Focus 状态 - 清晰焦点环
  &:focus-visible {
    outline: 2px solid var(--el-color-primary);
    outline-offset: 2px;
    border-radius: var(--el-border-radius-base);
  }

  // Loading 状态 - 旋转动画
  &.is-loading {
    position: relative;
    color: transparent !important;
    pointer-events: none;

    &::after {
      content: '';
      position: absolute;
      inset: 0;
      width: 16px;
      height: 16px;
      top: 50%;
      left: 50%;
      margin-left: -8px;
      margin-top: -8px;
      border: 2px solid currentColor;
      border-right-color: transparent;
      border-radius: 50%;
      animation: spin 0.6s linear infinite;
    }
  }
}

@keyframes spin {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

// 按钮组增强
.el-button-group {
  .el-button {
    // 按钮组内的按钮间距和分隔
    &:not(:last-child) {
      margin-right: 4px;
    }

    // 首个和最后一个按钮的圆角处理
    &:first-child {
      border-top-left-radius: var(--el-border-radius-base);
      border-bottom-left-radius: var(--el-border-radius-base);
    }

    &:last-child {
      border-top-right-radius: var(--el-border-radius-base);
      border-bottom-right-radius: var(--el-border-radius-base);
    }
  }
}

// ==========================================
// 2. 工具栏优化
// ==========================================

.editor-toolbar {
  // 工具栏分组样式
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 10px 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);
  background: linear-gradient(
    to bottom,
    var(--el-bg-color-page) 0%,
    rgba(0, 0, 0, 0.02) 100%
  );

  // 工具栏分组分隔线
  .el-button-group {
    position: relative;
    padding: 0 8px;

    &:not(:last-child)::after {
      content: '';
      position: absolute;
      right: 0;
      top: 50%;
      transform: translateY(-50%);
      width: 1px;
      height: 20px;
      background: var(--el-border-color-light);
    }
  }

  // 工具提示优化
  .el-tooltip__popper {
    .el-tooltip__inner {
      background: var(--el-bg-color-overlay);
      color: var(--el-text-color-primary);
      border: 1px solid var(--el-border-color);
      padding: 6px 10px;
      font-size: 12px;
      border-radius: 6px;
      box-shadow: $shadow-md;
    }
  }

  // 编译状态标签增强
  .compilation-status {
    .el-tag {
      display: inline-flex;
      align-items: center;
      gap: 4px;
      padding: 4px 10px;
      border-radius: 12px;
      font-weight: 500;
      font-size: 12px;
      transition: all $transition-base $easing-out;

      &.el-tag--success {
        background: linear-gradient(135deg, #d1fae5 0%, #a7f3d0 100%);
        border-color: #34d399;
        color: #065f46;
      }

      &.el-tag--danger {
        background: linear-gradient(135deg, #fee2e2 0%, #fecaca 100%);
        border-color: #f87171;
        color: #991b1b;
      }

      &.el-tag--warning {
        background: linear-gradient(135deg, #fef3c7 0%, #fde68a 100%);
        border-color: #fbbf24;
        color: #92400e;
      }

      &.el-tag--info {
        background: linear-gradient(135deg, #dbeafe 0%, #bfdbfe 100%);
        border-color: #60a5fa;
        color: #1e40af;
      }
    }
  }
}

// ==========================================
// 3. 状态栏优化
// ==========================================

.editor-status-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 6px 16px;
  border-top: 1px solid var(--el-border-color-lighter);
  background: var(--el-bg-color-overlay);
  font-size: 12px;
  color: var(--el-text-color-secondary);

  .status-left,
  .status-right {
    display: flex;
    gap: 16px;
    align-items: center;
  }

  // 状态指示器增强
  .status-indicator {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    padding: 2px 8px;
    border-radius: 12px;
    font-weight: 500;
    transition: all $transition-base $easing-out;

    &.is-saving {
      color: var(--el-color-primary);
      background-color: var(--el-color-primary-light-9);

      .el-icon {
        animation: rotating 2s linear infinite;
      }
    }

    &.is-unsaved {
      color: var(--el-color-warning);
      background-color: var(--el-color-warning-light-9);
    }

    &.is-saved {
      color: var(--el-color-success);
      background-color: var(--el-color-success-light-9);
    }

    &.is-modified {
      color: var(--el-color-warning);
      background-color: var(--el-color-warning-light-9);
    }
  }

  // 自动保存状态样式改进
  .auto-save-saving,
  .auto-save-unsaved,
  .auto-save-saved,
  .auto-save-ready {
    display: flex;
    align-items: center;
    gap: 4px;
    padding: 2px 8px;
    border-radius: 12px;
    font-weight: 500;
    transition: all $transition-base $easing-out;
  }

  .auto-save-saving {
    color: var(--el-color-primary);
    background-color: var(--el-color-primary-light-9);
  }

  .auto-save-unsaved {
    color: var(--el-color-warning);
    background-color: var(--el-color-warning-light-9);
  }

  .auto-save-saved {
    color: var(--el-color-success);
    background-color: var(--el-color-success-light-9);
  }

  .auto-save-ready {
    color: var(--el-text-color-secondary);
  }
}

// ==========================================
// 4. 预览面板优化
// ==========================================

.preview-panel {
  // 预览头部增强
  .preview-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 14px 18px;
    border-bottom: 1px solid var(--el-border-color-lighter);
    background: linear-gradient(
      to bottom,
      var(--el-bg-color-page) 0%,
      rgba(0, 0, 0, 0.02) 100%
    );

    h3 {
      margin: 0;
      font-size: 15px;
      font-weight: 600;
      color: var(--el-text-color-primary);
      display: flex;
      align-items: center;
      gap: 6px;

      &::before {
        content: '';
        width: 4px;
        height: 16px;
        background: var(--el-color-primary);
        border-radius: 2px;
      }
    }
  }

  // 预览控件优化
  .preview-controls {
    display: flex;
    gap: 6px;

    .el-button {
      min-width: 32px;
      height: 32px;
      padding: 0 8px;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 14px;
      font-weight: 500;
    }
  }
}

// ==========================================
// 5. 移动端标签栏优化
// ==========================================

.mobile-tabs {
  display: none;
  position: sticky;
  top: 0;
  z-index: 100;
  background: var(--el-bg-color-page);
  border-bottom: 1px solid var(--el-border-color);
  padding: 10px 12px;
  box-shadow: $shadow-sm;

  .mobile-tab {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    padding: 12px;
    border-radius: 10px;
    cursor: pointer;
    transition: all $transition-base $easing-out;
    color: var(--el-text-color-secondary);
    font-weight: 500;
    font-size: 14px;
    min-height: 44px; // 触摸目标最小尺寸
    position: relative;
    overflow: hidden;

    // 涟纹效果
    &::before {
      content: '';
      position: absolute;
      inset: 0;
      background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
      transform: translateX(-100%);
      transition: transform $transition-slow $easing-out;
    }

    &:active::before {
      transform: translateX(100%);
    }

    &.active {
      background: var(--el-color-primary);
      color: white;
      box-shadow: $shadow-md;

      &::after {
        content: '';
        position: absolute;
        bottom: -2px;
        left: 50%;
        transform: translateX(-50%);
        width: 20px;
        height: 3px;
        background: white;
        border-radius: 2px;
      }
    }

    .status-badge {
      margin-left: 4px;
    }
  }
}

.editor-main.is-mobile {
  .mobile-tabs {
    display: flex;
    gap: 8px;
  }
}

// ==========================================
// 6. 错误面板优化
// ==========================================

.error-panel {
  border-top: 1px solid var(--el-border-color-lighter);
  background: var(--el-bg-color-page);

  .error-header {
    padding: 10px 16px 0;
  }

  .error-content {
    max-height: 200px;
    overflow-y: auto;
    padding: 8px 16px;

    .error-item {
      display: flex;
      align-items: flex-start;
      gap: 10px;
      padding: 10px 12px;
      border-bottom: 1px solid var(--el-border-color-lighter);
      cursor: pointer;
      transition: all $transition-base $easing-out;
      border-left: 3px solid transparent;
      border-radius: 4px;

      &:last-child {
        border-bottom: none;
      }

      &:hover {
        background: var(--el-bg-color-overlay);
        transform: translateX(4px);
      }

      &.error {
        border-left-color: var(--el-color-danger);
        background: linear-gradient(90deg, rgba(239, 68, 68, 0.05) 0%, transparent 100%);
      }

      &.warning {
        border-left-color: var(--el-color-warning);
        background: linear-gradient(90deg, rgba(245, 158, 11, 0.05) 0%, transparent 100%);
      }

      .error-line {
        font-weight: 600;
        color: var(--el-text-color-primary);
        min-width: 60px;
        font-size: 12px;
      }

      .error-message {
        flex: 1;
        color: var(--el-text-color-regular);
        font-size: 13px;
        line-height: 1.4;
      }

      .el-icon {
        margin-top: 2px;
      }
    }
  }
}

// ==========================================
// 7. 文档大纲优化
// ==========================================

.document-outline {
  .outline-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 14px 16px;
    border-bottom: 1px solid var(--el-border-color-lighter);
    background: var(--el-bg-color-page);

    h3 {
      margin: 0;
      font-size: 14px;
      font-weight: 600;
      color: var(--el-text-color-primary);
    }
  }

  .outline-content {
    padding: 8px 12px;
    height: calc(100% - 50px);
    overflow-y: auto;
  }
}

// 大纲项优化（在外部组件中）
.outline-item {
  padding: 8px 12px;
  cursor: pointer;
  border-radius: 6px;
  transition: all $transition-base $easing-out;
  border-left: 2px solid transparent;

  &:hover {
    background: var(--el-bg-color-overlay);
    border-left-color: var(--el-border-color);
    transform: translateX(2px);
  }

  &.active {
    background: var(--el-color-primary-light-9);
    border-left-color: var(--el-color-primary);
    color: var(--el-color-primary);
    font-weight: 500;
  }
}

// ==========================================
// 8. 符号面板优化
// ==========================================

.symbol-palette {
  .symbol-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(56px, 1fr));
    gap: 10px;
    padding: 16px;

    .symbol-item {
      display: flex;
      align-items: center;
      justify-content: center;
      height: 50px;
      border: 1px solid var(--el-border-color-lighter);
      border-radius: 8px;
      cursor: pointer;
      transition: all $transition-base $easing-out;
      background: var(--el-bg-color);
      position: relative;
      overflow: hidden;

      // 涟纹效果
      &::before {
        content: '';
        position: absolute;
        inset: 0;
        background: radial-gradient(circle, rgba(99, 102, 241, 0.1) 0%, transparent 70%);
        opacity: 0;
        transform: scale(0);
        transition: all $transition-base $easing-out;
      }

      &:hover {
        border-color: var(--el-color-primary);
        transform: translateY(-2px);
        box-shadow: $shadow-md;

        &::before {
          opacity: 1;
          transform: scale(1);
        }
      }

      &:active {
        transform: translateY(0);
        box-shadow: $shadow-sm;
      }

      // Focus 状态
      &:focus-visible {
        outline: 2px solid var(--el-color-primary);
        outline-offset: 2px;
      }
    }
  }
}

// ==========================================
// 9. 响应式优化
// ==========================================

@media (max-width: 768px) {
  .latex-editor-view {
    .editor-header {
      padding: 12px 16px;

      .document-info {
        gap: 12px;

        .document-actions {
          width: 100%;

          .el-button-group {
            display: flex;
            width: 100%;

            .el-button {
              flex: 1;
              min-height: 44px; // 触摸目标最小尺寸
            }
          }
        }
      }
    }

    .editor-toolbar {
      padding: 10px 12px;
      gap: 8px;
      flex-wrap: wrap;

      .el-button-group {
        margin: 0 4px;
      }

      // 隐藏工具栏文本以节省空间
      .toolbar-text {
        display: none;
      }
    }

    .editor-status-bar {
      padding: 10px 14px;
      font-size: 11px;

      .status-left,
      .status-right {
        gap: 12px;
      }
    }
  }

  .symbol-palette {
    .symbol-grid {
      grid-template-columns: repeat(auto-fill, minmax(48px, 1fr));
      gap: 8px;
      padding: 12px;

      .symbol-item {
        height: 48px; // 触摸友好的尺寸
        min-height: 48px;
      }
    }
  }
}

// ==========================================
// 10. 无障碍增强
// ==========================================

// 屏幕阅读器只读内容
.sr-only {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border-width: 0;
}

// 仅在焦点时显示的轮廓
.focus-visible-only {
  &:focus:not(:focus-visible) {
    outline: none;
  }

  &:focus-visible {
    outline: 2px solid var(--el-color-primary);
    outline-offset: 2px;
    border-radius: 4px;
  }
}

// 高对比度模式支持
@media (prefers-contrast: high) {
  .el-button,
  .symbol-item,
  .outline-item {
    border-width: 2px;
  }

  .latex-textarea {
    border-width: 2px;
  }
}

// 减少动画模式支持
@media (prefers-reduced-motion: reduce) {
  *,
  *::before,
  *::after {
    animation-duration: 0.01ms !important;
    animation-iteration-count: 1 !important;
    transition-duration: 0.01ms !important;
  }
}

// ==========================================
// 11. 深色模式增强
// ==========================================

[data-theme="dark"] {
  .latex-editor-view {
    background: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);

    .editor-header {
      background: rgba(15, 23, 42, 0.8);
      backdrop-filter: blur(20px) saturate(180%);
      border-bottom: 1px solid rgba(148, 163, 184, 0.1);
    }

    .editor-toolbar {
      background: rgba(30, 41, 59, 0.5);

      .el-button-group {
        &:not(:last-child)::after {
          background: rgba(148, 163, 184, 0.2);
        }
      }
    }

    .editor-status-bar {
      background: rgba(15, 23, 42, 0.6);
      border-top: 1px solid rgba(148, 163, 184, 0.1);
    }

    .preview-panel {
      background: rgba(15, 23, 42, 0.5);

      .preview-content {
        background: rgba(30, 41, 59, 0.3);
      }
    }

    .document-outline {
      background: rgba(30, 41, 59, 0.5);
      border-right: 1px solid rgba(148, 163, 184, 0.1);
    }

    // 暗色模式下的发光效果
    .el-button.el-button--primary {
      box-shadow: 0 8px 32px rgba(99, 102, 241, 0.4);

      &:hover {
        box-shadow: 0 12px 40px rgba(99, 102, 241, 0.5);
      }
    }
  }
}

// ==========================================
// 12. 加载和动画状态
// ==========================================

// 淡入动画
@keyframes fadeIn {
  from {
    opacity: 0;
    transform: translateY(8px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

// 滑入动画
@keyframes slideIn {
  from {
    opacity: 0;
    transform: translateX(-10px);
  }
  to {
    opacity: 1;
    transform: translateX(0);
  }
}

// 缩放动画
@keyframes scaleIn {
  from {
    opacity: 0;
    transform: scale(0.95);
  }
  to {
    opacity: 1;
    transform: scale(1);
  }
}

// 应用动画到相关组件
.preview-content > * {
  animation: fadeIn 0.3s ease-out;
}

.document-outline .outline-item {
  animation: slideIn 0.2s ease-out;
}

.symbol-item {
  animation: scaleIn 0.2s ease-out;
}

// ==========================================
// 13. 滚动条美化
// ==========================================

// Webkit 滚动条
::-webkit-scrollbar {
  width: 10px;
  height: 10px;
}

::-webkit-scrollbar-track {
  background: var(--el-bg-color);
  border-radius: 10px;
}

::-webkit-scrollbar-thumb {
  background: var(--el-border-color-dark);
  border-radius: 10px;
  transition: background $transition-base;

  &:hover {
    background: var(--el-border-color-darker);
  }
}

// Firefox 滚动条
* {
  scrollbar-width: thin;
  scrollbar-color: var(--el-border-color-dark) var(--el-bg-color);
}

// ==========================================
// 14. 改进的空状态
// ==========================================

.preview-empty,
.editor-empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 16px;
  padding: 64px 32px;
  text-align: center;
  color: var(--el-text-color-secondary);

  .empty-icon {
    font-size: 64px;
    opacity: 0.4;
    margin-bottom: 16px;
  }

  .empty-title {
    font-size: 18px;
    font-weight: 600;
    color: var(--el-text-color-primary);
    margin-bottom: 8px;
  }

  .empty-description {
    font-size: 14px;
    color: var(--el-text-color-secondary);
    margin-bottom: 24px;
    max-width: 300px;
    line-height: 1.5;
  }

  .empty-action {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    padding: 10px 20px;
    background: var(--el-color-primary);
    color: white;
    border-radius: 8px;
    font-weight: 500;
    cursor: pointer;
    transition: all $transition-base $easing-out;

    &:hover {
      background: var(--el-color-primary-dark);
      transform: translateY(-1px);
      box-shadow: $shadow-md;
    }
  }
}

// ==========================================
// 15. 编译状态脉冲动画
// ==========================================

@keyframes pulse {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.7;
  }
}

.compilation-status .el-tag--compiling {
  animation: pulse 1.5s ease-in-out infinite;
}

// ==========================================
// 16. 协作用头像增强
// ==========================================

.collaboration-users {
  display: flex;
  align-items: center;
  gap: 12px;

  .el-avatar-group .el-avatar {
    border: 2px solid var(--el-bg-color-page);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
    transition: all $transition-base $easing-out;

    &:hover {
      transform: translateY(-2px);
      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
    }
  }

  .collaboration-status {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 4px 10px;
    background: var(--el-color-success-light-9);
    color: var(--el-color-success);
    border-radius: 12px;
    font-size: 12px;
    font-weight: 500;
  }
}

// ==========================================
// 17. 改进的下拉菜单
// ==========================================

.el-dropdown-menu {
  border: 1px solid var(--el-border-color-lighter);
  box-shadow: $shadow-lg;
  border-radius: 8px;
  padding: 4px;

  .el-dropdown-menu__item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    padding: 8px 12px;
    border-radius: 4px;
    transition: all $transition-fast $easing-out;

    &:hover {
      background: var(--el-bg-color-overlay);
    }

    &:active {
      background: var(--el-color-primary-light-9);
    }
  }
}

// ==========================================
// 18. 抽屉优化
// ==========================================

.el-drawer {
  .el-drawer__header {
    margin-bottom: 0;
    padding: 16px 20px;
    border-bottom: 1px solid var(--el-border-color-lighter);

    .el-drawer__title {
      font-size: 16px;
      font-weight: 600;
      color: var(--el-text-color-primary);
    }
  }
}

// ==========================================
// 19. 面包屑导航增强
// ==========================================

.el-breadcrumb {
  .el-breadcrumb__item {
    font-size: 13px;
    font-weight: 500;

    &:last-child {
      color: var(--el-text-color-primary);
      font-weight: 600;
    }
  }

  .el-breadcrumb__separator {
    color: var(--el-text-color-secondary);
    margin: 0 6px;
  }
}

// ==========================================
// 20. 成功/错误反馈动画
// ==========================================

// 成功状态动画
@keyframes successPulse {
  0% {
    box-shadow: 0 0 0 0 rgba(82, 196, 26, 0.7);
  }
  50% {
    box-shadow: 0 0 0 8px rgba(82, 196, 26, 0.3);
  }
  100% {
    box-shadow: 0 0 0 0 rgba(82, 196, 26, 0.7);
  }
}

.save-success {
  animation: successPulse 0.6s ease-in-out;
}

// 错误抖动动画
@keyframes shake {
  0%, 100% {
    transform: translateX(0);
  }
  10%, 30%, 50%, 70%, 90% {
    transform: translateX(-4px);
  }
  20%, 40%, 60%, 80% {
    transform: translateX(4px);
  }
}

.save-error {
  animation: shake 0.4s ease-in-out;
}

// ==========================================
// 21. 响应式字体大小
// ==========================================

@media (max-width: 768px) {
  .latex-editor-view {
    // 移动端字体调整
    font-size: 14px; // 防止iOS缩放

    .editor-header,
    .preview-header {
      font-size: 14px;

      h3 {
        font-size: 15px;
      }
    }

    .editor-toolbar {
      font-size: 13px;
    }

    .editor-status-bar {
      font-size: 11px;
    }
  }
}

// ==========================================
// 22. 打印样式
// ==========================================

@media print {
  .latex-editor-view {
    .editor-header,
    .editor-toolbar,
    .editor-status-bar,
    .mobile-tabs {
      display: none !important;
    }

    .editor-panel,
    .preview-panel {
      flex: 1;
      page-break-inside: avoid;
    }
  }
}

// ==========================================
// 23. 性能优化
// ==========================================

// GPU 加速
.editor-wrapper,
.preview-content,
.document-outline,
.symbol-grid {
  will-change: transform;
  transform: translateZ(0);
  backface-visibility: hidden;
}

// 内容可见性优化
.editor-wrapper > *,
.preview-content > * {
  contain: content style layout;
  content-visibility: auto;
}

// ==========================================
// 24. 自定义滚动条样式
// ==========================================

.editor-textarea,
.preview-content,
.document-outline,
.outline-content {
  &::-webkit-scrollbar {
    width: 8px;
    height: 8px;
  }

  &::-webkit-scrollbar-track {
    background: transparent;
  }

  &::-webkit-scrollbar-thumb {
    background: var(--el-border-color);
    border-radius: 4px;

    &:hover {
      background: var(--el-border-color-dark);
    }
  }

  & {
    scrollbar-width: thin;
    scrollbar-color: var(--el-border-color) transparent;
  }
}
