// React i18n hooks and utilities
import { useTranslation as useTranslationBase } from 'react-i18next';
import type { TranslationResources } from './locales';

// Typed hook with proper types
export function useTranslation() {
  const { t, i18n } = useTranslationBase<keyof TranslationResources>();

  return {
    t,
    i18n,
    changeLanguage: (lang: string) => i18n.changeLanguage(lang),
    currentLanguage: i18n.language as 'zh-CN' | 'en-US',
  };
}

// Helper function to translate with namespace
export function translate(key: keyof TranslationResources, params?: Record<string, any>): string {
  const { t } = useTranslationBase();
  return t(key, params);
}
