<template>
  <el-card shadow="hover" class="task-progress-card">
    <div class="card-header">
      <div class="task-info">
        <h3 class="task-title">{{ task.query }}</h3>
        <div class="task-meta">
          <el-tag :type="getStatusType(task.status)" size="small">
            {{ getStatusText(task.status) }}
          </el-tag>
          <el-tag size="small" type="info">{{ task.source }}</el-tag>
        </div>
      </div>
      <div class="task-status-icon" :class="task.status">
        <el-icon v-if="task.status === 'running'"><Loading /></el-icon>
        <el-icon v-else-if="task.status === 'completed'"><CircleCheck /></el-icon>
        <el-icon v-else-if="task.status === 'failed'"><CircleClose /></el-icon>
        <el-icon v-else><Clock /></el-icon>
      </div>
    </div>

    <div class="card-body">
      <!-- Progress Bar -->
      <div class="progress-section">
        <el-progress
          :percentage="task.progress"
          :status="getProgressStatus(task.status)"
          :stroke-width="8"
        />
        <div class="progress-stats">
          <span>{{ task.completedPapers || 0 }} / {{ task.totalPapers || 0 }} 篇论文</span>
          <span v-if="task.status === 'running'" class="live-badge">
            <span class="dot"></span>
            实时更新
          </span>
        </div>
      </div>

      <!-- Task Details -->
      <div class="task-details">
        <div class="detail-item">
          <el-icon><Timer /></el-icon>
          <span>开始时间: {{ formatTime(task.startedAt) }}</span>
        </div>
        <div v-if="task.completedAt" class="detail-item">
          <el-icon><CircleCheck /></el-icon>
          <span>完成时间: {{ formatTime(task.completedAt) }}</span>
        </div>
        <div v-if="task.errorMessage" class="detail-item error">
          <el-icon><Warning /></el-icon>
          <span>{{ task.errorMessage }}</span>
        </div>
      </div>

      <!-- Action Buttons -->
      <div class="card-actions">
        <el-button
          v-if="task.status === 'running' || task.status === 'pending'"
          type="warning"
          size="small"
          :icon="VideoPause"
          @click="$emit('pause', task)"
        >
          暂停
        </el-button>
        <el-button
          v-if="task.status === 'pending'"
          type="primary"
          size="small"
          :icon="VideoPlay"
          @click="$emit('resume', task)"
        >
          继续
        </el-button>
        <el-button
          v-if="task.status !== 'completed' && task.status !== 'failed'"
          type="danger"
          size="small"
          :icon="CircleClose"
          @click="$emit('cancel', task)"
        >
          取消
        </el-button>
        <el-button
          v-if="task.status === 'completed'"
          type="success"
          size="small"
          :icon="View"
          @click="$emit('view', task)"
        >
          查看结果
        </el-button>
        <el-button
          v-if="task.status === 'failed'"
          type="primary"
          size="small"
          :icon="RefreshRight"
          @click="$emit('retry', task)"
        >
          重试
        </el-button>
      </div>
    </div>
  </el-card>
</template>

<script setup lang="ts">
import { type CrawlerTask } from '@/api/modules/crawler'
import {
  Loading,
  CircleCheck,
  CircleClose,
  Clock,
  Timer,
  Warning,
  VideoPlay,
  VideoPause,
  View,
  RefreshRight
} from '@element-plus/icons-vue'

defineProps<{
  task: CrawlerTask
}>()

defineEmits<{
  pause: [task: CrawlerTask]
  resume: [task: CrawlerTask]
  cancel: [task: CrawlerTask]
  view: [task: CrawlerTask]
  retry: [task: CrawlerTask]
}>()

const getStatusType = (status: string) => {
  const types: Record<string, any> = {
    running: 'warning',
    completed: 'success',
    failed: 'danger',
    pending: 'info',
    cancelled: 'info'
  }
  return types[status] || 'info'
}

const getStatusText = (status: string) => {
  const texts: Record<string, string> = {
    running: '运行中',
    completed: '已完成',
    failed: '失败',
    pending: '待处理',
    cancelled: '已取消'
  }
  return texts[status] || status
}

const getProgressStatus = (status: string) => {
  if (status === 'completed') return 'success'
  if (status === 'failed') return 'exception'
  return undefined
}

const formatTime = (timestamp?: string) => {
  if (!timestamp) return '-'
  const date = new Date(timestamp)
  return date.toLocaleString()
}
</script>

<style scoped lang="scss">
.task-progress-card {
  margin-bottom: 15px;
  transition: all 0.3s;

  &:hover {
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.12);
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    margin-bottom: 15px;

    .task-info {
      flex: 1;

      .task-title {
        margin: 0 0 8px 0;
        font-size: 16px;
        font-weight: bold;
        color: #303133;
      }

      .task-meta {
        display: flex;
        gap: 8px;
      }
    }

    .task-status-icon {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 20px;

      &.running {
        background: #fff7e6;
        color: #e6a23c;
        animation: spin 1s linear infinite;
      }

      &.completed {
        background: #f0f9ff;
        color: #67c23a;
      }

      &.failed {
        background: #fef0f0;
        color: #f56c6c;
      }

      &.pending {
        background: #f4f4f5;
        color: #909399;
      }
    }
  }

  .card-body {
    .progress-section {
      margin-bottom: 15px;

      .progress-stats {
        display: flex;
        justify-content: space-between;
        margin-top: 8px;
        font-size: 12px;
        color: #909399;

        .live-badge {
          display: flex;
          align-items: center;
          gap: 5px;
          color: #67c23a;

          .dot {
            width: 6px;
            height: 6px;
            background: #67c23a;
            border-radius: 50%;
            animation: pulse 1.5s infinite;
          }
        }
      }
    }

    .task-details {
      margin-bottom: 15px;

      .detail-item {
        display: flex;
        align-items: center;
        gap: 6px;
        margin-bottom: 6px;
        font-size: 12px;
        color: #606266;

        &.error {
          color: #f56c6c;
        }
      }
    }

    .card-actions {
      display: flex;
      gap: 8px;
      flex-wrap: wrap;
    }
  }
}

@keyframes spin {
  from {
    transform: rotate(0deg);
  }
  to {
    transform: rotate(360deg);
  }
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.5;
  }
}
</style>
