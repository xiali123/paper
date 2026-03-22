import { createApp } from 'vue'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import 'element-plus/theme-chalk/dark/css-vars.css'
import * as ElementPlusIconsVue from '@element-plus/icons-vue'
import App from './App.vue'
import router from './router' // Import router configuration
import './assets/styles/design-system.css' // Premium design system
import './assets/theme.css' // Theme colors
import i18n from './i18n' // Import i18n configuration
import { setupStore } from './stores' // Import Pinia store setup

const app = createApp(App)

// Register Element Plus
app.use(ElementPlus)

// Register all Element Plus icons
for (const [key, component] of Object.entries(ElementPlusIconsVue)) {
  app.component(key, component)
}

// Setup Pinia store
setupStore(app)

app.use(router)
app.use(i18n)
app.mount('#app')
