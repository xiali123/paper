<template>
  <PanelDrawer
    :show="show"
    title="拼写检查"
    direction="rtl"
    size="400px"
    @update:show="$emit('update:show', $event)"
  >
    <SpellChecker
      :content="content"
      @replace="$emit('replace', $event)"
      @goto="$emit('goto', $event)"
    />
  </PanelDrawer>
</template>

<script setup lang="ts">
import PanelDrawer from './PanelDrawer.vue'
import SpellChecker from '@/components/latex/SpellChecker.vue'

interface Props {
  show: boolean
  content: string
}

defineProps<Props>()

defineEmits<{
  'update:show': [value: boolean]
  'replace': [data: any]
  'goto': [position: number]
}>()
</script>
