<template>
  <div class="register-container">
    <div class="register-card">
      <!-- Header -->
      <div class="register-header">
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
        <h1 class="register-title">{{ t('auth.createAccount') }}</h1>
        <p class="register-subtitle">{{ t('auth.registerSubtitle') }}</p>
      </div>

      <!-- Registration Form -->
      <form @submit.prevent="handleRegister" class="register-form" novalidate>
        <!-- Username Field -->
        <div class="form-group" :class="{ 'has-error': errors.username }">
          <label for="username">
            {{ t('auth.username') }}
            <span class="required">*</span>
          </label>
          <div class="input-wrapper">
            <span class="input-icon">
              <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
                <path fill-rule="evenodd" d="M10 9a3 3 0 100-6 3 3 0 000 6zm-7 9a7 7 0 1114 0H3z" clip-rule="evenodd"/>
              </svg>
            </span>
            <input
              id="username"
              v-model="form.username"
              type="text"
              :placeholder="t('auth.usernamePlaceholder')"
              :disabled="loading"
              @blur="validateUsername"
              @input="clearError('username')"
            />
          </div>
          <span v-if="errors.username" class="error-message">
            {{ errors.username }}
          </span>
        </div>

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
              @input="handlePasswordInput"
              @blur="validatePassword"
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

          <!-- Password Strength Indicator -->
          <div v-if="form.password" class="password-strength">
            <div class="strength-bar">
              <div
                class="strength-fill"
                :class="strength.level"
                :style="{ width: strength.percentage + '%' }"
              ></div>
            </div>
            <div class="strength-text">
              <span :class="strength.level">{{ t(`auth.passwordStrength.${strength.level}`) }}</span>
              <span class="strength-hint">{{ strength.hint }}</span>
            </div>
          </div>

          <span v-if="errors.password" class="error-message">
            {{ errors.password }}
          </span>
        </div>

        <!-- Confirm Password Field -->
        <div class="form-group" :class="{ 'has-error': errors.confirmPassword }">
          <label for="confirmPassword">
            {{ t('auth.confirmPassword') }}
            <span class="required">*</span>
          </label>
          <div class="input-wrapper">
            <span class="input-icon">
              <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
                <path fill-rule="evenodd" d="M5 9V7a5 5 0 0110 0v2a2 2 0 012 2v5a2 2 0 01-2 2H5a2 2 0 01-2-2v-5a2 2 0 012-2zm8-2v2H7V7a3 3 0 016 0z" clip-rule="evenodd"/>
              </svg>
            </span>
            <input
              id="confirmPassword"
              v-model="form.confirmPassword"
              :type="showConfirmPassword ? 'text' : 'password'"
              :placeholder="t('auth.confirmPasswordPlaceholder')"
              :disabled="loading"
              @blur="validateConfirmPassword"
              @input="clearError('confirmPassword')"
            />
            <button
              type="button"
              class="toggle-password"
              @click="showConfirmPassword = !showConfirmPassword"
              tabindex="-1"
            >
              <svg v-if="showConfirmPassword" width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
                <path fill-rule="evenodd" d="M3.707 2.293a1 1 0 00-1.414 1.414l14 14a1 1 0 001.414-1.414l-1.473-1.473A10.014 10.014 0 0019.542 10C18.268 5.943 14.478 3 10 3a9.958 9.958 0 00-4.512 1.074l-1.78-1.781zm4.261 4.26l1.514 1.515a2.003 2.003 0 012.45 2.45l1.514 1.514a4 4 0 00-5.478-5.478z" clip-rule="evenodd"/>
                <path d="M12.454 16.697L9.75 13.992a4 4 0 01-3.742-3.741L2.335 6.578A9.98 9.98 0 00.458 10c1.274 4.057 5.065 7 9.542 7 .847 0 1.669-.105 2.454-.303z"/>
              </svg>
              <svg v-else width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
                <path d="M10 12a2 2 0 100-4 2 2 0 000 4z"/>
                <path fill-rule="evenodd" d="M.458 10C1.732 5.943 5.522 3 10 3s8.268 2.943 9.542 7c-1.274 4.057-5.064 7-9.542 7S1.732 14.057.458 10zM14 10a4 4 0 11-8 0 4 4 0 018 0z" clip-rule="evenodd"/>
              </svg>
            </button>
          </div>
          <span v-if="errors.confirmPassword" class="error-message">
            {{ errors.confirmPassword }}
          </span>
        </div>

        <!-- Terms and Conditions -->
        <div class="form-group terms-group" :class="{ 'has-error': errors.terms }">
          <label class="checkbox-label">
            <input type="checkbox" v-model="form.agreeToTerms" :disabled="loading" />
            <span>
              {{ t('auth.agreeToTerms') }}
              <a href="/terms" target="_blank" class="link">{{ t('auth.termsOfService') }}</a>
              {{ t('auth.and') }}
              <a href="/privacy" target="_blank" class="link">{{ t('auth.privacyPolicy') }}</a>
            </span>
          </label>
          <span v-if="errors.terms" class="error-message">
            {{ errors.terms }}
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
        <button type="submit" class="register-button" :disabled="loading">
          <span v-if="loading" class="loading-spinner"></span>
          <span v-else>{{ t('auth.createAccountButton') }}</span>
        </button>
      </form>

      <!-- Footer -->
      <div class="register-footer">
        <p class="footer-text">{{ t('auth.alreadyHaveAccount') }}</p>
        <router-link to="/auth/login" class="login-link">
          {{ t('auth.login') }}
        </router-link>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { ElMessage } from 'element-plus'
import { useAuthStore } from '@/stores/auth'

const { t } = useI18n()
const router = useRouter()
const authStore = useAuthStore()

// Refs
const showPassword = ref(false)
const showConfirmPassword = ref(false)
const loading = ref(false)
const apiError = ref('')

// Form data
const form = reactive({
  username: '',
  email: '',
  password: '',
  confirmPassword: '',
  agreeToTerms: false
})

// Validation errors
const errors = reactive({
  username: '',
  email: '',
  password: '',
  confirmPassword: '',
  terms: ''
})

// Password strength calculation
const passwordStrength = computed(() => {
  const password = form.password
  if (!password) return { level: 'weak', percentage: 0, score: 0, hint: '' }

  let score = 0

  // Length check
  if (password.length >= 8) score += 1
  if (password.length >= 12) score += 1

  // Character variety
  if (/[a-z]/.test(password)) score += 1
  if (/[A-Z]/.test(password)) score += 1
  if (/[0-9]/.test(password)) score += 1
  if (/[^a-zA-Z0-9]/.test(password)) score += 1

  // Determine strength level
  let level = 'weak'
  let percentage = 0
  let hint = ''

  if (score <= 2) {
    level = 'weak'
    percentage = 33
    hint = t('auth.passwordStrength.hintWeak')
  } else if (score <= 4) {
    level = 'medium'
    percentage = 66
    hint = t('auth.passwordStrength.hintMedium')
  } else {
    level = 'strong'
    percentage = 100
    hint = t('auth.passwordStrength.hintStrong')
  }

  return { level, percentage, score, hint }
})

const strength = computed(() => passwordStrength.value)

// Clear specific error
const clearError = (field: keyof typeof errors) => {
  errors[field] = ''
  if (apiError.value) apiError.value = ''
}

// Validate username
const validateUsername = (): boolean => {
  if (!form.username) {
    errors.username = t('auth.validation.usernameRequired')
    return false
  }

  if (form.username.length < 3) {
    errors.username = t('auth.validation.usernameMinLength')
    return false
  }

  if (form.username.length > 20) {
    errors.username = t('auth.validation.usernameMaxLength')
    return false
  }

  if (!/^[a-zA-Z0-9_-]+$/.test(form.username)) {
    errors.username = t('auth.validation.usernameInvalid')
    return false
  }

  errors.username = ''
  return true
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

// Validate password
const validatePassword = (): boolean => {
  if (!form.password) {
    errors.password = t('auth.validation.passwordRequired')
    return false
  }

  if (form.password.length < 8) {
    errors.password = t('auth.validation.passwordMinLength')
    return false
  }

  if (!/[a-zA-Z]/.test(form.password) || !/[0-9]/.test(form.password)) {
    errors.password = t('auth.validation.passwordWeak')
    return false
  }

  errors.password = ''
  return true
}

// Validate confirm password
const validateConfirmPassword = (): boolean => {
  if (!form.confirmPassword) {
    errors.confirmPassword = t('auth.validation.confirmPasswordRequired')
    return false
  }

  if (form.password !== form.confirmPassword) {
    errors.confirmPassword = t('auth.validation.passwordMismatch')
    return false
  }

  errors.confirmPassword = ''
  return true
}

// Validate terms
const validateTerms = (): boolean => {
  if (!form.agreeToTerms) {
    errors.terms = t('auth.validation.termsRequired')
    return false
  }

  errors.terms = ''
  return true
}

// Handle password input for real-time strength calculation
const handlePasswordInput = () => {
  clearError('password')
  if (form.confirmPassword) {
    validateConfirmPassword()
  }
}

// Handle registration
const handleRegister = async () => {
  // Clear previous errors
  apiError.value = ''

  // Validate all fields
  const isUsernameValid = validateUsername()
  const isEmailValid = validateEmail()
  const isPasswordValid = validatePassword()
  const isConfirmPasswordValid = validateConfirmPassword()
  const isTermsValid = validateTerms()

  if (!isUsernameValid || !isEmailValid || !isPasswordValid || !isConfirmPasswordValid || !isTermsValid) {
    return
  }

  // Attempt registration
  loading.value = true

  try {
    const result = await authStore.register({
      username: form.username,
      email: form.email,
      password: form.password
    })

    if (result.success) {
      ElMessage.success(t('auth.registerSuccess'))

      // Redirect to dashboard
      router.push('/dashboard')
    } else {
      apiError.value = result.error || t('auth.registerFailed')
    }
  } catch (error: any) {
    apiError.value = error.message || t('auth.errorOccurred')
  } finally {
    loading.value = false
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
  position: relative;
  overflow: hidden;
}

.register-container::before {
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

.register-card {
  position: relative;
  width: 100%;
  max-width: 480px;
  background: white;
  border-radius: 20px;
  padding: 48px 40px;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
  animation: slideUp 0.5s ease-out;
  max-height: 90vh;
  overflow-y: auto;
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

.register-header {
  text-align: center;
  margin-bottom: 32px;
}

.logo {
  margin-bottom: 20px;
  display: flex;
  justify-content: center;
}

.register-title {
  font-size: 28px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 8px;
}

.register-subtitle {
  color: #6b7280;
  font-size: 15px;
}

.register-form {
  margin-bottom: 24px;
}

.form-group {
  margin-bottom: 20px;
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

.password-strength {
  margin-top: 8px;
}

.strength-bar {
  height: 4px;
  background: #e5e7eb;
  border-radius: 2px;
  overflow: hidden;
  margin-bottom: 6px;
}

.strength-fill {
  height: 100%;
  transition: all 0.3s ease;
  border-radius: 2px;
}

.strength-fill.weak {
  background: #ef4444;
}

.strength-fill.medium {
  background: #f59e0b;
}

.strength-fill.strong {
  background: #10b981;
}

.strength-text {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 12px;
}

.strength-text span:first-child {
  font-weight: 600;
  text-transform: capitalize;
}

.strength-text .weak {
  color: #ef4444;
}

.strength-text .medium {
  color: #f59e0b;
}

.strength-text .strong {
  color: #10b981;
}

.strength-hint {
  color: #9ca3af;
}

.terms-group {
  margin-top: 16px;
}

.checkbox-label {
  display: flex;
  align-items: flex-start;
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
  margin-top: 2px;
}

.checkbox-label input[type="checkbox"]:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

.link {
  color: #667eea;
  text-decoration: none;
  font-weight: 500;
  transition: all 0.2s;
}

.link:hover {
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

.register-button {
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
  margin-top: 24px;
}

.register-button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(102, 126, 234, 0.5);
}

.register-button:active:not(:disabled) {
  transform: translateY(0);
}

.register-button:disabled {
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

.register-footer {
  margin-top: 24px;
  padding-top: 24px;
  border-top: 1px solid #e5e7eb;
  text-align: center;
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
  font-size: 15px;
  transition: all 0.2s;
}

.login-link:hover {
  color: #5568d3;
  text-decoration: underline;
}

/* Responsive */
@media (max-width: 480px) {
  .register-card {
    padding: 32px 24px;
  }
}

/* Dark mode */
[data-theme="dark"] .register-container {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

[data-theme="dark"] .register-card {
  background: rgba(31, 41, 55, 0.98);
  border: 1px solid rgba(102, 126, 234, 0.3);
}

[data-theme="dark"] .register-title {
  color: #f3f4f6;
}

[data-theme="dark"] .register-subtitle,
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

[data-theme="dark"] .register-footer {
  border-top-color: rgba(102, 126, 234, 0.3);
}

[data-theme="dark"] .api-error {
  background: rgba(127, 29, 29, 0.3);
  border-color: rgba(239, 68, 68, 0.3);
  color: #fca5a5;
}

[data-theme="dark"] .strength-bar {
  background: rgba(255, 255, 255, 0.1);
}
</style>