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
  const activities = crawlerStore.history.slice(0, 10).map(task => ({
    id: task.id,
    type: task.status === 'completed' ? 'success' :
          task.status === 'failed' ? 'error' : 'info',
    message: `任务 "${task.query}" ${task.status}`,
    timestamp: new Date(task.completedAt || task.startedAt || Date.now()).getTime()
  }))
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
.crawler-dashboard {
  padding: 20px;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;

  .page-title {
    display: flex;
    align-items: center;
    gap: 10px;
    margin: 0;
    font-size: 24px;
  }

  .header-actions {
    display: flex;
    gap: 10px;
  }
}

.stats-row {
  margin-bottom: 20px;
}

.stat-card {
  .stat-content {
    display: flex;
    align-items: center;
    gap: 15px;

    .stat-icon {
      width: 60px;
      height: 60px;
      border-radius: 12px;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 28px;

      &.total {
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
      }

      &.active {
        background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
        color: white;
      }

      &.success {
        background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
        color: white;
      }

      &.papers {
        background: linear-gradient(135deg, #43e97b 0%, #38f9d7 100%);
        color: white;
      }
    }

    .stat-info {
      .stat-value {
        font-size: 32px;
        font-weight: bold;
        color: #303133;
        line-height: 1;
      }

      .stat-label {
        font-size: 14px;
        color: #909399;
        margin-top: 5px;
      }
    }
  }
}

.main-content {
  margin-bottom: 20px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-weight: bold;

  span {
    display: flex;
    align-items: center;
    gap: 5px;
  }
}

.empty-state {
  padding: 40px 0;
  text-align: center;
}

.tasks-list {
  .task-item {
    padding: 15px;
    border: 1px solid #ebeef5;
    border-radius: 8px;
    margin-bottom: 10px;
    transition: all 0.3s;

    &:hover {
      box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
    }

    .task-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 10px;

      .task-title {
        font-weight: bold;
        color: #303133;
      }
    }

    .task-progress {
      margin-bottom: 10px;
    }

    .task-info {
      display: flex;
      justify-content: space-between;
      font-size: 12px;
      color: #909399;
    }
  }
}

.chart-container {
  min-height: 300px;

  .empty-chart {
    display: flex;
    align-items: center;
    justify-content: center;
    height: 100%;
  }

  .status-chart {
    .chart-item {
      display: flex;
      align-items: center;
      gap: 10px;
      margin-bottom: 15px;

      .chart-label {
        width: 80px;
        font-size: 14px;
        color: #606266;
      }

      .chart-bar {
        flex: 1;
        height: 24px;
        background: #f5f7fa;
        border-radius: 4px;
        overflow: hidden;

        .chart-fill {
          height: 100%;
          transition: width 0.3s;
        }
      }

      .chart-value {
        width: 40px;
        text-align: right;
        font-weight: bold;
        color: #303133;
      }
    }
  }
}

.nodes-list {
  .node-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 15px;
    border: 1px solid #ebeef5;
    border-radius: 8px;
    margin-bottom: 10px;

    .node-info {
      .node-name {
        font-weight: bold;
        margin-bottom: 5px;
      }

      .node-status {
        display: flex;
        align-items: center;
        gap: 10px;
        font-size: 12px;

        .node-latency {
          color: #909399;
        }
      }
    }

    .node-stats {
      text-align: right;
      font-size: 12px;
      color: #909399;
    }
  }
}

.activity-list {
  .activity-item {
    display: flex;
    gap: 15px;
    padding: 15px 0;
    border-bottom: 1px solid #ebeef5;

    &:last-child {
      border-bottom: none;
    }

    .activity-icon {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      display: flex;
      align-items: center;
      justify-content: center;
      flex-shrink: 0;

      &.success {
        background: #f0f9ff;
        color: #67c23a;
      }

      &.warning {
        background: #fef0f0;
        color: #e6a23c;
      }

      &.error {
        background: #fef0f0;
        color: #f56c6c;
      }

      &.info {
        background: #f4f4f5;
        color: #909399;
      }
    }

    .activity-content {
      flex: 1;

      .activity-message {
        margin-bottom: 5px;
        color: #303133;
      }

      .activity-time {
        font-size: 12px;
        color: #909399;
      }
    }
  }
}

// Responsive
@media (max-width: 768px) {
  .page-header {
    flex-direction: column;
    gap: 10px;
    text-align: center;
  }

  .header-actions {
    width: 100%;
    justify-content: center;
  }

  .stat-content {
    .stat-icon {
      width: 50px;
      height: 50px;
      font-size: 24px;
    }

    .stat-info {
      .stat-value {
        font-size: 24px;
      }
    }
  }
}
</style>
