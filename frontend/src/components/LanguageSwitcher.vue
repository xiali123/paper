<template>
  <div class="language-switcher">
    <select
      v-model="currentLanguage"
      @change="changeLanguage(currentLanguage)"
      class="language-select"
      title="Switch Language"
    >
      <option value="zh-CN">🇨🇳 中文</option>
      <option value="en-US">🇺🇸 English</option>
    </select>
  </div>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import { useI18n } from 'vue-i18n'

const { locale } = useI18n()
const currentLanguage = ref(locale.value)

// Watch for locale changes and update select
watch(locale, (newLocale) => {
  currentLanguage.value = newLocale
  // Save to localStorage
  localStorage.setItem('papercrawler-language', newLocale)
})

const changeLanguage = (lang: string) => {
  locale.value = lang
  localStorage.setItem('papercrawler-language', lang)
}
</script>

<style scoped>
.language-switcher {
  display: inline-block;
}

.language-select {
  padding: 6px 12px;
  border: 1px solid var(--color-border-primary, #d1d5db);
  border-radius: 6px;
  background: var(--color-bg-secondary, #ffffff);
  color: var(--color-text-primary, #1f2937);
  font-size: 14px;
  cursor: pointer;
  transition: all 0.3s;
}

.language-select:hover {
  border-color: var(--color-primary, #667eea);
  background: var(--color-bg-tertiary, #f9fafb);
}

.language-select:focus {
  outline: none;
  border-color: var(--color-primary, #667eea);
  box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
}

[data-theme="dark"] .language-select {
  background: var(--color-bg-secondary, #1f2937);
  border-color: var(--color-border-primary, #374151);
  color: var(--color-text-primary, #f9fafb);
}

[data-theme="dark"] .language-select:hover {
  background: var(--color-bg-tertiary, #374151);
}
</style>
