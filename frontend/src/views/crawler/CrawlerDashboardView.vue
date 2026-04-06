<template>
  <div class="crawler-dashboard">
    <!-- Page Header -->
    <div class="page-header">
      <h1 class="page-title">
        <el-icon><Odometer /></el-icon>
        爬虫仪表盘
      </h1>
      <div class="header-actions">
        <el-button type="primary" :icon="Plus" @click="handleCreateTask">
          创建新任务
        </el-button>
        <el-button :icon="Refresh" @click="refreshData" :loading="loading">
          刷新
        </el-button>
      </div>
    </div>

    <!-- Statistics Cards -->
    <el-row :gutter="20" class="stats-row">
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon total">
              <el-icon><Document /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ stats.totalTasks || 0 }}</div>
              <div class="stat-label">任务总数</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon active">
              <el-icon><Loading /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ stats.runningTasks || 0 }}</div>
              <div class="stat-label">活跃任务</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon success">
              <el-icon><CircleCheck /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ successRate }}%</div>
              <div class="stat-label">成功率</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon papers">
              <el-icon><Files /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ stats.totalPapers || 0 }}</div>
              <div class="stat-label">论文总数</div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Main Content -->
    <el-row :gutter="20" class="main-content">
      <!-- Active Tasks -->
      <el-col :xs="24" :lg="14">
        <el-card shadow="hover" class="tasks-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><List /></el-icon>
                实时任务列表
              </span>
              <el-link type="primary" @click="$router.push('/crawler/tasks')">
                查看全部
              </el-link>
            </div>
          </template>

          <div v-if="activeTasks.length === 0" class="empty-state">
            <el-empty description="暂无活跃任务">
              <el-button type="primary" @click="handleCreateTask">
                创建新任务
              </el-button>
            </el-empty>
          </div>

          <div v-else class="tasks-list">
            <div
              v-for="task in activeTasks.slice(0, 5)"
              :key="task.id"
              class="task-item"
            >
              <div class="task-header">
                <span class="task-title">{{ task.query }}</span>
                <el-tag :type="getTaskStatusType(task.status)" size="small">
                  {{ task.status }}
                </el-tag>
              </div>
              <div class="task-progress">
                <el-progress
                  :percentage="task.progress"
                  :status="task.status === 'completed' ? 'success' : undefined"
                />
              </div>
              <div class="task-info">
                <span>{{ task.source }}</span>
                <span>{{ task.completedPapers || 0 }} / {{ task.totalPapers || 0 }}</span>
              </div>
            </div>
          </div>
        </el-card>
      </el-col>

      <!-- Task Status Distribution -->
      <el-col :xs="24" :lg="10">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><PieChart /></el-icon>
                任务状态分布
              </span>
            </div>
          </template>

          <div class="chart-container">
            <div v-if="stats.totalTasks === 0" class="empty-chart">
              <el-empty description="暂无数据" />
            </div>
            <div v-else class="status-chart">
              <div
                v-for="(value, key) in taskStatusDistribution"
                :key="key"
                class="chart-item"
              >
                <div class="chart-label">{{ key }}</div>
                <div class="chart-bar">
                  <div
                    class="chart-fill"
                    :style="{
                      width: `${(value / stats.totalTasks) * 100}%`,
                      backgroundColor: getStatusColor(key)
                    }"
                  ></div>
                </div>
                <div class="chart-value">{{ value }}</div>
              </div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Bottom Section -->
    <el-row :gutter="20" class="bottom-content">
      <!-- Node Health -->
      <el-col :xs="24" :lg="12">
        <el-card shadow="hover" class="nodes-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><Monitor /></el-icon>
                节点健康状态
              </span>
              <el-link type="primary" @click="$router.push('/crawler/nodes')">
                管理节点
              </el-link>
            </div>
          </template>

          <div v-if="nodes.length === 0" class="empty-state">
            <el-empty description="暂无节点" />
          </div>

          <div v-else class="nodes-list">
            <div
              v-for="node in nodes"
              :key="node.id"
              class="node-item"
            >
              <div class="node-info">
                <div class="node-name">{{ node.name }}</div>
                <div class="node-status">
                  <el-tag
                    :type="node.healthy ? 'success' : 'danger'"
                    size="small"
                  >
                    {{ node.healthy ? '健康' : '异常' }}
                  </el-tag>
                  <span class="node-latency">
                    延迟: {{ node.latency }}ms
                  </span>
                </div>
              </div>
              <div class="node-stats">
                <span>任务: {{ node.activeTasks }}</span>
                <span>成功率: {{ node.successRate }}%</span>
              </div>
            </div>
          </div>
        </el-card>
      </el-col>

      <!-- Recent Activity -->
      <el-col :xs="24" :lg="12">
        <el-card shadow="hover" class="activity-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><Clock /></el-icon>
                最近活动
              </span>
            </div>
          </template>

          <div v-if="recentActivities.length === 0" class="empty-state">
            <el-empty description="暂无活动记录" />
          </div>

          <div v-else class="activity-list">
            <div
              v-for="activity in recentActivities"
              :key="activity.id"
              class="activity-item"
            >
              <div class="activity-icon" :class="activity.type">
                <el-icon>
                  <component :is="getActivityIcon(activity.type)" />
                </el-icon>
              </div>
              <div class="activity-content">
                <div class="activity-message">{{ activity.message }}</div>
                <div class="activity-time">{{ formatTime(activity.timestamp) }}</div>
              </div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useRouter } from 'vue-router'
import { useCrawlerStore } from '@/stores/crawlerStore'
import { ElMessage } from 'element-plus'
import {
  Odometer,
  Plus,
  Refresh,
  Document,
  Loading,
  CircleCheck,
  Files,
  List,
  PieChart,
  Monitor,
  Clock,
  SuccessFilled,
  Warning,
  CircleClose
} from '@element-plus/icons-vue'
import type { CrawlerTask } from '@/api/modules/crawler'

const router = useRouter()
const crawlerStore = useCrawlerStore()

// State
const loading = ref(false)
const stats = ref<any>({})
const nodes = ref<Array<{
  id: string
  name: string
  healthy: boolean
  latency: number
  activeTasks: number
  successRate: number
}>>([])
const recentActivities = ref<Array<{
  id: string
  type: 'success' | 'warning' | 'error' | 'info'
  message: string
  timestamp: number
}>>([])

// Computed
const activeTasks = computed(() => crawlerStore.activeTasks)
const successRate = computed(() => {
  if (!stats.value.totalTasks) return 0
  return Math.round(
    ((stats.value.completedTasks || 0) / stats.value.totalTasks) * 100
  )
})

const taskStatusDistribution = computed(() => {
  return {
    运行中: stats.value.runningTasks || 0,
    已完成: stats.value.completedTasks || 0,
    失败: stats.value.failedTasks || 0,
    待处理: stats.value.pendingTasks || 0
  }
})

// Methods
const handleCreateTask = () => {
  router.push('/crawler/templates')
}

const refreshData = async () => {
  loading.value = true
  try {
    await Promise.all([
      crawlerStore.fetchStats(),
      crawlerStore.fetchHistory(1, 10)
    ])
    stats.value = crawlerStore.stats || {}
    updateRecentActivities()
    ElMessage.success('数据已刷新')
  } catch (error: any) {
    ElMessage.error('刷新失败: ' + error.message)
  } finally {
    loading.value = false
  }
}

const getTaskStatusType = (status: string) => {
  const types: Record<string, any> = {
    running: 'warning',
    completed: 'success',
    failed: 'danger',
    pending: 'info',
    cancelled: 'info'
  }
  return types[status] || 'info'
}

const getStatusColor = (status: string) => {
  const colors: Record<string, string> = {
    运行中: '#e6a23c',
    已完成: '#67c23a',
    失败: '#f56c6c',
    待处理: '#909399'
  }
  return colors[status] || '#409eff'
}

const getActivityIcon = (type: string) => {
  const icons: Record<string, any> = {
    success: SuccessFilled,
    warning: Warning,
    error: CircleClose,
    info: Clock
  }
  return icons[type] || Clock
}

const formatTime = (timestamp: number) => {
  const date = new Date(timestamp)
  const now = new Date()
  const diff = now.getTime() - date.getTime()

  if (diff < 60000) return '刚刚'
  if (diff < 3600000) return `${Math.floor(diff / 60000)} 分钟前`
  if (diff < 86400000) return `${Math.floor(diff / 3600000)} 小时前`
  return date.toLocaleString()
}

const updateRecentActivities = () => {
  const activities = (crawlerStore.history || []).slice(0, 10).map(task => {
    let type: 'success' | 'error' | 'warning' | 'info' = 'info'
    if (task.status === 'completed') type = 'success'
    else if (task.status === 'failed') type = 'error'
    else if (task.status === 'running') type = 'warning'

    return {
      id: task.id,
      type,
      message: `任务 "${task.query}" ${task.status}`,
      timestamp: new Date(task.completedAt || task.startedAt || Date.now()).getTime()
    }
  })
  recentActivities.value = activities
}

// Lifecycle
onMounted(async () => {
  await refreshData()
  // Refresh every 30 seconds
  const interval = setInterval(refreshData, 30000)
  onUnmounted(() => clearInterval(interval))
})
</script>

<style scoped lang="scss">
// ==========================================
// 爬虫仪表盘现代化样式
// Modern Crawler Dashboard Styles
// ==========================================

.crawler-dashboard {
  width: 100%;
  max-width: 1600px;
  margin: 0 auto;
}

// 页面头部
.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: $spacing-6;
  padding: $spacing-8;
  background: linear-gradient(135deg, #ffffff 0%, #f8fafc 100%);
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  margin-bottom: $spacing-6;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    box-shadow: $shadow-md;
  }

  .page-title {
    display: flex;
    align-items: center;
    gap: $spacing-3;
    margin: 0;
    font-size: $font-size-3xl;
    font-weight: $font-weight-bold;
    color: $text-primary;

    .el-icon {
      color: $primary-500;
    }
  }

  .header-actions {
    display: flex;
    gap: $spacing-3;
  }
}

// 统计卡片行
.stats-row {
  margin-bottom: $spacing-6;

  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

// 统计卡片
.stat-card {
  height: 100%;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  transition: all $duration-slow;
  overflow: hidden;

  &:hover {
    box-shadow: $shadow-md;
    transform: translateY(-4px);
  }

  :deep(.el-card__body) {
    padding: $spacing-6;
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  .stat-content {
    display: flex;
    align-items: center;
    gap: $spacing-5;

    .stat-icon {
      width: 64px;
      height: 64px;
      border-radius: $border-radius-lg;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 28px;
      flex-shrink: 0;
      box-shadow: $shadow-sm;

      &.total {
        background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
        color: white;
        box-shadow: 0 4px 14px 0 rgba($primary-500, 0.39);
      }

      &.active {
        background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
        color: white;
        box-shadow: 0 4px 14px 0 rgba(245, 87, 108, 0.39);
      }

      &.success {
        background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
        color: white;
        box-shadow: 0 4px 14px 0 rgba(79, 172, 254, 0.39);
      }

      &.papers {
        background: linear-gradient(135deg, #43e97b 0%, #38f9d7 100%);
        color: white;
        box-shadow: 0 4px 14px 0 rgba(67, 233, 123, 0.39);
      }
    }

    .stat-info {
      flex: 1;

      .stat-value {
        font-size: $font-size-3xl;
        font-weight: $font-weight-bold;
        color: $text-primary;
        line-height: 1;
        margin-bottom: $spacing-2;
      }

      .stat-label {
        font-size: $font-size-sm;
        color: $text-secondary;
        font-weight: $font-weight-medium;
      }
    }
  }
}

// 主内容区
.main-content {
  margin-bottom: $spacing-6;

  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

.bottom-content {
  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

// 卡片头部
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-weight: $font-weight-semibold;
  font-size: $font-size-base;

  span {
    display: flex;
    align-items: center;
    gap: $spacing-2;
    color: $text-primary;
  }

  .el-icon {
    color: $primary-500;
  }
}

// 空状态
.empty-state {
  padding: $spacing-12;
  text-align: center;
  background: linear-gradient(135deg, $gray-50 0%, #ffffff 100%);
  border-radius: $border-radius-lg;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
  }
}

// 任务列表
.tasks-list {
  .task-item {
    padding: $spacing-5;
    background: #ffffff;
    border: 1px solid $border-light;
    border-radius: $border-radius-lg;
    margin-bottom: $spacing-4;
    transition: all $duration-fast;
    cursor: pointer;

    &:hover {
      box-shadow: $shadow-md;
      border-color: $primary-200;
      transform: translateX(4px);
    }

    &:last-child {
      margin-bottom: 0;
    }

    .dark & {
      background: $gray-800;
      border-color: $gray-700;
    }

    .task-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: $spacing-4;

      .task-title {
        font-weight: $font-weight-semibold;
        color: $text-primary;
        font-size: $font-size-base;
      }
    }

    .task-progress {
      margin-bottom: $spacing-4;
    }

    .task-info {
      display: flex;
      justify-content: space-between;
      font-size: $font-size-sm;
      color: $text-secondary;
    }
  }
}

// 图表容器
.chart-card,
.tasks-card,
.nodes-card,
.activity-card {
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  transition: all $duration-slow;
  height: 100%;

  &:hover {
    box-shadow: $shadow-md;
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  :deep(.el-card__header) {
    border-bottom: 1px solid $border-light;
    padding: $spacing-5;
    background: linear-gradient(135deg, $gray-50 0%, #ffffff 100%);

    .dark & {
      background: linear-gradient(135deg, $gray-800 0%, $gray-700 100%);
      border-bottom-color: $gray-700;
    }
  }

  :deep(.el-card__body) {
    padding: $spacing-5;
  }
}

.chart-container {
  min-height: 300px;

  .empty-chart {
    display: flex;
    align-items: center;
    justify-content: center;
    height: 300px;
  }

  .status-chart {
    padding: $spacing-4;

    .chart-item {
      display: flex;
      align-items: center;
      gap: $spacing-4;
      margin-bottom: $spacing-5;

      &:last-child {
        margin-bottom: 0;
      }

      .chart-label {
        width: 80px;
        font-size: $font-size-sm;
        font-weight: $font-weight-medium;
        color: $text-regular;
        flex-shrink: 0;
      }

      .chart-bar {
        flex: 1;
        height: 28px;
        background: $gray-100;
        border-radius: $border-radius-base;
        overflow: hidden;
        box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.1);

        .dark & {
          background: $gray-700;
        }

        .chart-fill {
          height: 100%;
          transition: width $duration-slow $easing-ease-out;
          border-radius: $border-radius-base;
          position: relative;

          &::after {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: linear-gradient(90deg,
              transparent 0%,
              rgba(255, 255, 255, 0.2) 50%,
              transparent 100%
            );
          }
        }
      }

      .chart-value {
        width: 50px;
        text-align: right;
        font-weight: $font-weight-bold;
        color: $text-primary;
        font-size: $font-size-base;
        flex-shrink: 0;
      }
    }
  }
}

// 节点列表
.nodes-list {
  .node-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: $spacing-5;
    background: #ffffff;
    border: 1px solid $border-light;
    border-radius: $border-radius-lg;
    margin-bottom: $spacing-4;
    transition: all $duration-fast;

    &:hover {
      box-shadow: $shadow-sm;
      border-color: $primary-200;
    }

    &:last-child {
      margin-bottom: 0;
    }

    .dark & {
      background: $gray-800;
      border-color: $gray-700;
    }

    .node-info {
      flex: 1;

      .node-name {
        font-weight: $font-weight-semibold;
        color: $text-primary;
        margin-bottom: $spacing-2;
        font-size: $font-size-base;
      }

      .node-status {
        display: flex;
        align-items: center;
        gap: $spacing-3;
        font-size: $font-size-sm;

        .node-latency {
          color: $text-secondary;
        }
      }
    }

    .node-stats {
      text-align: right;
      font-size: $font-size-sm;
      color: $text-secondary;
      display: flex;
      flex-direction: column;
      gap: $spacing-1;
    }
  }
}

// 活动列表
.activity-list {
  .activity-item {
    display: flex;
    gap: $spacing-4;
    padding: $spacing-4 0;
    border-bottom: 1px solid $border-light;

    &:last-child {
      border-bottom: none;
    }

    .activity-icon {
      width: 40px;
      height: 40px;
      border-radius: $border-radius-full;
      display: flex;
      align-items: center;
      justify-content: center;
      flex-shrink: 0;
      font-size: 18px;

      &.success {
        background: rgba($success-color, 0.1);
        color: $success-color;
      }

      &.warning {
        background: rgba($warning-color, 0.1);
        color: $warning-color;
      }

      &.error {
        background: rgba($danger-color, 0.1);
        color: $danger-color;
      }

      &.info {
        background: rgba($info-color, 0.1);
        color: $info-color;
      }
    }

    .activity-content {
      flex: 1;

      .activity-message {
        margin-bottom: $spacing-2;
        color: $text-primary;
        font-size: $font-size-sm;
        line-height: $line-height-normal;
      }

      .activity-time {
        font-size: $font-size-xs;
        color: $text-secondary;
      }
    }
  }
}

// 响应式设计
@media (max-width: 1200px) {
  .crawler-dashboard {
    max-width: 100%;
  }
}

@media (max-width: 768px) {
  .page-header {
    flex-direction: column;
    align-items: flex-start;
    gap: $spacing-4;
    padding: $spacing-5;

    .page-title {
      font-size: $font-size-2xl;
    }

    .header-actions {
      width: 100%;

      .el-button {
        flex: 1;
      }
    }
  }

  .stat-card .stat-content {
    .stat-icon {
      width: 56px;
      height: 56px;
      font-size: 24px;
    }

    .stat-info .stat-value {
      font-size: $font-size-2xl;
    }
  }

  .nodes-list .node-item {
    flex-direction: column;
    align-items: flex-start;
    gap: $spacing-3;

    .node-stats {
      text-align: left;
      width: 100%;
      flex-direction: row;
      justify-content: space-between;
    }
  }
}

@media (max-width: 480px) {
  .page-header {
    padding: $spacing-4;
  }

  .stat-card,
  .chart-card,
  .tasks-card,
  .nodes-card,
  .activity-card {
    :deep(.el-card__header),
    :deep(.el-card__body) {
      padding: $spacing-4;
    }
  }
}
</style>
