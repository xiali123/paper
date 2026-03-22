<script setup lang="ts">
/**
 * 实时论文列表组件
 * 展示 WebSocket 实时更新功能
 */

import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
import { usePapersStore } from '@/stores/papers'
import { useWebSocketStore } from '@/stores/websocket'

const props = defineProps<{
  autoRefresh?: boolean
  showNotifications?: boolean
}>()

const papersStore = usePapersStore()
const wsStore = useWebSocketStore()

// UI 状态
const showUpdateBanner = ref(false)
const updatedPapers = ref<Set<string>>(new Set())
const notificationTimeout = ref<number | null>(null)

// 计算属性
const connectionStatus = computed(() => {
  if (!wsStore.isConnected) return 'Offline'
  if (papersStore.hasRealtimeUpdates) return 'Live'
  return 'Connected'
})

const connectionColor = computed(() => {
  switch (connectionStatus.value) {
    case 'Live': return 'success'
    case 'Connected': return 'info'
    case 'Offline': return 'error'
    default: return 'default'
  }
})

// 监听论文更新事件
const handlePaperUpdate = (event: CustomEvent) => {
  const paper = event.detail
  updatedPapers.value.add(String(paper.id))

  // 显示更新横幅
  if (props.showNotifications) {
    showUpdateBanner.value = true

    // 自动隐藏横幅
    if (notificationTimeout.value) {
      clearTimeout(notificationTimeout.value)
    }
    notificationTimeout.value = window.setTimeout(() => {
      showUpdateBanner.value = false
    }, 5000)
  }

  // 3秒后移除更新标记
  setTimeout(() => {
    updatedPapers.value.delete(String(paper.id))
  }, 3000)
}

// 生命周期
onMounted(() => {
  // 初始化 WebSocket 连接
  wsStore.initialize()

  // 设置论文数据监听
  papersStore.setupWebSocketListeners()

  // 监听自定义事件
  window.addEventListener('paper-update', handlePaperUpdate as EventListener)
})

onUnmounted(() => {
  // 清理监听器
  papersStore.cleanupWebSocketListeners()
  window.removeEventListener('paper-update', handlePaperUpdate as EventListener)

  if (notificationTimeout.value) {
    clearTimeout(notificationTimeout.value)
  }
})

// 监听搜索结果变化
watch(() => papersStore.searchResults, (newResults) => {
  if (newResults.length > 0 && papersStore.hasRealtimeUpdates) {
    // 有实时更新时刷新列表
  }
}, { deep: true })

// 方法
const refreshData = () => {
  // 清除更新标记
  updatedPapers.value.clear()
  showUpdateBanner.value = false

  // 重新加载数据
  if (papersStore.currentQuery.q) {
    papersStore.searchPapers(papersStore.currentQuery)
  }
}

const getPaperRowClass = (paperId: string) => {
  return updatedPapers.value.has(paperId) ? 'recently-updated' : ''
}
</script>

<template>
  <div class="real-time-paper-list">
    <!-- 连接状态栏 -->
    <div class="status-bar">
      <div class="connection-indicator">
        <v-chip
          :color="connectionColor"
          size="small"
          variant="outlined"
        >
          <v-icon start size="x-small">
            {{ wsStore.isConnected ? 'mdi-wifi' : 'mdi-wifi-off' }}
          </v-icon>
          {{ connectionStatus }}
        </v-chip>

        <span class="stats">
          {{ papersStore.searchTotal }} papers |
          {{ wsStore.stats.messagesReceived }} updates
        </span>
      </div>

      <v-btn
        v-if="papersStore.hasRealtimeUpdates"
        size="small"
        variant="text"
        @click="refreshData"
      >
        <v-icon start>mdi-refresh</v-icon>
        Refresh
      </v-btn>
    </div>

    <!-- 实时更新横幅 -->
    <v-alert
      v-if="showUpdateBanner && showNotifications"
      type="success"
      closable
      class="update-banner"
      @click:close="showUpdateBanner = false"
    >
      <template #prepend>
        <v-icon>mdi-update</v-icon>
      </template>
      <template #title>
        Real-time Updates Available
      </template>
      <template #default>
        {{ updatedPapers.size }} paper(s) have been updated.
        <v-btn size="small" variant="text" @click="refreshData">
          View Changes
        </v-btn>
      </template>
    </v-alert>

    <!-- 论文列表 -->
    <div class="paper-list">
      <div
        v-for="paper in papersStore.searchResults"
        :key="paper.id"
        :class="['paper-item', getPaperRowClass(paper.id)]"
      >
        <div class="paper-header">
          <h3 class="paper-title">
            <v-icon
              v-if="updatedPapers.has(paper.id)"
              size="small"
              color="success"
              class="update-icon"
            >
              mdi-update
            </v-icon>
            {{ paper.title }}
          </h3>
          <div class="paper-meta">
            <v-chip size="x-small" variant="outlined">
              {{ paper.year }}
            </v-chip>
            <v-chip size="x-small" variant="outlined">
              {{ paper.citation_count || 0 }} citations
            </v-chip>
          </div>
        </div>

        <div class="paper-authors">
          {{ paper.authors }}
        </div>

        <div v-if="paper.abstract" class="paper-abstract">
          {{ paper.abstract.substring(0, 200) }}...
        </div>

        <div class="paper-actions">
          <v-btn
            size="x-small"
            variant="text"
            :href="paper.url"
            target="_blank"
          >
            <v-icon start>mdi-open-in-new</v-icon>
            View
          </v-btn>

          <v-btn
            size="x-small"
            variant="text"
            @click="papersStore.toggleFavorite(paper.id)"
          >
            <v-icon start>
              {{ papersStore.isFavorite(paper.id) ? 'mdi-star' : 'mdi-star-outline' }}
            </v-icon>
            {{ papersStore.isFavorite(paper.id) ? 'Saved' : 'Save' }}
          </v-btn>
        </div>
      </div>

      <!-- 空状态 -->
      <div v-if="!papersStore.hasSearchResults" class="empty-state">
        <v-icon size="64" color="grey-lighten-1">
          mdi-file-document-outline
        </v-icon>
        <p>No papers found</p>
        <v-btn
          v-if="!wsStore.isConnected"
          variant="outlined"
          @click="wsStore.reconnect()"
        >
          <v-icon start>mdi-wifi</v-icon>
          Reconnect
        </v-btn>
      </div>
    </div>

    <!-- 加载状态 -->
    <div v-if="papersStore.isLoading" class="loading-state">
      <v-progress-circular indeterminate />
      <p>Loading papers...</p>
    </div>
  </div>
</template>

<style scoped>
.real-time-paper-list {
  width: 100%;
  max-width: 1200px;
  margin: 0 auto;
}

.status-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  background-color: #f5f5f5;
  border-radius: 8px;
  margin-bottom: 16px;
}

.connection-indicator {
  display: flex;
  align-items: center;
  gap: 12px;
}

.stats {
  font-size: 14px;
  color: #666;
}

.update-banner {
  margin-bottom: 16px;
}

.paper-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.paper-item {
  padding: 16px;
  background-color: white;
  border-radius: 8px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
  transition: all 0.3s ease;
}

.paper-item.recently-updated {
  background-color: #f0fdf4;
  border-left: 4px solid #4caf50;
  animation: highlight 2s ease;
}

@keyframes highlight {
  0%, 100% {
    transform: translateX(0);
  }
  50% {
    transform: translateX(4px);
  }
}

.paper-header {
  display: flex;
  justify-content: space-between;
  align-items: start;
  margin-bottom: 8px;
}

.paper-title {
  font-size: 18px;
  font-weight: 600;
  color: #333;
  margin: 0;
  display: flex;
  align-items: center;
  gap: 8px;
}

.update-icon {
  animation: pulse 1s ease infinite;
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.5;
  }
}

.paper-meta {
  display: flex;
  gap: 8px;
}

.paper-authors {
  color: #666;
  font-size: 14px;
  margin-bottom: 8px;
}

.paper-abstract {
  color: #888;
  font-size: 14px;
  line-height: 1.5;
  margin-bottom: 12px;
}

.paper-actions {
  display: flex;
  gap: 8px;
}

.empty-state {
  text-align: center;
  padding: 48px;
  color: #999;
}

.loading-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 48px;
  gap: 16px;
}

/* 深色主题支持 */
@media (prefers-color-scheme: dark) {
  .status-bar {
    background-color: #2c2c2c;
  }

  .paper-item {
    background-color: #1e1e1e;
  }

  .paper-item.recently-updated {
    background-color: #1a3a1a;
  }

  .paper-title {
    color: #fff;
  }

  .paper-authors {
    color: #aaa;
  }

  .paper-abstract {
    color: #888;
  }

  .stats {
    color: #aaa;
  }
}
</style>
