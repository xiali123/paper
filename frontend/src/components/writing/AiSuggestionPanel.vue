<template>
  <div class="ai-suggestion-panel">
    <div class="panel-header">
      <h4 class="panel-title">AI 写作建议</h4>
      <el-button
        type="primary"
        size="small"
        :icon="MagicStick"
        :loading="loading"
        @click="handleGenerate"
      >
        生成建议
      </el-button>
    </div>

    <div v-if="loading && suggestions.length === 0" class="panel-loading">
      <el-skeleton :rows="3" animated />
    </div>

    <div v-else-if="suggestions.length === 0" class="panel-empty">
      <el-empty description="暂无建议" :image-size="60">
        <el-button type="primary" :icon="MagicStick" @click="handleGenerate">
          生成 AI 建议
        </el-button>
      </el-empty>
    </div>

    <div v-else class="suggestion-list">
      <div
        v-for="suggestion in suggestions"
        :key="suggestion.id"
        class="suggestion-item"
        :class="{ 'suggestion-item--accepted': suggestion.status === 'accepted', 'suggestion-item--rejected': suggestion.status === 'rejected' }"
      >
        <div class="suggestion-header">
          <el-tag :type="getSuggestionTypeColor(suggestion.suggestion_type)" size="small">
            {{ getSuggestionTypeName(suggestion.suggestion_type) }}
          </el-tag>
          <span class="confidence">置信度: {{ Math.round(suggestion.confidence_score * 100) }}%</span>
        </div>

        <div class="suggestion-content">
          <div class="original-text">
            <span class="label">原文:</span>
            <del>{{ suggestion.original_text }}</del>
          </div>
          <div class="suggested-text">
            <span class="label">建议:</span>
            <span>{{ suggestion.suggested_text }}</span>
          </div>
        </div>

        <div v-if="suggestion.explanation" class="suggestion-explanation">
          <el-icon><InfoFilled /></el-icon>
          {{ suggestion.explanation }}
        </div>

        <div v-if="suggestion.status === 'pending'" class="suggestion-actions">
          <el-button size="small" type="success" :icon="Check" @click="$emit('accept', suggestion.id)">
            接受
          </el-button>
          <el-button size="small" :icon="Close" @click="$emit('reject', suggestion.id)">
            忽略
          </el-button>
        </div>
        <div v-else class="suggestion-status">
          <el-tag v-if="suggestion.status === 'accepted'" type="success" size="small">已接受</el-tag>
          <el-tag v-else-if="suggestion.status === 'rejected'" type="info" size="small">已忽略</el-tag>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { MagicStick, Check, Close, InfoFilled } from '@element-plus/icons-vue'
import type { WritingSuggestion } from '@/types/collaborative'

defineProps<{
  suggestions: WritingSuggestion[]
  loading?: boolean
}>()

const emit = defineEmits<{
  accept: [id: number]
  reject: [id: number]
  generate: []
}>()

function handleGenerate() {
  emit('generate')
}

function getSuggestionTypeName(type: string): string {
  const map: Record<string, string> = {
    grammar: '语法',
    style: '风格',
    structure: '结构',
    citation: '引用',
    content: '内容'
  }
  return map[type] || type
}

function getSuggestionTypeColor(type: string): string {
  const map: Record<string, string> = {
    grammar: 'danger',
    style: 'warning',
    structure: 'primary',
    citation: 'info',
    content: 'success'
  }
  return map[type] || ''
}
</script>

<style scoped lang="scss">
.ai-suggestion-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);
}

.panel-title {
  margin: 0;
  font-size: 14px;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.panel-loading,
.panel-empty {
  padding: 20px;
}

.suggestion-list {
  flex: 1;
  overflow-y: auto;
  padding: 12px;
}

.suggestion-item {
  background: var(--el-fill-color-light);
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 8px;
  padding: 12px;
  margin-bottom: 12px;

  &--accepted {
    border-color: var(--el-color-success);
    opacity: 0.7;
  }

  &--rejected {
    opacity: 0.5;
  }
}

.suggestion-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;

  .confidence {
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }
}

.suggestion-content {
  margin-bottom: 8px;

  .original-text,
  .suggested-text {
    display: flex;
    gap: 8px;
    margin-bottom: 4px;
    font-size: 13px;

    .label {
      font-weight: 500;
      color: var(--el-text-color-secondary);
      flex-shrink: 0;
    }
  }

  .original-text del {
    color: var(--el-text-color-secondary);
  }

  .suggested-text {
    color: var(--el-color-success);
  }
}

.suggestion-explanation {
  display: flex;
  gap: 6px;
    font-size: 12px;
    color: var(--el-text-color-secondary);
    margin-bottom: 8px;
    line-height: 1.4;
  }
}

.suggestion-actions {
  display: flex;
  gap: 8px;
}

.suggestion-status {
  margin-top: 4px;
}
</style>
