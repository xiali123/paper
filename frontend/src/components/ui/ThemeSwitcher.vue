<template>
  <el-dropdown trigger="click" @command="handleCommand">
    <el-button class="theme-switcher" :icon="icon" circle />
    <template #dropdown>
      <el-dropdown-menu>
        <el-dropdown-item command="light">
          <el-icon class="menu-icon"><Sunny /></el-icon>
          <span>浅色模式</span>
        </el-dropdown-item>
        <el-dropdown-item command="dark">
          <el-icon class="menu-icon"><Moon /></el-icon>
          <span>深色模式</span>
        </el-dropdown-item>
        <el-dropdown-item command="auto">
          <el-icon class="menu-icon"><Monitor /></el-icon>
          <span>跟随系统</span>
        </el-dropdown-item>
      </el-dropdown-menu>
    </template>
  </el-dropdown>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { Sunny, Moon, Monitor } from '@element-plus/icons-vue'
import { useDarkMode, type ThemeMode } from '@/composables/useDarkMode'

const { themeMode, setThemeMode } = useDarkMode()

const icon = computed(() => {
  switch (themeMode.value) {
    case 'light':
      return Sunny
    case 'dark':
      return Moon
    case 'auto':
      return Monitor
    default:
      return Monitor
  }
})

const handleCommand = (command: ThemeMode) => {
  setThemeMode(command)
}
</script>

<style scoped lang="scss">
.theme-switcher {
  --el-button-bg-color: var(--el-fill-color-light);
  --el-button-border-color: var(--el-border-color);
  --el-button-text-color: var(--el-text-color-regular);
  --el-button-hover-bg-color: var(--el-fill-color);
  --el-button-hover-border-color: var(--el-border-color);
  --el-button-hover-text-color: var(--el-color-primary);

  transition: all 0.3s;
}

.menu-icon {
  margin-right: 8px;
  font-size: 16px;
}

// 深色模式样式
:global(.dark-mode) {
  .theme-switcher {
    --el-button-bg-color: #2B2B2C;
    --el-button-border-color: #4C4D4F;
    --el-button-text-color: #E5EAF3;
    --el-button-hover-bg-color: #363637;
    --el-button-hover-border-color: #4C4D4F;
    --el-button-hover-text-color: #409EFF;
  }
}
</style>
