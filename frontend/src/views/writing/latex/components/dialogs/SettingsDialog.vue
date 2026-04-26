<template>
  <BaseDialog
    :show="show"
    title="编辑器设置"
    width="800px"
    @update:show="$emit('update:show', $event)"
  >
    <EditorSettings
      :theme="theme"
      :font-size="fontSize"
      :tab-size="tabSize"
      :word-wrap="wordWrap"
      :line-numbers="lineNumbers"
      :auto-save="autoSave"
      :auto-save-interval="autoSaveInterval"
      @update:theme="$emit('update:theme', $event)"
      @update:font-size="$emit('update:fontSize', $event)"
      @update:tab-size="$emit('update:tabSize', $event)"
      @update:word-wrap="$emit('update:wordWrap', $event)"
      @update:line-numbers="$emit('update:lineNumbers', $event)"
      @update:auto-save="$emit('update:autoSave', $event)"
      @update:auto-save-interval="$emit('update:autoSaveInterval', $event)"
    />
    <template #footer>
      <el-button @click="$emit('update:show', false)">关闭</el-button>
    </template>
  </BaseDialog>
</template>

<script setup lang="ts">
import BaseDialog from './BaseDialog.vue'
import EditorSettings from '@/components/latex/EditorSettings.vue'

interface Props {
  show: boolean
  theme?: string
  fontSize?: number
  tabSize?: number
  wordWrap?: boolean
  lineNumbers?: boolean
  autoSave?: boolean
  autoSaveInterval?: number
}

defineProps<Props>()

defineEmits<{
  'update:show': [value: boolean]
  'update:theme': [value: string]
  'update:fontSize': [value: number]
  'update:tabSize': [value: number]
  'update:wordWrap': [value: boolean]
  'update:lineNumbers': [value: boolean]
  'update:autoSave': [value: boolean]
  'update:autoSaveInterval': [value: number]
}>()
</script>
