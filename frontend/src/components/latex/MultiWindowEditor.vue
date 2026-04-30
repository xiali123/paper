<template>
  <div class="multi-window-editor" ref="containerRef">
    <!-- 窗口标签栏 -->
    <div class="window-tabs">
      <div
        v-for="window in windows"
        :key="window.id"
        class="window-tab"
        :class="{ 'is-active': activeWindowId === window.id }"
        @click="activateWindow(window.id)"
        @contextmenu.prevent="showTabMenu($event, window)"
      >
        <el-icon class="tab-icon"><Document /></el-icon>
        <span class="tab-title">{{ window.title }}</span>
        <span v-if="window.modified" class="tab-modified">●</span>
        <el-icon
          class="tab-close"
          :size="14"
          @click.stop="closeWindow(window.id)"
        >
          <Close />
        </el-icon>
      </div>
      <el-button
        class="new-tab-button"
        size="small"
        text
        @click="createNewWindow"
      >
        <el-icon><Plus /></el-icon>
      </el-button>
    </div>

    <!-- 分屏容器 -->
    <div
      class="split-container"
      :class="`layout-${layoutMode}`"
    >
      <!-- 主编辑区 -->
      <div
        v-if="layoutMode === 'single' || layoutMode === 'vertical-split' || layoutMode === 'horizontal-split'"
        class="editor-pane"
        :style="paneStyle(1)"
      >
        <LatexEditor
          v-if="activeWindow"
          :model-value="activeWindow.content"
          
          
          @update:model-value="handleContentUpdate"
          
        />
      </div>

      <!-- 第二编辑区（分屏模式） -->
      <div
        v-if="layoutMode === 'horizontal-split'"
        class="editor-pane split-horizontal"
        :style="paneStyle(2)"
      >
        <div v-if="!secondaryWindow" class="empty-pane">
          <el-empty description="选择一个文档进行分屏编辑" :image-size="60">
            <el-button size="small" @click="showWindowSelector">选择文档</el-button>
          </el-empty>
        </div>
        <LatexEditor
          v-else
          :model-value="secondaryWindow.content"
          
          
          @update:model-value="handleSecondaryContentUpdate"
        />
      </div>

      <!-- 垂直分屏 -->
      <div
        v-if="layoutMode === 'vertical-split'"
        class="editor-pane split-vertical"
        :style="paneStyle(2)"
      >
        <div v-if="!secondaryWindow" class="empty-pane">
          <el-empty description="选择一个文档进行分屏编辑" :image-size="60">
            <el-button size="small" @click="showWindowSelector">选择文档</el-button>
          </el-empty>
        </div>
        <LatexEditor
          v-else
          :model-value="secondaryWindow.content"
          
          
          @update:model-value="handleSecondaryContentUpdate"
        />
      </div>

      <!-- 分割线（可拖拽调整大小） -->
      <div
        v-if="layoutMode === 'horizontal-split'"
        class="splitter-horizontal"
        @mousedown="startSplitResize($event, 'horizontal')"
      >
        <div class="splitter-handle"></div>
      </div>

      <div
        v-if="layoutMode === 'vertical-split'"
        class="splitter-vertical"
        @mousedown="startSplitResize($event, 'vertical')"
      >
        <div class="splitter-handle"></div>
      </div>
    </div>

    <!-- 布局切换按钮 -->
    <div class="layout-controls">
      <el-dropdown trigger="click" @command="setLayoutMode">
        <el-button size="small" :icon="Grid">
          布局
        </el-button>
        <template #dropdown>
          <el-dropdown-menu>
            <el-dropdown-item command="single">
              <el-icon><Document /></el-icon>
              单窗口
            </el-dropdown-item>
            <el-dropdown-item command="horizontal-split">
              <el-icon><CopyDocument /></el-icon>
              水平分屏
            </el-dropdown-item>
            <el-dropdown-item command="vertical-split">
              <el-icon><CopyDocument style="transform: rotate(90deg)" />
              垂直分屏
            </el-dropdown-item>
          </el-dropdown-menu>
        </template>
      </el-dropdown>

      <el-dropdown trigger="click" @command="handleWindowAction">
        <el-button size="small" :icon="More">
          窗口
        </el-button>
        <template #dropdown>
          <el-dropdown-menu>
            <el-dropdown-item command="move-left" :disabled="layoutMode === 'single'">
              <el-icon><ArrowLeft /></el-icon>
              移至左侧
            </el-dropdown-item>
            <el-dropdown-item command="move-right" :disabled="layoutMode === 'single'">
              <el-icon><ArrowRight /></el-icon>
              移至右侧
            </el-dropdown-item>
            <el-dropdown-item command="swap" :disabled="layoutMode === 'single'">
              <el-icon><Sort /></el-icon>
              交换位置
            </el-dropdown-item>
          </el-dropdown-menu>
        </template>
      </el-dropdown>
    </div>

    <!-- 窗口选择对话框 -->
    <el-dialog v-model="showSelector" title="选择文档" width="500px">
      <el-list>
        <el-list-item
          v-for="window in availableWindows"
          :key="window.id"
          @click="selectSecondaryWindow(window)"
          class="window-selector-item"
        >
          <div class="selector-info">
            <div class="selector-title">{{ window.title }}</div>
            <div class="selector-path">{{ window.path || '未保存' }}</div>
          </div>
          <el-icon v-if="secondaryWindowId === window.id" color="#409eff">
            <Select />
          </el-icon>
        </el-list-item>
      </el-list>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue'
import {
  Document,
  Close,
  Plus,
  Grid,
  CopyDocument,
  ArrowLeft,
  ArrowRight,
  Sort,
  More,
  Select
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import LatexEditor from './LatexEditor.vue' // 使用现有LaTeX编辑器组件

interface EditorWindow {
  id: string
  title: string
  content: string
  path?: string
  modified: boolean
  cursorPosition?: { lineNumber: number; column: number }
}

type LayoutMode = 'single' | 'horizontal-split' | 'vertical-split'

// 状态
const windows = ref<EditorWindow[]>([])
const activeWindowId = ref<string>('')
const secondaryWindowId = ref<string | null>(null)
const layoutMode = ref<LayoutMode>('single')
const splitRatio = ref(50) // 分屏比例 (0-100)
const showSelector = ref(false)
const containerRef = ref<HTMLElement>()

// 当前激活窗口
const activeWindow = computed(() => {
  return windows.value.find(w => w.id === activeWindowId.value)
})

// 次要窗口（分屏）
const secondaryWindow = computed(() => {
  if (!secondaryWindowId.value) return null
  return windows.value.find(w => w.id === secondaryWindowId.value)
})

// 可选择的窗口（用于分屏）
const availableWindows = computed(() => {
  return windows.value.filter(w => w.id !== activeWindowId.value)
})

// 面板样式
const paneStyle = (pane: 1 | 2) => {
  if (layoutMode.value === 'single') {
    return { width: '100%', height: '100%' }
  }

  if (layoutMode.value === 'horizontal-split') {
    const width = pane === 1 ? splitRatio.value : (100 - splitRatio.value)
    return { width: `${width}%`, height: '100%' }
  }

  if (layoutMode.value === 'vertical-split') {
    const height = pane === 1 ? splitRatio.value : (100 - splitRatio.value)
    return { width: '100%', height: `${height}%` }
  }

  return {}
}

// 创建新窗口
const createNewWindow = () => {
  const newWindow: EditorWindow = {
    id: `window-${Date.now()}`,
    title: `未命名${windows.value.length + 1}`,
    content: '',
    modified: false
  }

  windows.value.push(newWindow)
  activateWindow(newWindow.id)
}

// 激活窗口
const activateWindow = (id: string) => {
  activeWindowId.value = id
  saveWindowState()
}

// 关闭窗口
const closeWindow = async (id: string) => {
  const window = windows.value.find(w => w.id === id)
  if (!window) return

  if (window.modified) {
    try {
      await ElMessageBox.confirm(
        `"${window.title}" 有未保存的更改，确定关闭吗？`,
        '确认关闭',
        { type: 'warning' }
      )
    } catch {
      return // 用户取消
    }
  }

  windows.value = windows.value.filter(w => w.id !== id)

  // 如果关闭的是当前窗口
  if (activeWindowId.value === id) {
    if (windows.value.length > 0) {
      activeWindowId.value = windows.value[0].id
    } else {
      activeWindowId.value = ''
    }
  }

  // 如果关闭的是次要窗口
  if (secondaryWindowId.value === id) {
    secondaryWindowId.value = null
  }

  // 如果只剩一个窗口，切换到单窗口模式
  if (windows.value.length === 1) {
    layoutMode.value = 'single'
  }

  saveWindowState()
}

// 设置布局模式
const setLayoutMode = (mode: LayoutMode) => {
  if (mode !== 'single' && windows.value.length < 2) {
    ElMessage.warning('需要至少两个文档才能使用分屏模式')
    return
  }

  layoutMode.value = mode

  if (mode === 'single') {
    secondaryWindowId.value = null
  } else if (mode === 'horizontal-split' || mode === 'vertical-split') {
    // 自动选择次要窗口
    if (!secondaryWindowId.value && windows.value.length > 1) {
      secondaryWindowId.value = windows.value.find(w => w.id !== activeWindowId.value)?.id || null
    }
  }

  saveWindowState()
}

// 显示窗口选择器
const showWindowSelector = () => {
  showSelector.value = true
}

// 选择次要窗口
const selectSecondaryWindow = (window: EditorWindow) => {
  secondaryWindowId.value = window.id
  showSelector.value = false
}

// 处理内容更新
const handleContentUpdate = (content: string) => {
  if (activeWindow.value) {
    activeWindow.value.content = content
    activeWindow.value.modified = true
  }
}

// 处理次要窗口内容更新
const handleSecondaryContentUpdate = (content: string) => {
  if (secondaryWindow.value) {
    secondaryWindow.value.content = content
    secondaryWindow.value.modified = true
  }
}

// 编辑器准备就绪
// 开始拖拽调整分割
const startSplitResize = (event: MouseEvent, direction: 'horizontal' | 'vertical') => {
  event.preventDefault()

  const container = containerRef.value
  if (!container) return

  const startX = event.clientX
  const startY = event.clientY
  const startRatio = splitRatio.value

  const handleMouseMove = (e: MouseEvent) => {
    if (!container) return

    const rect = container.getBoundingClientRect()

    if (direction === 'horizontal') {
      const deltaX = e.clientX - startX
      const deltaPercent = (deltaX / rect.width) * 100
      splitRatio.value = Math.max(20, Math.min(80, startRatio + deltaPercent))
    } else {
      const deltaY = e.clientY - startY
      const deltaPercent = (deltaY / rect.height) * 100
      splitRatio.value = Math.max(20, Math.min(80, startRatio + deltaPercent))
    }
  }

  const handleMouseUp = () => {
    document.removeEventListener('mousemove', handleMouseMove)
    document.removeEventListener('mouseup', handleMouseUp)
    saveWindowState()
  }

  document.addEventListener('mousemove', handleMouseMove)
  document.addEventListener('mouseup', handleMouseUp)
}

// 显示标签右键菜单
const showTabMenu = (event: MouseEvent, window: EditorWindow) => {
  // TODO: 实现右键菜单
}

// 处理窗口操作
const handleWindowAction = (action: string) => {
  switch (action) {
    case 'move-left':
      if (secondaryWindowId.value && activeWindowId.value !== secondaryWindowId.value) {
        ;[activeWindowId.value, secondaryWindowId.value] = [secondaryWindowId.value, activeWindowId.value]
      }
      break
    case 'move-right':
      if (activeWindowId.value !== secondaryWindowId.value) {
        ;[activeWindowId.value, secondaryWindowId.value] = [secondaryWindowId.value, activeWindowId.value]
      }
      break
    case 'swap':
      ;[activeWindowId.value, secondaryWindowId.value] = [secondaryWindowId.value, activeWindowId.value]
      break
  }
}

// 保存窗口状态
const saveWindowState = () => {
  const state = {
    windows: windows.value.map(w => ({
      ...w,
      content: '' // 不保存内容到localStorage
    })),
    activeWindowId: activeWindowId.value,
    secondaryWindowId: secondaryWindowId.value,
    layoutMode: layoutMode.value,
    splitRatio: splitRatio.value
  }

  localStorage.setItem('editor-windows', JSON.stringify(state))
}

// 加载窗口状态
const loadWindowState = () => {
  const saved = localStorage.getItem('editor-windows')
  if (saved) {
    try {
      const state = JSON.parse(saved)
      // 恢复窗口状态（内容需要从其他地方加载）
      layoutMode.value = state.layoutMode || 'single'
      splitRatio.value = state.splitRatio || 50
      secondaryWindowId.value = state.secondaryWindowId
    } catch (error) {
      console.error('Failed to load window state:', error)
    }
  }
}

// 快捷键支持
const handleKeyDown = (event: KeyboardEvent) => {
  // Ctrl+Shift+T: 新建窗口
  if (event.ctrlKey && event.shiftKey && event.key === 't') {
    event.preventDefault()
    createNewWindow()
  }

  // Ctrl+Tab: 切换到下一个窗口
  if (event.ctrlKey && event.key === 'Tab') {
    event.preventDefault()
    const currentIndex = windows.value.findIndex(w => w.id === activeWindowId.value)
    const nextIndex = (currentIndex + 1) % windows.value.length
    activateWindow(windows.value[nextIndex].id)
  }

  // Ctrl+Shift+Tab: 切换到上一个窗口
  if (event.ctrlKey && event.shiftKey && event.key === 'Tab') {
    event.preventDefault()
    const currentIndex = windows.value.findIndex(w => w.id === activeWindowId.value)
    const prevIndex = (currentIndex - 1 + windows.value.length) % windows.value.length
    activateWindow(windows.value[prevIndex].id)
  }

  // Ctrl+K Ctrl+L: 切换布局
  if (event.ctrlKey && event.key === 'k') {
    setTimeout(() => {
      const handleSecondKey = (e: KeyboardEvent) => {
        if (e.key === 'l') {
          e.preventDefault()
          cycleLayoutMode()
          document.removeEventListener('keydown', handleSecondKey)
        }
      }
      document.addEventListener('keydown', handleSecondKey, { once: true })
    }, 0)
  }
}

// 循环切换布局模式
const cycleLayoutMode = () => {
  const modes: LayoutMode[] = ['single', 'horizontal-split', 'vertical-split']
  const currentIndex = modes.indexOf(layoutMode.value)
  const nextMode = modes[(currentIndex + 1) % modes.length]
  setLayoutMode(nextMode)
}

// 生命周期
onMounted(() => {
  loadWindowState()

  // 创建默认窗口
  if (windows.value.length === 0) {
    createNewWindow()
  }

  document.addEventListener('keydown', handleKeyDown)
})

onUnmounted(() => {
  document.removeEventListener('keydown', handleKeyDown)
})

// 暴露方法
defineExpose({
  createNewWindow,
  openDocument: (title: string, content: string, path?: string) => {
    const newWindow: EditorWindow = {
      id: `window-${Date.now()}`,
      title,
      content,
      path,
      modified: false
    }
    windows.value.push(newWindow)
    activateWindow(newWindow.id)
  },
  closeAllWindows: () => {
    windows.value = []
    activeWindowId.value = ''
    secondaryWindowId.value = null
  }
})
</script>

<style scoped lang="scss">
.multi-window-editor {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: var(--el-bg-color-page);

  .window-tabs {
    display: flex;
    align-items: center;
    gap: 4px;
    padding: 8px 12px 0;
    background: var(--el-bg-color);
    border-bottom: 1px solid var(--el-border-color-light);

    .window-tab {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 6px 12px;
      border-radius: 6px 6px 0 0;
      cursor: pointer;
      user-select: none;
      border: 1px solid transparent;
      transition: all 0.2s;

      &:hover {
        background: var(--el-fill-color-light);
      }

      &.is-active {
        background: var(--el-bg-color-page);
        border-color: var(--el-border-color);
        border-bottom-color: var(--el-bg-color-page);
      }

      .tab-icon {
        font-size: 14px;
        color: var(--el-text-color-secondary);
      }

      .tab-title {
        font-size: 13px;
        max-width: 150px;
        overflow: hidden;
        text-overflow: ellipsis;
        white-space: nowrap;
      }

      .tab-modified {
        color: var(--el-color-warning);
      }

      .tab-close {
        opacity: 0;
        transition: opacity 0.2s;

        &:hover {
          color: var(--el-color-danger);
        }
      }

      &:hover .tab-close {
        opacity: 1;
      }
    }

    .new-tab-button {
      padding: 6px;
    }
  }

  .split-container {
    flex: 1;
    display: flex;
    position: relative;
    overflow: hidden;

    &.layout-single {
      .editor-pane {
        flex: 1;
      }
    }

    &.layout-horizontal-split {
      flex-direction: row;

      .editor-pane {
        flex: 0 0 auto;
      }

      .split-horizontal {
        border-left: 1px solid var(--el-border-color-light);
      }
    }

    &.layout-vertical-split {
      flex-direction: column;

      .editor-pane {
        flex: 0 0 auto;
      }

      .split-vertical {
        border-top: 1px solid var(--el-border-color-light);
      }
    }

    .editor-pane {
      position: relative;
      overflow: hidden;

      &.split-horizontal,
      &.split-vertical {
        position: relative;
      }
    }
  }

  .splitter-horizontal,
  .splitter-vertical {
    position: absolute;
    z-index: 10;
    background: var(--el-border-color);
    transition: background 0.2s;

    &:hover {
      background: var(--el-color-primary);
    }

    .splitter-handle {
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
    }
  }

  .splitter-horizontal {
    top: 0;
    bottom: 0;
    left: 50%;
    width: 4px;
    cursor: col-resize;

    .splitter-handle {
      width: 12px;
      height: 40px;
      border-radius: 2px;
      background: var(--el-border-color);
    }
  }

  .splitter-vertical {
    left: 0;
    right: 0;
    top: 50%;
    height: 4px;
    cursor: row-resize;

    .splitter-handle {
      width: 40px;
      height: 12px;
      border-radius: 2px;
      background: var(--el-border-color);
    }
  }

  .layout-controls {
    position: absolute;
    bottom: 20px;
    right: 20px;
    display: flex;
    gap: 8px;
    z-index: 20;

    .el-button {
      background: var(--el-bg-color);
      border-color: var(--el-border-color);
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
    }
  }

  .empty-pane {
    display: flex;
    align-items: center;
    justify-content: center;
    height: 100%;
    color: var(--el-text-color-secondary);
  }

  .window-selector-item {
    cursor: pointer;
    padding: 12px;
    display: flex;
    justify-content: space-between;
    align-items: center;

    &:hover {
      background: var(--el-fill-color-light);
    }

    .selector-info {
      .selector-title {
        font-weight: 500;
        margin-bottom: 4px;
      }

      .selector-path {
        font-size: 12px;
        color: var(--el-text-color-secondary);
      }
    }
  }
}
</style>
