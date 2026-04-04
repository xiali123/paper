# PaperCrawler Frontend

基于 Vue 3 + TypeScript + Vite + Element Plus 的现代化前端项目

## 技术栈

- **框架**: Vue 3.4+ (Composition API)
- **语言**: TypeScript 5.3+
- **构建工具**: Vite 5.0+
- **UI组件库**: Element Plus 2.13+
- **状态管理**: Pinia 3.0+
- **路由**: Vue Router 4.3+
- **HTTP客户端**: Axios 1.6+
- **图表库**: Chart.js 4.5+

## 项目结构

```
frontend/
├── public/                 # 静态资源
├── src/
│   ├── assets/            # 资源文件
│   ├── components/        # 组件
│   │   ├── common/        # 通用组件
│   │   └── layout/        # 布局组件
│   ├── views/             # 页面组件
│   │   ├── login/         # 登录页
│   │   ├── dashboard/     # 仪表盘
│   │   ├── papers/        # 论文管理
│   │   ├── crawler/       # 爬虫配置
│   │   ├── search/        # 搜索
│   │   ├── settings/      # 设置
│   │   └── error/         # 错误页
│   ├── stores/            # Pinia状态管理
│   ├── services/          # API服务
│   ├── types/             # TypeScript类型
│   ├── utils/             # 工具函数
│   ├── router/            # 路由配置
│   ├── styles/            # 样式文件
│   ├── App.vue            # 根组件
│   └── main.ts            # 入口文件
├── package.json
├── vite.config.ts         # Vite配置
├── tsconfig.json          # TypeScript配置
└── index.html
```

## 开发指南

### 安装依赖

```bash
npm install
```

### 启动开发服务器

```bash
npm run dev
```

访问 http://localhost:3000

### 构建生产版本

```bash
npm run build
```

### 预览生产版本

```bash
npm run preview
```

## 主要功能

- 用户登录认证
- 仪表盘数据展示
- 论文管理和搜索
- 爬虫配置和监控
- 系统设置

## 开发规范

- 使用 Composition API 编写组件
- 使用 TypeScript 编写类型安全的代码
- 遵循 ESLint 和 Prettier 代码规范
- 组件命名使用 PascalCase
- 文件命名使用 PascalCase（组件）或 kebab-case（工具类）

## 环境变量

- `.env.development` - 开发环境变量
- `.env.production` - 生产环境变量

主要配置项：
- `VITE_APP_API_BASE_URL` - API基础URL
- `VITE_APP_API_TIMEOUT` - API请求超时时间
- `VITE_APP_ENABLE_MOCK` - 是否启用Mock数据

## 浏览器支持

- Chrome >= 87
- Firefox >= 78
- Safari >= 14
- Edge >= 88

## License

MIT
