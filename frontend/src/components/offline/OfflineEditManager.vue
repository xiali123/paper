<template>
  <div class="offline-manager">
    <!-- 状态头部 -->
    <div class="status-header">
      <div class="status-indicator" :class="{ 'is-online': isOnline }">
        <div class="status-dot"></div>
        <span class="status-text">
          {{ isOnline ? '在线' : '离线' }}
        </span>
      </div>
      <div class="status-actions">
        <el-button
          v-if="offlineQueueSize > 0"
          size="small"
          type="primary"
          :loading="syncing"
          @click="handleSync"
        >
          <el-icon><Refresh /></el-icon>
          同步 ({{ offlineQueueSize }})
        </el-button>
      </div>
    </div>

    <!-- 离线统计 -->
    <div class="offline-stats">
      <div class="stat-card">
        <div class="stat-icon" :style="{ backgroundColor: isOnline ? '#67c23a' : '#f56c6c' }">
          <el-icon v-if="isOnline"><CircleCheckFilled /></el-icon>
          <el-icon v-else><CircleCloseFilled /></el-icon>
        </div>
        <div class="stat-content">
          <div class="stat-value">{{ isOnline ? '已连接' : '已断开' }}</div>
          <div class="stat-label">网络状态</div>
        </div>
      </div>

      <div class="stat-card">
        <div class="stat-icon" style="background: #e6a23c;">
          <el-icon><Clock /></el-icon>
        </div>
        <div class="stat-content">
          <div class="stat-value">{{ offlineQueueSize }}</div>
          <div class="stat-label">待同步操作</div>
        </div>
      </div>

      <div class="stat-card">
        <div class="stat-icon" style="background: #409eff;">
          <el-icon><Document /></el-icon>
        </div>
        <div class="stat-content">
          <div class="stat-value">{{ cachedFiles }}</div>
          <div class="stat-label">已缓存文件</div>
        </div>
      </div>
    </div>

    <!-- 离线操作列表 -->
    <div v-if="offlineQueueSize > 0" class="offline-queue">
      <div class="queue-header">
        <h4>离线操作队列</h4>
        <el-button size="small" text @click="handleClearQueue">
          清空队列
        </el-button>
      </div>
      <div class="queue-list">
        <div class="queue-item">
          <el-icon class="queue-icon"><Clock /></el-icon>
          <div class="queue-info">
            <div class="queue-title">文档修改</div>
            <div class="queue-time">等待同步...</div>
          </div>
          <el-tag size="small" type="warning">待同步</el-tag>
        </div>
      </div>
    </div>

    <!-- 离线编辑提示 -->
    <el-alert
      v-if="!isOnline"
      type="warning"
      :closable="false"
      show-icon
      class="offline-alert"
    >
      <template #title>
        当前处于离线模式
      </template>
      <template #default>
        <p>您的编辑操作将自动保存，待网络恢复后同步。</p>
        <p>可以继续编辑LaTeX文档，所有更改都保存在本地。</p>
      </template>
    </el-alert>

    <!-- Service Worker状态 -->
    <div class="sw-status">
      <div class="status-item">
        <span class="status-label">Service Worker:</span>
        <el-tag :type="isSWReady ? 'success' : 'danger'" size="small">
          {{ isSWReady ? '已启用' : '未启用' }}
        </el-tag>
      </div>
      <div v-if="updateAvailable" class="status-item">
        <el-button type="primary" size="small" @click="handleUpdate">
          发现新版本，点击更新
        </el-button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import {
  Refresh,
  CircleCheckFilled,
  CircleCloseFilled,
  Clock,
  Document
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { useServiceWorker } from '@/composables/useServiceWorker'

const {
  isOnline,
  isSWReady,
  offlineQueueSize,
  updateAvailable,
  syncOfflineQueue,
  clearOfflineQueue,
  skipWaiting
} = useServiceWorker()

const syncing = ref(false)
const cachedFiles = ref(42) // 模拟缓存文件数

// 同步离线队列
const handleSync = async () => {
  if (!isOnline.value) {
    ElMessage.warning('网络未连接，无法同步')
    return
  }

  syncing.value = true
  try {
    await syncOfflineQueue()
    ElMessage.success('同步已完成')
  } catch (error) {
    ElMessage.error('同步失败，将在稍后重试')
  } finally {
    syncing.value = false
  }
}

// 清空队列
const handleClearQueue = async () => {
  try {
    await ElMessageBox.confirm(
      '确定要清空离线操作队列吗？未同步的更改将丢失。',
      '确认操作',
      {
        type: 'warning',
        confirmButtonText: '清空',
        cancelButtonText: '取消'
      }
    )

    clearOfflineQueue()
    ElMessage.success('队列已清空')
  } catch {
    // 用户取消
  }
}

// 更新Service Worker
const handleUpdate = () => {
  ElMessageBox.confirm(
    '更新后将刷新页面，确定继续吗？',
    '确认更新',
    {
      confirmButtonText: '更新',
      cancelButtonText: '取消',
      type: 'info'
    }
  ).then(() => {
    skipWaiting()
  }).catch(() => {
    // 用户取消
  })
}
</script>

<style scoped lang="scss">
.offline-manager {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #f5f7fa;

  .status-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 16px;
    background: #fff;
    border-bottom: 1px solid #e4e7ed;

    .status-indicator {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 6px 12px;
      border-radius: 20px;
      background: #fef0f0;

      &.is-online {
        background: #f0f9ff;
      }

      .status-dot {
        width: 8px;
        height: 8px;
        border-radius: 50%;
        background: #f56c6c;

        .is-online & {
          background: #67c23a;
        }
      }

      .status-text {
        font-size: 14px;
        font-weight: 500;
        color: #303133;
      }
    }
  }

  .offline-stats {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 12px;
    padding: 16px;

    .stat-card {
      background: #fff;
      border-radius: 8px;
      padding: 12px;
      display: flex;
      align-items: center;
      gap: 12px;

      .stat-icon {
        width: 40px;
        height: 40px;
        border-radius: 8px;
        display: flex;
        align-items: center;
        justify-content: center;
        color: #fff;
        font-size: 20px;
      }

      .stat-content {
        .stat-value {
          font-size: 16px;
          font-weight: 600;
          color: #303133;
        }

        .stat-label {
          font-size: 12px;
          color: #909399;
          margin-top: 2px;
        }
      }
    }
  }

  .offline-queue {
    background: #fff;
    margin: 0 16px;
    border-radius: 8px;
    padding: 12px;

    .queue-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 12px;

      h4 {
        margin: 0;
        font-size: 14px;
        font-weight: 600;
        color: #303133;
      }
    }

    .queue-list {
      .queue-item {
        display: flex;
        align-items: center;
        gap: 12px;
        padding: 10px;
        border-radius: 6px;
        background: #f5f7fa;

        .queue-icon {
          font-size: 20px;
          color: #e6a23c;
        }

        .queue-info {
          flex: 1;

          .queue-title {
            font-size: 14px;
            font-weight: 500;
            color: #303133;
          }

          .queue-time {
            font-size: 12px;
            color: #909399;
            margin-top: 2px;
          }
        }
      }
    }
  }

  .offline-alert {
    margin: 16px;

    p {
      margin: 4px 0;
    }
  }

  .sw-status {
    padding: 16px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    background: #fff;
    border-top: 1px solid #e4e7ed;

    .status-item {
      display: flex;
      align-items: center;
      gap: 8px;

      .status-label {
        font-size: 12px;
        color: #909399;
      }
    }
  }
}
</style>
