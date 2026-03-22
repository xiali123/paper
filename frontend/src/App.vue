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
              <router-link to="/search" class="nav-link">
                <span class="nav-icon">🔍</span>
                <span class="nav-text">{{ $t('nav.search') }}</span>
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
import { useTheme } from './composables/useTheme'
import LanguageSwitcher from './components/LanguageSwitcher.vue'

const { theme, toggleTheme, isDark } = useTheme()
const backendStatus = ref(false)
const healthCheckInitialized = ref(false)

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
      const data = await response.json()
      console.log('Health check data:', data)
      backendStatus.value = data.status === 'ok'
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
  setTimeout(() => {
    checkBackend()
    healthCheckTimer = setInterval(checkBackend, 30000)
  }, 1000)
})

onUnmounted(() => {
  if (healthCheckTimer) {
    clearInterval(healthCheckTimer)
  }
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
}

.nav-icon {
  font-size: 18px;
  transition: all 0.3s;
}

.nav-text {
  font-size: 15px;
  font-weight: 500;
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

/* Dark Mode Support */
:deep(.dark) .app-footer {
  --footer-bg: #1a1a1a;
  --footer-border: #2d2d2d;
  --footer-text: #e9ecef;
  --footer-title: #f8f9fa;
  --footer-text-muted: #adb5bd;
  --footer-accent: #a78bfa;
  --footer-status-online: #34d399;
  --footer-status-offline: #f87171;
  --status-text-color: #e9ecef;
  background: #1a1a1a;
  border-top-color: #2d2d2d;
}

:deep(.dark) .status-indicator.online {
  background: #34d399;
}

:deep(.dark) .status-indicator:not(.online) {
  background: #f87171;
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
    flex-wrap: wrap;
    justify-content: center;
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
    flex-wrap: wrap;
    justify-content: center;
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
