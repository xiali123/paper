<template>
  <div class="performance-monitor" v-if="isVisible">
    <div class="monitor-header">
      <h3>Performance Monitor</h3>
      <el-button size="small" @click="toggleVisibility" text>
        <el-icon><Close /></el-icon>
      </el-button>
    </div>

    <div class="monitor-content">
      <!-- Real-time Metrics -->
      <div class="metric-section">
        <h4>Real-time Metrics</h4>
        <div class="metric-grid">
          <div class="metric-item">
            <span class="metric-label">Input Latency</span>
            <span class="metric-value" :class="getMetricClass(realTimeMetrics.inputLatency, 50)">
              {{ realTimeMetrics.inputLatency.toFixed(1) }} ms
            </span>
          </div>
          <div class="metric-item">
            <span class="metric-label">Render Time</span>
            <span class="metric-value" :class="getMetricClass(realTimeMetrics.renderTime, 100)">
              {{ realTimeMetrics.renderTime.toFixed(1) }} ms
            </span>
          </div>
          <div class="metric-item">
            <span class="metric-label">Memory Usage</span>
            <span class="metric-value" :class="getMemoryMetricClass(realTimeMetrics.memoryUsage)">
              {{ formatBytes(realTimeMetrics.memoryUsage) }}
            </span>
          </div>
          <div class="metric-item">
            <span class="metric-label">Cache Size</span>
            <span class="metric-value">
              {{ cacheStats.equation?.size || 0 }} equations
            </span>
          </div>
        </div>
      </div>

      <!-- Average Metrics -->
      <div class="metric-section">
        <h4>Average Metrics (Last Minute)</h4>
        <div class="metric-grid">
          <div class="metric-item">
            <span class="metric-label">Avg Input Latency</span>
            <span class="metric-value" :class="getMetricClass(averageMetrics.inputLatency, 50)">
              {{ averageMetrics.inputLatency.toFixed(1) }} ms
            </span>
          </div>
          <div class="metric-item">
            <span class="metric-label">Avg Render Time</span>
            <span class="metric-value" :class="getMetricClass(averageMetrics.renderTime, 100)">
              {{ averageMetrics.renderTime.toFixed(1) }} ms
            </span>
          </div>
          <div class="metric-item">
            <span class="metric-label">Avg Memory Usage</span>
            <span class="metric-value" :class="getMemoryMetricClass(averageMetrics.memoryUsage)">
              {{ formatBytes(averageMetrics.memoryUsage) }}
            </span>
          </div>
          <div class="metric-item">
            <span class="metric-label">Operations</span>
            <span class="metric-value">{{ totalOperations }}</span>
          </div>
        </div>
      </div>

      <!-- Performance Trends -->
      <div class="metric-section">
        <h4>Performance Trends</h4>
        <div class="trend-charts">
          <canvas ref="inputLatencyChart" width="300" height="100"></canvas>
          <canvas ref="renderTimeChart" width="300" height="100"></canvas>
        </div>
      </div>

      <!-- Alerts -->
      <div class="metric-section" v-if="alerts.length > 0">
        <h4>Performance Alerts</h4>
        <div class="alerts-list">
          <div
            v-for="(alert, index) in alerts.slice(-5)"
            :key="index"
            class="alert-item"
            :class="alert.severity"
          >
            <el-icon><Warning /></el-icon>
            <span class="alert-message">{{ alert.message }}</span>
            <span class="alert-time">{{ formatTime(alert.timestamp) }}</span>
          </div>
        </div>
      </div>

      <!-- Cache Statistics -->
      <div class="metric-section">
        <h4>Cache Statistics</h4>
        <div class="cache-stats">
          <div class="cache-stat-item">
            <span class="stat-label">Equation Cache</span>
            <div class="stat-details">
              <span>Size: {{ cacheStats.equation?.size || 0 }}</span>
              <span>Accesses: {{ cacheStats.equation?.totalAccesses || 0 }}</span>
            </div>
          </div>
          <div class="cache-stat-item">
            <span class="stat-label">Syntax Cache</span>
            <div class="stat-details">
              <span>Size: {{ cacheStats.syntax?.size || 0 }}</span>
              <span>Accesses: {{ cacheStats.syntax?.totalAccesses || 0 }}</span>
            </div>
          </div>
        </div>
      </div>

      <!-- Actions -->
      <div class="metric-section actions">
        <el-button size="small" @click="clearCache">Clear Cache</el-button>
        <el-button size="small" @click="resetMetrics">Reset Metrics</el-button>
        <el-button size="small" @click="exportMetrics">Export Data</el-button>
      </div>
    </div>
  </div>

  <!-- Floating toggle button -->
  <el-button
    v-else
    class="monitor-toggle"
    size="small"
    circle
    @click="toggleVisibility"
    title="Show Performance Monitor"
  >
    <el-icon><Monitor /></el-icon>
  </el-button>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
import { Close, Monitor, Warning } from '@element-plus/icons-vue'
import { performanceMonitor } from '@/utils/performance'
import { getLatexCacheStats, clearLatexCache } from '@/utils/optimizedLatexEditor'

interface MetricData {
  value: number
  timestamp: number
}

interface Alert {
  message: string
  severity: 'warning' | 'critical'
  timestamp: number
}

const isVisible = ref(false)
const updateInterval = ref<number | null>(null)
const inputLatencyChart = ref<HTMLCanvasElement | null>(null)
const renderTimeChart = ref<HTMLCanvasElement | null>(null)

// Real-time metrics
const realTimeMetrics = ref({
  inputLatency: 0,
  renderTime: 0,
  memoryUsage: 0
})

// Average metrics
const averageMetrics = ref({
  inputLatency: 0,
  renderTime: 0,
  memoryUsage: 0
})

// Performance trends
const inputLatencyTrend = ref<number[]>([])
const renderTimeTrend = ref<number[]>([])
const maxTrendLength = 50

// Alerts
const alerts = ref<Alert[]>([])

// Cache statistics
const cacheStats = ref<any>({
  equation: { size: 0, totalAccesses: 0 },
  syntax: { size: 0, totalAccesses: 0 }
})

// Total operations tracked
const totalOperations = ref(0)

// Performance thresholds
const THRESHOLDS = {
  inputLatency: {
    warning: 50,
    critical: 100
  },
  renderTime: {
    warning: 100,
    critical: 200
  },
  memoryUsage: {
    warning: 100 * 1024 * 1024, // 100MB
    critical: 150 * 1024 * 1024 // 150MB
  }
}

function toggleVisibility() {
  isVisible.value = !isVisible.value
}

function getMetricClass(value: number, threshold: number): string {
  if (value > threshold * 2) return 'critical'
  if (value > threshold) return 'warning'
  return 'good'
}

function getMemoryMetricClass(value: number): string {
  if (value > THRESHOLDS.memoryUsage.critical) return 'critical'
  if (value > THRESHOLDS.memoryUsage.warning) return 'warning'
  return 'good'
}

function formatBytes(bytes: number): string {
  if (bytes < 1024) return bytes + ' B'
  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB'
  return (bytes / (1024 * 1024)).toFixed(1) + ' MB'
}

function formatTime(timestamp: number): string {
  const date = new Date(timestamp)
  return date.toLocaleTimeString()
}

function updateMetrics() {
  // Get current memory usage
  const memoryInfo = performanceMonitor.getMemoryUsage()
  if (memoryInfo) {
    realTimeMetrics.value.memoryUsage = memoryInfo.used
    averageMetrics.value.memoryUsage = memoryInfo.used // Approximation
  }

  // Get average metrics from performance monitor
  const inputMetrics = performanceMonitor.getMetrics('input_handling')
  const renderMetrics = performanceMonitor.getMetrics('syntax_highlighting')

  if (inputMetrics.length > 0) {
    const recent = inputMetrics.slice(-10)
    const avgInput = recent.reduce((sum, m) => sum + m.value, 0) / recent.length
    realTimeMetrics.value.inputLatency = recent[recent.length - 1].value
    averageMetrics.value.inputLatency = avgInput

    // Update trend
    inputLatencyTrend.value.push(recent[recent.length - 1].value)
    if (inputLatencyTrend.value.length > maxTrendLength) {
      inputLatencyTrend.value.shift()
    }
  }

  if (renderMetrics.length > 0) {
    const recent = renderMetrics.slice(-10)
    const avgRender = recent.reduce((sum, m) => sum + m.value, 0) / recent.length
    realTimeMetrics.value.renderTime = recent[recent.length - 1].value
    averageMetrics.value.renderTime = avgRender

    // Update trend
    renderTimeTrend.value.push(recent[recent.length - 1].value)
    if (renderTimeTrend.value.length > maxTrendLength) {
      renderTimeTrend.value.shift()
    }
  }

  // Update total operations
  totalOperations.value = inputMetrics.length + renderMetrics.length

  // Check for performance issues and create alerts
  checkPerformanceThresholds()

  // Update cache stats
  try {
    cacheStats.value = getLatexCacheStats()
  } catch (error) {
    // Cache stats might not be available
  }

  // Draw charts
  drawCharts()
}

function checkPerformanceThresholds() {
  // Check input latency
  if (realTimeMetrics.value.inputLatency > THRESHOLDS.inputLatency.critical) {
    addAlert(
      `Input latency critical: ${realTimeMetrics.value.inputLatency.toFixed(1)}ms`,
      'critical'
    )
  } else if (realTimeMetrics.value.inputLatency > THRESHOLDS.inputLatency.warning) {
    addAlert(
      `Input latency high: ${realTimeMetrics.value.inputLatency.toFixed(1)}ms`,
      'warning'
    )
  }

  // Check render time
  if (realTimeMetrics.value.renderTime > THRESHOLDS.renderTime.critical) {
    addAlert(
      `Render time critical: ${realTimeMetrics.value.renderTime.toFixed(1)}ms`,
      'critical'
    )
  } else if (realTimeMetrics.value.renderTime > THRESHOLDS.renderTime.warning) {
    addAlert(
      `Render time high: ${realTimeMetrics.value.renderTime.toFixed(1)}ms`,
      'warning'
    )
  }

  // Check memory usage
  if (realTimeMetrics.value.memoryUsage > THRESHOLDS.memoryUsage.critical) {
    addAlert(
      `Memory usage critical: ${formatBytes(realTimeMetrics.value.memoryUsage)}`,
      'critical'
    )
  } else if (realTimeMetrics.value.memoryUsage > THRESHOLDS.memoryUsage.warning) {
    addAlert(
      `Memory usage high: ${formatBytes(realTimeMetrics.value.memoryUsage)}`,
      'warning'
    )
  }
}

function addAlert(message: string, severity: 'warning' | 'critical') {
  // Avoid duplicate alerts
  const lastAlert = alerts.value[alerts.value.length - 1]
  if (lastAlert && lastAlert.message === message) {
    return
  }

  alerts.value.push({
    message,
    severity,
    timestamp: Date.now()
  })

  // Keep only recent alerts
  if (alerts.value.length > 20) {
    alerts.value.shift()
  }
}

function drawCharts() {
  drawTrendChart(inputLatencyChart.value, inputLatencyTrend.value, '#67C23A', THRESHOLDS.inputLatency.warning)
  drawTrendChart(renderTimeChart.value, renderTimeTrend.value, '#409EFF', THRESHOLDS.renderTime.warning)
}

function drawTrendChart(
  canvas: HTMLCanvasElement | null,
  data: number[],
  color: string,
  threshold: number
) {
  if (!canvas) return

  const ctx = canvas.getContext('2d')
  if (!ctx) return

  const width = canvas.width
  const height = canvas.height

  // Clear canvas
  ctx.clearRect(0, 0, width, height)

  if (data.length < 2) return

  // Find max value for scaling
  const maxValue = Math.max(...data, threshold * 1.5)

  // Draw threshold line
  const thresholdY = height - (threshold / maxValue) * height
  ctx.strokeStyle = '#F56C6C'
  ctx.setLineDash([5, 5])
  ctx.beginPath()
  ctx.moveTo(0, thresholdY)
  ctx.lineTo(width, thresholdY)
  ctx.stroke()
  ctx.setLineDash([])

  // Draw trend line
  ctx.strokeStyle = color
  ctx.lineWidth = 2
  ctx.beginPath()

  data.forEach((value, index) => {
    const x = (index / (data.length - 1)) * width
    const y = height - (value / maxValue) * height

    if (index === 0) {
      ctx.moveTo(x, y)
    } else {
      ctx.lineTo(x, y)
    }
  })

  ctx.stroke()
}

function clearCache() {
  clearLatexCache()
  cacheStats.value = {
    equation: { size: 0, totalAccesses: 0 },
    syntax: { size: 0, totalAccesses: 0 }
  }
}

function resetMetrics() {
  performanceMonitor.clearMetrics()
  inputLatencyTrend.value = []
  renderTimeTrend.value = []
  alerts.value = []
  totalOperations.value = 0
  realTimeMetrics.value = {
    inputLatency: 0,
    renderTime: 0,
    memoryUsage: 0
  }
  averageMetrics.value = {
    inputLatency: 0,
    renderTime: 0,
    memoryUsage: 0
  }
}

function exportMetrics() {
  const data = {
    timestamp: new Date().toISOString(),
    realTimeMetrics: realTimeMetrics.value,
    averageMetrics: averageMetrics.value,
    totalOperations: totalOperations.value,
    alerts: alerts.value,
    cacheStats: cacheStats.value,
    trends: {
      inputLatency: inputLatencyTrend.value,
      renderTime: renderTimeTrend.value
    }
  }

  const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = `performance-metrics-${Date.now()}.json`
  link.click()
  URL.revokeObjectURL(url)
}

// Lifecycle
onMounted(() => {
  // Update metrics every second
  updateInterval.value = window.setInterval(updateMetrics, 1000)
  updateMetrics()
})

onUnmounted(() => {
  if (updateInterval.value) {
    clearInterval(updateInterval.value)
  }
})
</script>

<style scoped lang="scss">
.performance-monitor {
  position: fixed;
  top: 100px;
  right: 20px;
  width: 350px;
  max-height: 80vh;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  overflow: hidden;
  z-index: 1000;
}

.monitor-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color);
  background: var(--el-bg-color-page);

  h3 {
    margin: 0;
    font-size: 14px;
    font-weight: 600;
  }
}

.monitor-content {
  padding: 16px;
  max-height: calc(80vh - 50px);
  overflow-y: auto;
}

.metric-section {
  margin-bottom: 16px;

  h4 {
    margin: 0 0 8px 0;
    font-size: 12px;
    font-weight: 600;
    color: var(--el-text-color-secondary);
  }
}

.metric-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
}

.metric-item {
  display: flex;
  flex-direction: column;
  padding: 8px;
  background: var(--el-bg-color-page);
  border-radius: 4px;

  .metric-label {
    font-size: 11px;
    color: var(--el-text-color-secondary);
    margin-bottom: 4px;
  }

  .metric-value {
    font-size: 14px;
    font-weight: 600;

    &.good {
      color: var(--el-color-success);
    }

    &.warning {
      color: var(--el-color-warning);
    }

    &.critical {
      color: var(--el-color-danger);
    }
  }
}

.trend-charts {
  display: flex;
  flex-direction: column;
  gap: 8px;

  canvas {
    background: var(--el-bg-color-page);
    border: 1px solid var(--el-border-color);
    border-radius: 4px;
  }
}

.alerts-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.alert-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px;
  border-radius: 4px;
  font-size: 12px;

  &.warning {
    background: var(--el-color-warning-light-9);
    color: var(--el-color-warning);
  }

  &.critical {
    background: var(--el-color-danger-light-9);
    color: var(--el-color-danger);
  }

  .alert-message {
    flex: 1;
  }

  .alert-time {
    font-size: 10px;
    opacity: 0.8;
  }
}

.cache-stats {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.cache-stat-item {
  padding: 8px;
  background: var(--el-bg-color-page);
  border-radius: 4px;

  .stat-label {
    font-size: 12px;
    font-weight: 600;
    display: block;
    margin-bottom: 4px;
  }

  .stat-details {
    display: flex;
    justify-content: space-between;
    font-size: 11px;
    color: var(--el-text-color-secondary);
  }
}

.actions {
  display: flex;
  gap: 8px;
  justify-content: center;
}

.monitor-toggle {
  position: fixed;
  top: 100px;
  right: 20px;
  z-index: 1000;
  background: var(--el-color-primary);
  color: white;
  border: none;

  &:hover {
    background: var(--el-color-primary-light-3);
  }
}

// Scrollbar styling
.monitor-content::-webkit-scrollbar {
  width: 6px;
}

.monitor-content::-webkit-scrollbar-track {
  background: var(--el-bg-color-page);
}

.monitor-content::-webkit-scrollbar-thumb {
  background: var(--el-border-color);
  border-radius: 3px;
}

.monitor-content::-webkit-scrollbar-thumb:hover {
  background: var(--el-border-color-darker);
}
</style>
