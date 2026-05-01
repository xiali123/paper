<template>
  <div class="collaborative-editor">
    <div class="editor-header">
      <div class="document-info">
        <h2>{{ document?.title }}</h2>
        <el-tag :type="getStatusType(document?.status)">
          {{ document?.status }}
        </el-tag>
      </div>

      <div class="active-users">
        <div
          v-for="user in activeUsers"
          :key="user.id"
          :class="['user-cursor', `user-${user.id}`]"
          :style="{ borderColor: user.color }"
        >
          <span class="cursor-name">{{ user.name }}</span>
          <div class="cursor-line" :style="{ backgroundColor: user.color }" />
        </div>
      </div>
    </div>

    <!-- 编辑器区域 -->
    <div
      ref="editorRef"
      class="editor-content"
      contenteditable="true"
      @input="handleInput"
      @keydown="handleKeydown"
      @click="handleClick"
      @keyup="handleKeyUp"
    >
      {{ document?.content || '开始编辑...' }}
    </div>

    <!-- 连接状态 -->
    <div class="connection-status">
      <el-badge
        :value="connectionStatus"
        :type="isConnected ? 'success' : 'danger'"
      />
      <span>{{ isConnected ? '已连接' : '未连接' }}</span>
    </div>

    <!-- AI建议面板 -->
    <div v-if="suggestions.length" class="suggestions-panel">
      <h4>💡 AI写作建议</h4>
      <div
        v-for="(suggestion, index) in suggestions"
        :key="index"
        class="suggestion-item"
      >
        <div class="suggestion-type">{{ suggestion.suggestionType }}</div>
        <div class="suggestion-text">{{ suggestion.suggestedText }}</div>
        <div class="suggestion-actions">
          <el-button type="primary" size="small" @click="acceptSuggestion(index)">
            接受
          </el-button>
          <el-button size="small" @click="rejectSuggestion(index)">
            忽略
          </el-button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage } from 'element-plus'
import { collaborativeApi } from '@/api/modules/collaborative'
import { useWebSocket } from '@/composables/useWebSocket'
import type { CollaborativeDocument, OTOperation } from '@/types/collaborative'

const route = useRoute()
const documentId = Number(route.params.id)

const document = ref<CollaborativeDocument | null>(null)
const editorRef = ref<HTMLDivElement | null>(null)
const activeUsers = ref<Array<{ id: string; name: string; color: string }>>([])
const suggestions = ref<any[]>([])

// WebSocket连接
const token = localStorage.getItem('token') || ''
const wsUrl = collaborativeApi.getWebSocketUrl(documentId, token)

const { isConnected, send } = useWebSocket({
  url: wsUrl,
  onMessage: (data) => {
    handleWebSocketMessage(data)
  },
  onOpen: () => {
    if (import.meta.env.DEV) console.log('WebSocket connected')
  },
  onClose: () => {
    if (import.meta.env.DEV) console.log('WebSocket disconnected')
  }
})

const handleWebSocketMessage = (data: any) => {
  switch (data.type) {
    case 'operation':
      // 应用其他用户的操作
      applyRemoteOperation(data.operation)
      break
    case 'user_join':
      activeUsers.value.push(data.user)
      ElMessage.info(`${data.user.name} 加入了协作`)
      break
    case 'user_leave':
      activeUsers.value = activeUsers.value.filter(u => u.id !== data.userId)
      break
    case 'cursor_update':
      updateCursorPosition(data.userId, data.position)
      break
    case 'suggestion':
      suggestions.value.push(data.suggestion)
      break
  }
}

const handleInput = (event: Event) => {
  const target = event.target as HTMLDivElement
  const content = target.textContent || ''

  // 创建插入操作
  const operation: OTOperation = {
    id: generateOperationId(),
    type: 'insert',
    position: getCaretPosition(target),
    content: content,
    clientId: getClientId(),
    timestamp: Date.now()
  }

  // 发送操作到服务器
  send({
    type: 'operation',
    documentId,
    operation
  })
}

const handleKeydown = (event: KeyboardEvent) => {
  // 处理快捷键
  if (event.ctrlKey || event.metaKey) {
    if (event.key === 's') {
      event.preventDefault()
      saveDocument()
    }
  }
}

const handleClick = () => {
  // 发送光标位置更新
  const position = getCaretPosition(editorRef.value!)
  send({
    type: 'cursor_update',
    documentId,
    position
  })
}

const handleKeyUp = () => {
  // 防抖发送光标位置
  // ...
}

const applyRemoteOperation = (operation: OTOperation) => {
  if (!editorRef.value) return

  const editor = editorRef.value
  const currentContent = editor.textContent || ''

  if (operation.type === 'insert') {
    const before = currentContent.substring(0, operation.position)
    const after = currentContent.substring(operation.position)
    editor.textContent = before + operation.content + after
  } else if (operation.type === 'delete') {
    const before = currentContent.substring(0, operation.position)
    const after = currentContent.substring(operation.position + (operation.length || 1))
    editor.textContent = before + after
  }
}

const updateCursorPosition = (userId: string, position: number) => {
  // 更新其他用户的光标位置
  // 这里可以显示其他用户的光标
}

const acceptSuggestion = (index: number) => {
  const suggestion = suggestions.value[index]
  if (!editorRef.value) return

  const editor = editorRef.value
  const before = editor.textContent?.substring(0, suggestion.positionStart) || ''
  const after = editor.textContent?.substring(suggestion.positionEnd) || ''

  editor.textContent = before + suggestion.suggestedText + after
  suggestions.value.splice(index, 1)

  ElMessage.success('已接受AI建议')
}

const rejectSuggestion = (index: number) => {
  suggestions.value.splice(index, 1)
  ElMessage.info('已忽略AI建议')
}

const saveDocument = async () => {
  if (!document.value) return

  try {
    const content = editorRef.value?.textContent || ''
    await collaborativeApi.updateDocument(document.value.id, { content })
    ElMessage.success('保存成功')
  } catch (error) {
    ElMessage.error('保存失败')
  }
}

const getCaretPosition = (element: HTMLDivElement): number => {
  const selection = window.getSelection()
  if (!selection || selection.rangeCount === 0) return 0

  const range = selection.getRangeAt(0)
  const preCaretRange = range.cloneRange()
  preCaretRange.selectNodeContents(element)
  preCaretRange.setEnd(range.endContainer, range.endOffset)
  return preCaretRange.toString().length
}

const generateOperationId = (): string => {
  return `${Date.now()}-${Math.random().toString(36).substr(2, 9)}`
}

const getClientId = (): string => {
  let clientId = localStorage.getItem('clientId')
  if (!clientId) {
    clientId = generateOperationId()
    localStorage.setItem('clientId', clientId)
  }
  return clientId
}

const getStatusType = (status?: string) => {
  if (status === 'active') return 'success'
  if (status === 'archived') return 'info'
  return 'danger'
}

const loadDocument = async () => {
  try {
    const doc = await collaborativeApi.getDocument(documentId)
    document.value = doc
    if (editorRef.value) {
      editorRef.value.textContent = doc.content
    }
  } catch (error) {
    ElMessage.error('加载文档失败')
  }
}

onMounted(() => {
  loadDocument()
})

onUnmounted(() => {
  // 清理工作
})
</script>

<style scoped>
.collaborative-editor {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  overflow: hidden;
}

.editor-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 24px;
  border-bottom: 1px solid #e4e7ed;
}

.document-info {
  display: flex;
  align-items: center;
  gap: 12px;
}

.document-info h2 {
  font-size: 18px;
  font-weight: 600;
  margin: 0;
}

.active-users {
  display: flex;
  gap: 8px;
}

.user-cursor {
  position: relative;
  padding: 4px 8px;
  border-left: 3px solid;
  background: rgba(0, 0, 0, 0.05);
  border-radius: 4px;
  font-size: 12px;
}

.cursor-name {
  font-weight: 600;
}

.editor-content {
  flex: 1;
  padding: 32px;
  font-size: 16px;
  line-height: 1.8;
  color: #303133;
  overflow-y: auto;
  outline: none;
  white-space: pre-wrap;
  word-wrap: break-word;
}

.editor-content:focus {
  background: #fafafa;
}

.connection-status {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  background: #f5f7fa;
  border-top: 1px solid #e4e7ed;
  font-size: 13px;
}

.suggestions-panel {
  padding: 16px 24px;
  background: #f0f9ff;
  border-top: 1px solid #e4e7ed;
  max-height: 300px;
  overflow-y: auto;
}

.suggestions-panel h4 {
  font-size: 14px;
  font-weight: 600;
  margin: 0 0 12px 0;
}

.suggestion-item {
  background: white;
  border-radius: 8px;
  padding: 12px;
  margin-bottom: 8px;
  border-left: 3px solid #409eff;
}

.suggestion-type {
  font-size: 12px;
  font-weight: 600;
  color: #409eff;
  margin-bottom: 4px;
}

.suggestion-text {
  font-size: 14px;
  color: #606266;
  margin-bottom: 8px;
  line-height: 1.5;
}

.suggestion-actions {
  display: flex;
  gap: 8px;
}
</style>
