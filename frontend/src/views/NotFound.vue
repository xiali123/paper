<template>
  <div class="not-found-page">
    <div class="not-found-container">
      <h1 class="error-code">404</h1>
      <h2 class="error-title">{{ t('errors.pageNotFound') }}</h2>
      <p class="error-message">{{ t('errors.pageNotFoundMessage') }}</p>

      <div class="error-actions">
        <router-link to="/" class="btn-primary">
          {{ t('errors.goHome') }}
        </router-link>
        <button @click="goBack" class="btn-secondary">
          {{ t('errors.goBack') }}
        </button>
      </div>

      <div class="helpful-links">
        <h3>{{ t('errors.helpfulLinks') }}</h3>
        <ul>
          <li>
            <router-link to="/search">{{ t('nav.searchPapers') }}</router-link>
          </li>
          <li>
            <router-link to="/stats">{{ t('nav.statistics') }}</router-link>
          </li>
          <li v-if="!isAuthenticated">
            <router-link to="/login">{{ t('auth.login') }}</router-link>
          </li>
        </ul>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { useAuthStore } from '@/stores/auth'
import { computed } from 'vue'

const router = useRouter()
const { t } = useI18n()
const authStore = useAuthStore()

const isAuthenticated = computed(() => authStore.isAuthenticated)

const goBack = () => {
  if (window.history.length > 1) {
    router.back()
  } else {
    router.push('/')
  }
}
</script>

<style scoped>
.not-found-page {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 2rem;
}

.not-found-container {
  max-width: 600px;
  width: 100%;
  text-align: center;
}

.error-code {
  font-size: 8rem;
  font-weight: bold;
  color: var(--color-primary);
  line-height: 1;
  margin-bottom: 1rem;
}

.error-title {
  font-size: 2rem;
  margin-bottom: 1rem;
  color: var(--color-text-primary);
}

.error-message {
  font-size: 1.125rem;
  color: var(--color-text-secondary);
  margin-bottom: 2rem;
}

.error-actions {
  display: flex;
  gap: 1rem;
  justify-content: center;
  margin-bottom: 3rem;
}

.btn-primary,
.btn-secondary {
  padding: 0.75rem 1.5rem;
  border-radius: 4px;
  text-decoration: none;
  font-size: 1rem;
  cursor: pointer;
  border: none;
}

.btn-primary {
  background: var(--color-primary);
  color: white;
}

.btn-secondary {
  background: var(--color-surface);
  color: var(--color-text-primary);
  border: 1px solid var(--color-border);
}

.helpful-links {
  text-align: left;
  background: var(--color-surface);
  padding: 2rem;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.helpful-links h3 {
  margin-bottom: 1rem;
  font-size: 1.125rem;
  color: var(--color-text-primary);
}

.helpful-links ul {
  list-style: none;
  padding: 0;
}

.helpful-links li {
  margin-bottom: 0.5rem;
}

.helpful-links a {
  color: var(--color-primary);
  text-decoration: none;
  transition: text-decoration 0.2s;
}

.helpful-links a:hover {
  text-decoration: underline;
}
</style>
