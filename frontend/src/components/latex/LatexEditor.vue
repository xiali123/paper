<template>
  <div class="latex-editor" ref="containerRef">
    <MonacoEditor
      v-model="innerContent"
      :language="language"
      :theme="theme"
      :options="editorOptions"
      @change="handleChange"
      @ready="handleEditorReady"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, watch, computed } from 'vue'
import MonacoEditor from '@monaco-editor/react'

interface Props {
  modelValue: string
  language?: string
  theme?: 'vs-light' | 'vs-dark'
  readonly?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  language: 'latex',
  theme: 'vs-light',
  readonly: false
})

const emit = defineEmits<{
  'update:modelValue': [value: string]
  'change': [value: string]
  'ready': [editor: any]
}>()

const containerRef = ref<HTMLElement>()
const innerContent = computed({
  get: () => props.modelValue,
  set: (val) => emit('update:modelValue', val)
})

const editorOptions = {
  minimap: { enabled: false },
  fontSize: 14,
  lineNumbers: 'on' as const,
  scrollBeyondLastLine: false,
  wordWrap: 'on' as const,
  automaticLayout: true,
  tabSize: 2,
  readOnly: props.readonly,
  // LaTeX 特定配置
  quickSuggestions: {
    other: true,
    comments: false,
    strings: false
  },
  suggestOnTriggerCharacters: true,
  formatOnPaste: true,
  formatOnType: true
}

function handleChange(value: string | undefined) {
  emit('change', value ?? '')
}

function handleEditorReady(editor: any) {
  emit('ready', editor)

  // 注册 LaTeX 自动补全
  registerLaTeXCompletion(editor)
}

function registerLaTeXCompletion(editor: any) {
  // 常用 LaTeX 命令
  const latexCommands = [
    // 文档结构
    { label: '\\documentclass', insertText: '\\documentclass{${1:article}}' },
    { label: '\\usepackage', insertText: '\\usepackage{${1:package}}' },
    { label: '\\begin{document}', insertText: '\\begin{document}\n\t$0\n\\end{document}' },
    { label: '\\section', insertText: '\\section{${1:title}}' },
    { label: '\\subsection', insertText: '\\subsection{${1:title}}' },
    { label: '\\title', insertText: '\\title{${1:title}}' },
    { label: '\\author', insertText: '\\author{${1:author}}' },
    { label: '\\maketitle', insertText: '\\maketitle' },
    // 数学公式
    { label: '$$ (公式)', insertText: '$$\n${1:公式}\n$$' },
    { label: '\\frac', insertText: '\\frac{${1:numerator}}{${2:denominator}}' },
    { label: '\\sqrt', insertText: '\\sqrt{${1:n}}' },
    { label: '\\sum', insertText: '\\sum_{${1:i=1}}^{${2:n}}' },
    { label: '\\int', insertText: '\\int_{${1:a}}^{${2:b}}' },
    { label: '\\alpha', insertText: '\\alpha' },
    { label: '\\beta', insertText: '\\beta' },
    { label: '\\gamma', insertText: '\\gamma' },
    { label: '\\delta', insertText: '\\delta' },
    { label: '\\theta', insertText: '\\theta' },
    { label: '\\pi', insertText: '\\pi' },
    // 环境和列表
    { label: 'itemize', insertText: '\\begin{itemize}\n\t\\item $0\n\\end{itemize}' },
    { label: 'enumerate', insertText: '\\begin{enumerate}\n\t\\item $0\n\\end{enumerate}' },
    { label: 'figure', insertText: '\\begin{figure}\n\t\\centering\n\t\\includegraphics{$1}\n\t\\caption{$2}\n\\end{figure}' },
    { label: 'table', insertText: '\\begin{table}\n\t\\centering\n\t\\begin{tabular}{$1}\n\t$0\n\t\\end{tabular}\n\t\\caption{$2}\n\\end{table}' },
    // 参考文献
    { label: '\\cite', insertText: '\\cite{${1:key}}' },
    { label: '\\bibliographystyle', insertText: '\\bibliographystyle{${1:plain}}' },
    { label: '\\bibliography', insertText: '\\bibliography{${1:refs}}' },
    // 其他
    { label: '\\includegraphics', insertText: '\\includegraphics[width=${1:0.8}\\textwidth]{${2:file}}' },
    { label: '\\caption', insertText: '\\caption{${1:text}}' },
    { label: '\\label', insertText: '\\label{${1:key}}' },
    { label: '\\ref', insertText: '\\ref{${1:key}}' }
  ]

  // 设置自动补全提供程序
  monaco.languages.registerCompletionItemProvider('latex', {
    provideCompletionItems: () => {
      return {
        suggestions: latexCommands.map(cmd => ({
          label: cmd.label,
          kind: monaco.languages.CompletionItemKind.Function,
          insertText: cmd.insertText,
          insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
          documentation: cmd.label
        }))
      }
    }
  })
}
</script>

<style scoped lang="scss">
.latex-editor {
  height: 100%;
  width: 100%;
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 4px;
  overflow: hidden;

  :deep(.monaco-editor) {
    padding: 8px 0;
  }
}

// 深色模式
.dark {
  .latex-editor {
    border-color: var(--el-border-color);
  }
}
</style>
