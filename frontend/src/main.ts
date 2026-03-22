import { createApp } from 'vue'
import App from './App.vue'
import router from './router' // Import router configuration
import './assets/styles/design-system.css' // Premium design system
import './assets/theme.css' // Theme colors
import i18n from './i18n' // Import i18n configuration
import { setupStore } from './stores' // Import Pinia store setup

const app = createApp(App)

// Setup Pinia store
setupStore(app)

app.use(router)
app.use(i18n)
app.mount('#app')
