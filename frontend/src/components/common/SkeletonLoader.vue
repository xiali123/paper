<template>
  <div class="skeleton-loader" :class="variant">
    <div v-if="variant === 'card'" class="skeleton-card">
      <div class="skeleton-title"></div>
      <div class="skeleton-meta"></div>
      <div class="skeleton-text"></div>
      <div class="skeleton-text short"></div>
    </div>

    <div v-if="variant === 'list'" class="skeleton-list">
      <div v-for="i in count" :key="i" class="skeleton-list-item">
        <div class="skeleton-avatar"></div>
        <div class="skeleton-content">
          <div class="skeleton-line"></div>
          <div class="skeleton-line short"></div>
        </div>
      </div>
    </div>

    <div v-if="variant === 'text'" class="skeleton-text-block">
      <div class="skeleton-paragraph" v-for="i in count" :key="i">
        <div class="skeleton-line" v-for="j in lineCount" :key="j"></div>
      </div>
    </div>

    <div v-if="variant === 'stats'" class="skeleton-stats">
      <div v-for="i in count" :key="i" class="skeleton-stat-card">
        <div class="skeleton-stat-icon"></div>
        <div class="skeleton-stat-value"></div>
        <div class="skeleton-stat-label"></div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
interface Props {
  variant?: 'card' | 'list' | 'text' | 'stats'
  count?: number
  lineCount?: number
}

withDefaults(defineProps<Props>(), {
  variant: 'card',
  count: 3,
  lineCount: 4
})
</script>

<style scoped>
.skeleton-loader {
  width: 100%;
}

/* Base skeleton animation */
@keyframes skeleton-loading {
  0% {
    background-position: -200px 0;
  }
  100% {
    background-position: calc(200px + 100%) 0;
  }
}

.skeleton-card,
.skeleton-list-item,
.skeleton-stat-card {
  background: linear-gradient(
    90deg,
    var(--color-bg-secondary) 0px,
    var(--color-bg-tertiary) 40px,
    var(--color-bg-secondary) 80px
  );
  background-size: 200px 100%;
  animation: skeleton-loading 1.5s ease-in-out infinite;
}

/* Card variant */
.skeleton-card {
  padding: 20px;
  border-radius: 12px;
  margin-bottom: 16px;
}

.skeleton-title {
  height: 24px;
  width: 70%;
  border-radius: 4px;
  margin-bottom: 12px;
  background: linear-gradient(
    90deg,
    var(--color-bg-secondary) 0px,
    var(--color-bg-tertiary) 40px,
    var(--color-bg-secondary) 80px
  );
  background-size: 200px 100%;
  animation: skeleton-loading 1.5s ease-in-out infinite;
}

.skeleton-meta {
  height: 16px;
  width: 40%;
  border-radius: 4px;
  margin-bottom: 16px;
  background: linear-gradient(
    90deg,
    var(--color-bg-secondary) 0px,
    var(--color-bg-tertiary) 40px,
    var(--color-bg-secondary) 80px
  );
  background-size: 200px 100%;
  animation: skeleton-loading 1.5s ease-in-out infinite;
}

.skeleton-text {
  height: 14px;
  width: 100%;
  border-radius: 4px;
  margin-bottom: 8px;
  background: linear-gradient(
    90deg,
    var(--color-bg-secondary) 0px,
    var(--color-bg-tertiary) 40px,
    var(--color-bg-secondary) 80px
  );
  background-size: 200px 100%;
  animation: skeleton-loading 1.5s ease-in-out infinite;
}

.skeleton-text.short {
  width: 60%;
}

/* List variant */
.skeleton-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.skeleton-list-item {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 16px;
  border-radius: 8px;
}

.skeleton-avatar {
  width: 48px;
  height: 48px;
  border-radius: 50%;
  flex-shrink: 0;
  background: linear-gradient(
    90deg,
    var(--color-bg-secondary) 0px,
    var(--color-bg-tertiary) 40px,
    var(--color-bg-secondary) 80px
  );
  background-size: 200px 100%;
  animation: skeleton-loading 1.5s ease-in-out infinite;
}

.skeleton-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.skeleton-line {
  height: 14px;
  width: 100%;
  border-radius: 4px;
  background: linear-gradient(
    90deg,
    var(--color-bg-secondary) 0px,
    var(--color-bg-tertiary) 40px,
    var(--color-bg-secondary) 80px
  );
  background-size: 200px 100%;
  animation: skeleton-loading 1.5s ease-in-out infinite;
}

.skeleton-line.short {
  width: 60%;
}

/* Text variant */
.skeleton-text-block {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.skeleton-paragraph {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

/* Stats variant */
.skeleton-stats {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 20px;
}

.skeleton-stat-card {
  padding: 30px;
  border-radius: 15px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 16px;
}

.skeleton-stat-icon {
  width: 48px;
  height: 48px;
  border-radius: 50%;
  background: linear-gradient(
    90deg,
    var(--color-bg-secondary) 0px,
    var(--color-bg-tertiary) 40px,
    var(--color-bg-secondary) 80px
  );
  background-size: 200px 100%;
  animation: skeleton-loading 1.5s ease-in-out infinite;
}

.skeleton-stat-value {
  height: 36px;
  width: 80%;
  border-radius: 4px;
  background: linear-gradient(
    90deg,
    var(--color-bg-secondary) 0px,
    var(--color-bg-tertiary) 40px,
    var(--color-bg-secondary) 80px
  );
  background-size: 200px 100%;
  animation: skeleton-loading 1.5s ease-in-out infinite;
}

.skeleton-stat-label {
  height: 16px;
  width: 60%;
  border-radius: 4px;
  background: linear-gradient(
    90deg,
    var(--color-bg-secondary) 0px,
    var(--color-bg-tertiary) 40px,
    var(--color-bg-secondary) 80px
  );
  background-size: 200px 100%;
  animation: skeleton-loading 1.5s ease-in-out infinite;
}
</style>
