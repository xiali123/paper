<template>
  <div class="editor-toolbar">
    <!-- 主工具栏 -->
    <div class="toolbar-main" :class="{ 'is-float': isFloating }">
      <!-- 文件操作组 -->
      <div class="toolbar-group">
        <el-tooltip content="新建文档 (Ctrl+Alt+N)">
          <el-button text @click="$emit('new')">
            <el-icon><DocumentAdd /></el-icon>
            <span class="toolbar-label">新建</span>
          </el-button>
        </el-tooltip>
        <el-tooltip content="保存 (Ctrl+S)">
          <el-button
            text
            @click="$emit('save')"
            :loading="saving"
            :disabled="!canSave"
          >
            <el-icon><DocumentChecked /></el-icon>
            <span class="toolbar-label">{{ saveText }}</span>
          </el-button>
        </el-tooltip>
        <el-tooltip content="编译 (Ctrl+Enter)">
          <el-button
            text
            @click="$emit('compile')"
            :loading="compiling"
          >
            <el-icon><VideoPlay /></el-icon>
            <span class="toolbar-label">编译</span>
          </el-button>
        </el-tooltip>
      </div>

      <el-divider direction="vertical" />

      <!-- 编辑操作组 -->
      <div class="toolbar-group">
        <el-tooltip content="撤销 (Ctrl+Z)">
          <el-button text @click="$emit('undo')" :disabled="!canUndo">
            <el-icon><RefreshLeft /></el-icon>
          </el-button>
        </el-tooltip>
        <el-tooltip content="重做 (Ctrl+Y)">
          <el-button text @click="$emit('redo')" :disabled="!canRedo">
            <el-icon><RefreshRight /></el-icon>
          </el-button>
        </el-tooltip>
        <el-tooltip content="查找替换 (Ctrl+F)">
          <el-button text @click="$emit('find')">
            <el-icon><Search /></el-icon>
          </el-button>
        </el-tooltip>
      </div>

      <el-divider direction="vertical" />

      <!-- 插入操作组 -->
      <div class="toolbar-group">
        <el-dropdown @command="handleInsertCommand" trigger="click">
          <el-button text>
            <el-icon><Plus /></el-icon>
            <span class="toolbar-label">插入</span>
            <el-icon class="dropdown-arrow"><ArrowDown /></el-icon>
          </el-button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item command="section">
                <span class="dropdown-icon">§</span>
                章节
              </el-dropdown-item>
              <el-dropdown-item command="subsection">
                <span class="dropdown-icon">§§</span>
                小节
              </el-dropdown-item>
              <el-dropdown-item command="textbf" divided>
                <span class="dropdown-icon"><strong>B</strong></span>
                粗体
              </el-dropdown-item>
              <el-dropdown-item command="textit">
                <span class="dropdown-icon"><em>I</em></span>
                斜体
              </el-dropdown-item>
              <el-dropdown-item command="image">
                <span class="dropdown-icon">🖼️</span>
                图片
              </el-dropdown-item>
              <el-dropdown-item command="table">
                <span class="dropdown-icon">▦</span>
                表格
              </el-dropdown-item>
              <el-dropdown-item command="equation">
                <span class="dropdown-icon">∑</span>
                公式
              </el-dropdown-item>
              <el-dropdown-item command="cite" divided>
                <span class="dropdown-icon">🔗</span>
                引用
              </el-dropdown-item>
              <el-dropdown-item command="symbol">
                <span class="dropdown-icon">∑</span>
                符号面板
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>

        <el-tooltip content="快速插入 (Ctrl+Alt+X)">
          <el-button text @click="$emit('quick-insert')">
            <el-icon><Lightning /></el-icon>
          </el-button>
        </el-tooltip>

        <el-tooltip content="代码片段 (Ctrl+Alt+C)">
          <el-button text @click="$emit('snippets')">
            <el-icon><Memo /></el-icon>
          </el-button>
        </el-tooltip>
      </div>

      <el-divider direction="vertical" />

      <!-- 视图操作组 -->
      <div class="toolbar-group">
        <el-tooltip content="切换预览 (Ctrl+Shift+P)">
          <el-button
            text
            @click="$emit('toggle-preview')"
            :class="{ 'is-active': showPreview }"
          >
            <el-icon><View /></el-icon>
          </el-button>
        </el-tooltip>
        <el-tooltip content="切换大纲 (Ctrl+Shift+O)">
          <el-button
            text
            @click="$emit('toggle-outline')"
            :class="{ 'is-active': showOutline }"
          >
            <el-icon><List /></el-icon>
          </el-button>
        </el-tooltip>
        <el-tooltip :content="isFullscreen ? '退出全屏' : '全屏'">
          <el-button
            text
            @click="$emit('toggle-fullscreen')"
          >
            <el-icon><FullScreen v-if="!isFullscreen" /><Close v-else /></el-icon>
          </el-button>
        </el-tooltip>
      </div>

      <el-divider direction="vertical" />

      <!-- 更多操作 -->
      <div class="toolbar-group">
        <el-dropdown @command="handleMoreCommand" trigger="click">
          <el-button text>
            <el-icon><MoreFilled /></el-icon>
          </el-button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item command="export">
                <el-icon><Download /></el-icon>
                导出 PDF
              </el-dropdown-item>
              <el-dropdown-item command="settings">
                <el-icon><Setting /></el-icon>
                设置
              </el-dropdown-item>
              <el-dropdown-item command="shortcut" divided>
                <el-icon><QuestionFilled /></el-icon>
                快捷键
              </el-dropdown-item>
              <el-dropdown-item command="help">
                <el-icon><ChatDotSquare /></el-icon>
                帮助
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>

      <!-- 缩放控制 -->
      <div class="toolbar-group toolbar-zoom" v-if="showZoom">
        <el-tooltip content="缩小 (Ctrl+-)">
          <el-button text @click="$emit('zoom-out')">
            <el-icon><ZoomOut /></el-icon>
          </el-button>
        </el-tooltip>
        <span class="zoom-label">{{ Math.round(zoom * 100) }}%</span>
        <el-tooltip content="放大 (Ctrl++)">
          <el-button text @click="$emit('zoom-in')">
            <el-icon><ZoomIn /></el-icon>
          </el-button>
        </el-tooltip>
      </div>
    </div>

    <!-- 浮动模式切换按钮 -->
    <transition name="slide">
      <el-button
        v-if="!isFloating"
        class="float-toggle"
        :icon="Sort"
        circle
        size="small"
        @click="toggleFloatingMode"
      />
    </transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import {
  DocumentAdd,
  DocumentChecked,
  VideoPlay,
  RefreshLeft,
  RefreshRight,
  Search,
  Plus,
  ArrowDown,
  Lightning,
  Memo,
  View,
  List,
  FullScreen,
  Close,
  MoreFilled,
  Download,
  Setting,
  QuestionFilled,
  ChatDotSquare,
  ZoomIn,
  ZoomOut,
  Sort
} from '@element-plus/icons-vue'

interface Props {
  saving?: boolean
  compiling?: boolean
  canSave?: boolean
  canUndo?: boolean
  canRedo?: boolean
  showPreview?: boolean
  showOutline?: boolean
  isFullscreen?: boolean
  zoom?: number
  showZoom?: boolean
  isFloating?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  saving: false,
  compiling: false,
  canSave: true,
  canUndo: false,
  canRedo: false,
  showPreview: true,
  showOutline: false,
  isFullscreen: false,
  zoom: 1.0,
  showZoom: false,
  isFloating: false
})

const emit = defineEmits<{
  'new': []
  'save': []
  'compile': []
  'undo': []
  'redo': []
  'find': []
  'quick-insert': []
  'snippets': []
  'toggle-preview': []
  'toggle-outline': []
  'toggle-fullscreen': []
  'zoom-in': []
  'zoom-out': []
  'insert': [command: string]
  'more': [command: string]
}>()

const saveText = computed(() => {
  return props.saving ? '保存中...' : '保存'
})

function handleInsertCommand(command: string) {
  emit('insert', command)
}

function handleMoreCommand(command: string) {
  emit('more', command)
}

const isFloating = ref(props.isFloating)

function toggleFloatingMode() {
  isFloating.value = !isFloating.value
}
</script>

<style scoped lang="scss">
.editor-toolbar {
  position: relative;
  height: auto;
}

.toolbar-main {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 8px 12px;
  min-height: 44px;
  transition: all 0.3s;

  &.is-float {
    position: fixed;
    top: 100px;
    left: 50%;
    transform: translateX(-50%);
    background: var(--el-bg-color);
    border: 1px solid var(--el-border-color);
    border-radius: 8px;
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.15);
    padding: 8px 16px;
    z-index: 1000;
  }
}

.toolbar-group {
  display: flex;
  align-items: center;
  gap: 2px;
  padding: 0 4px;

  .toolbar-label {
    font-size: 12px;
    margin-left: 4px;
  }

  .is-active {
    color: var(--el-color-primary);
  }
}

.toolbar-zoom {
  margin-left: auto;
  padding-left: 12px;
  border-left: 1px solid var(--el-border-color);

  .zoom-label {
    font-size: 12px;
    min-width: 40px;
    text-align: center;
    color: var(--el-text-color-secondary);
  }
}

.dropdown-icon {
  font-size: 14px;
  margin-right: 6px;
}

.dropdown-arrow {
  font-size: 12px;
  margin-left: 2px;
  color: var(--el-text-color-secondary);
}

.float-toggle {
  position: absolute;
  right: 8px;
  top: 50%;
  transform: translateY(-50%);
}

// 下拉菜单样式
:deep(.el-dropdown-menu__item) {
  padding: 6px 12px;
  min-width: 160px;

  .dropdown-icon {
    display: inline-block;
    width: 20px;
    text-align: center;
    font-style: normal;
  }
}

// 动画
.slide-enter-active,
.slide-leave-active {
  transition: all 0.3s ease;
}

.slide-enter-from,
.slide-leave-to {
  transform: translateX(100%);
  opacity: 0;
}

// 响应式
@media (max-width: 768px) {
  .toolbar-label {
    display: none;
  }

  .toolbar-main {
    flex-wrap: wrap;
  }
}
</style>
