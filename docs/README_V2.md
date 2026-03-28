# PaperCrawler v2.0 - 智研论文

> 🎓 **AI驱动的多用户论文管理协作平台**

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-2.0.0-green.svg)](https://github.com/your-org/papercrawler)
[![Status](https://img.shields.io/badge/status-In%20Development-yellow.svg)](#)

---

## 📖 项目简介

PaperCrawler 是一个面向科研人员、学生、职场研究者的一站式论文管理工具，实现：

```
自动爬取论文 → 本地AI智能解析PDF → 多端同步存储 → 分级权限管理 → VIP增值功能
```

### 🎯 核心功能

- 🔍 **多源爬虫** - 支持知网、IEEE、ArXiv、PubMed等主流学术平台
- 🤖 **AI智能解析** - 本地Claude Code PDF梳理，保护隐私
- 📝 **笔记系统** - Markdown笔记、批注、高亮、导出
- 💾 **双存储** - 本地缓存 + 云端同步，离线可用
- 👥 **多用户权限** - 4级角色体系（user → premium → admin → superadmin）
- 💎 **VIP增值** - 解锁无限爬虫、AI解析、大容量存储

---

## 🚀 快速开始

### 环境要求

- **Node.js**: >= 16.0
- **MySQL**: >= 8.0
- **C++编译器**: 支持 C++17
- **Python**: >= 3.8（AI解析模块）

### 5分钟快速启动

#### 1. 克隆项目
```bash
git clone https://github.com/your-org/papercrawler.git
cd papercrawler
```

#### 2. 初始化数据库
```bash
# 创建MySQL数据库
mysql -u root -p -e "CREATE DATABASE papercrawler CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"

# 导入完整架构
mysql -u root -p papercrawler < database/complete-schema-mysql.sql

# 应用索引优化
mysql -u root -p papercrawler < database/indexes-optimization-guide.sql
```

#### 3. 启动后端服务
```bash
# 方式1：Mock API（快速原型）
node complete-mock-api.js

# 方式2：C++服务器（生产环境）
cd backend
mkdir -p build && cd build
cmake ..
make
./api_server
```

#### 4. 启动前端服务
```bash
cd frontend
npm install
npm run dev
```

#### 5. 访问应用
打开浏览器访问：http://localhost:5173

**默认测试账号：**
- 超级管理员: `superadmin@papercrawler.local` / `SuperAdmin123!`
- 管理员: `admin@papercrawler.local` / `Admin123!`
- VIP用户: `premium@example.com` / `Premium123!`
- 普通用户: `x2830540584@163.com` / `Xl1234567890*#`

---

## 📚 完整文档

### 🎯 新手入门

1. **[立即行动计划](ACTION_PLAN.md)** - 从这里开始！
   - 5步快速启动指南
   - 本周任务清单
   - 常用命令速查

2. **[项目实施路线图](PROJECT_ROADMAP.md)** - 21周完整开发计划
   - 9个实施阶段
   - 里程碑和时间表
   - 资源需求评估

### 📐 架构设计

3. **[系统架构分析](ARCHITECTURE-ANALYSIS-REPORT.md)** - 软件架构专家设计
   - 从单体到微服务的演进
   - 技术选型和决策记录
   - 性能和可扩展性

4. **[数据库设计文档](database/DATABASE_DOCUMENTATION.md)** - 数据库专家设计
   - 40+ 张表的完整架构
   - 索引优化策略
   - MySQL和SQLite双存储

### 💻 开发指南

5. **[前端实现计划](FRONTEND_IMPLEMENTATION_PLAN.md)** - 前端专家设计
   - 40+ 路由完整设计
   - 组件树和状态管理
   - 8周开发时间表

6. **[前端组件规范](FRONTEND_COMPONENT_SPECIFICATIONS.md)** - 组件开发规范
   - 布局组件
   - 论文管理组件
   - AI交互组件

7. **[前端快速参考](FRONTEND_QUICK_REFERENCE.md)** - 前端开发速查
   - 路由使用
   - Store模式
   - 权限控制

### 🤖 AI功能

8. **[AI PDF解析设计](AI_PDF_PARSING_MODULE_DESIGN.md)** - AI专家设计
   - Claude API集成方案
   - PDF解析技术选型
   - 成本优化策略

9. **[AI实现指南](AI_PDF_IMPLEMENTATION_GUIDE.md)** - AI模块开发
   - 环境搭建
   - 代码示例
   - 测试策略

10. **[AI提示词库](AI_PDF_PROMPT_TEMPLATES.md)** - Claude提示词
    - 学术论文总结
    - 研究脉络梳理
    - 问答交互模板

11. **[AI快速开始](AI_PDF_QUICK_START.md)** - AI功能快速上手
    - 5分钟入门
    - 成本计算
    - 性能基准

### 🔄 数据同步

12. **[数据同步架构](SYNC_ARCHITECTURE.md)** - 数据专家设计
    - 双向同步协议
    - 冲突解决策略
    - 离线队列设计

13. **[同步实现指南](SYNC_IMPLEMENTATION_GUIDE.md)** - 同步模块开发
    - 服务器端实现
    - 客户端引擎
    - 冲突解决UI

14. **[同步快速参考](SYNC_QUICK_REFERENCE.md)** - 同步功能速查
    - API端点
    - 数据库Schema
    - 监控指标

### 🗄️ 数据库

15. **[数据库README](database/README.md)** - 数据库快速开始
    - 初始化步骤
    - 迁移脚本
    - 性能调优

16. **[MySQL完整架构](database/complete-schema-mysql.sql)** - 云端数据库
    - 40+ 表定义
    - 外键和触发器
    - 存储过程

17. **[SQLite完整架构](database/complete-schema-sqlite.sql)** - 本地数据库
    - 离线优先设计
    - FTS5全文搜索
    - 同步支持

---

## 🏗️ 项目结构

```
PaperCrawler/
├── backend/                    # 后端服务
│   ├── src/                   # C++ 源代码
│   │   ├── api_server.cpp     # 主服务器
│   │   ├── auth_handlers.cpp  # 认证处理
│   │   └── database/          # 数据库访问层
│   ├── migrations/            # 数据库迁移脚本
│   └── tests/                 # 后端测试
│
├── frontend/                   # 前端应用
│   ├── src/
│   │   ├── api/              # API模块
│   │   │   └── modules/      # API模块化设计
│   │   ├── assets/           # 静态资源
│   │   ├── components/       # Vue组件
│   │   ├── router/           # 路由配置
│   │   ├── stores/           # Pinia状态管理
│   │   ├── views/            # 页面视图
│   │   └── utils/            # 工具函数
│   └── public/               # 公共资源
│
├── database/                   # 数据库相关
│   ├── complete-schema-mysql.sql       # MySQL架构
│   ├── complete-schema-sqlite.sql      # SQLite架构
│   ├── indexes-optimization-guide.sql  # 索引优化
│   ├── migration-script.sql            # 数据迁移
│   └── benchmark-queries.sql           # 性能测试
│
├── docs/                       # 项目文档
│   ├── ARCHITECTURE-ANALYSIS-REPORT.md
│   ├── FRONTEND_IMPLEMENTATION_PLAN.md
│   ├── AI_PDF_PARSING_MODULE_DESIGN.md
│   └── SYNC_ARCHITECTURE.md
│
├── desktop/                    # 桌面客户端（Qt）
└── scripts/                    # 工具脚本
    ├── start-all-services.bat
    └── deploy.sh
```

---

## 🎓 技术栈

### 前端
| 技术 | 版本 | 用途 |
|------|------|------|
| Vue | 3.4+ | 渐进式框架 |
| TypeScript | 5.0+ | 类型安全 |
| Element Plus | Latest | UI组件库 |
| Pinia | Latest | 状态管理 |
| Vue Router | 4.x | 路由管理 |
| Axios | Latest | HTTP客户端 |
| Vite | 5.x | 构建工具 |

### 后端
| 技术 | 版本 | 用途 |
|------|------|------|
| C++ | 17+ | 高性能API |
| cpp-httplib | Latest | HTTP库 |
| nlohmann/json | Latest | JSON处理 |
| MySQL | 8.0+ | 主数据库 |
| SQLite | 3.x | 本地缓存 |
| Redis | 7.x | 任务队列 |

### AI/爬虫
| 技术 | 版本 | 用途 |
|------|------|------|
| Claude API | Latest | AI分析 |
| PyMuPDF | Latest | PDF解析 |
| Scrapy | Latest | 爬虫框架 |
| BeautifulSoup | Latest | HTML解析 |

---

## 📊 开发进度

### 已完成 ✅
- [x] 系统架构设计
- [x] 数据库完整设计
- [x] 前端架构规划
- [x] 用户认证系统（基础版）
- [x] 管理员后台（基础版）
- [x] Mock API服务器

### 进行中 🚧
- [ ] 论文管理核心功能
- [ ] AI PDF解析模块
- [ ] 爬虫系统
- [ ] 数据同步
- [ ] VIP付费体系

### 计划中 📋
- [ ] 移动端适配
- [ ] 国际化（i18n）
- [ ] 性能优化
- [ ] 安全加固

---

## 🤝 参与贡献

欢迎贡献代码！请查看 [CONTRIBUTING.md](CONTRIBUTING.md)

### 开发流程

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'feat: Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 提交 Pull Request

### 代码规范

- **C++**: Google C++ Style Guide
- **Vue**: Vue官方风格指南
- **TypeScript**: TypeScript官方风格指南
- **数据库**: 数据库命名规范见文档

---

## 📝 开源协议

本项目采用 [MIT](LICENSE) 协议

---

## 👥 团队

- **项目负责人**: Your Name
- **架构师**: Software Architect Agent
- **后端开发**: Backend Developer
- **前端开发**: Frontend Developer
- **AI工程师**: AI Engineer

---

## 📮 联系我们

- **官网**: https://papercrawler.example.com
- **文档**: https://docs.papercrawler.example.com
- **Issue**: https://github.com/your-org/papercrawler/issues
- **邮件**: support@papercrawler.example.com

---

## 🙏 致谢

感谢以下开源项目：
- [Vue.js](https://vuejs.org/)
- [Element Plus](https://element-plus.org/)
- [Claude API](https://www.anthropic.com/)
- [MySQL](https://www.mysql.com/)
- [SQLite](https://www.sqlite.org/)

---

## 📈 路线图

- [x] v1.0.0 - 基础认证系统
- [x] v2.0.0 - 完整设计和规划
- [ ] v2.1.0 - 论文管理核心功能（进行中）
- [ ] v2.2.0 - AI PDF解析
- [ ] v2.3.0 - 爬虫系统
- [ ] v2.4.0 - 数据同步
- [ ] v2.5.0 - VIP付费体系
- [ ] v3.0.0 - 移动端支持

---

**⭐ 如果这个项目对你有帮助，请给个Star支持一下！**

---

<p align="center">
  <i>Made with ❤️ by PaperCrawler Team</i>
</p>
