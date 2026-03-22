# PaperCrawler 前端项目文件清单

## 核心源文件

### API 层
```
src/api/
├── index.ts                    # API 统一导出入口
└── modules/                    # API 功能模块
    ├── paper.ts               # 论文搜索、详情、列表 API
    ├── stats.ts               # 统计数据 API
    ├── export.ts              # 数据导出 API
    └── health.ts              # 健康检查 API
```

### Composables (Vue 组合式函数)
```
src/composables/
├── index.ts                    # 统一导出
├── useSearch.ts               # 搜索功能 composable
├── useStats.ts                # 统计功能 composable
├── usePaper.ts                # 论文详情 composable
└── useHealthCheck.ts          # 健康检查 composable
```

### 工具函数
```
src/utils/
├── index.ts                    # 统一导出
├── request.ts                 # Axios 配置和拦截器
├── debounce.ts                # 防抖和节流函数
├── format.ts                  # 格式化函数（数字、日期等）
└── validate.ts                # 验证函数
```

### 类型定义
```
src/types/
└── paper.ts                   # 论文相关 TypeScript 类型
```

### 页面组件
```
src/views/
├── Home.vue                   # 首页（快速搜索）
├── Search.vue                 # 搜索页（高级搜索）
├── Stats.vue                  # 统计页（数据展示）
└── PaperDetail.vue            # 论文详情页
```

### 路由配置
```
src/router/
└── index.ts                   # Vue Router 配置
```

## 配置文件

### 环境配置
```
.env.development               # 开发环境变量
.env.production                # 生产环境变量
```

### 构建配置
```
vite.config.ts                 # Vite 构建配置
tsconfig.json                  # TypeScript 配置
package.json                   # 项目依赖
```

## 文档文件

```
README_API_INTEGRATION.md      # API 集成详细文档
QUICK_START.md                 # 快速开始指南
INTEGRATION_TEST.md            # 集成测试指南
IMPLEMENTATION_SUMMARY.md      # 实现总结
```

## 启动脚本

```
start.bat                      # Windows 启动脚本
start.sh                       # Linux/Mac 启动脚本
```

## 文件统计

- **TypeScript 文件**: 15+ 个
- **Vue 组件**: 4 个主要页面
- **工具函数**: 20+ 个
- **API 端点**: 20+ 个
- **TypeScript 类型**: 10+ 个接口
- **文档页面**: 4 个详细文档

## 关键功能实现

### 1. 数据获取
- ✅ 论文搜索（关键词、年份、等级过滤）
- ✅ 论文详情获取
- ✅ 统计数据获取
- ✅ 健康检查

### 2. 状态管理
- ✅ 加载状态
- ✅ 错误状态
- ✅ 数据状态
- ✅ 分页状态

### 3. 用户体验
- ✅ 加载指示器
- ✅ 错误提示
- ✅ 防抖搜索
- ✅ 实时反馈

### 4. 性能优化
- ✅ 请求重试机制
- ✅ 防抖优化
- ✅ 代码分割
- ✅ 按需加载

### 5. 错误处理
- ✅ 网络错误处理
- ✅ 服务器错误处理
- ✅ 用户友好提示
- ✅ 自动重试

## 使用流程

1. **安装依赖**: `npm install`
2. **配置环境**: 编辑 `.env.development`
3. **启动开发**: `npm run dev` 或使用启动脚本
4. **访问应用**: `http://localhost:5173`
5. **测试功能**: 参考 `INTEGRATION_TEST.md`

## 技术栈

- **框架**: Vue 3 (Composition API)
- **语言**: TypeScript
- **构建**: Vite
- **HTTP**: Axios
- **路由**: Vue Router
- **状态**: Composables (自定义 hooks)

## 浏览器支持

- Chrome >= 90
- Firefox >= 88
- Safari >= 14
- Edge >= 90

---

**项目状态**: ✅ 完成并可用
**最后更新**: 2026-03-21
**维护状态**: 活跃开发中
