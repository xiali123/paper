<template>
  <div class="document-card" @click="$emit('edit', document)">
    <div class="document-card__header">
      <el-tag :type="getDocTypeColor(document.document_type)" size="small">
        {{ getDocTypeName(document.document_type) }}
      </el-tag>
      <el-tag v-if="document.status" :type="getStatusColor(document.status)" size="small" effect="plain">
        {{ getStatusName(document.status) }}
      </el-tag>
    </div>

    <h3 class="document-card__title">{{ document.title || '无标题文档' }}</h3>

    <p class="document-card__preview">
      {{ getPreview(document.content) }}
    </p>

    <div class="document-card__meta">
      <span class="meta-item">
        <el-icon><Document /></el-icon>
        {{ document.word_count }} 字
      </span>
      <span class="meta-item">
        <el-icon><Clock /></el-icon>
        {{ formatTime(document.updated_at) }}
      </span>
    </div>

    <div class="document-card__actions" @click.stop>
      <el-button type="primary" size="small" :icon="Edit" @click="$emit('edit', document)">
        编辑
      </el-button>
      <el-button type="danger" size="small" :icon="Delete" @click="$emit('delete', document)">
        删除
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { Document, Edit, Delete, Clock } from '@element-plus/icons-vue'
import type { CollaborativeDocument } from '@/types/collaborative'

defineProps<{
  document: CollaborativeDocument
}>()

defineEmits<{
  edit: [doc: CollaborativeDocument]
  delete: [doc: CollaborativeDocument]
}>()

function getDocTypeName(type: string): string {
  const map: Record<string, string> = {
    paper: '论文',
    report: '报告',
    note: '笔记',
    thesis: '学位论文',
    other: '其他'
  }
  return map[type] || type
}

function getDocTypeColor(type: string): string {
  const map: Record<string, string> = {
    paper: 'primary',
    report: 'success',
    note: 'info',
    thesis: 'warning',
    other: ''
  }
  return map[type] || ''
}

function getStatusName(status: string): string {
  const map: Record<string, string> = {
    draft: '草稿',
    active: '活跃',
    published: '已发布',
    archived: '已归档'
  }
  return map[status] || status
}

function getStatusColor(status: string): string {
  const map: Record<string, string> = {
    draft: 'info',
    active: 'success',
    published: 'success',
    archived: 'warning'
  }
  return map[status] || ''
}

function getPreview(content: string): string {
  if (!content) return '暂无内容'
  return content.length > 100 ? content.substring(0, 100) + '...' : content
}

function formatTime(time: string): string {
  const date = new Date(time)
  const now = new Date()
  const diff = now.getTime() - date.getTime()
  const minutes = Math.floor(diff / 60000)
  const hours = Math.floor(diff / 3600000)
  const days = Math.floor(diff / 86400000)

  if (minutes < 1) return '刚刚'
  if (minutes < 60) return `${minutes} 分钟前`
  if (hours < 24) return `${hours} 小时前`
  if (days < 7) return `${days} 天前`
  return date.toLocaleDateString('zh-CN')
}
</script>

<style scoped lang="scss">
.document-card {
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  padding: 16px;
  cursor: pointer;
  transition: all 0.3s ease;

  &:hover {
    border-color: var(--el-color-primary);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
    transform: translateY(-2px);
  }
}

.document-card__header {
  display: flex;
  gap: 8px;
  margin-bottom: 12px;
}

.document-card__title {
  font-size: 16px;
  font-weight: 600;
  color: var(--el-text-color-primary);
  margin: 0 0 8px 0;
  overflow: hidden;
  text-overflow: ellipsis;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
}

.document-card__preview {
  font-size: 14px;
  color: var(--el-text-color-regular);
  line-height: 1.5;
  margin: 0 0 12px 0;
  min-height: 40px;
}

.document-card__meta {
  display: flex;
  gap: 16px;
  margin-bottom: 12px;

  .meta-item {
    display: flex;
    align-items: center;
    gap: 4px;
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }
}

.document-card__actions {
  display: flex;
  gap: 8px;
  padding-top: 8px;
  border-top: 1px solid var(--el-border-color-lighter);
}
</style>
