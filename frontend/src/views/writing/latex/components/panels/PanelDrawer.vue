<template>
  <el-drawer
    :model-value="show"
    :title="title"
    :direction="direction"
    :size="size"
    @update:model-value="$emit('update:show', $event)"
  >
    <slot />
  </el-drawer>
</template>

<script setup lang="ts">
interface Props {
  show: boolean
  title: string
  direction?: 'rtl' | 'ltr' | 'ttb' | 'btt'
  size?: string | number
}

withDefaults(defineProps<Props>(), {
  direction: 'rtl',
  size: '400px'
})

defineEmits<{
  'update:show': [value: boolean]
}>()
</script>

<style scoped lang="scss">
:deep(.el-drawer) {
  .el-drawer__header {
    margin-bottom: 0;
    padding: 16px 20px;
    border-bottom: 1px solid var(--el-border-color-light);
  }

  .el-drawer__title {
    font-size: 16px;
    font-weight: 500;
  }

  .el-drawer__body {
    padding: 0;
    height: 100%;
    overflow: auto;
  }
}
</style>
