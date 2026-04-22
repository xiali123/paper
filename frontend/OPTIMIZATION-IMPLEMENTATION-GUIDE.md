# LaTeX Editor Optimization Implementation Guide

## 🎯 Immediate Implementation Steps (Week 1-2)

### Step 1: Bundle Optimization - Code Splitting

**Problem**: Current bundle size 3.58MB, target <1.5MB

**Solution**: Implement proper code splitting in Vite configuration

```javascript
// vite.config.js - OPTIMIZED VERSION
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import AutoImport from 'unplugin-auto-import/vite'
import Components from 'unplugin-vue-components/vite'

export default defineConfig({
  plugins: [
    vue(),
    AutoImport({
      imports: [
        'vue',
        'vue-router',
        'pinia',
      ],
    }),
    Components({
      dts: true,
    }),
  ],
  
  build: {
    target: 'es2015',
    
    // 🔥 CRITICAL: Manual chunk splitting
    rollupOptions: {
      output: {
        manualChunks: {
          // Core framework (loaded on every page)
          'vendor-core': ['vue', 'vue-router', 'pinia'],
          
          // UI components (large library)
          'vendor-element': ['element-plus'],
          
          // LaTeX rendering libraries
          'vendor-latex': ['katex', 'prismjs'],
          
          // Editor dependencies
          'vendor-editor': ['monaco-editor', '@monaco-editor/react'],
          
          // i18n and utilities
          'vendor-utils': ['vue-i18n', 'lodash-es', 'axios'],
          
          // Charts and analytics (loaded on demand)
          'vendor-charts': ['chart.js', 'vue-chartjs']
        },
        
        // Optimize chunk sizes
        chunkSizeWarningLimit: 1000, // 1MB warning threshold
      },
    },
    
    // Additional optimizations
    minify: 'terser',
    terserOptions: {
      compress: {
        drop_console: true,
        drop_debugger: true
      }
    },
    
    // CSS optimization
    cssCodeSplit: true,
    sourcemap: false
  },
  
  // Development optimizations
  server: {
    preTransformRequests: true
  }
})
```

**Expected Results**:
- Bundle size reduction: 3.58MB → ~2.1MB (40% reduction)
- Load time improvement: 1.21s → ~800ms
- Better caching: Framework chunks rarely change

### Step 2: Virtual Scrolling Implementation

**Problem**: Large documents (1000+ lines) cause performance issues

**Solution**: Replace textarea with virtualized line editor

```vue
<!-- components/latex/VirtualLatexEditor.vue -->
<template>
  <div class="virtual-latex-editor" ref="containerRef">
    <!-- Line numbers -->
    <div class="line-numbers">
      <div 
        v-for="lineNum in visibleLineNumbers" 
        :key="lineNum"
        class="line-number"
        :class="{ active: lineNum === cursorLine }"
      >
        {{ lineNum }}
      </div>
    </div>
    
    <!-- Virtual content area -->
    <div class="editor-content">
      <div class="content-wrapper" :style="{ height: totalHeight + 'px' }">
        <div 
          class="visible-lines"
          :style="{ transform: `translateY(${offsetY}px)` }"
        >
          <div 
            v-for="(line, index) in visibleLines" 
            :key="line.startIndex"
            class="editor-line"
            :data-line="line.lineNumber"
            @click="handleLineClick(line.lineNumber, $event)"
          >
            <span 
              v-html="line.highlightedContent"
              class="highlighted-line"
            ></span>
            <textarea
              v-if="isEditingLine(line.lineNumber)"
              v-model="line.content"
              @blur="finishEditing(line.lineNumber)"
              @keydown="handleLineKeydown($event, line.lineNumber)"
              class="line-editor"
              rows="1"
              spellcheck="false"
            />
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue'
import { useVirtualScroll } from '@/composables/useVirtualScroll'
import { useSyntaxHighlighting } from '@/composables/useSyntaxHighlighting'

interface Props {
  modelValue: string
  readonly?: boolean
}

const props = defineProps<Props>()
const emit = defineEmits(['update:modelValue', 'cursor-change'])

const containerRef = ref<HTMLElement>()
const lineHeight = 20 // pixels
const containerHeight = ref(600)

// Split content into lines
const documentLines = computed(() => {
  return props.modelValue.split('\n').map((content, index) => ({
    lineNumber: index + 1,
    content,
    startIndex: props.modelValue.split('\n').slice(0, index).join('\n').length + index
  }))
})

// Virtual scrolling logic
const {
  visibleStart,
  visibleEnd,
  offsetY,
  totalHeight,
  updateScroll
} = useVirtualScroll({
  itemCount: () => documentLines.value.length,
  itemHeight: lineHeight,
  containerHeight: () => containerHeight.value
})

// Visible lines with highlighting
const visibleLines = computed(() => {
  const start = visibleStart.value
  const end = Math.min(visibleEnd.value, documentLines.value.length)
  
  return documentLines.value
    .slice(start, end)
    .map(line => ({
      ...line,
      highlightedContent: highlightLine(line.content),
      isVisible: true
    }))
})

// Syntax highlighting (optimized)
const { highlightLine } = useSyntaxHighlighting()

// Cursor and editing state
const cursorLine = ref(1)
const cursorColumn = ref(1)
const editingLine = ref<number | null>(null)

// Line numbers for display
const visibleLineNumbers = computed(() => {
  const numbers = []
  for (let i = visibleStart.value + 1; i <= visibleEnd.value; i++) {
    numbers.push(i)
  }
  return numbers
})

// Handle scroll events
const handleScroll = (event: Event) => {
  const target = event.target as HTMLElement
  updateScroll(target.scrollTop)
}

// Handle line editing
const isEditingLine = (lineNumber: number) => editingLine.value === lineNumber

const handleLineClick = (lineNumber: number, event: MouseEvent) => {
  if (props.readonly) return
  
  const rect = (event.target as HTMLElement).getBoundingClientRect()
  const clickX = event.clientX - rect.left
  const column = Math.floor(clickX / 8) // Approximate character width
  
  cursorLine.value = lineNumber
  cursorColumn.value = Math.min(column + 1, documentLines.value[lineNumber - 1].content.length + 1)
  
  editingLine.value = lineNumber
  
  emit('cursor-change', { line: cursorLine.value, column: cursorColumn.value })
}

const finishEditing = (lineNumber: number) => {
  editingLine.value = null
  
  // Update the full document
  const updatedLines = [...documentLines.value]
  updatedLines[lineNumber - 1] = {
    ...updatedLines[lineNumber - 1],
    content: updatedLines[lineNumber - 1].content // This gets updated by v-model
  }
  
  const newContent = updatedLines.map(line => line.content).join('\n')
  emit('update:modelValue', newContent)
}

const handleLineKeydown = (event: KeyboardEvent, lineNumber: number) => {
  if (event.key === 'Enter') {
    event.preventDefault()
    // Handle line breaks
    const currentLine = documentLines.value[lineNumber - 1]
    const cursorPos = cursorColumn.value - 1
    const beforeCursor = currentLine.content.substring(0, cursorPos)
    const afterCursor = currentLine.content.substring(cursorPos)
    
    // Insert new line
    const updatedLines = [...documentLines.value]
    updatedLines[lineNumber - 1] = { ...currentLine, content: beforeCursor }
    updatedLines.splice(lineNumber, 0, {
      lineNumber: lineNumber + 1,
      content: afterCursor,
      startIndex: currentLine.startIndex + cursorPos + 1
    })
    
    // Update line numbers for subsequent lines
    for (let i = lineNumber + 1; i < updatedLines.length; i++) {
      updatedLines[i] = { ...updatedLines[i], lineNumber: i + 1 }
    }
    
    const newContent = updatedLines.map(line => line.content).join('\n')
    emit('update:modelValue', newContent)
    
    // Move cursor to new line
    cursorLine.value = lineNumber + 1
    cursorColumn.value = 1
    editingLine.value = lineNumber + 1
  } else if (event.key === 'ArrowUp' || event.key === 'ArrowDown') {
    event.preventDefault()
    
    if (event.key === 'ArrowUp' && cursorLine.value > 1) {
      cursorLine.value--
    } else if (event.key === 'ArrowDown' && cursorLine.value < documentLines.value.length) {
      cursorLine.value++
    }
    
    editingLine.value = cursorLine.value
    emit('cursor-change', { line: cursorLine.value, column: cursorColumn.value })
  }
}

// Lifecycle
onMounted(() => {
  if (containerRef.value) {
    containerHeight.value = containerRef.value.clientHeight
    containerRef.value.addEventListener('scroll', handleScroll)
  }
})

onUnmounted(() => {
  if (containerRef.value) {
    containerRef.value.removeEventListener('scroll', handleScroll)
  }
})

// Watch for content changes
watch(() => props.modelValue, () => {
  // Force recalculation of visible lines
  updateScroll(containerRef.value?.scrollTop || 0)
})
</script>

<style scoped>
.virtual-latex-editor {
  display: flex;
  height: 100%;
  font-family: 'Fira Code', 'Consolas', monospace;
  font-size: 14px;
  line-height: 20px;
  background: var(--el-bg-color);
  overflow: hidden;
}

.line-numbers {
  width: 50px;
  background: var(--el-bg-color-overlay);
  border-right: 1px solid var(--el-border-color-lighter);
  user-select: none;
  flex-shrink: 0;
}

.line-number {
  height: 20px;
  padding: 0 8px;
  text-align: right;
  color: var(--el-text-color-secondary);
  font-size: 12px;
}

.line-number.active {
  color: var(--el-color-primary);
  font-weight: bold;
}

.editor-content {
  flex: 1;
  overflow: auto;
  position: relative;
}

.content-wrapper {
  position: relative;
}

.visible-lines {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
}

.editor-line {
  height: 20px;
  padding: 0 8px;
  white-space: pre;
  position: relative;
}

.highlighted-line {
  display: block;
  height: 100%;
}

.line-editor {
  position: absolute;
  top: 0;
  left: 8px;
  right: 8px;
  height: 100%;
  border: none;
  outline: none;
  background: transparent;
  font: inherit;
  color: inherit;
  resize: none;
  overflow: hidden;
}

/* Syntax highlighting styles remain the same as original */
</style>
```

### Step 3: Web Workers for Heavy Operations

**Create Web Worker for Syntax Highlighting**

```javascript
// public/workers/prism.worker.js
importScripts('https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/prism.min.js');
importScripts('https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/components/prism-latex.min.js');

class PrismWorker {
  constructor() {
    this.cache = new Map();
    this.maxCacheSize = 100;
  }

  async highlight(content, language = 'latex') {
    // Check cache first
    const cacheKey = `${language}:${content.length}:${content.slice(0, 50)}`;
    if (this.cache.has(cacheKey)) {
      return this.cache.get(cacheKey);
    }

    try {
      const result = Prism.highlight(content, Prism.languages[language], language);
      
      // Add to cache with LRU eviction
      if (this.cache.size >= this.maxCacheSize) {
        const firstKey = this.cache.keys().next().value;
        this.cache.delete(firstKey);
      }
      this.cache.set(cacheKey, result);
      
      return result;
    } catch (error) {
      console.warn('Prism highlighting failed:', error);
      return content; // Return original content on error
    }
  }

  clearCache() {
    this.cache.clear();
  }
}

const prismWorker = new PrismWorker();

self.onmessage = async function(e) {
  const { type, payload, id } = e.data;

  switch (type) {
    case 'highlight':
      const { content, language } = payload;
      const result = await prismWorker.highlight(content, language);
      self.postMessage({ type: 'highlighted', result, id });
      break;

    case 'clearCache':
      prismWorker.clearCache();
      self.postMessage({ type: 'cacheCleared', id });
      break;

    default:
      console.warn('Unknown message type:', type);
  }
};
```

**Create KaTeX Web Worker**

```javascript
// public/workers/katex.worker.js
importScripts('https://cdn.jsdelivr.net/npm/katex@0.16.45/dist/katex.min.js');

class KatexWorker {
  constructor() {
    this.cache = new Map();
  }

  renderMath(content) {
    // Extract math expressions
    const mathExpressions = [];
    const mathRegex = /\$([^$\n]+?)\$|\$\$([^$]+?)\$\$/g;
    let match;
    
    while ((match = mathRegex.exec(content)) !== null) {
      const math = match[1] || match[2];
      const isDisplay = !!match[2];
      mathExpressions.push({ math, isDisplay, index: match.index });
    }
    
    // Render each math expression
    const results = [];
    for (const expr of mathExpressions) {
      try {
        const rendered = katex.renderToString(expr.math, {
          displayMode: expr.isDisplay,
          throwOnError: false,
          strict: false
        });
        
        results.push({
          original: expr.isDisplay ? `$$${expr.math}$$` : `$${expr.math}$`,
          rendered,
          index: expr.index,
          length: expr.isDisplay ? expr.math.length + 4 : expr.math.length + 2
        });
      } catch (error) {
        console.warn('KaTeX render error:', error);
        results.push({
          original: expr.isDisplay ? `$$${expr.math}$$` : `$${expr.math}$`,
          rendered: `<span class="katex-error">${expr.original}</span>`,
          index: expr.index,
          length: expr.isDisplay ? expr.math.length + 4 : expr.math.length + 2
        });
      }
    }
    
    return { mathResults: results, hasMath: mathExpressions.length > 0 };
  }
}

const katexWorker = new KatexWorker();

self.onmessage = function(e) {
  const { type, payload, id } = e.data;
  
  if (type === 'renderMath') {
    const result = katexWorker.renderMath(payload.content);
    self.postMessage({ type: 'mathRendered', result, id });
  }
};
```

### Step 4: Memory Leak Fixes

**Fixed LatexPreview Component**

```vue
<!-- components/latex/LatexPreviewFixed.vue -->
<template>
  <div class="latex-preview">
    <div class="preview-content" ref="contentRef">
      <div v-if="renderedContent" v-html="renderedContent" class="preview-rendered"></div>
      <div v-else class="preview-empty">
        <span>No content to preview</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted, onUnmounted, shallowRef } from 'vue'

interface Props {
  content: string
  theme?: 'light' | 'dark'
}

const props = withDefaults(defineProps<Props>(), {
  theme: 'light'
})

const contentRef = ref<HTMLElement>()
const renderedContent = shallowRef('')
const renderTimeout = ref<number | null>(null)
const stopWatchers: (() => void)[] = []

// Web worker instances
let prismWorker: Worker | null = null
let katexWorker: Worker | null = null

// Initialize web workers
const initializeWorkers = () => {
  try {
    prismWorker = new Worker('/workers/prism.worker.js')
    katexWorker = new Worker('/workers/katex.worker.js')
    
    // Set up message handlers
    if (prismWorker) {
      prismWorker.onmessage = (e) => {
        if (e.data.type === 'highlighted') {
          handleHighlightedContent(e.data.result)
        }
      }
    }
    
    if (katexWorker) {
      katexWorker.onmessage = (e) => {
        if (e.data.type === 'mathRendered') {
          handleMathRendered(e.data.result)
        }
      }
    }
  } catch (error) {
    console.warn('Failed to initialize Web Workers, falling back to main thread:', error)
  }
}

// Render content using web workers or main thread
const renderContent = async (content: string) => {
  if (!content?.trim()) {
    renderedContent.value = ''
    return
  }

  try {
    // Use web workers if available, otherwise fallback to main thread
    if (prismWorker && katexWorker) {
      // Send to web workers for processing
      const workerId = Date.now().toString()
      
      prismWorker.postMessage({
        type: 'highlight',
        payload: { content, language: 'latex' },
        id: workerId
      })
      
      // Also process math
      katexWorker.postMessage({
        type: 'renderMath',
        payload: { content },
        id: workerId
      })
    } else {
      // Fallback to main thread processing (optimized)
      await processContentMainThread(content)
    }
  } catch (error) {
    console.error('Content rendering failed:', error)
    renderedContent.value = content // Fallback to raw content
  }
}

// Main thread fallback with optimizations
const processContentMainThread = async (content: string) => {
  // Use requestIdleCallback or setTimeout to avoid blocking
  return new Promise<void>((resolve) => {
    if ('requestIdleCallback' in window) {
      requestIdleCallback(() => {
        const processed = processLatexContent(content)
        renderedContent.value = processed
        resolve()
      }, { timeout: 1000 })
    } else {
      setTimeout(() => {
        const processed = processLatexContent(content)
        renderedContent.value = processed
        resolve()
      }, 16) // 16ms = 60fps
    }
  })
}

// Optimized main thread processing
const processLatexContent = (content: string): string => {
  let html = content
  
  // Process math expressions first (most expensive)
  html = html.replace(/\$([^$\n]+?)\$/g, (_, math) => {
    try {
      return `<span class="math-inline">${escapeHtml(math)}</span>`
    } catch {
      return `<span class="math-error">$${escapeHtml(math)}$</span>`
    }
  })
  
  html = html.replace(/\$\$([^$]+?)\$\$/g, (_, math) => {
    try {
      return `<div class="math-display">${escapeHtml(math)}</div>`
    } catch {
      return `<div class="math-error">$$${escapeHtml(math)}$$</div>`
    }
  })
  
  // Process LaTeX commands (simplified)
  html = html.replace(/\\section\*?\{([^}]+)\}/g, '<h2>$1</h2>')
  html = html.replace(/\\textbf\{([^}]+)\}/g, '<strong>$1</strong>')
  html = html.replace(/\\textit\{([^}]+)\}/g, '<em>$1</em>')
  
  return html
}

// Web worker message handlers
const handleHighlightedContent = (highlightedContent: string) => {
  renderedContent.value = highlightedContent
}

const handleMathRendered = (mathResult: any) => {
  // Update content with rendered math
  if (mathResult.hasMath) {
    let content = renderedContent.value
    mathResult.mathResults.forEach((result: any) => {
      content = content.replace(result.original, result.rendered)
    })
    renderedContent.value = content
  }
}

// Optimized watcher with proper cleanup
const stopContentWatcher = watch(
  () => props.content,
  (newContent) => {
    // Clear previous timeout
    if (renderTimeout.value) {
      clearTimeout(renderTimeout.value)
    }
    
    // Debounced rendering
    renderTimeout.value = setTimeout(() => {
      renderContent(newContent)
    }, 300) // 300ms debounce
  },
  { immediate: true }
)

stopWatchers.push(stopContentWatcher)

// Utility function
const escapeHtml = (text: string): string => {
  const div = document.createElement('div')
  div.textContent = text
  return div.innerHTML
}

// Cleanup function
const cleanup = () => {
  // Clear timeout
  if (renderTimeout.value) {
    clearTimeout(renderTimeout.value)
    renderTimeout.value = null
  }
  
  // Stop all watchers
  stopWatchers.forEach(stop => stop())
  stopWatchers.length = 0
  
  // Terminate web workers
  if (prismWorker) {
    prismWorker.terminate()
    prismWorker = null
  }
  
  if (katexWorker) {
    katexWorker.terminate()
    katexWorker = null
  }
  
  // Clear refs
  contentRef.value = null
  renderedContent.value = ''
}

// Lifecycle
onMounted(() => {
  initializeWorkers()
})

onUnmounted(() => {
  cleanup()
})

// Expose cleanup method
defineExpose({ cleanup })
</script>

<style scoped>
.latex-preview {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);
}

.preview-content {
  flex: 1;
  overflow: auto;
  padding: 16px;
}

.preview-empty {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 100%;
  color: var(--el-text-color-secondary);
}

.preview-rendered {
  font-family: 'Latin Modern Math', 'Times New Roman', serif;
  font-size: 14px;
  line-height: 1.8;
  color: var(--el-text-color-primary);
