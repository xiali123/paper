<template>
  <div class="login-container">
    <div class="login-card">
      <!-- Header with Logo -->
      <div class="login-header">
        <div class="logo">
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
        <h1 class="login-title">{{ t('auth.welcomeTo') }} PaperCrawler</h1>
        <p class="login-subtitle">{{ t('auth.loginSubtitle') }}</p>
      </div>

      <!-- Login Form -->
      <form @submit.prevent="handleLogin" class="login-form" novalidate>
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

        <!-- Password Field -->
        <div class="form-group" :class="{ 'has-error': errors.password }">
          <label for="password">
            {{ t('auth.password') }}
            <span class="required">*</span>
          </label>
          <div class="input-wrapper">
            <span class="input-icon">
              <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
                <path fill-rule="evenodd" d="M5 9V7a5 5 0 0110 0v2a2 2 0 012 2v5a2 2 0 01-2 2H5a2 2 0 01-2-2v-5a2 2 0 012-2zm8-2v2H7V7a3 3 0 016 0z" clip-rule="evenodd"/>
              </svg>
            </span>
            <input
              id="password"
              v-model="form.password"
              :type="showPassword ? 'text' : 'password'"
              :placeholder="t('auth.passwordPlaceholder')"
              :disabled="loading"
              @blur="validatePassword"
              @input="clearError('password')"
            />
            <button
              type="button"
              class="toggle-password"
              @click="showPassword = !showPassword"
              tabindex="-1"
            >
              <svg v-if="showPassword" width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
                <path fill-rule="evenodd" d="M3.707 2.293a1 1 0 00-1.414 1.414l14 14a1 1 0 001.414-1.414l-1.473-1.473A10.014 10.014 0 0019.542 10C18.268 5.943 14.478 3 10 3a9.958 9.958 0 00-4.512 1.074l-1.78-1.781zm4.261 4.26l1.514 1.515a2.003 2.003 0 012.45 2.45l1.514 1.514a4 4 0 00-5.478-5.478z" clip-rule="evenodd"/>
                <path d="M12.454 16.697L9.75 13.992a4 4 0 01-3.742-3.741L2.335 6.578A9.98 9.98 0 00.458 10c1.274 4.057 5.065 7 9.542 7 .847 0 1.669-.105 2.454-.303z"/>
              </svg>
              <svg v-else width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
                <path d="M10 12a2 2 0 100-4 2 2 0 000 4z"/>
                <path fill-rule="evenodd" d="M.458 10C1.732 5.943 5.522 3 10 3s8.268 2.943 9.542 7c-1.274 4.057-5.064 7-9.542 7S1.732 14.057.458 10zM14 10a4 4 0 11-8 0 4 4 0 018 0z" clip-rule="evenodd"/>
              </svg>
            </button>
          </div>
          <span v-if="errors.password" class="error-message">
            {{ errors.password }}
          </span>
        </div>

        <!-- Remember Me & Forgot Password -->
        <div class="form-actions">
          <label class="checkbox-label">
            <input type="checkbox" v-model="form.rememberMe" :disabled="loading" />
            <span>{{ t('auth.rememberMe') }}</span>
          </label>
          <router-link to="/auth/forgot-password" class="forgot-link">
            {{ t('auth.forgotPassword') }}
          </router-link>
        </div>

        <!-- Error Display -->
        <div v-if="apiError" class="api-error">
          <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
            <path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z" clip-rule="evenodd"/>
          </svg>
          <span>{{ apiError }}</span>
        </div>

        <!-- Submit Button -->
        <button type="submit" class="login-button" :disabled="loading">
          <span v-if="loading" class="loading-spinner"></span>
          <span v-else>{{ t('auth.loginButton') }}</span>
        </button>
      </form>

      <!-- Social Login (Optional) -->
      <div class="social-login">
        <div class="divider">
          <span>{{ t('auth.orContinueWith') }}</span>
        </div>
        <div class="social-buttons">
          <button type="button" class="social-button google" :disabled="loading">
            <svg width="20" height="20" viewBox="0 0 20 20">
              <path fill="#4285F4" d="M19.82 9.48c0-.66-.06-1.3-.16-1.92H10v3.64h5.44c-.24 1.26-.96 2.32-2.04 3.04v2.36h3.3c1.92-1.78 3.02-4.4 3.02-7.48z"/>
              <path fill="#34A853" d="M10 18c2.7 0 4.96-.9 6.62-2.42l-3.3-2.36c-.9.6-2.06.96-3.32.96-2.56 0-4.72-1.72-5.5-4.04H1.1v2.44C2.76 15.76 6.16 18 10 18z"/>
              <path fill="#FBBC05" d="M4.5 10.14c-.2-.6-.3-1.24-.3-1.9 0-.66.1-1.3.3-1.9V3.76H1.1C.4 5.16 0 6.68 0 9.24s.4 4.08 1.1 5.48l3.4-2.58z"/>
              <path fill="#EA4335" d="M10 3.8c1.44 0 2.72.5 3.74 1.46l2.92-2.92C14.96.94 12.7 0 10 0 6.16 0 2.76 2.24 1.1 5.76l3.4 2.58C5.28 6.02 7.44 4.3 10 4.3v-.5z"/>
            </svg>
            Google
          </button>
          <button type="button" class="social-button github" :disabled="loading">
            <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
              <path fill-rule="evenodd" d="M10 0C4.477 0 0 4.484 0 10.017c0 4.425 2.865 8.18 6.839 9.504.5.092.682-.217.682-.483 0-.237-.008-.868-.013-1.703-2.782.605-3.369-1.343-3.369-1.343-.454-1.158-1.11-1.466-1.11-1.466-.908-.62.069-.608.069-.608 1.003.07 1.531 1.032 1.531 1.032.892 1.53 2.341 1.088 2.91.832.092-.647.35-1.088.636-1.338-2.22-.253-4.555-1.113-4.555-4.951 0-1.093.39-1.988 1.029-2.688-.103-.253-.446-1.272.098-2.65 0 0 .84-.27 2.75 1.026A9.564 9.564 0 0110 4.844c.85.004 1.705.115 2.504.337 1.909-1.296 2.747-1.027 2.747-1.027.546 1.379.202 2.398.1 2.651.64.7 1.028 1.595 1.028 2.688 0 3.848-2.339 4.695-4.566 4.943.359.309.678.92.678 1.855 0 1.338-.012 2.419-.012 2.747 0 .268.18.58.688.482A10.019 10.019 0 0020 10.017C20 4.484 15.522 0 10 0z" clip-rule="evenodd"/>
            </svg>
            GitHub
          </button>
        </div>
      </div>

      <!-- Footer -->
      <div class="login-footer">
        <p class="footer-text">{{ t('auth.noAccount') }}</p>
        <router-link to="/auth/register" class="register-link">
          {{ t('auth.createAccount') }}
        </router-link>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { ElMessage } from 'element-plus'
import { useAuthStore } from '@/stores'

const { t } = useI18n()
const router = useRouter()
const route = useRoute()
const authStore = useAuthStore()

// Refs
const emailInput = ref<HTMLInputElement>()
const showPassword = ref(false)
const loading = ref(false)
const apiError = ref('')

// Form data
const form = reactive({
  email: '',
  password: '',
  rememberMe: false
})

// Validation errors
const errors = reactive({
  email: '',
  password: ''
})

// Focus email input on mount and restore saved email
onMounted(() => {
  // Check if user previously checked "Remember Me"
  const rememberMe = localStorage.getItem('remember_me')
  const rememberedEmail = localStorage.getItem('remembered_email')

  if (rememberMe === 'true' && rememberedEmail) {
    form.email = rememberedEmail
    form.rememberMe = true
  }

  emailInput.value?.focus()
})

// Clear specific error
const clearError = (field: 'email' | 'password') => {
  errors[field] = ''
  if (apiError.value) apiError.value = ''
}

// Validate email or username
const validateEmail = (): boolean => {
  if (!form.email) {
    errors.email = t('auth.validation.emailRequired')
    return false
  }

  // 支持用户名或邮箱格式
  // 用户名：3-20个字符，字母数字下划线
  // 邮箱：标准邮箱格式
  const usernameRegex = /^\w{3,20}$/
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/

  if (!usernameRegex.test(form.email) && !emailRegex.test(form.email)) {
    errors.email = '请输入有效的用户名或邮箱地址'
    return false
  }

  errors.email = ''
  return true
}

// Validate password
const validatePassword = (): boolean => {
  if (!form.password) {
    errors.password = t('auth.validation.passwordRequired')
    return false
  }

  if (form.password.length < 6) {
    errors.password = t('auth.validation.passwordMinLength')
    return false
  }

  errors.password = ''
  return true
}

// Handle login
const handleLogin = async () => {
  // Clear previous errors
  apiError.value = ''

  // Validate form
  const isEmailValid = validateEmail()
  const isPasswordValid = validatePassword()

  if (!isEmailValid || !isPasswordValid) {
    return
  }

  // Attempt login
  loading.value = true

  try {
    const result = await authStore.login({
      email: form.email,
      password: form.password,
      rememberMe: form.rememberMe
    })

    if (result.success) {
      ElMessage.success(t('auth.loginSuccess'))

      // 检查是否有重定向URL
      const redirect = route.query.redirect as string

      // 重定向优先级: query参数 > dashboard > writing
      if (redirect) {
        router.push(redirect)
      } else {
        // 默认跳转到写作页面
        router.push('/writing')
      }
    } else {
      apiError.value = result.error || t('auth.loginFailed')
    }
  } catch (error: any) {
    apiError.value = error.message || t('auth.errorOccurred')
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.login-container {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
  position: relative;
  overflow: hidden;
}

.login-container::before {
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

.login-card {
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

.login-header {
  text-align: center;
  margin-bottom: 40px;
}

.logo {
  margin-bottom: 20px;
  display: flex;
  justify-content: center;
}

.login-title {
  font-size: 28px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 8px;
}

.login-subtitle {
  color: #6b7280;
  font-size: 15px;
}

.login-form {
  margin-bottom: 24px;
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

.toggle-password {
  position: absolute;
  right: 12px;
  background: none;
  border: none;
  cursor: pointer;
  color: #9ca3af;
  padding: 4px;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: color 0.2s;
}

.toggle-password:hover {
  color: #667eea;
}

.error-message {
  display: block;
  color: #ef4444;
  font-size: 13px;
  margin-top: 6px;
}

.form-actions {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
}

.checkbox-label {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 14px;
  color: #374151;
  cursor: pointer;
  user-select: none;
}

.checkbox-label input[type="checkbox"] {
  width: 18px;
  height: 18px;
  cursor: pointer;
  accent-color: #667eea;
}

.checkbox-label input[type="checkbox"]:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

.forgot-link {
  color: #667eea;
  text-decoration: none;
  font-size: 14px;
  font-weight: 500;
  transition: all 0.2s;
}

.forgot-link:hover {
  color: #5568d3;
  text-decoration: underline;
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

.login-button {
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
}

.login-button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(102, 126, 234, 0.5);
}

.login-button:active:not(:disabled) {
  transform: translateY(0);
}

.login-button:disabled {
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

.social-login {
  margin-top: 32px;
}

.divider {
  display: flex;
  align-items: center;
  margin-bottom: 20px;
}

.divider::before,
.divider::after {
  content: '';
  flex: 1;
  height: 1px;
  background: #e5e7eb;
}

.divider span {
  padding: 0 16px;
  color: #9ca3af;
  font-size: 13px;
  font-weight: 500;
}

.social-buttons {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.social-button {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 12px 16px;
  border: 2px solid #e5e7eb;
  border-radius: 10px;
  background: white;
  font-size: 14px;
  font-weight: 600;
  color: #374151;
  cursor: pointer;
  transition: all 0.2s;
}

.social-button:hover:not(:disabled) {
  border-color: #667eea;
  background: #f9fafb;
}

.social-button:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.login-footer {
  margin-top: 32px;
  padding-top: 24px;
  border-top: 1px solid #e5e7eb;
  text-align: center;
}

.footer-text {
  color: #6b7280;
  font-size: 14px;
  margin-bottom: 4px;
}

.register-link {
  color: #667eea;
  text-decoration: none;
  font-weight: 600;
  font-size: 15px;
  transition: all 0.2s;
}

.register-link:hover {
  color: #5568d3;
  text-decoration: underline;
}

/* Responsive */
@media (max-width: 480px) {
  .login-card {
    padding: 32px 24px;
  }

  .social-buttons {
    grid-template-columns: 1fr;
  }
}

/* Dark mode */
[data-theme="dark"] .login-container {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

[data-theme="dark"] .login-card {
  background: rgba(31, 41, 55, 0.98);
  border: 1px solid rgba(102, 126, 234, 0.3);
}

[data-theme="dark"] .login-title {
  color: #f3f4f6;
}

[data-theme="dark"] .login-subtitle,
[data-theme="dark"] .footer-text {
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

[data-theme="dark"] .checkbox-label {
  color: #e5e7eb;
}

[data-theme="dark"] .divider::before,
[data-theme="dark"] .divider::after {
  background: rgba(255, 255, 255, 0.1);
}

[data-theme="dark"] .social-button {
  background: rgba(255, 255, 255, 0.05);
  border-color: rgba(255, 255, 255, 0.1);
  color: #e5e7eb;
}

[data-theme="dark"] .social-button:hover:not(:disabled) {
  background: rgba(255, 255, 255, 0.08);
  border-color: #818cf8;
}

[data-theme="dark"] .login-footer {
  border-top-color: rgba(102, 126, 234, 0.3);
}

[data-theme="dark"] .api-error {
  background: rgba(127, 29, 29, 0.3);
  border-color: rgba(239, 68, 68, 0.3);
  color: #fca5a5;
}
</style>