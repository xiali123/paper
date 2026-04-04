<template>
  <div class="task-list">
    <!-- Page Header -->
    <div class="page-header">
      <h1 class="page-title">
        <el-icon><List /></el-icon>
        任务管理
      </h1>
      <div class="header-actions">
        <el-button type="primary" :icon="Plus" @click="handleCreateTask">
          创建新任务
        </el-button>
        <el-button :icon="Refresh" @click="refreshTasks" :loading="loading">
          刷新
        </el-button>
      </div>
    </div>

    <!-- Statistics -->
    <el-row :gutter="20" class="stats-row">
      <el-col :xs="12" :sm="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-label">活跃任务</div>
            <div class="stat-value active">{{ activeTasks.length }}</div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="12" :sm="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-label">已完成</div>
            <div class="stat-value success">{{ completedTasks.length }}</div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="12" :sm="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-label">失败</div>
            <div class="stat-value error">{{ failedTasks.length }}</div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="12" :sm="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-label">总论文数</div>
            <div class="stat-value">{{ totalPapers }}</div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Filter Tabs -->
    <el-card shadow="hover" class="filter-card">
      <el-tabs v-model="activeTab" @tab-change="handleTabChange">
        <el-tab-pane label="全部任务" name="all">
          <template #label>
            <span>
              <el-icon><Files /></el-icon>
              全部任务 ({{ tasks.length }})
            </span>
          </template>
        </el-tab-pane>
        <el-tab-pane label="运行中" name="running">
          <template #label>
            <span>
              <el-icon><Loading /></el-icon>
              运行中 ({{ activeTasks.length }})
            </span>
          </template>
        </el-tab-pane>
        <el-tab-pane label="已完成" name="completed">
          <template #label>
            <span>
              <el-icon><CircleCheck /></el-icon>
              已完成 ({{ completedTasks.length }})
            </span>
          </template>
        </el-tab-pane>
        <el-tab-pane label="失败" name="failed">
          <template #label>
            <span>
              <el-icon><CircleClose /></el-icon>
              失败 ({{ failedTasks.length }})
            </span>
          </template>
        </el-tab-pane>
      </el-tabs>
    </el-card>

    <!-- Task List -->
    <el-card shadow="hover" class="tasks-card">
      <div v-if="filteredTasks.length === 0" class="empty-state">
        <el-empty description="暂无任务">
          <el-button type="primary" @click="handleCreateTask">
            创建新任务
          </el-button>
        </el-empty>
      </div>

      <div v-else class="tasks-content">
        <!-- Batch Actions -->
        <div v-if="selectedTasks.length > 0" class="batch-actions-bar">
          <span>已选择 {{ selectedTasks.length }} 个任务</span>
          <div class="batch-buttons">
            <el-button
              type="danger"
              size="small"
              :icon="Delete"
              @click="handleBatchCancel"
            >
              批量取消
            </el-button>
            <el-button
              size="small"
              :icon="Delete"
              @click="handleBatchDelete"
            >
              批量删除
            </el-button>
            <el-button
              size="small"
              @click="selectedTasks = []"
            >
              取消选择
            </el-button>
          </div>
        </div>

        <!-- Task Items -->
        <div
          v-for="task in filteredTasks"
          :key="task.id"
          class="task-item"
          :class="{ 'task-selected': selectedTasks.includes(task.id) }"
        >
          <div class="task-checkbox">
            <el-checkbox
              v-model="selectedTasks"
              :label="task.id"
              v-if="task.status !== 'completed'"
            />
          </div>

          <div class="task-main">
            <div class="task-header">
              <div class="task-title">
                <el-icon><Search /></el-icon>
                {{ task.query }}
              </div>
              <div class="task-actions">
                <el-tag :type="getStatusType(task.status)" size="small">
                  {{ getStatusText(task.status) }}
                </el-tag>
                <el-tag size="small" type="info">{{ task.source }}</el-tag>
              </div>
            </div>

            <div class="task-progress">
              <el-progress
                :percentage="task.progress"
                :status="task.status === 'completed' ? 'success' :
                        task.status === 'failed' ? 'exception' : undefined"
                :stroke-width="8"
              />
              <div class="progress-info">
                <span>{{ task.completedPapers || 0 }} / {{ task.totalPapers || 0 }}</span>
                <span v-if="task.status === 'running'" class="live-indicator">
                  <span class="dot"></span>
                  实时更新
                </span>
              </div>
            </div>

            <div class="task-meta">
              <span>
                <el-icon><Clock /></el-icon>
                {{ formatTime(task.startedAt) }}
              </span>
              <span v-if="task.completedAt">
                <el-icon><CircleCheck /></el-icon>
                {{ formatTime(task.completedAt) }}
              </span>
              <span v-if="task.errorMessage" class="error-message">
                <el-icon><Warning /></el-icon>
                {{ task.errorMessage }}
              </span>
            </div>
          </div>

          <div class="task-operations">
            <el-dropdown @command="(cmd) => handleCommand(cmd, task)">
              <el-button type="primary" :icon="MoreFilled" circle />
              <template #dropdown>
                <el-dropdown-menu>
                  <el-dropdown-item
                    v-if="task.status === 'running' || task.status === 'pending'"
                    command="pause"
                    :icon="VideoPause"
                  >
                    暂停
                  </el-dropdown-item>
                  <el-dropdown-item
                    v-if="task.status === 'pending'"
                    command="resume"
                    :icon="VideoPlay"
                  >
                    继续
                  </el-dropdown-item>
                  <el-dropdown-item
                    v-if="task.status !== 'completed' && task.status !== 'failed'"
                    command="cancel"
                    :icon="CircleClose"
                  >
                    取消
                  </el-dropdown-item>
                  <el-dropdown-item
                    v-if="task.status === 'completed'"
                    command="view"
                    :icon="View"
                  >
                    查看结果
                  </el-dropdown-item>
                  <el-dropdown-item
                    v-if="task.status === 'completed'"
                    command="save"
                    :icon="Download"
                  >
                    保存到数据库
                  </el-dropdown-item>
                  <el-dropdown-item
                    v-if="task.status === 'failed'"
                    command="retry"
                    :icon="RefreshRight"
                  >
                    重试
                  </el-dropdown-item>
                  <el-dropdown-item
                    command="logs"
                    :icon="Document"
                    divided
                  >
                    查看日志
                  </el-dropdown-item>
                  <el-dropdown-item
                    command="delete"
                    :icon="Delete"
                  >
                    删除
                  </el-dropdown-item>
                </el-dropdown-menu>
              </template>
            </el-dropdown>
          </div>
        </div>
      </div>
    </el-card>

    <!-- Log Drawer -->
    <el-drawer
      v-model="logDrawerVisible"
      title="任务日志"
      size="50%"
    >
      <div v-if="currentTask" class="log-content">
        <div class="log-header">
          <h3>{{ currentTask.query }}</h3>
          <el-tag :type="getStatusType(currentTask.status)">
            {{ getStatusText(currentTask.status) }}
          </el-tag>
        </div>

        <el-divider />

        <div class="task-details">
          <el-descriptions :column="2" border>
            <el-descriptions-item label="任务ID">
              {{ currentTask.id }}
            </el-descriptions-item>
            <el-descriptions-item label="数据源">
              {{ currentTask.source }}
            </el-descriptions-item>
            <el-descriptions-item label="开始时间">
              {{ formatTime(currentTask.startedAt) }}
            </el-descriptions-item>
            <el-descriptions-item label="完成时间">
              {{ currentTask.completedAt ? formatTime(currentTask.completedAt) : '运行中...' }}
            </el-descriptions-item>
            <el-descriptions-item label="进度">
              {{ currentTask.progress }}%
            </el-descriptions-item>
            <el-descriptions-item label="论文数量">
              {{ currentTask.completedPapers || 0 }} / {{ currentTask.totalPapers || 0 }}
            </el-descriptions-item>
          </el-descriptions>
        </div>

        <el-divider>日志</el-divider>

        <div class="log-entries">
          <div
            v-for="(log, index) in mockLogs"
            :key="index"
            class="log-entry"
            :class="`log-${log.level}`"
          >
            <span class="log-time">{{ log.time }}</span>
            <span class="log-level">{{ log.level.toUpperCase() }}</span>
            <span class="log-message">{{ log.message }}</span>
          </div>
        </div>
      </div>
    </el-drawer>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useRouter } from 'vue-router'
import { useCrawlerStore } from '@/stores/crawlerStore'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  List,
  Plus,
  Refresh,
  Files,
  Loading,
  CircleCheck,
  CircleClose,
  Search,
  Clock,
  Warning,
  VideoPlay,
  VideoPause,
  View,
  Download,
  Document,
  Delete,
  MoreFilled,
  RefreshRight
} from '@element-plus/icons-vue'
import type { CrawlerTask } from '@/api/modules/crawler'

const router = useRouter()
const crawlerStore = useCrawlerStore()

// State
const loading = ref(false)
const activeTab = ref('all')
const selectedTasks = ref<string[]>([])
const logDrawerVisible = ref(false)
const currentTask = ref<CrawlerTask | null>(null)

// Mock logs for demonstration
const mockLogs = ref<Array<{
  time: string
  level: string
  message: string
}>>([])

// Computed
const tasks = computed(() => crawlerStore.tasks)
const activeTasks = computed(() => crawlerStore.activeTasks)
const completedTasks = computed(() => crawlerStore.completedTasks)
const failedTasks = computed(() => crawlerStore.failedTasks)

const totalPapers = computed(() => {
  return tasks.value.reduce((sum, task) => sum + (task.completedPapers || 0), 0)
})

const filteredTasks = computed(() => {
  switch (activeTab.value) {
    case 'running':
      return activeTasks.value
    case 'completed':
      return completedTasks.value
    case 'failed':
      return failedTasks.value
    default:
      return tasks.value
  }
})

// Methods
const handleCreateTask = () => {
  router.push('/crawler/templates')
}

const refreshTasks = async () => {
  loading.value = true
  try {
    await crawlerStore.fetchHistory(1, 50)
    ElMessage.success('刷新成功')
  } catch (error: any) {
    ElMessage.error('刷新失败：' + error.message)
  } finally {
    loading.value = false
  }
}

const handleTabChange = (tabName: string) => {
  activeTab.value = tabName
}

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

const formatTime = (timestamp?: string) => {
  if (!timestamp) return '-'
  const date = new Date(timestamp)
  return date.toLocaleString()
}

const handleCommand = async (command: string, task: CrawlerTask) => {
  switch (command) {
    case 'pause':
      await crawlerStore.pauseTask(task.id)
      ElMessage.success('任务已暂停')
      break
    case 'resume':
      await crawlerStore.resumeTask(task.id)
      ElMessage.success('任务已继续')
      break
    case 'cancel':
      try {
        await ElMessageBox.confirm('确定要取消这个任务吗？', '确认', {
          type: 'warning'
        })
        await crawlerStore.cancelTask(task.id)
        ElMessage.success('任务已取消')
      } catch {
        // User cancelled
      }
      break
    case 'view':
      router.push(`/papers?taskId=${task.id}`)
      break
    case 'save':
      try {
        if (task.papers && task.papers.length > 0) {
          await crawlerStore.savePapers(task.papers)
          ElMessage.success('论文已保存到数据库')
        } else {
          ElMessage.warning('没有可保存的论文')
        }
      } catch (error: any) {
        ElMessage.error('保存失败：' + error.message)
      }
      break
    case 'retry':
      try {
        const newTask = await crawlerStore.startTask({
          query: task.query,
          source: task.source,
          limit: task.totalPapers,
          options: task.options
        })
        ElMessage.success(`任务已重新启动，ID: ${newTask.id}`)
      } catch (error: any) {
        ElMessage.error('重试失败：' + error.message)
      }
      break
    case 'logs':
      currentTask.value = task
      generateMockLogs(task)
      logDrawerVisible.value = true
      break
    case 'delete':
      try {
        await ElMessageBox.confirm('确定要删除这个任务吗？', '确认', {
          type: 'warning'
        })
        // Remove from store (in real app, would call API)
        const index = tasks.value.findIndex(t => t.id === task.id)
        if (index !== -1) {
          tasks.value.splice(index, 1)
        }
        ElMessage.success('任务已删除')
      } catch {
        // User cancelled
      }
      break
  }
}

const handleBatchCancel = async () => {
  try {
    await ElMessageBox.confirm(
      `确定要取消选中的 ${selectedTasks.value.length} 个任务吗？`,
      '确认',
      { type: 'warning' }
    )

    for (const taskId of selectedTasks.value) {
      await crawlerStore.cancelTask(taskId)
    }

    selectedTasks.value = []
    ElMessage.success('批量取消成功')
  } catch {
    // User cancelled
  }
}

const handleBatchDelete = async () => {
  try {
    await ElMessageBox.confirm(
      `确定要删除选中的 ${selectedTasks.value.length} 个任务吗？`,
      '确认',
      { type: 'warning' }
    )

    // Remove from store (in real app, would call API)
    tasks.value = tasks.value.filter(t => !selectedTasks.value.includes(t.id))
    selectedTasks.value = []
    ElMessage.success('批量删除成功')
  } catch {
    // User cancelled
  }
}

const generateMockLogs = (task: CrawlerTask) => {
  const logs = []
  const levels = ['info', 'info', 'info', 'warning', 'error']

  logs.push({
    time: formatTime(task.startedAt),
    level: 'info',
    message: `任务开始，查询: ${task.query}`
  })

  for (let i = 0; i < 5; i++) {
    const level = levels[Math.floor(Math.random() * levels.length)]
    logs.push({
      time: new Date(Date.now() - Math.random() * 1000000).toLocaleTimeString(),
      level,
      message: `处理进度更新 ${Math.floor(Math.random() * 100)}%`
    })
  }

  if (task.status === 'completed') {
    logs.push({
      time: formatTime(task.completedAt),
      level: 'info',
      message: `任务完成，成功获取 ${task.completedPapers} 篇论文`
    })
  } else if (task.status === 'failed') {
    logs.push({
      time: new Date().toLocaleTimeString(),
      level: 'error',
      message: task.errorMessage || '任务失败'
    })
  }

  mockLogs.value = logs
}

// Lifecycle
onMounted(async () => {
  await refreshTasks()

  // Simulate real-time updates
  const interval = setInterval(() => {
    if (activeTasks.value.length > 0) {
      // Update progress for running tasks
      activeTasks.value.forEach(task => {
        if (task.progress < 100 && task.status === 'running') {
          task.progress = Math.min(100, task.progress + Math.random() * 5)
          task.completedPapers = Math.floor(
            (task.progress / 100) * (task.totalPapers || 10)
          )

          if (task.progress >= 100) {
            task.status = 'completed'
            task.completedAt = new Date().toISOString()
          }
        }
      })
    }
  }, 2000)

  onUnmounted(() => clearInterval(interval))
})
</script>

<style scoped lang="scss">
// ==========================================
// 任务列表页面现代化样式
// Modern Task List Styles
// ==========================================

.task-list {
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

.stat-card {
  height: 100%;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  transition: all $duration-slow;

  &:hover {
    box-shadow: $shadow-md;
    transform: translateY(-2px);
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  .stat-content {
    text-align: center;
    padding: $spacing-5;

    .stat-label {
      font-size: $font-size-sm;
      color: $text-secondary;
      margin-bottom: $spacing-3;
      font-weight: $font-weight-medium;
    }

    .stat-value {
      font-size: $font-size-3xl;
      font-weight: $font-weight-bold;
      color: $text-primary;
      line-height: 1;

      &.active {
        color: $warning-color;
      }

      &.success {
        color: $success-color;
      }

      &.error {
        color: $danger-color;
      }
    }
  }
}

// 筛选卡片
.filter-card {
  margin-bottom: $spacing-6;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  :deep(.el-card__body) {
    padding: $spacing-4;
  }

  :deep(.el-tabs__header) {
    margin: 0;
  }

  :deep(.el-tabs__nav-wrap::after) {
    display: none;
  }

  :deep(.el-tabs__item) {
    font-weight: $font-weight-medium;
  }
}

// 任务卡片
.tasks-card {
  min-height: 500px;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  :deep(.el-card__header) {
    border-bottom: 1px solid $border-light;
    padding: $spacing-5;

    .dark & {
      border-bottom-color: $gray-700;
    }
  }

  :deep(.el-card__body) {
    padding: $spacing-5;
  }
}

// 空状态
.empty-state {
  padding: $spacing-16;
  text-align: center;
  background: linear-gradient(135deg, $gray-50 0%, #ffffff 100%);
  border-radius: $border-radius-lg;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
  }
}

// 批量操作栏
.batch-actions-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: $spacing-4 $spacing-5;
  background: linear-gradient(90deg, rgba($primary-500, 0.1) 0%, rgba($primary-500, 0.05) 100%);
  border-radius: $border-radius-lg;
  margin-bottom: $spacing-5;
  border: 1px solid rgba($primary-500, 0.2);

  span {
    font-size: $font-size-sm;
    font-weight: $font-weight-medium;
    color: $primary-600;
  }

  .dark & span {
    color: $primary-400;
  }

  .batch-buttons {
    display: flex;
    gap: $spacing-2;
  }
}

// 任务项
.task-item {
  display: flex;
  gap: $spacing-4;
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

  &.task-selected {
    background: rgba($primary-500, 0.05);
    border-color: $primary-500;
    border-width: 2px;
  }

  &:last-child {
    margin-bottom: 0;
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  .task-checkbox {
    padding-top: $spacing-2;
  }

  .task-main {
    flex: 1;
    min-width: 0;

    .task-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: $spacing-4;
      gap: $spacing-4;

      .task-title {
        display: flex;
        align-items: center;
        gap: $spacing-2;
        font-size: $font-size-lg;
        font-weight: $font-weight-semibold;
        color: $text-primary;
        flex: 1;
        min-width: 0;

        .el-icon {
          color: $primary-500;
          flex-shrink: 0;
        }
      }

      .task-actions {
        display: flex;
        gap: $spacing-2;
        flex-shrink: 0;
      }
    }

    .task-progress {
      margin-bottom: $spacing-4;

      .progress-info {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-top: $spacing-2;
        font-size: $font-size-sm;
        color: $text-secondary;

        .live-indicator {
          display: flex;
          align-items: center;
          gap: $spacing-2;
          color: $success-color;
          font-weight: $font-weight-medium;

          .dot {
            width: 8px;
            height: 8px;
            background: $success-color;
            border-radius: 50%;
            animation: pulse 1.5s infinite;
          }
        }
      }
    }

    .task-meta {
      display: flex;
      gap: $spacing-6;
      font-size: $font-size-sm;
      color: $text-secondary;
      flex-wrap: wrap;

      span {
        display: flex;
        align-items: center;
        gap: $spacing-1;

        .el-icon {
          font-size: 14px;
        }
      }

      .error-message {
        color: $danger-color;
      }
    }
  }

  .task-operations {
    display: flex;
    align-items: center;
    flex-shrink: 0;
  }
}

// 日志抽屉内容
.log-content {
  .log-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: $spacing-6;

    h3 {
      margin: 0;
      font-size: $font-size-lg;
      color: $text-primary;
    }
  }

  .task-details {
    margin-bottom: $spacing-6;
  }

  .log-entries {
    max-height: 500px;
    overflow-y: auto;
    background: $gray-50;
    border-radius: $border-radius-lg;
    padding: $spacing-4;

    .dark & {
      background: $gray-900;
    }

    // 自定义滚动条
    &::-webkit-scrollbar {
      width: 8px;
    }

    &::-webkit-scrollbar-thumb {
      background: rgba($gray-400, 0.3);
      border-radius: $border-radius-full;
    }

    .log-entry {
      display: flex;
      gap: $spacing-4;
      padding: $spacing-3 0;
      border-bottom: 1px solid $border-light;
      font-family: $font-family-code;
      font-size: $font-size-sm;
      line-height: $line-height-normal;

      &:last-child {
        border-bottom: none;
      }

      .log-time {
        color: $text-secondary;
        min-width: 100px;
        font-size: $font-size-xs;
      }

      .log-level {
        font-weight: $font-weight-semibold;
        min-width: 70px;
        font-size: $font-size-xs;
      }

      .log-message {
        flex: 1;
        color: $text-primary;
        word-break: break-all;
      }

      &.log-info .log-level {
        color: $info-color;
      }

      &.log-warning .log-level {
        color: $warning-color;
      }

      &.log-error .log-level {
        color: $danger-color;
      }
    }
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

// 响应式设计
@media (max-width: 1200px) {
  .task-list {
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

  .task-item {
    flex-direction: column;
    gap: $spacing-3;

    .task-main {
      .task-header {
        flex-direction: column;
        align-items: flex-start;
        gap: $spacing-3;

        .task-title {
          font-size: $font-size-base;
        }
      }

      .task-meta {
        flex-direction: column;
        gap: $spacing-2;
      }
    }

    .task-operations {
      width: 100%;
      justify-content: flex-end;
    }
  }

  .batch-actions-bar {
    flex-direction: column;
    gap: $spacing-3;

    .batch-buttons {
      width: 100%;
      flex-wrap: wrap;

      .el-button {
        flex: 1;
      }
    }
  }

  .stat-card .stat-content .stat-value {
    font-size: $font-size-2xl;
  }
}

@media (max-width: 480px) {
  .page-header {
    padding: $spacing-4;
  }

  .tasks-card,
  .filter-card {
    :deep(.el-card__header),
    :deep(.el-card__body) {
      padding: $spacing-4;
    }
  }

  .task-item {
    padding: $spacing-4;
  }
}
</style>
