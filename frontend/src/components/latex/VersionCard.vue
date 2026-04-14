<template>
  <div
    class="version-card"
    :class="{
      selected,
      comparing,
      'is-auto-save': version.isAutoSave,
      'is-merged': version.isMerged
    }"
    @click="handleClick"
  >
    <!-- 卡片头部 -->
    <div class="card-header">
      <div class="version-badges">
        <el-tag v-if="version.isAutoSave" size="small" type="info">自动保存</el-tag>
        <el-tag v-if="version.isMerged" size="small" type="success">已合并</el-tag>
        <el-tag v-if="isMainBranch" size="small" type="primary">主分支</el-tag>
        <el-tag v-else size="small">{{ version.branchName }}</el-tag>
      </div>
      <el-checkbox
        :model-value="comparing"
        @change="handleToggleCompare"
        @click.stop
      />
    </div>

    <!-- 卡片内容 -->
    <div class="card-content">
      <!-- 版本摘要 -->
      <div class="version-summary">
        <el-icon class="summary-icon"><Document /></el-icon>
        <span class="summary-text">{{ version.summary || '未命名版本' }}</span>
      </div>

      <!-- 元信息 -->
      <div class="version-meta">
        <div class="meta-item">
          <el-icon><Clock /></el-icon>
          <span>{{ formatTime(version.timestamp) }}</span>
        </div>
        <div class="meta-item">
          <el-icon><User /></el-icon>
          <span>{{ version.author }}</span>
        </div>
      </div>

      <!-- 统计信息 -->
      <div class="version-stats">
        <div class="stat-item">
          <span class="stat-label">行数</span>
          <span class="stat-value">{{ version.totalLines }}</span>
        </div>
        <div class="stat-item">
          <span class="stat-label">变更</span>
          <span class="stat-value">+{{ version.changeCount }}</span>
        </div>
        <div class="stat-item">
          <span class="stat-label">位置</span>
          <span class="stat-value">v{{ version.position }}</span>
        </div>
      </div>

      <!-- 进度条指示器（如果有父子关系） -->
      <div v-if="version.depth > 0" class="depth-indicator">
        <el-progress
          :percentage="((version.position / (version.position + version.depth)) * 100)"
          :show-text="false"
          :stroke-width="2"
        />
        <span class="depth-text">深度: {{ version.depth }}</span>
      </div>
    </div>

    <!-- 卡片底部操作 -->
    <div class="card-footer">
      <el-button-group size="small">
        <el-button @click.stop="handlePreview">
          <el-icon><View /></el-icon>
          预览
        </el-button>
        <el-button @click.stop="handleCompare" :disabled="!canCompare">
          <el-icon><CopyDocument /></el-icon>
          对比
        </el-button>
        <el-button
          @click.stop="handleRestore"
          type="warning"
          :icon="RefreshLeft"
        >
          恢复
        </el-button>
      </el-button-group>
    </div>

    <!-- 选中边框 -->
    <div v-if="selected" class="selected-border"></div>

    <!-- 对比中边框 -->
    <div v-if="comparing" class="comparing-border"></div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import {
  Document,
  Clock,
  User,
  View,
  CopyDocument,
  RefreshLeft
} from '@element-plus/icons-vue'
import type { FrontendLatexVersionNode } from '@/api/adapters/latexAdapter'

const props = defineProps<{
  version: FrontendLatexVersionNode
  selected?: boolean
  comparing?: boolean
}>()

const emit = defineEmits<{
  (e: 'select', version: FrontendLatexVersionNode): void
  (e: 'preview', version: FrontendLatexVersionNode): void
  (e: 'compare', version: FrontendLatexVersionNode): void
  (e: 'restore', version: FrontendLatexVersionNode): void
  (e: 'toggleCompare', version: FrontendLatexVersionNode): void
}>()

// 计算属性
const isMainBranch = computed(() => props.version.branchName === 'main')

const canCompare = computed(() => {
  return !props.version.isAutoSave
})

// 格式化时间
const formatTime = (timestamp: Date) => {
  const date = new Date(timestamp)
  const now = new Date()
  const diffMs = now.getTime() - date.getTime()
  const diffMins = Math.floor(diffMs / 60000)
  const diffHours = Math.floor(diffMs / 3600000)
  const diffDays = Math.floor(diffMs / 86400000)

  if (diffMins < 1) return '刚刚'
  if (diffMins < 60) return `${diffMins}分钟前`
  if (diffHours < 24) return `${diffHours}小时前`
  if (diffDays < 7) return `${diffDays}天前`

  return date.toLocaleDateString('zh-CN')
}

// 事件处理
const handleClick = () => {
  emit('select', props.version)
}

const handlePreview = () => {
  emit('preview', props.version)
}

const handleCompare = () => {
  emit('compare', props.version)
}

const handleRestore = () => {
  emit('restore', props.version)
}

const handleToggleCompare = () => {
  emit('toggleCompare', props.version)
}
</script>

<style scoped lang="scss">
.version-card {
  position: relative;
  background: #fff;
  border: 1px solid #e4e7ed;
  border-radius: 8px;
  padding: 16px;
  cursor: pointer;
  transition: all 0.3s;
  overflow: hidden;

  &:hover {
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
    transform: translateY(-2px);
  }

  &.selected {
    border-color: #409EFF;
    background: #f0f7ff;
  }

  &.comparing {
    border-color: #E6A23C;
    background: #fdf6ec;
  }

  &.is-auto-save {
    opacity: 0.8;
    border-style: dashed;
  }

  &.is-merged {
    border-color: #67C23A;
  }

  .selected-border {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 3px;
    background: #409EFF;
  }

  .comparing-border {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 3px;
    background: #E6A23C;
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 12px;

    .version-badges {
      display: flex;
      gap: 4px;
      flex-wrap: wrap;
    }
  }

  .card-content {
    .version-summary {
      display: flex;
      align-items: flex-start;
      gap: 8px;
      margin-bottom: 12px;

      .summary-icon {
        color: #909399;
        margin-top: 2px;
      }

      .summary-text {
        flex: 1;
        font-weight: 500;
        color: #303133;
        line-height: 1.5;
      }
    }

    .version-meta {
      display: flex;
      flex-direction: column;
      gap: 6px;
      margin-bottom: 12px;
      padding: 8px 0;
      border-top: 1px solid #f5f7fa;
      border-bottom: 1px solid #f5f7fa;

      .meta-item {
        display: flex;
        align-items: center;
        gap: 6px;
        font-size: 12px;
        color: #606266;

        .el-icon {
          font-size: 14px;
          color: #909399;
        }
      }
    }

    .version-stats {
      display: flex;
      justify-content: space-around;
      margin-bottom: 12px;

      .stat-item {
        text-align: center;

        .stat-label {
          display: block;
          font-size: 11px;
          color: #909399;
          margin-bottom: 2px;
        }

        .stat-value {
          font-size: 14px;
          font-weight: 600;
          color: #303133;
        }
      }
    }

    .depth-indicator {
      margin-top: 8px;

      .depth-text {
        display: block;
        font-size: 11px;
        color: #909399;
        text-align: center;
        margin-top: 4px;
      }
    }
  }

  .card-footer {
    margin-top: 12px;
    padding-top: 12px;
    border-top: 1px solid #f5f7fa;

    .el-button-group {
      width: 100%;

      .el-button {
        flex: 1;
      }
    }
  }
}
</style>
