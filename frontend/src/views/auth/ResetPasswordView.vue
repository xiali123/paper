<template>
  <div class="reset-password-container">
    <div class="reset-password-card">
      <!-- Header -->
      <div class="reset-password-header">
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
        <h1 class="reset-password-title">{{ t('auth.resetPassword') }}</h1>
        <p class="reset-password-subtitle">{{ t('auth.resetPasswordSubtitle') }}</p>
      </div>

      <!-- Success State -->
      <div v-if="resetSuccess" class="success-state">
        <div class="success-icon">
          <svg width="64" height="64" viewBox="0 0 64 64" fill="none">
            <circle cx="32" cy="32" r="32" fill="#10b981" opacity="0.1"/>
            <circle cx="32" cy="32" r="24" fill="#10b981" opacity="0.2"/>
            <path d="M32 40L22 32L24.5 29.5L31 36L43 24L45 26L32 40Z" fill="#10b981"/>
          </svg>
        </div>
        <h2 class="success-title">{{ t('auth.passwordResetSuccess') }}</h2>
        <p class="success-message">{{ t('auth.passwordResetSuccessMessage') }}</p>

        <button class="login-button" @click="goToLogin">
          {{ t('auth.goToLogin') }}
        </button>
      </div>

      <!-- Form State -->
      <form v-else @submit.prevent="handleSubmit" class="reset-password-form" novalidate>
        <!-- Password Field -->
        <div class="form-group" :class="{ 'has-error': errors.password }">
          <label for="password">
            {{ t('auth.newPassword') }}
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
              :placeholder="t('auth.newPasswordPlaceholder')"
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
            </div>
          </div>

          <!-- Password Requirements -->
          <div class="password-requirements">
            <div class="requirement" :class="{ 'met': hasMinLength }">
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path v-if="hasMinLength" fill="#10b981" d="M13.354 3.646a.5.5 0 010 .708l-7 7a.5.5 0 01-.708 0l-3.5-3.5a.5.5 0 11.708-.708L6 10.293l6.646-6.647a.5.5 0 01.708 0z"/>
                <circle v-else cx="8" cy="8" r="6" fill="none" stroke="#9ca3af" stroke-width="1"/>
              </svg>
              <span>{{ t('auth.passwordRequirements.minLength') }}</span>
            </div>
            <div class="requirement" :class="{ 'met': hasUppercase }">
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path v-if="hasUppercase" fill="#10b981" d="M13.354 3.646a.5.5 0 010 .708l-7 7a.5.5 0 01-.708 0l-3.5-3.5a.5.5 0 11.708-.708L6 10.293l6.646-6.647a.5.5 0 01.708 0z"/>
                <circle v-else cx="8" cy="8" r="6" fill="none" stroke="#9ca3af" stroke-width="1"/>
              </svg>
              <span>{{ t('auth.passwordRequirements.uppercase') }}</span>
            </div>
            <div class="requirement" :class="{ 'met': hasLowercase }">
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path v-if="hasLowercase" fill="#10b981" d="M13.354 3.646a.5.5 0 010 .708l-7 7a.5.5 0 01-.708 0l-3.5-3.5a.5.5 0 11.708-.708L6 10.293l6.646-6.647a.5.5 0 01.708 0z"/>
                <circle v-else cx="8" cy="8" r="6" fill="none" stroke="#9ca3af" stroke-width="1"/>
              </svg>
              <span>{{ t('auth.passwordRequirements.lowercase') }}</span>
            </div>
            <div class="requirement" :class="{ 'met': hasNumber }">
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path v-if="hasNumber" fill="#10b981" d="M13.354 3.646a.5.5 0 010 .708l-7 7a.5.5 0 01-.708 0l-3.5-3.5a.5.5 0 11.708-.708L6 10.293l6.646-6.647a.5.5 0 01.708 0z"/>
                <circle v-else cx="8" cy="8" r="6" fill="none" stroke="#9ca3af" stroke-width="1"/>
              </svg>
              <span>{{ t('auth.passwordRequirements.number') }}</span>
            </div>
          </div>

          <span v-if="errors.password" class="error-message">
            {{ errors.password }}
          </span>
        </div>

        <!-- Confirm Password Field -->
        <div class="form-group" :class="{ 'has-error': errors.confirmPassword }">
          <label for="confirmPassword">
            {{ t('auth.confirmNewPassword') }}
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
              :placeholder="t('auth.confirmNewPasswordPlaceholder')"
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
          <span v-else>{{ t('auth.resetPasswordButton') }}</span>
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
import { ref, reactive, computed } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { ElMessage } from 'element-plus'
import { useAuthStore } from '@/stores'

const { t } = useI18n()
const router = useRouter()
const route = useRoute()
const authStore = useAuthStore()

// Refs
const showPassword = ref(false)
const showConfirmPassword = ref(false)
const loading = ref(false)
const resetSuccess = ref(false)
const apiError = ref('')

// Get reset token from URL
const resetToken = computed(() => route.query.token as string || '')

// Form data
const form = reactive({
  password: '',
  confirmPassword: ''
})

// Validation errors
const errors = reactive({
  password: '',
  confirmPassword: ''
})

// Password requirement checks
const hasMinLength = computed(() => form.password.length >= 8)
const hasUppercase = computed(() => /[A-Z]/.test(form.password))
const hasLowercase = computed(() => /[a-z]/.test(form.password))
const hasNumber = computed(() => /[0-9]/.test(form.password))

// Password strength calculation
const passwordStrength = computed(() => {
  const password = form.password
  if (!password) return { level: 'weak', percentage: 0, score: 0 }

  let score = 0
  if (hasMinLength.value) score += 1
  if (hasUppercase.value) score += 1
  if (hasLowercase.value) score += 1
  if (hasNumber.value) score += 1
  if (/[^a-zA-Z0-9]/.test(password)) score += 1

  let level = 'weak'
  let percentage = 0

  if (score <= 2) {
    level = 'weak'
    percentage = 33
  } else if (score <= 3) {
    level = 'medium'
    percentage = 66
  } else {
    level = 'strong'
    percentage = 100
  }

  return { level, percentage, score }
})

const strength = computed(() => passwordStrength.value)

// Clear specific error
const clearError = (field: 'password' | 'confirmPassword') => {
  errors[field] = ''
  if (apiError.value) apiError.value = ''
}

// Validate password
const validatePassword = (): boolean => {
  if (!form.password) {
    errors.password = t('auth.validation.passwordRequired')
    return false
  }

  if (!hasMinLength.value || !hasUppercase.value || !hasLowercase.value || !hasNumber.value) {
    errors.password = t('auth.validation.passwordRequirements')
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

// Handle password input
const handlePasswordInput = () => {
  clearError('password')
  if (form.confirmPassword) {
    validateConfirmPassword()
  }
}

// Handle form submission
const handleSubmit = async () => {
  // Clear previous errors
  apiError.value = ''

  // Check if token exists
  if (!resetToken.value) {
    apiError.value = t('auth.invalidResetToken')
    return
  }

  // Validate form
  const isPasswordValid = validatePassword()
  const isConfirmPasswordValid = validateConfirmPassword()

  if (!isPasswordValid || !isConfirmPasswordValid) {
    return
  }

  // Reset password
  loading.value = true

  try {
    const result = await authStore.resetPassword(resetToken.value, form.password)

    if (result.success) {
      resetSuccess.value = true
      ElMessage.success(t('auth.passwordResetSuccess'))
    } else {
      apiError.value = result.error || t('auth.passwordResetFailed')
    }
  } catch (error: any) {
    apiError.value = error.message || t('auth.errorOccurred')
  } finally {
    loading.value = false
  }
}

// Go to login
const goToLogin = () => {
  router.push('/auth/login')
}
</script>

<style scoped>
.reset-password-container {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
  position: relative;
  overflow: hidden;
}

.reset-password-container::before {
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

.reset-password-card {
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

.reset-password-header {
  text-align: center;
  margin-bottom: 40px;
}

.icon-wrapper {
  margin-bottom: 20px;
  display: flex;
  justify-content: center;
}

.reset-password-title {
  font-size: 28px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 8px;
}

.reset-password-subtitle {
  color: #6b7280;
  font-size: 15px;
  line-height: 1.5;
}

.reset-password-form {
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
  margin-bottom: 12px;
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
  font-size: 12px;
}

.strength-text span {
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

.password-requirements {
  margin-top: 12px;
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px;
}

.requirement {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  color: #6b7280;
}

.requirement.met {
  color: #10b981;
}

.requirement.met svg {
  color: #10b981;
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
  margin-bottom: 20px;
}

.submit-button:hover:not(:disabled),
.login-button:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(102, 126, 234, 0.5);
}

.submit-button:active:not(:disabled),
.login-button:active {
  transform: translateY(0);
}

.submit-button:disabled {
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
  .reset-password-card {
    padding: 32px 24px;
  }

  .password-requirements {
    grid-template-columns: 1fr;
  }
}

/* Dark mode */
[data-theme="dark"] .reset-password-container {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

[data-theme="dark"] .reset-password-card {
  background: rgba(31, 41, 55, 0.98);
  border: 1px solid rgba(102, 126, 234, 0.3);
}

[data-theme="dark"] .reset-password-title,
[data-theme="dark"] .success-title {
  color: #f3f4f6;
}

[data-theme="dark"] .reset-password-subtitle,
[data-theme="dark"] .success-message {
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

[data-theme="dark"] .requirement {
  color: #9ca3af;
}

[data-theme="dark"] .requirement.met {
  color: #34d399;
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