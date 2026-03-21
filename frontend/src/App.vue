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
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

.app-container {
  max-width: 1400px;
  margin: 0 auto;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.app-header {
  background: rgba(255, 255, 255, 0.98);
  padding: 20px 40px;
  box-shadow: 0 2px 20px rgba(0, 0, 0, 0.1);
}

.logo h1 {
  margin: 0;
  font-size: 32px;
  color: #667eea;
}

.logo p {
  margin: 5px 0 0 0;
  font-size: 14px;
  color: #666;
}

.nav {
  margin-top: 20px;
  display: flex;
  gap: 30px;
}

.nav-link {
  text-decoration: none;
  color: #333;
  font-weight: 500;
  padding: 8px 16px;
  border-radius: 8px;
  transition: all 0.3s;
}

.nav-link:hover {
  background: #f3f4f6;
  color: #667eea;
}

.nav-link.router-link-active {
  background: #667eea;
  color: white;
}

.app-main {
  flex: 1;
  padding: 40px 20px;
}

.app-footer {
  background: rgba(255, 255, 255, 0.95);
  padding: 20px;
  text-align: center;
  font-size: 14px;
  color: #666;
  border-top: 1px solid #e5e7eb;
}

.status-ok {
  color: #10b981;
  font-weight: 600;
}

.status-error {
  color: #ef4444;
  font-weight: 600;
}
</style>
