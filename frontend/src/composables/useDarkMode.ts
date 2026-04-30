/**
 * 深色模式管理
 * 支持系统跟随、手动切换、主题持久化
 */

import { ref, computed, watch, onMounted } from 'vue'
import { ElMessage } from 'element-plus'

export type ThemeMode = 'light' | 'dark' | 'auto'

const THEME_KEY = 'app-theme-mode'
const DARK_CLASS = 'dark-mode'

// 全局主题状态
const themeMode = ref<ThemeMode>('auto')
const isDark = ref(false)

// 获取系统深色模式偏好
const getSystemDarkMode = (): boolean => {
  if (typeof window === 'undefined') return false
  return window.matchMedia('(prefers-color-scheme: dark)').matches
}

// 应用主题到DOM
const applyTheme = (dark: boolean) => {
  if (typeof document === 'undefined') return

  isDark.value = dark

  if (dark) {
    document.documentElement.classList.add(DARK_CLASS)
    document.body.classList.add(DARK_CLASS)
  } else {
    document.documentElement.classList.remove(DARK_CLASS)
    document.body.classList.remove(DARK_CLASS)
  }

  // 更新编辑器主题
  updateEditorTheme(dark)
}

// 更新Monaco编辑器主题
const updateEditorTheme = (dark: boolean) => {
  const editor = (window as any).monacoEditor
  if (editor) {
    // Monaco主题
    import('monaco-editor').then(monaco => {
      monaco.editor.setTheme(dark ? 'vs-dark' : 'vs')
    })
  }
}

// 初始化主题
const initializeTheme = () => {
  // 从localStorage读取保存的主题
  const saved = localStorage.getItem(THEME_KEY) as ThemeMode
  if (saved && ['light', 'dark', 'auto'].includes(saved)) {
    themeMode.value = saved
  }

  updateTheme()

  // 监听系统主题变化
  if (typeof window !== 'undefined') {
    const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)')
    mediaQuery.addEventListener('change', () => {
      if (themeMode.value === 'auto') {
        updateTheme()
      }
    })
  }
}

// 更新当前主题
const updateTheme = () => {
  let dark = false

  switch (themeMode.value) {
    case 'light':
      dark = false
      break
    case 'dark':
      dark = true
      break
    case 'auto':
      dark = getSystemDarkMode()
      break
  }

  applyTheme(dark)
}

// 设置主题模式
export const setThemeMode = (mode: ThemeMode) => {
  themeMode.value = mode
  localStorage.setItem(THEME_KEY, mode)
  updateTheme()

  const modeText = {
    light: '浅色模式',
    dark: '深色模式',
    auto: '跟随系统'
  }

  ElMessage.success(`已切换到${modeText[mode]}`)
}

// 切换深色模式（在light/dark之间切换）
export const toggleDarkMode = () => {
  const newMode: ThemeMode = isDark.value ? 'light' : 'dark'
  setThemeMode(newMode)
}

// 深色模式组合式函数
export function useDarkMode() {
  onMounted(() => {
    initializeTheme()
  })

  // 监听主题变化
  watch(themeMode, () => {
    updateTheme()
  })

  return {
    // 状态
    themeMode,
    isDark: computed(() => isDark.value),

    // 方法
    setThemeMode,
    toggleDarkMode,

    // 工具方法
    isDarkMode: () => isDark.value,
    getSystemDarkMode
  }
}

// CSS变量（深色模式）
export const darkThemeVars = {
  '--el-bg-color': '#1a1a1a',
  '--el-bg-color-page': '#141414',
  '--el-bg-color-overlay': '#1d1d1d',
  '--el-text-color-primary': '#E5EAF3',
  '--el-text-color-regular': '#CFD3DC',
  '--el-text-color-secondary': '#A3A6AD',
  '--el-text-color-placeholder': '#8D9095',
  '--el-border-color': '#4C4D4F',
  '--el-border-color-light': '#414243',
  '--el-border-color-lighter': '#363637',
  '--el-border-color-extra-light': '#2B2B2C',
  '--el-border-color-dark': '#363637',
  '--el-border-color-darker': '#2B2B2C',
  '--el-fill-color': '#2B2B2C',
  '--el-fill-color-light': '#262727',
  '--el-fill-color-lighter': '#212223',
  '--el-fill-color-extra-light': '#191919',
  '--el-fill-color-dark': '#303133',
  '--el-fill-color-darker': '#363637',
  '--el-fill-color-blank': 'transparent',
  '--el-box-shadow': '0 12px 32px 4px rgba(0, 0, 0, .36)',
  '--el-box-shadow-light': '0 0 12px rgba(0, 0, 0, .72)',
  '--el-box-shadow-lighter': '0 0 6px rgba(0, 0, 0, .54)',
  '--el-box-shadow-dark': '0 16px 48px 16px rgba(0, 0, 0, .72)',
  '--el-disabled-bg-color': '#262727',
  '--el-disabled-text-color': '#7F8389',
  '--el-disabled-border-color': '#4C4D4F',
  '--el-overlay-color': 'rgba(0, 0, 0, .8)',
  '--el-overlay-color-light': 'rgba(0, 0, 0, .7)',
  '--el-overlay-color-lighter': 'rgba(0, 0, 0, .5)',
  '--el-mask-color': 'rgba(255, 255, 255, .9)',
  '--el-mask-color-extra-light': 'rgba(255, 255, 255, .3)'
}

// 应用Element Plus深色主题
export const applyElementPlusDark = () => {
  const styleId = 'dark-theme-vars'
  let styleEl = document.getElementById(styleId)

  if (!styleEl) {
    styleEl = document.createElement('style')
    styleEl.id = styleId
    document.head.appendChild(styleEl)
  }

  let css = ':root {'
  for (const [key, value] of Object.entries(darkThemeVars)) {
    css += `${key}: ${value};`
  }
  css += '}'

  styleEl.textContent = css
}
