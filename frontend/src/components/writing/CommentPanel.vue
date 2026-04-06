<template>
  <div class="comment-panel">
    <div class="panel-header">
      <h4 class="panel-title">评论 ({{ comments.length }})</h4>
    </div>

    <div v-if="loading && comments.length === 0" class="panel-loading">
      <el-skeleton :rows="3" animated />
    </div>

    <div v-else class="comment-content">
      <!-- 评论列表 -->
      <div class="comment-list">
        <div
          v-for="comment in comments"
          :key="comment.id"
          class="comment-item"
          :class="{ 'comment-item--resolved': comment.is_resolved }"
        >
          <div class="comment-header">
            <span class="comment-author">用户 {{ comment.user_id }}</span>
            <span class="comment-time">{{ formatTime(comment.created_at) }}</span>
          </div>
          <div class="comment-text">{{ comment.content }}</div>
          <div v-if="!comment.is_resolved" class="comment-actions">
            <el-button size="small" type="success" link @click="$emit('resolve', comment.id)">
              解决
            </el-button>
          </div>
          <div v-else class="comment-resolved">
            <el-tag size="small" type="success">已解决</el-tag>
          </div>
        </div>
      </div>

      <!-- 添加评论 -->
      <div class="comment-input">
        <el-input
          v-model="newComment"
          type="textarea"
          :rows="2"
          placeholder="添加评论..."
          maxlength="500"
          show-word-limit
        />
        <el-button
          type="primary"
          size="small"
          :disabled="!newComment.trim()"
          :loading="submitting"
          @click="handleSubmit"
        >
          发送
        </el-button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import type { DocumentComment } from '@/types/collaborative'

defineProps<{
  comments: DocumentComment[]
  loading?: boolean
}>()

const emit = defineEmits<{
  add: [data: { content: string }]
  resolve: [id: number]
}>()

const newComment = ref('')
const submitting = ref(false)

async function handleSubmit() {
  if (!newComment.value.trim()) return

  submitting.value = true
  try {
    emit('add', { content: newComment.value })
    newComment.value = ''
  } finally {
    submitting.value = false
  }
}

function formatTime(time: string): string {
  const date = new Date(time)
  const now = new Date()
  const diff = now.getTime() - date.getTime()
  const minutes = Math.floor(diff / 60000)

  if (minutes < 1) return '刚刚'
  if (minutes < 60) return `${minutes}分钟前`
  if (minutes < 1440) return `${Math.floor(minutes / 60)}小时前`
  return date.toLocaleDateString('zh-CN')
}
</script>

<style scoped lang="scss">
.comment-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-header {
  padding: 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);
}

.panel-title {
  margin: 0;
  font-size: 14px;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.panel-loading {
  padding: 20px;
}

.comment-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.comment-list {
  flex: 1;
  overflow-y: auto;
  padding: 12px;
}

.comment-item {
  padding: 10px;
  margin-bottom: 8px;
  background: var(--el-fill-color-light);
  border-radius: 6px;

  &--resolved {
    opacity: 0.6;
  }
}

.comment-header {
  display: flex;
  justify-content: space-between;
  margin-bottom: 4px;
}

.comment-author {
  font-weight: 500;
  font-size: 13px;
  color: var(--el-text-color-primary);
}

.comment-time {
  font-size: 12px;
  color: var(--el-text-color-secondary);
}

.comment-text {
  font-size: 13px;
  color: var(--el-text-color-regular);
  line-height: 1.4;
  margin-bottom: 4px;
  word-break: break-word;
}

.comment-actions {
  margin-top: 4px;
}

.comment-resolved {
  margin-top: 4px;
}

.comment-input {
  padding: 12px;
  border-top: 1px solid var(--el-border-color-lighter);
  display: flex;
  flex-direction: column;
  gap: 8px;
}
</style>
