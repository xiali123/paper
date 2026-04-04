<template>
  <div class="settings-page">
    <!-- Page Header -->
    <div class="page-header">
      <h1 class="page-title">
        <el-icon><Setting /></el-icon>
        系统设置
      </h1>
      <div class="header-actions">
        <el-button type="primary" :icon="Check" @click="handleSaveAll">
          保存所有更改
        </el-button>
      </div>
    </div>

    <!-- User Info Cards -->
    <el-row :gutter="20" class="stats-row">
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="user-card">
          <div class="user-content">
            <div class="user-avatar">
              <el-avatar :size="60">{{ profileForm.name.charAt(0) }}</el-avatar>
            </div>
            <div class="user-info">
              <div class="user-name">{{ profileForm.name }}</div>
              <div class="user-role">管理员</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon account">
              <el-icon><User /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ profileForm.username }}</div>
              <div class="stat-label">用户名</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon email">
              <el-icon><Message /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ profileForm.email }}</div>
              <div class="stat-label">邮箱</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon system">
              <el-icon><Odometer /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ basicForm.systemName }}</div>
              <div class="stat-label">系统</div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Settings Tabs -->
    <el-row :gutter="20" class="content-row">
      <el-col :span="24">
        <el-card shadow="hover" class="settings-card">
          <el-tabs v-model="activeTab" class="settings-tabs">
            <!-- Basic Settings -->
            <el-tab-pane name="basic">
              <template #label>
                <span class="tab-label">
                  <el-icon><Odometer /></el-icon>
                  基本设置
                </span>
              </template>
              <div class="tab-content">
                <div class="tab-header">
                  <h2>基本设置</h2>
                  <p>配置系统的基础信息</p>
                </div>
                <el-form :model="basicForm" label-width="140px" class="settings-form">
                  <el-form-item label="系统名称">
                    <el-input
                      v-model="basicForm.systemName"
                      placeholder="请输入系统名称"
                      clearable
                      size="large"
                    />
                  </el-form-item>
                  <el-form-item label="系统描述">
                    <el-input
                      v-model="basicForm.systemDesc"
                      type="textarea"
                      :rows="4"
                      placeholder="请输入系统描述"
                      resize="none"
                    />
                  </el-form-item>
                  <el-form-item label="系统语言">
                    <el-select v-model="basicForm.language" placeholder="选择语言" size="large">
                      <el-option label="简体中文" value="zh-CN" />
                      <el-option label="English" value="en-US" />
                    </el-select>
                  </el-form-item>
                  <el-form-item label="时区设置">
                    <el-select v-model="basicForm.timezone" placeholder="选择时区" size="large">
                      <el-option label="北京时间 (UTC+8)" value="Asia/Shanghai" />
                      <el-option label="纽约时间 (UTC-5)" value="America/New_York" />
                      <el-option label="伦敦时间 (UTC+0)" value="Europe/London" />
                    </el-select>
                  </el-form-item>
                </el-form>
              </div>
            </el-tab-pane>

            <!-- Profile Settings -->
            <el-tab-pane name="profile">
              <template #label>
                <span class="tab-label">
                  <el-icon><User /></el-icon>
                  个人设置
                </span>
              </template>
              <div class="tab-content">
                <div class="tab-header">
                  <h2>个人资料</h2>
                  <p>更新您的个人信息</p>
                </div>
                <div class="profile-header">
                  <el-avatar :size="100" class="profile-avatar">
                    {{ profileForm.name.charAt(0) }}
                  </el-avatar>
                  <el-button type="primary" plain size="small">
                    <el-icon><Upload /></el-icon>
                    更换头像
                  </el-button>
                </div>
                <el-form :model="profileForm" label-width="140px" class="settings-form">
                  <el-form-item label="用户名">
                    <el-input v-model="profileForm.username" disabled size="large">
                      <template #prefix>
                        <el-icon><User /></el-icon>
                      </template>
                    </el-input>
                  </el-form-item>
                  <el-form-item label="姓名">
                    <el-input
                      v-model="profileForm.name"
                      placeholder="请输入您的姓名"
                      clearable
                      size="large"
                    />
                  </el-form-item>
                  <el-form-item label="邮箱地址">
                    <el-input
                      v-model="profileForm.email"
                      placeholder="请输入邮箱地址"
                      clearable
                      size="large"
                    >
                      <template #prefix>
                        <el-icon><Message /></el-icon>
                      </template>
                    </el-input>
                  </el-form-item>
                  <el-form-item label="个人简介">
                    <el-input
                      v-model="profileForm.bio"
                      type="textarea"
                      :rows="4"
                      placeholder="介绍一下自己..."
                      resize="none"
                    />
                  </el-form-item>
                </el-form>
              </div>
            </el-tab-pane>

            <!-- Security Settings -->
            <el-tab-pane name="security">
              <template #label>
                <span class="tab-label">
                  <el-icon><Lock /></el-icon>
                  安全设置
                </span>
              </template>
              <div class="tab-content">
                <div class="tab-header">
                  <h2>安全设置</h2>
                  <p>管理您的密码和安全选项</p>
                </div>
                <el-form :model="securityForm" label-width="140px" class="settings-form">
                  <el-form-item label="当前密码">
                    <el-input
                      v-model="securityForm.currentPassword"
                      type="password"
                      placeholder="请输入当前密码"
                      show-password
                      size="large"
                    >
                      <template #prefix>
                        <el-icon><Lock /></el-icon>
                      </template>
                    </el-input>
                  </el-form-item>
                  <el-form-item label="新密码">
                    <el-input
                      v-model="securityForm.newPassword"
                      type="password"
                      placeholder="请输入新密码"
                      show-password
                      size="large"
                    />
                  </el-form-item>
                  <el-form-item label="确认密码">
                    <el-input
                      v-model="securityForm.confirmPassword"
                      type="password"
                      placeholder="请再次输入新密码"
                      show-password
                      size="large"
                    />
                  </el-form-item>
                  <el-form-item>
                    <el-button type="primary" size="large" @click="handleSaveSecurity">
                      <el-icon><Check /></el-icon>
                      修改密码
                    </el-button>
                    <el-button size="large">取消</el-button>
                  </el-form-item>
                </el-form>

                <el-divider />

                <div class="security-options">
                  <h3>两步验证</h3>
                  <p>为您的账户添加额外的安全保护</p>
                  <el-switch
                    v-model="securityForm.twoFactor"
                    active-text="已启用"
                    inactive-text="未启用"
                    size="large"
                  />
                </div>
              </div>
            </el-tab-pane>

            <!-- Notification Settings -->
            <el-tab-pane name="notifications">
              <template #label>
                <span class="tab-label">
                  <el-icon><Bell /></el-icon>
                  通知设置
                </span>
              </template>
              <div class="tab-content">
                <div class="tab-header">
                  <h2>通知偏好</h2>
                  <p>选择您希望接收的通知类型</p>
                </div>
                <div class="notification-settings">
                  <div class="notification-item">
                    <div class="notification-info">
                      <el-icon class="notification-icon"><Message /></el-icon>
                      <div>
                        <h4>邮件通知</h4>
                        <p>接收重要更新和提醒的邮件通知</p>
                      </div>
                    </div>
                    <el-switch v-model="notificationSettings.email" size="large" />
                  </div>
                  <el-divider />
                  <div class="notification-item">
                    <div class="notification-info">
                      <el-icon class="notification-icon"><ChatDotRound /></el-icon>
                      <div>
                        <h4>系统消息</h4>
                        <p>接收系统内消息和通知</p>
                      </div>
                    </div>
                    <el-switch v-model="notificationSettings.system" size="large" />
                  </div>
                  <el-divider />
                  <div class="notification-item">
                    <div class="notification-info">
                      <el-icon class="notification-icon"><Bell /></el-icon>
                      <div>
                        <h4>任务提醒</h4>
                        <p>爬虫任务完成和异常提醒</p>
                      </div>
                    </div>
                    <el-switch v-model="notificationSettings.tasks" size="large" />
                  </div>
                </div>
              </div>
            </el-tab-pane>

            <!-- Appearance Settings -->
            <el-tab-pane name="appearance">
              <template #label>
                <span class="tab-label">
                  <el-icon><Brush /></el-icon>
                  界面设置
                </span>
              </template>
              <div class="tab-content">
                <div class="tab-header">
                  <h2>外观设置</h2>
                  <p>自定义您的界面体验</p>
                </div>
                <div class="appearance-settings">
                  <div class="appearance-item">
                    <div class="appearance-info">
                      <el-icon class="appearance-icon" :size="32"><Moon /></el-icon>
                      <div>
                        <h4>深色模式</h4>
                        <p>切换到深色主题以减少眼睛疲劳</p>
                      </div>
                    </div>
                    <el-switch v-model="appearanceSettings.darkMode" size="large" />
                  </div>
                  <el-divider />
                  <div class="appearance-item">
                    <div class="appearance-info">
                      <el-icon class="appearance-icon" :size="32"><Grid /></el-icon>
                      <div>
                        <h4>侧边栏</h4>
                        <p>默认折叠侧边栏</p>
                      </div>
                    </div>
                    <el-switch v-model="appearanceSettings.sidebarCollapsed" size="large" />
                  </div>
                  <el-divider />
                  <div class="appearance-item">
                    <div class="appearance-info">
                      <el-icon class="appearance-icon" :size="32"><Odometer /></el-icon>
                      <div>
                        <h4>页面动画</h4>
                        <p>启用页面切换动画效果</p>
                      </div>
                    </div>
                    <el-switch v-model="appearanceSettings.animations" size="large" />
                  </div>
                </div>
              </div>
            </el-tab-pane>
          </el-tabs>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, watch, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { ElMessage } from 'element-plus'
import {
  Setting,
  Check,
  Odometer,
  User,
  Lock,
  Bell,
  Brush,
  Upload,
  Message,
  ChatDotRound,
  Moon,
  Grid
} from '@element-plus/icons-vue'

const route = useRoute()
const activeTab = ref('basic')

// 从路由参数或查询参数获取要激活的标签页
onMounted(() => {
  const tabFromParam = route.params.tab || route.query.tab
  if (tabFromParam && ['basic', 'profile', 'security', 'notifications', 'appearance'].includes(tabFromParam)) {
    activeTab.value = tabFromParam
  }
})

// 监听路由变化
watch(() => route.params.tab || route.query.tab, (newTab) => {
  if (newTab && ['basic', 'profile', 'security', 'notifications', 'appearance'].includes(newTab)) {
    activeTab.value = newTab
  }
})

// 基本设置
const basicForm = reactive({
  systemName: 'PaperCrawler',
  systemDesc: '学术论文爬取管理系统 - 高效、智能、易用',
  language: 'zh-CN',
  timezone: 'Asia/Shanghai',
})

// 个人设置
const profileForm = reactive({
  username: 'admin',
  name: '管理员',
  email: 'admin@example.com',
  bio: '',
})

// 安全设置
const securityForm = reactive({
  currentPassword: '',
  newPassword: '',
  confirmPassword: '',
  twoFactor: false,
})

// 通知设置
const notificationSettings = reactive({
  email: true,
  system: true,
  tasks: true,
})

// 界面设置
const appearanceSettings = reactive({
  darkMode: false,
  sidebarCollapsed: false,
  animations: true,
})

// 方法
function handleSaveBasic() {
  ElMessage.success('基本设置已保存')
}

function handleSaveProfile() {
  ElMessage.success('个人资料已更新')
}

function handleSaveSecurity() {
  if (securityForm.newPassword !== securityForm.confirmPassword) {
    ElMessage.error('两次输入的密码不一致')
    return
  }
  if (securityForm.newPassword.length < 6) {
    ElMessage.warning('密码长度不能少于6位')
    return
  }
  ElMessage.success('密码修改成功')
  securityForm.currentPassword = ''
  securityForm.newPassword = ''
  securityForm.confirmPassword = ''
}

function handleSaveAll() {
  ElMessage.success('所有设置已保存')
}
</script>

<style scoped lang="scss">
.settings-page {
  width: 100%;
  max-width: 1600px;
  margin: 0 auto;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: $spacing-6;
  padding: $spacing-8;
  background: linear-gradient(135deg, #ffffff 0%, #f8fafc 100%);
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  margin-bottom: $spacing-6;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    box-shadow: $shadow-md;
  }

  .page-title {
    display: flex;
    align-items: center;
    gap: $spacing-3;
    margin: 0;
    font-size: $font-size-3xl;
    font-weight: $font-weight-bold;
    color: $text-primary;

    .el-icon {
      color: $primary-500;
    }
  }

  .header-actions {
    display: flex;
    gap: $spacing-3;
  }
}

.stats-row {
  margin-bottom: $spacing-6;

  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

.user-card {
  height: 100%;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  transition: all $duration-slow;

  &:hover {
    box-shadow: $shadow-md;
    transform: translateY(-4px);
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  :deep(.el-card__body) {
    padding: $spacing-5;
  }

  .user-content {
    display: flex;
    align-items: center;
    gap: $spacing-4;

    .user-avatar {
      .el-avatar {
        background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
        color: white;
        font-size: $font-size-2xl;
        font-weight: $font-weight-bold;
      }
    }

    .user-info {
      .user-name {
        font-size: $font-size-lg;
        font-weight: $font-weight-semibold;
        color: $text-primary;
        margin-bottom: $spacing-1;
      }

      .user-role {
        font-size: $font-size-sm;
        color: $text-secondary;
      }
    }
  }
}

.stat-card {
  height: 100%;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  transition: all $duration-slow;
  overflow: hidden;

  &:hover {
    box-shadow: $shadow-md;
    transform: translateY(-4px);
  }

  :deep(.el-card__body) {
    padding: $spacing-6;
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  .stat-content {
    display: flex;
    align-items: center;
    gap: $spacing-4;

    .stat-icon {
      width: 48px;
      height: 48px;
      border-radius: $border-radius-lg;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 24px;
      flex-shrink: 0;
      box-shadow: $shadow-sm;

      &.account {
        background: linear-gradient(135deg, #409eff 0%, #66b1ff 100%);
        color: white;
      }

      &.email {
        background: linear-gradient(135deg, #67c23a 0%, #85ce61 100%);
        color: white;
      }

      &.system {
        background: linear-gradient(135deg, #e6a23c 0%, #f0c78a 100%);
        color: white;
      }
    }

    .stat-info {
      flex: 1;

      .stat-value {
        font-size: $font-size-base;
        font-weight: $font-weight-semibold;
        color: $text-primary;
        line-height: 1.2;
        margin-bottom: $spacing-1;
      }

      .stat-label {
        font-size: $font-size-sm;
        color: $text-regular;
        font-weight: $font-weight-medium;
      }
    }
  }
}

.content-row {
  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

.settings-card {
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  min-height: 600px;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  :deep(.el-card__body) {
    padding: 0;
  }
}

.settings-tabs {
  :deep(.el-tabs__header) {
    margin: 0;
    padding: 0 $spacing-6;
    background: linear-gradient(180deg, $gray-50 0%, #ffffff 100%);
    border-bottom: 1px solid $border-light;

    .dark & {
      background: linear-gradient(180deg, $gray-900 0%, $gray-800 100%);
      border-bottom-color: $gray-700;
    }
  }

  :deep(.el-tabs__nav-wrap::after) {
    display: none;
  }

  :deep(.el-tabs__item) {
    height: 60px;
    line-height: 60px;
    padding: 0 $spacing-6;
    font-size: $font-size-base;
    font-weight: $font-weight-medium;
    color: $text-secondary;
    border: none;
    transition: all $duration-fast;

    &:hover {
      color: $primary-600;
    }

    &.is-active {
      color: $primary-600;
      font-weight: $font-weight-semibold;

      .dark & {
        color: $primary-400;
      }
    }
  }

  :deep(.el-tabs__active-bar) {
    height: 3px;
    background: linear-gradient(90deg, $primary-500 0%, $primary-600 100%);
    border-radius: $border-radius-full;
  }

  .tab-label {
    display: flex;
    align-items: center;
    gap: $spacing-2;

    .el-icon {
      font-size: 18px;
    }
  }
}

.tab-content {
  padding: $spacing-8;
  animation: fadeIn $duration-base $easing-ease-out;

  @keyframes fadeIn {
    from {
      opacity: 0;
      transform: translateY(10px);
    }
    to {
      opacity: 1;
      transform: translateY(0);
    }
  }
}

.tab-header {
  text-align: center;
  margin-bottom: $spacing-8;
  padding-bottom: $spacing-6;
  border-bottom: 2px solid $border-light;

  h2 {
    font-size: $font-size-2xl;
    font-weight: $font-weight-semibold;
    color: $text-primary;
    margin: 0 0 $spacing-2 0;
  }

  p {
    font-size: $font-size-sm;
    color: $text-secondary;
    margin: 0;
  }
}

.settings-form {
  max-width: 700px;
  margin: 0 auto;

  :deep(.el-form-item__label) {
    font-weight: $font-weight-medium;
    color: $text-primary;
  }

  :deep(.el-input__wrapper) {
    border-radius: $border-radius-base;
    box-shadow: $shadow-sm;
    transition: all $duration-fast;

    &:hover {
      box-shadow: $shadow-base;
    }

    &.is-focus {
      box-shadow: 0 0 0 2px rgba($primary-500, 0.2);
    }
  }

  :deep(.el-textarea__inner) {
    border-radius: $border-radius-base;
  }
}

.profile-header {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: $spacing-4;
  margin-bottom: $spacing-8;
  padding-bottom: $spacing-8;
  border-bottom: 2px solid $border-light;

  .profile-avatar {
    background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
    color: white;
    font-size: $font-size-4xl;
    font-weight: $font-weight-bold;
    box-shadow: $shadow-lg;
  }
}

.security-options {
  margin-top: $spacing-8;
  padding: $spacing-6;
  background: linear-gradient(135deg, rgba($primary-50, 0.5) 0%, rgba($primary-100, 0.3) 100%);
  border-radius: $border-radius-lg;
  border: 1px solid rgba($primary-200, 0.3);

  .dark & {
    background: rgba($gray-700, 0.3);
    border-color: rgba($gray-600, 0.3);
  }

  h3 {
    font-size: $font-size-lg;
    font-weight: $font-weight-semibold;
    color: $text-primary;
    margin: 0 0 $spacing-2 0;
  }

  p {
    font-size: $font-size-sm;
    color: $text-secondary;
    margin: 0 0 $spacing-4 0;
  }
}

.notification-settings,
.appearance-settings {
  max-width: 900px;
  margin: 0 auto;
}

.notification-item,
.appearance-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: $spacing-5;
  border: 1px solid $border-light;
  border-radius: $border-radius-lg;
  transition: background $duration-fast;

  &:hover {
    background: $gray-50;

    .dark & {
      background: $gray-700;
    }
  }

  .notification-info,
  .appearance-info {
    display: flex;
    align-items: flex-start;
    gap: $spacing-4;
    flex: 1;

    .notification-icon,
    .appearance-icon {
      width: 48px;
      height: 48px;
      display: flex;
      align-items: center;
      justify-content: center;
      background: rgba($primary-500, 0.1);
      color: $primary-500;
      border-radius: $border-radius-lg;
      flex-shrink: 0;

      .dark & {
        background: rgba($gray-700, 0.5);
        color: $primary-400;
      }
    }

    h4 {
      font-size: $font-size-base;
      font-weight: $font-weight-semibold;
      color: $text-primary;
      margin: 0 0 $spacing-1 0;
    }

    p {
      font-size: $font-size-sm;
      color: $text-secondary;
      margin: 0;
    }
  }
}

// Responsive Design
@media (max-width: 768px) {
  .page-header {
    flex-direction: column;
    align-items: flex-start;
    gap: $spacing-4;
    padding: $spacing-5;

    .page-title {
      font-size: $font-size-2xl;
    }

    .header-actions {
      width: 100%;

      .el-button {
        flex: 1;
      }
    }
  }

  .user-card {
    .user-content {
      flex-direction: column;
      text-align: center;

      .user-avatar .el-avatar {
        width: 80px !important;
        height: 80px !important;
        font-size: $font-size-2xl !important;
      }
    }
  }

  .stat-card {
    .stat-content {
      .stat-value {
        font-size: $font-size-sm;
      }

      .stat-label {
        font-size: $font-size-xs;
      }
    }
  }

  .tab-content {
    padding: $spacing-4;
  }

  .settings-form {
    :deep(.el-form-item__label) {
      width: 100% !important;
      text-align: left;
      margin-bottom: $spacing-2;
    }
  }

  .notification-item,
  .appearance-item {
    flex-direction: column;
    align-items: flex-start;
    gap: $spacing-3;
  }
}
</style>
