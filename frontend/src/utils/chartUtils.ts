/**
 * Chart.js Utilities
 * 图表工具函数
 */

import { Chart, ChartConfiguration, ChartType, ChartData } from 'chart.js/auto'

/**
 * 图表颜色主题
 */
export const chartColors = {
  primary: '#667eea',
  secondary: '#764ba2',
  success: '#67c23a',
  warning: '#e6a23c',
  danger: '#f56c6c',
  info: '#409eff',

  gradients: {
    primary: 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)',
    success: 'linear-gradient(135deg, #67c23a 0%, #85ce61 100%)',
    warning: 'linear-gradient(135deg, #e6a23c 0%, #f0c78a 100%)',
    danger: 'linear-gradient(135deg, #f56c6c 0%, #f78ca0 100%)',
    info: 'linear-gradient(135deg, #409eff 0%, #66b1ff 100%)'
  }
}

/**
 * 创建折线图配置
 */
export function createLineChartConfig(
  labels: string[],
  datasets: Array<{ label: string; data: number[]; color: string }>
): ChartConfiguration {
  return {
    type: 'line',
    data: {
      labels,
      datasets: datasets.map(ds => ({
        label: ds.label,
        data: ds.data,
        borderColor: ds.color,
        backgroundColor: ds.color + '20',
        borderWidth: 2,
        tension: 0.4,
        fill: true
      }))
    },
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
      }
    }
  }
}

/**
 * 创建柱状图配置
 */
export function createBarChartConfig(
  labels: string[],
  datasets: Array<{ label: string; data: number[]; color: string }>
): ChartConfiguration {
  return {
    type: 'bar',
    data: {
      labels,
      datasets: datasets.map(ds => ({
        label: ds.label,
        data: ds.data,
        backgroundColor: ds.color,
        borderColor: ds.color + 'cc',
        borderWidth: 1
      }))
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: {
          display: true,
          position: 'top'
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
      }
    }
  }
}

/**
 * 创建饼图配置
 */
export function createPieChartConfig(
  labels: string[],
  data: number[]
): ChartConfiguration {
  const colors = [
    chartColors.primary,
    chartColors.success,
    chartColors.warning,
    chartColors.danger,
    chartColors.info
  ]

  return {
    type: 'pie',
    data: {
      labels,
      datasets: [{
        data,
        backgroundColor: colors.map(c => c + 'cc'),
        borderColor: '#fff',
        borderWidth: 2
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: {
          display: true,
          position: 'right'
        }
      }
    }
  }
}

/**
 * 创建雷达图配置
 */
export function createRadarChartConfig(
  labels: string[],
  data: number[],
  label: string = 'Score'
): ChartConfiguration {
  return {
    type: 'radar',
    data: {
      labels,
      datasets: [{
        label,
        data,
        backgroundColor: chartColors.primary + '40',
        borderColor: chartColors.primary,
        pointBackgroundColor: chartColors.primary,
        pointBorderColor: '#fff',
        pointHoverBackgroundColor: '#fff',
        pointHoverBorderColor: chartColors.primary
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: {
          display: false
        }
      },
      scales: {
        r: {
          beginAtZero: true,
          max: 1,
          ticks: {
            stepSize: 0.2
          },
          grid: {
            color: 'rgba(0, 0, 0, 0.05)'
          },
          angleLines: {
            color: 'rgba(0, 0, 0, 0.05)'
          },
          pointLabels: {
            font: {
              size: 12
            }
          }
        }
      }
    }
  }
}

/**
 * 图表字体配置
 */
export const chartDefaultOptions = {
  plugins: {
    legend: {
      labels: {
        font: {
          family: "'Inter', sans-serif",
          size: 12
        }
      }
    },
    tooltip: {
      titleFont: {
        family: "'Inter', sans-serif",
        size: 14,
        weight: 'bold'
      },
      bodyFont: {
        family: "'Inter', sans-serif",
        size: 12
      }
    }
  },
  scales: {
    x: {
      ticks: {
        font: {
          family: "'Inter', sans-serif",
          size: 11
        }
      },
      grid: {
        color: 'rgba(0, 0, 0, 0.05)'
      }
    },
    y: {
      ticks: {
        font: {
          family: "'Inter', sans-serif",
          size: 11
        }
      },
      grid: {
        color: 'rgba(0, 0, 0, 0.05)'
      }
    }
  }
}
