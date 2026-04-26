<template>
  <BaseDialog
    :show="show"
    title="文档统计"
    width="700px"
    @update:show="$emit('update:show', $event)"
  >
    <StatsDashboard v-if="stats" :stats="stats" :document-id="documentId" />
    <el-empty v-else description="没有统计数据" />
    <template #footer>
      <el-button @click="$emit('update:show', false)">关闭</el-button>
    </template>
  </BaseDialog>
</template>

<script setup lang="ts">
import BaseDialog from './BaseDialog.vue'
import StatsDashboard from '@/components/latex/StatsDashboard.vue'

interface Stats {
  totalChars: number
  totalWords: number
  totalLines: number
  nonEmptyLines: number
  paragraphs: number
  sentences: number
  totalPages: number
  readingTime: string
  formulas: number
  inlineFormulas: number
  displayFormulas: number
  references: number
  citations: number
  refs: number
  images: number
  figures: number
  tables: number
  tabulars: number
  environments: number
  customEnvs?: number
  packages: number
  macros?: number
  structure?: Record<string, number>
  topCommands?: Array<{ command: string; count: number }>
  totalCommands: number
  readingTimeFast?: string
}

interface Props {
  show: boolean
  stats?: Stats
  documentId?: string
}

withDefaults(defineProps<Props>(), {
  stats: undefined
})

defineEmits<{
  'update:show': [value: boolean]
}>()
</script>
