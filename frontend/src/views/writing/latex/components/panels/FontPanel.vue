<template>
  <PanelDrawer
    :show="show"
    title="字体选择"
    direction="rtl"
    size="350px"
    @update:show="$emit('update:show', $event)"
  >
    <FontSelector @select="$emit('select', $event)" />
  </PanelDrawer>
</template>

<script setup lang="ts">
import PanelDrawer from './PanelDrawer.vue'
import FontSelector from '@/components/latex/FontSelector.vue'

interface Props {
  show: boolean
}

defineProps<Props>()

defineEmits<{
  'update:show': [value: boolean]
  'select': [font: string]
}>()
</script>
