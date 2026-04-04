<template>
  <div class="ai-stats-page">
    <!-- Page Header -->
    <div class="page-header">
      <h1 class="page-title">
        <el-icon><DataAnalysis /></el-icon>
        AI 使用统计
      </h1>
      <div class="header-actions">
        <el-button type="primary" :icon="Download" @click="exportStats">
          导出报告
        </el-button>
        <el-button :icon="Refresh" @click="refreshStats" :loading="loading">
          刷新数据
        </el-button>
      </div>
    </div>

    <!-- Stats Content -->
    <div class="stats-content" v-loading="loading" element-loading-text="加载统计数据...">
      <!-- Overview Cards -->
      <el-row :gutter="20" class="stats-row">
        <el-col :xs="24" :sm="12" :md="6">
          <el-card shadow="hover" class="stat-card">
            <div class="stat-content">
              <div class="stat-icon total">
                <el-icon><TrendCharts /></el-icon>
              </div>
              <div class="stat-info">
                <div class="stat-value">{{ stats.totalGenerations }}</div>
                <div class="stat-label">总生成次数</div>
                <div class="stat-trend positive">
                  <span class="trend-icon">↑</span>
                  <span class="trend-value">+{{ stats.growthRate }}%</span>
                </div>
              </div>
            </div>
          </el-card>
        </el-col>
        <el-col :xs="24" :sm="12" :md="6">
          <el-card shadow="hover" class="stat-card">
            <div class="stat-content">
              <div class="stat-icon cost">
                <el-icon><Coin /></el-icon>
              </div>
              <div class="stat-info">
                <div class="stat-value">${{ stats.totalCost.toFixed(2) }}</div>
                <div class="stat-label">总花费</div>
                <div class="stat-trend">
                  <span class="trend-value">本月 ${{ stats.monthlyCost.toFixed(2) }}</span>
                </div>
              </div>
            </div>
          </el-card>
        </el-col>
        <el-col :xs="24" :sm="12" :md="6">
          <el-card shadow="hover" class="stat-card">
            <div class="stat-content">
              <div class="stat-icon time">
                <el-icon><Timer /></el-icon>
              </div>
              <div class="stat-info">
                <div class="stat-value">{{ stats.averageTime }}s</div>
                <div class="stat-label">平均响应时间</div>
                <div class="stat-trend positive">
                  <span class="trend-icon">↓</span>
                  <span class="trend-value">-{{ stats.timeImprovement }}%</span>
                </div>
              </div>
            </div>
          </el-card>
        </el-col>
        <el-col :xs="24" :sm="12" :md="6">
          <el-card shadow="hover" class="stat-card">
            <div class="stat-content">
              <div class="stat-icon success">
                <el-icon><SuccessFilled /></el-icon>
              </div>
              <div class="stat-info">
                <div class="stat-value">{{ stats.successRate }}%</div>
                <div class="stat-label">成功率</div>
                <div class="stat-trend positive">
                  <span class="trend-icon">↑</span>
                  <span class="trend-value">+{{ stats.successRateImprovement }}%</span>
                </div>
              </div>
            </div>
          </el-card>
        </el-col>
      </el-row>

      <!-- Usage by Type -->
      <el-row :gutter="20" class="content-row">
        <el-col :xs="24" :lg="12">
          <el-card shadow="hover" class="chart-card">
            <template #header>
              <div class="card-header">
                <span>
                  <el-icon><PieChart /></el-icon>
                  功能使用分布
                </span>
              </div>
            </template>

            <div class="usage-list">
              <!-- AI Review -->
              <div class="usage-item review">
                <div class="usage-header">
                  <div class="usage-icon">🧠</div>
                  <div class="usage-info">
                    <h4 class="usage-title">AI审稿人</h4>
                    <p class="usage-subtitle">智能论文审稿</p>
                  </div>
                </div>
                <div class="usage-stats">
                  <div class="usage-stat">
                    <span class="stat-label">使用次数</span>
                    <span class="stat-value">{{ stats.byType.review }}</span>
                  </div>
                  <div class="usage-stat">
                    <span class="stat-label">平均评分</span>
                    <span class="stat-value">{{ stats.averageReviewScore }}/10</span>
                  </div>
                  <div class="usage-stat">
                    <span class="stat-label">总花费</span>
                    <span class="stat-value">${{ stats.costByType.review.toFixed(2) }}</span>
                  </div>
                </div>
                <div class="usage-bar">
                  <div class="bar-fill review" :style="{ width: `${getPercentage(stats.byType.review)}%` }"></div>
                </div>
              </div>

              <!-- Literature Review -->
              <div class="usage-item literature">
                <div class="usage-header">
                  <div class="usage-icon">📚</div>
                  <div class="usage-info">
                    <h4 class="usage-title">文献综述生成器</h4>
                    <p class="usage-subtitle">系统性文献综述</p>
                  </div>
                </div>
                <div class="usage-stats">
                  <div class="usage-stat">
                    <span class="stat-label">使用次数</span>
                    <span class="stat-value">{{ stats.byType.literatureReview }}</span>
                  </div>
                  <div class="usage-stat">
                    <span class="stat-label">平均论文数</span>
                    <span class="stat-value">{{ stats.averagePaperCount }}</span>
                  </div>
                  <div class="usage-stat">
                    <span class="stat-label">总花费</span>
                    <span class="stat-value">${{ stats.costByType.literatureReview.toFixed(2) }}</span>
                  </div>
                </div>
                <div class="usage-bar">
                  <div class="bar-fill literature" :style="{ width: `${getPercentage(stats.byType.literatureReview)}%` }"></div>
                </div>
              </div>

              <!-- Research Plan -->
              <div class="usage-item plan">
                <div class="usage-header">
                  <div class="usage-icon">🎯</div>
                  <div class="usage-info">
                    <h4 class="usage-title">研究计划助手</h4>
                    <p class="usage-subtitle">智能研究规划</p>
                  </div>
                </div>
                <div class="usage-stats">
                  <div class="usage-stat">
                    <span class="stat-label">使用次数</span>
                    <span class="stat-value">{{ stats.byType.researchPlan }}</span>
                  </div>
                  <div class="usage-stat">
                    <span class="stat-label">平均可行性</span>
                    <span class="stat-value">{{ stats.averageFeasibility }}/10</span>
                  </div>
                  <div class="usage-stat">
                    <span class="stat-label">总花费</span>
                    <span class="stat-value">${{ stats.costByType.researchPlan.toFixed(2) }}</span>
                  </div>
                </div>
                <div class="usage-bar">
                  <div class="bar-fill plan" :style="{ width: `${getPercentage(stats.byType.researchPlan)}%` }"></div>
                </div>
              </div>
            </div>
          </el-card>
        </el-col>

        <!-- Performance Metrics -->
        <el-col :xs="24" :lg="12">
          <el-card shadow="hover" class="chart-card">
            <template #header>
              <div class="card-header">
                <span>
                  <el-icon><Odometer /></el-icon>
                  性能指标
                </span>
              </div>
            </template>

            <div class="performance-content">
              <!-- Response Time Chart -->
              <div class="performance-section">
                <h4 class="section-title">响应时间趋势</h4>
                <div class="chart-bars">
                  <div v-for="(value, index) in responseTimeChart" :key="index" class="chart-bar">
                    <div class="bar" :style="{ height: `${value}%` }"></div>
                    <span class="bar-label">{{ getDayLabel(index) }}</span>
                  </div>
                </div>
              </div>

              <!-- Success Rate Distribution -->
              <div class="performance-section">
                <h4 class="section-title">成功率分布</h4>
                <div class="distribution-list">
                  <div class="distribution-item">
                    <span class="distribution-label">成功</span>
                    <div class="distribution-bar">
                      <div class="bar-fill success" :style="{ width: `${stats.successRate}%` }"></div>
                    </div>
                    <span class="distribution-value">{{ stats.successRate }}%</span>
                  </div>
                  <div class="distribution-item">
                    <span class="distribution-label">失败</span>
                    <div class="distribution-bar">
                      <div class="bar-fill error" :style="{ width: `${100 - stats.successRate}%` }"></div>
                    </div>
                    <span class="distribution-value">{{ 100 - stats.successRate }}%</span>
                  </div>
                </div>
              </div>

              <!-- Token Usage -->
              <div class="performance-section">
                <h4 class="section-title">Token使用统计</h4>
                <div class="token-stats">
                  <div class="token-stat-item">
                    <span class="token-label">总Token数</span>
                    <span class="token-value">{{ formatNumber(stats.totalTokens) }}</span>
                  </div>
                  <div class="token-stat-item">
                    <span class="token-label">平均每次</span>
                    <span class="token-value">{{ formatNumber(stats.averageTokens) }}</span>
                  </div>
                  <div class="token-stat-item">
                    <span class="token-label">成本/Token</span>
                    <span class="token-value">${{ stats.costPerToken.toFixed(5) }}</span>
                  </div>
                </div>
              </div>
            </div>
          </el-card>
        </el-col>
      </el-row>

      <!-- Cost Analysis -->
      <el-row :gutter="20" class="content-row">
        <el-col :span="24">
          <el-card shadow="hover" class="cost-card">
            <template #header>
              <div class="card-header">
                <span>
                  <el-icon><Wallet /></el-icon>
                  成本分析与优化建议
                </span>
              </div>
            </template>

            <el-row :gutter="20">
              <el-col :xs="24" :md="12">
                <div class="cost-chart-section">
                  <h4 class="section-title">月度成本趋势</h4>
                  <div class="cost-bars">
                    <div v-for="(month, index) in monthlyCostChart" :key="index" class="cost-bar-item">
                      <div class="cost-bar" :style="{ height: `${getCostBarHeight(month.cost)}%` }">
                        <span class="cost-value">${{ month.cost.toFixed(0) }}</span>
                      </div>
                      <span class="cost-label">{{ month.label }}</span>
                    </div>
                  </div>
                </div>
              </el-col>

              <el-col :xs="24" :md="12">
                <div class="cost-tips-section">
                  <h4 class="section-title">成本优化建议</h4>
                  <div class="cost-tips">
                    <div class="cost-tip">
                      <span class="tip-icon">✅</span>
                      <div class="tip-content">
                        <div class="tip-title">使用缓存</div>
                        <div class="tip-description">重复内容使用缓存，节省95%成本</div>
                      </div>
                    </div>
                    <div class="cost-tip">
                      <span class="tip-icon">✅</span>
                      <div class="tip-content">
                        <div class="tip-title">批量处理</div>
                        <div class="tip-description">合并多个请求，减少API调用次数</div>
                      </div>
                    </div>
                    <div class="cost-tip">
                      <span class="tip-icon">✅</span>
                      <div class="tip-content">
                        <div class="tip-title">选择合适的模型</div>
                        <div class="tip-description">简单任务使用Mini版本，节省成本</div>
                      </div>
                    </div>
                  </div>
                </div>
              </el-col>
            </el-row>
          </el-card>
        </el-col>
      </el-row>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useAIStats } from '@/composables/useAIStats'
import {
  DataAnalysis,
  Download,
  Refresh,
  TrendCharts,
  Coin,
  Timer,
  SuccessFilled,
  PieChart,
  Odometer,
  Wallet
} from '@element-plus/icons-vue'

// Use AI stats composable
const {
  stats,
  loading,
  fetchStats,
  refresh: refreshStats
} = useAIStats()

// Chart data (mock)
const responseTimeChart = ref([15, 18, 14, 16, 15, 17, 15])
const monthlyCostChart = ref([
  { label: '1月', cost: 12 },
  { label: '2月', cost: 18 },
  { label: '3月', cost: 16 },
  { label: '4月', cost: 22 },
  { label: '5月', cost: 20 },
  { label: '6月', cost: 25 }
])

// Methods
const getPercentage = (value: number) => {
  const total = stats.value.totalGenerations
  if (total === 0) return 0
  return Math.round((value / total) * 100)
}

const getDayLabel = (index: number) => {
  const days = ['周一', '周二', '周三', '周四', '周五', '周六', '周日']
  return days[index % 7]
}

const getCostBarHeight = (cost: number) => {
  const maxCost = Math.max(...monthlyCostChart.value.map(m => m.cost))
  return (cost / maxCost) * 100
}

const formatNumber = (num: number) => {
  if (num >= 1000000) return `${(num / 1000000).toFixed(1)}M`
  if (num >= 1000) return `${(num / 1000).toFixed(1)}K`
  return num.toString()
}

const exportStats = () => {
  const data = JSON.stringify(stats.value, null, 2)
  const blob = new Blob([data], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `ai-stats-${new Date().toISOString().split('T')[0]}.json`
  a.click()
  URL.revokeObjectURL(url)
}

// Fetch stats on mount
fetchStats()
</script>

<style scoped lang="scss">
.ai-stats-page {
  width: 100%;
  max-width: 1600px;
  margin: 0 auto;
}

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
    box-shadow: $shadow-md;
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

.loading-state {
  min-height: 400px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.stats-content {
  display: flex;
  flex-direction: column;
  gap: $spacing-6;
}

.stats-row,
.content-row {
  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

.stat-card {
  height: 100%;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  transition: all $duration-slow;
  overflow: hidden;

  &:hover {
    box-shadow: $shadow-md;
    transform: translateY(-4px);
  }

  :deep(.el-card__body) {
    padding: $spacing-6;
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  .stat-content {
    display: flex;
    align-items: center;
    gap: $spacing-5;

    .stat-icon {
      width: 64px;
      height: 64px;
      border-radius: $border-radius-lg;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 28px;
      flex-shrink: 0;
      box-shadow: $shadow-sm;

      &.total {
        background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
        color: white;
      }

      &.cost {
        background: linear-gradient(135deg, #67c23a 0%, #85ce61 100%);
        color: white;
      }

      &.time {
        background: linear-gradient(135deg, #e6a23c 0%, #f0c78a 100%);
        color: white;
      }

      &.success {
        background: linear-gradient(135deg, #f56c6c 0%, #f89898 100%);
        color: white;
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
        color: $text-primary;
        margin-bottom: $spacing-2;
      }

      .stat-trend {
        display: flex;
        align-items: center;
        gap: $spacing-1;
        font-size: $font-size-xs;
        font-weight: $font-weight-semibold;

        &.positive {
          color: $success-color;
        }

        .trend-icon {
          font-size: $font-size-sm;
        }
      }
    }
  }
}

.chart-card,
.cost-card {
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
}

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

.usage-list {
  display: flex;
  flex-direction: column;
  gap: $spacing-6;
}

.usage-item {
  background: #ffffff;
  border: 1px solid $border-light;
  border-radius: $border-radius-lg;
  padding: $spacing-5;
  transition: all $duration-fast;

  &:hover {
    box-shadow: $shadow-sm;
  }

  .dark & {
    background: $gray-700;
    border-color: $gray-600;
  }

  .usage-header {
    display: flex;
    align-items: center;
    gap: $spacing-4;
    margin-bottom: $spacing-4;

    .usage-icon {
      font-size: 36px;
    }

    .usage-title {
      margin: 0 0 $spacing-1 0;
      font-size: $font-size-lg;
      font-weight: $font-weight-semibold;
      color: $text-primary;
    }

    .usage-subtitle {
      margin: 0;
      font-size: $font-size-sm;
      color: $text-secondary;
    }
  }

  .usage-stats {
    display: flex;
    justify-content: space-between;
    margin-bottom: $spacing-4;

    .usage-stat {
      display: flex;
      flex-direction: column;
      gap: $spacing-1;

      .stat-label {
        font-size: $font-size-xs;
        color: $text-secondary;
      }

      .stat-value {
        font-size: $font-size-base;
        font-weight: $font-weight-semibold;
        color: $text-primary;
      }
    }
  }

  .usage-bar {
    height: 8px;
    background: $gray-100;
    border-radius: $border-radius-full;
    overflow: hidden;

    .dark & {
      background: $gray-600;
    }

    .bar-fill {
      height: 100%;
      border-radius: $border-radius-full;
      transition: width $duration-slow;

      &.review {
        background: linear-gradient(90deg, $primary-500 0%, $primary-600 100%);
      }

      &.literature {
        background: linear-gradient(90deg, #28a745 0%, #20c997 100%);
      }

      &.plan {
        background: linear-gradient(90deg, #409eff 0%, #0056b3 100%);
      }
    }
  }
}

.performance-content {
  display: flex;
  flex-direction: column;
  gap: $spacing-6;
}

.performance-section {
  .section-title {
    margin: 0 0 $spacing-4 0;
    font-size: $font-size-base;
    font-weight: $font-weight-semibold;
    color: $text-primary;
  }

  .chart-bars {
    display: flex;
    align-items: flex-end;
    justify-content: space-between;
    height: 120px;
    gap: $spacing-2;

    .chart-bar {
      flex: 1;
      display: flex;
      flex-direction: column;
      align-items: center;
      height: 100%;

      .bar {
        width: 100%;
        background: linear-gradient(180deg, $primary-500 0%, $primary-600 100%);
        border-radius: $border-radius-base $border-radius-base 0 0;
        min-height: 20px;
        transition: height $duration-slow;
      }

      .bar-label {
        margin-top: $spacing-2;
        font-size: $font-size-xs;
        color: $text-secondary;
        font-weight: $font-weight-medium;
      }
    }
  }

  .distribution-list {
    display: flex;
    flex-direction: column;
    gap: $spacing-4;

    .distribution-item {
      display: flex;
      align-items: center;
      gap: $spacing-3;

      .distribution-label {
        min-width: 60px;
        font-size: $font-size-sm;
        font-weight: $font-weight-medium;
        color: $text-primary;
      }

      .distribution-bar {
        flex: 1;
        height: 24px;
        background: $gray-100;
        border-radius: $border-radius-full;
        overflow: hidden;

        .dark & {
          background: $gray-700;
        }

        .bar-fill {
          height: 100%;
          display: flex;
          align-items: center;
          padding: 0 $spacing-3;
          font-size: $font-size-xs;
          font-weight: $font-weight-bold;
          color: white;
          transition: width $duration-slow;

          &.success {
            background: $success-color;
          }

          &.error {
            background: $danger-color;
          }
        }
      }

      .distribution-value {
        min-width: 50px;
        text-align: right;
        font-size: $font-size-sm;
        font-weight: $font-weight-bold;
        color: $text-primary;
      }
    }
  }

  .token-stats {
    display: flex;
    flex-direction: column;
    gap: $spacing-3;

    .token-stat-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: $spacing-3;
      background: $gray-50;
      border-radius: $border-radius-lg;

      .dark & {
        background: $gray-700;
      }

      .token-label {
        font-size: $font-size-sm;
        color: $text-regular;
      }

      .token-value {
        font-size: $font-size-base;
        font-weight: $font-weight-bold;
        color: $text-primary;
      }
    }
  }
}

.cost-card {
  :deep(.el-card__body) {
    padding: $spacing-6;
  }
}

.cost-chart-section,
.cost-tips-section {
  .section-title {
    margin: 0 0 $spacing-4 0;
    font-size: $font-size-base;
    font-weight: $font-weight-semibold;
    color: $text-primary;
  }
}

.cost-chart-section {
  .cost-bars {
    display: flex;
    align-items: flex-end;
    justify-content: space-between;
    height: 200px;
    gap: $spacing-3;

    .cost-bar-item {
      flex: 1;
      display: flex;
      flex-direction: column;
      align-items: center;
      height: 100%;

      .cost-bar {
        width: 100%;
        background: linear-gradient(180deg, #67c23a 0%, #85ce61 100%);
        border-radius: $border-radius-base $border-radius-base 0 0;
        min-height: 40px;
        display: flex;
        align-items: flex-start;
        justify-content: center;
        padding-top: $spacing-2;

        .cost-value {
          font-size: $font-size-xs;
          font-weight: $font-weight-bold;
          color: white;
        }
      }

      .cost-label {
        margin-top: $spacing-2;
        font-size: $font-size-xs;
        color: $text-secondary;
        font-weight: $font-weight-medium;
      }
    }
  }
}

.cost-tips-section {
  .cost-tips {
    display: flex;
    flex-direction: column;
    gap: $spacing-4;

    .cost-tip {
      display: flex;
      gap: $spacing-3;
      padding: $spacing-4;
      background: $gray-50;
      border-radius: $border-radius-lg;
      border-left: 4px solid $success-color;

      .dark & {
        background: $gray-700;
      }

      .tip-icon {
        font-size: 20px;
        flex-shrink: 0;
      }

      .tip-content {
        flex: 1;

        .tip-title {
          font-size: $font-size-sm;
          font-weight: $font-weight-semibold;
          color: $text-primary;
          margin-bottom: $spacing-1;
        }

        .tip-description {
          font-size: $font-size-sm;
          color: $text-secondary;
          line-height: 1.5;
        }
      }
    }
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

  .usage-item {
    .usage-stats {
      flex-direction: column;
      gap: $spacing-2;
    }
  }

  .cost-bars {
    height: 150px !important;
  }
}
</style>
