<template>
  <div class="latex-editor-wrapper">
    <textarea
      ref="textareaRef"
      v-model="localContent"
      class="latex-editor-textarea"
      :class="editorClass"
      :placeholder="placeholder"
      :disabled="disabled"
      :readonly="readonly"
      @input="handleInput"
      @keydown="handleKeydown"
      @click="handleClick"
      @scroll="handleScroll"
      spellcheck="false"
    />

    <!-- 自动补全菜单 -->
    <Teleport to="body">
      <div
        v-if="showAutocomplete"
        ref="autocompleteMenuRef"
        class="latex-autocomplete-menu"
        :style="menuStyle"
        @click.stop
      >
        <div
          v-for="(item, index) in filteredCompletions"
          :key="item.name"
          class="autocomplete-item"
          :class="{ active: index === activeIndex }"
          @click="selectCompletion(item)"
          @mouseenter="activeIndex = index"
        >
          <div class="item-name">{{ item.name }}</div>
          <div class="item-detail">{{ item.detail }}</div>
        </div>

        <div
          v-if="filteredCompletions.length === 0"
          class="autocomplete-empty"
        >
          无匹配项
        </div>
      </div>
    </Teleport>

    <!-- 快捷键提示 -->
    <div
      v-if="showShortcutHint && shortcutHint"
      class="shortcut-hint"
    >
      {{ shortcutHint }}
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted, nextTick } from 'vue'
import { LATEX_EDITOR_CONFIG } from '@/config/latexEditor'

interface CompletionItem {
  name: string
  insert: string
  detail: string
  type: 'structure' | 'formatting' | 'environment' | 'math' | 'symbol'
}

interface Props {
  modelValue: string
  placeholder?: string
  disabled?: boolean
  readonly?: boolean
  editorClass?: string
  enableAutocomplete?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  placeholder: '输入 LaTeX 代码...',
  enableAutocomplete: true,
})

const emit = defineEmits<{
  'update:modelValue': [value: string]
  'cursor-position': [position: { line: number; column: number }]
  'select': [value: string]
}>()

// Refs
const textareaRef = ref<HTMLTextAreaElement>()
const autocompleteMenuRef = ref<HTMLElement>()

// State
const localContent = ref(props.modelValue)
const showAutocomplete = ref(false)
const activeIndex = ref(0)
const menuPosition = ref({ top: 0, left: 0 })
const cursorPosition = ref({ line: 1, column: 1 })
const currentPrefix = ref('')
const showShortcutHint = ref(false)
const shortcutHint = ref('')

// LaTeX 命令自动补全数据库
const COMPLETIONS: CompletionItem[] = [
  // 文档结构
  { name: '\\section', insert: '\\section{}', detail: '章节', type: 'structure' },
  { name: '\\subsection', insert: '\\subsection{}', detail: '小节', type: 'structure' },
  { name: '\\subsubsection', insert: '\\subsubsection{}', detail: '子小节', type: 'structure' },
  { name: '\\paragraph', insert: '\\paragraph{}', detail: '段落', type: 'structure' },
  { name: '\\subparagraph', insert: '\\subparagraph{}', detail: '子段落', type: 'structure' },

  // 文本格式
  { name: '\\textbf', insert: '\\textbf{}', detail: '粗体', type: 'formatting' },
  { name: '\\textit', insert: '\\textit{}', detail: '斜体', type: 'formatting' },
  { name: '\\texttt', insert: '\\texttt{}', detail: '等宽字体', type: 'formatting' },
  { name: '\\underline', insert: '\\underline{}', detail: '下划线', type: 'formatting' },
  { name: '\\emph', insert: '\\emph{}', detail: '强调', type: 'formatting' },
  { name: '\\textsc', insert: '\\textsc{}', detail: '小型大写字母', type: 'formatting' },

  // 环境
  { name: 'document', insert: '\\begin{document}\n  \\end{document}', detail: '文档环境', type: 'environment' },
  { name: 'itemize', insert: '\\begin{itemize}\n  \\item \n\\end{itemize}', detail: '无序列表', type: 'environment' },
  { name: 'enumerate', insert: '\\begin{enumerate}\n  \\item \n\\end{enumerate}', detail: '有序列表', type: 'environment' },
  { name: 'description', insert: '\\begin{description}\n  \\item[] \n\\end{description}', detail: '描述列表', type: 'environment' },
  { name: 'figure', insert: '\\begin{figure}\n  \\includegraphics{}\n  \\caption{}\n\\end{figure}', detail: '图片环境', type: 'environment' },
  { name: 'table', insert: '\\begin{table}\n  \\begin{tabular}{}\n  \\end{tabular}\n  \\caption{}\n\\end{table}', detail: '表格环境', type: 'environment' },
  { name: 'equation', insert: '\\begin{equation}\n  \n\\end{equation}', detail: '公式环境', type: 'environment' },
  { name: 'align', insert: '\\begin{align}\n  \n\\end{align}', detail: '对齐公式', type: 'environment' },
  { name: 'gather', insert: '\\begin{gather}\n  \n\\end{gather}', detail: '聚集公式', type: 'environment' },

  // 数学
  { name: '\\frac', insert: '\\frac{}{}', detail: '分数', type: 'math' },
  { name: '\\sqrt', insert: '\\sqrt{}', detail: '平方根', type: 'math' },
  { name: '\\sum', insert: '\\sum_{}^{}', detail: '求和', type: 'math' },
  { name: '\\prod', insert: '\\prod_{}^{}', detail: '乘积', type: 'math' },
  { name: '\\int', insert: '\\int_{}^{}', detail: '积分', type: 'math' },
  { name: '\\lim', insert: '\\lim_{}', detail: '极限', type: 'math' },
  { name: '\\alpha', insert: '\\alpha', detail: 'α', type: 'symbol' },
  { name: '\\beta', insert: '\\beta', detail: 'β', type: 'symbol' },
  { name: '\\gamma', insert: '\\gamma', detail: 'γ', type: 'symbol' },
  { name: '\\delta', insert: '\\delta', detail: 'δ', type: 'symbol' },
  { name: '\\epsilon', insert: '\\epsilon', detail: 'ε', type: 'symbol' },
  { name: '\\pi', insert: '\\pi', detail: 'π', type: 'symbol' },
  { name: '\\infty', insert: '\\infty', detail: '∞', type: 'symbol' },
]

// 过滤补全项
const filteredCompletions = computed(() => {
  if (!currentPrefix.value) {
    return COMPLETIONS
  }

  const prefix = currentPrefix.value.toLowerCase()
  return COMPLETIONS.filter(item =>
    item.name.toLowerCase().startsWith(prefix) ||
    item.detail.toLowerCase().includes(prefix)
  )
})

// 菜单样式
const menuStyle = computed(() => ({
  top: `${menuPosition.value.top + 20}px`,
  left: `${menuPosition.value.left}px`,
}))

/**
 * 处理输入事件
 */
function handleInput(event: Event) {
  const target = event.target as HTMLTextAreaElement
  localContent.value = target.value
  emit('update:modelValue', target.value)

  // 更新光标位置
  updateCursorPosition(target)

  // 检查是否应该显示自动补全
  if (props.enableAutocomplete) {
    checkAutocomplete(target)
  }
}

/**
 * 更新光标位置
 */
function updateCursorPosition(textarea: HTMLTextAreaElement) {
  const text = textarea.value.substring(0, textarea.selectionStart)
  const lines = text.split('\n')
  cursorPosition.value = {
    line: lines.length,
    column: lines[lines.length - 1].length + 1
  }
  emit('cursor-position', cursorPosition.value)
}

/**
 * 检查是否显示自动补全
 */
function checkAutocomplete(textarea: HTMLTextAreaElement) {
  const text = textarea.value.substring(0, textarea.selectionStart)

  // 检查当前单词是否以 \ 开头
  const match = text.match(/\\([a-zA-Z]*)$/)

  if (match) {
    currentPrefix.value = '\\' + match[1]
    showAutocomplete.value = true
    activeIndex.value = 0

    // 计算菜单位置
    updateMenuPosition(textarea)
  } else {
    showAutocomplete.value = false
    currentPrefix.value = ''
  }
}

/**
 * 更新菜单位置
 */
function updateMenuPosition(textarea: HTMLTextAreaElement) {
  const rect = textarea.getBoundingClientRect()
  const lineHeight = parseInt(getComputedStyle(textarea).lineHeight)

  // 计算光标的像素位置
  const textBeforeCursor = textarea.value.substring(0, textarea.selectionStart)
  const lines = textBeforeCursor.split('\n')
  const currentLine = lines[lines.length - 1]

  // 创建临时元素测量文本宽度
  const mirror = document.createElement('div')
  const styles = getComputedStyle(textarea)
  mirror.style.cssText = `
    position: absolute;
    visibility: hidden;
    white-space: pre;
    font-family: ${styles.fontFamily};
    font-size: ${styles.fontSize};
    font-weight: ${styles.fontWeight};
    letter-spacing: ${styles.letterSpacing};
    padding: ${styles.padding};
  `
  mirror.textContent = currentLine
  document.body.appendChild(mirror)

  const cursorX = mirror.offsetWidth
  document.body.removeChild(mirror)

  menuPosition.value = {
    top: rect.top + (lines.length - 1) * lineHeight - textarea.scrollTop,
    left: rect.left + cursorX - textarea.scrollLeft
  }
}

/**
 * 处理键盘事件
 */
function handleKeydown(event: KeyboardEvent) {
  if (!showAutocomplete.value) {
    // 显示快捷键提示
    showKeyHint(event)
    return
  }

  switch (event.key) {
    case 'ArrowDown':
      event.preventDefault()
      activeIndex.value = (activeIndex.value + 1) % filteredCompletions.value.length
      break

    case 'ArrowUp':
      event.preventDefault()
      activeIndex.value = activeIndex.value === 0
        ? filteredCompletions.value.length - 1
        : activeIndex.value - 1
      break

    case 'Enter':
    case 'Tab':
      event.preventDefault()
      selectCompletion(filteredCompletions.value[activeIndex.value])
      break

    case 'Escape':
      event.preventDefault()
      showAutocomplete.value = false
      break
  }
}

/**
 * 选择补全项
 */
function selectCompletion(item: CompletionItem) {
  if (!textareaRef.value) return

  const textarea = textareaRef.value
  const text = textarea.value
  const cursorPos = textarea.selectionStart

  // 找到命令的开始位置
  const beforeCursor = text.substring(0, cursorPos)
  const match = beforeCursor.match(/\\([a-zA-Z]*)$/)

  if (match) {
    const start = cursorPos - match[0].length
    const end = cursorPos

    // 替换命令
    const newText = text.substring(0, start) + item.insert + text.substring(end)
    localContent.value = newText
    emit('update:modelValue', newText)

    // 设置光标位置（在插入点）
    const insertPos = start + item.insert.indexOf('}')
    if (insertPos > 0) {
      nextTick(() => {
        textarea.focus()
        textarea.setSelectionRange(insertPos, insertPos)
      })
    }
  }

  showAutocomplete.value = false
  currentPrefix.value = ''
}

/**
 * 显示快捷键提示
 */
function showKeyHint(event: KeyboardEvent) {
  const hints: Record<string, string> = {
    's': `${LATEX_EDITOR_CONFIG.SHORTCUTS.SAVE} 保存`,
    'Enter': `${LATEX_EDITOR_CONFIG.SHORTCUTS.COMPILE} 编译`,
    '/': `${LATEX_EDITOR_CONFIG.SHORTCUTS.TOGGLE_COMMENT} 切换注释`,
    ' ': `${LATEX_EDITOR_CONFIG.SHORTCUTS.AUTOCOMPLETE} 自动补全`,
  }

  const key = event.key.toLowerCase()
  if (event.ctrlKey && hints[key]) {
    shortcutHint.value = hints[key]
    showShortcutHint.value = true

    setTimeout(() => {
      showShortcutHint.value = false
    }, 2000)
  }
}

/**
 * 处理点击事件
 */
function handleClick(event: MouseEvent) {
  // 点击外部关闭自动补全
  setTimeout(() => {
    if (autocompleteMenuRef.value && !autocompleteMenuRef.value.contains(event.target as Node)) {
      showAutocomplete.value = false
    }
  }, 100)
}

/**
 * 处理滚动事件
 */
function handleScroll() {
  // 滚动时更新菜单位置
  if (showAutocomplete.value && textareaRef.value) {
    updateMenuPosition(textareaRef.value)
  }
}

/**
 * 监听外部值变化
 */
watch(() => props.modelValue, (newValue) => {
  if (newValue !== localContent.value) {
    localContent.value = newValue
  }
})

/**
 * 点击外部关闭菜单
 */
function handleClickOutside(event: MouseEvent) {
  if (showAutocomplete.value &&
      autocompleteMenuRef.value &&
      !autocompleteMenuRef.value.contains(event.target as Node) &&
      textareaRef.value &&
      !textareaRef.value.contains(event.target as Node)) {
    showAutocomplete.value = false
  }
}

// 生命周期
onMounted(() => {
  document.addEventListener('click', handleClickOutside)
})

onUnmounted(() => {
  document.removeEventListener('click', handleClickOutside)
})

// 暴露方法
defineExpose({
  focus: () => textareaRef.value?.focus(),
  getText: () => localContent.value,
  setText: (value: string) => {
    localContent.value = value
    emit('update:modelValue', value)
  },
  insertText: (text: string) => {
    if (!textareaRef.value) return
    const textarea = textareaRef.value
    const start = textarea.selectionStart
    const end = textarea.selectionEnd
    const newValue = localContent.value.substring(0, start) + text + localContent.value.substring(end)
    localContent.value = newValue
    emit('update:modelValue', newValue)

    nextTick(() => {
      textarea.focus()
      const newPos = start + text.length
      textarea.setSelectionRange(newPos, newPos)
    })
  },
})
</script>

<style scoped lang="scss">
.latex-editor-wrapper {
  position: relative;
  height: 100%;
  display: flex;
  flex-direction: column;
}

.latex-editor-textarea {
  flex: 1;
  width: 100%;
  min-height: 200px;
  padding: 16px;
  border: 1px solid var(--el-border-color);
  border-radius: 6px;
  background: var(--el-bg-color);
  color: var(--el-text-color-primary);
  font-family: LATEX_EDITOR_CONFIG.EDITOR.DEFAULT_FONT_FAMILY;
  font-size: LATEX_EDITOR_CONFIG.EDITOR.DEFAULT_FONT_SIZE px;
  line-height: 1.6;
  resize: none;
  outline: none;
  transition: border-color 0.2s;

  &:focus {
    border-color: var(--el-color-primary);
  }

  &:disabled {
    background: var(--el-fill-color-light);
    color: var(--el-text-color-disabled);
    cursor: not-allowed;
  }

  &::placeholder {
    color: var(--el-text-color-placeholder);
  }
}

.latex-autocomplete-menu {
  position: fixed;
  min-width: 200px;
  max-width: 400px;
  max-height: 300px;
  overflow-y: auto;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color);
  border-radius: 6px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
  z-index: 9999;
  padding: 4px 0;
}

.autocomplete-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 8px 12px;
  cursor: pointer;
  transition: background 0.2s;

  &:hover,
  &.active {
    background: var(--el-fill-color-light);
  }
}

.item-name {
  font-family: 'Consolas', 'Monaco', monospace;
  font-weight: 500;
  color: var(--el-color-primary);
}

.item-detail {
  flex: 1;
  font-size: 12px;
  color: var(--el-text-color-secondary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.autocomplete-empty {
  padding: 12px;
  text-align: center;
  color: var(--el-text-color-secondary);
  font-size: 13px;
}

.shortcut-hint {
  position: absolute;
  bottom: 40px;
  right: 20px;
  padding: 8px 16px;
  background: var(--el-color-primary);
  color: white;
  border-radius: 4px;
  font-size: 13px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
  animation: slideIn 0.2s ease;
  z-index: 1000;
}

@keyframes slideIn {
  from {
    transform: translateY(10px);
    opacity: 0;
  }
  to {
    transform: translateY(0);
    opacity: 1;
  }
}
</style>
