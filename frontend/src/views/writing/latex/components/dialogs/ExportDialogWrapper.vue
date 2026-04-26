<template>
  <ExportDialog ref="dialogRef" @export="$emit('export', $event)" />
</template>

<script setup lang="ts">
import ExportDialog from '@/components/latex/ExportDialog.vue'

const dialogRef = ref<InstanceType<typeof ExportDialog> | null>(null)

defineEmits<{
  'export': [options: any]
}>()

defineExpose({
  open: (content?: string, format?: string) => {
    if (dialogRef.value?.open) {
      if (content !== undefined) {
        dialogRef.value?.open(content)
      } else {
        dialogRef.value?.open()
      }
    }
    // format is reserved for future use
    void format
  }
})
</script>
