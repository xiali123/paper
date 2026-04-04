<template>
  <div class="real-time-log-viewer">
    <div class="log-header">
      <div class="log-title">
        <el-icon><Document /></el-icon>
        <span>实时日志</span>
      </div>
      <div class="log-controls">
        <el-select
          v-model="logLevel"
          size="small"
          placeholder="日志级别"
          @change="handleLevelChange"
        >
          <el-option label="全部" value="" />
          <el-option label="信息" value="info" />
          <el-option label="警告" value="warning" />
          <el-option label="错误" value="error" />
          <el-option label="调试" value="debug" />
        </el-select>
        <el-checkbox
          v-model="autoScroll"
          size="small"
        >
          自动滚动
        </el-checkbox>
        <el-button
          size="small"
          :icon="Delete"
          @click="handleClear"
        >
          清空
        </el-button>
        <el-button
          size="small"
          :icon="Download"
          @click="handleExport"
        >
          导出
        </el-button>
      </div>
    </div>

    <div
      ref="logContainerRef"
      class="log-container"
      @scroll="handleScroll"
    >
      <div
        v-for="(log, index) in filteredLogs"
        :key="index"
        class="log-entry"
        :class="`log-${log.level}`"
      >
        <span class="log-time">{{ formatTime(log.timestamp) }}</span>
        <span class="log-level">{{ log.level.toUpperCase() }}</span>
        <span class="log-source" v-if="log.source">[{{ log.source }}]</span>
        <span class="log-message">{{ log.message }}</span>
      </div>

      <div v-if="filteredLogs.length === 0" class="log-empty">
        <el-empty description="暂无日志" :image-size="80" />
      </div>
    </div>

    <div class="log-footer">
      <span class="log-count">共 {{ filteredLogs.length }} 条日志</span>
      <span v-if="isLive" class="live-indicator">
        <span class="dot"></span>
        实时更新
      </span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, nextTick } from 'vue'
import { ElMessage } from 'element-plus'
import {
  Document,
  Delete,
  Download
} from '@element-plus/icons-vue'

export interface LogEntry {
  timestamp: number
  level: 'info' | 'warning' | 'error' | 'debug'
  message: string
  source?: string
}

const props = defineProps<{
  logs: LogEntry[]
  isLive?: boolean
}>()

const emit = defineEmits<{
  'clear': []
  'export': []
  'level-change': [level: string]
}>()

const logLevel = ref<string>('')
const autoScroll = ref(true)
const logContainerRef = ref<HTMLElement | null>(null)

const filteredLogs = computed(() => {
  if (!logLevel.value) {
    return props.logs
  }
  return props.logs.filter(log => log.level === logLevel.value)
})

const formatTime = (timestamp: number) => {
  const date = new Date(timestamp)
  return date.toLocaleTimeString('zh-CN', {
    hour12: false,
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit'
  })
}

const handleLevelChange = () => {
  emit('level-change', logLevel.value)
}

const handleClear = () => {
  emit('clear')
}

const handleExport = () => {
  const data = filteredLogs.value.map(log => ({
    time: new Date(log.timestamp).toISOString(),
    level: log.level,
    source: log.source || '',
    message: log.message
  }))

  const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `logs-${Date.now()}.json`
  a.click()
  URL.revokeObjectURL(url)

  ElMessage.success('日志导出成功')
  emit('export')
}

const handleScroll = () => {
  if (!logContainerRef.value) return

  const { scrollTop, scrollHeight, clientHeight } = logContainerRef.value
  const isAtBottom = scrollTop + clientHeight >= scrollHeight - 10

  autoScroll.value = isAtBottom
}

const scrollToBottom = () => {
  if (autoScroll.value && logContainerRef.value) {
    nextTick(() => {
      if (logContainerRef.value) {
        logContainerRef.value.scrollTop = logContainerRef.value.scrollHeight
      }
    })
  }
}

// Auto-scroll when new logs arrive
watch(
  () => props.logs.length,
  () => {
    scrollToBottom()
  }
)

// Initial scroll
watch(
  logContainerRef,
  () => {
    scrollToBottom()
  }
)

defineExpose({
  scrollToBottom
})
</script>

<style scoped lang="scss">
.real-time-log-viewer {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #1e1e1e;
  border-radius: 8px;
  overflow: hidden;

  .log-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 16px;
    background: #2d2d2d;
    border-bottom: 1px solid #404040;

    .log-title {
      display: flex;
      align-items: center;
      gap: 8px;
      color: #e0e0e0;
      font-weight: bold;
    }

    .log-controls {
      display: flex;
      gap: 8px;
      align-items: center;

      :deep(.el-select) {
        width: 120px;
      }

      :deep(.el-checkbox__label) {
        color: #e0e0e0;
      }
    }
  }

  .log-container {
    flex: 1;
    overflow-y: auto;
    padding: 12px 16px;
    font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
    font-size: 13px;
    line-height: 1.6;
    background: #1e1e1e;

    .log-entry {
      display: flex;
      gap: 12px;
      padding: 4px 0;
      color: #e0e0e0;
      white-space: pre-wrap;
      word-break: break-all;

      &:hover {
        background: #2d2d2d;
      }

      .log-time {
        color: #858585;
        min-width: 70px;
      }

      .log-level {
        font-weight: bold;
        min-width: 60px;
      }

      .log-source {
        color: #569cd6;
        min-width: 80px;
      }

      .log-message {
        flex: 1;
      }

      &.log-info .log-level {
        color: #4ec9b0;
      }

      &.log-warning .log-level {
        color: #dcdcaa;
      }

      &.log-error .log-level {
        color: #f44747;
      }

      &.log-debug .log-level {
        color: #9cdcfe;
      }
    }

    .log-empty {
      display: flex;
      align-items: center;
      justify-content: center;
      height: 100%;
      color: #858585;
    }
  }

  .log-footer {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 8px 16px;
    background: #2d2d2d;
    border-top: 1px solid #404040;
    font-size: 12px;
    color: #858585;

    .live-indicator {
      display: flex;
      align-items: center;
      gap: 6px;
      color: #4ec9b0;

      .dot {
        width: 8px;
        height: 8px;
        background: #4ec9b0;
        border-radius: 50%;
        animation: pulse 1.5s infinite;
      }
    }
  }
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.5;
  }
}

// Scrollbar styling
.log-container {
  &::-webkit-scrollbar {
    width: 8px;
  }

  &::-webkit-scrollbar-track {
    background: #2d2d2d;
  }

  &::-webkit-scrollbar-thumb {
    background: #555;
    border-radius: 4px;

    &:hover {
      background: #666;
    }
  }
}
</style>
