<template>
  <el-button size="small" @click="$emit('toggle-left-panel')" :class="{ 'is-active': showOutline || showProjectTree }">
    <el-icon><Menu /></el-icon>
    {{ leftPanelTitle }}
  </el-button>
</template>

<script setup lang="ts">
import { Menu } from '@element-plus/icons-vue'

interface Props {
  showOutline?: boolean
  showProjectTree?: boolean
  leftPanelTitle?: string
}

defineProps<Props>()

defineEmits<{
  'toggle-left-panel': []
}>()
</script>

<style scoped lang="scss">
.el-button.is-active {
  color: var(--el-color-primary);
}
</style>
