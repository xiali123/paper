<template>
  <div id="app">
    <div class="app-container">
      <header class="app-header">
        <div class="logo">
          <h1>📚 PaperCrawler</h1>
          <p>Academic Paper Search & Analysis Platform</p>
        </div>
        <nav class="nav">
          <router-link to="/" class="nav-link">首页</router-link>
          <router-link to="/search" class="nav-link">搜索</router-link>
          <router-link to="/stats" class="nav-link">统计</router-link>
          <button @click="toggleTheme" class="theme-toggle" :title="isDark() ? 'Switch to Light Mode' : 'Switch to Dark Mode'">
            {{ isDark() ? '☀️' : '🌙' }}
          </button>
        </nav>
      </header>

      <main class="app-main">
        <router-view />
      </main>

      <footer class="app-footer">
        <p>&copy; 2024 PaperCrawler | Powered by Vue 3 + C++ REST API</p>
        <p>Backend Status: <span :class="{ 'status-ok': backendStatus, 'status-error': !backendStatus }">{{ backendStatus ? 'Connected' : 'Disconnected' }}</span></p>
      </footer>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useTheme } from './composables/useTheme'

const { theme, toggleTheme, isDark } = useTheme()
const backendStatus = ref(false)

const checkBackend = async () => {
  try {
    const response = await fetch('http://localhost:8080/health')
    backendStatus.value = response.ok
  } catch {
    backendStatus.value = false
  }
}

onMounted(() => {
  checkBackend()
  setInterval(checkBackend, 10000)
})
</script>

<style scoped>
#app {
  min-height: 100vh;
  background: var(--color-bg-gradient);
}

.app-container {
  max-width: 1400px;
  margin: 0 auto;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.app-header {
  background: var(--color-bg-overlay);
  padding: 20px 40px;
  box-shadow: var(--shadow-md);
  backdrop-filter: blur(10px);
}

.logo h1 {
  margin: 0;
  font-size: 32px;
  color: var(--color-primary);
}

.logo p {
  margin: 5px 0 0 0;
  font-size: 14px;
  color: var(--color-text-secondary);
}

.nav {
  margin-top: 20px;
  display: flex;
  gap: 30px;
  align-items: center;
}

.nav-link {
  text-decoration: none;
  color: var(--color-text-primary);
  font-weight: 500;
  padding: 8px 16px;
  border-radius: 8px;
  transition: all var(--transition-normal);
}

.nav-link:hover {
  background: var(--color-bg-tertiary);
  color: var(--color-primary);
}

.nav-link.router-link-active {
  background: var(--color-primary);
  color: var(--color-text-inverse);
}

.theme-toggle {
  background: var(--color-bg-secondary);
  border: 2px solid var(--color-border-primary);
  border-radius: 8px;
  padding: 8px 16px;
  font-size: 20px;
  cursor: pointer;
  transition: all var(--transition-normal);
  margin-left: auto;
}

.theme-toggle:hover {
  background: var(--color-bg-tertiary);
  transform: scale(1.05);
}

.app-main {
  flex: 1;
  padding: 40px 20px;
}

.app-footer {
  background: var(--color-bg-overlay);
  padding: 20px;
  text-align: center;
  font-size: 14px;
  color: var(--color-text-secondary);
  border-top: 1px solid var(--color-border-primary);
  backdrop-filter: blur(10px);
}

.status-ok {
  color: var(--color-success);
  font-weight: 600;
}

.status-error {
  color: var(--color-error);
  font-weight: 600;
}
</style>
