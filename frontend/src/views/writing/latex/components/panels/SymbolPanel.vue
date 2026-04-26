<template>
  <PanelDrawer
    :show="show"
    title="LaTeX符号"
    direction="rtl"
    size="300px"
    @update:show="$emit('update:show', $event)"
  >
    <SymbolPalette @insert="$emit('insert', $event)" />
  </PanelDrawer>
</template>

<script setup lang="ts">
import PanelDrawer from './PanelDrawer.vue'
import SymbolPalette from '@/components/latex/SymbolPalette.vue'

interface Props {
  show: boolean
}

defineProps<Props>()

defineEmits<{
  'update:show': [value: boolean]
  'insert': [command: string]
}>()
</script>
