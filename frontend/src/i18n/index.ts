import { createI18n } from 'vue-i18n'

// Import translation files
import zhCN from './locales/zh-CN.json'
import enUS from './locales/en-US.json'

// Get saved language or use browser language
const getSavedLanguage = (): string => {
  const saved = localStorage.getItem('papercrawler-language')
  if (saved && (saved === 'zh-CN' || saved === 'en-US')) {
    return saved
  }
  // Detect browser language
  const browserLang = navigator.language
  if (browserLang.startsWith('zh')) {
    return 'zh-CN'
  }
  return 'en-US'
}

// Create i18n instance
const i18n = createI18n({
  legacy: false, // Use Composition API mode
  locale: getSavedLanguage(),
  fallbackLocale: 'en-US',
  messages: {
    'zh-CN': zhCN,
    'en-US': enUS
  },
  globalInjection: true
})

// Save language preference when it changes
export const setLanguage = (lang: string) => {
  i18n.global.locale.value = lang
  localStorage.setItem('papercrawler-language', lang)
}

export default i18n
