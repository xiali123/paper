<template>
  <div class="editor-toolbar">
    <!-- Left Panel Toggle -->
    <el-button-group>
      <el-button size="small" @click="$emit('toggle-left-panel')" :class="{ 'is-active': showOutline || showProjectTree }">
        <el-icon><Menu /></el-icon>
        {{ leftPanelTitle }}
      </el-button>

      <!-- Undo/Redo -->
      <el-tooltip content="撤销 (Ctrl+Z)" placement="top">
        <el-button size="small" @click="$emit('undo')" :disabled="!canUndo">
          <el-icon><RefreshLeft /></el-icon>
        </el-button>
      </el-tooltip>
      <el-tooltip content="重做 (Ctrl+Shift+Z)" placement="top">
        <el-button size="small" @click="$emit('redo')" :disabled="!canRedo">
          <el-icon><RefreshRight /></el-icon>
        </el-button>
      </el-tooltip>

      <el-divider direction="vertical" />

      <!-- Formatting Buttons -->
      <el-button size="small" @click="$emit('insert-command', 'textbf')">
        <b>B</b>
      </el-button>
      <el-button size="small" @click="$emit('insert-command', 'textit')">
        <i>I</i>
      </el-button>

      <!-- Environment Dropdown -->
      <el-dropdown size="small" @command="cmd => $emit('insert-environment', cmd)">
        <el-button size="small">
          <el-icon><Plus /></el-icon>
          环境
        </el-button>
        <template #dropdown>
          <el-dropdown-menu>
            <el-dropdown-item command="itemize">无序列表</el-dropdown-item>
            <el-dropdown-item command="enumerate">有序列表</el-dropdown-item>
            <el-dropdown-item command="equation">数学公式</el-dropdown-item>
            <el-dropdown-item command="figure">图片环境</el-dropdown-item>
            <el-dropdown-item command="table">表格环境</el-dropdown-item>
          </el-dropdown-menu>
        </template>
      </el-dropdown>

      <!-- Tool Buttons -->
      <el-tooltip content="代码片段 (Ctrl+Space)" placement="top">
        <el-button size="small" @click="$emit('toggle-panel', 'snippets')">
          <el-icon><Collection /></el-icon>
          片段
        </el-button>
      </el-tooltip>

      <el-tooltip content="快捷键 (?) " placement="top">
        <el-button size="small" @click="$emit('show-keyboard-shortcuts')">
          <el-icon><QuestionFilled /></el-icon>
        </el-button>
      </el-tooltip>

      <el-divider direction="vertical" />

      <!-- Find Button -->
      <el-tooltip content="查找替换 (Ctrl+F)" placement="top">
        <el-button size="small" @click="$emit('toggle-find-replace')">
          <el-icon><Search /></el-icon>
          查找
        </el-button>
      </el-tooltip>
    </el-button-group>

    <!-- Compilation Status -->
    <div class="compilation-status" v-if="compilationStatus !== 'idle'">
      <el-tag :type="compilationStatusType" size="small">
        <el-icon v-if="compilationStatus === 'compiling'"><Loading /></el-icon>
        {{ compilationStatusText }}
      </el-tag>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import {
  Menu,
  RefreshLeft,
  RefreshRight,
  Plus,
  Collection,
  QuestionFilled,
  Search,
  Loading
} from '@element-plus/icons-vue'

interface Props {
  showOutline?: boolean
  showProjectTree?: boolean
  leftPanelTitle?: string
  canUndo?: boolean
  canRedo?: boolean
  compilationStatus?: 'idle' | 'compiling' | 'success' | 'error' | 'warning'
  compilationStatusText?: string
}

const props = withDefaults(defineProps<Props>(), {
  showOutline: false,
  showProjectTree: false,
  leftPanelTitle: '大纲',
  canUndo: false,
  canRedo: false,
  compilationStatus: 'idle',
  compilationStatusText: ''
})

const emit = defineEmits<{
  'toggle-left-panel': []
  'undo': []
  'redo': []
  'insert-command': [command: string]
  'insert-environment': [env: string]
  'toggle-panel': [panel: string]
  'show-keyboard-shortcuts': []
  'toggle-find-replace': []
}>()

// emit is defined but not used directly - events are emitted via $emit in template
void emit

const compilationStatusType = computed(() => {
  switch (props.compilationStatus) {
    case 'compiling': return 'primary'
    case 'success': return 'success'
    case 'error': return 'danger'
    default: return 'info'
  }
})
</script>

<style scoped lang="scss">
.editor-toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 8px 12px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color);
  gap: 8px;
}

.compilation-status {
  display: flex;
  align-items: center;
  gap: 4px;
}

.el-button.is-active {
  color: var(--el-color-primary);
}

.el-divider--vertical {
  height: 20px;
  margin: 0 4px;
}
</style>
