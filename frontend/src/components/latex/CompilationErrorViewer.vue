<template>
  <div class="compilation-error-viewer" v-if="hasErrors">
    <!-- 错误摘要 -->
    <div class="error-summary" @click="showDetail = !showDetail">
      <div class="summary-header">
        <el-icon class="error-icon"><Warning /></el-icon>
        <span class="summary-text">
          {{ totalErrors }} 个错误，{{ totalWarnings }} 个警告
        </span>
        <el-icon class="toggle-icon" :class="{ expanded: showDetail }">
          <ArrowDown />
        </el-icon>
      </div>

      <!-- 主要错误预览 -->
      <div class="main-error" v-if="mainError">
        <span class="error-line">行 {{ mainError.line }}</span>
        <span class="error-message">{{ mainError.message }}</span>
      </div>
    </div>

    <!-- 详细错误列表 -->
    <transition name="expand">
      <div v-if="showDetail" class="error-detail">
        <!-- 过滤器 -->
        <div class="error-filters">
          <el-radio-group v-model="filterType" size="small">
            <el-radio-button value="all">全部</el-radio-button>
            <el-radio-button value="error">错误</el-radio-button>
            <el-radio-button value="warning">警告</el-radio-button>
          </el-radio-group>

          <el-input
            v-model="searchQuery"
            placeholder="搜索错误..."
            prefix-icon="Search"
            size="small"
            clearable
            style="width: 200px; margin-left: 12px;"
          />
        </div>

        <!-- 错误列表 -->
        <div class="error-list">
          <div
            v-for="error in filteredErrors"
            :key="error.id"
            class="error-item"
            :class="error.type"
            @click="jumpToError(error)"
          >
            <div class="error-header">
              <el-icon class="error-type-icon">
                <Warning v-if="error.type === 'error'" />
                <InfoFilled v-else />
              </el-icon>
              <span class="error-line-num">行 {{ error.line }}</span>
              <el-tag :type="error.type === 'error' ? 'danger' : 'warning'" size="small">
                {{ error.type === 'error' ? '错误' : '警告' }}
              </el-tag>
            </div>

            <div class="error-content">
              <div class="error-message">{{ error.message }}</div>

              <!-- 错误上下文 -->
              <div v-if="error.context" class="error-context">
                <pre>{{ error.context }}</pre>
              </div>

              <!-- 建议修复 -->
              <div v-if="error.suggestion" class="error-suggestion">
                <el-icon><Lightning /></el-icon>
                <span>建议：{{ error.suggestion }}</span>
              </div>

              <!-- 错误代码 -->
              <div v-if="error.code" class="error-code">
                错误代码：{{ error.code }}
              </div>
            </div>
          </div>

          <el-empty
            v-if="filteredErrors.length === 0"
            description="没有匹配的错误"
            :image-size="60"
          />
        </div>

        <!-- 错误操作 -->
        <div class="error-actions">
          <el-button size="small" @click="copyErrors">
            <el-icon><DocumentCopy /></el-icon>
            复制错误列表
          </el-button>
          <el-button size="small" @click="exportErrors">
            <el-icon><Download /></el-icon>
            导出错误报告
          </el-button>
          <el-button
            size="small"
            type="primary"
            @click="recompile"
            :loading="recompiling"
          >
            <el-icon><RefreshRight /></el-icon>
            重新编译
          </el-button>
        </div>
      </div>
    </transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import {
  Warning,
  InfoFilled,
  ArrowDown,
  Search,
  Lightning,
  DocumentCopy,
  Download,
  RefreshRight
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface CompilationError {
  id: string
  line: number
  column?: number
  type: 'error' | 'warning'
  message: string
  code?: string
  context?: string
  suggestion?: string
  file?: string
}

interface Props {
  errors: CompilationError[]
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'jump-to': [error: CompilationError]
  'recompile': []
}>()

// State
const showDetail = ref(false)
const filterType = ref<'all' | 'error' | 'warning'>('all')
const searchQuery = ref('')
const recompiling = ref(false)

// 计算属性
const hasErrors = computed(() => props.errors.length > 0)

const totalErrors = computed(() =>
  props.errors.filter(e => e.type === 'error').length
)

const totalWarnings = computed(() =>
  props.errors.filter(e => e.type === 'warning').length
)

const mainError = computed(() => {
  const errors = props.errors.filter(e => e.type === 'error')
  return errors[0] || null
})

const filteredErrors = computed(() => {
  let filtered = props.errors

  // 类型过滤
  if (filterType.value !== 'all') {
    filtered = filtered.filter(e => e.type === filterType.value)
  }

  // 搜索过滤
  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    filtered = filtered.filter(e =>
      e.message.toLowerCase().includes(query) ||
      e.code?.toLowerCase().includes(query) ||
      e.line.toString().includes(query)
    )
  }

  return filtered
})

// 方法
function jumpToError(error: CompilationError) {
  emit('jump-to', error)
  // 可以在这里添加滚动到编辑器对应行的逻辑
}

async function copyErrors() {
  const text = filteredErrors.value.map(err =>
    `[${err.type.toUpperCase()}] 行 ${err.line}: ${err.message}${err.code ? ` (${err.code})` : ''}`
  ).join('\n')

  try {
    await navigator.clipboard.writeText(text)
    ElMessage.success('错误列表已复制到剪贴板')
  } catch (err) {
    ElMessage.error('复制失败')
  }
}

async function exportErrors() {
  const report = {
    timestamp: new Date().toISOString(),
    summary: {
      totalErrors: totalErrors.value,
      totalWarnings: totalWarnings.value,
      total: props.errors.length
    },
    errors: filteredErrors.value
  }

  const blob = new Blob([JSON.stringify(report, null, 2)], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `latex-errors-${Date.now()}.json`
  document.body.appendChild(a)
  a.click()
  document.body.removeChild(a)
  URL.revokeObjectURL(url)

  ElMessage.success('错误报告已导出')
}

async function recompile() {
  recompiling.value = true
  try {
    emit('recompile')
  } finally {
    setTimeout(() => {
      recompiling.value = false
    }, 1000)
  }
}
</script>

<style scoped lang="scss">
.compilation-error-viewer {
  border: 1px solid var(--el-color-danger);
  border-radius: 8px;
  overflow: hidden;
  background: var(--el-color-danger-light-9);
}

.error-summary {
  padding: 12px 16px;
  cursor: pointer;
  user-select: none;
  transition: background 0.2s;

  &:hover {
    background: var(--el-color-danger-light-8);
  }
}

.summary-header {
  display: flex;
  align-items: center;
  gap: 8px;
}

.error-icon {
  font-size: 20px;
  color: var(--el-color-danger);
}

.summary-text {
  flex: 1;
  font-weight: 500;
  color: var(--el-text-color-primary);
}

.toggle-icon {
  font-size: 16px;
  color: var(--el-text-color-secondary);
  transition: transform 0.3s;

  &.expanded {
    transform: rotate(180deg);
  }
}

.main-error {
  margin-top: 8px;
  padding: 8px 12px;
  background: var(--el-fill-color);
  border-radius: 4px;
  font-size: 13px;
}

.error-line {
  font-family: 'Consolas', 'Monaco', monospace;
  font-weight: 600;
  color: var(--el-color-danger);
  margin-right: 12px;
}

.error-message {
  color: var(--el-text-color-regular);
}

// 展开动画
.expand-enter-active,
.expand-leave-active {
  transition: all 0.3s ease;
  overflow: hidden;
}

.expand-enter-from,
.expand-leave-to {
  max-height: 0;
  opacity: 0;
}

.expand-enter-to,
.expand-leave-from {
  max-height: 800px;
  opacity: 1;
}

.error-detail {
  border-top: 1px solid var(--el-border-color);
  background: var(--el-bg-color);
}

.error-filters {
  display: flex;
  align-items: center;
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color);
}

.error-list {
  max-height: 400px;
  overflow-y: auto;
  padding: 8px;
}

.error-item {
  margin-bottom: 8px;
  padding: 12px;
  background: var(--el-fill-color-light);
  border-radius: 6px;
  border-left: 3px solid transparent;
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    background: var(--el-fill-color);
  }

  &.error {
    border-left-color: var(--el-color-danger);
  }

  &.warning {
    border-left-color: var(--el-color-warning);
  }
}

.error-header {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
}

.error-type-icon {
  font-size: 16px;

  .error & {
    color: var(--el-color-danger);
  }

  .warning & {
    color: var(--el-color-warning);
  }
}

.error-line-num {
  font-family: 'Consolas', 'Monaco', monospace;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.error-content {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.error-message {
  color: var(--el-text-color-regular);
  line-height: 1.5;
}

.error-context {
  padding: 8px;
  background: var(--el-bg-color);
  border-radius: 4px;
  font-family: 'Consolas', 'Monaco', monospace;
  font-size: 12px;
  color: var(--el-text-color-secondary);
  overflow-x: auto;

  pre {
    margin: 0;
    white-space: pre-wrap;
  }
}

.error-suggestion {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px;
  background: var(--el-color-info-light-9);
  border-radius: 4px;
  font-size: 13px;
  color: var(--el-color-info);
}

.error-code {
  font-family: 'Consolas', 'Monaco', monospace;
  font-size: 11px;
  color: var(--el-text-color-secondary);
}

.error-actions {
  display: flex;
  gap: 8px;
  padding: 12px 16px;
  border-top: 1px solid var(--el-border-color);
  background: var(--el-fill-color-light);
}

// 自定义滚动条
.error-list::-webkit-scrollbar {
  width: 6px;
}

.error-list::-webkit-scrollbar-track {
  background: var(--el-fill-color);
}

.error-list::-webkit-scrollbar-thumb {
  background: var(--el-border-color);
  border-radius: 3px;

  &:hover {
    background: var(--el-border-color-darker);
  }
}
</style>
