<template>
  <div class="paper-card" :class="{ selected, 'is-read': paper.isRead }">
    <!-- 选择框 -->
    <div class="card-checkbox">
      <el-checkbox :model-value="selected" @change="$emit('select', paper.id)" />
    </div>

    <!-- 卡片内容 -->
    <div class="card-content" @click="handleView">
      <!-- 收藏图标 -->
      <div class="bookmark-icon" @click.stop="handleToggleBookmark">
        <el-icon v-if="paper.isBookmarked" color="#f56c6c" :size="20">
          <StarFilled />
        </el-icon>
        <el-icon v-else color="#909399" :size="20">
          <Star />
        </el-icon>
      </div>

      <!-- 论文标题 -->
      <h3 class="paper-title" :title="paper.title">
        {{ paper.title }}
      </h3>

      <!-- 作者 -->
      <div class="paper-authors" v-if="paper.authors">
        <el-icon><User /></el-icon>
        <span>{{ truncateText(paper.authors, 50) }}</span>
      </div>

      <!-- 发表信息 -->
      <div class="paper-info" v-if="paper.publication || paper.year">
        <el-icon><Document /></el-icon>
        <span>{{ paper.publication }}{{ paper.year ? ` (${paper.year})` : '' }}</span>
      </div>

      <!-- 摘要预览 -->
      <div class="paper-abstract" v-if="paper.abstract">
        {{ truncateText(paper.abstract, 120) }}
      </div>

      <!-- 标签 -->
      <div class="paper-tags" v-if="paper.tags || paper.category">
        <el-tag v-if="paper.category" size="small" type="primary">
          {{ paper.category }}
        </el-tag>
        <el-tag
          v-for="(tag, index) in parseTags(paper.tags)"
          :key="index"
          size="small"
        >
          {{ tag }}
        </el-tag>
        <el-tag v-if="paper.source" size="small" type="info">
          {{ sourceLabels[paper.source] || paper.source }}
        </el-tag>
      </div>

      <!-- 阅读进度 -->
      <div class="reading-progress" v-if="paper.readingProgress > 0">
        <el-progress
          :percentage="paper.readingProgress"
          :stroke-width="4"
          :show-text="false"
        />
        <span class="progress-text">{{ paper.readingProgress }}%</span>
      </div>
    </div>

    <!-- 操作按钮 -->
    <div class="card-actions">
      <el-button-group>
        <el-tooltip :content="$t('papers.markAsRead') || '标记已读'" placement="top">
          <el-button
            :type="paper.isRead ? 'success' : 'default'"
            :icon="paper.isRead ? 'View' : 'Hide'"
            size="small"
            @click="handleToggleRead"
          />
        </el-tooltip>

        <el-tooltip :content="$t('papers.edit') || '编辑'" placement="top">
          <el-button
            type="primary"
            :icon="Edit"
            size="small"
            @click="handleEdit"
          />
        </el-tooltip>

        <el-tooltip :content="$t('papers.delete') || '删除'" placement="top">
          <el-button
            type="danger"
            :icon="Delete"
            size="small"
            @click="handleDelete"
          />
        </el-tooltip>
      </el-button-group>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { Paper } from '@/api/modules/papers'
import { Star, StarFilled, User, Document, Edit, Delete, View, Hide } from '@element-plus/icons-vue'

interface Props {
  paper: Paper
  selected?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  selected: false
})

const emit = defineEmits<{
  select: [id: number]
  view: [paper: Paper]
  edit: [paper: Paper]
  delete: [paper: Paper]
  'toggle-bookmark': [paper: Paper]
  'toggle-read': [paper: Paper]
}>()

// 来源标签映射
const sourceLabels: Record<string, string> = {
  manual: '手动添加',
  cnki: '知网',
  ieee: 'IEEE',
  arxiv: 'ArXiv',
  pubmed: 'PubMed'
}

// 截断文本
function truncateText(text: string, maxLength: number): string {
  if (!text) return ''
  return text.length > maxLength ? text.substring(0, maxLength) + '...' : text
}

// 解析标签
function parseTags(tags: string): string[] {
  if (!tags) return []
  return tags.split(',').map(tag => tag.trim()).filter(tag => tag)
}

// 查看详情
function handleView() {
  emit('view', props.paper)
}

// 编辑
function handleEdit() {
  emit('edit', props.paper)
}

// 删除
function handleDelete() {
  emit('delete', props.paper)
}

// 切换收藏
function handleToggleBookmark() {
  emit('toggle-bookmark', props.paper)
}

// 切换已读
function handleToggleRead() {
  emit('toggle-read', props.paper)
}
</script>

<style scoped lang="scss">
.paper-card {
  position: relative;
  background: white;
  border: 2px solid #e4e7ed;
  border-radius: 8px;
  padding: 16px;
  margin-bottom: 16px;
  cursor: pointer;
  transition: all 0.3s;

  &:hover {
    border-color: #409eff;
    box-shadow: 0 4px 12px rgba(64, 158, 255, 0.2);
    transform: translateY(-2px);
  }

  &.selected {
    border-color: #409eff;
    background: #ecf5ff;
  }

  &.is-read {
    opacity: 0.7;
  }
}

.card-checkbox {
  position: absolute;
  top: 12px;
  left: 12px;
  z-index: 2;
}

.bookmark-icon {
  position: absolute;
  top: 12px;
  right: 12px;
  cursor: pointer;
  transition: transform 0.2s;

  &:hover {
    transform: scale(1.2);
  }
}

.card-content {
  padding: 8px 12px;
}

.paper-title {
  margin: 0 0 12px 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
  line-height: 1.4;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
  text-overflow: ellipsis;
  min-height: 44px;
}

.paper-authors,
.paper-info {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 8px;
  font-size: 13px;
  color: #606266;

  .el-icon {
    flex-shrink: 0;
  }

  span {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
}

.paper-abstract {
  margin-bottom: 12px;
  font-size: 13px;
  color: #909399;
  line-height: 1.5;
  display: -webkit-box;
  -webkit-line-clamp: 3;
  -webkit-box-orient: vertical;
  overflow: hidden;
  text-overflow: ellipsis;
}

.paper-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  margin-bottom: 12px;
}

.reading-progress {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 8px;

  .el-progress {
    flex: 1;
  }

  .progress-text {
    font-size: 12px;
    color: #909399;
    min-width: 40px;
    text-align: right;
  }
}

.card-actions {
  display: flex;
  justify-content: flex-end;
  padding-top: 12px;
  border-top: 1px solid #e4e7ed;
}

@media (max-width: 768px) {
  .paper-card {
    padding: 12px;
  }

  .paper-title {
    font-size: 14px;
  }

  .paper-authors,
  .paper-info {
    font-size: 12px;
  }

  .paper-abstract {
    font-size: 12px;
  }
}
</style>
