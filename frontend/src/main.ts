import { createApp } from 'vue'
import { createRouter, createWebHistory } from 'vue-router'
import App from './App.vue'
import Home from './views/Home.vue'
import Search from './views/Search.vue'
import Stats from './views/Stats.vue'
import './assets/styles/design-system.css' // Premium design system
import './assets/theme.css' // Theme colors
import i18n from './i18n' // Import i18n configuration
import { setupStore } from './stores' // Import Pinia store setup

const routes = [
  { path: '/', component: Home },
  { path: '/search', component: Search },
  { path: '/stats', component: Stats }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

const app = createApp(App)

// Setup Pinia store
setupStore(app)

app.use(router)
app.use(i18n)
app.mount('#app')
