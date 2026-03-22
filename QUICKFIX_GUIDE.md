# 后端分页修复 - 快速指南

## 当前状态

✅ **代码已修复**: [PaperCrawlerAPI.cpp](e:\PaperCrawler\core\src\core\PaperCrawlerAPI.cpp:172)
⏳ **等待重建**: 网络问题阻止依赖下载

## 临时解决方案

### 方案 A: 手动下载依赖（推荐）

1. **下载 nlohmann/json**:
   ```bash
   # 使用浏览器或下载工具
   https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
   # 放到: e:/PaperCrawler/core/external/nlohmann/json.hpp
   ```

2. **下载 spdlog**:
   ```bash
   git clone https://github.com/gabime/spdlog.git e:/PaperCrawler/core/external/spdlog
   ```

3. **下载 gumbo-parser**:
   ```bash
   git clone https://github.com/google/gumbo-parser.git e:/PaperCrawler/core/external/gumbo
   ```

4. **修改 CMakeLists.txt** 使用本地依赖：
   ```cmake
   # 替换 FetchContent 为本地路径
   add_subdirectory(external/nlohmann)
   add_subdirectory(external/spdlog)
   add_subdirectory(external/gumbo)
   ```

### 方案 B: 使用预编译库（最快）

如果您有其他可用的构建环境：

1. 在另一台机器上构建
2. 复制以下文件：
   - `PaperCrawlerCore.dll`
   - `PaperCrawlerServer.exe`
3. 替换现有文件

### 方案 C: 等待网络恢复

CMake会自动重试下载，只需：
```bash
cd e:/PaperCrawler/core/build2
cmake --build . --config Release
```

## 验证修复（无需重建）

在重建之前，您可以通过检查源代码验证修复：

```bash
# 查看修复的代码
cat e:/PaperCrawler/core/src/core/PaperCrawlerAPI.cpp | sed -n '172,188p'
```

应该看到：
```cpp
std::vector<Paper> PaperCrawlerAPI::getPapers(const std::string& keyword, int offset, int limit) {
    std::string sql;

    if (keyword.empty()) {
        // If no keyword, return all papers (sorted by id)
        sql = "SELECT * FROM cspaper ORDER BY id";
    } else {
        // Search in title field (using LIKE for partial matching)
        std::string escapedKeyword = DatabaseManager::getInstance().escape(keyword);
        sql = "SELECT * FROM cspaper WHERE title LIKE '%" + escapedKeyword + "%' ORDER BY id";
    }

    if (limit > 0) {
        sql += " LIMIT " + std::to_string(limit);
        if (offset > 0) {
            sql += " OFFSET " + std::to_string(offset);
        }
    }
```

## 测试桌面客户端（当前状态）

虽然后端使用旧代码，但您可以测试桌面客户端的UI：

```bash
# 启动桌面客户端
e:/PaperCrawler/desktop/build/PaperCrawlerDesktop.exe

# 测试搜索功能（分页会有bug，但UI可以测试）
```

## 自动构建脚本（网络恢复后使用）

```bash
# Windows
cd e:/PaperCrawler
backend\apply_pagination_fix.bat

# 或手动执行
cd e:/PaperCrawler/core/build2
cmake --build . --config Release

cd ../../backend/build
cmake --build . --config Release
```

## 预期结果对比

### 修复前（当前）
```json
{
  "papers": [
    {
      "id": 1,
      "title": "Paper 1: Deep Learning for learning&offset=3&limit=2"
    }
  ]
}
```

### 修复后
```json
{
  "papers": [
    {
      "id": 4,
      "title": "Paper 4: Deep Learning for learning"
    },
    {
      "id": 5,
      "title": "Paper 5: Machine Learning Applications"
    }
  ]
}
```

## 进度跟踪

- [x] 桌面客户端编译成功
- [x] 后端Bug修复完成
- [x] 源代码验证正确
- [ ] 等待网络恢复以下载依赖
- [ ] 重建后端服务
- [ ] 端到端测试

## 联系方式

如有问题，请参考：
- [STATUS.md](e:\PaperCrawler\STATUS.md) - 项目状态
- [BACKEND_PAGINATION_FIX.md](e:\PaperCrawler\BACKEND_PAGINATION_FIX.md) - 详细修复文档
