<template>
  <div class="review-mode-container">
    <!-- 审阅工具栏 -->
    <div class="review-toolbar" v-if="enabled">
      <div class="toolbar-section">
        <span class="toolbar-label">审阅模式</span>
        <el-switch v-model="enabled" @change="onToggle" />
      </div>

      <el-divider direction="vertical" />

      <div class="toolbar-actions">
        <el-button-group size="small">
          <el-tooltip content="添加批注 (Ctrl+Alt+C)">
            <el-button
              :type="commentMode ? 'primary' : 'default'"
              @click="toggleCommentMode"
            >
              <el-icon><ChatDotSquare /></el-icon>
            </el-button>
          </el-tooltip>

          <el-tooltip content="删除 (建议删除)">
            <el-button @click="addSuggestion('delete')">
              <el-icon><Delete /></el-icon>
            </el-button>
          </el-tooltip>

          <el-tooltip content="插入 (建议插入)">
            <el-button @click="addSuggestion('insert')">
              <el-icon><Plus /></el-icon>
            </el-button>
          </el-tooltip>

          <el-tooltip content="替换 (建议替换)">
            <el-button @click="addSuggestion('replace')">
              <el-icon><Edit /></el-icon>
            </el-button>
          </el-tooltip>
        </el-button-group>

        <el-dropdown @command="handleCommand" trigger="click">
          <el-button size="small">
            <el-icon><MoreFilled /></el-icon>
          </el-button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item command="accept-all">
                <span>✓ 接受所有修改</span>
              </el-dropdown-item>
              <el-dropdown-item command="reject-all">
                <span>✗ 拒绝所有修改</span>
              </el-dropdown-item>
              <el-dropdown-item command="export" divided>
                <span>导出审阅报告</span>
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
    </div>

    <!-- 批注列表侧边栏 -->
    <div class="comments-sidebar" v-if="enabled && showComments">
      <div class="sidebar-header">
        <h4>批注 ({{ comments.length }})</h4>
        <el-button text @click="showComments = false">
          <el-icon><Close /></el-icon>
        </el-button>
      </div>

      <div class="comments-list">
        <div
          v-for="comment in comments"
          :key="comment.id"
          class="comment-item"
          :class="{ resolved: comment.resolved }"
        >
          <div class="comment-header">
            <div class="comment-author">{{ comment.author }}</div>
            <div class="comment-time">{{ formatTime(comment.timestamp) }}</div>
            <div class="comment-actions">
              <el-button
                text
                size="small"
                @click="resolveComment(comment.id)"
                v-if="!comment.resolved"
              >
                ✓
              </el-button>
              <el-button
                text
                size="small"
                type="danger"
                @click="deleteComment(comment.id)"
              >
                ×
              </el-button>
            </div>
          </div>

          <div class="comment-quote" v-if="comment.quote">
            "{{ comment.quote }}"
          </div>

          <div class="comment-text">{{ comment.text }}</div>

          <div class="comment-replies" v-if="comment.replies?.length">
            <div
              v-for="reply in comment.replies"
              :key="reply.id"
              class="comment-reply"
            >
              <span class="reply-author">{{ reply.author }}:</span>
              <span class="reply-text">{{ reply.text }}</span>
            </div>
          </div>

          <div class="comment-reply-box">
            <el-input
              v-model="replyText[comment.id]"
              placeholder="回复..."
              size="small"
              @keyup.enter="addReply(comment.id)"
            >
              <template #append>
                <el-button
                  size="small"
                  @click="addReply(comment.id)"
                  :disabled="!replyText[comment.id]?.trim()"
                >
                  发送
                </el-button>
              </template>
            </el-input>
          </div>
        </div>

        <el-empty
          v-if="comments.length === 0"
          description="暂无批注"
          :image-size="60"
        />
      </div>
    </div>

    <!-- 浮动批注按钮 -->
    <el-button
      v-if="enabled && !showComments"
      class="comments-toggle"
      circle
      size="small"
      @click="showComments = true"
    >
      <el-badge :value="comments.length" :hidden="comments.length === 0">
        <el-icon><ChatDotSquare /></el-icon>
      </el-badge>
    </el-button>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import {
  ChatDotSquare,
  Delete,
  Plus,
  Edit,
  MoreFilled,
  Close
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'

interface CommentReply {
  id: string
  author: string
  text: string
  timestamp: number
}

interface Comment {
  id: string
  author: string
  text: string
  quote?: string
  line: number
  timestamp: number
  resolved: boolean
  replies?: CommentReply[]
}

interface Suggestion {
  id: string
  type: 'delete' | 'insert' | 'replace'
  line: number
  start: number
  end: number
  originalText: string
  suggestedText?: string
  author: string
  timestamp: number
  accepted?: boolean
}

const emit = defineEmits<{
  toggle: [enabled: boolean]
  insert: [text: string]
}>()

const enabled = ref(false)
const commentMode = ref(false)
const showComments = ref(false)
const comments = ref<Comment[]>([])
const suggestions = ref<Suggestion[]>([])
const replyText = ref<Record<string, string>>({})

// 切换审阅模式
const onToggle = (value: boolean | string | number) => {
  emit('toggle', Boolean(value))
}

// 切换批注模式
const toggleCommentMode = () => {
  commentMode.value = !commentMode.value
  ElMessage.info(commentMode.value ? '批注模式已开启，选择文本以添加批注' : '批注模式已关闭')
}

// 添加建议
const addSuggestion = (type: 'delete' | 'insert' | 'replace') => {
  // 这里需要与编辑器集成，获取选中的文本
  ElMessage.info(`选择文本以${type === 'delete' ? '删除' : type === 'insert' ? '插入' : '替换'}`)
}

// 处理命令
const handleCommand = async (command: string) => {
  switch (command) {
    case 'accept-all':
      await ElMessageBox.confirm('确定要接受所有修改建议吗？', '确认', {
        type: 'warning'
      })
      suggestions.value.forEach(s => s.accepted = true)
      ElMessage.success('已接受所有修改')
      break

    case 'reject-all':
      await ElMessageBox.confirm('确定要拒绝所有修改建议吗？', '确认', {
        type: 'warning'
      })
      suggestions.value = []
      ElMessage.success('已拒绝所有修改')
      break

    case 'export':
      exportReviewReport()
      break
  }
}

// 解析批注
const resolveComment = (id: string) => {
  const comment = comments.value.find(c => c.id === id)
  if (comment) {
    comment.resolved = true
    ElMessage.success('批注已标记为已解决')
  }
}

// 删除批注
const deleteComment = (id: string) => {
  const index = comments.value.findIndex(c => c.id === id)
  if (index > -1) {
    comments.value.splice(index, 1)
    ElMessage.success('批注已删除')
  }
}

// 添加回复
const addReply = (commentId: string) => {
  const text = replyText.value[commentId]?.trim()
  if (!text) return

  const comment = comments.value.find(c => c.id === commentId)
  if (comment) {
    if (!comment.replies) {
      comment.replies = []
    }
    comment.replies.push({
      id: `reply-${Date.now()}`,
      author: '当前用户',
      text,
      timestamp: Date.now()
    })
    replyText.value[commentId] = ''
    ElMessage.success('回复已添加')
  }
}

// 格式化时间
const formatTime = (timestamp: number) => {
  const date = new Date(timestamp)
  const now = new Date()
  const diff = now.getTime() - date.getTime()

  if (diff < 60000) {
    return '刚刚'
  } else if (diff < 3600000) {
    return `${Math.floor(diff / 60000)}分钟前`
  } else if (diff < 86400000) {
    return `${Math.floor(diff / 3600000)}小时前`
  } else {
    return date.toLocaleDateString()
  }
}

// 导出审阅报告
const exportReviewReport = () => {
  const report = {
    exportDate: new Date().toISOString(),
    summary: {
      totalComments: comments.value.length,
      resolvedComments: comments.value.filter(c => c.resolved).length,
      pendingComments: comments.value.filter(c => !c.resolved).length,
      totalSuggestions: suggestions.value.length,
      acceptedSuggestions: suggestions.value.filter(s => s.accepted).length
    },
    comments: comments.value,
    suggestions: suggestions.value
  }

  const blob = new Blob([JSON.stringify(report, null, 2)], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `review-report-${Date.now()}.json`
  a.click()
  URL.revokeObjectURL(url)

  ElMessage.success('审阅报告已导出')
}

// 添加批注（供外部调用）
const addComment = (quote: string, text: string, line: number) => {
  comments.value.push({
    id: `comment-${Date.now()}`,
    author: '当前用户',
    text,
    quote,
    line,
    timestamp: Date.now(),
    resolved: false
  })
  ElMessage.success('批注已添加')
}

// 暴露方法
defineExpose({
  addComment,
  enabled,
  comments,
  suggestions
})
</script>

<style scoped lang="scss">
.review-mode-container {
  position: relative;
}

.review-toolbar {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 8px 16px;
  background: var(--el-fill-color-light);
  border-bottom: 1px solid var(--el-border-color-light);
}

.toolbar-section {
  display: flex;
  align-items: center;
  gap: 12px;
}

.toolbar-label {
  font-size: 13px;
  font-weight: 500;
  color: var(--el-text-color-regular);
}

.toolbar-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}

.comments-sidebar {
  position: fixed;
  right: 0;
  top: 60px;
  bottom: 0;
  width: 360px;
  background: var(--el-bg-color);
  border-left: 1px solid var(--el-border-color-light);
  box-shadow: -2px 0 8px rgba(0, 0, 0, 0.1);
  display: flex;
  flex-direction: column;
  z-index: 1000;
}

.sidebar-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  border-bottom: 1px solid var(--el-border-color-light);

  h4 {
    margin: 0;
    font-size: 14px;
    font-weight: 600;
  }
}

.comments-list {
  flex: 1;
  overflow-y: auto;
  padding: 16px;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.comment-item {
  padding: 12px;
  background: var(--el-fill-color-light);
  border-radius: 8px;
  border: 2px solid var(--el-color-primary);

  &.resolved {
    opacity: 0.6;
    border-color: var(--el-color-success);
  }
}

.comment-header {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
}

.comment-author {
  flex: 1;
  font-size: 13px;
  font-weight: 500;
  color: var(--el-text-color-primary);
}

.comment-time {
  font-size: 11px;
  color: var(--el-text-color-secondary);
}

.comment-actions {
  display: flex;
  gap: 4px;
}

.comment-quote {
  padding: 8px;
  background: var(--el-fill-color);
  border-radius: 4px;
  font-size: 12px;
  font-style: italic;
  color: var(--el-text-color-secondary);
  margin-bottom: 8px;
  border-left: 3px solid var(--el-color-warning);
}

.comment-text {
  font-size: 13px;
  color: var(--el-text-color-regular);
  line-height: 1.6;
  margin-bottom: 12px;
}

.comment-replies {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin-bottom: 12px;
  padding-left: 12px;
  border-left: 2px solid var(--el-border-color-light);
}

.comment-reply {
  font-size: 12px;
  line-height: 1.5;
}

.reply-author {
  font-weight: 500;
  color: var(--el-text-color-primary);
}

.reply-text {
  color: var(--el-text-color-regular);
}

.comment-reply-box {
  :deep(.el-input-group) {
    .el-input__inner {
      font-size: 12px;
    }
  }
}

.comments-toggle {
  position: fixed;
  right: 24px;
  bottom: 80px;
  z-index: 999;
}
</style>
