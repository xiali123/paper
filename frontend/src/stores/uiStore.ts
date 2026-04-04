/**
 * UI Store (Pinia)
 *
 * Manages UI state including:
 * - Theme (light/dark/system)
 * - Language preference
 * - Sidebar state
 * - Layout preferences
 * - Notification settings
 *
 * @module stores/uiStore
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export type Theme = 'light' | 'dark' | 'system'
export type Language = 'en' | 'zh'
export type SidebarPosition = 'left' | 'right'
export type Density = 'compact' | 'comfortable' | 'spacious'

export interface UISettings {
  theme: Theme
  language: Language
  sidebarCollapsed: boolean
  sidebarPosition: SidebarPosition
  density: Density
  fontSize: number
  reducedMotion: boolean
  highContrast: boolean
  notifications: boolean
  sounds: boolean
  autoSave: boolean
  autoSaveInterval: number
}

export interface NotificationSettings {
  enable: boolean
  position: 'top-right' | 'top-left' | 'bottom-right' | 'bottom-left'
  duration: number
  showProgress: boolean
  allowClose: boolean
  maxVisible: number
}

export const useUIStore = defineStore(
  'ui',
  () => {
    // ========================================================================
    // State
    // ========================================================================

    /** UI settings */
    const settings = ref<UISettings>({
      theme: 'system',
      language: 'en',
      sidebarCollapsed: false,
      sidebarPosition: 'left',
      density: 'comfortable',
      fontSize: 14,
      reducedMotion: false,
      highContrast: false,
      notifications: true,
      sounds: false,
      autoSave: true,
      autoSaveInterval: 30000 // 30 seconds
    })

    /** Notification settings */
    const notificationSettings = ref<NotificationSettings>({
      enable: true,
      position: 'top-right',
      duration: 3000,
      showProgress: true,
      allowClose: true,
      maxVisible: 3
    })

    /** Loading overlay state */
    const loadingOverlay = ref(false)

    /** Loading message */
    const loadingMessage = ref('')

    /** Active modal stack */
    const modalStack = ref<string[]>([])

    /** Active drawer */
    const activeDrawer = ref<string | null>(null)

    /** Notification queue */
    const notificationQueue = ref<Array<{
      id: string
      type: 'success' | 'error' | 'warning' | 'info'
      title: string
      message: string
      duration?: number
    }>>([])

    // ========================================================================
    // Computed Properties
    // ========================================================================

    /** Current theme (resolved) */
    const currentTheme = computed<'light' | 'dark'>(() => {
      if (settings.value.theme === 'system') {
        return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'
      }
      return settings.value.theme
    })

    /** Is dark mode */
    const isDarkMode = computed(() => currentTheme.value === 'dark')

    /** Is sidebar collapsed */
    const isSidebarCollapsed = computed(() => settings.value.sidebarCollapsed)

    /** Is RTL (right-to-left) */
    const isRTL = computed(() => false) // Can be extended for RTL languages

    /** Font size in pixels */
    const fontSize = computed(() => `${settings.value.fontSize}px`)

    /** Has modal open */
    const hasModal = computed(() => modalStack.value.length > 0)

    /** Top modal */
    const topModal = computed(() => modalStack.value[modalStack.value.length - 1])

    /** Has drawer open */
    const hasDrawer = computed(() => activeDrawer.value !== null)

    /** Has notifications */
    const hasNotifications = computed(() => notificationQueue.value.length > 0)

    /** Density spacing values */
    const densitySpacing = computed(() => {
      switch (settings.value.density) {
        case 'compact':
          return { xs: 4, sm: 8, md: 12, lg: 16, xl: 20 }
        case 'comfortable':
          return { xs: 8, sm: 12, md: 16, lg: 20, xl: 24 }
        case 'spacious':
          return { xs: 12, sm: 16, md: 20, lg: 24, xl: 32 }
      }
    })

    // ========================================================================
    // Actions
    // ========================================================================

    /**
     * Set theme
     */
    function setTheme(theme: Theme) {
      settings.value.theme = theme
      applyTheme()
    }

    /**
     * Set language
     */
    function setLanguage(language: Language) {
      settings.value.language = language
    }

    /**
     * Toggle sidebar
     */
    function toggleSidebar() {
      settings.value.sidebarCollapsed = !settings.value.sidebarCollapsed
    }

    /**
     * Collapse sidebar
     */
    function collapseSidebar() {
      settings.value.sidebarCollapsed = true
    }

    /**
     * Expand sidebar
     */
    function expandSidebar() {
      settings.value.sidebarCollapsed = false
    }

    /**
     * Set sidebar position
     */
    function setSidebarPosition(position: SidebarPosition) {
      settings.value.sidebarPosition = position
    }

    /**
     * Set density
     */
    function setDensity(density: Density) {
      settings.value.density = density
    }

    /**
     * Set font size
     */
    function setFontSize(size: number) {
      settings.value.fontSize = Math.min(12, Math.max(20, size))
    }

    /**
     * Increase font size
     */
    function increaseFontSize() {
      setFontSize(settings.value.fontSize + 1)
    }

    /**
     * Decrease font size
     */
    function decreaseFontSize() {
      setFontSize(settings.value.fontSize - 1)
    }

    /**
     * Toggle reduced motion
     */
    function toggleReducedMotion() {
      settings.value.reducedMotion = !settings.value.reducedMotion
    }

    /**
     * Toggle high contrast
     */
    function toggleHighContrast() {
      settings.value.highContrast = !settings.value.highContrast
    }

    /**
     * Toggle notifications
     */
    function toggleNotifications() {
      settings.value.notifications = !settings.value.notifications
    }

    /**
     * Toggle sounds
     */
    function toggleSounds() {
      settings.value.sounds = !settings.value.sounds
    }

    /**
     * Toggle auto-save
     */
    function toggleAutoSave() {
      settings.value.autoSave = !settings.value.autoSave
    }

    /**
     * Set auto-save interval
     */
    function setAutoSaveInterval(interval: number) {
      settings.value.autoSaveInterval = Math.max(10000, interval) // Minimum 10 seconds
    }

    /**
     * Show loading overlay
     */
    function showLoading(message = 'Loading...') {
      loadingMessage.value = message
      loadingOverlay.value = true
    }

    /**
     * Hide loading overlay
     */
    function hideLoading() {
      loadingOverlay.value = false
      loadingMessage.value = ''
    }

    /**
     * Open modal
     */
    function openModal(modalId: string) {
      modalStack.value.push(modalId)
    }

    /**
     * Close modal
     */
    function closeModal(modalId?: string) {
      if (modalId) {
        const index = modalStack.value.indexOf(modalId)
        if (index !== -1) {
          modalStack.value.splice(index, 1)
        }
      } else {
        modalStack.value.pop()
      }
    }

    /**
     * Close all modals
     */
    function closeAllModals() {
      modalStack.value = []
    }

    /**
     * Open drawer
     */
    function openDrawer(drawerId: string) {
      activeDrawer.value = drawerId
    }

    /**
     * Close drawer
     */
    function closeDrawer() {
      activeDrawer.value = null
    }

    /**
     * Show notification
     */
    function showNotification(
      type: 'success' | 'error' | 'warning' | 'info',
      title: string,
      message: string,
      duration?: number
    ) {
      if (!settings.value.notifications || !notificationSettings.value.enable) {
        return
      }

      const id = `notification_${Date.now()}_${Math.random()}`
      const notification = {
        id,
        type,
        title,
        message,
        duration: duration || notificationSettings.value.duration
      }

      notificationQueue.value.push(notification)

      // Auto-remove after duration
      if (notification.duration > 0) {
        setTimeout(() => {
          removeNotification(id)
        }, notification.duration)
      }

      // Limit visible notifications
      if (notificationQueue.value.length > notificationSettings.value.maxVisible) {
        notificationQueue.value.shift()
      }
    }

    /**
     * Remove notification
     */
    function removeNotification(id: string) {
      const index = notificationQueue.value.findIndex(n => n.id === id)
      if (index !== -1) {
        notificationQueue.value.splice(index, 1)
      }
    }

    /**
     * Clear all notifications
     */
    function clearNotifications() {
      notificationQueue.value = []
    }

    /**
     * Update notification settings
     */
    function updateNotificationSettings(updates: Partial<NotificationSettings>) {
      notificationSettings.value = { ...notificationSettings.value, ...updates }
    }

    /**
     * Reset to defaults
     */
    function resetToDefaults() {
      settings.value = {
        theme: 'system',
        language: 'en',
        sidebarCollapsed: false,
        sidebarPosition: 'left',
        density: 'comfortable',
        fontSize: 14,
        reducedMotion: false,
        highContrast: false,
        notifications: true,
        sounds: false,
        autoSave: true,
        autoSaveInterval: 30000
      }

      notificationSettings.value = {
        enable: true,
        position: 'top-right',
        duration: 3000,
        showProgress: true,
        allowClose: true,
        maxVisible: 3
      }

      applyTheme()
    }

    /**
     * Reset state
     */
    function reset() {
      resetToDefaults()
      loadingOverlay.value = false
      loadingMessage.value = ''
      modalStack.value = []
      activeDrawer.value = null
      notificationQueue.value = []
    }

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * Apply theme to document
     */
    function applyTheme() {
      const theme = currentTheme.value
      document.documentElement.setAttribute('data-theme', theme)

      if (theme === 'dark') {
        document.documentElement.classList.add('dark')
      } else {
        document.documentElement.classList.remove('dark')
      }
    }

    /**
     * Listen for system theme changes
     */
    function setupThemeListener() {
      const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)')

      mediaQuery.addEventListener('change', () => {
        if (settings.value.theme === 'system') {
          applyTheme()
        }
      })
    }

    // Initialize theme on store creation
    applyTheme()
    setupThemeListener()

    // ========================================================================
    // Return
    // ========================================================================

    return {
      // State
      settings,
      notificationSettings,
      loadingOverlay,
      loadingMessage,
      modalStack,
      activeDrawer,
      notificationQueue,

      // Computed
      currentTheme,
      isDarkMode,
      isSidebarCollapsed,
      isRTL,
      fontSize,
      hasModal,
      topModal,
      hasDrawer,
      hasNotifications,
      densitySpacing,

      // Actions
      setTheme,
      setLanguage,
      toggleSidebar,
      collapseSidebar,
      expandSidebar,
      setSidebarPosition,
      setDensity,
      setFontSize,
      increaseFontSize,
      decreaseFontSize,
      toggleReducedMotion,
      toggleHighContrast,
      toggleNotifications,
      toggleSounds,
      toggleAutoSave,
      setAutoSaveInterval,
      showLoading,
      hideLoading,
      openModal,
      closeModal,
      closeAllModals,
      openDrawer,
      closeDrawer,
      showNotification,
      removeNotification,
      clearNotifications,
      updateNotificationSettings,
      resetToDefaults,
      reset
    }
  },
  {
    persist: {
      key: 'ui-store',
      storage: localStorage,
      paths: ['settings', 'notificationSettings']
    }
  }
)
