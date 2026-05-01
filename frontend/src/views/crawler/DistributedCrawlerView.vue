<template>
  <div class="distributed-crawler" role="main" aria-label="分布式爬虫">
    <!-- Page Header -->
    <div class="page-header">
      <div class="header-left">
        <h1 class="page-title">
          <el-icon><Share /></el-icon>
          分布式爬虫管理
        </h1>
        <p class="page-description">
          管理分布式爬虫节点、监控任务执行状态、查看集群统计
        </p>
      </div>
      <div class="header-actions">
        <el-button type="primary" :icon="Plus" @click="showCreateTaskDialog = true">
          创建分布式任务
        </el-button>
        <el-button :icon="Setting" @click="showSettingsDialog = true">
          集群设置
        </el-button>
        <el-button :icon="Refresh" @click="refreshData" :loading="loading">
          刷新
        </el-button>
      </div>
    </div>

    <!-- Cluster Overview -->
    <el-row :gutter="16" class="cluster-overview">
      <el-col :xs="12" :sm="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon nodes">
              <el-icon><Monitor /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ clusterStats.totalNodes }}</div>
              <div class="stat-label">\{\{ \$t('workerNodes') \}\}</div>
            </div>
          </div>
          <div class="stat-detail">
            <span class="detail-item">
              <el-icon><CircleCheck /></el-icon>
              在线: {{ clusterStats.onlineNodes }}
            </span>
            <span class="detail-item">
              <el-icon><Connection /></el-icon>
              繁忙: {{ clusterStats.busyNodes }}
            </span>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="12" :sm="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon tasks">
              <el-icon><List /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ clusterStats.totalTasks }}</div>
              <div class="stat-label">\{\{ \$t('totalTasks') \}\}</div>
            </div>
          </div>
          <div class="stat-detail">
            <span class="detail-item">
              <el-icon><Loading /></el-icon>
              执行中: {{ clusterStats.runningTasks }}
            </span>
            <span class="detail-item">
              <el-icon><CircleCheck /></el-icon>
              完成: {{ clusterStats.completedTasks }}
            </span>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="12" :sm="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon papers">
              <el-icon><Document /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ clusterStats.totalPapers }}</div>
              <div class="stat-label">\{\{ \$t('totalPapers') \}\}</div>
            </div>
          </div>
          <div class="stat-detail">
            <span class="detail-item">今日: +{{ clusterStats.todayPapers }}</span>
            <span class="detail-item">成功率: {{ clusterStats.successRate }}%</span>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="12" :sm="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon performance">
              <el-icon><TrendCharts /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ clusterStats.avgResponseTime }}ms</div>
              <div class="stat-label">\{\{ \$t('avgResponseTime') \}\}</div>
            </div>
          </div>
          <div class="stat-detail">
            <span class="detail-item">吞吐: {{ clusterStats.throughput }}/min</span>
            <span class="detail-item">负载: {{ clusterStats.loadPercent }}%</span>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Main Content -->
    <el-row :gutter="16" class="main-content">
      <!-- Left: Workers List -->
      <el-col :xs="24" :lg="8">
        <el-card shadow="hover" class="workers-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><Monitor /></el-icon>
                工作节点
              </span>
              <div class="header-actions">
                <el-select v-model="workerFilter" size="small" style="width: 120px">
                  <el-option label="全部" value="all" />
                  <el-option label="在线" value="online" />
                  <el-option label="离线" value="offline" />
                  <el-option label="繁忙" value="busy" />
                </el-select>
              </div>
            </div>
          </template>

          <div class="workers-list">
            <div
              v-for="worker in filteredWorkers"
              :key="worker.nodeId"
              :class="['worker-item', { active: selectedWorker?.nodeId === worker.nodeId }]"
              @click="selectWorker(worker)"
            >
              <div class="worker-header">
                <div class="worker-name">
                  <el-icon
                    :class="{
                      'status-online': worker.status === 'ONLINE',
                      'status-offline': worker.status === 'OFFLINE',
                      'status-busy': worker.status === 'BUSY'
                    }"
                  >
                    <SuccessFilled />
                  </el-icon>
                  {{ worker.name || worker.nodeId }}
                </div>
                <el-tag :type="getWorkerStatusType(worker.status)" size="small">
                  {{ worker.status }}
                </el-tag>
              </div>

              <div class="worker-meta">
                <span class="meta-item">
                  <el-icon><Location /></el-icon>
                  {{ worker.location || 'Unknown' }}
                </span>
                <span class="meta-item">
                  <el-icon><Timer /></el-icon>
                  {{ worker.currentTasks }}/{{ worker.maxConcurrentTasks }}
                </span>
              </div>

              <div class="worker-stats">
                <div class="stat-item">
                  <span class="label">完成:</span>
                  <span class="value">{{ worker.tasksCompleted }}</span>
                </div>
                <div class="stat-item">
                  <span class="label">失败:</span>
                  <span class="value error">{{ worker.tasksFailed }}</span>
                </div>
                <div class="stat-item">
                  <span class="label">成功率:</span>
                  <span class="value">{{ worker.successRate }}%</span>
                </div>
              </div>

              <div class="worker-actions">
                <el-button size="small" text @click.stop="handleWorkerAction('disable', worker)" v-if="worker.status !== 'DISABLED'">
                  <el-icon><RemoveFilled /></el-icon> 禁用
                </el-button>
                <el-button size="small" text @click.stop="handleWorkerAction('enable', worker)" v-else>
                  <el-icon><CirclePlusFilled /></el-icon> 启用
                </el-button>
                <el-button size="small" text @click.stop="handleViewWorkerLogs(worker)">
                  <el-icon><Document /></el-icon> 日志
                </el-button>
              </div>
            </div>

            <el-empty v-if="filteredWorkers.length === 0" description="暂无工作节点" :image-size="60" />
          </div>
        </el-card>
      </el-col>

      <!-- Center: Task Queue -->
      <el-col :xs="24" :lg="8">
        <el-card shadow="hover" class="tasks-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><List /></el-icon>
                任务队列
              </span>
              <div class="header-actions">
                <el-select v-model="taskFilter" size="small" style="width: 120px">
                  <el-option label="全部" value="all" />
                  <el-option label="等待中" value="pending" />
                  <el-option label="执行中" value="running" />
                  <el-option label="已完成" value="completed" />
                </el-select>
              </div>
            </div>
          </template>

          <div class="tasks-queue">
            <div
              v-for="task in filteredTasks"
              :key="task.taskId"
              :class="['task-item', `priority-${task.priority.toLowerCase()}`]"
              @click="selectTask(task)"
            >
              <div class="task-header">
                <div class="task-name">{{ task.templateName || task.taskId }}</div>
                <el-tag :type="getPriorityType(task.priority)" size="small">
                  {{ task.priority }}
                </el-tag>
              </div>

              <div class="task-meta">
                <span class="meta-item">
                  <el-icon><Monitor /></el-icon>
                  {{ task.workerNodeId || '等待分配' }}
                </span>
                <span class="meta-item">
                  <el-icon><Clock /></el-icon>
                  {{ formatTime(task.createdAt) }}
                </span>
              </div>

              <div v-if="task.status === 'RUNNING'" class="task-progress">
                <el-progress
                  :percentage="task.progress || 0"
                  :indeterminate="task.progress === undefined"
                />
                <div class="progress-stats">
                  已爬取: {{ task.crawledCount || 0 }} 篇
                </div>
              </div>

              <div v-else-if="task.status === 'COMPLETED'" class="task-result">
                <el-icon class="success-icon"><CircleCheck /></el-icon>
                <span>完成: {{ task.resultCount || 0 }} 篇</span>
              </div>

              <div v-else-if="task.status === 'FAILED'" class="task-error">
                <el-icon class="error-icon"><CircleClose /></el-icon>
                <span>{{ task.errorMsg || '执行失败' }}</span>
              </div>

              <div class="task-actions">
                <el-button
                  v-if="task.status === 'PENDING'"
                  size="small"
                  type="primary"
                  @click.stop="handleAssignTask(task)"
                >
                  分配节点
                </el-button>
                <el-button
                  v-if="task.status === 'FAILED'"
                  size="small"
                  @click.stop="handleRetryTask(task)"
                >
                  重试
                </el-button>
                <el-button
                  v-if="task.status === 'RUNNING'"
                  size="small"
                  type="danger"
                  @click.stop="handleCancelTask(task)"
                >
                  取消
                </el-button>
                <el-button
                  size="small"
                  @click.stop="handleViewTaskDetails(task)"
                >
                  详情
                </el-button>
              </div>
            </div>

            <el-empty v-if="filteredTasks.length === 0" description="暂无任务" :image-size="60" />
          </div>
        </el-card>
      </el-col>

      <!-- Right: Worker Details / Task Details -->
      <el-col :xs="24" :lg="8">
        <!-- Worker Details -->
        <el-card v-if="selectedWorker" shadow="hover" class="details-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><Monitor /></el-icon>
                节点详情
              </span>
              <el-button size="small" text @click="selectedWorker = null">
                <el-icon><Close /></el-icon>
              </el-button>
            </div>
          </template>

          <div class="worker-details">
            <div class="detail-section">
              <h4>基本信息</h4>
              <div class="detail-grid">
                <div class="detail-item">
                  <span class="label">节点ID:</span>
                  <span class="value">{{ selectedWorker.nodeId }}</span>
                </div>
                <div class="detail-item">
                  <span class="label">类型:</span>
                  <span class="value">{{ selectedWorker.type }}</span>
                </div>
                <div class="detail-item">
                  <span class="label">IP地址:</span>
                  <span class="value">{{ selectedWorker.ipAddress }}</span>
                </div>
                <div class="detail-item">
                  <span class="label">位置:</span>
                  <span class="value">{{ selectedWorker.location || 'Unknown' }}</span>
                </div>
              </div>
            </div>

            <div class="detail-section">
              <h4>性能指标</h4>
              <div class="performance-chart">
                <div class="chart-item">
                  <span class="chart-label">平均响应时间</span>
                  <span class="chart-value">{{ selectedWorker.avgResponseTime }}ms</span>
                </div>
                <el-progress
                  :percentage="Math.min(100, (selectedWorker.currentTasks / selectedWorker.maxConcurrentTasks) * 100)"
                  :color="getLoadColor(selectedWorker.currentTasks / selectedWorker.maxConcurrentTasks)"
                >
                  <span class="progress-text">负载: {{ selectedWorker.currentTasks }}/{{ selectedWorker.maxConcurrentTasks }}</span>
                </el-progress>
              </div>
            </div>

            <div class="detail-section">
              <h4>当前任务</h4>
              <div v-if="selectedWorker.currentTaskList && selectedWorker.currentTaskList.length > 0" class="current-tasks">
                <div v-for="task in selectedWorker.currentTaskList" :key="task.taskId" class="current-task-item">
                  <span class="task-name">{{ task.templateName }}</span>
                  <el-progress :percentage="task.progress || 0" :show-text="false" />
                </div>
              </div>
              <el-empty v-else description="暂无执行中任务" :image-size="60" />
            </div>

            <div class="detail-section">
              <h4>支持的功能</h4>
              <div class="supported-features">
                <el-tag v-if="selectedWorker.supportsJsRendering" type="success" size="small">
                  JS渲染
                </el-tag>
                <el-tag
                  v-for="feature in selectedWorker.supportedTemplateTypes"
                  :key="feature"
                  size="small"
                >
                  {{ feature }}
                </el-tag>
              </div>
            </div>
          </div>
        </el-card>

        <!-- Task Details -->
        <el-card v-else-if="selectedTask" shadow="hover" class="details-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><List /></el-icon>
                任务详情
              </span>
              <el-button size="small" text @click="selectedTask = null">
                <el-icon><Close /></el-icon>
              </el-button>
            </div>
          </template>

          <div class="task-details">
            <div class="detail-section">
              <h4>基本信息</h4>
              <div class="detail-grid">
                <div class="detail-item">
                  <span class="label">任务ID:</span>
                  <span class="value">{{ selectedTask.taskId }}</span>
                </div>
                <div class="detail-item">
                  <span class="label">模板:</span>
                  <span class="value">{{ selectedTask.templateName }}</span>
                </div>
                <div class="detail-item">
                  <span class="label">优先级:</span>
                  <span class="value">{{ selectedTask.priority }}</span>
                </div>
                <div class="detail-item">
                  <span class="label">状态:</span>
                  <span class="value">{{ selectedTask.status }}</span>
                </div>
              </div>
            </div>

            <div class="detail-section">
              <h4>执行参数</h4>
              <div class="params-grid">
                <div v-for="(value, key) in selectedTask.parameters" :key="key" class="param-item">
                  <span class="param-key">{{ key }}:</span>
                  <span class="param-value">{{ value }}</span>
                </div>
              </div>
            </div>

            <div v-if="selectedTask.status === 'RUNNING'" class="detail-section">
              <h4>执行进度</h4>
              <el-progress :percentage="selectedTask.progress || 0" />
              <div class="progress-stats">
                <span>已爬取: {{ selectedTask.crawledCount || 0 }}</span>
                <span>成功: {{ selectedTask.successCount || 0 }}</span>
                <span>失败: {{ selectedTask.failedCount || 0 }}</span>
              </div>
            </div>

            <div v-if="selectedTask.status === 'COMPLETED'" class="detail-section">
              <h4>执行结果</h4>
              <div class="result-summary">
                <div class="result-item">
                  <span class="result-label">总结果数:</span>
                  <span class="result-value">{{ selectedTask.resultCount || 0 }}</span>
                </div>
                <div class="result-item">
                  <span class="result-label">执行时间:</span>
                  <span class="result-value">{{ selectedTask.executionTime || 0 }}s</span>
                </div>
              </div>
              <el-button type="primary" size="small" @click="handleViewResults(selectedTask)">
                查看结果
              </el-button>
            </div>

            <div v-if="selectedTask.errorMsg" class="detail-section">
              <h4>错误信息</h4>
              <el-alert type="error" :closable="false">
                {{ selectedTask.errorMsg }}
              </el-alert>
            </div>
          </div>
        </el-card>

        <!-- Default View -->
        <el-card v-else shadow="hover" class="details-card">
          <div class="empty-details">
            <el-empty description="选择一个工作节点或任务查看详情" :image-size="100">
              <el-icon><Share /></el-icon>
              <p>分布式爬虫系统</p>
              <p>实时监控节点状态和任务执行</p>
            </el-empty>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Create Task Dialog -->
    <el-dialog
      v-model="showCreateTaskDialog"
      title="创建分布式爬虫任务"
      width="600px"
    >
      <el-form :model="newTask" label-width="120px">
        <el-form-item label="爬虫模板">
          <el-select v-model="newTask.templateId" placeholder="选择模板" style="width: 100%">
            <el-option
              v-for="template in templates"
              :key="template.id"
              :label="template.name"
              :value="template.id"
            />
          </el-select>
        </el-form-item>

        <el-form-item label="任务优先级">
          <el-select v-model="newTask.priority" style="width: 100%">
            <el-option label="低" value="LOW" />
            <el-option label="普通" value="NORMAL" />
            <el-option label="高" value="HIGH" />
            <el-option label="紧急" value="URGENT" />
          </el-select>
        </el-form-item>

        <el-form-item label="目标节点">
          <el-select v-model="newTask.workerNodeId" placeholder="自动分配" clearable style="width: 100%">
            <el-option
              v-for="worker in onlineWorkers"
              :key="worker.nodeId"
              :label="worker.name || worker.nodeId"
              :value="worker.nodeId"
            />
          </el-select>
          <div class="form-tip">留空则自动分配最优节点</div>
        </el-form-item>

        <el-form-item label="任务参数">
          <el-input
            v-model="newTask.parametersJson"
            type="textarea"
            :rows="4"
            placeholder='{"keywords": "AI", "maxResults": 100}'
          />
        </el-form-item>
      </el-form>

      <template #footer>
        <el-button @click="showCreateTaskDialog = false">取消</el-button>
        <el-button type="primary" @click="handleCreateTask">创建任务</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import {
  Plus,
  Setting,
  Refresh,
  Share,
  Monitor,
  List,
  Document,
  Loading,
  CircleCheck,
  Connection,
  Location,
  Timer,
  TrendCharts,
  Clock,
  RemoveFilled,
  CirclePlusFilled,
  Close,
  SuccessFilled,
  CircleClose
} from '@element-plus/icons-vue'

// Types
interface WorkerNode {
  nodeId: string
  name?: string
  type: string
  status: 'ONLINE' | 'OFFLINE' | 'BUSY' | 'DISABLED'
  location?: string
  ipAddress: string
  maxConcurrentTasks: number
  currentTasks: number
  tasksCompleted: number
  tasksFailed: number
  successRate: number
  avgResponseTime: number
  supportsJsRendering: boolean
  supportedTemplateTypes: string[]
  currentTaskList?: Task[]
}

interface Task {
  taskId: string
  templateId: string
  templateName: string
  priority: 'LOW' | 'NORMAL' | 'HIGH' | 'URGENT'
  status: 'PENDING' | 'RUNNING' | 'COMPLETED' | 'FAILED' | 'CANCELLED'
  workerNodeId?: string
  createdAt: string
  progress?: number
  crawledCount?: number
  successCount?: number
  failedCount?: number
  resultCount?: number
  executionTime?: number
  errorMsg?: string
  parameters: Record<string, string>
}

// Data
const workers = ref<WorkerNode[]>([])
const tasks = ref<Task[]>([])
const templates = ref<any[]>([])
const selectedWorker = ref<WorkerNode | null>(null)
const selectedTask = ref<Task | null>(null)
const workerFilter = ref('all')
const taskFilter = ref('all')
const loading = ref(false)
const showCreateTaskDialog = ref(false)
const showSettingsDialog = ref(false)

const newTask = ref({
  templateId: '',
  priority: 'NORMAL',
  workerNodeId: '',
  parametersJson: '{}'
})

// Computed
const clusterStats = computed(() => ({
  totalNodes: workers.value.length,
  onlineNodes: workers.value.filter(w => w.status === 'ONLINE').length,
  busyNodes: workers.value.filter(w => w.status === 'BUSY').length,
  totalTasks: tasks.value.length,
  runningTasks: tasks.value.filter(t => t.status === 'RUNNING').length,
  completedTasks: tasks.value.filter(t => t.status === 'COMPLETED').length,
  totalPapers: 1234,
  todayPapers: 56,
  successRate: 95,
  avgResponseTime: 450,
  throughput: 120,
  loadPercent: 65
}))

const filteredWorkers = computed(() => {
  if (workerFilter.value === 'all') return workers.value
  if (workerFilter.value === 'online') return workers.value.filter(w => w.status === 'ONLINE')
  if (workerFilter.value === 'offline') return workers.value.filter(w => w.status === 'OFFLINE')
  if (workerFilter.value === 'busy') return workers.value.filter(w => w.status === 'BUSY')
  return workers.value
})

const filteredTasks = computed(() => {
  if (taskFilter.value === 'all') return tasks.value
  if (taskFilter.value === 'pending') return tasks.value.filter(t => t.status === 'PENDING')
  if (taskFilter.value === 'running') return tasks.value.filter(t => t.status === 'RUNNING')
  if (taskFilter.value === 'completed') return tasks.value.filter(t => t.status === 'COMPLETED')
  return tasks.value
})

const onlineWorkers = computed(() =>
  workers.value.filter(w => w.status === 'ONLINE')
)

// Methods
const refreshData = async () => {
  loading.value = true
  try {
    // Load workers from API
    const workersRes = await fetch('/api/crawler/workers')
    const workersData = await workersRes.json()
    workers.value = workersData.items || []

    // Load tasks from API
    const tasksRes = await fetch('/api/crawler/tasks')
    const tasksData = await tasksRes.json()
    tasks.value = tasksData.items || []

    // Load templates
    const templatesRes = await fetch('/api/crawler/templates')
    const templatesData = await templatesRes.json()
    templates.value = templatesData.items || []
  } catch (error) {
    console.error('Failed to load data:', error)
  } finally {
    loading.value = false
  }
}

const selectWorker = (worker: WorkerNode) => {
  selectedWorker.value = worker
  selectedTask.value = null
}

const selectTask = (task: Task) => {
  selectedTask.value = task
  selectedWorker.value = null
}

const handleWorkerAction = async (action: string, worker: WorkerNode) => {
  try {
    if (action === 'disable') {
      await fetch(`/api/crawler/workers/${worker.nodeId}/disable`, { method: 'POST' })
      ElMessage.success('节点已禁用')
    } else if (action === 'enable') {
      await fetch(`/api/crawler/workers/${worker.nodeId}/enable`, { method: 'POST' })
      ElMessage.success('节点已启用')
    }
    refreshData()
  } catch (error) {
    ElMessage.error('操作失败')
  }
}

const handleAssignTask = async (task: Task) => {
  try {
    const onlineWorker = onlineWorkers.value[0]
    if (!onlineWorker) {
      ElMessage.warning('没有可用的在线节点')
      return
    }

    await fetch(`/api/crawler/tasks/${task.taskId}/assign`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ workerNodeId: onlineWorker.nodeId })
    })

    ElMessage.success('任务已分配')
    refreshData()
  } catch (error) {
    ElMessage.error('分配失败')
  }
}

const handleRetryTask = async (task: Task) => {
  try {
    await fetch(`/api/crawler/tasks/${task.taskId}/retry`, { method: 'POST' })
    ElMessage.success('任务已重新提交')
    refreshData()
  } catch (error) {
    ElMessage.error('重试失败')
  }
}

const handleCancelTask = async (task: Task) => {
  try {
    await fetch(`/api/crawler/tasks/${task.taskId}`, { method: 'DELETE' })
    ElMessage.success('任务已取消')
    refreshData()
  } catch (error) {
    ElMessage.error('取消失败')
  }
}

const handleViewWorkerLogs = (worker: WorkerNode) => {
  // Show worker logs
  ElMessage.info('日志功能开发中')
}

const handleViewTaskDetails = (task: Task) => {
  selectTask(task)
}

const handleViewResults = (task: Task) => {
  ElMessage.info('结果查看功能开发中')
}

const handleCreateTask = async () => {
  try {
    let parameters = {}
    try {
      parameters = JSON.parse(newTask.value.parametersJson)
    } catch {
      ElMessage.error('参数格式错误')
      return
    }

    await fetch('/api/crawler/tasks', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        templateId: newTask.value.templateId,
        priority: newTask.value.priority,
        workerNodeId: newTask.value.workerNodeId || undefined,
        parameters
      })
    })

    ElMessage.success('任务已创建')
    showCreateTaskDialog.value = false
    refreshData()
  } catch (error) {
    ElMessage.error('创建失败')
  }
}

const getWorkerStatusType = (status: string) => {
  const types: Record<string, any> = {
    ONLINE: 'success',
    OFFLINE: 'danger',
    BUSY: 'warning',
    DISABLED: 'info'
  }
  return types[status] || 'info'
}

const getPriorityType = (priority: string) => {
  const types: Record<string, any> = {
    LOW: 'info',
    NORMAL: '',
    HIGH: 'warning',
    URGENT: 'danger'
  }
  return types[priority] || ''
}

const formatTime = (time: string) => {
  const date = new Date(time)
  const now = new Date()
  const diff = Math.floor((now.getTime() - date.getTime()) / 1000)

  if (diff < 60) return `${diff}秒前`
  if (diff < 3600) return `${Math.floor(diff / 60)}分钟前`
  if (diff < 86400) return `${Math.floor(diff / 3600)}小时前`
  return date.toLocaleDateString()
}

const getLoadColor = (ratio: number) => {
  if (ratio < 0.5) return '#67c23a'
  if (ratio < 0.8) return '#e6a23c'
  return '#f56c6c'
}

// Lifecycle
onMounted(() => {
  refreshData()
})
</script>

<style scoped lang="scss">
.distributed-crawler {
  padding: 24px;
  max-width: 1600px;
  margin: 0 auto;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 24px;

  .header-left {
    flex: 1;
  }

  .page-title {
    font-size: 28px;
    font-weight: 600;
    margin: 0 0 8px 0;
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .page-description {
    color: var(--el-text-color-secondary);
    margin: 0;
  }

  .header-actions {
    display: flex;
    gap: 12px;
  }
}

.cluster-overview {
  margin-bottom: 24px;
}

.stat-card {
  .stat-content {
    display: flex;
    align-items: center;
    gap: 16px;
    margin-bottom: 12px;
  }

  .stat-icon {
    width: 48px;
    height: 48px;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: 12px;
    font-size: 24px;

    &.nodes {
      background: #e3f2fd;
      color: #1976d2;
    }

    &.tasks {
      background: #f3e5f5;
      color: #7b1fa2;
    }

    &.papers {
      background: #e8f5e9;
      color: #388e3c;
    }

    &.performance {
      background: #fff3e0;
      color: #f57c00;
    }
  }

  .stat-value {
    font-size: 24px;
    font-weight: 600;
  }

  .stat-label {
    font-size: 14px;
    color: var(--el-text-color-secondary);
  }

  .stat-detail {
    display: flex;
    gap: 16px;
    font-size: 12px;
    color: var(--el-text-color-secondary);

    .detail-item {
      display: flex;
      align-items: center;
      gap: 4px;
    }
  }
}

.main-content {
  margin-top: 24px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-weight: 600;
}

.workers-list,
.tasks-queue {
  max-height: 600px;
  overflow-y: auto;
}

.worker-item,
.task-item {
  padding: 12px;
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  margin-bottom: 12px;
  cursor: pointer;
  transition: all 0.3s;

  &:hover {
    border-color: var(--el-color-primary);
  }

  &.active {
    background: var(--el-color-primary-light-9);
    border-color: var(--el-color-primary);
  }

  &.priority-urgent {
    border-left: 4px solid #f56c6c;
  }

  &.priority-high {
    border-left: 4px solid #e6a23c;
  }

  .worker-header,
  .task-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 8px;
  }

  .worker-name {
    display: flex;
    align-items: center;
    gap: 8px;
    font-weight: 600;

    .status-online {
      color: #67c23a;
    }

    .status-offline {
      color: #909399;
    }

    .status-busy {
      color: #e6a23c;
    }
  }

  .worker-meta,
  .task-meta {
    display: flex;
    gap: 16px;
    font-size: 12px;
    color: var(--el-text-color-secondary);
    margin-bottom: 8px;

    .meta-item {
      display: flex;
      align-items: center;
      gap: 4px;
    }
  }

  .worker-stats {
    display: flex;
    gap: 16px;
    margin-bottom: 8px;
    font-size: 12px;

    .stat-item {
      .label {
        color: var(--el-text-color-secondary);
      }

      .value {
        font-weight: 600;

        &.error {
          color: var(--el-color-danger);
        }
      }
    }
  }

  .worker-actions,
  .task-actions {
    display: flex;
    gap: 8px;
  }

  .task-progress {
    margin: 8px 0;

    .progress-stats {
      font-size: 12px;
      color: var(--el-text-color-secondary);
      margin-top: 4px;
    }
  }

  .task-result {
    display: flex;
    align-items: center;
    gap: 8px;
    color: var(--el-color-success);
  }

  .task-error {
    display: flex;
    align-items: center;
    gap: 8px;
    color: var(--el-color-danger);
  }
}

.worker-details,
.task-details {
  .detail-section {
    margin-bottom: 24px;

    h4 {
      margin: 0 0 12px 0;
      font-size: 16px;
      font-weight: 600;
    }
  }

  .detail-grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 12px;

    .detail-item {
      .label {
        color: var(--el-text-color-secondary);
        margin-right: 8px;
      }

      .value {
        font-weight: 500;
      }
    }
  }

  .performance-chart {
    .chart-item {
      display: flex;
      justify-content: space-between;
      margin-bottom: 8px;
    }

    .progress-text {
      font-size: 12px;
    }
  }

  .current-tasks {
    .current-task-item {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-bottom: 8px;

      .task-name {
        flex: 1;
        font-size: 14px;
      }
    }
  }

  .supported-features {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
  }

  .params-grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 8px;

    .param-item {
      font-size: 12px;

      .param-key {
        color: var(--el-text-color-secondary);
        margin-right: 8px;
      }

      .param-value {
        font-weight: 500;
      }
    }
  }

  .progress-stats {
    display: flex;
    gap: 16px;
    margin-top: 8px;
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }

  .result-summary {
    .result-item {
      display: flex;
      justify-content: space-between;
      margin-bottom: 8px;
    }

    .result-label {
      color: var(--el-text-color-secondary);
    }

    .result-value {
      font-weight: 600;
    }
  }
}

.empty-details {
  .el-empty {
    padding: 60px 20px;

    p {
      margin: 8px 0;
      color: var(--el-text-color-secondary);
    }

    .el-icon {
      font-size: 60px;
      color: var(--el-color-primary);
    }
  }
}

.form-tip {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin-top: 4px;
}

/* Dark mode */
[data-theme="dark"] .distributed-crawler {
  color: #f3f4f6;
}

[data-theme="dark"] .distributed-crawler .page-header,
[data-theme="dark"] .distributed-crawler .page-title,
[data-theme="dark"] .distributed-crawler .card-header {
  color: #f3f4f6;
}

[data-theme="dark"] .distributed-crawler .page-subtitle,
[data-theme="dark"] .distributed-crawler .page-description {
  color: #9ca3af;
}

[data-theme="dark"] .distributed-crawler .stat-card,
[data-theme="dark"] .distributed-crawler .filter-card,
[data-theme="dark"] .distributed-crawler .tasks-card,
[data-theme="dark"] .distributed-crawler .chart-card,
[data-theme="dark"] .distributed-crawler .table-card,
[data-theme="dark"] .distributed-crawler .form-card,
[data-theme="dark"] .distributed-crawler .detail-header,
[data-theme="dark"] .distributed-crawler .detail-content,
[data-theme="dark"] .distributed-crawler .toolbar,
[data-theme="dark"] .distributed-crawler .export-options {
  background: rgba(40, 40, 45, 0.98) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #f3f4f6;
}

[data-theme="dark"] .distributed-crawler .stat-value,
[data-theme="dark"] .distributed-crawler .metric-value {
  color: #f3f4f6;
}

[data-theme="dark"] .distributed-crawler .stat-label,
[data-theme="dark"] .distributed-crawler .metric-label {
  color: #9ca3af;
}

[data-theme="dark"] .distributed-crawler .empty-state,
[data-theme="dark"] .distributed-crawler .empty-text {
  color: #9ca3af;
}
</style>
