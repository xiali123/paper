<template>
  <div class="reset-password-page">
    <div class="reset-password-container">
      <h1>{{ t('auth.resetPassword') }}</h1>
      <p class="subtitle">{{ t('auth.resetPasswordSubtitle') }}</p>

      <form @submit.prevent="handleSubmit" class="reset-password-form">
        <div class="form-group">
          <label for="password">{{ t('auth.newPassword') }}</label>
          <input
            id="password"
            v-model="password"
            type="password"
            :placeholder="t('auth.passwordPlaceholder')"
            required
          />
        </div>

        <div class="form-group">
          <label for="confirmPassword">{{ t('auth.confirmPassword') }}</label>
          <input
            id="confirmPassword"
            v-model="confirmPassword"
            type="password"
            :placeholder="t('auth.confirmPasswordPlaceholder')"
            required
          />
        </div>

        <button type="submit" class="btn-primary" :disabled="loading || !isFormValid">
          {{ loading ? t('common.loading') : t('auth.resetPassword') }}
        </button>

        <p v-if="message" :class="['message', messageType]">
          {{ message }}
        </p>
      </form>

      <div class="links">
        <router-link to="/login">{{ t('auth.backToLogin') }}</router-link>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRoute } from 'vue-router'
import { useI18n } from 'vue-i18n'

const { t } = useI18n()
const route = useRoute()

const password = ref('')
const confirmPassword = ref('')
const loading = ref(false)
const message = ref('')
const messageType = ref<'success' | 'error'>('success')

const isFormValid = computed(() => {
  return password.value.length >= 8 && password.value === confirmPassword.value
})

const handleSubmit = async () => {
  if (!isFormValid.value) {
    message.value = t('auth.passwordsDoNotMatch')
    messageType.value = 'error'
    return
  }

  loading.value = true
  message.value = ''

  try {
    const token = route.query.token as string

    // TODO: Implement password reset
    // For now, show a placeholder message
    await new Promise(resolve => setTimeout(resolve, 1000))
    message.value = t('auth.passwordResetSuccess')
    messageType.value = 'success'
  } catch (error: any) {
    message.value = error.message || t('auth.errorOccurred')
    messageType.value = 'error'
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.reset-password-page {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 20px;
}

.reset-password-container {
  max-width: 400px;
  width: 100%;
}

h1 {
  text-align: center;
  margin-bottom: 0.5rem;
}

.subtitle {
  text-align: center;
  color: var(--color-text-secondary);
  margin-bottom: 2rem;
}

.reset-password-form {
  background: var(--color-surface);
  padding: 2rem;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.form-group {
  margin-bottom: 1.5rem;
}

label {
  display: block;
  margin-bottom: 0.5rem;
  font-weight: 500;
}

input {
  width: 100%;
  padding: 0.75rem;
  border: 1px solid var(--color-border);
  border-radius: 4px;
  font-size: 1rem;
}

button {
  width: 100%;
  padding: 0.75rem;
  background: var(--color-primary);
  color: white;
  border: none;
  border-radius: 4px;
  font-size: 1rem;
  cursor: pointer;
}

button:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.message {
  margin-top: 1rem;
  padding: 0.75rem;
  border-radius: 4px;
  text-align: center;
}

.message.success {
  background: var(--color-success);
  color: white;
}

.message.error {
  background: var(--color-error);
  color: white;
}

.links {
  margin-top: 1.5rem;
  text-align: center;
}

.links a {
  color: var(--color-primary);
  text-decoration: none;
}

.links a:hover {
  text-decoration: underline;
}
</style>
