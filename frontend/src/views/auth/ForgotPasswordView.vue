<template>
  <div class="forgot-password-container">
    <div class="forgot-password-card">
      <!-- Header -->
      <div class="forgot-password-header">
        <div class="icon-wrapper">
          <svg width="48" height="48" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
            <rect width="48" height="48" rx="12" fill="url(#gradient)"/>
            <path d="M24 14L34 24L24 34L14 24L24 14Z" fill="white"/>
            <defs>
              <linearGradient id="gradient" x1="0" y1="0" x2="48" y2="48">
                <stop offset="0%" stop-color="#667eea"/>
                <stop offset="100%" stop-color="#764ba2"/>
              </linearGradient>
            </defs>
          </svg>
        </div>
        <h1 class="forgot-password-title">{{ t('auth.forgotPassword') }}</h1>
        <p class="forgot-password-subtitle">{{ t('auth.forgotPasswordSubtitle') }}</p>
      </div>

      <!-- Success State -->
      <div v-if="emailSent" class="success-state">
        <div class="success-icon">
          <svg width="64" height="64" viewBox="0 0 64 64" fill="none">
            <circle cx="32" cy="32" r="32" fill="#10b981" opacity="0.1"/>
            <circle cx="32" cy="32" r="24" fill="#10b981" opacity="0.2"/>
            <path d="M32 40L22 32L24.5 29.5L31 36L43 24L45 26L32 40Z" fill="#10b981"/>
          </svg>
        </div>
        <h2 class="success-title">{{ t('auth.emailSent') }}</h2>
        <p class="success-message">
          {{ t('auth.passwordResetEmailSent') }}
          <strong>{{ form.email }}</strong>
        </p>
        <p class="success-hint">{{ t('auth.checkEmailInstructions') }}</p>

        <button class="resend-button" @click="handleResend" :disabled="loading">
          <span v-if="loading" class="loading-spinner"></span>
          <span v-else>{{ t('auth.resendEmail') }}</span>
        </button>

        <div class="back-to-login">
          <router-link to="/auth/login" class="back-link">
            <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
              <path fill-rule="evenodd" d="M9.354 1.646a.5.5 0 01-.008.716l-4.847 4.847a.5.5 0 01-.708 0l-1.146-1.146a.5.5 0 010-.708l4-4a.5.5 0 01.708 0l1.146 1.146a.5.5 0 01-.008.708z" clip-rule="evenodd"/>
            </svg>
            {{ t('auth.backToLogin') }}
          </router-link>
        </div>
      </div>

      <!-- Form State -->
      <form v-else @submit.prevent="handleSubmit" class="forgot-password-form" novalidate>
        <!-- Email Field -->
        <div class="form-group" :class="{ 'has-error': errors.email }">
          <label for="email">
            {{ t('auth.email') }}
            <span class="required">*</span>
          </label>
          <div class="input-wrapper">
            <span class="input-icon">
              <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
                <path d="M2.003 5.884L10 9.882l7.997-3.998A2 2 0 0016 4H4a2 2 0 00-1.997 1.884z"/>
                <path d="M18 8.118l-8 4-8-4V14a2 2 0 002 2h12a2 2 0 002-2V8.118z"/>
              </svg>
            </span>
            <input
              id="email"
              ref="emailInput"
              v-model="form.email"
              type="email"
              :placeholder="t('auth.emailPlaceholder')"
              :disabled="loading"
              @blur="validateEmail"
              @input="clearError('email')"
            />
          </div>
          <span v-if="errors.email" class="error-message">
            {{ errors.email }}
          </span>
        </div>

        <!-- Error Display -->
        <div v-if="apiError" class="api-error">
          <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
            <path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z" clip-rule="evenodd"/>
          </svg>
          <span>{{ apiError }}</span>
        </div>

        <!-- Submit Button -->
        <button type="submit" class="submit-button" :disabled="loading">
          <span v-if="loading" class="loading-spinner"></span>
          <span v-else>{{ t('auth.sendResetLink') }}</span>
        </button>

        <!-- Back to Login -->
        <div class="back-to-login">
          <router-link to="/auth/login" class="back-link">
            <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
              <path fill-rule="evenodd" d="M9.354 1.646a.5.5 0 01-.008.716l-4.847 4.847a.5.5 0 01-.708 0l-1.146-1.146a.5.5 0 010-.708l4-4a.5.5 0 01.708 0l1.146 1.146a.5.5 0 01-.008.708z" clip-rule="evenodd"/>
            </svg>
            {{ t('auth.backToLogin') }}
          </router-link>
        </div>
      </form>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'
import { ElMessage } from 'element-plus'
import { useAuthStore } from '@/stores'

const { t } = useI18n()
const authStore = useAuthStore()

// Refs
const emailInput = ref<HTMLInputElement>()
const loading = ref(false)
const emailSent = ref(false)
const apiError = ref('')

// Form data
const form = reactive({
  email: ''
})

// Validation errors
const errors = reactive({
  email: ''
})

// Focus email input on mount
onMounted(() => {
  emailInput.value?.focus()
})

// Clear specific error
const clearError = (field: 'email') => {
  errors[field] = ''
  if (apiError.value) apiError.value = ''
}

// Validate email
const validateEmail = (): boolean => {
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/

  if (!form.email) {
    errors.email = t('auth.validation.emailRequired')
    return false
  }

  if (!emailRegex.test(form.email)) {
    errors.email = t('auth.validation.emailInvalid')
    return false
  }

  errors.email = ''
  return true
}

// Handle form submission
const handleSubmit = async () => {
  // Clear previous errors
  apiError.value = ''

  // Validate email
  if (!validateEmail()) {
    return
  }

  // Request password reset
  loading.value = true

  try {
    const result = await authStore.requestPasswordReset(form.email)

    if (result.success) {
      emailSent.value = true
      ElMessage.success(t('auth.resetEmailSentSuccess'))
    } else {
      apiError.value = result.error || t('auth.resetEmailFailed')
    }
  } catch (error: any) {
    apiError.value = error.message || t('auth.errorOccurred')
  } finally {
    loading.value = false
  }
}

// Handle resend email
const handleResend = async () => {
  // Clear success state and resubmit
  emailSent.value = false
  await handleSubmit()
}
</script>

<style scoped>
.forgot-password-container {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
  position: relative;
  overflow: hidden;
}

.forgot-password-container::before {
  content: '';
  position: absolute;
  top: -50%;
  left: -50%;
  width: 200%;
  height: 200%;
  background: radial-gradient(circle, rgba(255,255,255,0.1) 1px, transparent 1px);
  background-size: 50px 50px;
  animation: backgroundScroll 20s linear infinite;
}

@keyframes backgroundScroll {
  0% { transform: translate(0, 0); }
  100% { transform: translate(50px, 50px); }
}

.forgot-password-card {
  position: relative;
  width: 100%;
  max-width: 440px;
  background: white;
  border-radius: 20px;
  padding: 48px 40px;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
  animation: slideUp 0.5s ease-out;
}

@keyframes slideUp {
  from {
    opacity: 0;
    transform: translateY(20px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.forgot-password-header {
  text-align: center;
  margin-bottom: 40px;
}

.icon-wrapper {
  margin-bottom: 20px;
  display: flex;
  justify-content: center;
}

.forgot-password-title {
  font-size: 28px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 8px;
}

.forgot-password-subtitle {
  color: #6b7280;
  font-size: 15px;
  line-height: 1.5;
}

.forgot-password-form {
  margin-bottom: 24px;
}

.success-state {
  text-align: center;
  padding: 20px 0;
}

.success-icon {
  margin-bottom: 24px;
}

.success-title {
  font-size: 24px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 16px;
}

.success-message {
  color: #6b7280;
  font-size: 15px;
  line-height: 1.6;
  margin-bottom: 8px;
}

.success-message strong {
  color: #1f2937;
  font-weight: 600;
}

.success-hint {
  color: #9ca3af;
  font-size: 14px;
  margin-bottom: 32px;
}

.form-group {
  margin-bottom: 24px;
}

.form-group label {
  display: block;
  margin-bottom: 8px;
  font-weight: 600;
  color: #374151;
  font-size: 14px;
}

.required {
  color: #ef4444;
  margin-left: 2px;
}

.input-wrapper {
  position: relative;
  display: flex;
  align-items: center;
}

.input-icon {
  position: absolute;
  left: 14px;
  color: #9ca3af;
  pointer-events: none;
}

.input-wrapper input {
  width: 100%;
  padding: 12px 16px 12px 44px;
  border: 2px solid #e5e7eb;
  border-radius: 12px;
  font-size: 15px;
  transition: all 0.2s;
  background: #f9fafb;
  color: #1f2937;
}

.input-wrapper input:focus {
  outline: none;
  border-color: #667eea;
  background: white;
  box-shadow: 0 0 0 4px rgba(102, 126, 234, 0.1);
}

.input-wrapper input:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.form-group.has-error input {
  border-color: #ef4444;
}

.form-group.has-error input:focus {
  box-shadow: 0 0 0 4px rgba(239, 68, 68, 0.1);
}

.error-message {
  display: block;
  color: #ef4444;
  font-size: 13px;
  margin-top: 6px;
}

.api-error {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 16px;
  background: #fef2f2;
  border: 1px solid #fecaca;
  border-radius: 8px;
  color: #991b1b;
  font-size: 14px;
  margin-bottom: 20px;
}

.api-error svg {
  flex-shrink: 0;
  color: #dc2626;
}

.submit-button,
.resend-button {
  width: 100%;
  padding: 14px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border: none;
  border-radius: 12px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
  margin-bottom: 20px;
}

.submit-button:hover:not(:disabled),
.resend-button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(102, 126, 234, 0.5);
}

.submit-button:active:not(:disabled),
.resend-button:active:not(:disabled) {
  transform: translateY(0);
}

.submit-button:disabled,
.resend-button:disabled {
  opacity: 0.7;
  cursor: not-allowed;
  transform: none;
}

.loading-spinner {
  width: 20px;
  height: 20px;
  border: 2px solid rgba(255, 255, 255, 0.3);
  border-top-color: white;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

.back-to-login {
  text-align: center;
}

.back-link {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  color: #667eea;
  text-decoration: none;
  font-weight: 600;
  font-size: 14px;
  transition: all 0.2s;
}

.back-link:hover {
  color: #5568d3;
  text-decoration: underline;
}

/* Responsive */
@media (max-width: 480px) {
  .forgot-password-card {
    padding: 32px 24px;
  }
}

/* Dark mode */
[data-theme="dark"] .forgot-password-container {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

[data-theme="dark"] .forgot-password-card {
  background: rgba(31, 41, 55, 0.98);
  border: 1px solid rgba(102, 126, 234, 0.3);
}

[data-theme="dark"] .forgot-password-title,
[data-theme="dark"] .success-title {
  color: #f3f4f6;
}

[data-theme="dark"] .forgot-password-subtitle,
[data-theme="dark"] .success-message,
[data-theme="dark"] .success-hint {
  color: #9ca3af;
}

[data-theme="dark"] .form-group label {
  color: #e5e7eb;
}

[data-theme="dark"] .input-wrapper input {
  background: rgba(255, 255, 255, 0.05);
  border-color: rgba(255, 255, 255, 0.1);
  color: #f3f4f6;
}

[data-theme="dark"] .input-wrapper input:focus {
  background: rgba(255, 255, 255, 0.08);
  border-color: #818cf8;
}

[data-theme="dark"] .input-icon {
  color: #6b7280;
}

[data-theme="dark"] .api-error {
  background: rgba(127, 29, 29, 0.3);
  border-color: rgba(239, 68, 68, 0.3);
  color: #fca5a5;
}

[data-theme="dark"] .success-message strong {
  color: #f3f4f6;
}
</style>