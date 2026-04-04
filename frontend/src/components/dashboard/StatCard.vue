<template>
  <el-card
    :class="['stat-card', `stat-card--${type}`]"
    :body-style="{ padding: '20px' }"
    shadow="hover"
  >
    <div class="stat-card__content">
      <div class="stat-card__icon">
        <el-icon :size="32">
          <component :is="icon" />
        </el-icon>
      </div>
      <div class="stat-card__info">
        <div class="stat-card__value">
          <span v-if="loading" class="loading-skeleton">--</span>
          <span v-else>{{ formattedValue }}</span>
          <el-tag
            v-if="trend && !loading"
            :type="trendType"
            size="small"
            class="stat-card__trend"
          >
            {{ trendText }}
          </el-tag>
        </div>
        <div class="stat-card__label">{{ label }}</div>
        <div v-if="description" class="stat-card__description">
          {{ description }}
        </div>
      </div>
    </div>
  </el-card>
</template>

<script setup lang="ts">
import { computed } from 'vue'

/**
 * 统计卡片组件
 * 用于显示仪表盘中的统计数据
 */
interface Props {
  /** 统计数值 */
  value: number
  /** 标签文本 */
  label: string
  /** 图标组件 */
  icon: any
  /** 卡片类型 */
  type?: 'primary' | 'success' | 'warning' | 'danger' | 'info'
  /** 描述文本 */
  description?: string
  /** 趋势值 (正数为增长,负数为下降) */
  trend?: number
  /** 是否加载中 */
  loading?: boolean
  /** 是否显示千分位 */
  showDelimiter?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  type: 'primary',
  loading: false,
  showDelimiter: true
})

/**
 * 格式化数值显示
 */
const formattedValue = computed(() => {
  if (props.showDelimiter) {
    return props.value.toLocaleString()
  }
  return props.value.toString()
})

/**
 * 趋势标签类型
 */
const trendType = computed(() => {
  if (!props.trend) return 'info'
  return props.trend > 0 ? 'success' : 'danger'
})

/**
 * 趋势文本
 */
const trendText = computed(() => {
  if (!props.trend) return ''
  const absTrend = Math.abs(props.trend)
  const sign = props.trend > 0 ? '+' : ''
  return `${sign}${absTrend}%`
})
</script>

<style scoped lang="scss">
.stat-card {
  transition: all 0.3s ease;
  height: 100%;
  border: none;

  &:hover {
    transform: translateY(-4px);
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.12) !important;
  }

  &__content {
    display: flex;
    align-items: center;
    gap: 16px;
  }

  &__icon {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 64px;
    height: 64px;
    border-radius: 12px;
    color: #fff;
    flex-shrink: 0;
  }

  &__info {
    flex: 1;
    min-width: 0;
  }

  &__value {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 28px;
    font-weight: 700;
    line-height: 1.2;
    margin-bottom: 4px;

    .loading-skeleton {
      color: #c0c4cc;
      animation: pulse 1.5s ease-in-out infinite;
    }
  }

  &__trend {
    font-size: 12px;
    font-weight: 600;
  }

  &__label {
    font-size: 14px;
    color: #606266;
    margin-bottom: 2px;
  }

  &__description {
    font-size: 12px;
    color: #909399;
  }

  // 类型样式
  &--primary {
    .stat-card__icon {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    }
  }

  &--success {
    .stat-card__icon {
      background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
    }
  }

  &--warning {
    .stat-card__icon {
      background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
    }
  }

  &--danger {
    .stat-card__icon {
      background: linear-gradient(135deg, #fa709a 0%, #fee140 100%);
    }
  }

  &--info {
    .stat-card__icon {
      background: linear-gradient(135deg, #30cfd0 0%, #330867 100%);
    }
  }
}

@keyframes pulse {
  0%,
  100% {
    opacity: 1;
  }
  50% {
    opacity: 0.5;
  }
}

// 响应式设计
@media (max-width: 768px) {
  .stat-card {
    &__icon {
      width: 48px;
      height: 48px;

      :deep(.el-icon) {
        font-size: 24px !important;
      }
    }

    &__value {
      font-size: 24px;
    }
  }
}
</style>
