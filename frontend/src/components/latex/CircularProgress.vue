<template>
  <Teleport to="body">
    <transition name="fade">
      <div v-if="visible" class="circular-progress-overlay" @click="closeOnOverlay && $emit('close')">
        <div class="circular-progress-container" @click.stop>
          <div class="progress-ring">
            <svg class="progress-ring-svg" :width="size" :height="size">
              <circle
                class="progress-ring-circle-bg"
                :stroke-width="strokeWidth"
                fill="transparent"
                :r="radius"
                :cx="center"
                :cy="center"
              />
              <circle
                class="progress-ring-circle"
                :stroke-width="strokeWidth"
                fill="transparent"
                :r="radius"
                :cx="center"
                :cy="center"
                :style="circleStyle"
              />
            </svg>
            <div class="progress-content">
              <div class="progress-icon">
                <el-icon class="is-loading" :size="iconSize">
                  <Loading />
                </el-icon>
              </div>
              <div class="progress-text">{{ text }}</div>
              <div v-if="showPercentage" class="progress-percentage">
                {{ Math.round(progress) }}%
              </div>
            </div>
          </div>
          <div v-if="showCancel" class="progress-cancel">
            <el-button @click="$emit('cancel')" size="small" text>
              <el-icon><Close /></el-icon>
              取消
            </el-button>
          </div>
        </div>
      </div>
    </transition>
  </Teleport>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { Loading, Close } from '@element-plus/icons-vue'

interface Props {
  visible?: boolean
  text?: string
  progress?: number
  size?: number
  strokeWidth?: number
  showPercentage?: boolean
  showCancel?: boolean
  closeOnOverlay?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  visible: false,
  text: '加载中...',
  progress: 0,
  size: 120,
  strokeWidth: 8,
  showPercentage: false,
  showCancel: true,
  closeOnOverlay: false
})

defineEmits<{
  'close': []
  'cancel': []
}>()

const center = computed(() => props.size / 2)
const radius = computed(() => (props.size - props.strokeWidth) / 2)
const iconSize = computed(() => props.size * 0.3)
const circumference = computed(() => 2 * Math.PI * radius.value)

const circleStyle = computed(() => {
  const offset = circumference.value - (props.progress / 100) * circumference.value
  return {
    strokeDasharray: `${circumference.value} ${circumference.value}`,
    strokeDashoffset: `${offset}px`
  }
})
</script>

<style scoped lang="scss">
.circular-progress-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(0, 0, 0, 0.5);
  backdrop-filter: blur(4px);
  z-index: 9999;
}

.circular-progress-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 24px;
}

.progress-ring {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
}

.progress-ring-svg {
  transform: rotate(-90deg);
  filter: drop-shadow(0 4px 8px rgba(0, 0, 0, 0.2));
}

.progress-ring-circle-bg {
  stroke: rgba(255, 255, 255, 0.1);
}

.progress-ring-circle {
  stroke: var(--el-color-primary, #409eff);
  transition: stroke-dashoffset 0.35s;
  stroke-linecap: round;
}

.progress-content {
  position: absolute;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
}

.progress-icon {
  color: var(--el-color-primary, #409eff);
}

.progress-text {
  font-size: 14px;
  font-weight: 500;
  color: #fff;
  text-align: center;
  max-width: 150px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.progress-percentage {
  font-size: 18px;
  font-weight: 600;
  color: #fff;
}

.progress-cancel {
  .el-button {
    color: rgba(255, 255, 255, 0.8);

    &:hover {
      color: #fff;
    }
  }
}

// 动画
.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s, transform 0.3s;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

.fade-enter-from .circular-progress-container,
.fade-leave-to .circular-progress-container {
  transform: scale(0.9);
}
</style>
