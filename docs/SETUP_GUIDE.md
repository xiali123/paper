# PaperCrawler - 前端开发环境快速设置指南

> 更新时间：2026-03-22

---

## 📋 前置要求

- **Node.js** 18.0+ （推荐使用 LTS 版本）
- **npm** 9.0+ 或 **yarn** / **pnpm**
- **Git** （用于版本控制）

### 检查 Node.js 版本

```bash
node --version
```

如果未安装，请访问：https://nodejs.org/

---

## 🚀 快速开始

### 1. 安装依赖

```bash
cd frontend
npm install
```

这将安装所有必需的依赖，包括：
- Vue 3.4+ - 前端框架
- Vue Router 4 - 路由管理
- Pinia 3 - 状态管理
- Vue i18n 9 - 国际化
- Element Plus 2 - UI 组件库
- Axios - HTTP 客户端
- sass-embedded - SASS/SCSS 预处理器
- TypeScript 5 - 类型支持
- Vite 5 - 构建工具

### 2. 启动开发服务器

```bash
npm run dev
```

服务器将在 `http://localhost:5173` 启动

### 3. 启动 Mock API（可选）

在另一个终端窗口：

```bash
# 在项目根目录
node complete-mock-api.js
```

Mock API 将在 `http://localhost:8082` 运行

---

## 📦 依赖说明

### 核心依赖

| 依赖 | 版本 | 用途 |
|------|------|------|
| vue | ^3.4.21 | 核心框架 |
| vue-router | ^4.3.0 | 路由管理 |
| pinia | ^3.0.4 | 状态管理 |
| vue-i18n | ^9.14.5 | 国际化 |
| element-plus | ^2.13.6 | UI 组件库 |
| @element-plus/icons-vue | ^2.3.2 | Element Plus 图标 |
| axios | ^1.13.6 | HTTP 客户端 |
| chart.js | ^4.5.1 | 图表库 |
| vue-chartjs | ^5.3.3 | Vue 图表封装 |

### 开发依赖

| 依赖 | 版本 | 用途 |
|------|------|------|
| vite | ^5.2.0 | 构建工具 |
| @vitejs/plugin-vue | ^5.0.4 | Vue 3 插件 |
| sass-embedded | 最新 | SASS/SCSS 预处理器 |
| typescript | 5.0+ | 类型检查（内置） |

---

## 🔧 常见问题

### Q1: 出现 "Cannot find module" 错误

**解决方案**：删除 node_modules 并重新安装

```bash
cd frontend
rm -rf node_modules package-lock.json  # Linux/Mac
# 或
RMDIR /S /Q node_modules
DEL package-lock.json  # Windows

npm install
```

### Q2: Vite 启动失败，端口被占用

**解决方案**：修改端口或终止占用进程

```bash
# 查找占用 5173 端口的进程
netstat -ano | findstr :5173

# 终止进程（替换 PID）
taskkill /PID <PID> /F
```

或修改 `vite.config.ts`：

```typescript
export default defineConfig({
  server: {
    port: 3000  // 使用其他端口
  }
})
```

### Q3: Element Plus 组件无法解析

**解决方案**：确保安装了 element-plus

```bash
npm install element-plus @element-plus/icons-vue
```

### Q4: SCSS 样式无法编译

**解决方案**：安装 sass-embedded

```bash
npm install -D sass-embedded
```

### Q5: TypeScript 类型错误

**解决方案**：重启 TypeScript 服务器

VS Code 中：
1. 按 `Ctrl+Shift+P`
2. 输入 "TypeScript: Restart TS Server"

---

## 📁 项目结构

```
frontend/
├── public/                 # 静态资源
├── src/
│   ├── assets/            # 资源文件（样式、图片等）
│   ├── components/        # Vue 组件
│   │   ├── common/       # 通用组件
│   │   ├── layout/       # 布局组件
│   │   └── paper/        # 论文相关组件
│   ├── i18n/             # 国际化配置
│   ├── router/           # 路由配置
│   ├── stores/           # Pinia stores
│   ├── views/            # 页面组件
│   ├── api/              # API 模块
│   ├── composables/      # 组合式函数
│   ├── types/            # TypeScript 类型
│   ├── utils/            # 工具函数
│   ├── App.vue           # 根组件
│   └── main.ts           # 入口文件
├── index.html            # HTML 模板
├── vite.config.ts        # Vite 配置
├── tsconfig.json         # TypeScript 配置
└── package.json          # 项目配置
```

---

## 🎯 开发命令

```bash
# 启动开发服务器
npm run dev

# 构建生产版本
npm run build

# 预览生产构建
npm run preview

# 类型检查
npm run type-check

# 代码检查（如果配置了）
npm run lint
```

---

## 🔌 API 配置

默认 API 地址：`http://localhost:8080`

修改 API 地址，编辑 `src/utils/request.ts`：

```typescript
const api = axios.create({
  baseURL: 'http://your-api-url:port',
  timeout: 30000
})
```

---

## 🌐 国际化

项目支持中文和英文：

- 中文：`src/i18n/locales/zh-CN.json`
- 英文：`src/i18n/locales/en-US.json`

添加新翻译：

1. 在两个语言文件中添加相同的键
2. 在组件中使用：`{{ $t('key.path') }}`

---

## 🎨 主题定制

项目支持亮色和暗色主题：

- 主题 CSS：`src/assets/theme.css`
- 设计系统：`src/assets/styles/design-system.css`

修改主题颜色，编辑 `theme.css` 中的 CSS 变量。

---

## 📱 浏览器支持

- Chrome >= 90
- Firefox >= 88
- Safari >= 14
- Edge >= 90

---

## 🚢 生产构建

### 构建

```bash
npm run build
```

构建产物将生成在 `frontend/dist/` 目录

### 部署

1. 将 `dist/` 目录的内容上传到服务器
2. 配置服务器（Nginx/Apache）支持 SPA 路由

**Nginx 配置示例**：

```nginx
location / {
    try_files $uri $uri/ /index.html;
}
```

---

## 🔍 调试技巧

### Vue DevTools

安装浏览器扩展：
- Chrome: Vue.js devtools
- Firefox: Vue.js devtools

### 控制台日志

```typescript
console.log('变量值:', variable)
console.table(array)
console.trace('调用栈')
```

### Network 检查

打开浏览器开发者工具 > Network 标签：
- 查看 API 请求
- 检查响应数据
- 监控加载时间

---

## 📚 相关文档

- **Vue 3 文档**: https://vuejs.org/
- **Element Plus 文档**: https://element-plus.org/
- **Pinia 文档**: https://pinia.vuejs.org/
- **Vue Router 文档**: https://router.vuejs.org/
- **Vite 文档**: https://vitejs.dev/

---

## 🆘 获取帮助

遇到问题？

1. 查看本文档的"常见问题"部分
2. 查看项目 README.md
3. 查看 DEVELOPMENT_PROGRESS.md
4. 提交 Issue

---

**最后更新**: 2026-03-22
**维护者**: PaperCrawler Development Team
