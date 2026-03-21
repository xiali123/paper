<template>
  <div class="language-switcher">
    <select
      v-model="currentLanguage"
      @change="changeLanguage(currentLanguage)"
      class="language-select"
      :title="$t('settings.language')"
    >
      <option value="zh-CN">🇨🇳 中文</option>
      <option value="en-US">🇺🇸 English</option>
    </select>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import { useTranslation } from '@/i18n/react-i18n';

const { currentLanguage, changeLanguage } = useTranslation();
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
  transition: all var(--transition-normal, 0.3s);
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
