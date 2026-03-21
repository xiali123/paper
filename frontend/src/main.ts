import { createApp } from 'vue'
import { createRouter, createWebHistory } from 'vue-router'
import App from './App.vue'
import Home from './views/Home.vue'
import Search from './views/Search.vue'
import Stats from './views/Stats.vue'
import './assets/theme.css'
import './i18n' // Import i18n configuration

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
app.use(router)
app.mount('#app')
