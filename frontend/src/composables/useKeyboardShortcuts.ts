import { onMounted, onUnmounted } from 'vue'

export interface KeyboardShortcut {
  /** 快捷键组合，如 'Ctrl+S', 'Ctrl+Shift+P' */
  key: string
  /** 回调函数 */
  handler: (event: KeyboardEvent) => void
  /** 描述 */
  description?: string
  /** 是否阻止默认行为，默认true */
  preventDefault?: boolean
}

/**
 * 解析快捷键字符串
 */
function parseShortcutKey(shortcut: string): {
  ctrl: boolean
  shift: boolean
  alt: boolean
  meta: boolean
  key: string
} {
  const parts = shortcut.toLowerCase().split('+')
  const result = {
    ctrl: false,
    shift: false,
    alt: false,
    meta: false,
    key: ''
  }

  for (const part of parts) {
    switch (part) {
      case 'ctrl':
      case 'control':
        result.ctrl = true
        break
      case 'shift':
        result.shift = true
        break
      case 'alt':
        result.alt = true
        break
      case 'meta':
      case 'cmd':
      case 'command':
        result.meta = true
        break
      default:
        result.key = part
    }
  }

  return result
}

/**
 * 检查事件是否匹配快捷键
 */
function matchShortcut(event: KeyboardEvent, shortcut: string): boolean {
  const parsed = parseShortcutKey(shortcut)

  // 检查修饰键
  if (parsed.ctrl !== (event.ctrlKey || event.metaKey)) return false
  if (parsed.shift !== event.shiftKey) return false
  if (parsed.alt !== event.altKey) return false
  if (parsed.meta !== event.metaKey) return false

  // 检查按键
  const eventKey = event.key.toLowerCase()

  // 特殊按键映射
  const keyMap: Record<string, string[]> = {
    ' ': ['space', ' '],
    'escape': ['esc', 'escape'],
    'arrowup': ['up', 'arrowup'],
    'arrowdown': ['down', 'arrowdown'],
    'arrowleft': ['left', 'arrowleft'],
    'arrowright': ['right', 'arrowright'],
    'enter': ['enter', 'return'],
    'tab': ['tab']
  }

  // 检查是否匹配
  if (parsed.key === eventKey) return true

  // 检查特殊按键
  for (const [standardKey, aliases] of Object.entries(keyMap)) {
    if (aliases.includes(parsed.key) && aliases.includes(eventKey)) {
      return true
    }
  }

  return false
}

/**
 * 键盘快捷键 Hook
 */
export function useKeyboardShortcuts(
  shortcuts: KeyboardShortcut[],
  options?: {
    /** 是否启用，默认true */
    enabled?: (() => boolean) | boolean
    /** 目标元素，默认document */
    target?: HTMLElement | Document
  }
) {
  const {
    enabled = true,
    target = document
  } = options || {}

  const handleKeyDown = (event: KeyboardEvent) => {
    // 检查是否启用
    const isEnabled = typeof enabled === 'function' ? enabled() : enabled
    if (!isEnabled) return

    // 忽略输入框中的快捷键（除非是特定快捷键）
    const targetElement = event.target as HTMLElement
    const isInputElement =
      targetElement.tagName === 'INPUT' ||
      targetElement.tagName === 'TEXTAREA' ||
      targetElement.isContentEditable

    // 如果在输入框中，只处理特定快捷键
    if (isInputElement) {
      const allowedInInput = shortcuts.filter(s => s.preventDefault !== false)
      for (const shortcut of allowedInInput) {
        if (matchShortcut(event, shortcut.key)) {
          if (shortcut.preventDefault !== false) {
            event.preventDefault()
          }
          shortcut.handler(event)
          return
        }
      }
      return
    }

    // 检查所有快捷键
    for (const shortcut of shortcuts) {
      if (matchShortcut(event, shortcut.key)) {
        if (shortcut.preventDefault !== false) {
          event.preventDefault()
        }
        shortcut.handler(event)
        return
      }
    }
  }

  onMounted(() => {
    target.addEventListener('keydown', handleKeyDown)
  })

  onUnmounted(() => {
    target.removeEventListener('keydown', handleKeyDown)
  })

  return {
    /** 手动触发快捷键 */
    triggerShortcut: (key: string) => {
      const shortcut = shortcuts.find(s => s.key === key)
      if (shortcut) {
        const event = new KeyboardEvent('keydown', {
          key: parseShortcutKey(key).key,
          ctrlKey: parseShortcutKey(key).ctrl,
          shiftKey: parseShortcutKey(key).shift,
          altKey: parseShortcutKey(key).alt,
          metaKey: parseShortcutKey(key).meta
        })
        shortcut.handler(event)
      }
    }
  }
}

/**
 * LaTeX编辑器常用快捷键配置
 */
export function getLatexShortcuts(handlers: {
  onSave?: () => void
  onCompile?: () => void
  onBold?: () => void
  onItalic?: () => void
  onUnderline?: () => void
  onUndo?: () => void
  onRedo?: () => void
  onFind?: () => void
  onReplace?: () => void
  onGoToLine?: () => void
  onTogglePreview?: () => void
  onToggleOutline?: () => void
  onToggleSnippets?: () => void
  onShowWelcome?: () => void
  onShowRecent?: () => void
  onShowSettings?: () => void
  onShowStats?: () => void
  onCommandPalette?: () => void
}): KeyboardShortcut[] {
  const shortcuts: KeyboardShortcut[] = []

  if (handlers.onSave) {
    shortcuts.push({
      key: 'Ctrl+S',
      handler: handlers.onSave,
      description: '保存文档'
    })
  }

  if (handlers.onCompile) {
    shortcuts.push({
      key: 'Ctrl+Enter',
      handler: handlers.onCompile,
      description: '编译文档'
    })
  }

  if (handlers.onBold) {
    shortcuts.push({
      key: 'Ctrl+B',
      handler: handlers.onBold,
      description: '粗体'
    })
  }

  if (handlers.onItalic) {
    shortcuts.push({
      key: 'Ctrl+I',
      handler: handlers.onItalic,
      description: '斜体'
    })
  }

  if (handlers.onUnderline) {
    shortcuts.push({
      key: 'Ctrl+U',
      handler: handlers.onUnderline,
      description: '下划线'
    })
  }

  if (handlers.onUndo) {
    shortcuts.push({
      key: 'Ctrl+Z',
      handler: handlers.onUndo,
      description: '撤销'
    })
  }

  if (handlers.onRedo) {
    shortcuts.push({
      key: 'Ctrl+Shift+Z',
      handler: handlers.onRedo,
      description: '重做'
    })
  }

  if (handlers.onFind) {
    shortcuts.push({
      key: 'Ctrl+F',
      handler: handlers.onFind,
      description: '查找'
    })
  }

  if (handlers.onReplace) {
    shortcuts.push({
      key: 'Ctrl+H',
      handler: handlers.onReplace,
      description: '替换'
    })
  }

  if (handlers.onGoToLine) {
    shortcuts.push({
      key: 'Ctrl+G',
      handler: handlers.onGoToLine,
      description: '跳转到行'
    })
  }

  if (handlers.onTogglePreview) {
    shortcuts.push({
      key: 'Ctrl+P',
      handler: handlers.onTogglePreview,
      description: '切换预览'
    })
  }

  if (handlers.onToggleOutline) {
    shortcuts.push({
      key: 'Ctrl+O',
      handler: handlers.onToggleOutline,
      description: '切换大纲'
    })
  }

  if (handlers.onToggleSnippets) {
    shortcuts.push({
      key: 'Ctrl+Space',
      handler: handlers.onToggleSnippets,
      description: '切换代码片段'
    })
  }

  if (handlers.onShowWelcome) {
    shortcuts.push({
      key: 'F1',
      handler: handlers.onShowWelcome,
      description: '欢迎引导'
    })
  }

  if (handlers.onShowRecent) {
    shortcuts.push({
      key: 'Ctrl+R',
      handler: handlers.onShowRecent,
      description: '最近文档'
    })
  }

  if (handlers.onShowSettings) {
    shortcuts.push({
      key: 'Ctrl+Alt+S',
      handler: handlers.onShowSettings,
      description: '编辑器设置'
    })
  }

  if (handlers.onShowStats) {
    shortcuts.push({
      key: 'Ctrl+Alt+D',
      handler: handlers.onShowStats,
      description: '文档统计'
    })
  }

  if (handlers.onCommandPalette) {
    shortcuts.push({
      key: 'Ctrl+Shift+P',
      handler: handlers.onCommandPalette,
      description: '命令面板'
    })
  }

  return shortcuts
}
