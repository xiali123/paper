<template>
  <div class="welcome-banner">
    <div class="welcome-banner__background">
      <div class="welcome-banner__gradient"></div>
      <div class="welcome-banner__pattern"></div>
    </div>

    <div class="welcome-banner__content">
      <div class="welcome-banner__main">
        <div class="welcome-banner__greeting">
          <h1 class="welcome-banner__title">
            {{ greeting }}, {{ userName }}!
          </h1>
          <p class="welcome-banner__subtitle">{{ welcomeMessage }}</p>
        </div>

        <div class="welcome-banner__info">
          <div class="welcome-banner__date">
            <el-icon :size="18"><Calendar /></el-icon>
            <span>{{ currentDate }}</span>
          </div>
          <div class="welcome-banner__weather" v-if="weather">
            <el-icon :size="18"><Sunny /></el-icon>
            <span>{{ weather.temperature }}°C {{ weatherCondition }}</span>
          </div>
        </div>
      </div>

      <div class="welcome-banner__actions">
        <el-button type="primary" size="large" @click="handleStart">
          <el-icon class="ml-2"><Right /></el-icon>
          开始使用
        </el-button>
        <el-button size="large" @click="handleTour">
          <el-icon class="ml-2"><Guide /></el-icon>
          功能导览
        </el-button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { Calendar, Sunny, Right, Guide } from '@element-plus/icons-vue'
import { format } from 'date-fns'
import { zhCN } from 'date-fns/locale'
import type { WeatherInfo } from '@/types/dashboard'

/**
 * 欢迎横幅组件
 * 显示用户问候、日期、天气等信息
 */
interface Props {
  /** 用户名 */
  userName?: string
  /** 天气信息 */
  weather?: WeatherInfo
}

const props = withDefaults(defineProps<Props>(), {
  userName: '研究者',
  weather: undefined
})

const emit = defineEmits<{
  (e: 'start'): void
  (e: 'tour'): void
}>()

const currentHour = ref(new Date().getHours())

/**
 * 问候语
 */
const greeting = computed(() => {
  const hour = currentHour.value
  if (hour < 6) return '凌晨好'
  if (hour < 9) return '早上好'
  if (hour < 12) return '上午好'
  if (hour < 14) return '中午好'
  if (hour < 18) return '下午好'
  if (hour < 22) return '晚上好'
  return '夜深了'
})

/**
 * 欢迎消息
 */
const welcomeMessage = computed(() => {
  const hour = currentHour.value
  const messages = [
    '今天也是充满发现的一天',
    '准备好开始新的研究探索了吗',
    '让知识引领你前行',
    '探索未知，发现精彩',
    '每一天都是新的开始',
    '保持好奇心，继续探索',
    '研究之路，永不止步'
  ]

  if (hour < 12) {
    return messages[Math.floor(Math.random() * messages.length)]
  } else if (hour < 18) {
    return '下午好，继续加油'
  } else {
    return '晚上好，适时休息'
  }
})

/**
 * 当前日期
 */
const currentDate = computed(() => {
  return format(new Date(), 'yyyy年MM月dd日 EEEE', { locale: zhCN })
})

/**
 * 天气状况
 */
const weatherCondition = computed(() => {
  if (!props.weather) return ''

  const conditions: Record<WeatherInfo['condition'], string> = {
    sunny: '晴',
    cloudy: '多云',
    rainy: '雨',
    snowy: '雪'
  }

  return conditions[props.weather.condition] || ''
})

/**
 * 开始使用
 */
const handleStart = () => {
  emit('start')
}

/**
 * 功能导览
 */
const handleTour = () => {
  emit('tour')
}

onMounted(() => {
  // 每分钟更新一次时间
  const timer = setInterval(() => {
    currentHour.value = new Date().getHours()
  }, 60000)

  // 组件销毁时清除定时器
  return () => clearInterval(timer)
})
</script>

<style scoped lang="scss">
.welcome-banner {
  position: relative;
  overflow: hidden;
  border-radius: 12px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: #fff;
  margin-bottom: 24px;

  &__background {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    z-index: 0;
  }

  &__gradient {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: linear-gradient(135deg, rgba(102, 126, 234, 0.9) 0%, rgba(118, 75, 162, 0.9) 100%);
  }

  &__pattern {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-image:
      radial-gradient(circle at 20% 50%, rgba(255, 255, 255, 0.1) 0%, transparent 50%),
      radial-gradient(circle at 80% 80%, rgba(255, 255, 255, 0.1) 0%, transparent 50%);
    animation: pattern-move 20s linear infinite;
  }

  &__content {
    position: relative;
    z-index: 1;
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 32px 40px;
    gap: 24px;
  }

  &__main {
    flex: 1;
  }

  &__greeting {
    margin-bottom: 16px;
  }

  &__title {
    font-size: 32px;
    font-weight: 700;
    margin-bottom: 8px;
    line-height: 1.2;
  }

  &__subtitle {
    font-size: 16px;
    opacity: 0.9;
    margin: 0;
  }

  &__info {
    display: flex;
    gap: 24px;
    margin-top: 16px;
  }

  &__date,
  &__weather {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 14px;
    opacity: 0.9;
  }

  &__actions {
    display: flex;
    gap: 12px;
    flex-shrink: 0;

    .el-button {
      border-color: rgba(255, 255, 255, 0.3);
      color: #fff;

      &:hover {
        background: rgba(255, 255, 255, 0.1);
        border-color: rgba(255, 255, 255, 0.5);
      }

      &.el-button--primary {
        background: #fff;
        color: #667eea;
        border-color: #fff;

        &:hover {
          background: rgba(255, 255, 255, 0.9);
        }
      }
    }
  }
}

@keyframes pattern-move {
  0% {
    transform: translate(0, 0);
  }
  50% {
    transform: translate(10px, 10px);
  }
  100% {
    transform: translate(0, 0);
  }
}

// 响应式设计
@media (max-width: 1024px) {
  .welcome-banner__content {
    flex-direction: column;
    align-items: flex-start;
  }

  .welcome-banner__actions {
    width: 100%;
    flex-direction: column;

    .el-button {
      width: 100%;
    }
  }
}

@media (max-width: 768px) {
  .welcome-banner {
    border-radius: 8px;
    margin-bottom: 16px;

    &__content {
      padding: 24px 20px;
    }

    &__title {
      font-size: 24px;
    }

    &__subtitle {
      font-size: 14px;
    }

    &__info {
      flex-direction: column;
      gap: 8px;
    }

    &__actions {
      .el-button {
        font-size: 14px;
        padding: 12px 20px;
      }
    }
  }
}

@media (max-width: 480px) {
  .welcome-banner__title {
    font-size: 20px;
  }
}
</style>
