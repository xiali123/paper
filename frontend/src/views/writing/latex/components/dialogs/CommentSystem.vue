<template>
  <BaseDialog
    :show="show"
    title="评论批注"
    width="600px"
    @update:show="$emit('update:show', $event)"
  >
    <div class="comment-system">
      <!-- 评论列表 -->
      <div class="comments-list">
        <div
          v-for="comment in sortedComments"
          :key="comment.id"
          class="comment-item"
          :class="{ 'is-resolved': comment.resolved }"
        >
          <div class="comment-avatar">
            <el-avatar :size="32" :src="comment.avatar">
              {{ comment.author?.[0] || '?' }}
            </el-avatar>
          </div>

          <div class="comment-content">
            <div class="comment-header">
              <span class="comment-author">{{ comment.author }}</span>
              <span class="comment-time">{{ formatTime(comment.createdAt) }}</span>
              <el-tag v-if="comment.resolved" size="small" type="success">已解决</el-tag>
              <el-dropdown trigger="click" @command="(cmd) => handleCommentAction(cmd, comment)">
                <el-button text size="small">
                  <el-icon><MoreFilled /></el-icon>
                </el-button>
                <template #dropdown>
                  <el-dropdown-menu>
                    <el-dropdown-item command="reply">回复</el-dropdown-item>
                    <el-dropdown-item command="resolve" v-if="!comment.resolved">标记已解决</el-dropdown-item>
                    <el-dropdown-item command="reopen" v-else>重新打开</el-dropdown-item>
                    <el-dropdown-item command="edit" v-if="isOwnComment(comment)">编辑</el-dropdown-item>
                    <el-dropdown-item command="delete" v-if="isOwnComment(comment)" divided>删除</el-dropdown-item>
                  </el-dropdown-menu>
                </template>
              </el-dropdown>
            </div>

            <!-- 引用内容 -->
            <div v-if="comment.quote" class="comment-quote">
              <blockquote>
                "{{ comment.quote }}"
              </blockquote>
              <div class="quote-location" @click="$emit('navigate', comment.line)">
                <el-icon><Position /></el-icon>
                第{{ comment.line }}行
              </div>
            </div>

            <!-- 评论内容 -->
            <div class="comment-text">
              {{ comment.text }}
            </div>

            <!-- 回复列表 -->
            <div v-if="comment.replies && comment.replies.length > 0" class="comment-replies">
              <div
                v-for="reply in comment.replies"
                :key="reply.id"
                class="reply-item"
              >
                <el-avatar :size="24" :src="reply.avatar">
                  {{ reply.author?.[0] || '?' }}
                </el-avatar>
                <div class="reply-content">
                  <div class="reply-header">
                    <span class="reply-author">{{ reply.author }}</span>
                    <span class="reply-time">{{ formatTime(reply.createdAt) }}</span>
                  </div>
                  <div class="reply-text">{{ reply.text }}</div>
                </div>
              </div>
            </div>

            <!-- 回复输入 -->
            <div v-if="replyingTo === comment.id" class="reply-input">
              <el-input
                v-model="replyText"
                type="textarea"
                :rows="2"
                placeholder="输入回复..."
                @keyup.enter.exact="submitReply(comment)"
              />
              <div class="reply-actions">
                <el-button size="small" @click="cancelReply">取消</el-button>
                <el-button size="small" type="primary" @click="submitReply(comment)">
                  回复
                </el-button>
              </div>
            </div>

            <div v-else class="comment-actions">
              <el-button text size="small" @click="startReply(comment)">
                <el-icon><ChatDotSquare /></el-icon>
                回复
              </el-button>
            </div>
          </div>
        </div>

        <el-empty v-if="comments.length === 0" description="暂无评论" />
      </div>

      <!-- 添加评论表单 -->
      <div class="add-comment">
        <h4>添加评论</h4>
        <el-input
          v-model="newComment.text"
          type="textarea"
          :rows="3"
          placeholder="输入评论内容..."
        />
        <div v-if="quoteText" class="comment-quote-preview">
          <div class="quote-header">
            <span>引用内容:</span>
            <el-button text size="small" @click="clearQuote">
              <el-icon><Close /></el-icon>
            </el-button>
          </div>
          <blockquote>{{ quoteText }}</blockquote>
        </div>
        <div class="comment-form-actions">
          <el-checkbox v-model="newComment.resolved">标记为已解决</el-checkbox>
          <el-button type="primary" @click="addComment" :disabled="!newComment.text">
            <el-icon><Plus /></el-icon>
            添加评论
          </el-button>
        </div>
      </div>
    </div>

    <template #footer>
      <div class="footer-stats">
        <el-tag>总计: {{ comments.length }} 条</el-tag>
        <el-tag type="success">已解决: {{ resolvedCount }}</el-tag>
        <el-tag type="warning">待解决: {{ unresolvedCount }}</el-tag>
      </div>
    </template>
  </BaseDialog>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { MoreFilled, Position, ChatDotSquare, Close, Plus } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import BaseDialog from './BaseDialog.vue'

interface Comment {
  id: string | number
  author: string
  avatar?: string
  text: string
  quote?: string
  line?: number
  resolved: boolean
  createdAt: number
  replies?: Reply[]
}

interface Reply {
  id: string | number
  author: string
  avatar?: string
  text: string
  createdAt: number
}

interface Props {
  show: boolean
  comments: Comment[]
  currentUser?: string
  quoteText?: string
  quoteLine?: number
}

const props = withDefaults(defineProps<Props>(), {
  currentUser: 'User',
  quoteText: '',
  comments: () => []
})

const emit = defineEmits<{
  'update:show': [value: boolean]
  'add': [comment: Omit<Comment, 'id'>]
  'update': [commentId: string | number, updates: Partial<Comment>]
  'delete': [commentId: string | number]
  'reply': [commentId: string | number, reply: Omit<Reply, 'id'>]
  'navigate': [line: number]
}>()

// 状态
const replyingTo = ref<string | number | null>(null)
const replyText = ref('')
const newComment = ref({
  text: '',
  resolved: false
})

// 计算属性
const sortedComments = computed(() => {
  return [...props.comments].sort((a, b) => b.createdAt - a.createdAt)
})

const resolvedCount = computed(() => {
  return props.comments.filter(c => c.resolved).length
})

const unresolvedCount = computed(() => {
  return props.comments.filter(c => !c.resolved).length
})

// 方法
function formatTime(timestamp: number): string {
  const now = Date.now()
  const diff = now - timestamp

  if (diff < 60000) return '刚刚'
  if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`
  if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`
  return `${Math.floor(diff / 86400000)}天前`
}

function isOwnComment(comment: Comment): boolean {
  return comment.author === props.currentUser
}

function handleCommentAction(action: string, comment: Comment) {
  switch (action) {
    case 'reply':
      startReply(comment)
      break
    case 'resolve':
      updateComment(comment.id, { resolved: true })
      break
    case 'reopen':
      updateComment(comment.id, { resolved: false })
      break
    case 'delete':
      emit('delete', comment.id)
      break
  }
}

function startReply(comment: Comment) {
  replyingTo.value = comment.id
}

function cancelReply() {
  replyingTo.value = null
  replyText.value = ''
}

function submitReply(comment: Comment) {
  if (!replyText.value.trim()) {
    ElMessage.warning('请输入回复内容')
    return
  }

  emit('reply', comment.id, {
    author: props.currentUser,
    text: replyText.value,
    createdAt: Date.now()
  })

  replyText.value = ''
  replyingTo.value = null
  ElMessage.success('回复已发送')
}

function addComment() {
  if (!newComment.value.text.trim()) {
    ElMessage.warning('请输入评论内容')
    return
  }

  const comment: Omit<Comment, 'id'> = {
    author: props.currentUser,
    text: newComment.value.text,
    quote: props.quoteText,
    line: props.quoteLine,
    resolved: newComment.value.resolved,
    createdAt: Date.now()
  }

  emit('add', comment)
  newComment.value.text = ''
  clearQuote()
  ElMessage.success('评论已添加')
}

function updateComment(commentId: string | number, updates: Partial<Comment>) {
  emit('update', commentId, updates)
}

function clearQuote() {
  emit('update:show', false)
  setTimeout(() => emit('update:show', true), 0)
}
</script>

<style scoped lang="scss">
.comment-system {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.comments-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
  max-height: 500px;
  overflow-y: auto;
}

.comment-item {
  display: flex;
  gap: 12px;
  padding: 12px;
  background: var(--el-fill-color-blank);
  border-radius: 8px;
  border: 1px solid var(--el-border-color);

  &.is-resolved {
    opacity: 0.7;
  }
}

.comment-avatar {
  flex-shrink: 0;
}

.comment-content {
  flex: 1;
  min-width: 0;
}

.comment-header {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
}

.comment-author {
  font-weight: 500;
}

.comment-time {
  font-size: 12px;
  color: var(--el-text-color-secondary);
}

.comment-quote {
  position: relative;
  padding: 8px 12px;
  margin: 8px 0;
  background: var(--el-fill-color-light);
  border-left: 3px solid var(--el-color-primary);
  border-radius: 4px;

  blockquote {
    margin: 0;
    font-style: italic;
    color: var(--el-text-color-secondary);
  }

  .quote-location {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    margin-top: 4px;
    font-size: 12px;
    color: var(--el-color-primary);
    cursor: pointer;

    &:hover {
      text-decoration: underline;
    }
  }
}

.comment-text {
  line-height: 1.6;
}

.comment-replies {
  margin-top: 12px;
  padding-left: 12px;
  border-left: 2px solid var(--el-border-color);
}

.reply-item {
  display: flex;
  gap: 8px;
  margin-top: 8px;
}

.reply-content {
  flex: 1;
}

.reply-header {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 4px;
}

.reply-author {
  font-weight: 500;
  font-size: 13px;
}

.reply-time {
  font-size: 11px;
  color: var(--el-text-color-secondary);
}

.reply-text {
  font-size: 13px;
  line-height: 1.5;
}

.reply-input {
  margin-top: 8px;
}

.reply-actions {
  display: flex;
  gap: 8px;
  margin-top: 8px;
}

.comment-actions {
  margin-top: 8px;
}

.add-comment {
  padding: 16px;
  background: var(--el-fill-color-blank);
  border-radius: 8px;

  h4 {
    margin: 0 0 12px 0;
  }
}

.comment-quote-preview {
  margin-bottom: 12px;
  padding: 8px;
  background: var(--el-fill-color-light);
  border-radius: 4px;

  .quote-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 4px;
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }

  blockquote {
    margin: 0;
    font-style: italic;
    color: var(--el-text-color-secondary);
  }
}

.comment-form-actions {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-top: 12px;
}

.footer-stats {
  display: flex;
  gap: 8px;
  justify-content: center;
}
</style>
