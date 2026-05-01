/**
 * Chart.js Composable
 * 数据可视化图表
 */

import { ref, onMounted, onUnmounted, watch } from 'vue'
import { Chart, ChartConfiguration, ChartType, CategoryScale, LinearScale, PointElement, LineElement, BarElement, ArcElement, RadialLinearScale, Title, Tooltip, Legend, Filler } from 'chart.js'

Chart.register(CategoryScale, LinearScale, PointElement, LineElement, BarElement, ArcElement, RadialLinearScale, Title, Tooltip, Legend, Filler)

export interface ChartOptions {
  type: ChartType
  data: ChartConfiguration['data']
  options?: ChartConfiguration['options']
  responsive?: boolean
  maintainAspectRatio?: boolean
}

export function useChart(canvasId: string) {
  const chart = ref<Chart | null>(null)
  const canvasElement = ref<HTMLCanvasElement | null>(null)

  const createChart = (options: ChartOptions) => {
    if (!canvasElement.value) {
      console.error('Canvas element not found')
      return
    }

    // 销毁已存在的图表
    if (chart.value) {
      chart.value.destroy()
    }

    const config: ChartConfiguration = {
      type: options.type,
      data: options.data,
      options: {
        responsive: options.responsive ?? true,
        maintainAspectRatio: options.maintainAspectRatio ?? true,
        plugins: {
          legend: {
            display: true,
            position: 'top'
          },
          tooltip: {
            enabled: true,
            mode: 'index',
            intersect: false
          }
        },
        ...options.options
      }
    }

    chart.value = new Chart(canvasElement.value, config)
  }

  const updateChart = (newData: ChartConfiguration['data']) => {
    if (chart.value) {
      chart.value.data = newData
      chart.value.update()
    }
  }

  const destroyChart = () => {
    if (chart.value) {
      chart.value.destroy()
      chart.value = null
    }
  }

  onUnmounted(() => {
    destroyChart()
  })

  return {
    canvasElement,
    chart,
    createChart,
    updateChart,
    destroyChart
  }
}

// 预设图表配置
export const chartPresets = {
  // 折线图 - 趋势分析
  lineChart: (labels: string[], datasets: any[]) => ({
    datasets: datasets.map(dataset => ({
      label: dataset.label,
      data: dataset.data,
      borderColor: dataset.color,
      backgroundColor: dataset.backgroundColor || 'transparent',
      borderWidth: 2,
      tension: 0.4,
      fill: false
    }))
  }),

  // 柱状图 - 统计对比
  barChart: (labels: string[], datasets: any[]) => ({
    datasets: datasets.map(dataset => ({
      label: dataset.label,
      data: dataset.data,
      backgroundColor: dataset.backgroundColor,
      borderColor: dataset.borderColor,
      borderWidth: 1
    }))
  }),

  // 饼图 - 分布占比
  pieChart: (labels: string[], datasets: any[]) => ({
    datasets: datasets.map(dataset => ({
      data: dataset.data,
      backgroundColor: dataset.backgroundColor,
      borderColor: '#fff',
      borderWidth: 2
    }))
  }),

  // 雷达图 - 多维度评估
  radarChart: (labels: string[], datasets: any[]) => ({
    datasets: datasets.map(dataset => ({
      label: dataset.label,
      data: dataset.data,
      backgroundColor: dataset.backgroundColor || 'rgba(102, 126, 234, 0.2)',
      borderColor: dataset.borderColor || 'rgb(102, 126, 234)',
      borderWidth: 2,
      pointBackgroundColor: dataset.borderColor || 'rgb(102, 126, 234)'
    }))
  })
}
