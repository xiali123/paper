<template>
  <div class="editor-status-bar">
    <!-- 左侧：状态信息 -->
    <div class="status-left">
      <div class="status-item" v-if="saveStatus">
        <el-icon class="status-icon" :class="saveStatus.class">
          <component :is="saveStatus.icon" />
        </el-icon>
        <span class="status-text">{{ saveStatus.text }}</span>
      </div>

      <el-divider direction="vertical" />

      <div class="status-item">
        <span class="cursor-position">行 {{ cursorLine }}, 列 {{ cursorColumn }}</span>
      </div>

      <div class="status-item" v-if="selection">
        <span>已选择 {{ selection.length }} 个字符</span>
      </div>

      <el-divider direction="vertical" />

      <div class="status-item encoding">
        <span>UTF-8</span>
      </div>

      <div class="status-item">
        <span>LaTeX</span>
      </div>
    </div>

    <!-- 中间：编译状态 -->
    <div class="status-center" v-if="compileStatus">
      <el-tag
        :type="compileStatus.type"
        :icon="compileStatus.icon"
        size="small"
        effect="plain"
      >
        {{ compileStatus.text }}
      </el-tag>
    </div>

    <!-- 右侧：操作按钮 -->
    <div class="status-right">
      <el-tooltip content="快捷键 (F1)">
        <el-button text size="small" @click="$emit('show-shortcuts')">
          <el-icon><QuestionFilled /></el-icon>
        </el-button>
      </el-tooltip>

      <el-tooltip content="快速插入 (Ctrl+Alt+X)">
        <el-button text size="small" @click="$emit('show-quick-insert')">
          <el-icon><Plus /></el-icon>
        </el-button>
      </el-tooltip>

      <el-tooltip content="AI 公式识别">
        <el-button text size="small" @click="$emit('show-ai-recognize')">
          <el-icon><MagicStick /></el-icon>
        </el-button>
      </el-tooltip>

      <el-divider direction="vertical" />

      <el-dropdown @command="handleCommand" trigger="click">
        <el-button text size="small">
          <el-icon><Setting /></el-icon>
        </el-button>
        <template #dropdown>
          <el-dropdown-menu>
            <el-dropdown-item command="theme">
              <span>🎨 主题</span>
            </el-dropdown-item>
            <el-dropdown-item command="font">
              <span>🔤 字体</span>
            </el-dropdown-item>
            <el-dropdown-item command="settings" divided>
              <span>⚙️ 设置</span>
            </el-dropdown-item>
          </el-dropdown-menu>
        </template>
      </el-dropdown>
    </div>

    <!-- 进度条（保存/编译时显示） -->
    <div class="status-progress" v-if="showProgress">
      <el-progress
        :percentage="progressPercent"
        :status="progressStatus"
        :stroke-width="2"
        :show-text="false"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'
import {
  QuestionFilled,
  Plus,
  MagicStick,
  Setting,
  Loading,
  CircleCheckFilled,
  CircleCloseFilled
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface Selection {
  start: { line: number; column: number }
  end: { line: number; column: number }
  length: number
}

interface Props {
  saving?: boolean
  saved?: boolean
  compiling?: boolean
  compileSuccess?: boolean
  compileError?: boolean
  cursorLine?: number
  cursorColumn?: number
  selection?: Selection | null
}

const props = withDefaults(defineProps<Props>(), {
  cursorLine: 1,
  cursorColumn: 1,
  selection: null
})

defineEmits<{
  'show-shortcuts': []
  'show-quick-insert': []
  'show-ai-recognize': []
}>()

const showProgress = ref(false)
const progressPercent = ref(0)
const progressStatus = ref<'success' | 'exception' | 'warning' | undefined>()

// 保存状态
const saveStatus = computed(() => {
  if (props.saving) {
    return { icon: Loading, text: '保存中...', class: 'saving' }
  }
  if (props.saved) {
    return { icon: CircleCheckFilled, text: '已保存', class: 'saved' }
  }
  return { icon: CircleCloseFilled, text: '未保存', class: 'unsaved' }
})

// 编译状态
const compileStatus = computed(() => {
  if (props.compiling) {
    return { icon: Loading, text: '编译中...', type: 'warning' as const }
  }
  if (props.compileSuccess) {
    return { icon: CircleCheckFilled, text: '编译成功', type: 'success' as const }
  }
  if (props.compileError) {
    return { icon: CircleCloseFilled, text: '编译失败', type: 'danger' as const }
  }
  return null
})

// 处理命令
const handleCommand = (command: string) => {
  switch (command) {
    case 'theme':
      ElMessage.info('主题设置')
      break
    case 'font':
      ElMessage.info('字体设置')
      break
    case 'settings':
      ElMessage.info('编辑器设置')
      break
  }
}

// 模拟进度条
const simulateProgress = () => {
  showProgress.value = true
  progressPercent.value = 0
  progressStatus.value = undefined

  const interval = setInterval(() => {
    progressPercent.value += 10
    if (progressPercent.value >= 100) {
      clearInterval(interval)
      progressStatus.value = 'success'
      setTimeout(() => {
        showProgress.value = false
        progressPercent.value = 0
      }, 1000)
    }
  }, 200)
}

defineExpose({
  simulateProgress
})
</script>

<style scoped lang="scss">
.editor-status-bar {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 4px 16px;
  background: var(--el-fill-color-light);
  border-top: 1px solid var(--el-border-color-light);
  font-size: 12px;
  color: var(--el-text-color-secondary);
  min-height: 32px;
}

.status-left,
.status-center,
.status-right {
  display: flex;
  align-items: center;
  gap: 8px;
}

.status-left {
  flex: 1;
}

.status-item {
  display: flex;
  align-items: center;
  gap: 4px;
  white-space: nowrap;

  &.encoding,
  &.cursor-position {
    font-family: 'Consolas', 'Monaco', monospace;
    font-size: 11px;
  }
}

.status-icon {
  font-size: 14px;

  &.saving {
    color: var(--el-color-warning);
    animation: rotate 1s linear infinite;
  }

  &.saved {
    color: var(--el-color-success);
  }

  &.unsaved {
    color: var(--el-color-info);
  }
}

@keyframes rotate {
  from {
    transform: rotate(0deg);
  }
  to {
    transform: rotate(360deg);
  }
}

.status-text {
  color: var(--el-text-color-regular);
}

.status-progress {
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
  height: 2px;
  background: var(--el-fill-color);

  :deep(.el-progress) {
    .el-progress-bar__outer {
      border-radius: 0;
    }
  }
}

:deep(.el-divider--vertical) {
  height: 16px;
  margin: 0;
}

// Dark mode overrides
[data-theme="dark"] {
  .editor-status-bar {
    background: rgba(40, 40, 45, 0.98);
    border-top-color: rgba(102, 126, 234, 0.3);
    color: #9ca3af;
  }

  .status-text {
    color: #f3f4f6;
  }

  .status-progress {
    background: rgba(30, 30, 35, 0.95);
  }
}
</style>
