# PaperCrawler 数据库架构设计文档集

> **设计日期**: 2026-04-05  
> **架构师**: Software Architect Agent  
> **项目**: PaperCrawler 学术论文管理系统

---

## 📚 文档结构

本目录包含PaperCrawler项目的完整数据库架构设计文档，按以下结构组织：

```
docs/database/
├── README.md (本文件)
├── DATABASE_ARCHITECTURE_DESIGN.md (完整架构设计)
├── DATABASE_MIGRATION_GUIDE.md (实施指南)
├── DATABASE_QUICK_REFERENCE.md (快速参考)
└── ADRs/ (架构决策记录)
    ├── ADR-001-database-architecture.md (数据库架构决策)
    ├── ADR-002-vertical-partitioning.md (垂直分表决策)
    └── ADR-003-read-write-splitting.md (读写分离决策)
```

---

## 🎯 文档导航

### 1. 快速开始

**如果您是新加入项目的开发者**，建议按以下顺序阅读：

1. **[DATABASE_QUICK_REFERENCE.md](DATABASE_QUICK_REFERENCE.md)** - 快速参考
   - 架构概览
   - 核心决策
   - 快速开始指南
   - 常用查询示例

2. **[DATABASE_ARCHITECTURE_DESIGN.md](DATABASE_ARCHITECTURE_DESIGN.md)** - 完整架构设计
   - 架构模式选择
   - 数据模型设计
   - 可扩展性设计
   - 性能和可靠性
   - 安全性设计

### 2. 实施指南

**如果您准备实施数据库架构**，请阅读：

**[DATABASE_MIGRATION_GUIDE.md](DATABASE_MIGRATION_GUIDE.md)** - 实施指南
- 分阶段实施计划（7周）
- 详细任务清单
- 验收标准
- 故障排查手册
- 性能基准测试

### 3. 架构决策

**如果您想了解关键决策的背景**，请阅读：

**[ADRs/](ADRs/)** - 架构决策记录
- [ADR-001: 采用单数据库 + 垂直分表架构](ADRs/ADR-001-database-architecture.md)
- [ADR-002: 采用垂直分表策略](ADRs/ADR-002-vertical-partitioning.md)
- [ADR-003: 采用主从复制 + 读写分离](ADRs/ADR-003-read-write-splitting.md)

---

## 🚀 核心架构决策

| 决策 | 选择 | 文档 | 状态 |
|------|------|------|------|
| **数据库架构** | 单数据库 + 垂直分表 | [ADR-001](ADRs/ADR-001-database-architecture.md) | ✅ 已接受 |
| **表拆分策略** | 垂直分表（核心+扩展+用户） | [ADR-002](ADRs/ADR-002-vertical-partitioning.md) | ✅ 已接受 |
| **读写分离** | 主从复制 + 应用路由 | [ADR-003](ADRs/ADR-003-read-write-splitting.md) | ✅ 已接受 |
| **缓存策略** | 三级缓存（本地+Redis+DB） | [架构设计](DATABASE_ARCHITECTURE_DESIGN.md#缓存层设计) | ✅ 已接受 |
| **全文搜索** | MySQL Fulltext | [架构设计](DATABASE_ARCHITECTURE_DESIGN.md#索引优化策略) | ✅ 已接受 |

---

## 📊 数据库Schema概览

### 域划分

```
PaperCrawler Database
├── User Domain (用户域)
│   ├── users (用户表)
│   ├── user_sessions (会话表)
│   └── vip_subscriptions (订阅表)
│
├── Paper Domain (论文域) ⭐
│   ├── papers_core (核心表)
│   ├── papers_extended (扩展表)
│   ├── papers_user_data (用户数据表)
│   ├── journals (期刊表)
│   ├── authors (作者表)
│   └── paper_authors (关联表)
│
├── Crawler Domain (爬虫域)
│   ├── crawler_templates (模板)
│   ├── distributed_crawl_tasks (任务)
│   ├── crawler_workers (节点)
│   └── scheduled_crawl_tasks (定时任务)
│
├── AI Domain (AI域)
│   ├── ai_conversations (对话)
│   ├── ai_messages (消息)
│   └── ai_parsing_cache (缓存)
│
└── Analytics Domain (分析域)
    ├── search_history (搜索历史)
    ├── admin_audit_logs (审计日志)
    └── system_statistics (统计)
```

### 核心表结构

#### papers_core（核心表）

- **大小**: ~1KB/记录
- **用途**: 高频访问的基础字段
- **索引**: title, year, citation_count, FULLTEXT
- **详情**: [DATABASE_ARCHITECTURE_DESIGN.md#papers_core](DATABASE_ARCHITECTURE_DESIGN.md#2-论文域表paper-domain)

#### papers_extended（扩展表）

- **大小**: ~1.5MB/记录
- **用途**: 低频访问的大文本字段
- **索引**: 无（按主键查询）
- **详情**: [DATABASE_ARCHITECTURE_DESIGN.md#papers_extended](DATABASE_ARCHITECTURE_DESIGN.md#扩展表papers_extended)

#### papers_user_data（用户数据表）

- **大小**: ~500B/记录
- **用途**: 用户个性化数据
- **索引**: (user_id, is_favorite), (user_id, reading_status)
- **详情**: [DATABASE_ARCHITECTURE_DESIGN.md#papers_user_data](DATABASE_ARCHITECTURE_DESIGN.md#用户数据表papers_user_data)

---

## ⚡ 性能目标

| 指标 | 当前 | 目标 | 提升 |
|------|------|------|------|
| **数据规模** | 12篇 | 100万篇 | 83000倍 |
| **查询延迟** | ~2000ms | <100ms | 20倍 |
| **并发用户** | 7个 | 10000+ | 1400倍 |
| **缓存命中率** | 0% | >70% | - |
| **可用性** | 未知 | >99.9% | - |

---

## 🗓️ 实施计划

### 7周实施路线图

| 阶段 | 周期 | 主要任务 | 验收标准 |
|------|------|---------|---------|
| **阶段1** | 第1周 | 数据库初始化 | 所有表创建成功 |
| **阶段2** | 第2-3周 | 应用层适配 | 缓存命中率 > 70% |
| **阶段3** | 第4-5周 | 性能优化 | 查询延迟 < 100ms |
| **阶段4** | 第6周 | 高可用和备份 | 备份成功率 100% |
| **阶段5** | 第7周 | 安全加固 | 安全扫描无高危漏洞 |

**详细实施指南**: [DATABASE_MIGRATION_GUIDE.md](DATABASE_MIGRATION_GUIDE.md)

---

## 🔒 安全特性

- ✅ 密码加密（bcrypt + salt）
- ✅ 敏感字段加密（AES-256）
- ✅ 行级安全（Row-Level Security）
- ✅ 审计日志（完整审计）
- ✅ TLS加密传输
- ✅ 最小权限原则

**详情**: [DATABASE_ARCHITECTURE_DESIGN.md#安全性设计](DATABASE_ARCHITECTURE_DESIGN.md#安全性设计)

---

## 📊 监控指标

### 关键指标

| 指标 | 目标 | 测量方法 | 告警阈值 |
|------|------|---------|---------|
| **查询延迟** | < 100ms | 应用层日志 | > 200ms |
| **缓存命中率** | > 70% | Redis INFO | < 50% |
| **慢查询比例** | < 5% | 慢查询日志 | > 10% |
| **复制延迟** | < 1s | SHOW SLAVE STATUS | > 10s |
| **连接数使用率** | < 80% | SHOW STATUS | > 90% |

**详情**: [DATABASE_ARCHITECTURE_DESIGN.md#监控和维护](DATABASE_ARCHITECTURE_DESIGN.md#监控和维护)

---

## 🚀 快速开始

### 环境准备

```bash
# 1. 安装MySQL 8.0+
sudo apt-get install mysql-server

# 2. 安装Redis 6.0+
sudo apt-get install redis-server

# 3. 创建数据库
mysql -u root -p -e "
CREATE DATABASE PaperCrawler 
CHARACTER SET utf8mb4 
COLLATE utf8mb4_unicode_ci;
"

# 4. 执行迁移脚本
cd backend/migrations
for file in 00*.sql; do
    mysql -u root -p PaperCrawler < $file
done
```

**详细指南**: [DATABASE_MIGRATION_GUIDE.md](DATABASE_MIGRATION_GUIDE.md)

---

## 🤝 贡献指南

### 更新文档

如果您需要更新数据库架构文档，请遵循以下流程：

1. **更新架构决策**
   - 创建新的ADR文档（重大决策）
   - 或更新现有ADR（决策变更）

2. **更新实施指南**
   - 更新DATABASE_MIGRATION_GUIDE.md
   - 添加新的任务清单
   - 更新验收标准

3. **更新快速参考**
   - 更新DATABASE_QUICK_REFERENCE.md
   - 保持与主文档同步

4. **版本控制**
   - 提交Git commit
   - 标注变更原因
   - 通知团队成员

### 文档审查

- [ ] 技术准确性
- [ ] 代码示例可运行
- [ ] 交叉引用正确
- [ ] 语法和拼写检查

---

## 📞 支持和反馈

### 获取帮助

如果您在实施过程中遇到问题：

1. **查看故障排查手册**
   - [DATABASE_MIGRATION_GUIDE.md#故障排查手册](DATABASE_MIGRATION_GUIDE.md#-故障排查手册)

2. **查看架构决策记录**
   - [ADRs/](ADRs/) - 了解决策背景

3. **提交Issue**
   - 在项目仓库提交详细问题

4. **联系架构师**
   - 通过项目Issue追踪

### 反馈渠道

- **架构问题**: 提交ADR讨论
- **实施问题**: 提交Issue
- **文档改进**: 提交Pull Request

---

## 📚 相关文档

### 项目文档

- **[ARCHITECTURE_ANALYSIS.md](../ARCHITECTURE_ANALYSIS.md)** - 项目架构分析
- **[DATA_LAYER_ARCHITECTURE_ANALYSIS.md](../backend/DATA_LAYER_ARCHITECTURE_ANALYSIS.md)** - 数据层架构分析
- **[CLAUDE.md](../backend/CLAUDE.md)** - 开发指南

### 外部资源

- [MySQL官方文档](https://dev.mysql.com/doc/)
- [Redis官方文档](https://redis.io/documentation)
- [Prometheus监控](https://prometheus.io/docs/)
- [Designing Data-Intensive Applications](https://dataintensive.net/)

---

## 📝 版本历史

| 版本 | 日期 | 变更 | 作者 |
|------|------|------|------|
| **1.0** | 2026-04-05 | 初始版本 | Software Architect Agent |

---

## ✅ 验收清单

### 文档完整性

- [ ] DATABASE_ARCHITECTURE_DESIGN.md (完整架构设计)
- [ ] DATABASE_MIGRATION_GUIDE.md (实施指南)
- [ ] DATABASE_QUICK_REFERENCE.md (快速参考)
- [ ] ADR-001 (数据库架构决策)
- [ ] ADR-002 (垂直分表决策)
- [ ] ADR-003 (读写分离决策)

### 文档质量

- [ ] 所有代码示例可运行
- [ ] 所有交叉引用正确
- [ ] 所有图表清晰
- [ ] 无语法和拼写错误

### 实施准备

- [ ] 环境准备完成
- [ ] 团队培训完成
- [ ] 实施计划确认
- [ ] 验收标准明确

---

**文档集版本**: 1.0  
**最后更新**: 2026-04-05  
**下次审查**: 实施完成后

**维护者**: Software Architect Agent  
**项目**: PaperCrawler
