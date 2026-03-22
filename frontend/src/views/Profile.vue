<template>
  <div class="profile-page">
    <!-- Profile Header Banner -->
    <div class="profile-header">
      <div class="header-banner">
        <div class="banner-bg"></div>
        <div class="header-content">
          <div class="user-avatar-large">
            {{ userInitial }}
          </div>
          <div class="user-info">
            <h1 class="user-name">{{ user?.fullName || user?.username || 'User' }}</h1>
            <p class="user-email">{{ user?.email || '' }}</p>
            <div class="user-badges">
              <span class="badge badge-role">{{ getRoleLabel(user?.role) }}</span>
              <span class="badge badge-verified">✓ 已验证</span>
            </div>
          </div>
          <div class="user-stats">
            <div class="stat-item">
              <span class="stat-number">{{ userStats.searchCount || 0 }}</span>
              <span class="stat-label">搜索次数</span>
            </div>
            <div class="stat-item">
              <span class="stat-number">{{ userStats.favoriteCount || 0 }}</span>
              <span class="stat-label">收藏论文</span>
            </div>
            <div class="stat-item">
              <span class="stat-number">{{ userStats.viewCount || 0 }}</span>
              <span class="stat-label">浏览次数</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Main Content Grid -->
    <div class="profile-content">
      <!-- Left Column - Settings -->
      <div class="settings-column">
        <!-- Account Settings Card -->
        <div class="settings-card">
          <div class="card-header">
            <div class="header-left">
              <span class="header-icon">⚙️</span>
              <h2 class="card-title">账号设置</h2>
            </div>
            <div class="header-tabs">
              <button
                v-for="tab in settingsTabs"
                :key="tab.key"
                @click="activeTab = tab.key"
                :class="['tab-button', { active: activeTab === tab.key }]"
              >
                {{ tab.label }}
              </button>
            </div>
          </div>

          <div class="card-content">
            <!-- Profile Info Tab -->
            <div v-if="activeTab === 'profile'" class="tab-content">
              <form @submit.prevent="handleUpdateProfile" class="profile-form">
                <div class="form-section">
                  <h3 class="section-title">基本信息</h3>

                  <div class="form-grid">
                    <div class="form-group">
                      <label class="form-label">用户名</label>
                      <div class="input-wrapper disabled">
                        <span class="input-icon">👤</span>
                        <input
                          v-model="profile.username"
                          type="text"
                          disabled
                          class="form-input"
                        />
                        <span class="input-suffix">🔒</span>
                      </div>
                      <span class="form-hint">用户名无法修改</span>
                    </div>

                    <div class="form-group">
                      <label class="form-label">邮箱地址</label>
                      <div class="input-wrapper disabled">
                        <span class="input-icon">📧</span>
                        <input
                          v-model="profile.email"
                          type="email"
                          disabled
                          class="form-input"
                        />
                        <span class="input-suffix">🔒</span>
                      </div>
                      <span class="form-hint">邮箱无法修改</span>
                    </div>

                    <div class="form-group">
                      <label class="form-label">全名</label>
                      <div class="input-wrapper">
                        <span class="input-icon">📝</span>
                        <input
                          v-model="profile.fullName"
                          type="text"
                          placeholder="请输入您的全名"
                          class="form-input"
                        />
                      </div>
                    </div>

                    <div class="form-group">
                      <label class="form-label">所属机构</label>
                      <div class="input-wrapper">
                        <span class="input-icon">🏢</span>
                        <input
                          v-model="profile.affiliation"
                          type="text"
                          placeholder="请输入您的所属机构"
                          class="form-input"
                        />
                      </div>
                    </div>
                  </div>
                </div>

                <div class="form-actions">
                  <button
                    type="button"
                    @click="resetProfile"
                    class="btn btn-secondary"
                  >
                    重置
                  </button>
                  <button
                    type="submit"
                    class="btn btn-primary"
                    :disabled="loading || !hasProfileChanges"
                  >
                    <span v-if="loading" class="loading-spinner"></span>
                    <span>{{ loading ? '保存中...' : '保存更改' }}</span>
                  </button>
                </div>
              </form>
            </div>

            <!-- Password Tab -->
            <div v-if="activeTab === 'password'" class="tab-content">
              <form @submit.prevent="handleChangePassword" class="password-form">
                <div class="form-section">
                  <h3 class="section-title">修改密码</h3>
                  <p class="section-description">为了您的账号安全，请定期更换密码</p>

                  <div class="form-group">
                    <label class="form-label">当前密码</label>
                    <div class="input-wrapper">
                      <span class="input-icon">🔑</span>
                      <input
                        v-model="passwordForm.currentPassword"
                        :type="showCurrentPassword ? 'text' : 'password'"
                        placeholder="请输入当前密码"
                        class="form-input"
                      />
                      <button
                        type="button"
                        @click="showCurrentPassword = !showCurrentPassword"
                        class="input-toggle"
                      >
                        {{ showCurrentPassword ? '👁️' : '👁️‍🗨️' }}
                      </button>
                    </div>
                  </div>

                  <div class="form-group">
                    <label class="form-label">新密码</label>
                    <div class="input-wrapper">
                      <span class="input-icon">🔐</span>
                      <input
                        v-model="passwordForm.newPassword"
                        :type="showNewPassword ? 'text' : 'password'"
                        placeholder="请输入新密码（至少8位）"
                        class="form-input"
                      />
                      <button
                        type="button"
                        @click="showNewPassword = !showNewPassword"
                        class="input-toggle"
                      >
                        {{ showNewPassword ? '👁️' : '👁️‍🗨️' }}
                      </button>
                    </div>
                    <div class="password-strength">
                      <div class="strength-bar">
                        <div
                          class="strength-fill"
                          :class="passwordStrength.level"
                          :style="{ width: passwordStrength.percent + '%' }"
                        ></div>
                      </div>
                      <span class="strength-text">{{ passwordStrength.text }}</span>
                    </div>
                  </div>

                  <div class="form-group">
                    <label class="form-label">确认新密码</label>
                    <div class="input-wrapper">
                      <span class="input-icon">✓</span>
                      <input
                        v-model="passwordForm.confirmPassword"
                        :type="showConfirmPassword ? 'text' : 'password'"
                        placeholder="请再次输入新密码"
                        class="form-input"
                        :class="{ 'input-error': passwordForm.confirmPassword && passwordForm.newPassword !== passwordForm.confirmPassword }"
                      />
                      <button
                        type="button"
                        @click="showConfirmPassword = !showConfirmPassword"
                        class="input-toggle"
                      >
                        {{ showConfirmPassword ? '👁️' : '👁️‍🗨️' }}
                      </button>
                    </div>
                    <span v-if="passwordForm.confirmPassword && passwordForm.newPassword !== passwordForm.confirmPassword" class="form-error">
                      两次输入的密码不一致
                    </span>
                  </div>
                </div>

                <div class="form-actions">
                  <button
                    type="button"
                    @click="resetPassword"
                    class="btn btn-secondary"
                  >
                    重置
                  </button>
                  <button
                    type="submit"
                    class="btn btn-primary"
                    :disabled="passwordLoading || !isPasswordFormValid"
                  >
                    <span v-if="passwordLoading" class="loading-spinner"></span>
                    <span>{{ passwordLoading ? '修改中...' : '修改密码' }}</span>
                  </button>
                </div>
              </form>
            </div>

            <!-- Notifications Tab -->
            <div v-if="activeTab === 'notifications'" class="tab-content">
              <div class="notifications-section">
                <h3 class="section-title">通知设置</h3>
                <p class="section-description">选择您希望接收的通知类型</p>

                <div class="notification-items">
                  <div class="notification-item">
                    <div class="item-info">
                      <span class="item-icon">📧</span>
                      <div class="item-text">
                        <span class="item-title">邮件通知</span>
                        <span class="item-description">接收重要更新和提醒</span>
                      </div>
                    </div>
                    <label class="toggle-switch">
                      <input type="checkbox" v-model="notifications.email" />
                      <span class="toggle-slider"></span>
                    </label>
                  </div>

                  <div class="notification-item">
                    <div class="item-info">
                      <span class="item-icon">🔔</span>
                      <div class="item-text">
                        <span class="item-title">搜索提醒</span>
                        <span class="item-description">新论文匹配您的搜索条件时通知</span>
                      </div>
                    </div>
                    <label class="toggle-switch">
                      <input type="checkbox" v-model="notifications.searchAlerts" />
                      <span class="toggle-slider"></span>
                    </label>
                  </div>

                  <div class="notification-item">
                    <div class="item-info">
                      <span class="item-icon">📊</span>
                      <div class="item-text">
                        <span class="item-title">统计报告</span>
                        <span class="item-description">每周发送使用统计报告</span>
                      </div>
                    </div>
                    <label class="toggle-switch">
                      <input type="checkbox" v-model="notifications.weeklyReport" />
                      <span class="toggle-slider"></span>
                    </label>
                  </div>

                  <div class="notification-item">
                    <div class="item-info">
                      <span class="item-icon">🎓</span>
                      <div class="item-text">
                        <span class="item-title">学术动态</span>
                        <span class="item-description">顶刊新发表论文推荐</span>
                      </div>
                    </div>
                    <label class="toggle-switch">
                      <input type="checkbox" v-model="notifications.academicUpdates" />
                      <span class="toggle-slider"></span>
                    </label>
                  </div>
                </div>

                <div class="form-actions">
                  <button
                    @click="saveNotifications"
                    class="btn btn-primary"
                    :disabled="notificationLoading"
                  >
                    <span v-if="notificationLoading" class="loading-spinner"></span>
                    <span>{{ notificationLoading ? '保存中...' : '保存设置' }}</span>
                  </button>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Right Column - Activity & Stats -->
      <div class="activity-column">
        <!-- Account Info Card -->
        <div class="info-card">
          <div class="info-header">
            <span class="info-icon">ℹ️</span>
            <h3 class="info-title">账号信息</h3>
          </div>
          <div class="info-content">
            <div class="info-row">
              <span class="info-label">注册时间</span>
              <span class="info-value">{{ formatDate(user?.createdAt) }}</span>
            </div>
            <div class="info-row">
              <span class="info-label">最后登录</span>
              <span class="info-value">{{ formatRelativeTime(user?.lastLoginAt) }}</span>
            </div>
            <div class="info-row">
              <span class="info-label">账号状态</span>
              <span class="info-value status-active">活跃</span>
            </div>
          </div>
        </div>

        <!-- Recent Activity Card -->
        <div class="activity-card">
          <div class="card-header">
            <span class="header-icon">🕐</span>
            <h3 class="card-title">最近活动</h3>
          </div>
          <div class="activity-list">
            <div
              v-for="(activity, index) in recentActivities"
              :key="index"
              class="activity-item"
            >
              <span class="activity-icon">{{ activity.icon }}</span>
              <div class="activity-content">
                <span class="activity-text">{{ activity.text }}</span>
                <span class="activity-time">{{ formatRelativeTime(activity.time) }}</span>
              </div>
            </div>
            <div v-if="recentActivities.length === 0" class="empty-state">
              <span class="empty-icon">📭</span>
              <span class="empty-text">暂无活动记录</span>
            </div>
          </div>
        </div>

        <!-- Quick Actions Card -->
        <div class="actions-card">
          <div class="card-header">
            <span class="header-icon">⚡</span>
            <h3 class="card-title">快捷操作</h3>
          </div>
          <div class="actions-grid">
            <button @click="router.push('/')" class="action-btn">
              <span class="action-icon">🔍</span>
              <span class="action-text">搜索论文</span>
            </button>
            <button @click="router.push('/stats')" class="action-btn">
              <span class="action-icon">📊</span>
              <span class="action-text">查看统计</span>
            </button>
            <button @click="exportData" class="action-btn">
              <span class="action-icon">📥</span>
              <span class="action-text">导出数据</span>
            </button>
            <button @click="router.push('/help')" class="action-btn">
              <span class="action-icon">❓</span>
              <span class="action-text">帮助中心</span>
            </button>
          </div>
        </div>
      </div>
    </div>

    <!-- Success/Error Message -->
    <transition name="fade">
      <div v-if="message" :class="['toast-message', messageType]">
        <span class="toast-icon">{{ messageType === 'success' ? '✓' : '✕' }}</span>
        <span class="toast-text">{{ message }}</span>
        <button @click="message = ''" class="toast-close">✕</button>
      </div>
    </transition>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { useAuthStore } from '@/stores/auth'

const router = useRouter()
const { t } = useI18n()
const authStore = useAuthStore()

const user = ref<any>(null)
const loading = ref(false)
const passwordLoading = ref(false)
const notificationLoading = ref(false)
const message = ref('')
const messageType = ref<'success' | 'error'>('success')
const activeTab = ref('profile')

// Password visibility toggles
const showCurrentPassword = ref(false)
const showNewPassword = ref(false)
const showConfirmPassword = ref(false)

// Settings tabs
const settingsTabs = [
  { key: 'profile', label: '个人信息' },
  { key: 'password', label: '密码安全' },
  { key: 'notifications', label: '通知设置' }
]

// User stats (mock data)
const userStats = ref({
  searchCount: 156,
  favoriteCount: 23,
  viewCount: 892
})

// Profile form
const originalProfile = reactive({
  username: '',
  email: '',
  fullName: '',
  affiliation: ''
})

const profile = reactive({
  username: '',
  email: '',
  fullName: '',
  affiliation: ''
})

// Password form
const passwordForm = reactive({
  currentPassword: '',
  newPassword: '',
  confirmPassword: ''
})

// Notifications
const notifications = reactive({
  email: true,
  searchAlerts: false,
  weeklyReport: true,
  academicUpdates: false
})

// Recent activities (mock data)
const recentActivities = ref([
  {
    icon: '🔍',
    text: '搜索了 "machine learning"',
    time: new Date(Date.now() - 5 * 60 * 1000)
  },
  {
    icon: '📄',
    text: '查看了论文 "Deep Learning for CV"',
    time: new Date(Date.now() - 30 * 60 * 1000)
  },
  {
    icon: '⭐',
    text: '收藏了 "Attention Mechanisms"',
    time: new Date(Date.now() - 2 * 60 * 60 * 1000)
  },
  {
    icon: '📊',
    text: '查看了统计数据',
    time: new Date(Date.now() - 5 * 60 * 60 * 1000)
  }
])

// Computed properties
const userInitial = computed(() => {
  const fullName = user.value?.fullName || user.value?.username || ''
  return fullName.charAt(0).toUpperCase()
})

const hasProfileChanges = computed(() => {
  return profile.fullName !== originalProfile.fullName ||
         profile.affiliation !== originalProfile.affiliation
})

const passwordStrength = computed(() => {
  const password = passwordForm.newPassword
  if (!password) return { level: '', percent: 0, text: '' }

  let score = 0
  if (password.length >= 8) score++
  if (password.length >= 12) score++
  if (/[a-z]/.test(password)) score++
  if (/[A-Z]/.test(password)) score++
  if (/[0-9]/.test(password)) score++
  if (/[^a-zA-Z0-9]/.test(password)) score++

  if (score <= 2) return { level: 'weak', percent: 33, text: '弱' }
  if (score <= 4) return { level: 'medium', percent: 66, text: '中等' }
  return { level: 'strong', percent: 100, text: '强' }
})

const isPasswordFormValid = computed(() => {
  return passwordForm.currentPassword &&
         passwordForm.newPassword &&
         passwordForm.confirmPassword &&
         passwordForm.newPassword === passwordForm.confirmPassword &&
         passwordForm.newPassword.length >= 8
})

// Methods
const getRoleLabel = (role: string) => {
  const roles: Record<string, string> = {
    'user': '普通用户',
    'admin': '管理员',
    'premium': '高级用户'
  }
  return roles[role] || '用户'
}

const formatDate = (date: string | undefined) => {
  if (!date) return '未知'
  return new Date(date).toLocaleDateString('zh-CN', {
    year: 'numeric',
    month: 'long',
    day: 'numeric'
  })
}

const formatRelativeTime = (date: string | Date | undefined) => {
  if (!date) return '未知'
  const now = new Date()
  const target = new Date(date)
  const diff = now.getTime() - target.getTime()

  const minutes = Math.floor(diff / 60000)
  const hours = Math.floor(diff / 3600000)
  const days = Math.floor(diff / 86400000)

  if (minutes < 1) return '刚刚'
  if (minutes < 60) return `${minutes} 分钟前`
  if (hours < 24) return `${hours} 小时前`
  if (days < 7) return `${days} 天前`
  return formatDate(date)
}

const handleUpdateProfile = async () => {
  if (!hasProfileChanges.value) return

  loading.value = true
  message.value = ''

  try {
    // TODO: Implement API call
    await new Promise(resolve => setTimeout(resolve, 1000))

    // Update original profile
    Object.assign(originalProfile, profile)

    message.value = '个人信息更新成功！'
    messageType.value = 'success'
  } catch (error: any) {
    message.value = error.message || '更新失败，请稍后重试'
    messageType.value = 'error'
  } finally {
    loading.value = false
  }
}

const handleChangePassword = async () => {
  if (!isPasswordFormValid.value) return

  passwordLoading.value = true
  message.value = ''

  try {
    // TODO: Implement API call
    await new Promise(resolve => setTimeout(resolve, 1500))

    message.value = '密码修改成功！请重新登录'
    messageType.value = 'success'

    // Clear form
    passwordForm.currentPassword = ''
    passwordForm.newPassword = ''
    passwordForm.confirmPassword = ''
  } catch (error: any) {
    message.value = error.message || '密码修改失败'
    messageType.value = 'error'
  } finally {
    passwordLoading.value = false
  }
}

const saveNotifications = async () => {
  notificationLoading.value = true

  try {
    // TODO: Implement API call
    await new Promise(resolve => setTimeout(resolve, 800))

    message.value = '通知设置已保存！'
    messageType.value = 'success'
  } catch (error: any) {
    message.value = error.message || '保存失败'
    messageType.value = 'error'
  } finally {
    notificationLoading.value = false
  }
}

const resetProfile = () => {
  profile.fullName = originalProfile.fullName
  profile.affiliation = originalProfile.affiliation
}

const resetPassword = () => {
  passwordForm.currentPassword = ''
  passwordForm.newPassword = ''
  passwordForm.confirmPassword = ''
}

const exportData = () => {
  message.value = '数据导出功能开发中...'
  messageType.value = 'success'
  setTimeout(() => message.value = '', 3000)
}

onMounted(() => {
  if (authStore.user) {
    user.value = authStore.user

    // Initialize profile
    profile.username = user.value.username || ''
    profile.email = user.value.email || ''
    profile.fullName = user.value.fullName || ''
    profile.affiliation = user.value.affiliation || ''

    // Store original for change detection
    Object.assign(originalProfile, profile)
  }
})
</script>

<style scoped>
/* ===================================
   PROFILE PAGE LAYOUT
   =================================== */
.profile-page {
  min-height: 100vh;
  background: linear-gradient(135deg, #f5f7fa 0%, #e4e9f2 100%);
  padding-bottom: 40px;
}

/* ===================================
   PROFILE HEADER BANNER
   =================================== */
.profile-header {
  margin-bottom: 32px;
}

.header-banner {
  position: relative;
  background: white;
  border-radius: 16px;
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.08);
  overflow: hidden;
}

.banner-bg {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 120px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  opacity: 0.9;
}

.header-content {
  position: relative;
  padding: 24px 32px 32px;
  display: flex;
  align-items: flex-end;
  gap: 24px;
}

.user-avatar-large {
  width: 100px;
  height: 100px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border-radius: 50%;
  font-size: 48px;
  font-weight: 700;
  box-shadow: 0 8px 32px rgba(102, 126, 234, 0.4);
  border: 4px solid white;
}

.user-info {
  flex: 1;
  padding-bottom: 8px;
}

.user-name {
  margin: 0 0 8px 0;
  font-size: 32px;
  font-weight: 700;
  color: #1f2937;
}

.user-email {
  margin: 0 0 12px 0;
  font-size: 16px;
  color: #6b7280;
}

.user-badges {
  display: flex;
  gap: 8px;
}

.badge {
  padding: 4px 12px;
  border-radius: 16px;
  font-size: 13px;
  font-weight: 600;
}

.badge-role {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.badge-verified {
  background: #d1fae5;
  color: #065f46;
}

.user-stats {
  display: flex;
  gap: 32px;
  padding-bottom: 8px;
}

.stat-item {
  text-align: center;
}

.stat-number {
  display: block;
  font-size: 24px;
  font-weight: 700;
  color: #667eea;
}

.stat-label {
  font-size: 13px;
  color: #6b7280;
}

/* ===================================
   MAIN CONTENT GRID
   =================================== */
.profile-content {
  display: grid;
  grid-template-columns: 2fr 1fr;
  gap: 24px;
  max-width: 1400px;
  margin: 0 auto;
  padding: 0 24px;
}

@media (max-width: 1024px) {
  .profile-content {
    grid-template-columns: 1fr;
  }
}

/* ===================================
   SETTINGS COLUMN (LEFT)
   =================================== */
.settings-card {
  background: white;
  border-radius: 16px;
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.08);
  overflow: hidden;
}

.card-header {
  padding: 24px 32px;
  border-bottom: 1px solid #e5e7eb;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 12px;
}

.header-icon {
  font-size: 24px;
}

.card-title {
  margin: 0;
  font-size: 20px;
  font-weight: 700;
  color: #1f2937;
}

.header-tabs {
  display: flex;
  gap: 8px;
}

.tab-button {
  padding: 8px 16px;
  border: none;
  background: transparent;
  color: #6b7280;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s;
  font-weight: 600;
  font-size: 14px;
}

.tab-button:hover {
  background: #f3f4f6;
}

.tab-button.active {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.card-content {
  padding: 32px;
}

.tab-content {
  animation: fadeIn 0.3s ease;
}

@keyframes fadeIn {
  from { opacity: 0; transform: translateY(10px); }
  to { opacity: 1; transform: translateY(0); }
}

/* ===================================
   FORM STYLES
   =================================== */
.profile-form,
.password-form {
  display: flex;
  flex-direction: column;
  gap: 32px;
}

.form-section {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.section-title {
  margin: 0;
  font-size: 18px;
  font-weight: 700;
  color: #1f2937;
}

.section-description {
  margin: 0;
  font-size: 14px;
  color: #6b7280;
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 20px;
}

.form-group {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.form-label {
  font-size: 14px;
  font-weight: 600;
  color: #374151;
}

.input-wrapper {
  position: relative;
  display: flex;
  align-items: center;
  background: #f9fafb;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  transition: all 0.2s;
}

.input-wrapper:focus-within {
  border-color: #667eea;
  background: white;
  box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
}

.input-wrapper.disabled {
  background: #f3f4f6;
  cursor: not-allowed;
}

.input-icon {
  padding: 0 12px;
  font-size: 16px;
}

.form-input {
  flex: 1;
  border: none;
  background: transparent;
  padding: 12px 8px;
  font-size: 14px;
  color: #1f2937;
  outline: none;
}

.form-input:disabled {
  cursor: not-allowed;
  color: #9ca3af;
}

.form-input.input-error {
  border-color: #ef4444;
}

.input-suffix {
  padding: 0 12px;
  font-size: 14px;
  color: #9ca3af;
}

.input-toggle {
  padding: 0 12px;
  background: none;
  border: none;
  cursor: pointer;
  font-size: 16px;
  transition: transform 0.2s;
}

.input-toggle:hover {
  transform: scale(1.1);
}

.form-hint {
  font-size: 12px;
  color: #9ca3af;
}

.form-error {
  font-size: 12px;
  color: #ef4444;
}

/* Password Strength */
.password-strength {
  display: flex;
  align-items: center;
  gap: 12px;
}

.strength-bar {
  flex: 1;
  height: 4px;
  background: #e5e7eb;
  border-radius: 2px;
  overflow: hidden;
}

.strength-fill {
  height: 100%;
  transition: all 0.3s;
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
  font-weight: 600;
}

.strength-fill.weak + .strength-text {
  color: #ef4444;
}

.strength-fill.medium + .strength-text {
  color: #f59e0b;
}

.strength-fill.strong + .strength-text {
  color: #10b981;
}

/* Form Actions */
.form-actions {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
  padding-top: 16px;
  border-top: 1px solid #e5e7eb;
}

.btn {
  padding: 12px 24px;
  border: none;
  border-radius: 8px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s;
  display: flex;
  align-items: center;
  gap: 8px;
}

.btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.btn-secondary {
  background: #f3f4f6;
  color: #374151;
}

.btn-secondary:hover:not(:disabled) {
  background: #e5e7eb;
}

.btn-primary {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.btn-primary:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(102, 126, 234, 0.4);
}

.loading-spinner {
  width: 14px;
  height: 14px;
  border: 2px solid white;
  border-top-color: transparent;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

/* ===================================
   NOTIFICATIONS TOGGLES
   =================================== */
.notification-items {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.notification-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  background: #f9fafb;
  border-radius: 12px;
  transition: all 0.2s;
}

.notification-item:hover {
  background: #f3f4f6;
}

.item-info {
  display: flex;
  align-items: center;
  gap: 12px;
  flex: 1;
}

.item-icon {
  font-size: 24px;
}

.item-text {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.item-title {
  font-size: 14px;
  font-weight: 600;
  color: #1f2937;
}

.item-description {
  font-size: 13px;
  color: #6b7280;
}

.toggle-switch {
  position: relative;
  width: 48px;
  height: 26px;
}

.toggle-switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

.toggle-slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #d1d5db;
  transition: 0.3s;
  border-radius: 26px;
}

.toggle-slider:before {
  position: absolute;
  content: "";
  height: 20px;
  width: 20px;
  left: 3px;
  bottom: 3px;
  background-color: white;
  transition: 0.3s;
  border-radius: 50%;
}

.toggle-switch input:checked + .toggle-slider {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

.toggle-switch input:checked + .toggle-slider:before {
  transform: translateX(22px);
}

/* ===================================
   ACTIVITY COLUMN (RIGHT)
   =================================== */
.info-card,
.activity-card,
.actions-card {
  background: white;
  border-radius: 16px;
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.08);
  padding: 24px;
  margin-bottom: 24px;
}

.info-header,
.activity-card .card-header,
.actions-card .card-header {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 20px;
}

.info-icon,
.activity-card .header-icon,
.actions-card .header-icon {
  font-size: 20px;
}

.info-title,
.activity-card .card-title,
.actions-card .card-title {
  margin: 0;
  font-size: 16px;
  font-weight: 700;
  color: #1f2937;
}

/* Info Card */
.info-content {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.info-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 0;
  border-bottom: 1px solid #f3f4f6;
}

.info-row:last-child {
  border-bottom: none;
}

.info-label {
  font-size: 14px;
  color: #6b7280;
}

.info-value {
  font-size: 14px;
  font-weight: 600;
  color: #1f2937;
}

.status-active {
  color: #10b981;
}

/* Activity List */
.activity-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.activity-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px;
  background: #f9fafb;
  border-radius: 8px;
  transition: all 0.2s;
}

.activity-item:hover {
  background: #f3f4f6;
}

.activity-icon {
  font-size: 20px;
}

.activity-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.activity-text {
  font-size: 14px;
  color: #1f2937;
}

.activity-time {
  font-size: 12px;
  color: #9ca3af;
}

.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  padding: 32px;
  text-align: center;
}

.empty-icon {
  font-size: 48px;
  opacity: 0.5;
}

.empty-text {
  font-size: 14px;
  color: #9ca3af;
}

/* Actions Grid */
.actions-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
}

.action-btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  padding: 16px;
  background: #f9fafb;
  border: 2px solid transparent;
  border-radius: 12px;
  cursor: pointer;
  transition: all 0.2s;
}

.action-btn:hover {
  background: white;
  border-color: #667eea;
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.2);
  transform: translateY(-2px);
}

.action-icon {
  font-size: 24px;
}

.action-text {
  font-size: 13px;
  font-weight: 600;
  color: #1f2937;
}

/* ===================================
   TOAST MESSAGE
   =================================== */
.toast-message {
  position: fixed;
  top: 24px;
  right: 24px;
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 16px 20px;
  border-radius: 12px;
  box-shadow: 0 10px 40px rgba(0, 0, 0, 0.2);
  z-index: 9999;
  animation: slideIn 0.3s ease;
}

@keyframes slideIn {
  from {
    transform: translateX(400px);
    opacity: 0;
  }
  to {
    transform: translateX(0);
    opacity: 1;
  }
}

.toast-message.success {
  background: #d1fae5;
  color: #065f46;
  border-left: 4px solid #10b981;
}

.toast-message.error {
  background: #fee2e2;
  color: #991b1b;
  border-left: 4px solid #ef4444;
}

.toast-icon {
  font-size: 18px;
  font-weight: 700;
}

.toast-text {
  flex: 1;
  font-weight: 600;
}

.toast-close {
  background: none;
  border: none;
  cursor: pointer;
  font-size: 16px;
  padding: 4px;
}

/* Fade transition */
.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

/* ===================================
   RESPONSIVE DESIGN
   =================================== */
@media (max-width: 768px) {
  .header-content {
    flex-direction: column;
    align-items: center;
    text-align: center;
  }

  .user-stats {
    margin-top: 16px;
  }

  .profile-content {
    padding: 0 16px;
  }

  .card-header {
    flex-direction: column;
    align-items: flex-start;
    gap: 16px;
  }

  .header-tabs {
    width: 100%;
    justify-content: space-between;
  }

  .tab-button {
    flex: 1;
    padding: 10px;
    text-align: center;
  }

  .form-grid {
    grid-template-columns: 1fr;
  }

  .form-actions {
    flex-direction: column;
  }

  .form-actions .btn {
    width: 100%;
    justify-content: center;
  }
}
</style>
