<template>
  <div class="interest-radar-container">
    <div class="chart-header">
      <h3>🎯 研究兴趣分布</h3>
      <el-select v-model="selectedUserId" placeholder="选择用户" size="small" style="width: 150px">
        <el-option label="我" :value="0" />
        <el-option label="同行平均" :value="1" />
      </el-select>
    </div>

    <div class="chart-wrapper">
      <canvas
        ref="chartCanvas"
        :id="chartId"
        style="max-height: 400px; width: 100%"
      ></canvas>
    </div>

    <div class="interest-details">
      <div
        v-for="(interest, index) in topInterests"
        :key="index"
        class="interest-item"
      >
        <div class="interest-header">
          <span class="interest-keyword">{{ interest.keyword }}</span>
          <span class="interest-score">{{ interest.score.toFixed(2) }}</span>
        </div>
        <div class="interest-bar">
          <div
            class="interest-fill"
            :style="{
              width: (interest.score * 100) + '%',
              background: getInterestColor(interest.trend)
            }"
          ></div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, computed } from 'vue'
import { useChart, chartPresets } from '@/composables/useChart'
import type { ResearchInterest } from '@/types/analytics'

interface Props {
  chartId?: string
  interests?: ResearchInterest[]
}

const props = withDefaults(defineProps<Props>(), {
  chartId: () => `radar-chart-${Date.now()}`,
  interests: undefined
})

const selectedUserId = ref(0)
const { canvasElement, createChart } = useChart(props.chartId)

const allInterests = ref<ResearchInterest[]>([])
const topInterests = computed(() => {
  return allInterests.value
    .sort((a, b) => b.score - a.score)
    .slice(0, 5)
})

const generateMockData = () => {
  return [
    { keyword: '机器学习', score: 0.95, trend: 'rising' as const, paperCount: 15 },
    { keyword: '深度学习', score: 0.88, trend: 'rising' as const, paperCount: 12 },
    { keyword: '自然语言处理', score: 0.82, trend: 'stable' as const, paperCount: 10 },
    { keyword: '计算机视觉', score: 0.75, trend: 'stable' as const, paperCount: 8 },
    { keyword: '强化学习', score: 0.68, trend: 'rising' as const, paperCount: 6 },
    { keyword: '知识图谱', score: 0.55, trend: 'declining' as const, paperCount: 4 },
    { keyword: '数据挖掘', score: 0.48, trend: 'stable' as const, paperCount: 3 },
    { keyword: '推荐系统', score: 0.42, trend: 'rising' as const, paperCount: 2 }
  ]
}

const initChart = () => {
  const interests = props.interests || generateMockData()
  allInterests.value = interests

  // 取前5个兴趣
  const top5 = interests.slice(0, 5)

  const chartData = {
    labels: top5.map(i => i.keyword),
    datasets: [
      {
        label: '兴趣强度',
        data: top5.map(i => i.score),
        backgroundColor: 'rgba(102, 126, 234, 0.2)',
        borderColor: 'rgb(102, 126, 234)',
        pointBackgroundColor: 'rgb(102, 126, 234)',
        pointBorderColor: '#fff',
        pointHoverBackgroundColor: '#fff',
        pointHoverBorderColor: 'rgb(102, 126, 234)'
      }
    ]
  }

  createChart({
    type: 'radar',
    data: chartData,
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
  })
}

const getInterestColor = (trend: string) => {
  if (trend === 'rising') return 'linear-gradient(90deg, #67c23a 0%, #85ce61 100%)'
  if (trend === 'stable') return 'linear-gradient(90deg, #409eff 0%, #66b1ff 100%)'
  return 'linear-gradient(90deg, #909399 0%, #b1b3b8 100%)'
}

onMounted(() => {
  initChart()
})
</script>

<style scoped>
.interest-radar-container {
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

.interest-details {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.interest-item {
  padding: 12px;
  background: #f5f7fa;
  border-radius: 8px;
}

.interest-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
}

.interest-keyword {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
}

.interest-score {
  font-size: 16px;
  font-weight: 700;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.interest-bar {
  height: 8px;
  background: #e4e7ed;
  border-radius: 4px;
  overflow: hidden;
}

.interest-fill {
  height: 100%;
  transition: width 0.5s ease;
}
</style>
