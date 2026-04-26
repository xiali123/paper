<template>
  <AiFormulaRecognizer ref="dialogRef" @insert="$emit('insert', $event)" />
</template>

<script setup lang="ts">
import AiFormulaRecognizer from '@/components/latex/AiFormulaRecognizer.vue'

const dialogRef = ref<InstanceType<typeof AiFormulaRecognizer> | null>(null)

defineEmits<{
  'insert': [formula: string]
}>()

defineExpose({
  open: () => dialogRef.value?.open()
})
</script>
