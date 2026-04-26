<template>
  <PanelDrawer
    :show="show"
    title="代码片段"
    direction="rtl"
    size="400px"
    @update:show="$emit('update:show', $event)"
  >
    <LatexSnippets @insert="$emit('insert', $event)" />
  </PanelDrawer>
</template>

<script setup lang="ts">
import PanelDrawer from './PanelDrawer.vue'
import LatexSnippets from '@/components/latex/LatexSnippets.vue'

interface Props {
  show: boolean
}

defineProps<Props>()

defineEmits<{
  'update:show': [value: boolean]
  'insert': [snippet: any]
}>()
</script>
