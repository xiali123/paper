# SimpleMath 第三方库示例

**版本**: 1.0
**类型**: 第三方动态DLL示例
**最后更新**: 2026-04-04

---

## 📚 概述

这是一个简单的数学库示例，演示如何将第三方C库编译为动态DLL并集成到项目中。

**功能**:
- 基本数学运算（加、减、乘、除）
- 数组操作（求和、平均值）
- 字符串工具（大小写转换）

---

## 📂 目录结构

```
core/external/simplemath/
├── README.md                 # 本文件
├── build_info.txt           # 库信息
├── include/                 # 公共头文件
│   └── simplemath.h
├── src/                     # 源代码
│   ├── basic_ops.c
│   ├── array_ops.c
│   └── string_ops.c
├── api/                     # DLL导出头文件
│   └── simplemath_api.hpp
└── win32/                   # Windows兼容性（可选）
    └── strings.h
```

---

## 🔧 文件内容

### 1. build_info.txt

```txt
Name: SimpleMath
Version: 1.0.0
Type: Dynamic DLL
Language: C
Author: PaperCrawler Team
License: MIT
Description: Simple mathematics library demonstrating third-party DLL compilation
```

### 2. include/simplemath.h

```c
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// 基本运算
int add(int a, int b);
int subtract(int a, int b);
int multiply(int a, int b);
double divide(double a, double b);

// 数组操作
int sum_array(const int* array, int length);
double average_array(const int* array, int length);

// 字符串工具
void to_upper_case(char* str);
void to_lower_case(char* str);

#ifdef __cplusplus
}
#endif
```

### 3. src/basic_ops.c

```c
#include "simplemath.h"

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int multiply(int a, int b) {
    return a * b;
}

double divide(double a, double b) {
    if (b == 0.0) return 0.0;
    return a / b;
}
```

### 4. src/array_ops.c

```c
#include "simplemath.h"

int sum_array(const int* array, int length) {
    int sum = 0;
    for (int i = 0; i < length; i++) {
        sum += array[i];
    }
    return sum;
}

double average_array(const int* array, int length) {
    if (length == 0) return 0.0;
    int sum = sum_array(array, length);
    return (double)sum / length;
}
```

### 5. src/string_ops.c

```c
#include "simplemath.h"
#include <ctype.h>
#include <string.h>

void to_upper_case(char* str) {
    if (!str) return;
    for (size_t i = 0; i < strlen(str); i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

void to_lower_case(char* str) {
    if (!str) return;
    for (size_t i = 0; i < strlen(str); i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}
```

### 6. api/simplemath_api.hpp

```cpp
#pragma once

// DLL导出/导入宏
#ifdef SIMPLEMATH_EXPORTS
#define SIMPLEMATH_API __declspec(dllexport)
#else
#define SIMPLEMATH_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 导出所有公共函数
SIMPLEMATH_API int add(int a, int b);
SIMPLEMATH_API int subtract(int a, int b);
SIMPLEMATH_API int multiply(int a, int b);
SIMPLEMATH_API double divide(double a, double b);

SIMPLEMATH_API int sum_array(const int* array, int length);
SIMPLEMATH_API double average_array(const int* array, int length);

SIMPLEMATH_API void to_upper_case(char* str);
SIMPLEMATH_API void to_lower_case(char* str);

#ifdef __cplusplus
}
#endif
```

### 7. win32/strings.h（可选，仅在需要Unix兼容性时）

```c
#pragma once
#ifndef _MSC_VER
#error This strings.h replacement is for Windows (MSVC) only
#endif

#include <string.h>

// strncasecmp的Windows替代
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
```

---

## 🚀 编译步骤

### 1. 验证库结构

```bash
cd backend
bash scripts/verify_third_party_library.sh simplemath
```

### 2. 在CMakeLists.txt中配置

```cmake
# backend/CMakeLists.txt

# 自动编译为动态DLL
add_third_party_library(simplemath)
```

### 3. 编译

```bash
cd backend/build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### 4. 验证输出

```bash
ls -lh modules/third_party/libsimplemath.dll
# 应该看到类似：-rwxr-xr-x 1 user group 12K Apr  4 06:30 libsimplemath.dll
```

---

## 💡 在业务模块中使用

```cpp
// src/business/MyModule.cpp
#include "simplemath.h"  // 直接包含头文件

class MyModule : public BusinessModuleBase {
private:
    void process() {
        // 使用第三方库函数
        int result = add(10, 20);  // 调用simplemath库
        printf("Result: %d\n", result);
    }
};
```

---

## ✅ 验证清单

- [ ] 目录结构正确（include/, src/, api/, win32/）
- [ ] build_info.txt存在
- [ ] DLL导出头文件已创建（api/simplemath_api.hpp）
- [ ] 所有源文件正确实现
- [ ] 通过verify_third_party_library.sh验证
- [ ] 编译生成libsimplemath.dll
- [ ] 文件大小合理（约10-20KB）

---

## 📚 扩展建议

当创建实际的第三方库时，参考此示例：

1. **保持简单** - 从小功能开始
2. **完整文档** - 每个函数都有注释
3. **错误处理** - 添加参数验证
4. **单元测试** - 在tests/目录添加测试
5. **示例代码** - 在examples/目录添加使用示例

---

**相关文档**:
- [THIRD_PARTY_LIBRARY_BUILD_GUIDE.md](../THIRD_PARTY_LIBRARY_BUILD_GUIDE.md)
- [backend/CLAUDE.md](../CLAUDE.md)
