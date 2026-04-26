<template>
  <PanelDrawer
    :show="show"
    title="表格生成器"
    direction="rtl"
    size="600px"
    @update:show="$emit('update:show', $event)"
  >
    <TableGenerator @insert="$emit('insert', $event)" />
  </PanelDrawer>
</template>

<script setup lang="ts">
import PanelDrawer from './PanelDrawer.vue'
import TableGenerator from '@/components/latex/TableGenerator.vue'

interface Props {
  show: boolean
}

defineProps<Props>()

defineEmits<{
  'update:show': [value: boolean]
  'insert': [code: string]
}>()
</script>
