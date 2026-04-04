# PaperCrawler 编译指南（包含Redis缓存层）

## 编译日期
2026-04-01

## 新增代码
本次编译包含以下**新实现的Redis缓存层代码**：

### 新增源文件
1. `backend/src/data/RedisConnection.cpp` - Redis连接实现
2. `backend/src/data/RedisConnectionPool.cpp` - Redis连接池实现
3. `backend/include/data/RedisConnection.hpp` - Redis连接头文件
4. `backend/include/data/RedisConnectionPool.hpp` - Redis连接池头文件

### 修改文件
1. `backend/src/data/CacheModule.cpp` - 增强为Redis+Memory混合缓存
2. `backend/include/data/CacheModule.hpp` - 添加Redis相关方法
3. `backend/CMakeLists.txt` - 添加hiredis支持
4. `backend/config.json` - 添加cache配置节

## 编译步骤

### 前提条件

#### 1. 安装MinGW-w64（如果尚未安装）

**选项A：使用MSYS2**
```bash
# 下载MSYS2: https://www.msys2.org/

# 在MSYS2终端中执行
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make
```

**选项B：手动安装MinGW-w64**
```bash
# 下载: https://www.mingw-w64.org/downloads/
# 或使用: https://github.com/niXman/mingw-builds-binaries/releases
```

**选项C：使用vcpkg**
```bash
# 安装vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat

# 安装MinGW
./vcpkg install mingw-w64
```

#### 2. 安装hiredis库（可选，用于Redis缓存）

**如果未安装hiredis，项目会自动降级到内存缓存模式**

**选项A：使用vcpkg（推荐）**
```bash
vcpkg install hiredis:x64-mingw-static
```

**选项B：从源码编译**
```bash
git clone https://github.com/redis/hiredis.git
cd hiredis
make
make install PREFIX=/c/hiredis
```

**选项C：使用预编译库**
```bash
# 下载hiredis DLL和库文件
# 放置到: e:/PaperCrawler/core/external/hiredis/
```

### 编译流程

#### 步骤1：设置环境变量

**在Git Bash或MSYS2中执行**：
```bash
# 添加MinGW到PATH（根据实际安装路径调整）
export PATH="/c/Program Files/msys64/mingw64/bin:$PATH"

# 或添加到 ~/.bashrc 永久生效
echo 'export PATH="/c/Program Files/msys64/mingw64/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

**在Windows CMD中执行**：
```cmd
set PATH=C:\Program Files\msys64\mingw64\bin;%PATH%
```

#### 步骤2：验证编译器

```bash
gcc --version
g++ --version
mingw32-make --version
cmake --version
```

**预期输出**：
```
gcc (x86_64-posix-seh-rev0, Built by MinGW-W64 project) 13.1.0
g++ (x86_64-posix-seh-rev0, Built by MinGW-W64 project) 13.1.0
mingw32-make: GNU Make 4.4.1
cmake version 3.28.0
```

#### 步骤3：清理旧的构建文件

```bash
cd e:/PaperCrawler/backend/build
rm -rf *
```

#### 步骤4：运行CMake配置

```bash
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
```

**预期输出（包含Redis检测）**：
```
-- Backend sources:
--   Redis cache detection:
--     hiredis: [FOUND/NOT FOUND]
--     Cache mode: [Redis/Memory fallback]
--   ...
-- PaperCrawlerServer configured successfully
```

**如果hiredis未找到，会显示**：
```
-- WARNING: hiredis not found, using in-memory cache only
-- Cache mode: Memory-only (USE_MEMORY_CACHE defined)
```

#### 步骤5：编译项目

```bash
mingw32-make -j4
```

**预期编译时间**: 2-5分钟（取决于硬件）

**预期输出**：
```
[ 10%] Building CXX object CMakeFiles/PaperCrawlerServer.dir/src/data/RedisConnection.cpp.obj
[ 15%] Building CXX object CMakeFiles/PaperCrawlerServer.dir/src/data/RedisConnectionPool.cpp.obj
[ 20%] Building CXX object CMakeFilesServer.dir/src/data/CacheModule.cpp.obj
...
[100%] Built target PaperCrawlerServer
```

**常见编译错误及解决**：

##### 错误1：找不到hiredis头文件
```
fatal error: hiredis/hiredis.h: No such file or directory
```

**解决方案A：禁用Redis**
```bash
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
# 然后编辑 CMakeCache.txt，添加: USE_MEMORY_CACHE:BOOL=ON
```

**解决方案B：安装hiredis**
```bash
# 使用vcpkg安装
vcpkg install hiredis:x64-mingw-static
```

##### 错误2：未定义的引用
```
undefined reference to `redisConnect'
```

**解决方案**：确保hiredis库已正确链接
```bash
# 检查CMakeLists.txt中的链接配置
target_link_libraries(PaperCrawlerServer PRIVATE hiredis::hiredis)
```

##### 错误3：MySQL库找不到
```
cannot find -lmysql
```

**解决方案**：
```bash
# 复制MySQL库到项目目录
cp "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib" \
   e:/PaperCrawler/backend/build/
```

#### 步骤6：复制依赖DLL

```bash
cd e:/PaperCrawler/backend/build

# 复制cURL DLL
cp ../core/external/curl-8.19.0_4-win64-mingw/bin/libcurl-x64.dll .

# 复制MySQL DLL
cp "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.dll" .

# 如果使用hiredis DLL，也复制过来
cp ../core/external/hiredis/dll/libhiredis.dll .  # 如果存在
```

**验证DLL**：
```bash
ls -lh *.dll
# 应该看到：
# - libcurl-x64.dll
# - libmysql.dll
# - libhiredis.dll (可选)
```

#### 步骤7：验证可执行文件

```bash
ls -lh PaperCrawlerServer.exe
```

**预期输出**：
```
-rwxr-xr-x 1 Administ 197121 3.2M Apr 1 09:00 PaperCrawlerServer.exe
```

**检查依赖**：
```bash
ldd PaperCrawlerServer.exe | grep -E "(mysql|curl|hiredis)"
```

**预期输出**：
```
libcurl-x64.dll => /usr/local/lib/libcurl-x64.dll
libmysql.dll => /usr/local/lib/libmysql.dll
libhiredis.dll => /usr/local/lib/libhiredis.dll (如果启用Redis)
```

#### 步骤8：部署到Release目录

```bash
cd e:/PaperCrawler/backend

# 创建Release目录（如果不存在）
mkdir -p Release/modules/dynamic

# 复制可执行文件
cp build/PaperCrawlerServer.exe Release/

# 复制DLL
cp build/*.dll Release/

# 复制配置文件
cp config.json Release/

# 复制动态模块（如果有）
cp -r build/Release/modules/dynamic/*.dll Release/modules/dynamic/ 2>/dev/null || true
```

## 运行和测试

### 启动服务器

```bash
cd e:/PaperCrawler/backend/Release
./PaperCrawlerServer.exe config.json
```

**预期启动日志（成功）**：
```
[PaperCrawler] Starting server...
[Module] Loading modules...
[Database] Connecting to MySQL: localhost:3306/papercrawler
[Database] Connection pool created: 10 connections
[Cache] Initializing cache module...
[Cache]   Host: localhost:6379
[Cache]   Pool size: 10
[Cache]   Redis enabled: true
[Cache] Redis connection pool warmed up: 10 connections
[Cache] Redis connection successful!
[Cache] Initialization complete
[Cache] Cleanup thread started (interval: 5 min)
[HTTP] Server listening on 0.0.0.0:8080
[Router] Registered 40+ routes
[PaperCrawler] Server started successfully!
```

**如果Redis未启用（降级到内存缓存）**：
```
[Cache] Redis connection failed, falling back to memory cache
[Cache] Using memory cache only (Redis disabled)
```

### 测试缓存功能

#### 1. 健康检查

```bash
curl http://localhost:8080/api/health
```

**预期响应**：
```json
{
  "status": "ok",
  "cache": {
    "type": "redis",  // 或 "memory"
    "available": true,
    "connections": 10
  }
}
```

#### 2. 测试缓存操作

```bash
# SET操作
curl -X POST http://localhost:8080/api/cache/set \
  -H "Content-Type: application/json" \
  -d '{"key": "test_key", "value": "test_value"}'

# GET操作
curl http://localhost:8080/api/cache/get?key=test_key

# DELETE操作
curl -X DELETE http://localhost:8080/api/cache/delete?key=test_key
```

#### 3. 查看缓存统计

```bash
curl http://localhost:8080/api/cache/stats
```

**预期响应**：
```json
{
  "total_keys": "5",
  "hit_count": "120",
  "miss_count": "30",
  "hit_rate": "80.0%",
  "redis_enabled": "true",
  "total_connections": "10",
  "active_connections": "2",
  "available_connections": "8",
  "memory_used_bytes": "20480"
}
```

### 测试业务模块缓存集成

#### 论文缓存测试

```bash
# 第一次请求（缓存未命中，从数据库读取）
curl http://localhost:8080/api/papers/1 \
  -H "Authorization: Bearer $TOKEN"

# 第二次请求（缓存命中，从Redis读取）
curl http://localhost:8080/api/papers/1 \
  -H "Authorization: Bearer $TOKEN"

# 查看统计（应该看到hit_count增加）
curl http://localhost:8080/api/cache/stats
```

#### 用户会话缓存测试

```bash
# 登录
TOKEN=$(curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "password"}' \
  | jq -r '.access_token')

# 验证会话（第一次）
curl http://localhost:8080/api/auth/current \
  -H "Authorization: Bearer $TOKEN"

# 验证会话（第二次，从缓存读取）
curl http://localhost:8080/api/auth/current \
  -H "Authorization: Bearer $TOKEN"
```

## 性能测试

### 使用Apache Bench

```bash
# 安装ab工具（如果未安装）
# Windows: 下载Apache httpd

# 测试论文详情API
ab -n 1000 -c 10 \
   -H "Authorization: Bearer $TOKEN" \
   http://localhost:8080/api/papers/1
```

**预期性能提升**：
- 无缓存: ~50ms/请求
- 有缓存: ~2ms/请求
- **提升25倍**

### 使用wrk（推荐）

```bash
# 安装wrk
# git clone https://github.com/wg/wrk.git && cd wrk && make

# 测试
wrk -t4 -c100 -d30s \
    -H "Authorization: Bearer $TOKEN" \
    http://localhost:8080/api/papers/1
```

**预期结果**：
```
Running 30s test @ http://localhost:8080/api/papers/1
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     +/-   Stdev
    Latency     2.05ms    1.23ms   87.50%
    Req/Sec     12.34k     1.23k    75.00%
  1476k requests in 30.00s, 1.23GB read
Requests/sec:  49200.00
Transfer/sec:     42.12MB
```

## 故障排查

### 问题1：服务器启动崩溃（Segmentation Fault）

**症状**：
```
Segmentation fault (core dumped)
```

**可能原因**：
1. DLL加载问题
2. 静态初始化顺序问题
3. MySQL连接失败

**解决方案**：
```bash
# 检查DLL依赖
ldd PaperCrawlerServer.exe

# 使用GDB调试
gdb PaperCrawlerServer.exe
(gdb) run
(gdb) backtrace  # 查看调用栈

# 或使用WinDbg（Windows）
windbg PaperCrawlerServer.exe
```

### 问题2：Redis连接失败

**症状**：
```
[Cache] Redis connection failed, falling back to memory cache
```

**解决方案**：
```bash
# 检查Redis是否运行
redis-cli ping

# 启动Redis
redis-server

# 或使用Docker
docker run -p 6379:6379 -d redis
```

### 问题3：MySQL连接失败

**症状**：
```
[Database] Failed to connect to MySQL
```

**解决方案**：
```bash
# 检查MySQL是否运行
mysql -uroot -p123456 -e "SELECT 1"

# 启动MySQL
sudo systemctl start mysql  # Linux
# 或在Windows服务管理器中启动
```

## 编译选项说明

### CMake构建类型

```bash
# Release模式（优化，生产环境）
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Debug模式（调试符号，开发环境）
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
```

### 启用/禁用Redis

```bash
# 强制使用内存缓存（不依赖hiredis）
cmake .. -G "MinGW Makefiles" -DUSE_MEMORY_CACHE=ON

# 强制启用Redis（需要hiredis）
cmake .. -G "MinGW Makefiles" -DUSE_REDIS_CACHE=ON
```

## 下一步

1. ✅ **Redis缓存实现** - 已完成
2. ⏳ **编译项目** - 执行本指南
3. ⏳ **测试Redis功能** - 验证缓存工作正常
4. ⏳ **业务模块集成** - 在PaperApiModule等模块中使用缓存
5. ⏳ **性能测试** - 验证20-30倍性能提升

## 相关文档

- [Redis缓存实施计划](./REDIS_CACHE_IMPLEMENTATION.md)
- [Redis缓存完成报告](./REDIS_CACHE_COMPLETED.md)
- [服务器崩溃调试指南](./SERVER_CRASH_DEBUG.md)
- [项目全面梳理计划](./C:\Users\Administrator\.claude\plans\zazzy-mapping-journal.md)

---

**状态**: ✅ 代码实现完成，待编译测试
**优先级**: P1（性能优化）
**预期收益**: API响应时间提升 20-30倍
