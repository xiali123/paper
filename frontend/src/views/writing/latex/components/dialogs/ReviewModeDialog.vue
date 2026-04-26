<template>
  <ReviewMode @toggle="$emit('toggle', $event)" />
</template>

<script setup lang="ts">
import ReviewMode from '@/components/latex/ReviewMode.vue'

defineEmits<{
  'toggle': [enabled: boolean]
}>()
</script>
