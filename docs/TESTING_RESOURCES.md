# PaperCrawler 测试资源清单

本文档列出了所有为测试 PaperCrawler 认证系统而创建的资源和工具。

---

## 🚀 启动脚本（位于项目根目录）

### start-test-env.bat
**用途**: 一键启动完整的测试环境（后端 + 前端）

**使用方法**:
```bash
# Windows: 双击运行
start-test-env.bat

# Git Bash:
cmd //c start-test-env.bat
```

**功能**:
- 自动检查数据库是否存在
- 在新窗口启动后端服务器
- 在新窗口启动前端开发服务器
- 显示访问地址和测试账号

### start-backend.bat
**用途**: 单独启动后端服务器

**使用方法**:
```bash
start-backend.bat
```

**功能**:
- 检查并创建测试数据库
- 验证数据库 schema
- 启动后端服务器 (http://127.0.0.1:8080)

### start-frontend.bat
**用途**: 单独启动前端开发服务器

**使用方法**:
```bash
start-frontend.bat
```

**功能**:
- 检查并安装依赖
- 启动前端开发服务器 (http://localhost:5173)

---

## 🛠️ 测试工具（位于 backend 目录）

### quick_health_check.py
**用途**: 系统健康检查

**使用方法**:
```bash
cd backend
python quick_health_check.py
```

**检查项**:
- 数据库存在性和完整性
- 后端可执行文件
- 配置文件正确性

**输出示例**:
```
[PASS] Database
      Database OK, 1 test user(s) found
[PASS] Backend Binary
      Backend compiled
[PASS] Configuration
      Config OK
[SUCCESS] All checks passed!
```

### verify_db_schema.py
**用途**: 详细验证数据库 schema

**使用方法**:
```bash
cd backend
python verify_db_schema.py
```

**验证内容**:
- 所有表是否创建
- 索引是否完整
- 触发器是否存在
- 外键约束是否正确
- 列定义是否符合要求

### create_test_db.py
**用途**: 创建测试数据库

**使用方法**:
```bash
cd backend
python create_test_db.py
```

**功能**:
- 从 migration SQL 创建数据库
- 创建所有表、索引、触发器
- 记录迁移版本
- 显示创建结果

**输出示例**:
```
Database Created Successfully!
Tables created:
  [OK] users
  [OK] user_sessions
  [OK] login_attempts
  ...
```

### insert_test_user.py
**用途**: 插入测试用户

**使用方法**:
```bash
cd backend
python insert_test_user.py
```

**创建的用户**:
- 邮箱: test@example.com
- 密码: TestPass123!
- 用户名: testuser
- 角色: user

**输出示例**:
```
Test User Created Successfully!
You can now login with:
  Email:    test@example.com
  Password: TestPass123!
```

---

## 📚 文档（位于项目根目录）

### READY_TO_TEST.md
**用途**: 系统就绪状态报告

**内容**:
- 系统状态总览
- 已完成工作清单
- 立即开始测试指南
- 快速验证清单
- 故障排查指南
- 文档导航

**适合**: 第一次启动系统前查看

### QUICKSTART.md
**用途**: 快速启动指南

**内容**:
- 一键启动说明
- 测试账号信息
- 快速测试流程
- 常见问题解决
- 检查清单

**适合**: 快速上手，5分钟内开始测试

### MANUAL_TESTING_GUIDE.md
**用途**: 完整的手动测试指南

**内容**:
- 环境准备
- 分阶段测试流程（8个阶段）
- API 端点测试
- 安全功能测试
- 故障排查
- 测试检查清单

**适合**: 完整测试认证系统所有功能

### TESTING_SUMMARY.md
**用途**: 测试总结和快速参考

**内容**:
- 测试完成状态
- 已创建的测试工具
- 关键测试点
- 测试命令速查
- 下一步行动

**适合**: 了解测试进度和快速查找命令

### TEST_REPORT.md
**用途**: 详细的测试执行报告

**内容**:
- 测试执行时间
- 测试结果统计
- 详细测试结果
- 功能验证清单
- 手动测试清单
- 已知问题和注意事项

**适合**: 查看完整的测试结果和质量评估

---

## 📊 其他验证脚本

### verify-auth.sh（项目根目录）
**用途**: 快速验证所有认证组件

**使用方法**:
```bash
bash verify-auth.sh
```

**验证项**: 26 个核心组件

### test-authentication.sh（项目根目录）
**用途**: 综合测试套件

**使用方法**:
```bash
bash test-authentication.sh
```

**测试项**: 35+ 验证检查

### validate_auth.py（项目根目录）
**用途**: Python 跨平台验证脚本

**使用方法**:
```bash
python validate_auth.py
```

**优点**: 编码安全，Windows 友好

---

## 📁 文件位置索引

### 启动相关
```
e:/PaperCrawler/
├── start-test-env.bat          # 一键启动
├── start-backend.bat           # 启动后端
├── start-frontend.bat          # 启动前端
└── README_TESTING.md           # 本文档
```

### 测试工具
```
e:/PaperCrawler/backend/
├── papercrawler_test.db        # 测试数据库
├── quick_health_check.py       # 健康检查
├── verify_db_schema.py         # Schema 验证
├── create_test_db.py           # 创建数据库
└── insert_test_user.py         # 插入测试用户
```

### 文档
```
e:/PaperCrawler/
├── READY_TO_TEST.md            # 就绪报告
├── QUICKSTART.md               # 快速指南
├── MANUAL_TESTING_GUIDE.md     # 测试指南
├── TESTING_SUMMARY.md          # 测试总结
├── TEST_REPORT.md              # 测试报告
├── TESTING_RESOURCES.md        # 本文档
├── verify-auth.sh              # 验证脚本
├── test-authentication.sh      # 测试脚本
└── validate_auth.py            # Python 验证
```

---

## 🎯 使用场景导航

### 场景 1: 第一次使用
**推荐文档**: [QUICKSTART.md](QUICKSTART.md)

**步骤**:
1. 阅读 QUICKSTART.md
2. 运行 `start-test-env.bat`
3. 打开浏览器访问 http://localhost:5173
4. 使用测试账号登录

### 场景 2: 完整功能测试
**推荐文档**: [MANUAL_TESTING_GUIDE.md](MANUAL_TESTING_GUIDE.md)

**步骤**:
1. 阅读 MANUAL_TESTING_GUIDE.md
2. 按照文档逐阶段测试
3. 记录测试结果
4. 报告发现的问题

### 场景 3: 系统检查
**推荐工具**: `quick_health_check.py`

**步骤**:
1. 运行 `cd backend && python quick_health_check.py`
2. 检查所有项是否通过
3. 如有失败，查看具体错误信息
4. 根据提示修复问题

### 场景 4: 数据库问题
**推荐工具**: `verify_db_schema.py` + `create_test_db.py`

**步骤**:
1. 运行 `cd backend && python verify_db_schema.py`
2. 如果有问题，删除旧数据库：`rm papercrawler_test.db`
3. 重新创建：`python create_test_db.py`
4. 插入测试用户：`python insert_test_user.py`

### 场景 5: 查看测试结果
**推荐文档**: [TEST_REPORT.md](TEST_REPORT.md)

**内容**:
- 测试覆盖率统计
- 功能验证结果
- 已知问题列表
- 下一步建议

---

## 🔗 快速链接

### 立即开始
- [快速启动指南](QUICKSTART.md)
- [就绪状态报告](READY_TO_TEST.md)

### 测试相关
- [完整测试指南](MANUAL_TESTING_GUIDE.md)
- [测试总结](TESTING_SUMMARY.md)
- [详细测试报告](TEST_REPORT.md)

### 系统文档
- [认证系统指南](AUTHENTICATION_GUIDE.md)
- [实现总结](AUTHENTICATION_IMPLEMENTATION_SUMMARY.md)

---

## 📞 获取帮助

如果遇到问题：

1. **首先运行健康检查**:
   ```bash
   cd backend
   python quick_health_check.py
   ```

2. **查看故障排查章节**:
   - [QUICKSTART.md - 常见问题](QUICKSTART.md)
   - [MANUAL_TESTING_GUIDE.md - 故障排查](MANUAL_TESTING_GUIDE.md)

3. **查看相关文档**:
   - [AUTHENTICATION_GUIDE.md - 认证系统说明](AUTHENTICATION_GUIDE.md)

---

## ✅ 检查清单

使用前检查：
- [ ] 已阅读 QUICKSTART.md
- [ ] 已运行健康检查
- [ ] 后端已编译（PaperCrawlerServer.exe 存在）
- [ ] 前端依赖已安装（node_modules 存在）

测试前检查：
- [ ] 测试数据库已创建
- [ ] 测试用户已插入
- [ ] 启动脚本可执行

测试后检查：
- [ ] 所有功能正常工作
- [ ] 令牌管理正确
- [ ] 安全功能有效
- [ ] 无控制台错误

---

**祝测试顺利！** 🎉

如有任何问题，请参考相应文档或运行健康检查脚本。
