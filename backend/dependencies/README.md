# 第三方依赖库目录

此目录用于存放PaperCrawler后端项目所需的第三方依赖库。

## 目录结构

```
dependencies/
├── runtime/        # 运行时依赖 (DLL, SO, DYLIB)
├── dlls/          # Windows DLL文件
├── libs/          # 静态库文件 (.lib, .a)
└── README.md      # 本文件
```

## 运行时依赖说明

### Windows平台

#### libcurl-x64.dll
- **用途**: HTTP客户端库
- **依赖模块**: SearchApiModule, HttpClient
- **来源**: cURL官方发布包或vcpkg
- **版本**: 8.x
- **架构**: x64

#### libmysql.dll
- **用途**: MySQL客户端库
- **依赖模块**: 数据库模块
- **来源**: MySQL官方安装包
- **版本**: 8.0
- **架构**: x64

#### OpenSSL DLLs (如果需要)
- **libssl-1_1-x64.dll**
- **libcrypto-1_1-x64.dll**
- **用途**: SSL/TLS支持
- **依赖模块**: HttpClient (HTTPS)
- **来源**: OpenSSL官方或vcpkg

## 自动部署

CMake构建系统会自动将这些依赖复制到构建输出目录：

```
backend/build/Release/
├── libcurl-x64.dll      # 自动从 dependencies/runtime/ 复制
├── libmysql.dll          # 自动从 dependencies/runtime/ 复制
└── modules/dynamic/      # 业务模块DLL
```

## 更新依赖

### 添加新依赖

1. 将DLL文件复制到 `dependencies/runtime/`
2. 在 `CMakeLists.txt` 中添加复制命令
3. 重新构建项目

### 推荐获取方式

#### vcpkg (推荐)
```bash
vcpkg install curl:x64-windows
vcpkg install mysql:x64-windows
vcpkg install openssl:x64-windows
```

#### 官方下载
- libcurl: https://curl.se/download.html
- MySQL: https://dev.mysql.com/downloads/mysql/
- OpenSSL: https://www.openssl.org/source/

## 版本兼容性

确保第三方依赖库的架构与项目编译配置一致：
- 架构: x64 (64位)
- 编译器: MSVC 2019/2022 (Visual Studio)
- 运行时: MSVC RT (v143/v144)

## 故障排除

### DLL加载失败

如果看到"Failed to load library"错误：

1. 检查DLL是否在 `dependencies/runtime/` 目录
2. 确认DLL架构是x64
3. 检查DLL版本兼容性
4. 使用Dependency Walker查看依赖链

### 清理和重建

```bash
# 清理构建目录
cd backend/build
rm -rf Release/

# 重新构建
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```
