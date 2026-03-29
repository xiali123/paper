<template>
  <div id="app">
    <div class="app-container">
      <!-- Enhanced Header -->
      <header class="app-header">
        <div class="header-content">
          <div class="logo-section">
            <div class="logo-wrapper">
              <span class="logo-icon">📚</span>
              <div class="logo-text">
                <h1 class="logo-title">PaperCrawler</h1>
                <p class="logo-subtitle">{{ $t('app.description') }}</p>
              </div>
            </div>
          </div>

          <nav class="nav-section">
            <div class="nav-links">
              <router-link to="/" class="nav-link">
                <span class="nav-icon">🏠</span>
                <span class="nav-text">{{ $t('nav.home') }}</span>
              </router-link>
              <router-link to="/papers" class="nav-link">
                <span class="nav-icon">📚</span>
                <span class="nav-text">{{ $t('nav.searchPapers') }}</span>
              </router-link>
              <router-link to="/search" class="nav-link">
                <span class="nav-icon">🔍</span>
                <span class="nav-text">{{ $t('nav.search') }}</span>
              </router-link>
              <router-link to="/crawler" class="nav-link">
                <span class="nav-icon">🕷️</span>
                <span class="nav-text">爬虫</span>
              </router-link>
              <router-link to="/stats" class="nav-link">
                <span class="nav-icon">📊</span>
                <span class="nav-text">{{ $t('nav.stats') }}</span>
              </router-link>
            </div>

            <div class="nav-controls">
              <LanguageSwitcher />
              <button
                @click="toggleTheme"
                class="theme-toggle-btn"
                :title="isDark() ? $t('theme.light') : $t('theme.dark')"
              >
                <span class="theme-icon">{{ isDark() ? '☀️' : '🌙' }}</span>
              </button>

              <!-- Auth Section -->
              <div class="auth-section">
                <div v-if="authStore.isAuthenticated" class="user-menu">
                  <div class="user-info" @click="toggleUserDropdown">
                    <span class="user-avatar">{{ userInitial }}</span>
                    <span class="user-name">{{ authStore.user?.fullName || authStore.user?.username || 'User' }}</span>
                    <span class="dropdown-arrow">▼</span>
                  </div>
                  <div v-if="showUserDropdown" class="user-dropdown">
                    <div class="dropdown-item" @click="goToProfile">
                      <span class="dropdown-icon">👤</span>
                      <span>{{ $t('nav.profile') }}</span>
                    </div>
                    <div class="dropdown-divider"></div>
                    <div class="dropdown-item logout" @click="handleLogout">
                      <span class="dropdown-icon">🚪</span>
                      <span>{{ $t('nav.logout') }}</span>
                    </div>
                  </div>
                </div>
                <div v-else class="auth-buttons">
                  <router-link to="/login" class="auth-btn login-btn">
                    <span>{{ $t('nav.login') }}</span>
                  </router-link>
                  <router-link to="/register" class="auth-btn register-btn">
                    <span>{{ $t('nav.register') }}</span>
                  </router-link>
                </div>
              </div>

              <div class="health-status" :class="{ online: backendStatus }">
                <span class="status-dot"></span>
                <span class="status-text">{{ $t('app.status') }}</span>
              </div>
            </div>
          </nav>
        </div>
      </header>

      <!-- Main Content -->
      <main class="app-main">
        <router-view />
      </main>

      <!-- Enhanced Footer -->
      <footer class="app-footer" :class="footerClass">
        <div class="footer-container">
          <div class="footer-content">
            <!-- Brand -->
            <div class="footer-brand">
              <span class="footer-logo">📚</span>
              <span class="footer-name">PaperCrawler</span>
            </div>

            <!-- Links Container -->
            <div class="footer-links-wrapper">
              <!-- Features -->
              <div class="footer-links-group">
                <span class="footer-link-item">
                  <span class="link-icon">🔍</span>
                  <span>智能搜索</span>
                </span>
                <span class="footer-link-item">
                  <span class="link-icon">📊</span>
                  <span>数据分析</span>
                </span>
                <span class="footer-link-item">
                  <span class="link-icon">⭐</span>
                  <span>顶刊追踪</span>
                </span>
                <span class="footer-link-item">
                  <span class="link-icon">📈</span>
                  <span>趋势洞察</span>
                </span>
              </div>

              <div class="footer-group-divider"></div>

              <!-- Community -->
              <div class="footer-links-group">
                <span class="footer-link-item">
                  <span class="link-icon">📧</span>
                  <span>联系我们</span>
                </span>
                <span class="footer-link-item">
                  <span class="link-icon">💬</span>
                  <span>反馈建议</span>
                </span>
                <span class="footer-link-item">
                  <span class="link-icon">🌐</span>
                  <span>GitHub</span>
                </span>
                <span class="footer-link-item">
                  <span class="link-icon">📖</span>
                  <span>使用文档</span>
                </span>
              </div>

              <div class="footer-group-divider"></div>

              <!-- Legal -->
              <div class="footer-links-group">
                <span class="footer-link-item">
                  <span class="link-icon">📜</span>
                  <span>隐私政策</span>
                </span>
                <span class="footer-link-item">
                  <span class="link-icon">⚖️</span>
                  <span>使用条款</span>
                </span>
                <span class="footer-link-item">
                  <span class="link-icon">❓</span>
                  <span>帮助中心</span>
                </span>
                <span class="footer-link-item">
                  <span class="link-icon">🔄</span>
                  <span>更新日志</span>
                </span>
              </div>
            </div>

            <!-- Status & Copyright -->
            <div class="footer-status-section">
              <div class="footer-status">
                <span class="status-indicator" :class="{ online: backendStatus }"></span>
              </div>
              <div class="footer-copyright">
                <span>© 2024 PaperCrawler</span>
                <span class="copyright-divider">•</span>
                <span>Made with ❤️</span>
              </div>
            </div>
          </div>
        </div>
      </footer>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useTheme } from './composables/useTheme'
import { useAuthStore } from './stores/auth'
import LanguageSwitcher from './components/LanguageSwitcher.vue'

const router = useRouter()
const { theme, toggleTheme, isDark } = useTheme()
const authStore = useAuthStore()
const backendStatus = ref(false)
const healthCheckInitialized = ref(false)
const showUserDropdown = ref(false)

// User initial for avatar
const userInitial = computed(() => {
  const fullName = authStore.user?.fullName || authStore.user?.username || ''
  return fullName.charAt(0).toUpperCase()
})

// Toggle user dropdown
const toggleUserDropdown = () => {
  showUserDropdown.value = !showUserDropdown.value
}

// Close dropdown when clicking outside
const handleClickOutside = (event: MouseEvent) => {
  const target = event.target as HTMLElement
  const userMenu = document.querySelector('.user-menu')
  if (userMenu && !userMenu.contains(target)) {
    showUserDropdown.value = false
  }
}

// Go to profile page
const goToProfile = () => {
  showUserDropdown.value = false
  router.push('/profile')
}

// Handle logout
const handleLogout = async () => {
  try {
    await authStore.logout()
    showUserDropdown.value = false
    router.push('/login')
  } catch (error) {
    console.error('Logout failed:', error)
  }
}

// Computed class for footer to support dark mode
const footerClass = computed(() => ({
  'dark-mode': isDark.value
}))

const checkBackend = async () => {
  console.log('Checking backend health...')
  try {
    const response = await fetch('/api/health')
    console.log('Health check response:', response.status)

    if (response.ok) {
      const result = await response.json()
      console.log('Health check result:', result)
      // 后端返回格式：{ status: "ok", timestamp: "..." }
      backendStatus.value = result.status === 'ok'
      healthCheckInitialized.value = true
    } else {
      console.warn('Health check failed with status:', response.status)
      backendStatus.value = false
      healthCheckInitialized.value = true
    }
  } catch (error) {
    console.error('Backend health check error:', error)
    backendStatus.value = false
    healthCheckInitialized.value = true
  }
}

let healthCheckTimer: ReturnType<typeof setInterval> | null = null

onMounted(() => {
  console.log('App mounted, starting health check...')

  // Add click outside listener for dropdown
  document.addEventListener('click', handleClickOutside)

  // Initialize auth store
  authStore.initializeAuth()

  setTimeout(() => {
    checkBackend()
    healthCheckTimer = setInterval(checkBackend, 30000)
  }, 1000)
})

onUnmounted(() => {
  if (healthCheckTimer) {
    clearInterval(healthCheckTimer)
  }
  document.removeEventListener('click', handleClickOutside)
})
</script>

<style scoped>
#app {
  min-height: 100vh;
  background: var(--bg-gradient-subtle);
}

.app-container {
  width: 100%;
  max-width: 1280px;
  margin-left: auto;
  margin-right: auto;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  padding: 0;
}

/* ===================================
   HEADER STYLES
   =================================== */
.app-header {
  background: rgba(248, 249, 250, 0.85);
  padding: 16px 24px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.08);
  backdrop-filter: blur(20px) saturate(180%);
  border-bottom: 2px solid rgba(102, 126, 234, 0.1);
  position: sticky;
  top: 0;
  z-index: var(--z-sticky);
}

.header-content {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 24px;
  max-width: 1400px;
  margin: 0 auto;
  width: 100%;
}

/* Logo Section */
.logo-section {
  flex-shrink: 0;
}

.logo-wrapper {
  display: flex;
  align-items: center;
  gap: 12px;
}

.logo-icon {
  font-size: 32px;
  filter: drop-shadow(0 2px 8px rgba(102, 126, 234, 0.3));
  animation: float 3s ease-in-out infinite;
}

@keyframes float {
  0%, 100% { transform: translateY(0); }
  50% { transform: translateY(-5px); }
}

.logo-text {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.logo-title {
  margin: 0;
  font-size: 24px;
  font-weight: 800;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  line-height: 1.2;
  letter-spacing: -0.5px;
}

.logo-subtitle {
  margin: 0;
  font-size: 13px;
  color: #6b7280;
  font-weight: 500;
}

/* Navigation Section */
.nav-section {
  display: flex;
  align-items: center;
  gap: 16px;
  flex: 1;
  justify-content: center;
}

.nav-links {
  display: flex;
  gap: 8px;
  flex-wrap: nowrap;
}

.nav-link {
  display: flex;
  align-items: center;
  gap: 8px;
  text-decoration: none;
  color: #374151;
  font-weight: 600;
  padding: 12px 20px;
  border-radius: 24px;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  position: relative;
  font-size: 15px;
  background: transparent;
  border: 2px solid transparent;
  white-space: nowrap;
}

.nav-icon {
  font-size: 18px;
  transition: all 0.3s;
}

.nav-text {
  font-size: 15px;
  font-weight: 500;
  white-space: nowrap;
}

.nav-link:hover {
  background: linear-gradient(135deg, rgba(102, 126, 234, 0.1) 0%, rgba(118, 75, 162, 0.1) 100%);
  color: #667eea;
  border-color: rgba(102, 126, 234, 0.2);
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.15);
}

.nav-link:hover .nav-icon {
  transform: scale(1.15);
}

.nav-link.router-link-active {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border-color: transparent;
  box-shadow: 0 6px 20px rgba(102, 126, 234, 0.3);
  transform: translateY(-1px);
}

.nav-link.router-link-active .nav-icon {
  opacity: 1;
  filter: brightness(1.2);
}

/* Navigation Controls */
.nav-controls {
  display: flex;
  align-items: center;
  gap: 12px;
}

.health-status {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  background: rgba(255, 255, 255, 0.9);
  border-radius: 24px;
  font-size: 13px;
  font-weight: 600;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  border: 1px solid #e5e7eb;
  backdrop-filter: blur(10px);
  transition: all 0.3s;
}

.health-status.online {
  background: rgba(34, 197, 94, 0.1);
  border-color: rgba(34, 197, 94, 0.3);
  color: #16a34a;
}

.health-status:not(.online) {
  background: rgba(239, 68, 68, 0.1);
  border-color: rgba(239, 68, 68, 0.3);
  color: #dc2626;
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: currentColor;
  animation: pulse 2s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
    transform: scale(1);
  }
  50% {
    opacity: 0.8;
    transform: scale(1.1);
  }
}

.status-text {
  font-size: 13px;
  font-weight: 600;
  white-space: nowrap;
}

.theme-toggle-btn {
  width: 48px;
  height: 48px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #f3f4f6 0%, #e5e7eb 100%);
  border: none;
  border-radius: 50%;
  font-size: 20px;
  cursor: pointer;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.theme-toggle-btn:hover {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  transform: scale(1.1) rotate(15deg);
  box-shadow: 0 6px 20px rgba(102, 126, 234, 0.4);
}

.theme-toggle-btn:hover .theme-icon {
  color: white;
}

.theme-icon {
  filter: drop-shadow(0 1px 2px rgba(0, 0, 0, 0.1));
  transition: color 0.3s;
}

/* Auth Section */
.auth-section {
  display: flex;
  align-items: center;
  gap: 12px;
}

.auth-buttons {
  display: flex;
  align-items: center;
  gap: 8px;
}

.auth-btn {
  padding: 8px 20px;
  border-radius: 20px;
  font-size: 14px;
  font-weight: 600;
  text-decoration: none;
  transition: all 0.3s;
  cursor: pointer;
}

.login-btn {
  background: transparent;
  color: #667eea;
  border: 2px solid #667eea;
}

.login-btn:hover {
  background: #667eea;
  color: white;
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.3);
}

.register-btn {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border: 2px solid transparent;
}

.register-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(102, 126, 234, 0.4);
}

/* User Menu */
.user-menu {
  position: relative;
}

.user-info {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 12px 6px 6px;
  background: rgba(255, 255, 255, 0.9);
  border: 2px solid #e5e7eb;
  border-radius: 24px;
  cursor: pointer;
  transition: all 0.3s;
}

.user-info:hover {
  border-color: #667eea;
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.2);
}

.user-avatar {
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border-radius: 50%;
  font-size: 14px;
  font-weight: 700;
}

.user-name {
  font-size: 14px;
  font-weight: 600;
  color: #374151;
  max-width: 120px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.dropdown-arrow {
  font-size: 10px;
  color: #9ca3af;
  transition: transform 0.3s;
}

.user-info:hover .dropdown-arrow {
  transform: rotate(180deg);
}

/* User Dropdown */
.user-dropdown {
  position: absolute;
  top: calc(100% + 8px);
  right: 0;
  min-width: 200px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 10px 40px rgba(0, 0, 0, 0.15);
  border: 1px solid #e5e7eb;
  overflow: hidden;
  z-index: 1000;
  animation: dropdownFadeIn 0.2s ease-out;
}

@keyframes dropdownFadeIn {
  from {
    opacity: 0;
    transform: translateY(-10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.dropdown-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px 16px;
  cursor: pointer;
  transition: all 0.2s;
  font-size: 14px;
  color: #374151;
}

.dropdown-item:hover {
  background: #f3f4f6;
}

.dropdown-item.logout {
  color: #dc2626;
}

.dropdown-item.logout:hover {
  background: #fef2f2;
}

.dropdown-icon {
  font-size: 16px;
}

.dropdown-divider {
  height: 1px;
  background: #e5e7eb;
  margin: 4px 0;
}

/* ===================================
   MAIN CONTENT
   =================================== */
.app-main {
  flex: 1;
  padding: var(--space-6) var(--space-5);
}

/* ===================================
   FOOTER STYLES
   =================================== */
.app-footer {
  --footer-bg: #f8f9fa;
  --footer-border: #e9ecef;
  --footer-text: #495057;
  --footer-title: #212529;
  --footer-text-muted: #6c757d;
  --footer-accent: #667eea;
  --footer-status-online: #10b981;
  --footer-status-offline: #dc3545;
  --status-text-color: #495057;

  background: var(--footer-bg);
  border-top: 1px solid var(--footer-border);
  color: var(--footer-text);
  margin-top: auto;
  transition: background-color 0.3s, color 0.3s;
}

.footer-container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 10px 24px;
}

/* Footer Content - Compact Horizontal Layout */
.footer-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;
}

/* Brand */
.footer-brand {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-shrink: 0;
}

.footer-logo {
  font-size: 20px;
  line-height: 1;
  filter: drop-shadow(0 2px 4px rgba(102, 126, 234, 0.2));
}

.footer-name {
  font-size: 14px;
  font-weight: 800;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

/* Links Container */
.footer-links-wrapper {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 20px;
  flex: 1;
  flex-wrap: wrap;
}

.footer-links-group {
  display: flex;
  align-items: center;
  gap: 12px;
}

.footer-group-divider {
  width: 1px;
  height: 16px;
  background: var(--footer-border, #dee2e6);
  flex-shrink: 0;
}

.footer-link-item {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 11px;
  color: var(--footer-text, #495057);
  cursor: pointer;
  transition: all 0.2s;
  white-space: nowrap;
}

.link-icon {
  font-size: 12px;
  transition: transform 0.2s;
}

.footer-link-item:hover {
  color: var(--footer-accent, #667eea);
  transform: translateY(-2px);
}

.footer-link-item:hover .link-icon {
  transform: scale(1.15);
}

/* Status Section */
.footer-status-section {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-shrink: 0;
}

.footer-status {
  display: flex;
  align-items: center;
  gap: 6px;
  padding-right: 12px;
  border-right: 1px solid var(--footer-border, #dee2e6);
}

.status-indicator {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: var(--footer-status-offline, #dc3545);
  transition: all 0.3s;
}

.status-indicator.online {
  background: var(--footer-status-online, #10b981);
  box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.5);
  animation: pulse 2s infinite;
}

.footer-copyright {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 10px;
  color: var(--footer-text-muted, #6c757d);
  white-space: nowrap;
}

.copyright-divider {
  color: var(--footer-text-muted, #adb5bd);
  font-weight: 600;
}

@keyframes pulse {
  0%, 100% { box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.5); }
  50% { box-shadow: 0 0 0 3px rgba(16, 185, 129, 0); }
}

/* ===================================
   DARK MODE SUPPORT
   =================================== */

/* Header Dark Mode */
[data-theme="dark"] .app-header {
  background: rgba(30, 30, 35, 0.95) !important;
  border-bottom-color: rgba(102, 126, 234, 0.2) !important;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3) !important;
}

[data-theme="dark"] .logo-subtitle {
  color: #9ca3af !important;
}

[data-theme="dark"] .nav-link {
  color: #e5e7eb !important;
  border-color: transparent !important;
}

[data-theme="dark"] .nav-link:hover {
  background: rgba(102, 126, 234, 0.15) !important;
  color: #a78bfa !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .nav-link.router-link-active {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%) !important;
  color: white !important;
}

/* Theme Toggle Button Dark Mode */
[data-theme="dark"] .theme-toggle-btn {
  background: rgba(40, 40, 45, 0.9) !important;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3) !important;
}

[data-theme="dark"] .theme-toggle-btn:hover {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%) !important;
  box-shadow: 0 6px 20px rgba(102, 126, 234, 0.4) !important;
}

/* Health Status Dark Mode */
[data-theme="dark"] .health-status {
  background: rgba(40, 40, 45, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #e5e7eb !important;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3) !important;
}

[data-theme="dark"] .health-status.online {
  background: rgba(34, 197, 94, 0.15) !important;
  border-color: rgba(34, 197, 94, 0.4) !important;
  color: #4ade80 !important;
}

[data-theme="dark"] .health-status:not(.online) {
  background: rgba(239, 68, 68, 0.15) !important;
  border-color: rgba(239, 68, 68, 0.4) !important;
  color: #f87171 !important;
}

/* Footer Dark Mode */
[data-theme="dark"] .app-footer {
  --footer-bg: rgba(30, 30, 35, 0.98);
  --footer-border: rgba(102, 126, 234, 0.2);
  --footer-text: #e5e7eb;
  --footer-title: #f3f4f6;
  --footer-text-muted: #9ca3af;
  --footer-accent: #a78bfa;
  --footer-status-online: #34d399;
  --footer-status-offline: #f87171;
  --status-text-color: #e5e7eb;
  background: var(--footer-bg) !important;
  border-top-color: var(--footer-border) !important;
  color: var(--footer-text) !important;
}

[data-theme="dark"] .footer-group-divider {
  background: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .footer-link-item {
  color: #9ca3af !important;
}

[data-theme="dark"] .footer-link-item:hover {
  color: #a78bfa !important;
}

[data-theme="dark"] .status-indicator.online {
  background: #34d399 !important;
}

[data-theme="dark"] .status-indicator:not(.online) {
  background: #f87171 !important;
}

[data-theme="dark"] .footer-status {
  border-right-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .footer-copyright {
  color: #9ca3af !important;
}

[data-theme="dark"] .copyright-divider {
  color: #6b7280 !important;
}

/* ===================================
   RESPONSIVE DESIGN
   =================================== */
@media (max-width: 1024px) {
  .app-container {
    padding: 0;
  }

  .header-content {
    flex-direction: column;
    gap: 16px;
    padding: 0 16px;
  }

  .nav-section {
    width: 100%;
    justify-content: space-between;
    flex-wrap: wrap;
  }

  .nav-links {
    flex: 1;
    justify-content: center;
    flex-wrap: nowrap;
    gap: 6px;
  }

  .nav-link {
    padding: 10px 16px;
    font-size: 14px;
  }

  .app-main {
    padding: 20px 16px;
  }

  .footer-container {
    padding: 8px 16px;
  }

  .footer-content {
    gap: 16px;
  }

  .footer-logo {
    font-size: 20px;
  }

  .footer-name {
    font-size: 14px;
  }

  .footer-links-group {
    gap: 12px;
  }

  .footer-link-item {
    font-size: 11px;
  }

  .link-icon {
    font-size: 12px;
  }

  .status-text {
    font-size: 11px;
  }

  .footer-copyright {
    font-size: 10px;
  }
}

@media (max-width: 768px) {
  .footer-content {
    flex-direction: column;
    align-items: flex-start;
    gap: 12px;
  }

  .footer-brand {
    width: 100%;
  }

  .footer-links-group {
    width: 100%;
    justify-content: flex-start;
  }

  .footer-status-section {
    width: 100%;
    align-items: flex-start;
  }
}

@media (max-width: 640px) {
  .app-header {
    padding: 12px 0;
  }

  .header-content {
    gap: 12px;
  }

  .nav-links {
    flex-wrap: nowrap;
    justify-content: center;
    gap: 6px;
  }

  .nav-text {
    display: none;
  }

  .app-main {
    padding: 16px 12px;
  }

  .footer-container {
    padding: 8px 12px;
  }

  .footer-logo {
    font-size: 18px;
  }

  .footer-name {
    font-size: 13px;
  }

  .footer-links-group {
    gap: 10px;
  }

  .footer-link-item {
    font-size: 10px;
  }

  .link-icon {
    font-size: 11px;
  }

  .status-text {
    font-size: 10px;
  }

  .footer-copyright {
    font-size: 9px;
  }
}

@media (max-width: 640px) {
  .app-header {
    padding: 12px 0;
  }

  .header-content {
    gap: 12px;
  }

  .nav-links {
    flex-wrap: nowrap;
    justify-content: center;
    gap: 6px;
  }

  .nav-text {
    display: none;
  }

  .app-main {
    padding: 16px 12px;
  }

  .footer-container {
    padding: 10px 12px;
  }

  .footer-logo {
    font-size: 18px;
  }

  .footer-brand-text {
    gap: 1px;
  }

  .footer-name {
    font-size: 13px;
  }

  .footer-tagline {
    font-size: 10px;
  }

  .footer-link {
    font-size: 11px;
  }

  .link-icon {
    font-size: 11px;
  }

  .status-text {
    font-size: 10px;
  }

  .footer-info {
    font-size: 10px;
  }
}
</style>
