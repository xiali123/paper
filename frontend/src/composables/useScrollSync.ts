import { ref, watch, onUnmounted, readonly, type Ref, type ComputedRef } from 'vue'

export interface ScrollSyncOptions {
  /** 编辑器元素 */
  editorElement: Ref<HTMLElement | undefined> | ComputedRef<HTMLElement | undefined>
  /** 预览元素 */
  previewElement: Ref<HTMLElement | undefined>
  /** 是否启用同步 */
  enabled?: Ref<boolean> | boolean
  /** 同步方向 */
  direction?: 'bi-directional' | 'editor-to-preview' | 'preview-to-editor'
  /** 容差比例 */
  tolerance?: number
}

/**
 * 编辑器与预览滚动同步
 */
export function useScrollSync(options: ScrollSyncOptions) {
  const {
    editorElement,
    previewElement,
    enabled = ref(true),
    direction = 'bi-directional',
    tolerance = 0.05
  } = options

  // 是否正在同步（防止循环触发）
  const isSyncing = ref(false)
  const isEnabled = ref(typeof enabled === 'boolean' ? enabled : enabled.value)

  // 监听 enabled 变化
  if (typeof enabled !== 'boolean') {
    watch(enabled, (value) => {
      isEnabled.value = value
    })
  }

  /**
   * 计算滚动百分比
   */
  function getScrollPercentage(element: HTMLElement): number {
    const scrollTop = element.scrollTop
    const scrollHeight = element.scrollHeight - element.clientHeight
    return scrollHeight > 0 ? scrollTop / scrollHeight : 0
  }

  /**
   * 同步编辑器滚动到预览
   */
  function syncEditorToPreview() {
    if (!isEnabled.value || isSyncing.value) return
    if (!editorElement.value || !previewElement.value) return

    try {
      isSyncing.value = true

      const editorScrollPct = getScrollPercentage(editorElement.value)
      const previewScrollHeight = previewElement.value.scrollHeight - previewElement.value.clientHeight

      previewElement.value.scrollTop = editorScrollPct * previewScrollHeight
    } finally {
      setTimeout(() => {
        isSyncing.value = false
      }, 50)
    }
  }

  /**
   * 同步预览滚动到编辑器
   */
  function syncPreviewToEditor() {
    if (!isEnabled.value || isSyncing.value) return
    if (!editorElement.value || !previewElement.value) return

    try {
      isSyncing.value = true

      const previewScrollPct = getScrollPercentage(previewElement.value)
      const editorScrollHeight = editorElement.value.scrollHeight - editorElement.value.clientHeight

      editorElement.value.scrollTop = previewScrollPct * editorScrollHeight
    } finally {
      setTimeout(() => {
        isSyncing.value = false
      }, 50)
    }
  }

  /**
   * 设置滚动同步监听
   */
  function setupListeners() {
    const editor = editorElement.value
    const preview = previewElement.value

    if (!editor || !preview) return

    // 编辑器滚动事件
    if (direction === 'bi-directional' || direction === 'editor-to-preview') {
      editor.addEventListener('scroll', syncEditorToPreview, { passive: true })
    }

    // 预览滚动事件
    if (direction === 'bi-directional' || direction === 'preview-to-editor') {
      preview.addEventListener('scroll', syncPreviewToEditor, { passive: true })
    }
  }

  /**
   * 移除滚动同步监听
   */
  function removeListeners() {
    const editor = editorElement.value
    const preview = previewElement.value

    if (!editor || !preview) return

    editor.removeEventListener('scroll', syncEditorToPreview)
    preview.removeEventListener('scroll', syncPreviewToEditor)
  }

  // 监听元素变化
  watch([editorElement, previewElement], () => {
    removeListeners()
    setupListeners()
  })

  // 清理
  onUnmounted(() => {
    removeListeners()
  })

  return {
    isSyncing: readonly(isSyncing),
    isEnabled: readonly(isEnabled),
    syncEditorToPreview,
    syncPreviewToEditor,
    enable: () => { isEnabled.value = true },
    disable: () => { isEnabled.value = false }
  }
}
