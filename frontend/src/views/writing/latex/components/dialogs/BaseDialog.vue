<template>
  <el-dialog
    :model-value="show"
    :title="title"
    :width="width"
    :close-on-click-modal="closeOnClickModal"
    :close-on-press-escape="closeOnPressEscape"
    :show-close="showClose"
    :destroy-on-close="destroyOnClose"
    @update:model-value="$emit('update:show', $event)"
    @close="$emit('close')"
    @open="$emit('open')"
    @opened="$emit('opened')"
    @closed="$emit('closed')"
  >
    <slot />
    <template v-if="showFooter" #footer>
      <slot name="footer" />
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
interface Props {
  show: boolean
  title: string
  width?: string | number
  closeOnClickModal?: boolean
  closeOnPressEscape?: boolean
  showClose?: boolean
  destroyOnClose?: boolean
  showFooter?: boolean
}

withDefaults(defineProps<Props>(), {
  width: '600px',
  closeOnClickModal: true,
  closeOnPressEscape: true,
  showClose: true,
  destroyOnClose: false,
  showFooter: false
})

defineEmits<{
  'update:show': [value: boolean]
  'close': []
  'open': []
  'opened': []
  'closed': []
}>()
</script>

<style scoped lang="scss">
:deep(.el-dialog) {
  border-radius: 8px;
  overflow: hidden;
}

:deep(.el-dialog__header) {
  margin: 0;
  padding: 16px 20px;
  border-bottom: 1px solid var(--el-border-color-light);
  background: var(--el-fill-color-blank);
}

:deep(.el-dialog__title) {
  font-size: 16px;
  font-weight: 500;
}

:deep(.el-dialog__body) {
  padding: 20px;
  max-height: 70vh;
  overflow-y: auto;
}

:deep(.el-dialog__footer) {
  padding: 16px 20px;
  border-top: 1px solid var(--el-border-color-light);
  background: var(--el-fill-color-blank);
}
</style>
