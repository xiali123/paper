<template>
  <QuickInsert ref="dialogRef" @insert="$emit('insert', $event)" />
</template>

<script setup lang="ts">
import QuickInsert from '@/components/latex/QuickInsert.vue'

const dialogRef = ref<InstanceType<typeof QuickInsert> | null>(null)

defineEmits<{
  'insert': [code: string]
}>()

defineExpose({
  open: () => dialogRef.value?.open()
})
</script>
