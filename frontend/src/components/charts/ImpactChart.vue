<template>
  <div class="impact-chart-container">
    <div class="chart-header">
      <h3>📊 学术影响力趋势</h3>
      <el-radio-group v-model="timeRange" size="small" @change="updateChart">
        <el-radio-button label="6m">6个月</el-radio-button>
        <el-radio-button label="1y">1年</el-radio-button>
        <el-radio-button label="2y">2年</el-radio-button>
      </el-radio-group>
    </div>

    <div class="chart-wrapper">
      <canvas
        ref="chartCanvas"
        :id="chartId"
        style="max-height: 400px; width: 100%"
      ></canvas>
    </div>

    <div class="chart-legend">
      <div
        v-for="(item, index) in legendItems"
        :key="index"
        class="legend-item"
      >
        <div
          class="legend-color"
          :style="{ backgroundColor: item.color }"
        ></div>
        <span class="legend-label">{{ item.label }}</span>
        <span class="legend-value">{{ item.value }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, watch } from 'vue'
import { useChart, chartPresets } from '@/composables/useChart'

interface Props {
  chartId?: string
  metrics?: {
    labels: string[]
    citations: number[]
    downloads: number[]
    views: number[]
  }
}

const props = withDefaults(defineProps<Props>(), {
  chartId: () => `impact-chart-${Date.now()}`,
  metrics: undefined
})

const timeRange = ref('1y')
const { canvasElement, createChart, updateChart } = useChart(props.chartId)

const legendItems = ref<Array<{
  label: string
  color: string
  value: string
}>[]>([])

const generateMockData = () => {
  const months = []
  const citations = []
  const downloads = []
  const views = []

  const now = new Date()
  const range = timeRange.value === '6m' ? 6 : timeRange.value === '1y' ? 12 : 24

  for (let i = range - 1; i >= 0; i--) {
    const date = new Date(now.getFullYear(), now.getMonth() - i, 1)
    months.push(`${date.getMonth() + 1}月`)

    // 模拟数据
    citations.push(Math.floor(Math.random() * 50) + 10)
    downloads.push(Math.floor(Math.random() * 200) + 50)
    views.push(Math.floor(Math.random() * 500) + 100)
  }

  return { labels: months, citations, downloads, views }
}

const initChart = () => {
  const data = props.metrics || generateMockData()

  const chartData = {
    labels: data.labels,
    datasets: [
      {
        label: '引用数',
        data: data.citations,
        color: '#667eea',
        backgroundColor: 'rgba(102, 126, 234, 0.1)',
        borderColor: '#667eea'
      },
      {
        label: '下载量',
        data: data.downloads,
        color: '#67c23a',
        backgroundColor: 'rgba(103, 194, 58, 0.1)',
        borderColor: '#67c23a'
      },
      {
        label: '浏览量',
        data: data.views,
        color: '#e6a23c',
        backgroundColor: 'rgba(230, 162, 60, 0.1)',
        borderColor: '#e6a23c'
      }
    ]
  }

  createChart({
    type: 'line',
    data: chartData,
    options: {
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: {
          display: true,
          position: 'top'
        },
        tooltip: {
          mode: 'index',
          intersect: false
        }
      },
      scales: {
        y: {
          beginAtZero: true,
          grid: {
            color: 'rgba(0, 0, 0, 0.05)'
          }
        },
        x: {
          grid: {
            display: false
          }
        }
      },
      interaction: {
        mode: 'nearest',
        axis: 'x',
        intersect: false
      }
    }
  })

  // 更新图例
  legendItems.value = [
    {
      label: '引用数',
      color: '#667eea',
      value: data.citations[data.citations.length - 1].toString()
    },
    {
      label: '下载量',
      color: '#67c23a',
      value: data.downloads[data.downloads.length - 1].toString()
    },
    {
      label: '浏览量',
      color: '#e6a23c',
      value: data.views[data.views.length - 1].toString()
    }
  ]
}

const updateChart = () => {
  const data = generateMockData()

  const chartData = {
    labels: data.labels,
    datasets: [
      {
        label: '引用数',
        data: data.citations
      },
      {
        label: '下载量',
        data: data.downloads
      },
      {
        label: '浏览量',
        data: data.views
      }
    ]
  }

  updateChart(chartData)
}

watch(() => props.metrics, () => {
  if (props.metrics) {
    initChart()
  }
})

onMounted(() => {
  initChart()
})
</script>

<style scoped>
.impact-chart-container {
  background: white;
  border-radius: 12px;
  padding: 24px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.chart-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
}

.chart-header h3 {
  font-size: 18px;
  font-weight: 600;
  margin: 0;
}

.chart-wrapper {
  position: relative;
  margin-bottom: 24px;
}

.chart-legend {
  display: flex;
  justify-content: center;
  gap: 24px;
  padding-top: 16px;
  border-top: 1px solid #e4e7ed;
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 8px;
}

.legend-color {
  width: 12px;
  height: 12px;
  border-radius: 2px;
}

.legend-label {
  font-size: 14px;
  color: #606266;
}

.legend-value {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
}
</style>
