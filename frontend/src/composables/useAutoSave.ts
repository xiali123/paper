import { ref, computed, watch, onUnmounted } from 'vue'
import { debounce } from 'lodash-es'

export interface AutoSaveOptions {
  /** 自动保存间隔（毫秒），默认30秒 */
  interval?: number
  /** 防抖延迟（毫秒），默认2000ms */
  debounceDelay?: number
  /** 是否启用本地存储备份，默认true */
  enableLocalStorage?: boolean
  /** 保存回调函数 */
  onSave: (content: string) => Promise<void> | void
  /** 保存成功回调 */
  onSuccess?: () => void
  /** 保存失败回调 */
  onError?: (error: Error) => void
}

export interface AutoSaveState {
  isSaving: boolean
  lastSavedAt: Date | null
  lastSavedContent: string | null
  hasUnsavedChanges: boolean
  saveCount: number
}

export function useAutoSave(
  content: Ref<string>,
  documentId: Ref<string | null>,
  options: AutoSaveOptions
) {
  const {
    interval = 30000,
    debounceDelay = 2000,
    enableLocalStorage = true,
    onSave,
    onSuccess,
    onError
  } = options

  // 状态
  const state = ref<AutoSaveState>({
    isSaving: false,
    lastSavedAt: null,
    lastSavedContent: null,
    hasUnsavedChanges: false,
    saveCount: 0
  })

  // 定时器
  let autoSaveTimer: number | null = null
  let debouncedSave: (() => void) | null = null

  // 本地存储键
  const getStorageKey = () => {
    return documentId.value ? `latex_autosave_${documentId.value}` : null
  }

  // 从本地存储加载
  const loadFromLocalStorage = (): string | null => {
    if (!enableLocalStorage || !documentId.value) return null

    const key = getStorageKey()
    if (!key) return null

    try {
      const data = localStorage.getItem(key)
      if (data) {
        const parsed = JSON.parse(data)
        if (parsed.content && parsed.timestamp) {
          // 检查是否过期（24小时）
          const age = Date.now() - parsed.timestamp
          if (age < 24 * 60 * 60 * 1000) {
            return parsed.content
          } else {
            // 过期则删除
            localStorage.removeItem(key)
          }
        }
      }
    } catch (error) {
      console.warn('Failed to load from local storage:', error)
    }

    return null
  }

  // 保存到本地存储
  const saveToLocalStorage = (content: string) => {
    if (!enableLocalStorage || !documentId.value) return

    const key = getStorageKey()
    if (!key) return

    try {
      localStorage.setItem(key, JSON.stringify({
        content,
        timestamp: Date.now()
      }))
    } catch (error) {
      console.warn('Failed to save to local storage:', error)
    }
  }

  // 清除本地存储
  const clearLocalStorage = () => {
    if (!enableLocalStorage || !documentId.value) return

    const key = getStorageKey()
    if (key) {
      localStorage.removeItem(key)
    }
  }

  // 执行保存
  const performSave = async () => {
    if (state.value.isSaving) return

    const currentContent = content.value
    const hasChanges = currentContent !== state.value.lastSavedContent

    if (!hasChanges) return

    state.value.isSaving = true

    try {
      // 保存到本地存储作为备份
      saveToLocalStorage(currentContent)

      // 调用保存回调
      await onSave(currentContent)

      // 更新状态
      state.value.lastSavedAt = new Date()
      state.value.lastSavedContent = currentContent
      state.value.hasUnsavedChanges = false
      state.value.saveCount++

      onSuccess?.()
    } catch (error) {
      console.error('Auto-save failed:', error)
      onError?.(error as Error)
    } finally {
      state.value.isSaving = false
    }
  }

  // 防抖保存
  const debouncedPerformSave = debounce(performSave, debounceDelay)

  // Watcher stop handle
  let contentWatcher: (() => void) | null = null

  // 启动自动保存
  const startAutoSave = () => {
    // 创建防抖保存函数
    debouncedSave = debouncedPerformSave

    // 设置定时保存
    autoSaveTimer = window.setInterval(() => {
      performSave()
    }, interval)

    // 监听内容变化
    contentWatcher = watch(content, (newContent, oldContent) => {
      const hasChanges = newContent !== state.value.lastSavedContent
      state.value.hasUnsavedChanges = hasChanges

      if (hasChanges) {
        debouncedSave?.()
      }
    })
  }

  // 停止自动保存
  const stopAutoSave = () => {
    if (contentWatcher) { contentWatcher(); contentWatcher = null }
    if (autoSaveTimer) {
      clearInterval(autoSaveTimer)
      autoSaveTimer = null
    }

    if (debouncedSave) {
      debouncedSave.cancel()
      debouncedSave = null
    }
  }

  // 手动保存
  const manualSave = async () => {
    await performSave()
  }

  // 恢复自动保存的内容
  const restoreAutoSave = (): string | null => {
    const savedContent = loadFromLocalStorage()
    if (savedContent && savedContent !== content.value) {
      return savedContent
    }
    return null
  }

  // 组件卸载时清理
  onUnmounted(() => {
    stopAutoSave()
  })

  return {
    // 状态
    state,
    isSaving: computed(() => state.value.isSaving),
    lastSavedAt: computed(() => state.value.lastSavedAt),
    hasUnsavedChanges: computed(() => state.value.hasUnsavedChanges),
    saveCount: computed(() => state.value.saveCount),

    // 方法
    startAutoSave,
    stopAutoSave,
    manualSave,
    restoreAutoSave,
    clearLocalStorage,
    loadFromLocalStorage
  }
}
