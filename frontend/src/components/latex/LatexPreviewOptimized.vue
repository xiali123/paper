<template>
  <div class="latex-preview" ref="previewContainerRef">
    <div v-if="loading && !renderedHtml" class="preview-loading">
      <el-icon class="is-loading" :size="32"><Loading /></el-icon>
      <p>渲染预览中...</p>
      <el-progress
        v-if="renderProgress > 0"
        :percentage="renderProgress"
        :indeterminate="false"
        :stroke-width="2"
      />
    </div>

    <div
      v-else-if="error"
      class="preview-error"
    >
      <el-alert
        type="error"
        :title="error"
        :closable="false"
        show-icon
      />
    </div>

    <div
      v-else-if="!renderedHtml"
      class="preview-empty"
    >
      <el-empty
        :description="isEmpty ? '文档为空' : '开始编写LaTeX以查看预览'"
        :image-size="80"
      />
    </div>

    <div
      v-else
      ref="previewContentRef"
      class="preview-content"
      :class="{ 'preview-scrolled': isScrolled }"
      @scroll="handleScroll"
      tabindex="0"
      role="region"
      aria-label="LaTeX预览"
      aria-live="polite"
    >
      <div
        v-html="renderedHtml"
        class="preview-rendered"
        :style="{ minHeight: contentHeight + 'px' }"
      />

      <!-- 加载更多指示器 -->
      <div
        v-if="hasMoreContent"
        class="load-more-indicator"
        @click="loadMoreContent"
      >
        <el-icon><Loading /></el-icon>
        <span>加载更多内容...</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted, onUnmounted, computed, nextTick } from 'vue'
import { Loading } from '@element-plus/icons-vue'
import DOMPurify from 'dompurify'
import { getLatexRenderer } from '@/utils/latexRenderer'
import { performanceMonitor } from '@/utils/performance'

interface Props {
  content: string
  autoRender?: boolean
  debounceTime?: number
}

const props = withDefaults(defineProps<Props>(), {
  autoRender: true,
  debounceTime: 300
})

const emit = defineEmits<{
  rendered: [html: string, time: number]
  error: [error: string]
}>()

// Refs
const previewContainerRef = ref<HTMLElement>()
const previewContentRef = ref<HTMLElement>()

// State
const renderedHtml = ref('')
const loading = ref(false)
const error = ref<string | null>(null)
const renderProgress = ref(0)
const isScrolled = ref(false)
const hasMoreContent = ref(false)
const contentHeight = ref(0)

// 渲染器
const renderer = getLatexRenderer()

// 计算文档是否为空
const isEmpty = computed(() => !props.content || props.content.trim().length === 0)

// 防抖定时器
let renderTimer: number | null = null
let abortController: AbortController | null = null

/**
 * 渲染LaTeX内容
 */
async function renderLatex() {
  // 取消之前的渲染
  if (abortController) {
    abortController.abort()
  }
  abortController = new AbortController()

  const contentToRender = props.content || ''

  if (!contentToRender.trim()) {
    renderedHtml.value = ''
    error.value = null
    renderProgress.value = 0
    return
  }

  loading.value = true
  error.value = null
  renderProgress.value = 10

  const startTime = performance.now()

  try {
    // 检查取消信号
    if (abortController.signal.aborted) {
      throw new Error('Render aborted')
    }

    // 使用渲染器进行增量渲染
    let html = ''
    const isLargeDocument = contentToRender.length > 50000 // 50KB

    if (isLargeDocument) {
      // 大文档：增量渲染
      renderProgress.value = 30

      // 先渲染前半部分
      const halfPoint = Math.floor(contentToRender.length / 2)
      const firstHalf = contentToRender.substring(0, halfPoint)
      html = await renderer.renderFull(firstHalf)

      renderProgress.value = 60

      await nextTick()

      // 检查取消信号
      if (abortController.signal.aborted) {
        throw new Error('Render aborted')
      }

      // 更新显示
      updateDisplayedHtml(html)
      renderProgress.value = 80

      // 异步渲染剩余部分
      setTimeout(async () => {
        try {
          const secondHalf = contentToRender.substring(halfPoint)
          const secondHtml = await renderer.renderFull(secondHalf)
          updateDisplayedHtml(html + secondHtml)
          renderProgress.value = 100

          if (previewContentRef.value) {
            contentHeight.value = previewContentRef.value.scrollHeight
          }
        } catch (err) {
          if ((err as Error).name !== 'AbortError') {
            console.warn('Second half render failed:', err)
          }
        }
      }, 100)
    } else {
      // 小文档：直接渲染
      html = await renderer.renderFull(contentToRender)
      renderProgress.value = 90
    }

    // 清理HTML
    html = sanitizeHtml(html)

    // 更新显示
    updateDisplayedHtml(html)
    renderProgress.value = 100

    // 性能监控
    const endTime = performance.now()
    const renderTime = endTime - startTime

    performanceMonitor.recordLatexRender({
      contentLength: contentToRender.length,
      renderTime,
      chunkSize: isLargeDocument ? contentToRender.length / 2 : contentToRender.length
    })

    emit('rendered', html, renderTime)

    // 计算内容高度
    await nextTick()
    if (previewContentRef.value) {
      contentHeight.value = previewContentRef.value.scrollHeight
      hasMoreContent.value = contentHeight.value > 10000 // 超过10000px可能有更多
    }
  } catch (err: any) {
    if (err.name === 'AbortError') {
      if (import.meta.env.DEV) {
        console.log('[LatexPreviewOptimized] Render aborted')
      }
      return
    }

    const errorMsg = err instanceof Error ? err.message : String(err)
    error.value = errorMsg
    emit('error', errorMsg)
    console.warn('[LatexPreviewOptimized] Render failed:', err)
  } finally {
    loading.value = false
    abortController = null
  }
}

/**
 * 清理HTML（使用DOMPurify）
 */
function sanitizeHtml(html: string): string {
  return DOMPurify.sanitize(html, {
    ALLOWED_TAGS: [
      'p', 'br', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6',
      'strong', 'em', 'u', 'sub', 'sup', 'code', 'pre',
      'blockquote', 'ul', 'ol', 'li', 'table', 'tbody',
      'thead', 'tr', 'td', 'th', 'figure', 'figcaption',
      'img', 'span', 'a', 'div', 'center'
    ],
    ALLOWED_ATTR: [
      'class', 'id', 'style', 'href', 'title', 'src',
      'alt', 'loading', 'target', 'width', 'height'
    ],
    ALLOW_DATA_ATTR: false,
    FORBID_TAGS: ['script', 'object', 'embed', 'iframe'],
    FORBID_ATTR: ['onerror', 'onload', 'onclick', 'onmouseover']
  })
}

/**
 * 更新显示的HTML
 */
function updateDisplayedHtml(html: string) {
  renderedHtml.value = html
}

/**
 * 加载更多内容（占位）
 */
function loadMoreContent() {
  // 对于大文档，可以实现虚拟滚动加载
  ElMessage.info('内容已全部加载')
}

/**
 * 处理滚动事件
 */
function handleScroll(event: Event) {
  const target = event.target as HTMLElement
  isScrolled.value = target.scrollTop > 100
}

/**
 * 防抖渲染
 */
function scheduleRender() {
  if (renderTimer) {
    clearTimeout(renderTimer)
  }

  renderTimer = window.setTimeout(() => {
    renderLatex()
    renderTimer = null
  }, props.debounceTime)
}

/**
 * 强制刷新
 */
function refresh() {
  renderer.clearCache()
  renderLatex()
}

/**
 * 监听内容变化
 */
watch(() => props.content, () => {
  if (props.autoRender) {
    scheduleRender()
  }
}, { debounce: props.debounceTime })

// 生命周期
onMounted(() => {
  if (props.autoRender && props.content) {
    renderLatex()
  }
})

onUnmounted(() => {
  if (renderTimer) {
    clearTimeout(renderTimer)
  }
  if (abortController) {
    abortController.abort()
  }
})

// 暴露方法
defineExpose({
  refresh,
  renderLatex
})
</script>

<style scoped lang="scss">
.latex-preview {
  height: 100%;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.preview-loading,
.preview-error,
.preview-empty {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 16px;
  padding: 20px;
  color: var(--el-text-color-secondary);

  .el-icon {
    font-size: 32px;
    color: var(--el-color-primary);
  }

  p {
    margin: 0;
  }
}

.preview-content {
  flex: 1;
  overflow-y: auto;
  overflow-x: hidden;
  padding: 20px;

  &:focus {
    outline: none;
  }
}

.preview-rendered {
  max-width: 800px;
  margin: 0 auto;
  line-height: 1.8;
  color: var(--el-text-color-primary);

  // 基础样式
  h1, h2, h3, h4, h5, h6 {
    margin-top: 1.5em;
    margin-bottom: 0.5em;
    font-weight: 600;
    line-height: 1.3;
  }

  h1 { font-size: 2em; }
  h2 { font-size: 1.5em; }
  h3 { font-size: 1.25em; }
  h4 { font-size: 1.1em; }

  p {
    margin: 0.5em 0;
  }

  strong, b {
    font-weight: 600;
  }

  em, i {
    font-style: italic;
  }

  code {
    background: var(--el-fill-color-light);
    padding: 2px 6px;
    border-radius: 3px;
    font-family: 'Consolas', 'Monaco', monospace;
    font-size: 0.9em;
  }

  pre {
    background: var(--el-fill-color-light);
    padding: 12px;
    border-radius: 6px;
    overflow-x: auto;
    margin: 1em 0;

    code {
      background: none;
      padding: 0;
    }
  }

  blockquote {
    border-left: 4px solid var(--el-border-color);
    padding-left: 1em;
    margin: 1em 0;
    color: var(--el-text-color-secondary);
  }

  ul, ol {
    padding-left: 2em;
    margin: 0.5em 0;
  }

  li {
    margin: 0.25em 0;
  }

  table {
    border-collapse: collapse;
    width: 100%;
    margin: 1em 0;

    th, td {
      border: 1px solid var(--el-border-color);
      padding: 8px 12px;
      text-align: left;
    }

    th {
      background: var(--el-fill-color-light);
      font-weight: 600;
    }
  }

  img {
    max-width: 100%;
    height: auto;
    display: block;
    margin: 1em auto;
  }

  .image-error {
    color: var(--el-color-danger);
    background: var(--el-color-danger-light-9);
    padding: 4px 8px;
    border-radius: 4px;
    font-size: 0.9em;
    display: inline-block;
  }
}

.load-more-indicator {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 16px;
  color: var(--el-text-color-secondary);
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    color: var(--el-color-primary);
  }
}

.preview-scrolled {
  // 可以添加滚动时的特殊样式
}
</style>
