<template>
  <el-card
    class="chart-container"
    :body-style="{ padding: '20px', height: '100%' }"
    shadow="hover"
  >
    <template #header>
      <div class="chart-header">
        <div class="chart-header__title">
          <el-icon v-if="icon" class="chart-header__icon">
            <component :is="icon" />
          </el-icon>
          <span>{{ title }}</span>
        </div>
        <div class="chart-header__actions">
          <slot name="actions"></slot>
          <el-button
            v-if="showRefresh"
            :loading="loading"
            :icon="Refresh"
            circle
            size="small"
            @click="handleRefresh"
          />
        </div>
      </div>
    </template>

    <div
      ref="chartWrapper"
      class="chart-wrapper"
      :style="{ height: `${height}px` }"
    >
      <canvas v-if="!loading && !error" ref="chartRef"></canvas>
      <div v-else-if="loading" class="chart-loading">
        <el-icon class="is-loading" :size="40">
          <Loading />
        </el-icon>
        <p>加载中...</p>
      </div>
      <div v-else-if="error" class="chart-error">
        <el-icon :size="40">
          <Warning />
        </el-icon>
        <p>{{ error }}</p>
        <el-button size="small" @click="handleRefresh">重试</el-button>
      </div>
      <div v-else-if="!hasData" class="chart-empty">
        <el-empty description="暂无数据" :image-size="80" />
      </div>
    </div>
  </el-card>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount, watch, computed, nextTick } from 'vue'
import { Chart, type ChartConfiguration, type ChartType } from 'chart.js/auto'
import { Refresh, Loading, Warning } from '@element-plus/icons-vue'

/**
 * 图表容器组件
 * 封装 Chart.js 提供统一的图表展示
 */
interface Props {
  /** 图表标题 */
  title: string
  /** 图表类型 */
  type: ChartType
  /** 图表数据 */
  data: ChartConfiguration['data']
  /** 图表配置 */
  options?: ChartConfiguration['options']
  /** 图表高度 */
  height?: number
  /** 是否显示刷新按钮 */
  showRefresh?: boolean
  /** 是否加载中 */
  loading?: boolean
  /** 错误信息 */
  error?: string
  /** 图标组件 */
  icon?: any
}

const props = withDefaults(defineProps<Props>(), {
  height: 300,
  showRefresh: true,
  loading: false,
  error: ''
})

const emit = defineEmits<{
  (e: 'refresh'): void
}>()

const chartRef = ref<HTMLCanvasElement>()
const chartWrapper = ref<HTMLDivElement>()
let chartInstance: Chart | null = null

/**
 * 是否有数据
 */
const hasData = computed(() => {
  if (!props.data?.datasets) return false
  return props.data.datasets.some((dataset) => {
    const data = dataset.data as any[]
    return data && data.length > 0
  })
})

/**
 * 初始化图表
 */
const initChart = () => {
  if (!chartRef.value) return

  // 销毁已存在的图表
  if (chartInstance) {
    chartInstance.destroy()
  }

  // 创建新图表
  const config: ChartConfiguration = {
    type: props.type,
    data: props.data,
    options: {
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: {
          display: true,
          position: 'bottom',
          labels: {
            padding: 16,
            usePointStyle: true,
            font: {
              size: 12
            }
          }
        },
        tooltip: {
          backgroundColor: 'rgba(0, 0, 0, 0.8)',
          padding: 12,
          titleFont: {
            size: 14,
            weight: 'bold'
          },
          bodyFont: {
            size: 13
          },
          cornerRadius: 4,
          displayColors: true
        }
      },
      animation: {
        duration: 750,
        easing: 'easeInOutQuart'
      },
      ...props.options
    }
  }

  chartInstance = new Chart(chartRef.value, config)
}

/**
 * 更新图表数据
 */
const updateChart = () => {
  if (!chartInstance || !props.data) return

  chartInstance.data = props.data
  chartInstance.update()
}

/**
 * 刷新图表
 */
const handleRefresh = () => {
  emit('refresh')
}

/**
 * 响应式处理
 */
const handleResize = () => {
  if (chartInstance) {
    chartInstance.resize()
  }
}

// 监听数据变化
watch(
  () => props.data,
  () => {
    if (props.data) {
      nextTick(() => {
        if (chartInstance) {
          updateChart()
        } else {
          initChart()
        }
      })
    }
  },
  { deep: true }
)

// 监听配置变化
watch(
  () => props.options,
  () => {
    if (chartInstance && props.options) {
      chartInstance.options = { ...chartInstance.options, ...props.options }
      chartInstance.update()
    }
  },
  { deep: true }
)

// 生命周期
onMounted(() => {
  nextTick(() => {
    if (props.data && !props.loading) {
      initChart()
    }
  })

  window.addEventListener('resize', handleResize)
})

onBeforeUnmount(() => {
  if (chartInstance) {
    chartInstance.destroy()
    chartInstance = null
  }

  window.removeEventListener('resize', handleResize)
})

// 暴露方法
defineExpose({
  initChart,
  updateChart,
  chart: chartInstance
})
</script>

<style scoped lang="scss">
.chart-container {
  height: 100%;
  border: none;
  transition: all 0.3s ease;

  &:hover {
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.12) !important;
  }

  :deep(.el-card__header) {
    padding: 16px 20px;
    border-bottom: 1px solid #f0f0f0;
  }

  :deep(.el-card__body) {
    padding: 20px;
  }
}

.chart-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;

  &__title {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 16px;
    font-weight: 600;
    color: #303133;
  }

  &__icon {
    font-size: 18px;
    color: #409eff;
  }

  &__actions {
    display: flex;
    align-items: center;
    gap: 8px;
  }
}

.chart-wrapper {
  position: relative;
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
}

.chart-loading,
.chart-error,
.chart-empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100%;
  color: #909399;

  p {
    margin-top: 12px;
    font-size: 14px;
  }

  .el-icon {
    color: #c0c4cc;
  }
}

.chart-error {
  .el-icon {
    color: #f56c6c;
  }
}

// 响应式设计
@media (max-width: 768px) {
  .chart-header {
    flex-direction: column;
    align-items: flex-start;

    &__actions {
      width: 100%;
      justify-content: flex-end;
    }
  }
}
</style>
