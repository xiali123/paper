<template>
  <div
    ref="panelRef"
    class="preview-panel"
    :class="{
      'preview-only': layoutMode === 'preview-only',
      'side-by-side': layoutMode === 'side-by-side' && !isMobile,
      'vertical-split': layoutMode === 'vertical-split' && !isMobile
    }"
    v-if="show && !isMobile"
    :style="layoutMode === 'side-by-side' && !isMobile ? { flex: `0 0 ${100 - editorPanelWidth}%` } : {}"
  >
    <div class="preview-header">
      <h3>实时预览</h3>
      <div class="preview-controls">
        <el-button-group size="small">
          <el-button @click="$emit('zoom-out')">
            <el-icon><ZoomOut /></el-icon>
          </el-button>
          <el-button @click="$emit('zoom-in')">
            <el-icon><ZoomIn /></el-icon>
          </el-button>
          <el-button @click="$emit('reset-zoom')">
            {{ Math.round(previewScale * 100) }}%
          </el-button>
        </el-button-group>
      </div>

      <div class="preview-mode-toggle">
        <el-radio-group :model-value="previewMode" @update:model-value="handlePreviewModeChange" size="small">
          <el-radio-button value="html">HTML预览</el-radio-button>
          <el-radio-button value="pdf" :disabled="!pdfUrl">PDF预览</el-radio-button>
        </el-radio-group>
      </div>
    </div>

    <div class="preview-content" ref="scrollElement">
      <LatexPreview
        v-if="previewMode === 'html'"
        :content="content"
        :scale="previewScale"
        :theme="theme"
        ref="latexPreviewRef"
      />
      <PdfViewer
        v-else-if="previewMode === 'pdf' && pdfUrl"
        :pdf-url="pdfUrl"
        :pdf-id="pdfId"
        :document-id="documentId"
        :project-id="projectId"
        ref="pdfViewerRef"
      />
      <div v-else-if="previewMode === 'pdf' && !pdfUrl" class="pdf-placeholder">
        <el-empty description="请先编译文档生成PDF">
          <el-button type="primary" @click="$emit('compile')">立即编译</el-button>
        </el-empty>
      </div>
    </div>

    <!-- Error Panel -->
    <div class="error-panel" v-if="hasErrors || hasWarnings">
      <div class="error-header">
        <el-tabs :model-value="activeErrorTab" @update:model-value="handleErrorTabChange">
          <el-tab-pane name="errors" v-if="hasErrors">
            <template #label>
              <el-badge :value="errors.length" type="danger">
                <el-icon><Warning /></el-icon>
                错误
              </el-badge>
            </template>
          </el-tab-pane>
          <el-tab-pane name="warnings" v-if="hasWarnings">
            <template #label>
              <el-badge :value="warnings.length" type="warning">
                <el-icon><InfoFilled /></el-icon>
                警告
              </el-badge>
            </template>
          </el-tab-pane>
        </el-tabs>
      </div>

      <div class="error-content">
        <div
          v-for="error in activeErrors"
          :key="error.line"
          class="error-item"
          @click="$emit('navigate-to-error', error)"
        >
          <el-icon :class="error.type"><Warning /></el-icon>
          <span class="error-line">第 {{ error.line }} 行:</span>
          <span class="error-message">{{ error.message }}</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { ZoomIn, ZoomOut, Warning, InfoFilled } from '@element-plus/icons-vue'
import LatexPreview from '@/components/latex/LatexPreview.vue'
import PdfViewer from '@/components/latex/PdfViewer.vue'
import type { CompilationError } from '@/architecture/stores/latexEditor'

interface Props {
  show?: boolean
  layoutMode?: 'side-by-side' | 'vertical-split' | 'editor-only' | 'preview-only'
  editorPanelWidth?: number
  isMobile?: boolean
  previewMode?: 'html' | 'pdf'
  previewScale?: number
  content: string
  theme?: 'light' | 'dark'
  pdfUrl?: string | null
  pdfId?: string
  documentId?: string
  projectId?: string
  errors?: CompilationError[]
  warnings?: CompilationError[]
  activeErrorTab?: 'errors' | 'warnings'
}

const props = withDefaults(defineProps<Props>(), {
  show: true,
  layoutMode: 'side-by-side',
  editorPanelWidth: 50,
  isMobile: false,
  previewMode: 'html',
  previewScale: 1,
  theme: 'light',
  pdfUrl: null,
  errors: () => [],
  warnings: () => [],
  activeErrorTab: 'errors'
})

const emit = defineEmits<{
  'update:previewMode': [mode: 'html' | 'pdf']
  'zoom-in': []
  'zoom-out': []
  'reset-zoom': []
  'compile': []
  'update:activeErrorTab': [tab: 'errors' | 'warnings']
  'navigate-to-error': [error: CompilationError]
}>()

// Type-safe event handlers
function handlePreviewModeChange(mode: string | number | boolean | undefined) {
  if (mode === 'html' || mode === 'pdf') {
    emit('update:previewMode', mode)
  }
}

function handleErrorTabChange(tab: string | number | boolean | undefined) {
  if (tab === 'errors' || tab === 'warnings') {
    emit('update:activeErrorTab', tab)
  }
}

const panelRef = ref<HTMLElement | null>(null)
const scrollElement = ref<HTMLElement | null>(null)
const latexPreviewRef = ref<InstanceType<typeof LatexPreview> | null>(null)
const pdfViewerRef = ref<InstanceType<typeof PdfViewer> | null>(null)

const hasErrors = computed(() => props.errors.length > 0)
const hasWarnings = computed(() => props.warnings.length > 0)

const activeErrors = computed(() => {
  return props.activeErrorTab === 'errors' ? props.errors : props.warnings
})

defineExpose({
  panelRef,
  scrollElement,
  latexPreviewRef,
  pdfViewerRef
})
</script>

<style scoped lang="scss">
.preview-panel {
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);
  border-left: 1px solid var(--el-border-color);
  overflow: hidden;

  &.preview-only {
    border-left: none;
  }

  &.side-by-side {
    min-width: 0;
  }

  &.vertical-split {
    border-top: 1px solid var(--el-border-color);
  }
}

.preview-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color);

  h3 {
    margin: 0;
    font-size: 16px;
    font-weight: 500;
  }
}

.preview-controls {
  display: flex;
  align-items: center;
  gap: 12px;
}

.preview-mode-toggle {
  display: flex;
  align-items: center;
}

.preview-content {
  flex: 1;
  overflow-y: auto;
  padding: 16px;
  position: relative;
}

.pdf-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 400px;
}

.error-panel {
  border-top: 1px solid var(--el-border-color);
  background: var(--el-fill-color-blank);
}

.error-header {
  padding: 8px 16px;
  border-bottom: 1px solid var(--el-border-color);
}

.error-content {
  max-height: 200px;
  overflow-y: auto;
  padding: 8px 0;
}

.error-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  cursor: pointer;
  transition: background 0.2s;

  &:hover {
    background: var(--el-fill-color-light);
  }

  .error-line {
    color: var(--el-text-color-secondary);
    font-size: 12px;
  }

  .error-message {
    flex: 1;
    font-size: 13px;
  }
}

.el-icon.error {
  color: var(--el-color-danger);
}

.el-icon.warning {
  color: var(--el-color-warning);
}
</style>
