import { createApp } from 'vue'
import { createPinia } from 'pinia'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import 'element-plus/theme-chalk/dark/css-vars.css'
import * as ElementPlusIconsVue from '@element-plus/icons-vue'
import zhCn from 'element-plus/dist/locale/zh-cn.mjs'

import App from './App.vue'
import router from './router'
import i18n from './i18n'

import './styles/index.scss'

// 自定义指令
import clickOutsideDirective from './directives/clickOutside'

const app = createApp(App)

const pinia = createPinia()
app.use(pinia)

app.use(router)
app.use(i18n)

app.use(ElementPlus, {
  locale: zhCn,
})

// 注册全局指令
app.directive('click-outside', clickOutsideDirective)

for (const [key, component] of Object.entries(ElementPlusIconsVue)) {
  app.component(key, component)
}

app.mount('#app')
