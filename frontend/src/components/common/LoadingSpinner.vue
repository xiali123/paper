<template>
  <div class="loading-spinner" :class="[`size-${size}`, `variant-${variant}`]">
    <div class="spinner-ring">
      <div class="spinner-segment" v-for="i in 8" :key="i" :style="getSegmentStyle(i)"></div>
    </div>
    <div v-if="text" class="spinner-text">{{ text }}</div>
  </div>
</template>

<script setup lang="ts">
interface Props {
  size?: 'small' | 'medium' | 'large'
  variant?: 'primary' | 'secondary' | 'white'
  text?: string
}

const props = withDefaults(defineProps<Props>(), {
  size: 'medium',
  variant: 'primary'
})

const getSegmentStyle = (index: number) => {
  const delay = (index - 1) * 0.1
  return {
    animationDelay: `${delay}s`
  }
}
</script>

<style scoped>
.loading-spinner {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 16px;
}

.spinner-ring {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
}

.spinner-ring::before {
  content: '';
  position: absolute;
  inset: -8px;
  border-radius: 50%;
  background: radial-gradient(circle, rgba(99, 102, 241, 0.2) 0%, transparent 70%);
  animation: pulse 2s ease-in-out infinite;
}

.spinner-segment {
  position: absolute;
  width: 12%;
  height: 40%;
  border-radius: 50%;
  animation: spinner-fade 1.2s linear infinite;
  backdrop-filter: blur(10px);
}

.size-small .spinner-ring {
  width: 24px;
  height: 24px;
}

.size-medium .spinner-ring {
  width: 40px;
  height: 40px;
}

.size-large .spinner-ring {
  width: 64px;
  height: 64px;
}

.variant-primary .spinner-segment {
  background: var(--color-primary);
}

.variant-secondary .spinner-segment {
  background: var(--color-text-secondary);
}

.variant-white .spinner-segment {
  background: var(--color-text-inverse);
}

@keyframes spinner-fade {
  0%, 100% {
    opacity: 0.15;
  }
  50% {
    opacity: 1;
  }
}

.spinner-text {
  font-size: 14px;
  color: var(--color-text-secondary);
  font-weight: 500;
}
</style>
