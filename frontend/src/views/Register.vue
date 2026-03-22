<template>
  <div class="register-container">
    <div class="register-card">
      <div class="register-header">
        <h1 class="register-title">{{ t('auth.register') }}</h1>
        <p class="register-subtitle">{{ t('auth.registerSubtitle') || 'Join PaperCrawler today' }}</p>
      </div>

      <form @submit.prevent="handleRegister" class="register-form">
        <div class="form-group">
          <label for="username">{{ t('auth.username') }}</label>
          <input
            id="username"
            v-model="form.username"
            type="text"
            :placeholder="t('auth.usernamePlaceholder')"
            required
            :class="{ 'error': errors.username }"
            @blur="validateUsername"
          />
          <span v-if="errors.username" class="error-message">{{ errors.username }}</span>
        </div>

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
          <label for="fullName">{{ t('auth.fullName') }}</label>
          <input
            id="fullName"
            v-model="form.fullName"
            type="text"
            :placeholder="t('auth.fullNamePlaceholder') || 'Your full name'"
            required
            :class="{ 'error': errors.fullName }"
            @blur="validateFullName"
          />
          <span v-if="errors.fullName" class="error-message">{{ errors.fullName }}</span>
        </div>

        <div class="form-group">
          <label for="password">{{ t('auth.password') }}</label>
          <div class="password-input">
            <input
              id="password"
              v-model="form.password"
              :type="showPassword ? 'text' : 'password'"
              :placeholder="t('auth.passwordPlaceholder') || 'Create a strong password'"
              required
              :class="{ 'error': errors.password }"
              @blur="validatePassword"
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

        <div class="form-group">
          <label for="confirmPassword">{{ t('auth.confirmPassword') }}</label>
          <div class="password-input">
            <input
              id="confirmPassword"
              v-model="form.confirmPassword"
              :type="showConfirmPassword ? 'text' : 'password'"
              :placeholder="t('auth.confirmPasswordPlaceholder')"
              required
              :class="{ 'error': errors.confirmPassword }"
              @blur="validateConfirmPassword"
              @keyup.enter="handleRegister"
            />
            <button
              type="button"
              class="toggle-password"
              @click="showConfirmPassword = !showConfirmPassword"
              tabindex="-1"
            >
              {{ showConfirmPassword ? '👁️' : '👁️‍🗨️' }}
            </button>
          </div>
          <span v-if="errors.confirmPassword" class="error-message">{{ errors.confirmPassword }}</span>
        </div>

        <div class="form-group checkbox-group">
          <label class="checkbox-label">
            <input type="checkbox" v-model="form.acceptTerms" required />
            <span>I accept the
              <a href="/terms" target="_blank" class="link">Terms of Service</a>
              and
              <a href="/privacy" target="_blank" class="link">Privacy Policy</a>
            </span>
          </label>
        </div>

        <button
          type="submit"
          class="register-button"
          :disabled="authStore.loading || !form.acceptTerms"
        >
          {{ authStore.loading ? t('common.loading') : t('auth.registerButton') || 'Create Account' }}
        </button>
      </form>

      <div class="register-footer">
        <p class="footer-text">{{ t('auth.hasAccount') || 'Already have an account?' }}</p>
        <router-link to="/login" class="login-link">
          {{ t('auth.login') }}
        </router-link>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import { useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { ElMessage } from '@/utils/notification'
import { useAuthStore } from '@/stores/auth'

const { t } = useI18n()
const router = useRouter()
const authStore = useAuthStore()

const showPassword = ref(false)
const showConfirmPassword = ref(false)
const errors = reactive({
  username: '',
  email: '',
  fullName: '',
  password: '',
  confirmPassword: ''
})

const form = reactive({
  username: '',
  email: '',
  fullName: '',
  password: '',
  confirmPassword: '',
  acceptTerms: false
})

const validateUsername = () => {
  if (!form.username) {
    errors.username = 'Please enter a username'
    return false
  } else if (form.username.length < 3 || form.username.length > 30) {
    errors.username = 'Username must be 3-30 characters'
    return false
  } else if (!/^[a-zA-Z0-9_-]+$/.test(form.username)) {
    errors.username = 'Username can only contain letters, numbers, underscores, and hyphens'
    return false
  }
  errors.username = ''
  return true
}

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

const validateFullName = () => {
  if (!form.fullName) {
    errors.fullName = 'Please enter your full name'
    return false
  }
  errors.fullName = ''
  return true
}

const validatePassword = () => {
  if (!form.password) {
    errors.password = 'Please enter a password'
    return false
  } else if (form.password.length < 8) {
    errors.password = 'Password must be at least 8 characters'
    return false
  } else if (!/[A-Z]/.test(form.password)) {
    errors.password = 'Password must contain at least one uppercase letter'
    return false
  } else if (!/[a-z]/.test(form.password)) {
    errors.password = 'Password must contain at least one lowercase letter'
    return false
  } else if (!/[0-9]/.test(form.password)) {
    errors.password = 'Password must contain at least one number'
    return false
  } else if (!/[!@#$%^&*()_+\-=\[\]{};':"\\|,.<>\/?]/.test(form.password)) {
    errors.password = 'Password must contain at least one special character'
    return false
  }

  // If confirm password has value, validate it too
  if (form.confirmPassword) {
    validateConfirmPassword()
  }

  errors.password = ''
  return true
}

const validateConfirmPassword = () => {
  if (!form.confirmPassword) {
    errors.confirmPassword = 'Please confirm your password'
    return false
  } else if (form.confirmPassword !== form.password) {
    errors.confirmPassword = 'Passwords do not match'
    return false
  }
  errors.confirmPassword = ''
  return true
}

async function handleRegister() {
  const isValid =
    validateUsername() &&
    validateEmail() &&
    validateFullName() &&
    validatePassword() &&
    validateConfirmPassword()

  if (!isValid) {
    return
  }

  if (!form.acceptTerms) {
    ElMessage.warning('You must accept the terms to continue')
    return
  }

  try {
    const result = await authStore.register({
      username: form.username,
      email: form.email,
      password: form.password,
      fullName: form.fullName
    })

    if (result.success) {
      ElMessage.success(t('auth.registerSuccess') || 'Account created successfully!')
      router.push('/')
    } else {
      ElMessage.error(result.error || t('auth.registerFailed') || 'Registration failed')
    }
  } catch (error: any) {
    console.error('Registration error:', error)
    ElMessage.error(error.message || t('auth.errorOccurred'))
  }
}
</script>

<style scoped>
.register-container {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
}

.register-card {
  width: 100%;
  max-width: 480px;
  background: white;
  border-radius: 16px;
  padding: 40px;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
}

.register-header {
  text-align: center;
  margin-bottom: 32px;
}

.register-title {
  font-size: 28px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 8px;
}

.register-subtitle {
  color: #6b7280;
  font-size: 14px;
}

.register-form {
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

input[type="text"],
input[type="email"],
input[type="password"] {
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

.checkbox-group {
  margin-bottom: 20px;
}

.checkbox-label {
  display: flex;
  align-items: flex-start;
  gap: 8px;
  font-size: 14px;
  color: #374151;
  cursor: pointer;
  line-height: 1.5;
}

.checkbox-label input[type="checkbox"] {
  width: auto;
  margin: 0;
  margin-top: 2px;
}

.link {
  color: #667eea;
  text-decoration: none;
}

.link:hover {
  text-decoration: underline;
}

.register-button {
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

.register-button:hover:not(:disabled) {
  background: #5568d3;
}

.register-button:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.register-footer {
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

.login-link {
  color: #667eea;
  text-decoration: none;
  font-weight: 600;
  transition: color 0.3s ease;
}

.login-link:hover {
  color: #5568d3;
  text-decoration: underline;
}

/* Dark mode support */
[data-theme="dark"] .register-container {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

[data-theme="dark"] .register-card {
  background: rgba(40, 40, 45, 0.98);
  border: 1px solid rgba(102, 126, 234, 0.3);
}

[data-theme="dark"] .register-title {
  color: #f3f4f6;
}

[data-theme="dark"] .register-subtitle,
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

[data-theme="dark"] .link,
[data-theme="dark"] .login-link {
  color: #818cf8;
}

[data-theme="dark"] .link:hover,
[data-theme="dark"] .login-link:hover {
  color: #a5b4fc;
}

[data-theme="dark"] .register-footer {
  border-top-color: rgba(102, 126, 234, 0.3);
}
</style>
