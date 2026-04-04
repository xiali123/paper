import { defineStore } from 'pinia'
import { ref } from 'vue'

export const useAppStore = defineStore('app', () => {
  const sidebarCollapsed = ref(false)
  const theme = ref<'light' | 'dark'>('light')
  const loading = ref(false)
  const pageLoading = ref(false)

  function toggleSidebar() {
    sidebarCollapsed.value = !sidebarCollapsed.value
  }

  function setSidebarCollapsed(collapsed: boolean) {
    sidebarCollapsed.value = collapsed
  }

  function toggleTheme() {
    theme.value = theme.value === 'light' ? 'dark' : 'light'
    document.documentElement.classList.toggle('dark', theme.value === 'dark')
  }

  function setTheme(newTheme: 'light' | 'dark') {
    theme.value = newTheme
    document.documentElement.classList.toggle('dark', newTheme === 'dark')
  }

  function setLoading(value: boolean) {
    loading.value = value
  }

  function setPageLoading(value: boolean) {
    pageLoading.value = value
  }

  return {
    sidebarCollapsed,
    theme,
    loading,
    pageLoading,
    toggleSidebar,
    setSidebarCollapsed,
    toggleTheme,
    setTheme,
    setLoading,
    setPageLoading,
  }
})
