<script setup lang="ts">
/**
 * WebSocket 连接状态指示器组件
 * 显示实时连接状态、更新通知和统计信息
 */

import { computed, onMounted, onUnmounted, ref } from 'vue'
import { useWebSocketStore } from '../stores/websocket'

const wsStore = useWebSocketStore()

const showTooltip = ref(false)
const tooltipTimeout = ref<number | null>(null)

// 计算状态图标
const statusIcon = computed(() => {
  switch (wsStore.connectionState) {
    case 'connected':
      return 'mdi-wifi'
    case 'connecting':
    case 'reconnecting':
      return 'mdi-wifi-strength-1-alert'
    case 'disconnected':
      return 'mdi-wifi-off'
    case 'error':
      return 'mdi-wifi-strength-0'
    default:
      return 'mdi-help-network'
  }
})

// 状态颜色
const statusColor = computed(() => {
  switch (wsStore.connectionState) {
    case 'connected':
      return 'success'
    case 'connecting':
    case 'reconnecting':
      return 'warning'
    case 'disconnected':
    case 'error':
      return 'error'
    default:
      return 'default'
  }
})

// 显示工具提示
const showTooltipDelayed = () => {
  if (tooltipTimeout.value !== null) {
    clearTimeout(tooltipTimeout.value)
  }
  tooltipTimeout.value = window.setTimeout(() => {
    showTooltip.value = true
  }, 500)
}

// 隐藏工具提示
const hideTooltip = () => {
  if (tooltipTimeout.value !== null) {
    clearTimeout(tooltipTimeout.value)
    tooltipTimeout.value = null
  }
  showTooltip.value = false
}

// 点击重连
const handleClick = () => {
  if (wsStore.connectionState === 'disconnected' ||
      wsStore.connectionState === 'error') {
    wsStore.reconnect()
  }
}

// 组件挂载时初始化 WebSocket
onMounted(() => {
  wsStore.initialize()
})

onUnmounted(() => {
  if (tooltipTimeout.value !== null) {
    clearTimeout(tooltipTimeout.value)
  }
})
</script>

<template>
  <div
    class="websocket-status"
    @mouseenter="showTooltipDelayed"
    @mouseleave="hideTooltip"
    @click="handleClick"
  >
    <!-- 状态指示器 -->
    <div class="status-indicator">
      <v-icon :icon="statusIcon" :color="statusColor" size="small" />

      <!-- 更新通知徽章 -->
      <div
        v-if="wsStore.hasUpdates"
        class="notification-badge"
        @click.stop="wsStore.clearNotifications()"
      >
        {{ wsStore.updateNotifications }}
      </div>
    </div>

    <!-- 工具提示 -->
    <transition name="fade">
      <div v-if="showTooltip" class="status-tooltip">
        <div class="tooltip-header">
          <v-icon :icon="statusIcon" :color="statusColor" size="x-small" />
          <span class="status-text">{{ wsStore.connectionStatusText }}</span>
        </div>

        <div class="tooltip-content">
          <div class="tooltip-row">
            <span class="tooltip-label">Connection Time:</span>
            <span class="tooltip-value">{{ wsStore.getConnectionUptime() }}</span>
          </div>

          <div class="tooltip-row">
            <span class="tooltip-label">Messages:</span>
            <span class="tooltip-value">
              {{ wsStore.stats.messagesReceived }} received /
              {{ wsStore.stats.messagesSent }} sent
            </span>
          </div>

          <div v-if="wsStore.stats.reconnectCount > 0" class="tooltip-row">
            <span class="tooltip-label">Reconnections:</span>
            <span class="tooltip-value">{{ wsStore.stats.reconnectCount }}</span>
          </div>

          <div v-if="wsStore.lastUpdateTime" class="tooltip-row">
            <span class="tooltip-label">Last Update:</span>
            <span class="tooltip-value">
              {{ new Date(wsStore.lastUpdateTime).toLocaleTimeString() }}
            </span>
          </div>
        </div>

        <div v-if="wsStore.hasUpdates" class="tooltip-actions">
          <v-btn
            size="x-small"
            variant="text"
            @click.stop="wsStore.clearNotifications()"
          >
            Clear Notifications
          </v-btn>
        </div>
      </div>
    </transition>
  </div>
</template>

<style scoped>
.websocket-status {
  position: relative;
  display: inline-block;
  cursor: pointer;
}

.status-indicator {
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  border-radius: 50%;
  transition: all 0.3s ease;
}

.status-indicator:hover {
  background-color: rgba(0, 0, 0, 0.05);
}

.notification-badge {
  position: absolute;
  top: -4px;
  right: -4px;
  display: flex;
  align-items: center;
  justify-content: center;
  min-width: 18px;
  height: 18px;
  padding: 0 4px;
  background-color: #f44336;
  color: white;
  font-size: 10px;
  font-weight: bold;
  border-radius: 9px;
  border: 2px solid white;
  animation: pulse 2s infinite;
}

@keyframes pulse {
  0%, 100% {
    transform: scale(1);
  }
  50% {
    transform: scale(1.1);
  }
}

.status-tooltip {
  position: absolute;
  bottom: 100%;
  right: 0;
  margin-bottom: 8px;
  min-width: 200px;
  background-color: white;
  border-radius: 8px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.15);
  padding: 12px;
  z-index: 1000;
  pointer-events: none;
}

.tooltip-header {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
  padding-bottom: 8px;
  border-bottom: 1px solid #eee;
}

.status-text {
  font-weight: 600;
  font-size: 14px;
  color: #333;
}

.tooltip-content {
  margin-bottom: 8px;
}

.tooltip-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 4px;
  font-size: 12px;
}

.tooltip-label {
  color: #666;
  font-weight: 500;
}

.tooltip-value {
  color: #333;
  font-weight: 600;
}

.tooltip-actions {
  display: flex;
  justify-content: center;
  padding-top: 8px;
  border-top: 1px solid #eee;
  pointer-events: auto;
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.2s ease;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

/* 深色主题支持 */
@media (prefers-color-scheme: dark) {
  .status-tooltip {
    background-color: #2c2c2c;
    box-shadow: 0 2px 12px rgba(0, 0, 0, 0.3);
  }

  .status-text,
  .tooltip-value {
    color: #fff;
  }

  .tooltip-label {
    color: #aaa;
  }

  .tooltip-header,
  .tooltip-actions {
    border-color: #444;
  }

  .status-indicator:hover {
    background-color: rgba(255, 255, 255, 0.1);
  }
}
</style>
