<template>
  <div class="latex-editor">
    <!-- 语法高亮预览层 -->
    <pre class="latex-highlight" v-if="showHighlight && !isFocused" aria-hidden="true">
      <code v-html="highlightedCode"></code>
    </pre>

    <!-- 实际编辑器 -->
    <textarea
      ref="textareaRef"
      v-model="innerContent"
      class="latex-textarea"
      spellcheck="false"
      @focus="isFocused = true; showHighlight = false"
      @blur="isFocused = false; showHighlight = true"
      @input="handleInput"
      @scroll="syncScroll"
    ></textarea>

    <!-- LaTeX 快捷工具栏 -->
    <div class="latex-toolbar" v-if="showToolbar">
      <el-tooltip content="行内公式 $...$" placement="top">
        <el-button size="small" @click="insert('$', '$')">$</el-button>
      </el-tooltip>
      <el-tooltip content="块级公式 $$...$$" placement="top">
        <el-button size="small" @click="insert('$$\n', '\n$$')">$$</el-button>
      </el-tooltip>
      <el-divider direction="vertical" />
      <el-tooltip content="粗体 \\textbf{}" placement="top">
        <el-button size="small" @click="insert('\\textbf{', '}')"><b>B</b></el-button>
      </el-tooltip>
      <el-tooltip content="斜体 \\textit{}" placement="top">
        <el-button size="small" @click="insert('\\textit{', '}')"><i>I</i></el-button>
      </el-tooltip>
      <el-divider direction="vertical" />
      <el-tooltip content="章节 \\section{}" placement="top">
        <el-button size="small" @click="insert('\\section{', '}')">§</el-button>
      </el-tooltip>
      <el-dropdown trigger="click" @command="handleCommand">
        <el-button size="small">
          更多
          <el-icon><ArrowDown /></el-icon>
        </el-button>
        <template #dropdown>
          <el-dropdown-item command="itemize">• 无序列表</el-dropdown-item>
          <el-dropdown-item command="enumerate">1. 有序列表</el-dropdown-item>
          <el-dropdown-item command="figure">📷 图片</el-dropdown-item>
          <el-dropdown-item command="table">📊 表格</el-dropdown-item>
          <el-dropdown-item command="cite">📎 引用</el-dropdown-item>
        </template>
      </el-dropdown>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, nextTick } from 'vue'
import { ElMessage } from 'element-plus'
import { ArrowDown } from '@element-plus/icons-vue'
import Prism from 'prismjs'
import 'prismjs/themes/prism-tomorrow.css'
import 'prismjs/components/prism-latex'

interface Props {
  modelValue: string
  readonly?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  readonly: false
})

const emit = defineEmits<{
  'update:modelValue': [value: string]
  'change': [value: string]
}>()

const textareaRef = ref<HTMLTextAreaElement>()
const isFocused = ref(false)
const showHighlight = ref(true)
const showToolbar = ref(true)

const innerContent = computed({
  get: () => props.modelValue,
  set: (val) => emit('update:modelValue', val)
})

// 语法高亮代码
const highlightedCode = computed(() => {
  if (!innerContent.value) return ''
  try {
    return Prism.highlight(innerContent.value, Prism.languages.latex, 'latex')
  } catch {
    return innerContent.value
  }
})

function handleInput() {
  emit('change', innerContent.value)
}

// 同步滚动
function syncScroll() {
  if (!showHighlight.value) return
  // 预览层滚动同步
}

// 插入文本
function insert(before: string, after: string) {
  const textarea = textareaRef.value
  if (!textarea) return

  const start = textarea.selectionStart
  const end = textarea.selectionEnd
  const text = innerContent.value

  const newText = text.substring(0, start) + before + after + text.substring(end)

  emit('update:modelValue', newText)

  nextTick(() => {
    textarea.focus()
    textarea.selectionStart = textarea.selectionEnd = start + before.length
  })
}

// 下拉菜单命令
function handleCommand(cmd: string) {
  const snippets: Record<string, [string, string]> = {
    itemize: ['\\begin{itemize}\n  \\item ', '\n\\end{itemize}'],
    enumerate: ['\\begin{enumerate}\n  \\item ', '\n\\end{enumerate}'],
    figure: ['\\begin{figure}[h]\n  \\centering\n  \\includegraphics[width=0.8\\textwidth]{', '}\n  \\caption{}\n\\end{figure}'],
    table: ['\\begin{table}[h]\n  \\centering\n  \\begin{tabular}{}\n  \\end{tabular}\n  \\caption{}\n\\end{table}'],
    cite: ['\\cite{', '}']
  }

  const snippet = snippets[cmd]
  if (snippet) {
    insert(snippet[0], snippet[1])
  }
}
</script>

<style scoped lang="scss">
.latex-editor {
  height: 100%;
  width: 100%;
  display: flex;
  flex-direction: column;
  position: relative;
}

.latex-textarea {
  flex: 1;
  width: 100%;
  min-height: 0;
  border: none;
  outline: none;
  resize: none;
  padding: 16px;
  font-family: 'Fira Code', 'Consolas', 'Monaco', 'Courier New', monospace;
  font-size: 14px;
  line-height: 1.6;
  background: var(--el-bg-color);
  color: var(--el-text-color-primary);
  white-space: pre;
  overflow-wrap: normal;
  overflow-x: auto;
  tab-size: 2;
}

.latex-highlight {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  margin: 0;
  padding: 16px;
  font-family: 'Fira Code', 'Consolas', 'Monaco', 'Courier New', monospace;
  font-size: 14px;
  line-height: 1.6;
  pointer-events: none;
  white-space: pre-wrap;
  overflow-wrap: normal;
  overflow-x: auto;
  z-index: 1;

  code {
    background: transparent;
    font-family: inherit;
  }

  // Prism 语法高亮样式
  :deep(.token.comment),
  :deep(.token.prolog),
  :deep(.token.doctype),
  :deep(.token.cdata) {
    color: #6a737d;
  }

  :deep(.token.punctuation),
  :deep(.token.namespace) {
    color: #586e75;
  }

  :deep(.token.property),
  :deep(.token.tag),
  :deep(.token.boolean),
  :deep(.token.number),
  :deep(.token.constant),
  :deep(.token.symbol),
  :deep(.token.deleted) {
    color: #e36209;
  }

  :deep(.token.selector),
  :deep(.token.attr-name),
  :deep(.token.string),
  :deep(.token.char),
  :deep(.token.builtin),
  :deep(.token.inserted) {
    color: #795e26;
  }

  :deep(.token.operator),
  :deep(.token.entity),
  :deep(.token.url) {
    color: #56b6c2;
  }

  :deep(.token.atrule),
  :deep(.token.keyword),
  :deep(.token.function) {
    color: #c678dd;
  }
}

.latex-toolbar {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 8px;
  border-top: 1px solid var(--el-border-color-lighter);
  background: var(--el-bg-color-page);

  .el-divider--vertical {
    height: 20px;
  }
}

// 深色模式适配
.dark {
  .latex-textarea {
    background: #1e1e1e;
  }

  .latex-highlight {
    :deep(.token.comment) { color: #6a737d; }
    :deep(.token.function) { color: #61afef; }
    :deep(.token.keyword) { color: #c678dd; }
    :deep(.token.string) { color: #98c379; }
    :deep(.token.number) { color: #d19a66; }
    :deep(.token.operator) { color: #56b6c2; }
  }
}
</style>
