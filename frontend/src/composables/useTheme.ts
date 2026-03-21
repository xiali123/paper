import { ref, watch } from 'vue'

export type Theme = 'light' | 'dark'

const STORAGE_KEY = 'papercrawler-theme'

// Global theme state
const theme = ref<Theme>((localStorage.getItem(STORAGE_KEY) as Theme) || 'light')

// Apply theme to document
const applyTheme = (newTheme: Theme) => {
  if (typeof document !== 'undefined') {
    document.documentElement.setAttribute('data-theme', newTheme)
  }
}

// Initialize theme on load
if (typeof window !== 'undefined') {
  applyTheme(theme.value)
}

// Watch for theme changes and persist
watch(theme, (newTheme) => {
  applyTheme(newTheme)
  if (typeof window !== 'undefined') {
    localStorage.setItem(STORAGE_KEY, newTheme)
  }
})

export function useTheme() {
  const toggleTheme = () => {
    theme.value = theme.value === 'light' ? 'dark' : 'light'
  }

  const setTheme = (newTheme: Theme) => {
    theme.value = newTheme
  }

  const isDark = () => theme.value === 'dark'
  const isLight = () => theme.value === 'light'

  return {
    theme,
    toggleTheme,
    setTheme,
    isDark,
    isLight
  }
}
