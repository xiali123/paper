<template>
  <div class="login-container">
    <div class="login-card">
      <div class="login-header">
        <h1 class="login-title">PaperCrawler</h1>
        <p class="login-subtitle">{{ t('auth.login') }}</p>
      </div>

      <form @submit.prevent="handleLogin" class="login-form">
        <div class="form-group">
          <label for="email">{{ t('auth.email') }}</label>
          <input
            id="email"
            v-model="form.email"
            type="email"
            :placeholder="t('auth.emailPlaceholder')"
            required
            :class="{ 'error': errors.email }"
            @blur="validateEmail"
          />
          <span v-if="errors.email" class="error-message">{{ errors.email }}</span>
        </div>

        <div class="form-group">
          <label for="password">{{ t('auth.password') }}</label>
          <div class="password-input">
            <input
              id="password"
              v-model="form.password"
              :type="showPassword ? 'text' : 'password'"
              :placeholder="t('auth.passwordPlaceholder')"
              required
              :class="{ 'error': errors.password }"
              @blur="validatePassword"
              @keyup.enter="handleLogin"
            />
            <button
              type="button"
              class="toggle-password"
              @click="showPassword = !showPassword"
              tabindex="-1"
            >
              {{ showPassword ? '👁️' : '👁️‍🗨️' }}
            </button>
          </div>
          <span v-if="errors.password" class="error-message">{{ errors.password }}</span>
        </div>

        <div class="form-actions">
          <label class="checkbox-label">
            <input type="checkbox" v-model="form.rememberMe" />
            <span>{{ t('auth.rememberMe') || 'Remember me' }}</span>
          </label>
          <router-link to="/forgot-password" class="forgot-link">
            {{ t('auth.forgotPassword') }}
          </router-link>
        </div>

        <button
          type="submit"
          class="login-button"
          :disabled="authStore.loading"
        >
          {{ authStore.loading ? t('common.loading') : t('auth.loginButton') }}
        </button>
      </form>

      <div class="login-footer">
        <p class="footer-text">{{ t('auth.noAccount') }}</p>
        <router-link to="/register" class="register-link">
          {{ t('auth.register') }}
        </router-link>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { ElMessage } from '@/utils/notification'
import { useAuthStore } from '@/stores/auth'

const { t } = useI18n()
const router = useRouter()
const route = useRoute()
const authStore = useAuthStore()

const showPassword = ref(false)
const errors = reactive({
  email: '',
  password: ''
})

const form = reactive({
  email: '',
  password: '',
  rememberMe: false
})

const validateEmail = () => {
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/
  if (!form.email) {
    errors.email = 'Please enter your email'
    return false
  } else if (!emailRegex.test(form.email)) {
    errors.email = 'Please enter a valid email'
    return false
  }
  errors.email = ''
  return true
}

const validatePassword = () => {
  if (!form.password) {
    errors.password = 'Please enter your password'
    return false
  } else if (form.password.length < 8) {
    errors.password = 'Password must be at least 8 characters'
    return false
  }
  errors.password = ''
  return true
}

async function handleLogin() {
  console.log('🔵 [Login] handleLogin called')
  console.log('🔵 [Login] Form data:', { email: form.email, passwordLength: form.password.length })

  const isEmailValid = validateEmail()
  const isPasswordValid = validatePassword()

  console.log('🔵 [Login] Validation:', { isEmailValid, isPasswordValid })

  if (!isEmailValid || !isPasswordValid) {
    console.log('❌ [Login] Validation failed')
    return
  }

  console.log('✅ [Login] Validation passed, calling authStore.login()')
  console.log('🔵 [Login] authStore.loading before:', authStore.loading)

  try {
    const result = await authStore.login({
      email: form.email,
      password: form.password
    })

    console.log('🔵 [Login] authStore.login() returned')
    console.log('🔵 [Login] authStore.loading after:', authStore.loading)
    console.log('🟢 [Login] Result:', result)
    console.log('🟢 [Login] User:', authStore.user)
    console.log('🟢 [Login] Tokens:', authStore.tokens)

    if (result.success) {
      ElMessage.success(t('auth.loginSuccess') || 'Login successful!')

      // Redirect to intended page or home
      const redirect = (route.query.redirect as string) || '/'
      console.log('🔵 [Login] Redirecting to:', redirect)
      router.push(redirect)
    } else {
      console.log('❌ [Login] Login failed:', result.error)
      ElMessage.error(result.error || t('auth.loginFailed') || 'Login failed')
    }
  } catch (error: any) {
    console.error('🔴 [Login] Exception caught:', error)
    ElMessage.error(error.message || t('auth.errorOccurred'))
  }

  console.log('🔵 [Login] handleLogin finished, authStore.loading:', authStore.loading)
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
}

.login-card {
  width: 100%;
  max-width: 420px;
  background: white;
  border-radius: 16px;
  padding: 40px;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
}

.login-header {
  text-align: center;
  margin-bottom: 32px;
}

.login-title {
  font-size: 28px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 8px;
}

.login-subtitle {
  color: #6b7280;
  font-size: 14px;
}

.login-form {
  margin-bottom: 24px;
}

.form-group {
  margin-bottom: 20px;
}

label {
  display: block;
  margin-bottom: 6px;
  font-weight: 500;
  color: #374151;
  font-size: 14px;
}

input[type="email"],
input[type="password"],
input[type="text"] {
  width: 100%;
  padding: 12px;
  border: 1px solid #d1d5db;
  border-radius: 8px;
  font-size: 14px;
  transition: all 0.2s;
  box-sizing: border-box;
}

input:focus {
  outline: none;
  border-color: #667eea;
  box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
}

input.error {
  border-color: #ef4444;
}

.password-input {
  position: relative;
  display: flex;
  align-items: center;
}

.password-input input {
  flex: 1;
  padding-right: 40px;
}

.toggle-password {
  position: absolute;
  right: 8px;
  background: none;
  border: none;
  cursor: pointer;
  font-size: 18px;
  padding: 4px;
  opacity: 0.6;
  transition: opacity 0.2s;
}

.toggle-password:hover {
  opacity: 1;
}

.error-message {
  display: block;
  color: #ef4444;
  font-size: 12px;
  margin-top: 4px;
}

.form-actions {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.checkbox-label {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 14px;
  color: #374151;
  cursor: pointer;
}

.checkbox-label input[type="checkbox"] {
  width: auto;
  margin: 0;
}

.forgot-link {
  color: #667eea;
  text-decoration: none;
  font-size: 14px;
  transition: color 0.3s ease;
}

.forgot-link:hover {
  color: #5568d3;
  text-decoration: underline;
}

.login-button {
  width: 100%;
  padding: 12px;
  background: #667eea;
  color: white;
  border: none;
  border-radius: 8px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s;
}

.login-button:hover:not(:disabled) {
  background: #5568d3;
}

.login-button:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.login-footer {
  text-align: center;
  margin-top: 24px;
  padding-top: 24px;
  border-top: 1px solid #e5e7eb;
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
  transition: color 0.3s ease;
}

.register-link:hover {
  color: #5568d3;
  text-decoration: underline;
}

/* Dark mode support */
[data-theme="dark"] .login-container {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

[data-theme="dark"] .login-card {
  background: rgba(40, 40, 45, 0.98);
  border: 1px solid rgba(102, 126, 234, 0.3);
}

[data-theme="dark"] .login-title {
  color: #f3f4f6;
}

[data-theme="dark"] .login-subtitle,
[data-theme="dark"] .footer-text {
  color: #9ca3af;
}

[data-theme="dark"] label {
  color: #e5e7eb;
}

[data-theme="dark"] input[type="email"],
[data-theme="dark"] input[type="password"],
[data-theme="dark"] input[type="text"] {
  background: rgba(255, 255, 255, 0.05);
  border-color: rgba(255, 255, 255, 0.1);
  color: #f3f4f6;
}

[data-theme="dark"] .checkbox-label {
  color: #e5e7eb;
}

[data-theme="dark"] .forgot-link,
[data-theme="dark"] .register-link {
  color: #818cf8;
}

[data-theme="dark"] .forgot-link:hover,
[data-theme="dark"] .register-link:hover {
  color: #a5b4fc;
}

[data-theme="dark"] .login-footer {
  border-top-color: rgba(102, 126, 234, 0.3);
}
</style>
