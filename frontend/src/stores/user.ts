import { defineStore } from 'pinia'
import { ref, computed, watch } from 'vue'
import type { Paper } from '../types'

/**
 * User Store - 用户偏好设置管理
 * 管理用户偏好、主题设置、语言设置和其他个性化配置
 */
export interface UserPreferences {
  searchPageSize: number
  searchHistoryEnabled: boolean
  autoSaveFavorites: boolean
  showAbstracts: boolean
  compactView: boolean
  defaultSearchLevel?: string
  defaultSearchYear?: number
}

export interface ThemeSettings {
  mode: 'light' | 'dark' | 'auto'
  primaryColor: string
  fontSize: 'small' | 'medium' | 'large'
  highContrast: boolean
  reducedMotion: boolean
}

export type AppLanguage = 'en' | 'zh' | 'es' | 'fr' | 'de' | 'ja'

export const useUserStore = defineStore(
  'user',
  () => {
    // State
    const preferences = ref<UserPreferences>({
      searchPageSize: 20,
      searchHistoryEnabled: true,
      autoSaveFavorites: true,
      showAbstracts: false,
      compactView: false
    })

    const theme = ref<ThemeSettings>({
      mode: 'auto',
      primaryColor: '#1976d2',
      fontSize: 'medium',
      highContrast: false,
      reducedMotion: false
    })

    const language = ref<AppLanguage>('en')
    const sidebarCollapsed = ref(false)
    const tableColumns = ref<string[]>(['title', 'authors', 'journal', 'year', 'level', 'actions'])

    // Getters
    const currentTheme = computed(() => {
      if (theme.value.mode === 'auto') {
        const isDark = window.matchMedia('(prefers-color-scheme: dark)').matches
        return isDark ? 'dark' : 'light'
      }
      return theme.value.mode
    })

    const isDarkMode = computed(() => currentTheme.value === 'dark')

    const isLightMode = computed(() => currentTheme.value === 'light')

    const fontSizeClass = computed(() => {
      switch (theme.value.fontSize) {
        case 'small':
          return 'text-sm'
        case 'large':
          return 'text-lg'
        default:
          return 'text-base'
      }
    })

    const highContrastClass = computed(() => {
      return theme.value.highContrast ? 'high-contrast' : ''
    })

    const reducedMotionClass = computed(() => {
      return theme.value.reducedMotion ? 'reduced-motion' : ''
    })

    // Actions
    function updatePreferences(newPreferences: Partial<UserPreferences>): void {
      preferences.value = { ...preferences.value, ...newPreferences }
    }

    function updateTheme(newTheme: Partial<ThemeSettings>): void {
      theme.value = { ...theme.value, ...newTheme }
      applyTheme()
    }

    function setThemeMode(mode: 'light' | 'dark' | 'auto'): void {
      theme.value.mode = mode
      applyTheme()
    }

    function toggleDarkMode(): void {
      if (theme.value.mode === 'dark') {
        theme.value.mode = 'light'
      } else if (theme.value.mode === 'light') {
        theme.value.mode = 'dark'
      } else {
        // auto模式下，切换到dark
        theme.value.mode = 'dark'
      }
      applyTheme()
    }

    function setLanguage(lang: AppLanguage): void {
      language.value = lang
    }

    function toggleSidebar(): void {
      sidebarCollapsed.value = !sidebarCollapsed.value
    }

    function setSidebarCollapsed(collapsed: boolean): void {
      sidebarCollapsed.value = collapsed
    }

    function setTableColumns(columns: string[]): void {
      tableColumns.value = columns
    }

    function toggleTableColumn(column: string): void {
      if (tableColumns.value.includes(column)) {
        tableColumns.value = tableColumns.value.filter(c => c !== column)
      } else {
        tableColumns.value.push(column)
      }
    }

    function applyTheme(): void {
      const root = document.documentElement

      // 应用主题模式
      if (currentTheme.value === 'dark') {
        root.classList.add('dark')
        root.classList.remove('light')
      } else {
        root.classList.add('light')
        root.classList.remove('dark')
      }

      // 应用字体大小
      root.classList.remove('text-sm', 'text-base', 'text-lg')
      root.classList.add(fontSizeClass.value)

      // 应用高对比度
      if (theme.value.highContrast) {
        root.classList.add('high-contrast')
      } else {
        root.classList.remove('high-contrast')
      }

      // 应用减少动画
      if (theme.value.reducedMotion) {
        root.classList.add('reduced-motion')
      } else {
        root.classList.remove('reduced-motion')
      }

      // 应用主色调
      root.style.setProperty('--primary-color', theme.value.primaryColor)
    }

    function exportSettings(): string {
      const settings = {
        preferences: preferences.value,
        theme: theme.value,
        language: language.value,
        sidebarCollapsed: sidebarCollapsed.value,
        tableColumns: tableColumns.value
      }
      return JSON.stringify(settings, null, 2)
    }

    function importSettings(settingsJson: string): boolean {
      try {
        const settings = JSON.parse(settingsJson)

        if (settings.preferences) {
          preferences.value = { ...preferences.value, ...settings.preferences }
        }

        if (settings.theme) {
          theme.value = { ...theme.value, ...settings.theme }
          applyTheme()
        }

        if (settings.language) {
          language.value = settings.language
        }

        if (typeof settings.sidebarCollapsed === 'boolean') {
          sidebarCollapsed.value = settings.sidebarCollapsed
        }

        if (Array.isArray(settings.tableColumns)) {
          tableColumns.value = settings.tableColumns
        }

        return true
      } catch (error) {
        console.error('Import settings error:', error)
        return false
      }
    }

    function reset(): void {
      preferences.value = {
        searchPageSize: 20,
        searchHistoryEnabled: true,
        autoSaveFavorites: true,
        showAbstracts: false,
        compactView: false
      }

      theme.value = {
        mode: 'auto',
        primaryColor: '#1976d2',
        fontSize: 'medium',
        highContrast: false,
        reducedMotion: false
      }

      language.value = 'en'
      sidebarCollapsed.value = false
      tableColumns.value = ['title', 'authors', 'journal', 'year', 'level', 'actions']

      applyTheme()
    }

    // 监听主题变化，自动应用
    watch(() => theme.value.mode, () => {
      applyTheme()
    })

    // 监听系统主题变化（仅当设置为auto时）
    if (typeof window !== 'undefined') {
      const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)')
      mediaQuery.addEventListener('change', () => {
        if (theme.value.mode === 'auto') {
          applyTheme()
        }
      })
    }

    return {
      // State
      preferences,
      theme,
      language,
      sidebarCollapsed,
      tableColumns,

      // Getters
      currentTheme,
      isDarkMode,
      isLightMode,
      fontSizeClass,
      highContrastClass,
      reducedMotionClass,

      // Actions
      updatePreferences,
      updateTheme,
      setThemeMode,
      toggleDarkMode,
      setLanguage,
      toggleSidebar,
      setSidebarCollapsed,
      setTableColumns,
      toggleTableColumn,
      applyTheme,
      exportSettings,
      importSettings,
      reset
    }
  },
  {
    persist: {
      key: 'user-store',
      storage: localStorage
    }
  }
)
