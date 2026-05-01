<template>
  <div class="tab-content">
    <!-- 监控子标签 -->
    <el-tabs :model-value="monitoringSubTab" @update:model-value="(val: string) => $emit('update:monitoringSubTab', val)" class="monitoring-sub-tabs">
      <!-- 系统资源 -->
      <el-tab-pane label="系统资源" name="resources">
        <el-row :gutter="20" class="mb-3">
          <el-col :span="6">
            <el-card shadow="hover">
              <el-statistic title="CPU 使用率" :value="systemMetrics.cpu_percent || 0" :precision="1" suffix="%" />
              <el-progress :percentage="systemMetrics.cpu_percent || 0" :color="getProgressColor(systemMetrics.cpu_percent || 0)" />
            </el-card>
          </el-col>
          <el-col :span="6">
            <el-card shadow="hover">
              <el-statistic title="内存使用率" :value="systemMetrics.memory_percent || 0" :precision="1" suffix="%" />
              <el-progress :percentage="systemMetrics.memory_percent || 0" :color="getProgressColor(systemMetrics.memory_percent || 0)" />
              <div class="text-xs text-gray-500 mt-1">
                {{ formatBytes(systemMetrics.memory_used_mb * 1024 * 1024) }} / {{ formatBytes(systemMetrics.memory_total_mb * 1024 * 1024) }}
              </div>
            </el-card>
          </el-col>
          <el-col :span="6">
            <el-card shadow="hover">
              <el-statistic title="磁盘使用率" :value="systemMetrics.disk_percent || 0" :precision="1" suffix="%" />
              <el-progress :percentage="systemMetrics.disk_percent || 0" :color="getProgressColor(systemMetrics.disk_percent || 0)" />
              <div class="text-xs text-gray-500 mt-1">
                {{ formatBytes(systemMetrics.disk_used_gb * 1024 * 1024 * 1024) }} / {{ formatBytes(systemMetrics.disk_total_gb * 1024 * 1024 * 1024) }}
              </div>
            </el-card>
          </el-col>
          <el-col :span="6">
            <el-card shadow="hover">
              <div class="uptime-display">
                <div class="uptime-label">运行时长</div>
                <div class="uptime-value">{{ formatUptime(systemMetrics.uptime_seconds || 0) }}</div>
              </div>
            </el-card>
          </el-col>
        </el-row>
        <el-row :gutter="20" class="mb-3">
          <el-col :span="12">
            <el-card shadow="hover">
              <template #header>
                <span>网络流量</span>
              </template>
              <el-row>
                <el-col :span="12">
                  <div class="text-center">
                    <div class="text-2xl font-bold text-blue-500">{{ (systemMetrics.network_rx_mbps || 0).toFixed(2) }}</div>
                    <div class="text-xs text-gray-500">下载 (MB/s)</div>
                  </div>
                </el-col>
                <el-col :span="12">
                  <div class="text-center">
                    <div class="text-2xl font-bold text-green-500">{{ (systemMetrics.network_tx_mbps || 0).toFixed(2) }}</div>
                    <div class="text-xs text-gray-500">上传 (MB/s)</div>
                  </div>
                </el-col>
              </el-row>
            </el-card>
          </el-col>
          <el-col :span="12">
            <el-card shadow="hover">
              <template #header>
                <span>活跃连接</span>
              </template>
              <div class="text-center">
                <div class="text-4xl font-bold">{{ systemMetrics.active_connections || 0 }}</div>
              </div>
            </el-card>
          </el-col>
        </el-row>
        <el-button @click="$emit('load-system-metrics')" :loading="metricsLoading">
          <el-icon><Refresh /></el-icon> 刷新数据
        </el-button>
      </el-tab-pane>

      <!-- 服务健康 -->
      <el-tab-pane label="服务健康" name="services">
        <el-table :data="serviceHealthList" stripe v-loading="metricsLoading" style="width: 100%">
          <el-table-column prop="name" label="服务名称" min-width="180" />
          <el-table-column label="状态" width="100">
            <template #default="{ row }">
              <el-tag :type="row.status === 'healthy' ? 'success' : row.status === 'degraded' ? 'warning' : 'danger'" size="small">
                {{ ({ healthy: '健康', degraded: '降级', down: '离线' } as Record<string, string>)[row.status] || row.status }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="response_time_ms" label="响应时间" width="110" align="center">
            <template #default="{ row }">
              {{ row.response_time_ms }} ms
            </template>
          </el-table-column>
          <el-table-column prop="error_message" label="错误信息" min-width="200" show-overflow-tooltip />
          <el-table-column prop="last_check" label="最后检查" min-width="160" />
        </el-table>
        <el-button @click="$emit('load-service-health')" class="mt-3" :loading="metricsLoading">
          <el-icon><Refresh /></el-icon> 刷新状态
        </el-button>
      </el-tab-pane>

      <!-- 系统日志 -->
      <el-tab-pane label="系统日志" name="logs">
        <el-row :gutter="20" class="mb-3">
          <el-col :span="6">
            <el-select :model-value="logLevelFilter" placeholder="日志级别" clearable @change="(val: string) => { $emit('update:logLevelFilter', val); $emit('load-system-logs') }">
              <el-option label="全部" value="" />
              <el-option label="调试 (debug)" value="debug" />
              <el-option label="信息 (info)" value="info" />
              <el-option label="警告 (warning)" value="warning" />
              <el-option label="错误 (error)" value="error" />
              <el-option label="严重 (critical)" value="critical" />
            </el-select>
          </el-col>
          <el-col :span="6">
            <el-select :model-value="logModuleFilter" placeholder="模块" clearable @change="(val: string) => { $emit('update:logModuleFilter', val); $emit('load-system-logs') }">
              <el-option label="全部" value="" />
              <el-option label="AdminApiModule" value="AdminApiModule" />
              <el-option label="AuthApiModule" value="AuthApiModule" />
              <el-option label="UserApiModule" value="UserApiModule" />
            </el-select>
          </el-col>
          <el-col :span="12">
            <el-button type="danger" @click="$emit('clean-logs')" :disabled="!logLevelFilter && !logModuleFilter">
              <el-icon><Delete /></el-icon> 清理旧日志
            </el-button>
          </el-col>
        </el-row>
        <el-table :data="systemLogs" stripe v-loading="metricsLoading" style="width: 100%">
          <el-table-column prop="level" label="级别" width="90">
            <template #default="{ row }">
              <el-tag :type="getLogLevelTagType(row.level)" size="small">{{ row.level }}</el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="module" label="模块" width="140" />
          <el-table-column prop="message" label="消息" min-width="300" show-overflow-tooltip />
          <el-table-column prop="file" label="文件" width="150" show-overflow-tooltip />
          <el-table-column prop="line" label="行号" width="70" align="center" />
          <el-table-column prop="created_at" label="时间" min-width="160" />
        </el-table>
        <el-pagination
          :current-page="logPagination.page"
          :page-size="logPagination.limit"
          :total="logPagination.total"
          layout="total, prev, pager, next"
          @current-change="(p: number) => $emit('log-page-change', p)"
          class="mt-3"
        />
      </el-tab-pane>

      <!-- 性能指标 -->
      <el-tab-pane label="性能指标" name="performance">
        <el-row :gutter="20" class="mb-3">
          <el-col :span="12">
            <el-card shadow="hover">
              <template #header>
                <span>慢查询 TOP 10</span>
              </template>
              <el-table :data="slowQueries" stripe v-loading="metricsLoading" size="small" style="width: 100%">
                <el-table-column prop="endpoint" label="端点" min-width="150" show-overflow-tooltip />
                <el-table-column prop="execution_time_ms" label="耗时" width="80" align="center">
                  <template #default="{ row }">
                    {{ row.execution_time_ms }} ms
                  </template>
                </el-table-column>
                <el-table-column prop="module" label="模块" width="120" />
              </el-table>
            </el-card>
          </el-col>
          <el-col :span="12">
            <el-card shadow="hover">
              <template #header>
                <span>性能瓶颈</span>
              </template>
              <el-table :data="bottlenecks" stripe v-loading="metricsLoading" size="small" style="width: 100%">
                <el-table-column prop="type" label="类型" width="100">
                  <template #default="{ row }">
                    <el-tag :type="row.severity === 'high' ? 'danger' : row.severity === 'medium' ? 'warning' : 'info'" size="small">
                      {{ ({ slow_query: '慢查询', high_error_rate: '高错误率', unhealthy_service: '服务异常' } as Record<string, string>)[row.type] || row.type }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="endpoint" label="端点" min-width="120" show-overflow-tooltip />
                <el-table-column prop="description" label="描述" min-width="150" show-overflow-tooltip />
              </el-table>
            </el-card>
          </el-col>
        </el-row>
        <el-button @click="$emit('load-performance-data')" :loading="metricsLoading">
          <el-icon><Refresh /></el-icon> 刷新性能数据
        </el-button>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script setup lang="ts">
import { Refresh, Delete } from '@element-plus/icons-vue'

defineProps<{
  monitoringSubTab: string
  metricsLoading: boolean
  systemMetrics: any
  serviceHealthList: any[]
  systemLogs: any[]
  logLevelFilter: string
  logModuleFilter: string
  logPagination: { page: number; limit: number; total: number }
  slowQueries: any[]
  bottlenecks: any[]
  getProgressColor: (percent: number) => string
  formatBytes: (bytes: number) => string
  formatUptime: (seconds: number) => string
  getLogLevelTagType: (level: string) => string
}>()

defineEmits<{
  (e: 'update:monitoringSubTab', value: string): void
  (e: 'update:logLevelFilter', value: string): void
  (e: 'update:logModuleFilter', value: string): void
  (e: 'load-system-metrics'): void
  (e: 'load-service-health'): void
  (e: 'load-system-logs'): void
  (e: 'load-performance-data'): void
  (e: 'clean-logs'): void
  (e: 'log-page-change', page: number): void
}>()
</script>
