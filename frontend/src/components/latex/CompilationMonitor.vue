<template>
  <div class="compilation-monitor">
    <!-- 监控头部 -->
    <div class="monitor-header">
      <div class="header-title">
        <el-icon><DataAnalysis /></el-icon>
        <span>编译性能监控</span>
      </div>
      <div class="header-actions">
        <el-tag size="small" :type="getStatusType(status)">
          {{ status }}
        </el-tag>
        <el-button-group size="small">
          <el-button :icon="Refresh" @click="refreshStats" :loading="loading" />
          <el-button :icon="Download" @click="exportReport" />
          <el-button :icon="Delete" @click="clearHistory" />
        </el-button-group>
      </div>
    </div>

    <!-- 编译概览 -->
    <div class="stats-overview">
      <div class="stat-card">
        <div class="stat-icon" style="background: #ecf5ff; color: #409eff;">
          <el-icon><Timer /></el-icon>
        </div>
        <div class="stat-content">
          <div class="stat-value">{{ averageTime }}ms</div>
          <div class="stat-label">平均编译时间</div>
        </div>
      </div>

      <div class="stat-card">
        <div class="stat-icon" style="background: #f0f9ff; color: #67c23a;">
          <el-icon><SuccessFilled /></el-icon>
        </div>
        <div class="stat-content">
          <div class="stat-value">{{ successRate }}%</div>
          <div class="stat-label">成功率</div>
        </div>
      </div>

      <div class="stat-card">
        <div class="stat-icon" style="background: #fef0f0; color: #f56c6c;">
          <el-icon><WarningFilled /></el-icon>
        </div>
        <div class="stat-content">
          <div class="stat-value">{{ totalErrors }}</div>
          <div class="stat-label">错误数</div>
        </div>
      </div>

      <div class="stat-card">
        <div class="stat-icon" style="background: #fff7e6; color: #e6a23c;">
          <el-icon><Document /></el-icon>
        </div>
        <div class="stat-content">
          <div class="stat-value">{{ totalCompilations }}</div>
          <div class="stat-label">编译次数</div>
        </div>
      </div>
    </div>

    <!-- 性能图表 -->
    <div class="performance-chart">
      <div class="chart-header">
        <h4>编译时间趋势</h4>
        <el-radio-group v-model="chartRange" size="small">
          <el-radio-button value="10">最近10次</el-radio-button>
          <el-radio-button value="20">最近20次</el-radio-button>
          <el-radio-button value="50">最近50次</el-radio-button>
        </el-radio-group>
      </div>
      <div ref="chartContainer" class="chart-container"></div>
    </div>

    <!-- 编译历史 -->
    <div class="compilation-history">
      <div class="history-header">
        <h4>编译历史</h4>
        <el-select v-model="historyFilter" size="small" placeholder="筛选类型">
          <el-option label="全部" value="all" />
          <el-option label="成功" value="success" />
          <el-option label="失败" value="error" />
          <el-option label="警告" value="warning" />
        </el-select>
      </div>
      <div class="history-list">
        <div
          v-for="(record, index) in filteredHistory"
          :key="index"
          class="history-item"
          :class="`status-${record.status}`"
        >
          <div class="history-status">
            <el-icon v-if="record.status === 'success'" color="#67c23a"><SuccessFilled /></el-icon>
            <el-icon v-else-if="record.status === 'error'" color="#f56c6c"><CircleCloseFilled /></el-icon>
            <el-icon v-else color="#e6a23c"><WarningFilled /></el-icon>
          </div>
          <div class="history-content">
            <div class="history-title">
              {{ record.documentName || '未命名文档' }}
              <el-tag v-if="record.incremental" size="small" type="info">增量</el-tag>
            </div>
            <div class="history-meta">
              <span>{{ record.compileTime }}ms</span>
              <span>{{ formatTime(record.timestamp) }}</span>
            </div>
            <div v-if="record.errors?.length" class="history-errors">
              <el-tag
                v-for="(error, idx) in record.errors.slice(0, 3)"
                :key="idx"
                size="small"
                type="danger"
              >
                行{{ error.line }}: {{ error.message }}
              </el-tag>
              <span v-if="record.errors.length > 3" class="more-errors">
                +{{ record.errors.length - 3 }} 更多
              </span>
            </div>
          </div>
          <div class="history-actions">
            <el-button size="small" text @click="viewDetails(record)">
              详情
            </el-button>
          </div>
        </div>
        <el-empty v-if="filteredHistory.length === 0" description="暂无编译记录" />
      </div>
    </div>

    <!-- 详情对话框 -->
    <el-dialog v-model="detailVisible" title="编译详情" width="600px">
      <div v-if="selectedRecord" class="compilation-detail">
        <el-descriptions :column="2" border>
          <el-descriptions-item label="文档名称">
            {{ selectedRecord.documentName }}
          </el-descriptions-item>
          <el-descriptions-item label="编译时间">
            {{ selectedRecord.compileTime }}ms
          </el-descriptions-item>
          <el-descriptions-item label="状态">
            <el-tag :type="selectedRecord.status === 'success' ? 'success' : 'danger'">
              {{ selectedRecord.status }}
            </el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="类型">
            <el-tag v-if="selectedRecord.incremental" type="info">增量编译</el-tag>
            <el-tag v-else type="primary">完整编译</el-tag>
          </el-descriptions-item>
          <el-descriptions-item label="时间戳" :span="2">
            {{ formatTime(selectedRecord.timestamp) }}
          </el-descriptions-item>
        </el-descriptions>

        <div v-if="selectedRecord.log" class="compilation-log">
          <h5>编译日志</h5>
          <pre>{{ selectedRecord.log }}</pre>
        </div>

        <div v-if="selectedRecord.errors?.length" class="compilation-errors">
          <h5>错误列表</h5>
          <el-table :data="selectedRecord.errors" size="small" max-height="200">
            <el-table-column prop="line" label="行号" width="80" />
            <el-table-column prop="message" label="错误信息" />
            <el-table-column prop="type" label="类型" width="80">
              <template #default="{ row }">
                <el-tag :type="row.type === 'error' ? 'danger' : 'warning'" size="small">
                  {{ row.type }}
                </el-tag>
              </template>
            </el-table-column>
          </el-table>
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from 'vue'
import {
  DataAnalysis,
  Refresh,
  Download,
  Delete,
  Timer,
  SuccessFilled,
  WarningFilled,
  Document,
  CircleCloseFilled
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface CompilationRecord {
  documentId: string
  documentName?: string
  status: 'success' | 'error' | 'warning'
  compileTime: number
  timestamp: number
  incremental: boolean
  log?: string
  errors?: Array<{ line: number; message: string; type: string }>
  warnings?: Array<{ line: number; message: string; type: string }>
}

const props = defineProps<{
  documentId?: string
}>()

const status = ref('idle')
const loading = ref(false)
const chartRange = ref('10')
const historyFilter = ref('all')
const detailVisible = ref(false)
const selectedRecord = ref<CompilationRecord | null>(null)

// 编译历史记录
const compilationHistory = ref<CompilationRecord[]>([])
const chartContainer = ref<HTMLElement>()

// 计算属性
const totalCompilations = computed(() => compilationHistory.value.length)
const successCompilations = computed(() =>
  compilationHistory.value.filter(r => r.status === 'success').length
)
const successRate = computed(() => {
  if (totalCompilations.value === 0) return 0
  return Math.round((successCompilations.value / totalCompilations.value) * 100)
})

const averageTime = computed(() => {
  if (compilationHistory.value.length === 0) return 0
  const times = compilationHistory.value.map(r => r.compileTime)
  return Math.round(times.reduce((a, b) => a + b, 0) / times.length)
})

const totalErrors = computed(() =>
  compilationHistory.value.reduce((sum, r) => sum + (r.errors?.length || 0), 0)
)

const filteredHistory = computed(() => {
  if (historyFilter.value === 'all') return compilationHistory.value
  return compilationHistory.value.filter(r => r.status === historyFilter.value)
})

// 获取状态类型
const getStatusType = (s: string) => {
  const types: Record<string, any> = {
    idle: 'info',
    compiling: 'warning',
    success: 'success',
    error: 'danger'
  }
  return types[s] || 'info'
}

// 格式化时间
const formatTime = (timestamp: number): string => {
  const date = new Date(timestamp)
  return date.toLocaleString('zh-CN', {
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit'
  })
}

// 添加编译记录
const addRecord = (record: CompilationRecord) => {
  compilationHistory.value.unshift(record)
  // 限制历史记录数量
  if (compilationHistory.value.length > 100) {
    compilationHistory.value = compilationHistory.value.slice(0, 100)
  }
  saveHistory()
}

// 清空历史
const clearHistory = () => {
  compilationHistory.value = []
  saveHistory()
  ElMessage.success('已清空编译历史')
}

// 刷新统计
const refreshStats = () => {
  loadHistory()
  ElMessage.success('统计已刷新')
}

// 导出报告
const exportReport = () => {
  const report = {
    summary: {
      totalCompilations: totalCompilations.value,
      successRate: successRate.value,
      averageTime: averageTime.value,
      totalErrors: totalErrors.value
    },
    history: compilationHistory.value
  }

  const blob = new Blob([JSON.stringify(report, null, 2)], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = `compilation-report-${Date.now()}.json`
  link.click()
  URL.revokeObjectURL(url)
  ElMessage.success('报告已导出')
}

// 查看详情
const viewDetails = (record: CompilationRecord) => {
  selectedRecord.value = record
  detailVisible.value = true
}

// 保存历史到本地存储
const saveHistory = () => {
  localStorage.setItem('latex_compilation_history', JSON.stringify(compilationHistory.value))
}

// 从本地存储加载历史
const loadHistory = () => {
  const saved = localStorage.getItem('latex_compilation_history')
  if (saved) {
    try {
      compilationHistory.value = JSON.parse(saved)
    } catch (error) {
      console.error('Failed to parse compilation history:', error)
    }
  }
}

// 生命周期
onMounted(() => {
  loadHistory()
})

// 暴露方法供外部调用
defineExpose({
  addRecord,
  setStatus: (s: string) => { status.value = s },
  clearHistory
})
</script>

<style scoped lang="scss">
.compilation-monitor {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #f5f7fa;

  .monitor-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 16px;
    background: #fff;
    border-bottom: 1px solid #e4e7ed;

    .header-title {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 16px;
      font-weight: 600;
      color: #303133;
    }
  }

  .stats-overview {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 16px;
    padding: 16px;

    .stat-card {
      background: #fff;
      border-radius: 8px;
      padding: 16px;
      display: flex;
      align-items: center;
      gap: 12px;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);

      .stat-icon {
        width: 48px;
        height: 48px;
        border-radius: 8px;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 24px;
      }

      .stat-content {
        .stat-value {
          font-size: 20px;
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

  .performance-chart {
    background: #fff;
    margin: 0 16px 16px;
    border-radius: 8px;
    padding: 16px;

    .chart-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 16px;

      h4 {
        margin: 0;
        font-size: 14px;
        font-weight: 600;
        color: #303133;
      }
    }

    .chart-container {
      height: 200px;
      background: #f5f7fa;
      border-radius: 4px;
      display: flex;
      align-items: center;
      justify-content: center;
      color: #909399;
      font-size: 12px;
    }
  }

  .compilation-history {
    flex: 1;
    background: #fff;
    margin: 0 16px 16px;
    border-radius: 8px;
    padding: 16px;
    overflow: hidden;
    display: flex;
    flex-direction: column;

    .history-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 16px;

      h4 {
        margin: 0;
        font-size: 14px;
        font-weight: 600;
        color: #303133;
      }
    }

    .history-list {
      flex: 1;
      overflow-y: auto;

      .history-item {
        display: flex;
        align-items: flex-start;
        gap: 12px;
        padding: 12px;
        border-radius: 6px;
        margin-bottom: 8px;
        border: 1px solid #e4e7ed;
        transition: all 0.2s;

        &:hover {
          background: #f5f7fa;
          border-color: #c0c4cc;
        }

        &.status-success {
          border-left: 3px solid #67c23a;
        }

        &.status-error {
          border-left: 3px solid #f56c6c;
          background: #fef0f0;
        }

        &.status-warning {
          border-left: 3px solid #e6a23c;
        }

        .history-status {
          flex-shrink: 0;
          font-size: 20px;
          margin-top: 2px;
        }

        .history-content {
          flex: 1;
          min-width: 0;

          .history-title {
            font-weight: 500;
            color: #303133;
            margin-bottom: 4px;
          }

          .history-meta {
            display: flex;
            gap: 12px;
            font-size: 12px;
            color: #909399;
            margin-bottom: 4px;
          }

          .history-errors {
            display: flex;
            flex-wrap: wrap;
            gap: 4px;
            margin-top: 4px;

            .more-errors {
              font-size: 12px;
              color: #909399;
              padding: 0 4px;
            }
          }
        }

        .history-actions {
          flex-shrink: 0;
        }
      }
    }
  }

  .compilation-detail {
    .compilation-log,
    .compilation-errors {
      margin-top: 20px;

      h5 {
        margin: 0 0 12px;
        font-size: 14px;
        font-weight: 600;
        color: #303133;
      }

      pre {
        background: #f5f7fa;
        padding: 12px;
        border-radius: 4px;
        font-size: 12px;
        max-height: 200px;
        overflow: auto;
      }
    }
  }
}
</style>
