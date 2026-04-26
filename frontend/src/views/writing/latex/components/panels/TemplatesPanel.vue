<template>
  <PanelDrawer
    :show="show"
    title="模板管理"
    direction="rtl"
    size="600px"
    @update:show="$emit('update:show', $event)"
  >
    <TemplateManager @insert="$emit('insert', $event)" />
  </PanelDrawer>
</template>

<script setup lang="ts">
import PanelDrawer from './PanelDrawer.vue'
import TemplateManager from '@/components/latex/TemplateManager.vue'

interface Props {
  show: boolean
}

defineProps<Props>()

defineEmits<{
  'update:show': [value: boolean]
  'insert': [content: string]
}>()
</script>
