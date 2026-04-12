import { ref, computed, watch } from 'vue'

export interface UndoRedoOptions {
  /** 最大历史记录数 */
  maxSize?: number
  /** 是否启用 */
  enabled?: Ref<boolean> | boolean
}

export interface HistoryEntry {
  content: string
  cursorPosition?: {
    line: number
    column: number
  }
  timestamp: number
}

/**
 * 撤销/重做 Hook
 */
export function useUndoRedo(
  content: Ref<string>,
  cursorPosition: Ref<{ line: number; column: number }>,
  options: UndoRedoOptions = {}
) {
  const {
    maxSize = 100,
    enabled = ref(true)
  } = options

  // 历史记录
  const history = ref<HistoryEntry[]>([])
  const currentIndex = ref(-1)

  // 状态
  const isEnabled = ref(typeof enabled === 'boolean' ? enabled : enabled.value)
  const isUndoing = ref(false)
  const isRedoing = ref(false)

  // 监听 enabled 变化
  if (typeof enabled !== 'boolean') {
    watch(enabled, (value) => {
      isEnabled.value = value
    })
  }

  // 计算属性
  const canUndo = computed(() => isEnabled.value && currentIndex.value > 0)
  const canRedo = computed(() => isEnabled.value && currentIndex.value < history.value.length - 1)

  /**
   * 添加历史记录
   */
  function pushHistory(entry?: Partial<HistoryEntry>) {
    if (!isEnabled.value) return

    const newEntry: HistoryEntry = {
      content: content.value,
      cursorPosition: { ...cursorPosition.value },
      timestamp: Date.now(),
      ...entry
    }

    // 如果当前不在历史记录末尾，删除后面的记录
    if (currentIndex.value < history.value.length - 1) {
      history.value = history.value.slice(0, currentIndex.value + 1)
    }

    // 添加新记录
    history.value.push(newEntry)
    currentIndex.value = history.value.length - 1

    // 限制历史记录大小
    if (history.value.length > maxSize) {
      history.value.shift()
      currentIndex.value--
    }
  }

  /**
   * 撤销
   */
  function undo(): boolean {
    if (!canUndo.value) return false

    isUndoing.value = true
    try {
      currentIndex.value--
      const entry = history.value[currentIndex.value]

      if (entry) {
        content.value = entry.content
        if (entry.cursorPosition) {
          cursorPosition.value = entry.cursorPosition
        }
        return true
      }
      return false
    } finally {
      isUndoing.value = false
    }
  }

  /**
   * 重做
   */
  function redo(): boolean {
    if (!canRedo.value) return false

    isRedoing.value = true
    try {
      currentIndex.value++
      const entry = history.value[currentIndex.value]

      if (entry) {
        content.value = entry.content
        if (entry.cursorPosition) {
          cursorPosition.value = entry.cursorPosition
        }
        return true
      }
      return false
    } finally {
      isRedoing.value = false
    }
  }

  /**
   * 清空历史
   */
  function clear() {
    history.value = []
    currentIndex.value = -1
  }

  /**
   * 获取历史记录
   */
  function getHistory(): HistoryEntry[] {
    return [...history.value]
  }

  /**
   * 监听内容变化，自动添加历史记录
   */
  let debounceTimer: number | null = null
  let lastContent = ''

  watch(
    () => content.value,
    (newContent) => {
      if (isUndoing.value || isRedoing.value) return
      if (newContent === lastContent) return

      lastContent = newContent

      // 防抖，避免频繁添加历史记录
      if (debounceTimer) {
        clearTimeout(debounceTimer)
      }

      debounceTimer = setTimeout(() => {
        pushHistory()
        debounceTimer = null
      }, 500)
    },
    { immediate: true }
  )

  // 初始化历史记录
  if (content.value) {
    pushHistory()
  }

  return {
    // 状态
    canUndo,
    canRedo,
    isUndoing: readonly(isUndoing),
    isRedoing: readonly(isRedoing),
    historySize: computed(() => history.value.length),
    currentIndex: computed(() => currentIndex.value),

    // 方法
    undo,
    redo,
    pushHistory,
    clear,
    getHistory
  }
}
