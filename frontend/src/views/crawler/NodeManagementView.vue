<template>
  <div class="node-management" role="main" aria-label="节点管理">
    <!-- Page Header -->
    <div class="page-header">
      <h1 class="page-title">
        <el-icon><Monitor /></el-icon>
        节点管理
      </h1>
      <div class="header-actions">
        <el-button type="primary" :icon="Plus" @click="handleAddNode">
          添加节点
        </el-button>
        <el-button :icon="Refresh" @click="refreshNodes" :loading="loading">
          刷新
        </el-button>
      </div>
    </div>

    <!-- Statistics -->
    <el-row :gutter="20" class="stats-row">
      <el-col :xs="12" :sm="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon total">
              <el-icon><Monitor /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ nodes.length }}</div>
              <div class="stat-label">总节点数</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="12" :sm="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon healthy">
              <el-icon><CircleCheck /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ healthyNodesCount }}</div>
              <div class="stat-label">\{\{ \$t('healthyNode') \}\}</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="12" :sm="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon active">
              <el-icon><Loading /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ totalActiveTasks }}</div>
              <div class="stat-label">\{\{ \$t('activeTasks') \}\}</div>
            </div>
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
              <div class="stat-value">{{ avgSuccessRate }}%</div>
              <div class="stat-label">平均成功率</div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Performance Charts -->
    <el-row :gutter="20" class="charts-row">
      <el-col :xs="24" :lg="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><TrendCharts /></el-icon>
                节点性能趋势
              </span>
            </div>
          </template>

          <div class="chart-container">
            <div ref="performanceChartRef" class="chart"></div>
          </div>
        </el-card>
      </el-col>

      <el-col :xs="24" :lg="12">
        <el-card shadow="hover" class="chart-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><PieChart /></el-icon>
                任务分布
              </span>
            </div>
          </template>

          <div class="chart-container">
            <div ref="distributionChartRef" class="chart"></div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Nodes List -->
    <el-card shadow="hover" class="nodes-card">
      <template #header>
        <div class="card-header">
          <span>
            <el-icon><List /></el-icon>
            节点列表
          </span>
          <el-radio-group v-model="viewMode" size="small">
            <el-radio-button label="grid">网格</el-radio-button>
            <el-radio-button label="list">列表</el-radio-button>
          </el-radio-group>
        </div>
      </template>

      <div v-if="nodes.length === 0" class="empty-state">
        <el-empty description="暂无节点">
          <el-button type="primary" @click="handleAddNode">
            添加节点
          </el-button>
        </el-empty>
      </div>

      <!-- Grid View -->
      <div v-else-if="viewMode === 'grid'" class="nodes-grid">
        <el-row :gutter="20">
          <el-col
            v-for="node in nodes"
            :key="node.id"
            :xs="24"
            :sm="12"
            :lg="8"
          >
            <el-card shadow="hover" class="node-card">
              <div class="node-header">
                <div class="node-status" :class="{ healthy: node.healthy }">
                  <span class="dot"></span>
                  {{ node.healthy ? '健康' : '异常' }}
                </div>
                <el-dropdown @command="(cmd) => handleNodeCommand(cmd, node)">
                  <el-icon class="more-icon"><MoreFilled /></el-icon>
                  <template #dropdown>
                    <el-dropdown-menu>
                      <el-dropdown-item command="edit" :icon="Edit">
                        编辑
                      </el-dropdown-item>
                      <el-dropdown-item
                        command="toggle"
                        :icon="node.enabled ? VideoPause : VideoPlay"
                      >
                        {{ node.enabled ? '禁用' : '启用' }}
                      </el-dropdown-item>
                      <el-dropdown-item command="test" :icon="Connection">
                        测试连接
                      </el-dropdown-item>
                      <el-dropdown-item command="delete" :icon="Delete" divided>
                        删除
                      </el-dropdown-item>
                    </el-dropdown-menu>
                  </template>
                </el-dropdown>
              </div>

              <div class="node-body">
                <h3 class="node-name">{{ node.name }}</h3>
                <div class="node-info">
                  <div class="info-item">
                    <el-icon><Location /></el-icon>
                    <span>{{ node.url }}</span>
                  </div>
                  <div class="info-item">
                    <el-icon><Timer /></el-icon>
                    <span>延迟: {{ node.latency }}ms</span>
                  </div>
                  <div class="info-item">
                    <el-icon><Files /></el-icon>
                    <span>任务: {{ node.activeTasks }}</span>
                  </div>
                  <div class="info-item">
                    <el-icon><TrendCharts /></el-icon>
                    <span>成功率: {{ node.successRate }}%</span>
                  </div>
                </div>
              </div>

              <div class="node-footer">
                <el-button
                  type="primary"
                  size="small"
                  @click="handleViewDetails(node)"
                  style="width: 100%"
                >
                  查看详情
                </el-button>
              </div>
            </el-card>
          </el-col>
        </el-row>
      </div>

      <!-- List View -->
      <el-table v-else :data="nodes" v-loading="loading" border stripe>
        <el-table-column prop="name" label="节点名称" min-width="150" />
        <el-table-column prop="url" label="地址" min-width="200" show-overflow-tooltip />
        <el-table-column label="状态" width="100" align="center">
          <template #default="{ row }">
            <el-tag :type="row.healthy ? 'success' : 'danger'">
              {{ row.healthy ? '健康' : '异常' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="latency" label="延迟" width="100" align="center">
          <template #default="{ row }">
            {{ row.latency }}ms
          </template>
        </el-table-column>
        <el-table-column prop="activeTasks" label="活跃任务" width="100" align="center" />
        <el-table-column prop="successRate" label="成功率" width="100" align="center">
          <template #default="{ row }">
            {{ row.successRate }}%
          </template>
        </el-table-column>
        <el-table-column label="操作" width="200" fixed="right">
          <template #default="{ row }">
            <el-button
              link
              type="primary"
              :icon="Edit"
              @click="handleEditNode(row)"
            >
              编辑
            </el-button>
            <el-button
              link
              type="warning"
              :icon="row.enabled ? VideoPause : VideoPlay"
              @click="handleToggleNode(row)"
            >
              {{ row.enabled ? '禁用' : '启用' }}
            </el-button>
            <el-button
              link
              type="danger"
              :icon="Delete"
              @click="handleDeleteNode(row)"
            >
              删除
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- Node Details Dialog -->
    <el-dialog
      v-model="detailsDialogVisible"
      :title="`节点详情 - ${currentNode?.name}`"
      width="70%"
    >
      <div v-if="currentNode" class="node-details">
        <el-tabs>
          <el-tab-pane label="基本信息">
            <el-descriptions :column="2" border>
              <el-descriptions-item label="节点ID">
                {{ currentNode.id }}
              </el-descriptions-item>
              <el-descriptions-item label="节点名称">
                {{ currentNode.name }}
              </el-descriptions-item>
              <el-descriptions-item label="地址">
                {{ currentNode.url }}
              </el-descriptions-item>
              <el-descriptions-item label="状态">
                <el-tag :type="currentNode.healthy ? 'success' : 'danger'">
                  {{ currentNode.healthy ? '健康' : '异常' }}
                </el-tag>
              </el-descriptions-item>
              <el-descriptions-item label="启用状态">
                <el-tag :type="currentNode.enabled ? 'success' : 'info'">
                  {{ currentNode.enabled ? '已启用' : '已禁用' }}
                </el-tag>
              </el-descriptions-item>
              <el-descriptions-item label="延迟">
                {{ currentNode.latency }}ms
              </el-descriptions-item>
              <el-descriptions-item label="活跃任务">
                {{ currentNode.activeTasks }}
              </el-descriptions-item>
              <el-descriptions-item label="成功率">
                {{ currentNode.successRate }}%
              </el-descriptions-item>
            </el-descriptions>
          </el-tab-pane>

          <el-tab-pane label="性能指标">
            <div class="performance-metrics">
              <el-row :gutter="20">
                <el-col :span="12">
                  <div class="metric-card">
                    <div class="metric-label">平均响应时间</div>
                    <div class="metric-value">{{ currentNode.latency }}ms</div>
                  </div>
                </el-col>
                <el-col :span="12">
                  <div class="metric-card">
                    <div class="metric-label">总任务数</div>
                    <div class="metric-value">{{ currentNode.totalTasks || 0 }}</div>
                  </div>
                </el-col>
                <el-col :span="12">
                  <div class="metric-card">
                    <div class="metric-label">成功任务</div>
                    <div class="metric-value">{{ currentNode.successTasks || 0 }}</div>
                  </div>
                </el-col>
                <el-col :span="12">
                  <div class="metric-card">
                    <div class="metric-label">失败任务</div>
                    <div class="metric-value">{{ currentNode.failedTasks || 0 }}</div>
                  </div>
                </el-col>
              </el-row>
            </div>
          </el-tab-pane>

          <el-tab-pane label="历史记录">
            <el-timeline>
              <el-timeline-item
                v-for="(log, index) in mockLogs"
                :key="index"
                :timestamp="log.time"
                :type="log.type"
              >
                {{ log.message }}
              </el-timeline-item>
            </el-timeline>
          </el-tab-pane>
        </el-tabs>
      </div>

      <template #footer>
        <el-button @click="detailsDialogVisible = false">关闭</el-button>
      </template>
    </el-dialog>

    <!-- Add/Edit Node Dialog -->
    <el-dialog
      v-model="nodeFormDialogVisible"
      :title="isEditMode ? '编辑节点' : '添加节点'"
      width="50%"
    >
      <el-form :model="nodeForm" label-width="100px">
        <el-form-item label="节点名称" required>
          <el-input v-model="nodeForm.name" placeholder="输入节点名称" />
        </el-form-item>
        <el-form-item label="节点地址" required>
          <el-input v-model="nodeForm.url" placeholder="http://localhost:8080" />
        </el-form-item>
        <el-form-item label="描述">
          <el-input
            v-model="nodeForm.description"
            type="textarea"
            :rows="3"
            placeholder="节点描述（可选）"
          />
        </el-form-item>
      </el-form>

      <template #footer>
        <el-button @click="nodeFormDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleSaveNode">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  Monitor,
  Plus,
  Refresh,
  CircleCheck,
  Loading,
  TrendCharts,
  PieChart,
  List,
  Edit,
  Delete,
  VideoPlay,
  VideoPause,
  Connection,
  MoreFilled,
  Location,
  Timer,
  Files
} from '@element-plus/icons-vue'

// State
const loading = ref(false)
const viewMode = ref<'grid' | 'list'>('grid')
const detailsDialogVisible = ref(false)
const nodeFormDialogVisible = ref(false)
const isEditMode = ref(false)

const nodes = ref<Array<{
  id: string
  name: string
  url: string
  healthy: boolean
  enabled: boolean
  latency: number
  activeTasks: number
  successRate: number
  totalTasks?: number
  successTasks?: number
  failedTasks?: number
  description?: string
}>>([])

const currentNode = ref<typeof nodes.value[0] | null>(null)
const performanceChartRef = ref<HTMLElement | null>(null)
const distributionChartRef = ref<HTMLElement | null>(null)

const nodeForm = ref({
  id: '',
  name: '',
  url: '',
  description: ''
})

const mockLogs = ref<Array<{
  time: string
  type: 'primary' | 'success' | 'warning' | 'danger'
  message: string
}>>([])

// Computed
const healthyNodesCount = computed(() => nodes.value.filter(n => n.healthy).length)
const totalActiveTasks = computed(() => nodes.value.reduce((sum, n) => sum + n.activeTasks, 0))
const avgSuccessRate = computed(() => {
  if (nodes.value.length === 0) return 0
  return Math.round(nodes.value.reduce((sum, n) => sum + n.successRate, 0) / nodes.value.length)
})

// Methods
const refreshNodes = async () => {
  loading.value = true
  try {
    // Simulate API call
    await new Promise(resolve => setTimeout(resolve, 1000))
    ElMessage.success('刷新成功')
  } catch (error: any) {
    ElMessage.error('刷新失败：' + error.message)
  } finally {
    loading.value = false
  }
}

const handleAddNode = () => {
  isEditMode.value = false
  nodeForm.value = {
    id: '',
    name: '',
    url: '',
    description: ''
  }
  nodeFormDialogVisible.value = true
}

const handleEditNode = (node: typeof nodes.value[0]) => {
  isEditMode.value = true
  nodeForm.value = {
    id: node.id,
    name: node.name,
    url: node.url,
    description: node.description || ''
  }
  nodeFormDialogVisible.value = true
}

const handleSaveNode = async () => {
  if (!nodeForm.value.name || !nodeForm.value.url) {
    ElMessage.warning('请填写节点名称和地址')
    return
  }

  try {
    if (isEditMode.value) {
      const index = nodes.value.findIndex(n => n.id === nodeForm.value.id)
      if (index !== -1) {
        nodes.value[index] = {
          ...nodes.value[index],
          name: nodeForm.value.name,
          url: nodeForm.value.url,
          description: nodeForm.value.description
        }
      }
      ElMessage.success('节点更新成功')
    } else {
      const newNode = {
        id: `node_${Date.now()}`,
        name: nodeForm.value.name,
        url: nodeForm.value.url,
        description: nodeForm.value.description,
        healthy: true,
        enabled: true,
        latency: Math.floor(Math.random() * 200) + 50,
        activeTasks: 0,
        successRate: 100,
        totalTasks: 0,
        successTasks: 0,
        failedTasks: 0
      }
      nodes.value.push(newNode)
      ElMessage.success('节点添加成功')
    }
    nodeFormDialogVisible.value = false
  } catch (error: any) {
    ElMessage.error('保存失败：' + error.message)
  }
}

const handleToggleNode = async (node: typeof nodes.value[0]) => {
  try {
    node.enabled = !node.enabled
    ElMessage.success(`节点已${node.enabled ? '启用' : '禁用'}`)
  } catch (error: any) {
    ElMessage.error('操作失败：' + error.message)
  }
}

const handleDeleteNode = async (node: typeof nodes.value[0]) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除节点 "${node.name}" 吗？`,
      '确认删除',
      { type: 'warning' }
    )

    const index = nodes.value.findIndex(n => n.id === node.id)
    if (index !== -1) {
      nodes.value.splice(index, 1)
    }
    ElMessage.success('删除成功')
  } catch {
    // User cancelled
  }
}

const handleNodeCommand = async (command: string, node: typeof nodes.value[0]) => {
  switch (command) {
    case 'edit':
      handleEditNode(node)
      break
    case 'toggle':
      handleToggleNode(node)
      break
    case 'test':
      ElMessage.info('正在测试连接...')
      await new Promise(resolve => setTimeout(resolve, 1000))
      node.latency = Math.floor(Math.random() * 200) + 50
      node.healthy = Math.random() > 0.2
      ElMessage.success(`连接测试完成，延迟: ${node.latency}ms`)
      break
    case 'delete':
      handleDeleteNode(node)
      break
  }
}

const handleViewDetails = (node: typeof nodes.value[0]) => {
  currentNode.value = node

  // Generate mock logs
  mockLogs.value = [
    {
      time: '2024-01-15 10:30:00',
      type: 'success',
      message: '节点启动成功'
    },
    {
      time: '2024-01-15 10:35:00',
      type: 'primary',
      message: '接受新任务: task_123'
    },
    {
      time: '2024-01-15 10:40:00',
      type: 'success',
      message: '任务完成: task_123'
    },
    {
      time: '2024-01-15 10:45:00',
      type: 'warning',
      message: '高负载警告: CPU 85%'
    },
    {
      time: '2024-01-15 10:50:00',
      type: 'danger',
      message: '连接超时: 重试中...'
    }
  ]

  detailsDialogVisible.value = true
}

// Lifecycle
onMounted(async () => {
  // Initialize with mock data
  nodes.value = [
    {
      id: 'node_1',
      name: '主节点',
      url: 'http://localhost:8080',
      description: '主要爬虫节点',
      healthy: true,
      enabled: true,
      latency: 45,
      activeTasks: 3,
      successRate: 98,
      totalTasks: 1250,
      successTasks: 1225,
      failedTasks: 25
    },
    {
      id: 'node_2',
      name: '备用节点',
      url: 'http://localhost:8081',
      description: '备用爬虫节点',
      healthy: true,
      enabled: true,
      latency: 120,
      activeTasks: 1,
      successRate: 95,
      totalTasks: 800,
      successTasks: 760,
      failedTasks: 40
    },
    {
      id: 'node_3',
      name: '远程节点',
      url: 'http://remote.example.com',
      description: '远程服务器节点',
      healthy: false,
      enabled: false,
      latency: 0,
      activeTasks: 0,
      successRate: 85,
      totalTasks: 500,
      successTasks: 425,
      failedTasks: 75
    }
  ]

  await refreshNodes()
})
</script>

<style scoped lang="scss">
// ==========================================
// 节点管理页面现代化样式
// Modern Node Management Styles
// ==========================================

.node-management {
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

// 统计和图表行
.stats-row,
.charts-row {
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

  &:hover {
    box-shadow: $shadow-md;
    transform: translateY(-2px);
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  :deep(.el-card__body) {
    padding: $spacing-5;
  }

  .stat-content {
    display: flex;
    align-items: center;
    gap: $spacing-5;

    .stat-icon {
      width: 56px;
      height: 56px;
      border-radius: $border-radius-lg;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 24px;
      color: white;
      box-shadow: $shadow-sm;
      flex-shrink: 0;

      &.total {
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      }

      &.healthy {
        background: linear-gradient(135deg, #43e97b 0%, #38f9d7 100%);
      }

      &.active {
        background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
      }

      &.performance {
        background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
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

// 图表卡片
.chart-card {
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

  .chart-container {
    min-height: 300px;
    display: flex;
    align-items: center;
    justify-content: center;

    .chart {
      width: 100%;
      height: 300px;
      background: $gray-50;
      border-radius: $border-radius-lg;
      display: flex;
      align-items: center;
      justify-content: center;
      color: $text-secondary;
      font-size: $font-size-sm;

      .dark & {
        background: $gray-900;
      }
    }
  }
}

// 节点卡片
.nodes-card {
  margin-top: $spacing-6;
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

// 节点网格
.nodes-grid {
  .node-card {
    margin-bottom: $spacing-5;
    height: 100%;
    display: flex;
    flex-direction: column;
    border: 1px solid $border-light;
    border-radius: $border-radius-lg;
    box-shadow: $shadow-sm;
    transition: all $duration-slow;
    overflow: hidden;

    &:hover {
      box-shadow: $shadow-md;
      transform: translateY(-4px);
    }

    .dark & {
      background: $gray-800;
      border-color: $gray-700;
    }

    :deep(.el-card__body) {
      padding: 0;
      height: 100%;
      display: flex;
      flex-direction: column;
    }

    .node-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: $spacing-5;
      background: linear-gradient(135deg, $gray-50 0%, #ffffff 100%);
      border-bottom: 1px solid $border-light;

      .dark & {
        background: linear-gradient(135deg, $gray-800 0%, $gray-700 100%);
        border-bottom-color: $gray-700;
      }

      .node-status {
        display: flex;
        align-items: center;
        gap: $spacing-2;
        font-size: $font-size-sm;
        font-weight: $font-weight-medium;
        padding: $spacing-2 $spacing-3;
        border-radius: $border-radius-full;

        &.healthy {
          background: rgba($success-color, 0.1);
          color: $success-color;
        }

        &:not(.healthy) {
          background: rgba($danger-color, 0.1);
          color: $danger-color;
        }

        .dot {
          width: 8px;
          height: 8px;
          border-radius: 50%;
          background: currentColor;

          &.healthy {
            background: $success-color;
            animation: pulse 2s infinite;
          }

          &:not(.healthy) {
            background: $danger-color;
          }
        }
      }

      .more-icon {
        cursor: pointer;
        font-size: 18px;
        color: $text-secondary;
        padding: $spacing-2;
        border-radius: $border-radius-base;
        transition: all $duration-fast;

        &:hover {
          color: $primary-600;
          background: rgba($primary-500, 0.1);
        }

        .dark &:hover {
          color: $primary-400;
          background: rgba($primary-400, 0.1);
        }
      }
    }

    .node-body {
      flex: 1;
      padding: $spacing-5;

      .node-name {
        font-size: $font-size-lg;
        font-weight: $font-weight-semibold;
        color: $text-primary;
        margin: 0 0 $spacing-4 0;
      }

      .node-info {
        .info-item {
          display: flex;
          align-items: center;
          gap: $spacing-2;
          margin-bottom: $spacing-3;
          font-size: $font-size-sm;
          color: $text-regular;

          .el-icon {
            color: $primary-500;
            font-size: 16px;
          }

          &:last-child {
            margin-bottom: 0;
          }
        }
      }
    }

    .node-footer {
      margin-top: auto;
      padding: $spacing-4 $spacing-5;
      border-top: 1px solid $border-light;

      .dark & {
        border-top-color: $gray-700;
      }
    }
  }
}

// 节点详情
.node-details {
  .performance-metrics {
    .metric-card {
      background: $gray-50;
      border-radius: $border-radius-lg;
      padding: $spacing-6;
      text-align: center;
      margin-bottom: $spacing-5;
      border: 1px solid $border-light;
      transition: all $duration-fast;

      &:hover {
        background: $gray-100;
      }

      .dark & {
        background: $gray-800;
        border-color: $gray-700;

        &:hover {
          background: $gray-700;
        }
      }

      .metric-label {
        font-size: $font-size-sm;
        color: $text-secondary;
        margin-bottom: $spacing-3;
        font-weight: $font-weight-medium;
      }

      .metric-value {
        font-size: $font-size-3xl;
        font-weight: $font-weight-bold;
        color: $text-primary;
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
  .node-management {
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
      width: 48px;
      height: 48px;
      font-size: 20px;
    }

    .stat-info .stat-value {
      font-size: $font-size-2xl;
    }
  }

  .nodes-grid .node-card .node-body {
    .node-info .info-item {
      font-size: $font-size-xs;
    }
  }
}

@media (max-width: 480px) {
  .page-header {
    padding: $spacing-4;
  }

  .chart-card,
  .nodes-card,
  .stat-card {
    :deep(.el-card__header),
    :deep(.el-card__body) {
      padding: $spacing-4;
    }
  }

  .nodes-grid .node-card {
    .node-header {
      padding: $spacing-4;
    }

    .node-body {
      padding: $spacing-4;
    }

    .node-footer {
      padding: $spacing-3 $spacing-4;
    }
  }
}

/* Dark mode */
[data-theme="dark"] .node-management {
  color: #f3f4f6;
}

[data-theme="dark"] .node-management .page-header,
[data-theme="dark"] .node-management .page-title,
[data-theme="dark"] .node-management .card-header {
  color: #f3f4f6;
}

[data-theme="dark"] .node-management .page-subtitle,
[data-theme="dark"] .node-management .page-description {
  color: #9ca3af;
}

[data-theme="dark"] .node-management .stat-card,
[data-theme="dark"] .node-management .filter-card,
[data-theme="dark"] .node-management .tasks-card,
[data-theme="dark"] .node-management .chart-card,
[data-theme="dark"] .node-management .table-card,
[data-theme="dark"] .node-management .form-card,
[data-theme="dark"] .node-management .detail-header,
[data-theme="dark"] .node-management .detail-content,
[data-theme="dark"] .node-management .toolbar,
[data-theme="dark"] .node-management .export-options {
  background: rgba(40, 40, 45, 0.98) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #f3f4f6;
}

[data-theme="dark"] .node-management .stat-value,
[data-theme="dark"] .node-management .metric-value {
  color: #f3f4f6;
}

[data-theme="dark"] .node-management .stat-label,
[data-theme="dark"] .node-management .metric-label {
  color: #9ca3af;
}

[data-theme="dark"] .node-management .empty-state,
[data-theme="dark"] .node-management .empty-text {
  color: #9ca3af;
}
</style>
