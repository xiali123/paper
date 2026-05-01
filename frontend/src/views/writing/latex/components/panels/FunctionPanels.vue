<template>
  <!-- BibTeX文献管理器 -->
  <el-drawer
    v-model="bibtexVisible"
    title="BibTeX文献管理"
    direction="rtl"
    size="500px"
  >
    <BibTeXManager @insert-citation="$emit('insert-citation', $event)" />
  </el-drawer>

  <!-- LaTeX宏管理器 -->
  <el-drawer
    v-model="macroVisible"
    title="LaTeX宏管理器"
    direction="rtl"
    size="450px"
  >
    <MacroManager @insert-macro="$emit('insert-macro', $event)" />
  </el-drawer>

  <!-- 图片资源管理器 -->
  <el-drawer
    v-model="imageVisible"
    title="图片资源管理"
    direction="rtl"
    size="600px"
  >
    <ImageResourceManager @insert-image="$emit('insert-image', $event)" />
  </el-drawer>

  <!-- Git集成面板 -->
  <el-drawer
    v-model="gitVisible"
    title="版本控制 (Git)"
    direction="ltr"
    size="600px"
  >
    <GitIntegrationPanel />
  </el-drawer>

  <!-- 字数统计面板 -->
  <el-drawer
    v-model="wordCountVisible"
    title="字数统计"
    direction="rtl"
    size="380px"
  >
    <WordCountPanel :content="editorContent" />
  </el-drawer>

  <!-- 投稿前检查面板 -->
  <el-drawer
    v-model="checkerVisible"
    title="投稿前检查"
    direction="rtl"
    size="480px"
  >
    <SubmissionChecker :content="editorContent" @fix="$emit('checker-fix', $event)" />
  </el-drawer>

  <!-- 代码折叠导航 -->
  <el-drawer
    v-model="codeFoldVisible"
    title="文档大纲"
    direction="ltr"
    size="320px"
  >
    <CodeFoldNavigator
      :content="editorContent"
      @jump-to-line="$emit('jump-to-line', $event)"
    />
  </el-drawer>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import BibTeXManager from '@/components/latex/BibTeXManager.vue'
import MacroManager from '@/components/latex/MacroManager.vue'
import ImageResourceManager from '@/components/latex/ImageResourceManager.vue'
import GitIntegrationPanel from '@/components/latex/GitIntegrationPanel.vue'
import WordCountPanel from '@/components/latex/WordCountPanel.vue'
import SubmissionChecker from '@/components/latex/SubmissionChecker.vue'
import CodeFoldNavigator from '@/components/latex/CodeFoldNavigator.vue'

interface Props {
  showBibTeXManager: boolean
  showMacroManager: boolean
  showImageResourceManager: boolean
  showGitIntegration: boolean
  showWordCount: boolean
  showSubmissionChecker: boolean
  showCodeFoldNavigator: boolean
  editorContent: string
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'update:showBibTeXManager': [value: boolean]
  'update:showMacroManager': [value: boolean]
  'update:showImageResourceManager': [value: boolean]
  'update:showGitIntegration': [value: boolean]
  'update:showWordCount': [value: boolean]
  'update:showSubmissionChecker': [value: boolean]
  'update:showCodeFoldNavigator': [value: boolean]
  'insert-citation': [citation: string]
  'insert-macro': [macroCode: string]
  'insert-image': [imageCode: string]
  'checker-fix': [fix: { type: string; replacement: string }]
  'jump-to-line': [lineNumber: number]
}>()

// Use computed getters/setters to bridge props to v-model on el-drawer
const bibtexVisible = computed({
  get: () => props.showBibTeXManager,
  set: (val: boolean) => emit('update:showBibTeXManager', val)
})

const macroVisible = computed({
  get: () => props.showMacroManager,
  set: (val: boolean) => emit('update:showMacroManager', val)
})

const imageVisible = computed({
  get: () => props.showImageResourceManager,
  set: (val: boolean) => emit('update:showImageResourceManager', val)
})

const gitVisible = computed({
  get: () => props.showGitIntegration,
  set: (val: boolean) => emit('update:showGitIntegration', val)
})

const wordCountVisible = computed({
  get: () => props.showWordCount,
  set: (val: boolean) => emit('update:showWordCount', val)
})

const checkerVisible = computed({
  get: () => props.showSubmissionChecker,
  set: (val: boolean) => emit('update:showSubmissionChecker', val)
})

const codeFoldVisible = computed({
  get: () => props.showCodeFoldNavigator,
  set: (val: boolean) => emit('update:showCodeFoldNavigator', val)
})
</script>
