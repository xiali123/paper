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
      <footer class="app-footer">
        <div class="footer-content">
          <div class="footer-main">
            <div class="footer-brand">
              <div class="footer-logo">
                <span class="footer-logo-icon">📚</span>
                <span class="footer-logo-text">PaperCrawler</span>
              </div>
              <p class="footer-description">高效的学术论文检索与分析平台</p>
            </div>

            <div class="footer-links">
              <div class="footer-link-group">
                <h4 class="footer-link-title">功能</h4>
                <a href="#search" class="footer-link">论文搜索</a>
                <a href="#stats" class="footer-link">数据统计</a>
              </div>
              <div class="footer-link-group">
                <h4 class="footer-link-title">关于</h4>
                <a href="#" class="footer-link">使用说明</a>
                <a href="#" class="footer-link">技术支持</a>
              </div>
            </div>
          </div>

          <div class="footer-bottom">
            <div class="footer-bottom-left">
              <p class="footer-copyright">{{ $t('footer.copyright') }}</p>
              <p class="footer-powered">{{ $t('footer.poweredBy') }}</p>
            </div>
            <div v-if="backendStatus" class="footer-status">
              <span class="status-dot status-ok"></span>
              <span class="status-text">{{ $t('backend.connected') }}</span>
            </div>
          </div>
        </div>
      </footer>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import { useTheme } from './composables/useTheme'
import LanguageSwitcher from './components/LanguageSwitcher.vue'

const { theme, toggleTheme, isDark } = useTheme()
const backendStatus = ref(false)
const healthCheckInitialized = ref(false)

const checkBackend = async () => {
  console.log('Checking backend health...')
  try {
    // 使用代理路径 /api/health
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
  // 延迟1秒后首次检查，确保页面完全加载
  setTimeout(() => {
    checkBackend()
    // 每30秒检查一次
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
  max-width: 1400px;
  margin-left: auto;
  margin-right: auto;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  padding-left: var(--space-5);
  padding-right: var(--space-5);
}

/* ===================================
   HEADER STYLES
   =================================== */
.app-header {
  background: var(--bg-overlay);
  padding: var(--space-4) 0;
  box-shadow: var(--shadow-sm);
  backdrop-filter: blur(20px);
  border-bottom: 1px solid var(--border-primary);
  position: sticky;
  top: 0;
  z-index: var(--z-sticky);
}

.header-content {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: var(--space-8);
}

/* Logo Section */
.logo-section {
  flex-shrink: 0;
}

.logo-wrapper {
  display: flex;
  align-items: center;
  gap: var(--space-3);
}

.logo-icon {
  font-size: var(--font-3xl);
  filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.1));
}

.logo-text {
  display: flex;
  flex-direction: column;
  gap: var(--space-1);
}

.logo-title {
  margin: 0;
  font-size: var(--font-2xl);
  font-weight: var(--font-bold);
  background: var(--bg-gradient-hero);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  line-height: 1;
  letter-spacing: var(--tracking-tight);
}

.logo-subtitle {
  margin: 0;
  font-size: var(--font-xs);
  color: var(--text-secondary);
  font-weight: var(--font-medium);
}

/* Navigation Section */
.nav-section {
  display: flex;
  align-items: center;
  gap: var(--space-6);
}

.nav-links {
  display: flex;
  gap: var(--space-2);
}

.nav-link {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  text-decoration: none;
  color: var(--text-primary);
  font-weight: var(--font-semibold);
  padding: var(--space-3) var(--space-4);
  border-radius: var(--radius-lg);
  transition: all var(--duration-normal);
  position: relative;
  font-size: var(--font-sm);
}

.nav-icon {
  font-size: var(--font-lg);
  opacity: 0.7;
}

.nav-text {
  font-size: var(--font-sm);
}

.nav-link:hover {
  background: var(--bg-tertiary);
  color: var(--color-primary-600);
  transform: translateY(-1px);
}

.nav-link.router-link-active {
  background: var(--color-primary-600);
  color: white;
  box-shadow: var(--shadow-primary);
}

.nav-link.router-link-active .nav-icon {
  opacity: 1;
}

/* Navigation Controls */
.nav-controls {
  display: flex;
  align-items: center;
  gap: var(--space-3);
}

.theme-toggle-btn {
  width: 40px;
  height: 40px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--bg-secondary);
  border: 2px solid var(--border-primary);
  border-radius: var(--radius-lg);
  font-size: var(--font-xl);
  cursor: pointer;
  transition: all var(--duration-normal);
}

.theme-toggle-btn:hover {
  background: var(--bg-tertiary);
  border-color: var(--color-primary-500);
  transform: scale(1.05);
}

.theme-icon {
  filter: drop-shadow(0 1px 2px rgba(0, 0, 0, 0.1));
}

/* ===================================
   MAIN CONTENT
   =================================== */
.app-main {
  flex: 1;
  padding: var(--space-8) 0;
}

/* ===================================
   FOOTER STYLES
   =================================== */
.app-footer {
  background: var(--bg-overlay);
  border-top: 1px solid var(--border-primary);
  backdrop-filter: blur(20px);
  margin-top: auto;
}

.footer-content {
  padding: var(--space-8) 0;
}

.footer-main {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: var(--space-8);
  margin-bottom: var(--space-6);
}

/* Footer Brand */
.footer-brand {
  display: flex;
  flex-direction: column;
  gap: var(--space-3);
}

.footer-logo {
  display: flex;
  align-items: center;
  gap: var(--space-2);
}

.footer-logo-icon {
  font-size: var(--font-size-2xl);
}

.footer-logo-text {
  font-size: var(--font-lg);
  font-weight: var(--font-bold);
  color: var(--text-primary);
}

.footer-description {
  font-size: var(--font-sm);
  color: var(--text-secondary);
  margin: 0;
  max-width: var(--container-lg);
  line-height: var(--leading-relaxed);
}

/* Footer Links */
.footer-links {
  display: flex;
  gap: var(--space-8);
  justify-content: flex-end;
}

.footer-link-group {
  display: flex;
  flex-direction: column;
  gap: var(--space-3);
}

.footer-link-title {
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  color: var(--text-primary);
  margin: 0;
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.footer-link {
  font-size: var(--font-sm);
  color: var(--text-secondary);
  text-decoration: none;
  transition: all var(--duration-fast);
}

.footer-link:hover {
  color: var(--color-primary-600);
  transform: translateX(2px);
}

/* Footer Bottom */
.footer-bottom {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-top: var(--space-6);
  border-top: 1px solid var(--color-border-primary);
}

.footer-bottom-left {
  display: flex;
  flex-direction: column;
  gap: var(--space-1);
}

.footer-copyright,
.footer-powered {
  font-size: var(--font-xs);
  color: var(--text-secondary);
  margin: 0;
}

.footer-powered {
  opacity: 0.8;
}

.footer-status {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-3);
  background: rgba(16, 185, 129, 0.1);
  border-radius: var(--radius-full);
}

.status-dot {
  width: var(--space-2);
  height: var(--space-2);
  border-radius: 50%;
  animation: pulse 2s infinite;
}

.status-ok {
  background: var(--color-success);
  box-shadow: 0 0 8px rgba(16, 185, 129, 0.5);
}

.status-error {
  background: var(--color-error);
  box-shadow: 0 0 8px rgba(239, 68, 68, 0.5);
}

.status-text {
  font-size: var(--font-size-xs);
  font-weight: var(--font-weight-semibold);
  color: var(--color-success);
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.5;
  }
}

/* ===================================
   RESPONSIVE DESIGN
   =================================== */
@media (max-width: 1024px) {
  .app-container {
    padding-left: var(--space-4);
    padding-right: var(--space-4);
  }

  .header-content {
    flex-direction: column;
    gap: var(--space-4);
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

  .footer-main {
    grid-template-columns: 1fr;
    gap: var(--space-6);
  }

  .footer-links {
    justify-content: flex-start;
  }
}

@media (max-width: 640px) {
  .nav-links {
    flex-wrap: wrap;
    justify-content: center;
  }

  .nav-text {
    display: none;
  }

  .footer-bottom {
    flex-direction: column;
    gap: var(--space-4);
    text-align: center;
  }
}
</style>
