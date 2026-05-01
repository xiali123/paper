<template>
  <div class="profile-page" role="main" aria-label="个人资料">
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
              <el-icon class="header-icon" :size="24"><Setting /></el-icon>
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
                        <el-icon class="input-icon"><User /></el-icon>
                        <input
                          v-model="profile.username"
                          type="text"
                          disabled
                          class="form-input"
                        />
                        <el-icon class="input-suffix"><Lock /></el-icon>
                      </div>
                      <span class="form-hint">用户名无法修改</span>
                    </div>

                    <div class="form-group">
                      <label class="form-label">邮箱地址</label>
                      <div class="input-wrapper disabled">
                        <el-icon class="input-icon"><Message /></el-icon>
                        <input
                          v-model="profile.email"
                          type="email"
                          disabled
                          class="form-input"
                        />
                        <el-icon class="input-suffix"><Lock /></el-icon>
                      </div>
                      <span class="form-hint">邮箱无法修改</span>
                    </div>

                    <div class="form-group">
                      <label class="form-label">全名</label>
                      <div class="input-wrapper">
                        <el-icon class="input-icon"><Edit /></el-icon>
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
                        <el-icon class="input-icon"><OfficeBuilding /></el-icon>
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
                      <el-icon class="input-icon"><Lock /></el-icon>
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
                        <el-icon><View v-if="showCurrentPassword" /><Hide v-else /></el-icon>
                      </button>
                    </div>
                  </div>

                  <div class="form-group">
                    <label class="form-label">新密码</label>
                    <div class="input-wrapper">
                      <el-icon class="input-icon"><Lock /></el-icon>
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
                        <el-icon><View v-if="showNewPassword" /><Hide v-else /></el-icon>
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
                      <el-icon class="input-icon"><Check /></el-icon>
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
                        <el-icon><View v-if="showConfirmPassword" /><Hide v-else /></el-icon>
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
                      <el-icon class="item-icon" :size="24"><Message /></el-icon>
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
                      <el-icon class="item-icon" :size="24"><Bell /></el-icon>
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
                      <el-icon class="item-icon" :size="24"><TrendCharts /></el-icon>
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
                      <el-icon class="item-icon" :size="24"><Document /></el-icon>
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
        <!-- Quick Actions Card -->
        <div class="actions-card">
          <div class="card-header">
            <el-icon class="header-icon" :size="20"><Lightning /></el-icon>
            <h3 class="card-title">快捷操作</h3>
          </div>
          <div class="actions-grid">
            <button @click="router.push('/')" class="action-btn">
              <el-icon class="action-icon"><Search /></el-icon>
              <span class="action-text">搜索论文</span>
            </button>
            <button @click="router.push('/stats')" class="action-btn">
              <el-icon class="action-icon"><DataAnalysis /></el-icon>
              <span class="action-text">查看统计</span>
            </button>
            <button @click="exportData" class="action-btn">
              <el-icon class="action-icon"><Download /></el-icon>
              <span class="action-text">导出数据</span>
            </button>
            <button @click="router.push('/help')" class="action-btn">
              <el-icon class="action-icon"><QuestionFilled /></el-icon>
              <span class="action-text">帮助中心</span>
            </button>
          </div>
        </div>

        <!-- Account Info Card -->
        <div class="info-card">
          <div class="info-header">
            <el-icon class="info-icon" :size="20"><InfoFilled /></el-icon>
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
            <el-icon class="header-icon" :size="20"><Clock /></el-icon>
            <h3 class="card-title">最近活动</h3>
          </div>
          <div class="activity-list">
            <div
              v-for="(activity, index) in recentActivities"
              :key="index"
              class="activity-item"
            >
              <component :is="getActivityIcon(activity.icon)" class="activity-icon" />
              <div class="activity-content">
                <span class="activity-text">{{ activity.text }}</span>
                <span class="activity-time">{{ formatRelativeTime(activity.time) }}</span>
              </div>
            </div>
            <div v-if="recentActivities.length === 0" class="empty-state">
              <el-icon class="empty-icon" :size="48"><Document /></el-icon>
              <span class="empty-text">暂无活动记录</span>
            </div>
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
import { useAuthStore } from '@/stores'
import {
  User,
  Message,
  Lock,
  Bell,
  Setting,
  InfoFilled,
  Clock,
  Lightning,
  Search,
  DataAnalysis,
  Download,
  QuestionFilled,
  View,
  Hide,
  Check,
  Edit,
  OfficeBuilding,
  CircleCheck,
  Document,
  Star,
  TrendCharts
} from '@element-plus/icons-vue'

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
    icon: 'Search',
    text: '搜索了 "machine learning"',
    time: new Date(Date.now() - 5 * 60 * 1000)
  },
  {
    icon: 'Document',
    text: '查看了论文 "Deep Learning for CV"',
    time: new Date(Date.now() - 30 * 60 * 1000)
  },
  {
    icon: 'Star',
    text: '收藏了 "Attention Mechanisms"',
    time: new Date(Date.now() - 2 * 60 * 60 * 1000)
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

const getActivityIcon = (iconName: string) => {
  const iconMap: Record<string, any> = {
    'Search': Search,
    'Document': Document,
    'Star': Star,
    'TrendCharts': TrendCharts,
    'DataAnalysis': DataAnalysis
  }
  return iconMap[iconName] || Document
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

<style scoped lang="scss">
// ==========================================
// 现代化Profile页面样式
// Modern Profile Page Styles
// ==========================================

.profile-page {
  min-height: 100vh;
  background: linear-gradient(135deg, $gray-50 0%, $gray-100 100%);
  padding: $spacing-6 0;
  width: 100%;

  .dark & {
    background: linear-gradient(135deg, $gray-900 0%, $gray-800 100%);
  }
}

// ==========================================
// PROFILE HEADER BANNER
// ==========================================
.profile-header {
  margin-bottom: $spacing-6;
  max-width: 1800px;
  margin-left: auto;
  margin-right: auto;
  padding: 0 $spacing-6;
}

.header-banner {
  position: relative;
  background: #ffffff;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-xl;
  overflow: hidden;
  border: 1px solid $border-light;
  transition: all $duration-slow;

  &::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 120px;
    background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
    opacity: 0.05;
    z-index: 0;
  }

  &:hover {
    box-shadow: $shadow-2xl;
    transform: translateY(-2px);
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;

    &::before {
      opacity: 0.1;
    }
  }
}

.banner-bg {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 120px;
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  opacity: 0.9;

  .dark & {
    background: linear-gradient(135deg, $primary-400 0%, $primary-500 100%);
  }
}

.header-content {
  position: relative;
  padding: $spacing-8 $spacing-8 $spacing-10;
  display: flex;
  align-items: flex-end;
  gap: $spacing-8;
}

.user-avatar-large {
  width: 100px;
  height: 100px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  color: #ffffff;
  border-radius: $border-radius-full;
  font-size: 48px;
  font-weight: $font-weight-bold;
  box-shadow: 0 8px 32px rgba($primary-500, 0.4), inset 0 2px 8px rgba(255, 255, 255, 0.2);
  border: 4px solid #ffffff;
  transition: all $duration-slow;
  position: relative;
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    top: -50%;
    left: -50%;
    width: 200%;
    height: 200%;
    background: radial-gradient(circle, rgba(255, 255, 255, 0.3) 0%, transparent 70%);
    transition: transform $duration-slow;
  }

  &:hover {
    transform: scale(1.08);
    box-shadow: 0 12px 40px rgba($primary-500, 0.5), inset 0 2px 8px rgba(255, 255, 255, 0.2);

    &::before {
      transform: translate(25%, 25%);
    }
  }

  .dark & {
    border-color: $gray-800;
  }
}

.user-info {
  flex: 1;
  padding-bottom: $spacing-2;
}

.user-name {
  margin: 0 0 $spacing-2 0;
  font-size: $font-size-3xl;
  font-weight: $font-weight-bold;
  color: $text-primary;

  .dark & {
    color: $gray-100;
  }
}

.user-email {
  margin: 0 0 $spacing-3 0;
  font-size: $font-size-base;
  color: $text-secondary;
}

.user-badges {
  display: flex;
  gap: $spacing-2;
}

.badge {
  padding: $spacing-2 $spacing-4;
  border-radius: $border-radius-full;
  font-size: $font-size-base;
  font-weight: $font-weight-semibold;
  box-shadow: $shadow-sm;
}

.badge-role {
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  color: #ffffff;
  box-shadow: 0 2px 8px rgba($primary-500, 0.3);
}

.badge-verified {
  background: linear-gradient(135deg, rgba($success-color, 0.15) 0%, rgba($success-color, 0.05) 100%);
  color: $success-color;
  border: 1px solid $success-color;

  .dark & {
    background: linear-gradient(135deg, rgba($success-color, 0.25) 0%, rgba($success-color, 0.1) 100%);
  }
}

.user-stats {
  display: flex;
  gap: $spacing-10;
  padding-bottom: $spacing-3;
}

.stat-item {
  text-align: center;
  position: relative;
  padding: $spacing-4 $spacing-6;
  border-radius: $border-radius-xl;
  background: linear-gradient(135deg, rgba($primary-50, 0.5) 0%, rgba($primary-100, 0.3) 100%);
  border: 1px solid rgba($primary-200, 0.5);
  transition: all $duration-fast;

  .dark & {
    background: linear-gradient(135deg, rgba($gray-700, 0.5) 0%, rgba($gray-800, 0.3) 100%);
    border-color: rgba($gray-600, 0.5);
  }

  &:hover {
    transform: translateY(-4px);
    box-shadow: $shadow-lg;
  }

  // 为每个统计项使用不同的颜色
  &:nth-child(1) {
    background: linear-gradient(135deg, rgba($primary-50, 0.6) 0%, rgba($primary-100, 0.4) 100%);
    border-color: rgba($primary-200, 0.6);

    .dark & {
      background: linear-gradient(135deg, rgba($primary-900, 0.4) 0%, rgba($primary-800, 0.3) 100%);
      border-color: rgba($primary-700, 0.5);
    }
  }

  &:nth-child(2) {
    background: linear-gradient(135deg, rgba($warning-color, 0.6) 0%, rgba($warning-color, 0.4) 100%);
    border-color: rgba($warning-color, 0.6);

    .dark & {
      background: linear-gradient(135deg, rgba($warning-color, 0.4) 0%, rgba($warning-color, 0.3) 100%);
      border-color: rgba($warning-color, 0.5);
    }
  }

  &:nth-child(3) {
    background: linear-gradient(135deg, rgba($success-color, 0.6) 0%, rgba($success-color, 0.4) 100%);
    border-color: rgba($success-color, 0.6);

    .dark & {
      background: linear-gradient(135deg, rgba($success-color, 0.4) 0%, rgba($success-color, 0.3) 100%);
      border-color: rgba($success-color, 0.5);
    }
  }
}

.stat-number {
  display: block;
  font-size: $font-size-3xl;
  font-weight: $font-weight-bold;
  transition: all $duration-fast;
  margin-bottom: $spacing-2;
  letter-spacing: -0.5px;

  // 为每个数字使用不同的渐变色
  .stat-item:nth-child(1) & {
    background: linear-gradient(135deg, $primary-600 0%, $primary-500 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    filter: drop-shadow(0 2px 6px rgba($primary-600, 0.4));

    .dark & {
      background: linear-gradient(135deg, $primary-400 0%, $primary-300 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
      filter: drop-shadow(0 2px 6px rgba($primary-400, 0.4));
    }
  }

  .stat-item:nth-child(2) & {
    background: linear-gradient(135deg, $warning-color 0%, $warning-color 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    filter: drop-shadow(0 2px 6px rgba($warning-color, 0.4));

    .dark & {
      background: linear-gradient(135deg, $warning-color 0%, $warning-color 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
      filter: drop-shadow(0 2px 6px rgba($warning-color, 0.4));
    }
  }

  .stat-item:nth-child(3) & {
    background: linear-gradient(135deg, $success-color 0%, $success-color 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    filter: drop-shadow(0 2px 6px rgba($success-color, 0.4));

    .dark & {
      background: linear-gradient(135deg, $success-color 0%, $success-color 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
      filter: drop-shadow(0 2px 6px rgba($success-color, 0.4));
    }
  }
}

.stat-label {
  font-size: $font-size-sm;
  color: $text-secondary;
  font-weight: $font-weight-semibold;
  text-transform: uppercase;
  letter-spacing: 0.5px;

  .dark & {
    color: $gray-400;
  }
}

// ==========================================
// MAIN CONTENT GRID
// ==========================================
.profile-content {
  display: grid;
  grid-template-columns: minmax(0, 2fr) minmax(380px, 1.2fr);
  gap: $spacing-8;
  max-width: 1800px;
  margin: 0 auto;
  padding: 0 $spacing-6;
}

@media (max-width: 1200px) {
  .profile-content {
    grid-template-columns: 1fr;
    gap: $spacing-6;
    padding: 0 $spacing-4;
  }
}

// ==========================================
// SETTINGS COLUMN (LEFT)
// ==========================================
.settings-card {
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border-radius: $border-radius-xl;
  box-shadow: $shadow-xl;
  overflow: hidden;
  border: 1px solid $border-light;
  transition: all $duration-slow;

  &:hover {
    box-shadow: $shadow-2xl;
    transform: translateY(-2px);
  }

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    border-color: $gray-700;
  }
}

.card-header {
  padding: $spacing-6 $spacing-8;
  border-bottom: 1px solid $border-light;
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: $spacing-4;
  background: linear-gradient(90deg, rgba($primary-500, 0.02) 0%, transparent 100%);

  .dark & {
    border-bottom-color: $gray-700;
    background: linear-gradient(90deg, rgba($primary-400, 0.05) 0%, transparent 100%);
  }
}

.header-left {
  display: flex;
  align-items: center;
  gap: $spacing-3;
}

.header-icon {
  color: $primary-600;

  .dark & {
    color: $primary-400;
  }
}

.card-title {
  margin: 0;
  font-size: $font-size-2xl;
  font-weight: $font-weight-bold;
  color: $text-primary;

  .dark & {
    color: $gray-100;
  }
}

.header-tabs {
  display: flex;
  gap: $spacing-2;
  flex-wrap: wrap;
}

.tab-button {
  padding: $spacing-2 $spacing-4;
  border: none;
  background: transparent;
  color: $text-secondary;
  border-radius: $border-radius-base;
  cursor: pointer;
  transition: all $duration-fast;
  font-weight: $font-weight-semibold;
  font-size: $font-size-base;

  &:hover {
    background: rgba($primary-500, 0.1);
    color: $primary-600;
  }

  .dark &:hover {
    background: rgba($primary-400, 0.1);
    color: $primary-400;
  }

  &.active {
    background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
    color: #ffffff;
  }
}

.card-content {
  padding: $spacing-8;
}

.tab-content {
  animation: fadeIn $duration-base $easing-ease-out;
}

@keyframes fadeIn {
  from { opacity: 0; transform: translateY(10px); }
  to { opacity: 1; transform: translateY(0); }
}

// ==========================================
// FORM STYLES
// ==========================================
.profile-form,
.password-form {
  display: flex;
  flex-direction: column;
  gap: $spacing-8;
}

.form-section {
  display: flex;
  flex-direction: column;
  gap: $spacing-5;
}

.section-title {
  margin: 0;
  font-size: $font-size-lg;
  font-weight: $font-weight-bold;
  color: $text-primary;

  .dark & {
    color: $gray-100;
  }
}

.section-description {
  margin: 0;
  font-size: $font-size-sm;
  color: $text-secondary;
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: $spacing-6;
}

.form-group {
  display: flex;
  flex-direction: column;
  gap: $spacing-3;
}

.form-label {
  font-size: $font-size-base;
  font-weight: $font-weight-semibold;
  color: $text-regular;

  .dark & {
    color: $gray-300;
  }
}

.input-wrapper {
  position: relative;
  display: flex;
  align-items: center;
  background: linear-gradient(135deg, $gray-50 0%, #ffffff 100%);
  border: 2px solid $border-light;
  border-radius: $border-radius-lg;
  transition: all $duration-fast;
  box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.05);

  &:focus-within {
    border-color: $primary-500;
    background: #ffffff;
    box-shadow: 0 0 0 4px rgba($primary-500, 0.1), inset 0 1px 3px rgba(0, 0, 0, 0.05);
    transform: translateY(-1px);
  }

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    border-color: $gray-600;
    box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.3);

    &:focus-within {
      background: $gray-800;
      border-color: $primary-400;
      box-shadow: 0 0 0 4px rgba($primary-400, 0.1), inset 0 1px 3px rgba(0, 0, 0, 0.3);
    }
  }

  &.disabled {
    background: $gray-100;
    cursor: not-allowed;
    opacity: 0.7;

    .dark & {
      background: $gray-800;
    }
  }
}

.input-icon {
  padding: 0 $spacing-3;
  color: $text-secondary;
}

.form-input {
  flex: 1;
  border: none;
  background: transparent;
  padding: $spacing-4 $spacing-3;
  font-size: $font-size-sm;
  color: $text-primary;
  outline: none;

  .dark & {
    color: $gray-100;
  }

  &:disabled {
    cursor: not-allowed;
    color: $text-placeholder;
  }

  &.input-error {
    border-color: $danger-color;
  }
}

.input-suffix {
  padding: 0 $spacing-3;
  font-size: $font-size-sm;
  color: $text-placeholder;
}

.input-toggle {
  padding: 0 $spacing-3;
  background: none;
  border: none;
  cursor: pointer;
  color: $text-secondary;
  transition: transform $duration-fast;

  &:hover {
    transform: scale(1.1);
    color: $primary-600;
  }
}

.form-hint {
  font-size: $font-size-sm;
  color: $text-placeholder;
}

.form-error {
  font-size: $font-size-sm;
  color: $danger-color;
}

// Password Strength
.password-strength {
  display: flex;
  align-items: center;
  gap: $spacing-3;
}

.strength-bar {
  flex: 1;
  height: 6px;
  background: $border-light;
  border-radius: $border-radius-full;
  overflow: hidden;
  box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.1);

  .dark & {
    background: $gray-600;
  }
}

.strength-fill {
  height: 100%;
  transition: all $duration-base;
  position: relative;

  &::after {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: inherit;
    border-radius: inherit;
    animation: shimmer 2s infinite;
  }

  &.weak {
    background: linear-gradient(90deg, $danger-color, darken($danger-color, 20%));
  }

  &.medium {
    background: linear-gradient(90deg, $warning-color, darken($warning-color, 20%));
  }

  &.strong {
    background: linear-gradient(90deg, $success-color, darken($success-color, 20%));
  }
}

@keyframes shimmer {
  0% { background-position: -200% 0; }
  100% { background-position: 200% 0; }
}

.strength-text {
  font-size: $font-size-xs;
  font-weight: $font-weight-semibold;

  .strength-fill.weak + & {
    color: $danger-color;
  }

  .strength-fill.medium + & {
    color: $warning-color;
  }

  .strength-fill.strong + & {
    color: $success-color;
  }
}

// Form Actions
.form-actions {
  display: flex;
  justify-content: flex-end;
  gap: $spacing-4;
  padding-top: $spacing-6;
  border-top: 1px solid $border-light;

  .dark & {
    border-top-color: $gray-700;
  }
}

.btn {
  padding: $spacing-3 $spacing-6;
  border: none;
  border-radius: $border-radius-lg;
  font-size: $font-size-base;
  font-weight: $font-weight-semibold;
  cursor: pointer;
  transition: all $duration-fast;
  display: flex;
  align-items: center;
  gap: $spacing-2;
  box-shadow: $shadow-sm;
  position: relative;
  overflow: hidden;

  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
    box-shadow: none;
  }

  &::before {
    content: '';
    position: absolute;
    top: 0;
    left: -100%;
    width: 100%;
    height: 100%;
    background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.3), transparent);
    transition: left $duration-base;
  }

  &:hover:not(:disabled) {
    &::before {
      left: 100%;
    }
  }
}

.btn-secondary {
  background: $gray-100;
  color: $text-primary;

  .dark & {
    background: $gray-700;
    color: $gray-300;
  }

  &:hover:not(:disabled) {
    background: $gray-200;
  }

  .dark &:hover:not(:disabled) {
    background: $gray-600;
  }
}

.btn-primary {
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  color: #ffffff;

  &:hover:not(:disabled) {
    transform: translateY(-2px);
    box-shadow: 0 6px 20px rgba($primary-500, 0.4);
  }
}

.loading-spinner {
  width: 14px;
  height: 14px;
  border: 2px solid #ffffff;
  border-top-color: transparent;
  border-radius: $border-radius-full;
  animation: spin 0.8s linear infinite;
  box-shadow: 0 0 4px rgba(255, 255, 255, 0.3);
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

// ==========================================
// NOTIFICATIONS TOGGLES
// ==========================================
.notification-items {
  display: flex;
  flex-direction: column;
  gap: $spacing-4;
}

.notification-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: $spacing-4 $spacing-5;
  background: linear-gradient(135deg, $gray-50 0%, #ffffff 100%);
  border-radius: $border-radius-lg;
  transition: all $duration-fast;
  border: 1px solid $border-light;

  &:hover {
    background: #ffffff;
    border-color: $primary-300;
    box-shadow: $shadow-sm;
    transform: translateX(2px);
  }

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    border-color: $gray-600;

    &:hover {
      background: $gray-800;
      border-color: $primary-500;
    }
  }
}

.item-info {
  display: flex;
  align-items: center;
  gap: $spacing-3;
  flex: 1;
}

.item-icon {
  color: $primary-600;

  .dark & {
    color: $primary-400;
  }
}

.item-text {
  display: flex;
  flex-direction: column;
  gap: $spacing-1;
}

.item-title {
  font-size: $font-size-base;
  font-weight: $font-weight-semibold;
  color: $text-primary;

  .dark & {
    color: $gray-100;
  }
}

.item-description {
  font-size: $font-size-sm;
  color: $text-secondary;
}

.toggle-switch {
  position: relative;
  width: 48px;
  height: 26px;

  input {
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
    background-color: $gray-300;
    transition: $duration-base;
    border-radius: $border-radius-full;
    box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.2);

    .dark & {
      background-color: $gray-600;
      box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.5);
    }

    &:before {
      position: absolute;
      content: "";
      height: 20px;
      width: 20px;
      left: 3px;
      bottom: 3px;
      background-color: #ffffff;
      transition: $duration-base;
      border-radius: $border-radius-full;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
    }
  }

  input:checked + .toggle-slider {
    background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
    box-shadow: 0 2px 8px rgba($primary-500, 0.3);
  }

  input:checked + .toggle-slider:before {
    transform: translateX(22px);
  }

  &:hover .toggle-slider {
    background-color: $gray-400;

    .dark & {
      background-color: $gray-500;
    }
  }
}

// ==========================================
// ACTIVITY COLUMN (RIGHT)
// ==========================================
.activity-column {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: $spacing-4;
  align-content: start;
}

.info-card,
.activity-card,
.actions-card {
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border-radius: $border-radius-xl;
  box-shadow: $shadow-lg;
  padding: $spacing-4;
  border: 1px solid $border-light;
  transition: all $duration-slow;

  &:hover {
    box-shadow: $shadow-xl;
    transform: translateY(-2px);
    border-color: $primary-200;
  }

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    border-color: $gray-700;

    &:hover {
      border-color: $primary-500;
    }
  }
}

.actions-card {
  grid-column: 1 / -1;
}

.info-header,
.activity-card .card-header,
.actions-card .card-header {
  display: flex;
  align-items: center;
  gap: $spacing-2;
  margin-bottom: $spacing-3;
}

.info-icon,
.activity-card .header-icon,
.actions-card .header-icon {
  color: $primary-600;

  .dark & {
    color: $primary-400;
  }
}

.info-title,
.activity-card .card-title,
.actions-card .card-title {
  margin: 0;
  font-size: $font-size-base;
  font-weight: $font-weight-bold;
  color: $text-primary;

  .dark & {
    color: $gray-100;
  }
}

// Info Card
.info-content {
  display: flex;
  flex-direction: column;
  gap: $spacing-2;
}

.info-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: $spacing-1 0;
  border-bottom: 1px solid $border-light;

  &:last-child {
    border-bottom: none;
  }

  .dark & {
    border-bottom-color: $gray-700;
  }
}

.info-label {
  font-size: $font-size-sm;
  color: $text-secondary;
}

.info-value {
  font-size: $font-size-sm;
  font-weight: $font-weight-semibold;
  color: $text-primary;

  .dark & {
    color: $gray-100;
  }
}

.status-active {
  color: $success-color;
}

// Activity List
.activity-list {
  display: flex;
  flex-direction: column;
  gap: $spacing-2;
}

.activity-item {
  display: flex;
  align-items: center;
  gap: $spacing-2;
  padding: $spacing-3;
  background: linear-gradient(90deg, $gray-50 0%, transparent 100%);
  border-radius: $border-radius-base;
  transition: all $duration-fast;
  border-left: 3px solid transparent;

  &:hover {
    background: linear-gradient(90deg, rgba($primary-500, 0.1) 0%, transparent 100%);
    border-left-color: $primary-500;
    transform: translateX(4px);
  }

  .dark & {
    background: linear-gradient(90deg, $gray-700 0%, transparent 100%);

    &:hover {
      background: linear-gradient(90deg, rgba($primary-400, 0.15) 0%, transparent 100%);
      border-left-color: $primary-400;
    }
  }
}

.activity-icon {
  width: 20px;
  height: 20px;
  color: $primary-600;

  .dark & {
    color: $primary-400;
  }
}

.activity-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: $spacing-1;
}

.activity-text {
  font-size: $font-size-sm;
  color: $text-primary;

  .dark & {
    color: $gray-100;
  }
}

.activity-time {
  font-size: $font-size-xs;
  color: $text-placeholder;
}

.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: $spacing-2;
  padding: $spacing-6 $spacing-4;
  text-align: center;
}

.empty-icon {
  width: 48px;
  height: 48px;
  opacity: 0.5;
  color: $text-secondary;
}

.empty-text {
  font-size: $font-size-sm;
  color: $text-placeholder;
}

// Actions Grid
.actions-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: $spacing-2;
  margin-top: $spacing-2;
}

@media (max-width: 1400px) {
  .actions-grid {
    grid-template-columns: repeat(2, 1fr);
  }
}

.action-btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: $spacing-2;
  padding: $spacing-4 $spacing-3;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border: 2px solid $border-light;
  border-radius: $border-radius-lg;
  cursor: pointer;
  transition: all $duration-fast;
  min-height: 75px;
  position: relative;
  overflow: hidden;
  box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.05);

  &::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: linear-gradient(135deg, rgba($primary-500, 0.08) 0%, rgba($primary-600, 0.03) 100%);
    opacity: 0;
    transition: opacity $duration-fast;
  }

  &:hover {
    background: #ffffff;
    border-color: $primary-500;
    box-shadow: 0 6px 16px rgba($primary-500, 0.25), 0 2px 8px rgba($primary-500, 0.15);
    transform: translateY(-4px);

    &::before {
      opacity: 1;
    }

    .action-icon {
      transform: scale(1.1);
    }
  }

  &:active {
    transform: translateY(-2px);
  }

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    border-color: $gray-600;
    box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.3);

    &:hover {
      background: $gray-800;
      border-color: $primary-400;
      box-shadow: 0 6px 16px rgba($primary-400, 0.25), 0 2px 8px rgba($primary-400, 0.15);
    }
  }
}

.action-icon {
  width: 24px;
  height: 24px;
  color: $primary-600;
  margin-bottom: 0;

  .dark & {
    color: $primary-400;
  }
}

.action-text {
  font-size: $font-size-xs;
  font-weight: $font-weight-semibold;
  color: $text-primary;
  text-align: center;
  line-height: 1.2;

  .dark & {
    color: $gray-100;
  }
}

// ==========================================
// TOAST MESSAGE
// ==========================================
.toast-message {
  position: fixed;
  top: $spacing-6;
  right: $spacing-6;
  display: flex;
  align-items: center;
  gap: $spacing-3;
  padding: $spacing-4 $spacing-5;
  border-radius: $border-radius-lg;
  box-shadow: $shadow-2xl;
  z-index: 9999;
  animation: slideIn $duration-base $easing-ease-out;
  backdrop-filter: blur(8px);
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
  background: linear-gradient(135deg, rgba($success-color, 0.95) 0%, rgba($success-color, 0.85) 100%);
  color: #ffffff;
  border-left: 4px solid lighten($success-color, 20%);
  box-shadow: 0 8px 24px rgba($success-color, 0.4);

  .dark & {
    background: linear-gradient(135deg, rgba($success-color, 0.9) 0%, rgba($success-color, 0.8) 100%);
  }
}

.toast-message.error {
  background: linear-gradient(135deg, rgba($danger-color, 0.95) 0%, rgba($danger-color, 0.85) 100%);
  color: #ffffff;
  border-left: 4px solid lighten($danger-color, 20%);
  box-shadow: 0 8px 24px rgba($danger-color, 0.4);

  .dark & {
    background: linear-gradient(135deg, rgba($danger-color, 0.9) 0%, rgba($danger-color, 0.8) 100%);
  }
}

.toast-icon {
  font-size: $font-size-base;
  font-weight: $font-weight-bold;
}

.toast-text {
  flex: 1;
  font-weight: $font-weight-semibold;
}

.toast-close {
  background: none;
  border: none;
  cursor: pointer;
  font-size: $font-size-base;
  padding: $spacing-1;
}

// Fade transition
.fade-enter-active,
.fade-leave-active {
  transition: opacity $duration-base;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

// ==========================================
// RESPONSIVE DESIGN
// ==========================================
@media (max-width: 1400px) {
  .activity-column {
    grid-template-columns: 1fr;
  }

  .actions-card {
    grid-column: 1 / -1;
  }

  .actions-grid {
    grid-template-columns: repeat(4, 1fr);
  }
}

@media (max-width: 1024px) {
  .actions-grid {
    grid-template-columns: repeat(2, 1fr);
  }
}

@media (max-width: 768px) {
  .profile-page {
    padding: $spacing-4 0;
  }

  .profile-header {
    padding: 0 $spacing-4;
    margin-bottom: $spacing-4;
  }

  .header-content {
    flex-direction: column;
    align-items: center;
    text-align: center;
    gap: $spacing-4;
  }

  .user-stats {
    margin-top: $spacing-3;
  }

  .card-header {
    flex-direction: column;
    align-items: flex-start;
    gap: $spacing-3;
  }

  .header-tabs {
    width: 100%;
    justify-content: space-between;
  }

  .tab-button {
    flex: 1;
    padding: $spacing-2;
    text-align: center;
    font-size: $font-size-xs;
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

  .activity-column {
    grid-template-columns: 1fr;
  }

  .actions-grid {
    grid-template-columns: repeat(2, 1fr);
  }
}
	

/* Dark mode */
[data-theme="dark"] .profile-page {
  color: #f3f4f6;
}

[data-theme="dark"] .profile-page .page-header,
[data-theme="dark"] .profile-page .page-title,
[data-theme="dark"] .profile-page .card-header {
  color: #f3f4f6;
}

[data-theme="dark"] .profile-page .page-subtitle,
[data-theme="dark"] .profile-page .page-description {
  color: #9ca3af;
}

[data-theme="dark"] .profile-page .stat-card,
[data-theme="dark"] .profile-page .filter-card,
[data-theme="dark"] .profile-page .tasks-card,
[data-theme="dark"] .profile-page .chart-card,
[data-theme="dark"] .profile-page .table-card,
[data-theme="dark"] .profile-page .form-card,
[data-theme="dark"] .profile-page .detail-header,
[data-theme="dark"] .profile-page .detail-content,
[data-theme="dark"] .profile-page .toolbar,
[data-theme="dark"] .profile-page .export-options {
  background: rgba(40, 40, 45, 0.98) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #f3f4f6;
}

[data-theme="dark"] .profile-page .stat-value,
[data-theme="dark"] .profile-page .metric-value {
  color: #f3f4f6;
}

[data-theme="dark"] .profile-page .stat-label,
[data-theme="dark"] .profile-page .metric-label {
  color: #9ca3af;
}

[data-theme="dark"] .profile-page .empty-state,
[data-theme="dark"] .profile-page .empty-text {
  color: #9ca3af;
}
</style>
