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
      @scroll="handleScroll"
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
import { ref, nextTick, computed, watch, onUnmounted, onMounted } from 'vue'
import { ArrowDown } from '@element-plus/icons-vue'
import { performanceMonitor, checkPerformanceThreshold, PERFORMANCE_THRESHOLDS } from '@/utils/performance'
import { highlightSyntax } from '@/utils/workers'
import { debounce } from '@/utils/performance'
import { useTextVirtualScroll } from '@/composables/useVirtualScroll'

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
  'cursor-change': [position: { line: number; column: number }]
  'scroll': [event: Event]
}>()

const textareaRef = ref<HTMLTextAreaElement>()
const isFocused = ref(false)
const showHighlight = ref(true)
const showToolbar = ref(true)

// Setup virtual scrolling for large documents
const containerHeight = ref(400)
const lineHeight = 20

const innerContent = computed({
  get: () => props.modelValue,
  set: (val) => emit('update:modelValue', val)
})

// 语法高亮代码 - now async with Web Worker
const highlightedCode = ref('')
const isHighlighting = ref(false)

async function updateHighlightedCode() {
  if (!innerContent.value) {
    highlightedCode.value = ''
    return
  }

  isHighlighting.value = true

  try {
    const result = await highlightSyntax(innerContent.value)
    highlightedCode.value = result.html

    // Check performance threshold
    checkPerformanceThreshold(
      'syntax_highlighting',
      result.processingTime,
      PERFORMANCE_THRESHOLDS.renderTime
    )
  } catch (error) {
    console.warn('Syntax highlighting failed:', error)
    highlightedCode.value = innerContent.value
  } finally {
    isHighlighting.value = false
  }
}

// Debounced highlighting to avoid excessive processing
const debouncedHighlight = debounce(updateHighlightedCode, 300)
let highlightTimeout: number | null = null

// Watch for content changes - optimized to prevent memory leaks
watch(() => innerContent.value, (newContent, oldContent) => {
  // Skip if content hasn't actually changed (prevents unnecessary processing)
  if (newContent === oldContent) return

  // Adjust debounce time based on content size
  const newLength = newContent?.length ?? 0
  const debounceTime = newLength > 5000 ? 500 : 300

  if (highlightTimeout) {
    clearTimeout(highlightTimeout)
  }

  highlightTimeout = setTimeout(() => {
    updateHighlightedCode()
    highlightTimeout = null
  }, debounceTime)
}, { immediate: true })

function handleInput() {
  const endTimer = performanceMonitor.startTimer('input_handling', {
    contentLength: innerContent.value.length
  })

  if (import.meta.env.DEV) {
    console.log('LatexEditor input:', innerContent.value.length)
  }

  emit('change', innerContent.value)
  updateCursorPosition()
  endTimer()
}

function updateCursorPosition() {
  const textarea = textareaRef.value
  if (!textarea) return

  const cursorPos = textarea.selectionStart
  const textBeforeCursor = textarea.value.substring(0, cursorPos)
  const lines = textBeforeCursor.split('\n')
  const line = lines.length
  const column = lines[lines.length - 1].length + 1

  emit('cursor-change', { line, column })
}

// Virtual scroll setup
const virtualScroll = useTextVirtualScroll(
  innerContent,
  containerHeight,
  lineHeight
)

// 同步滚动
function handleScroll(event: Event) {
  // Sync highlight layer scroll with textarea scroll
  const textarea = event.target as HTMLTextAreaElement
  const highlightLayer = textarea.previousElementSibling as HTMLElement
  if (highlightLayer && highlightLayer.classList.contains('latex-highlight')) {
    highlightLayer.scrollTop = textarea.scrollTop
    highlightLayer.scrollLeft = textarea.scrollLeft
  }

  // Emit scroll event for parent to handle sync
  emit('scroll', event)

  if (import.meta.env.DEV) {
    console.log('[LatexEditor] Scroll position:', textarea.scrollTop)
  }
}

// Focus the editor
function focus() {
  const textarea = textareaRef.value
  if (textarea) {
    textarea.focus()
  }
}

// Update container height when component mounts and connect virtual scroll
onMounted(() => {
  if (textareaRef.value) {
    const rect = textareaRef.value.getBoundingClientRect()
    containerHeight.value = rect.height
    // Connect the textarea to the virtual scroll container
    virtualScroll.containerRef.value = textareaRef.value
  }
})

// Navigate to specific line and column - enhanced with virtual scrolling
function navigateTo(position: { line: number; column?: number }) {
  const textarea = textareaRef.value
  if (!textarea) return

  const { line, column = 1 } = position
  const text = textarea.value
  const lines = text.split('\n')

  // Calculate the character position for the specified line and column
  let charPosition = 0
  for (let i = 0; i < line - 1 && i < lines.length; i++) {
    charPosition += lines[i].length + 1 // +1 for the newline character
  }

  // Add the column offset
  charPosition += Math.min(column - 1, lines[line - 1]?.length || 0)

  // Ensure position is within bounds
  charPosition = Math.min(charPosition, text.length)

  // First, ensure the highlight layer is hidden by setting focus state
  isFocused.value = true
  showHighlight.value = false

  // Set cursor position
  textarea.focus()
  textarea.selectionStart = charPosition
  textarea.selectionEnd = charPosition

  // Scroll to make the cursor visible within the textarea
  // Calculate line height and position
  const lineHeight = parseFloat(getComputedStyle(textarea).lineHeight) || 22.4
  const textareaHeight = textarea.clientHeight
  const currentScrollTop = textarea.scrollTop

  // Calculate target scroll position to center the line
  const targetLineTop = (line - 1) * lineHeight
  const targetScrollTop = targetLineTop - (textareaHeight / 2) + (lineHeight / 2)

  // Smooth scroll to the target position
  textarea.scrollTo({
    top: Math.max(0, targetScrollTop),
    behavior: 'smooth'
  })

  if (import.meta.env.DEV) {
    console.log('Navigated to line', line, 'column', column, 'char position', charPosition)
  }
}

// 插入文本
function insert(before: string, after: string) {
  const endTimer = performanceMonitor.startTimer('text_insertion', {
    beforeLength: before.length,
    afterLength: after.length
  })

  const textarea = textareaRef.value
  if (!textarea) {
    endTimer()
    return
  }

  const start = textarea.selectionStart
  const end = textarea.selectionEnd
  const text = innerContent.value

  const newText = text.substring(0, start) + before + after + text.substring(end)

  emit('update:modelValue', newText)

  nextTick(() => {
    textarea.focus()
    textarea.selectionStart = textarea.selectionEnd = start + before.length
    endTimer()
  })
}

// 下拉菜单命令
function handleCommand(cmd: string) {
  const snippets: Record<string, [string, string]> = {
    itemize: ['\\begin{itemize}\n  \\item ', '\n\\end{itemize}'],
    enumerate: ['\\begin{enumerate}\n  \\item ', '\n\\end{enumerate}'],
    figure: ['\\begin{figure}[h]\n  \\centering\n  \\includegraphics[width=0.8\\textwidth]{', '}\n  \\caption{}\n\\end{figure}'],
    table: ['\\begin{table}[h]\n  \\centering\n  \\begin{tabular}{', '}\n  \\end{tabular}\n  \\caption{}\n\\end{table}'],
    cite: ['\\cite{', '}']
  }

  const snippet = snippets[cmd]
  if (snippet) {
    insert(snippet[0], snippet[1])
  }
}

// 清理定时器防止内存泄漏
onUnmounted(() => {
  if (highlightTimeout) {
    clearTimeout(highlightTimeout)
  }
})

// Expose methods to parent component
defineExpose({
  focus,
  navigateTo
})
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
  white-space: pre;
  overflow: auto;
  overflow-y: hidden;
  z-index: 1;
  opacity: 0.8;

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
