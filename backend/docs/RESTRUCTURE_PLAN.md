# 目录重组执行计划

## 目标结构

```
backend/
├── include/
│   ├── core/              # 核心框架（原framework + server）
│   ├── business/         # 业务API（保持）
│   ├── data/              # 数据层（保持）
│   ├── network/           # 网络层（保持）
│   └── features/          # 功能模块（合并所有小模块）
│       ├── performance/   # 原 performance/
│       ├── security/       # 原 security/
│       ├── infrastructure/# 原 system/ + monitoring/ + config/
│       ├── resilience/     # 原 resilience/ + scheduler/
│       └── operations/     # 原 validation/ + notification/ + docs/ + backup/ + proxy/
│
├── src/                   # 源文件（结构同include）
├── examples/              # 示例代码（从src/移动）
├── docs/                  # 文档（从根目录移动）
├── lib/                   # 第三方库
├── tests/                 # 测试文件
├── config/                # 配置文件（保持）
└── build/                 # 编译输出（保持）
```

## 目录映射

### include/ 目录
```
framework/    → core/
server/        → core/
models/        → core/models/

business/      → business/ (保持)
communication/ → core/communication/
pool/          → core/pool/

data/          → data/ (保持)
database/      → 删除（重复，并入data/）

network/       → network/ (保持)

performance/   → features/performance/
security/       → features/security/
system/        → features/infrastructure/
monitoring/    → features/infrastructure/
config/        → features/infrastructure/

resilience/    → features/resilience/
scheduler/     → features/resilience/

filter/        → features/operations/
queue/         → features/operations/
handler/       → features/operations/

validation/    → features/operations/
notification/  → features/operations/
documentation/ → features/operations/
backup/        → features/operations/
proxy/         → features/operations/

modules/       → 删除（空目录或重复）
```

### src/ 目录
```
api_server.cpp           → examples/
api_server_*.cpp         → examples/
auth_middleware.cpp      → examples/
auth_handlers.cpp         → examples/
paper_handlers.cpp        → examples/
websocket_server.*       → examples/
*_test.cpp               → examples/
standalone_server.cpp     → examples/

其他目录与include/对应
```

## 执行步骤

1. 创建新目录结构
2. 移动include/目录文件
3. 移动src/目录文件
4. 清理旧文件和空目录
5. 更新CMakeLists.txt
6. 测试编译
7. 提交Git
