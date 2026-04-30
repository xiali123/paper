<template>
  <div class="cursor-tracker">
    <!-- 渲染其他用户的光标 -->
    <div
      v-for="cursor in remoteCursors"
      :key="cursor.userId"
      class="remote-cursor"
      :style="getCursorStyle(cursor)"
    >
      <div class="cursor-flag" :style="{ backgroundColor: cursor.color }">
        <span class="cursor-name">{{ cursor.userName }}</span>
      </div>
      <div class="cursor-line" :style="{ backgroundColor: cursor.color }"></div>
    </div>

    <!-- 渲染选择区域 -->
    <div
      v-for="selection in remoteSelections"
      :key="`${selection.userId}-selection`"
      class="remote-selection"
      :style="getSelectionStyle(selection)"
    ></div>

    <!-- 渲染编辑锁 -->
    <div
      v-for="lock in editLocks"
      :key="`lock-${lock.line}`"
      class="edit-lock"
      :style="{ top: `${lock.line * 20}px` }"
      :title="`Locked by ${lock.userName}`"
    >
      <el-icon><Lock /></el-icon>
    </div>

    <!-- 冲突警告 -->
    <el-alert
      v-if="hasConflict"
      type="warning"
      :closable="false"
      show-icon
      class="conflict-alert"
    >
      <template #title>
        检测到编辑冲突
      </template>
      <div class="conflict-details">
        <p>与 {{ conflictUsers.join(', ') }} 同时编辑相同区域</p>
        <el-button size="small" @click="resolveConflict">解决冲突</el-button>
      </div>
    </el-alert>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue'
import { Lock } from '@element-plus/icons-vue'
import { useWebSocket } from '@/composables/useWebSocket'

interface RemoteCursor {
  userId: string
  userName: string
  color: string
  line: number
  column: number
  timestamp: number
}

interface RemoteSelection {
  userId: string
  userName: string
  color: string
  startLine: number
  endLine: number
  startColumn: number
  endColumn: number
}

interface EditLock {
  line: number
  userId: string
  userName: string
  timestamp: number
}

interface Props {
  documentId: string
  currentUserId: string
  editorLines: number
}

const props = defineProps<Props>()

const emit = defineEmits<{
  conflict: [users: string[]]
  resolve: []
}>()

const remoteCursors = ref<RemoteCursor[]>([])
const remoteSelections = ref<RemoteSelection[]>([])
const editLocks = ref<EditLock[]>([])
const cursorUpdateThrottle = ref<number | null>(null)

// WebSocket连接
const ws = useWebSocket(
  {
    url: `${import.meta.env.VITE_WS_URL || 'ws://localhost:8087/ws'}/collaborate/${props.documentId}`,
    reconnectInterval: 2000,
    maxReconnectAttempts: 10
  },
  {
    onMessage: (message) => {
      handleWebSocketMessage(message)
    }
  }
)

// 检测冲突
const hasConflict = computed(() => {
  const myCursorLine = getCurrentCursorLine()
  return remoteCursors.value.some(cursor => {
    const distance = Math.abs(cursor.line - myCursorLine)
    return distance <= 3 // 3行内视为冲突
  })
})

const conflictUsers = computed(() => {
  const myCursorLine = getCurrentCursorLine()
  const conflicting = remoteCursors.value.filter(cursor => {
    const distance = Math.abs(cursor.line - myCursorLine)
    return distance <= 3
  })
  return [...new Set(conflicting.map(c => c.userName))]
})

// 获取当前光标行（从Monaco编辑器获取）
function getCurrentCursorLine(): number {
  const editor = (window as any).monacoEditor
  if (editor) {
    const position = editor.getPosition()
    return position ? position.lineNumber - 1 : 0
  }
  return 0
}

// 处理WebSocket消息
function handleWebSocketMessage(message: any) {
  switch (message.type) {
    case 'cursor_update':
      handleCursorUpdate(message.data)
      break
    case 'selection_update':
      handleSelectionUpdate(message.data)
      break
    case 'edit_lock':
      handleEditLock(message.data)
      break
    case 'edit_unlock':
      handleEditUnlock(message.data)
      break
  }
}

// 处理光标更新
function handleCursorUpdate(data: any) {
  const existingIndex = remoteCursors.value.findIndex(c => c.userId === data.userId)

  if (data.userId === props.currentUserId) {
    return // 忽略自己的光标
  }

  const cursor: RemoteCursor = {
    userId: data.userId,
    userName: data.userName,
    color: stringToColor(data.userId),
    line: data.line,
    column: data.column || 0,
    timestamp: Date.now()
  }

  if (existingIndex >= 0) {
    remoteCursors.value[existingIndex] = cursor
  } else {
    remoteCursors.value.push(cursor)
  }

  // 5秒后移除不活跃的光标
  setTimeout(() => {
    const index = remoteCursors.value.findIndex(c => c.userId === data.userId)
    if (index >= 0 && remoteCursors.value[index].timestamp === cursor.timestamp) {
      remoteCursors.value.splice(index, 1)
    }
  }, 5000)
}

// 处理选择更新
function handleSelectionUpdate(data: any) {
  if (data.userId === props.currentUserId) return

  const existingIndex = remoteSelections.value.findIndex(s => s.userId === data.userId)

  const selection: RemoteSelection = {
    userId: data.userId,
    userName: data.userName,
    color: stringToColor(data.userId),
    startLine: data.startLine,
    endLine: data.endLine,
    startColumn: data.startColumn || 0,
    endColumn: data.endColumn || 0
  }

  if (existingIndex >= 0) {
    if (data.startLine === data.endLine) {
      remoteSelections.value.splice(existingIndex, 1)
    } else {
      remoteSelections.value[existingIndex] = selection
    }
  } else if (data.startLine !== data.endLine) {
    remoteSelections.value.push(selection)
  }
}

// 处理编辑锁
function handleEditLock(data: any) {
  if (data.userId === props.currentUserId) return

  const existingLock = editLocks.value.findIndex(l => l.line === data.line)

  if (existingLock < 0) {
    editLocks.value.push({
      line: data.line,
      userId: data.userId,
      userName: data.userName,
      timestamp: Date.now()
    })
  }

  // 10秒后自动释放锁
  setTimeout(() => {
    const index = editLocks.value.findIndex(l => l.line === data.line)
    if (index >= 0) {
      editLocks.value.splice(index, 1)
    }
  }, 10000)
}

// 处理解锁
function handleEditUnlock(data: any) {
  const index = editLocks.value.findIndex(l => l.line === data.line)
  if (index >= 0) {
    editLocks.value.splice(index, 1)
  }
}

// 发送光标位置
function sendCursorPosition() {
  const line = getCurrentCursorLine()
  const editor = (window as any).monacoEditor
  const column = editor ? editor.getPosition()?.column || 0 : 0

  ws.send({
    type: 'cursor_update',
    data: {
      userId: props.currentUserId,
      userName: getCurrentUserName(),
      line,
      column,
      documentId: props.documentId
    }
  })
}

// 发送选择区域
function sendSelection() {
  const editor = (window as any).monacoEditor
  if (!editor) return

  const selection = editor.getSelection()
  if (!selection) return

  ws.send({
    type: 'selection_update',
    data: {
      userId: props.currentUserId,
      userName: getCurrentUserName(),
      startLine: selection.startLineNumber - 1,
      endLine: selection.endLineNumber - 1,
      startColumn: selection.startColumn - 1,
      endColumn: selection.endColumn - 1,
      documentId: props.documentId
    }
  })
}

// 获取当前用户名
function getCurrentUserName(): string {
  return localStorage.getItem('userName') || 'Anonymous'
}

// 生成用户颜色
function stringToColor(str: string): string {
  let hash = 0
  for (let i = 0; i < str.length; i++) {
    hash = str.charCodeAt(i) + ((hash << 5) - hash)
  }

  const colors = [
    '#FF6B6B', '#4ECDC4', '#45B7D1', '#FFA07A',
    '#98D8C8', '#F7DC6F', '#BB8FCE', '#85C1E2'
  ]

  return colors[Math.abs(hash) % colors.length]
}

// 获取光标样式
function getCursorStyle(cursor: RemoteCursor) {
  return {
    transform: `translate(${cursor.column * 8}px, ${cursor.line * 20}px)`,
    borderColor: cursor.color
  }
}

// 获取选择区域样式
function getSelectionStyle(selection: RemoteSelection) {
  const top = selection.startLine * 20
  const height = (selection.endLine - selection.startLine + 1) * 20

  return {
    top: `${top}px`,
    height: `${height}px`,
    backgroundColor: `${selection.color}33`,
    borderLeft: `3px solid ${selection.color}`
  }
}

// 解决冲突
function resolveConflict() {
  emit('resolve')
}

// 监听编辑器光标变化
function setupEditorListeners() {
  const editor = (window as any).monacoEditor
  if (!editor) return

  editor.onDidChangeCursorPosition(() => {
    if (cursorUpdateThrottle.value) {
      clearTimeout(cursorUpdateThrottle.value)
    }

    cursorUpdateThrottle.value = window.setTimeout(() => {
      sendCursorPosition()
    }, 100)
  })

  editor.onDidChangeCursorSelection(() => {
    sendSelection()
  })
}

// 生命周期
onMounted(() => {
  setupEditorListeners()

  // 定期清理过期光标
  setInterval(() => {
    const now = Date.now()
    remoteCursors.value = remoteCursors.value.filter(
      c => now - c.timestamp < 5000
    )
  }, 5000)
})

onUnmounted(() => {
  ws.disconnect()
})

// 监听冲突变化
watch(hasConflict, (isConflict) => {
  if (isConflict) {
    emit('conflict', conflictUsers.value)
  }
})
</script>

<style scoped lang="scss">
.cursor-tracker {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  pointer-events: none;
  z-index: 100;
}

.remote-cursor {
  position: absolute;
  width: 2px;
  height: 20px;
  transition: all 0.2s ease;

  .cursor-flag {
    position: absolute;
    top: -20px;
    left: 0;
    padding: 2px 6px;
    border-radius: 3px;
    font-size: 11px;
    color: white;
    white-space: nowrap;
    animation: flagBounce 0.3s ease;
  }

  .cursor-line {
    width: 100%;
    height: 100%;
  }
}

@keyframes flagBounce {
  0% { transform: translateY(-5px); opacity: 0; }
  100% { transform: translateY(0); opacity: 1; }
}

.remote-selection {
  position: absolute;
  left: 0;
  right: 0;
  transition: all 0.2s ease;
}

.edit-lock {
  position: absolute;
  right: 10px;
  width: 16px;
  height: 16px;
  color: var(--el-color-warning);
  background: var(--el-bg-color);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  animation: lockPulse 1s infinite;
}

@keyframes lockPulse {
  0%, 100% { transform: scale(1); }
  50% { transform: scale(1.1); }
}

.conflict-alert {
  position: fixed;
  top: 80px;
  right: 20px;
  z-index: 1000;
  max-width: 400px;

  .conflict-details {
    margin-top: 8px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 12px;

    p {
      margin: 0;
      flex: 1;
    }
  }
}
</style>
