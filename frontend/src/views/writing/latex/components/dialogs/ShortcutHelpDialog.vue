<template>
  <ShortcutHelp ref="dialogRef" />
</template>

<script setup lang="ts">
import ShortcutHelp from '@/components/latex/ShortcutHelp.vue'

const dialogRef = ref<InstanceType<typeof ShortcutHelp> | null>(null)

defineExpose({
  open: () => dialogRef.value?.open()
})
</script>
