<template>
  <div class="pdf-viewer">
    <div class="viewer-header">
      <h4 class="viewer-title">PDF 预览</h4>
      <div class="viewer-controls">
        <el-button-group size="small">
          <el-button :disabled="currentPage <= 1" @click="previousPage">
            <el-icon><ArrowLeft /></el-icon>
          </el-button>
          <el-button disabled>
            {{ currentPage }} / {{ totalPages || '-' }}
          </el-button>
          <el-button :disabled="currentPage >= totalPages" @click="nextPage">
            <el-icon><ArrowRight /></el-icon>
          </el-button>
        </el-button-group>
        <el-button-group size="small" style="margin-left: 8px">
          <el-button :disabled="scale <= 0.5" @click="zoomOut">
            <el-icon><ZoomOut /></el-icon>
          </el-button>
          <el-button disabled>{{ Math.round(scale * 100) }}%</el-button>
          <el-button :disabled="scale >= 3" @click="zoomIn">
            <el-icon><ZoomIn /></el-icon>
          </el-button>
        </el-button-group>
        <el-button size="small" @click="downloadPdf" style="margin-left: 8px">
          <el-icon><Download /></el-icon>
          下载
        </el-button>
      </div>
    </div>

    <div class="viewer-content" ref="contentRef">
      <div v-if="loading" class="viewer-loading">
        <el-icon class="is-loading" :size="40"><Loading /></el-icon>
        <p>加载PDF中...</p>
      </div>

      <div v-else-if="error" class="viewer-error">
        <el-alert type="error" :closable="false">
          <template #title>
            {{ error }}
          </template>
        </el-alert>
      </div>

      <div v-else-if="!pdfUrl" class="viewer-empty">
        <el-empty description="编译LaTeX以生成PDF预览" :image-size="60" />
      </div>

      <div v-else class="pdf-container" :style="{ transform: `scale(${scale})` }">
        <canvas ref="canvasRef" class="pdf-canvas"></canvas>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted, onUnmounted, nextTick } from 'vue'
import { ElMessage } from 'element-plus'
import {
  ArrowLeft,
  ArrowRight,
  ZoomIn,
  ZoomOut,
  Download,
  Loading
} from '@element-plus/icons-vue'

// 动态导入 PDF.js
let pdfjsLib: any = null

interface Props {
  pdfUrl?: string
  initialScale?: number
}

const props = withDefaults(defineProps<Props>(), {
  initialScale: 1.0
})

const contentRef = ref<HTMLElement>()
const canvasRef = ref<HTMLCanvasElement>()
const loading = ref(false)
const error = ref<string | null>(null)
const currentPage = ref(1)
const totalPages = ref(0)
const scale = ref(props.initialScale)

let pdfDocument: any = null
let pageRendering = false

async function loadPdfJs() {
  if (pdfjsLib) return pdfjsLib

  try {
    const pdfJsModule = await import('pdfjs-dist')
    pdfjsLib = pdfJsModule.default || pdfJsModule

    // 设置 worker
    const pdfJsWorker = await import('pdfjs-dist/build/pdf.worker.entry')
    pdfjsLib.GlobalWorkerOptions.workerSrc = pdfJsWorker.default

    if (import.meta.env.DEV) {
      console.log('[PdfViewer] PDF.js loaded successfully')
    }

    return pdfjsLib
  } catch (e) {
    if (import.meta.env.DEV) {
      console.warn('[PdfViewer] PDF.js import failed:', e)
    }
    throw new Error('PDF.js加载失败，请检查网络连接')
  }
}

async function renderPage(pageNum: number) {
  if (!pdfDocument || !canvasRef.value || pageRendering) {
    return
  }

  pageRendering = true

  try {
    const page = await pdfDocument.getPage(pageNum)
    const viewport = page.getViewport({ scale: scale.value })

    const canvas = canvasRef.value
    const context = canvas.getContext('2d')

    if (!context) {
      throw new Error('无法获取Canvas上下文')
    }

    canvas.height = viewport.height
    canvas.width = viewport.width

    const renderContext = {
      canvasContext: context,
      viewport: viewport
    }

    await page.render(renderContext).promise
    pageRendering = false
  } catch (err) {
    pageRendering = false
    error.value = `渲染第${pageNum}页失败: ${err instanceof Error ? err.message : String(err)}`
  }
}

async function loadPdf(url: string) {
  if (!url) {
    pdfDocument = null
    totalPages.value = 0
    currentPage.value = 1
    return
  }

  loading.value = true
  error.value = null

  try {
    const pdfjs = await loadPdfJs()

    // 添加缓存破坏参数（如果没有的话）
    const urlWithCacheBust = url.includes('?t=') ? url : `${url}?t=${Date.now()}`

    if (import.meta.env.DEV) {
      console.log('[PdfViewer] Loading PDF with cache-busting URL:', urlWithCacheBust)
    }

    // 加载PDF文档
    const loadingTask = pdfjs.getDocument(urlWithCacheBust)
    pdfDocument = await loadingTask.promise

    totalPages.value = pdfDocument.numPages
    currentPage.value = 1

    if (import.meta.env.DEV) {
      console.log('[PdfViewer] PDF loaded:', {
        url,
        pages: totalPages.value
      })
    }

    // 渲染第一页
    await nextTick()
    await renderPage(1)

    ElMessage.success(`PDF加载成功，共${totalPages.value}页`)
  } catch (err) {
    error.value = err instanceof Error ? err.message : String(err)
    console.error('[PdfViewer] Failed to load PDF:', err)
    ElMessage.error('PDF加载失败')
  } finally {
    loading.value = false
  }
}

function previousPage() {
  if (currentPage.value > 1) {
    currentPage.value--
    renderPage(currentPage.value)
  }
}

function nextPage() {
  if (currentPage.value < totalPages.value) {
    currentPage.value++
    renderPage(currentPage.value)
  }
}

function zoomIn() {
  if (scale.value < 3) {
    scale.value = Math.min(3, scale.value + 0.25)
    renderPage(currentPage.value)
  }
}

function zoomOut() {
  if (scale.value > 0.5) {
    scale.value = Math.max(0.5, scale.value - 0.25)
    renderPage(currentPage.value)
  }
}

async function downloadPdf() {
  if (!props.pdfUrl) {
    ElMessage.warning('没有可下载的PDF')
    return
  }

  try {
    // 添加缓存破坏参数（如果没有的话）
    const urlWithCacheBust = props.pdfUrl.includes('?t=') ? props.pdfUrl : `${props.pdfUrl}?t=${Date.now()}`

    const response = await fetch(urlWithCacheBust)
    const blob = await response.blob()
    const url = window.URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `latex-export-${Date.now()}.pdf`
    document.body.appendChild(a)
    a.click()
    window.URL.revokeObjectURL(url)
    document.body.removeChild(a)
    ElMessage.success('PDF下载成功')
  } catch (err) {
    console.error('[PdfViewer] Download failed:', err)
    ElMessage.error('PDF下载失败')
  }
}

// 监听 PDF URL 变化
watch(() => props.pdfUrl, (newUrl, oldUrl) => {
  if (newUrl !== oldUrl) {
    loadPdf(newUrl)
  }
}, { immediate: true })

// 暴露方法供父组件调用
defineExpose({
  refresh: () => {
    if (props.pdfUrl) {
      loadPdf(props.pdfUrl)
    }
  }
})

onUnmounted(() => {
  pdfDocument = null
})
</script>

<style scoped lang="scss">
.pdf-viewer {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);
  overflow: hidden;
}

.viewer-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);
  flex-shrink: 0;
}

.viewer-title {
  margin: 0;
  font-size: 14px;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.viewer-controls {
  display: flex;
  align-items: center;
}

.viewer-content {
  flex: 1;
  overflow: auto;
  display: flex;
  justify-content: center;
  padding: 20px;
  background-color: #525659; // PDF阅读器标准背景色
}

.viewer-loading,
.viewer-error,
.viewer-empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100%;
  color: var(--el-text-color-secondary);
  gap: 12px;
}

.pdf-container {
  display: inline-block;
  box-shadow: 0 0 10px rgba(0, 0, 0, 0.5);
  background-color: white;
  transform-origin: top center;
  transition: transform 0.2s ease;
}

.pdf-canvas {
  display: block;
  max-width: 100%;
  height: auto;
}
</style>
