<template>
  <div class="ai-research-plan-page">
    <!-- Page Header -->
    <div class="page-header">
      <h1 class="page-title">
        <el-icon><Notebook /></el-icon>
        AI 研究计划助手
      </h1>
      <div class="header-actions">
        <el-button type="primary" :icon="Plus" @click="handleNewPlan">
          创建计划
        </el-button>
        <el-button :icon="Clock" @click="$router.push('/ai/history')">
          历史记录
        </el-button>
      </div>
    </div>

    <!-- Statistics Cards -->
    <el-row :gutter="20" class="stats-row">
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon total">
              <el-icon><Document /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ stats.totalPlans || 0 }}</div>
              <div class="stat-label">总计划数</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon success">
              <el-icon><Timer /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">~18s</div>
              <div class="stat-label">生成时间</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon duration">
              <el-icon><Calendar /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">60</div>
              <div class="stat-label">最大周期(月)</div>
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
              <div class="stat-value">95%</div>
              <div class="stat-label">成本降低</div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Main Content -->
    <el-row :gutter="20" class="main-content">
      <el-col :xs="24" :lg="16">
        <el-card shadow="hover" class="plan-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><Edit /></el-icon>
                研究计划生成器
              </span>
            </div>
          </template>
          <ResearchPlanVisualization />
        </el-card>
      </el-col>

      <el-col :xs="24" :lg="8">
        <el-card shadow="hover" class="info-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><InfoFilled /></el-icon>
                功能特点
              </span>
            </div>
          </template>
          <div class="feature-list">
            <div class="feature-item">
              <div class="feature-icon">
                <el-icon><Medal /></el-icon>
              </div>
              <div class="feature-text">
                <h4>SMART目标</h4>
                <p>自动设定研究目标</p>
              </div>
            </div>
            <div class="feature-item">
              <div class="feature-icon">
                <el-icon><Tools /></el-icon>
              </div>
              <div class="feature-text">
                <h4>方法论设计</h4>
                <p>详细研究方法</p>
              </div>
            </div>
            <div class="feature-item">
              <div class="feature-icon">
                <el-icon><DataLine /></el-icon>
              </div>
              <div class="feature-text">
                <h4>时间线规划</h4>
                <p>可视化阶段安排</p>
              </div>
            </div>
            <div class="feature-item">
              <div class="feature-icon">
                <el-icon><Wallet /></el-icon>
              </div>
              <div class="feature-text">
                <h4>预算评估</h4>
                <p>资源需求分析</p>
              </div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import {
  Notebook,
  Plus,
  Clock,
  Document,
  Timer,
  Calendar,
  Coin,
  Edit,
  InfoFilled,
  Medal,
  Tools,
  DataLine,
  Wallet
} from '@element-plus/icons-vue'
import ResearchPlanVisualization from '@/components/ai/ResearchPlanVisualization.vue'

const stats = ref({
  totalPlans: 0
})

const handleNewPlan = () => {
  if (import.meta.env.DEV) console.log('Creating new research plan')
}
</script>

<style scoped lang="scss">
.ai-research-plan-page {
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
      color: #409eff;
    }
  }

  .header-actions {
    display: flex;
    gap: $spacing-3;
  }
}

.stats-row {
  margin-bottom: $spacing-6;

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
        background: linear-gradient(135deg, #409eff 0%, #66b1ff 100%);
        color: white;
      }

      &.success {
        background: linear-gradient(135deg, #67c23a 0%, #85ce61 100%);
        color: white;
      }

      &.duration {
        background: linear-gradient(135deg, #e6a23c 0%, #f0c78a 100%);
        color: white;
      }

      &.cost {
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
        color: $text-regular;
        font-weight: $font-weight-medium;
      }
    }
  }
}

.main-content {
  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

.plan-card,
.info-card {
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
    color: #409eff;
  }
}

.feature-list {
  display: flex;
  flex-direction: column;
  gap: $spacing-5;

  .feature-item {
    display: flex;
    gap: $spacing-4;
    align-items: flex-start;

    .feature-icon {
      width: 40px;
      height: 40px;
      border-radius: $border-radius-lg;
      background: rgba(#409eff, 0.1);
      display: flex;
      align-items: center;
      justify-content: center;
      flex-shrink: 0;
      color: #409eff;

      .dark & {
        background: rgba($gray-700, 0.5);
        color: #66b1ff;
      }

      .el-icon {
        font-size: $font-size-xl;
      }
    }

    .feature-text {
      flex: 1;

      h4 {
        margin: 0 0 $spacing-1 0;
        font-size: $font-size-base;
        font-weight: $font-weight-semibold;
        color: $text-primary;

        .dark & {
          color: $gray-100;
        }
      }

      p {
        margin: 0;
        font-size: $font-size-sm;
        color: $text-regular;
        line-height: 1.5;
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
}
</style>
