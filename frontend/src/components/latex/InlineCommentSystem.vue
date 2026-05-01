/**
 * LaTeX行内评论系统
 * 支持选中区域评论、评论线程、@提及、已解决状态
 */

<template>
  <div class="inline-comment-system">
    <!-- 评论标记层 -->
    <div class="comment-markers-layer">
      <div
        v-for="marker in commentMarkers"
        :key="marker.id"
        class="comment-marker"
        :class="{ 'is-resolved': marker.resolved }"
        :style="getMarkerStyle(marker)"
        @click="selectComment(marker.id)"
      >
        <div class="marker-dot" :style="{ backgroundColor: marker.color }"></div>
        <div class="marker-count">{{ marker.commentCount }}</div>
      </div>
    </div>

    <!-- 选中评论按钮 -->
    <transition name="fade">
      <div
        v-if="showCommentButton && selectedRange"
        class="comment-float-button"
        :style="commentButtonStyle"
      >
        <el-button size="small" type="primary" @click="openCommentDialog">
          <el-icon><ChatDotRound /></el-icon>
          添加评论
        </el-button>
      </div>
    </transition>

    <!-- 评论详情面板 -->
    <el-drawer
      v-model="showCommentDrawer"
      title="评论详情"
      :size="400"
      direction="rtl"
    >
      <div v-if="selectedComment" class="comment-detail">
        <!-- 引用文本 -->
        <div class="quoted-text">
          <el-icon><ChatLineSquare /></el-icon>
          <span>{{ selectedComment.quotedText }}</span>
        </div>

        <!-- 位置信息 -->
        <div class="comment-location">
          <el-tag size="small" type="info">
            第 {{ selectedRange?.startLine }}-{{ selectedRange?.endLine }} 行
          </el-tag>
        </div>

        <!-- 评论列表 -->
        <div class="comment-thread">
          <div
            v-for="reply in selectedComment.replies"
            :key="reply.id"
            class="comment-reply"
          >
            <div class="reply-header">
              <el-avatar :size="32" :src="reply.avatar">
                {{ reply.userName.charAt(0) }}
              </el-avatar>
              <div class="reply-meta">
                <span class="reply-author">{{ reply.userName }}</span>
                <span class="reply-time">{{ formatTime(reply.timestamp) }}</span>
              </div>
            </div>
            <div class="reply-content">{{ reply.content }}</div>
            <!-- @提及高亮 -->
            <div v-if="reply.mentions?.length" class="reply-mentions">
              <el-tag
                v-for="mention in reply.mentions"
                :key="mention"
                size="small"
                type="warning"
              >
                @{{ mention }}
              </el-tag>
            </div>
          </div>
        </div>

        <!-- 回复输入 -->
        <div class="reply-input">
          <el-input
            v-model="replyText"
            type="textarea"
            :rows="3"
            placeholder="输入回复... (使用 @username 提及用户)"
            @keydown="handleReplyKeydown"
          />
          <div class="reply-actions">
            <el-button size="small" @click="showEmojiPicker = !showEmojiPicker">
              😊
            </el-button>
            <el-button
              type="primary"
              size="small"
              :disabled="!replyText.trim()"
              :loading="replying"
              @click="submitReply"
            >
              回复
            </el-button>
          </div>
        </div>

        <!-- 解决状态 -->
        <div class="comment-status">
          <el-button
            v-if="!selectedComment.resolved"
            type="success"
            size="small"
            @click="resolveComment"
          >
            <el-icon><Select /></el-icon>
            标记为已解决
          </el-button>
          <el-button
            v-else
            size="small"
            @click="reopenComment"
          >
            <el-icon><RefreshLeft /></el-icon>
            重新打开
          </el-button>
          <el-button
            type="danger"
            size="small"
            @click="deleteComment"
          >
            删除评论
          </el-button>
        </div>
      </div>

      <el-empty v-else description="选择一个评论查看详情" />
    </el-drawer>

    <!-- 新建评论对话框 -->
    <el-dialog
      v-model="showNewCommentDialog"
      title="添加评论"
      width="500px"
    >
      <div v-if="selectedRange" class="new-comment-form">
        <div class="quoted-text-preview">
          <el-icon><ChatLineSquare /></el-icon>
          <span>{{ getQuotedTextPreview() }}</span>
        </div>

        <el-input
          v-model="newCommentText"
          type="textarea"
          :rows="4"
          placeholder="输入评论内容... (使用 @username 提及其他用户)"
          @keydown="handleNewCommentKeydown"
        />

        <div class="comment-options">
          <el-checkbox v-model="assignToMe">分配给我</el-checkbox>
          <el-select
            v-model="commentPriority"
            size="small"
            style="width: 120px"
          >
            <el-option label="普通" value="normal" />
            <el-option label="重要" value="high" />
            <el-option label="紧急" value="urgent" />
          </el-select>
        </div>
      </div>

      <template #footer>
        <el-button @click="showNewCommentDialog = false">取消</el-button>
        <el-button
          type="primary"
          :disabled="!newCommentText.trim()"
          :loading="submitting"
          @click="submitNewComment"
        >
          添加评论
        </el-button>
      </template>
    </el-dialog>

    <!-- @提及用户选择器 -->
    <transition name="fade">
      <div
        v-if="showMentionPicker"
        class="mention-picker"
        :style="mentionPickerStyle"
      >
        <div class="mention-search">
          <el-input
            v-model="mentionSearch"
            size="small"
            placeholder="搜索用户..."
            prefix-icon="Search"
          />
        </div>
        <div class="mention-list">
          <div
            v-for="user in filteredUsers"
            :key="user.id"
            class="mention-item"
            @click="insertMention(user)"
          >
            <el-avatar :size="24" :src="user.avatar">
              {{ user.name.charAt(0) }}
            </el-avatar>
            <span>{{ user.name }}</span>
          </div>
        </div>
      </div>
    </transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue'
import {
  ChatDotRound,
  ChatLineSquare,
  Select,
  RefreshLeft,
  Search
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import { useWebSocket } from '@/composables/useWebSocket'

interface CommentMarker {
  id: string
  startLine: number
  endLine: number
  startColumn: number
  endColumn: number
  color: string
  commentCount: number
  resolved: boolean
}

interface CommentThread {
  id: string
  quotedText: string
  startLine: number
  endLine: number
  resolved: boolean
  replies: CommentReply[]
  createdAt: number
}

interface CommentReply {
  id: string
  userName: string
  userId: string
  avatar?: string
  content: string
  timestamp: number
  mentions?: string[]
}

interface User {
  id: string
  name: string
  avatar?: string
}

interface Props {
  editor?: any
  documentId: string
  currentUserId: string
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'comment-added': [comment: CommentThread]
  'comment-resolved': [commentId: string]
  'comment-deleted': [commentId: string]
}>()

// 状态
const comments = ref<CommentThread[]>([])
const commentMarkers = ref<CommentMarker[]>([])
const selectedCommentId = ref<string | null>(null)
const selectedRange = ref<{ startLine: number; endLine: number; text: string } | null>(null)
const showCommentDrawer = ref(false)
const showNewCommentDialog = ref(false)
const showCommentButton = ref(false)
const commentButtonPosition = ref({ x: 0, y: 0 })

// 新建评论
const newCommentText = ref('')
const assigningTo = ref<string[]>([])
const commentPriority = ref<'normal' | 'high' | 'urgent'>('normal')
const assignToMe = ref(false)
const submitting = ref(false)

// 回复
const replyText = ref('')
const replying = ref(false)
const showEmojiPicker = ref(false)

// @提及
const showMentionPicker = ref(false)
const mentionSearch = ref('')
const mentionPickerPosition = ref({ x: 0, y: 0 })

// 模拟用户列表
const availableUsers = ref<User[]>([
  { id: '1', name: '张三' },
  { id: '2', name: '李四' },
  { id: '3', name: '王五' },
  { id: '4', name: '赵六' }
])

// WebSocket连接
const ws = useWebSocket(
  {
    url: `${import.meta.env.VITE_WS_URL || 'ws://localhost:8087/ws'}/comments/${props.documentId}`,
    reconnectInterval: 2000
  },
  {
    onMessage: (message) => {
      handleCommentMessage(message)
    }
  }
)

// 当前选中的评论
const selectedComment = computed(() => {
  return comments.value.find(c => c.id === selectedCommentId.value) || null
})

// 过滤用户
const filteredUsers = computed(() => {
  if (!mentionSearch.value) return availableUsers.value
  const search = mentionSearch.value.toLowerCase()
  return availableUsers.value.filter(u =>
    u.name.toLowerCase().includes(search)
  )
})

// 评论按钮样式
const commentButtonStyle = computed(() => ({
  position: 'fixed',
  left: `${commentButtonPosition.value.x}px`,
  top: `${commentButtonPosition.value.y}px`,
  zIndex: 1000
}))

// 提及选择器样式
const mentionPickerStyle = computed(() => ({
  position: 'fixed',
  left: `${mentionPickerPosition.value.x}px`,
  top: `${mentionPickerPosition.value.y}px`,
  zIndex: 1001
}))

// 获取标记样式
const getMarkerStyle = (marker: CommentMarker) => {
  const editor = props.editor || (window as any).monacoEditor
  if (!editor) return {}

  const top = editor.getTopForLineNumber(marker.startLine)
  const lineHeight = editor.getOption(60) // line height

  return {
    position: 'absolute',
    top: `${top}px`,
    left: '0',
    height: `${(marker.endLine - marker.startLine + 1) * lineHeight}px`,
    borderLeft: `3px solid ${marker.color}`,
    backgroundColor: `${marker.color}11`
  }
}

// 获取引用文本预览
const getQuotedTextPreview = () => {
  if (!selectedRange.value) return ''
  const text = selectedRange.value.text
  const maxLength = 100
  return text.length > maxLength
    ? text.substring(0, maxLength) + '...'
    : text
}

// 选择评论
const selectComment = (commentId: string) => {
  selectedCommentId.value = commentId
  showCommentDrawer.value = true
}

// 打开评论对话框
const openCommentDialog = () => {
  showNewCommentDialog.value = true
}

// 处理文本选择
const handleTextSelection = () => {
  const editor = props.editor || (window as any).monacoEditor
  if (!editor) return

  const selection = editor.getSelection()
  if (!selection || selection.isEmpty()) {
    showCommentButton.value = false
    return
  }

  const selectedText = editor.getModel()?.getValueInRange(selection) || ''
  if (!selectedText.trim()) {
    showCommentButton.value = false
    return
  }

  // 获取选择区域的屏幕位置
  const top = editor.getTopForLineNumber(selection.endLineNumber)
  const left = editor.getOffsetForColumn(selection.endLineNumber, selection.endColumn)

  selectedRange.value = {
    startLine: selection.startLineNumber,
    endLine: selection.endLineNumber,
    text: selectedText
  }

  commentButtonPosition.value = {
    x: left + 50,
    y: top + 30
  }

  showCommentButton.value = true
}

// 提交新评论
const submitNewComment = async () => {
  if (!newCommentText.value.trim() || !selectedRange.value) return

  submitting.value = true

  // 解析@提及
  const mentions = newCommentText.value.match(/@(\w+)/g)?.map(m => m.substring(1)) || []

  const newComment: CommentThread = {
    id: `comment-${Date.now()}`,
    quotedText: selectedRange.value.text,
    startLine: selectedRange.value.startLine,
    endLine: selectedRange.value.endLine,
    resolved: false,
    replies: [{
      id: `reply-${Date.now()}`,
      userName: '当前用户',
      userId: props.currentUserId,
      content: newCommentText.value,
      timestamp: Date.now(),
      mentions
    }],
    createdAt: Date.now()
  }

  comments.value.push(newComment)

  // 添加标记
  const color = stringToColor(newComment.id)
  commentMarkers.value.push({
    id: newComment.id,
    startLine: selectedRange.value.startLine,
    endLine: selectedRange.value.endLine,
    startColumn: 0,
    endColumn: 0,
    color,
    commentCount: 1,
    resolved: false
  })

  // 发送到服务器
  ws.send({
    type: 'new_comment',
    data: {
      documentId: props.documentId,
      comment: newComment
    }
  })

  emit('comment-added', newComment)

  // 重置状态
  newCommentText.value = ''
  showNewCommentDialog.value = false
  showCommentButton.value = false
  submitting.value = false

  ElMessage.success('评论已添加')
}

// 提交回复
const submitReply = async () => {
  if (!replyText.value.trim() || !selectedComment.value) return

  replying.value = true

  const mentions = replyText.value.match(/@(\w+)/g)?.map(m => m.substring(1)) || []

  const newReply: CommentReply = {
    id: `reply-${Date.now()}`,
    userName: '当前用户',
    userId: props.currentUserId,
    content: replyText.value,
    timestamp: Date.now(),
    mentions
  }

  selectedComment.value.replies.push(newReply)

  // 更新标记数量
  const marker = commentMarkers.value.find(m => m.id === selectedComment.value?.id)
  if (marker) {
    marker.commentCount = selectedComment.value.replies.length
  }

  ws.send({
    type: 'new_reply',
    data: {
      commentId: selectedComment.value.id,
      reply: newReply
    }
  })

  replyText.value = ''
  replying.value = false

  ElMessage.success('回复已发送')
}

// 解决评论
const resolveComment = () => {
  if (!selectedComment.value) return

  selectedComment.value.resolved = true

  const marker = commentMarkers.value.find(m => m.id === selectedComment.value?.id)
  if (marker) {
    marker.resolved = true
  }

  ws.send({
    type: 'resolve_comment',
    data: {
      commentId: selectedComment.value.id
    }
  })

  emit('comment-resolved', selectedComment.value.id)
  ElMessage.success('评论已标记为解决')
}

// 重新打开评论
const reopenComment = () => {
  if (!selectedComment.value) return

  selectedComment.value.resolved = false

  const marker = commentMarkers.value.find(m => m.id === selectedComment.value?.id)
  if (marker) {
    marker.resolved = false
  }

  ws.send({
    type: 'reopen_comment',
    data: {
      commentId: selectedComment.value.id
    }
  })

  ElMessage.success('评论已重新打开')
}

// 删除评论
const deleteComment = () => {
  if (!selectedComment.value) return

  const index = comments.value.findIndex(c => c.id === selectedComment.value.id)
  if (index >= 0) {
    comments.value.splice(index, 1)
  }

  const markerIndex = commentMarkers.value.findIndex(m => m.id === selectedComment.value?.id)
  if (markerIndex >= 0) {
    commentMarkers.value.splice(markerIndex, 1)
  }

  ws.send({
    type: 'delete_comment',
    data: {
      commentId: selectedComment.value.id
    }
  })

  emit('comment-deleted', selectedComment.value.id)
  showCommentDrawer.value = false

  ElMessage.success('评论已删除')
}

// 插入@提及
const insertMention = (user: User) => {
  const textarea = document.querySelector('.new-comment-form textarea, .reply-input textarea') as HTMLTextAreaElement
  if (!textarea) return

  const start = textarea.selectionStart
  const end = textarea.selectionEnd
  const text = textarea.value
  const before = text.substring(0, start)
  const after = text.substring(end)

  // 找到最后一个@的位置
  const lastAtIndex = before.lastIndexOf('@')
  if (lastAtIndex >= 0) {
    const newText = before.substring(0, lastAtIndex) + `@${user.name} ` + after
    if (newCommentText.value.includes(before)) {
      newCommentText.value = newText
    } else {
      replyText.value = newText
    }
  }

  showMentionPicker.value = false
  mentionSearch.value = ''

  // 聚焦回输入框
  setTimeout(() => {
    textarea.focus()
  }, 0)
}

// 处理回复按键
const handleReplyKeydown = (event: KeyboardEvent) => {
  // Ctrl+Enter 发送
  if (event.ctrlKey && event.key === 'Enter') {
    event.preventDefault()
    submitReply()
  }

  // 检测@输入
  const target = event.target as HTMLTextAreaElement
  const cursorPosition = target.selectionStart
  const textBeforeCursor = target.value.substring(0, cursorPosition)

  const lastAtIndex = textBeforeCursor.lastIndexOf('@')
  if (lastAtIndex >= 0) {
    const afterAtIndex = textBeforeCursor.substring(lastAtIndex + 1)
    if (!afterAtIndex.includes(' ')) {
      mentionSearch.value = afterAtIndex
      showMentionPicker.value = true

      // 获取输入框位置
      const rect = target.getBoundingClientRect()
      mentionPickerPosition.value = {
        x: rect.left,
        y: rect.bottom + 5
      }
    } else {
      showMentionPicker.value = false
    }
  }
}

// 处理新评论按键
const handleNewCommentKeydown = (event: KeyboardEvent) => {
  // Ctrl+Enter 发送
  if (event.ctrlKey && event.key === 'Enter') {
    event.preventDefault()
    submitNewComment()
  }

  // 复用回复的提及处理
  handleReplyKeydown(event)
}

// 处理评论WebSocket消息
const handleCommentMessage = (message: any) => {
  switch (message.type) {
    case 'new_comment':
      comments.value.push(message.data.comment)
      break
    case 'new_reply':
      const comment = comments.value.find(c => c.id === message.data.commentId)
      if (comment) {
        comment.replies.push(message.data.reply)
      }
      break
    case 'resolve_comment':
      const resolvedComment = comments.value.find(c => c.id === message.data.commentId)
      if (resolvedComment) {
        resolvedComment.resolved = true
      }
      break
    case 'delete_comment':
      const index = comments.value.findIndex(c => c.id === message.data.commentId)
      if (index >= 0) {
        comments.value.splice(index, 1)
      }
      break
  }
}

// 生成颜色
const stringToColor = (str: string): string => {
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

// 格式化时间
const formatTime = (timestamp: number): string => {
  const date = new Date(timestamp)
  const now = new Date()
  const diff = now.getTime() - date.getTime()
  const minutes = Math.floor(diff / 60000)

  if (minutes < 1) return '刚刚'
  if (minutes < 60) return `${minutes}分钟前`
  if (minutes < 1440) return `${Math.floor(minutes / 60)}小时前`
  return date.toLocaleDateString('zh-CN')
}

// 设置编辑器监听器
const setupEditorListeners = () => {
  const editor = props.editor || (window as any).monacoEditor
  if (!editor) return

  editor.onDidChangeCursorSelection(() => {
    handleTextSelection()
  })
}

// 生命周期
onMounted(() => {
  setupEditorListeners()

  // 加载现有评论
  ws.send({
    type: 'load_comments',
    data: {
      documentId: props.documentId
    }
  })
})

onUnmounted(() => {
  ws.disconnect()
})

// 点击其他地方隐藏按钮
document.addEventListener('click', (e) => {
  const target = e.target as HTMLElement
  if (!target.closest('.comment-float-button') && !target.closest('.monaco-editor')) {
    showCommentButton.value = false
  }
})

// 暴露方法
defineExpose({
  addComment: (comment: CommentThread) => {
    comments.value.push(comment)
  },
  getComments: () => comments.value,
  resolveCommentById: (id: string) => {
    const comment = comments.value.find(c => c.id === id)
    if (comment) {
      comment.resolved = true
    }
  }
})
</script>

<style scoped lang="scss">
.inline-comment-system {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  pointer-events: none;
}

.comment-markers-layer {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  pointer-events: none;
}

.comment-marker {
  position: absolute;
  pointer-events: auto;
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    opacity: 0.8;
  }

  &.is-resolved {
    opacity: 0.4;
    border-left-style: dashed !important;
  }

  .marker-dot {
    position: absolute;
    top: 0;
    left: 0;
    width: 8px;
    height: 8px;
    border-radius: 50%;
  }

  .marker-count {
    position: absolute;
    top: -8px;
    left: 10px;
    background: var(--el-bg-color);
    border: 1px solid var(--el-border-color);
    border-radius: 10px;
    padding: 0 6px;
    font-size: 10px;
    font-weight: 600;
  }
}

.comment-float-button {
  pointer-events: auto;
}

.fade-enter-active, .fade-leave-active {
  transition: opacity 0.2s;
}

.fade-enter-from, .fade-leave-to {
  opacity: 0;
}

.comment-detail {
  display: flex;
  flex-direction: column;
  height: 100%;
  gap: 16px;
}

.quoted-text {
  display: flex;
  gap: 8px;
  padding: 12px;
  background: var(--el-fill-color-light);
  border-left: 3px solid var(--el-color-primary);
  border-radius: 4px;
  font-size: 13px;
  color: var(--el-text-color-regular);
  font-style: italic;
}

.comment-location {
  display: flex;
  justify-content: center;
}

.comment-thread {
  flex: 1;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.comment-reply {
  display: flex;
  flex-direction: column;
  gap: 8px;
  padding: 12px;
  background: var(--el-fill-color-blank);
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 8px;

  .reply-header {
    display: flex;
    gap: 8px;
    align-items: center;
  }

  .reply-meta {
    display: flex;
    flex-direction: column;
  }

  .reply-author {
    font-weight: 600;
    font-size: 13px;
  }

  .reply-time {
    font-size: 11px;
    color: var(--el-text-color-secondary);
  }

  .reply-content {
    font-size: 13px;
    line-height: 1.5;
    color: var(--el-text-color-regular);
  }

  .reply-mentions {
    display: flex;
    gap: 4px;
    flex-wrap: wrap;
  }
}

.reply-input {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.reply-actions {
  display: flex;
  justify-content: space-between;
}

.comment-status {
  display: flex;
  gap: 8px;
  padding-top: 8px;
  border-top: 1px solid var(--el-border-color-lighter);
}

.new-comment-form {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.quoted-text-preview {
  display: flex;
  gap: 8px;
  padding: 12px;
  background: var(--el-fill-color-light);
  border-radius: 4px;
  font-size: 12px;
  color: var(--el-text-color-secondary);
}

.comment-options {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.mention-picker {
  position: fixed;
  width: 200px;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  overflow: hidden;
  pointer-events: auto;
}

.mention-search {
  padding: 8px;
  border-bottom: 1px solid var(--el-border-color-lighter);
}

.mention-list {
  max-height: 200px;
  overflow-y: auto;
}

.mention-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  cursor: pointer;
  transition: background 0.2s;

  &:hover {
    background: var(--el-fill-color-light);
  }
}
</style>
