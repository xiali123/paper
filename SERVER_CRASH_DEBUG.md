# PaperCrawler服务器启动调试指南

## 问题描述
PaperCrawlerServer.exe在启动时立即崩溃（Segmentation Fault），甚至在main函数开始执行前就崩溃。

## 已确认的信息
- ✅ 编译成功（0个错误）
- ✅ 可执行文件生成：3.1MB
- ✅ DLL依赖已复制：libcurl-x64.dll, libmysql.dll
- ✅ spdlog单独测试正常
- ✅ 配置文件正确（MySQL配置）

## 崩溃特征
- 程序在main函数执行前崩溃
- 无任何输出或错误消息
- Segmentation fault (exit code 139)
- 可能是全局对象静态初始化问题

## 调试步骤

### 1. 使用Windows调试器
```bash
# 在Visual Studio或WinDbg中调试
windbg backend/build/PaperCrawlerServer.exe ../config.json
```

### 2. 逐步注释全局对象
编辑 `src/core/main.cpp`，临时注释掉全局对象：

```cpp
// 临时注释以隔离问题
// std::unique_ptr<HttpServerModule> g_httpServer;
// std::shared_ptr<DatabaseModule> g_databaseModule;
// std::unique_ptr<PooledConnection> g_dbConnection;
```

### 3. 检查静态初始化顺序
- 查看是否有循环依赖
- 检查模块的静态全局对象
- 使用`__attribute__((init_priority(1000)))`控制初始化顺序

### 4. 检查特定模块
可能的问题模块：
- DatabaseModule（复杂依赖）
- HttpServerModule（网络初始化）
- PluginManager（动态加载）

### 5. 最小化测试
创建最小main.cpp：
```cpp
#include <iostream>
int main() {
    printf("Minimal test\n");
    return 0;
}
```

## 常见原因
1. **全局对象构造函数异常**
   - 检查DatabaseModule构造函数
   - 检查静态容器的初始化

2. **DLL加载问题**
   - 检查MySQL DLL版本兼容性
   - 检查cURL DLL依赖

3. **静态初始化顺序**
   - 某个静态全局对象在初始化时访问未初始化的对象
   - 检查ModuleRegistry或ServiceContainer的静态初始化

4. **编译器优化问题**
   - 尝试-O0编译（无优化）
   - 检查LTO（链接时优化）设置

## 建议的快速修复尝试

### 方案A：禁用优化编译
```bash
cd backend/build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
mingw32-make PaperCrawlerServer -j4
```

### 方案B：简化main函数
在main.cpp开头添加异常捕获：
```cpp
int main(int argc, char* argv[]) {
    try {
        // 最简单的启动逻辑
        return 0;
    } catch (...) {
        printf("Caught exception during startup\n");
        return 1;
    }
}
```

### 方案C：检查编译时内存布局
```bash
# 检查符号表和内存布局
nm PaperCrawlerServer.exe | grep Global
objdump -x PaperCrawlerServer.exe | head -100
```

## 下一步行动
1. ✅ 继续其他任务（Redis缓存、AI模块等）
2. ⏸️ 调试服务器启动问题（需要调试工具）
3. 🔄 考虑简化服务器初始化流程

## 资源需求
- Windows调试器（WinDbg/Visual Studio）
- 或Linux环境（gdb调试工具）
- 或Docker容器（统一编译环境）

---
**状态**: 等待调试
**优先级**: P1（阻塞测试）
**影响**: 无法进行API端点测试

