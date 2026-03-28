# Backend Pagination Bug Fix

## Issue Identified

The backend API has a critical SQL bug in the `getPapers()` method that prevents pagination from working correctly.

## Root Cause

**File**: `e:\PaperCrawler\core\src\core\PaperCrawlerAPI.cpp`
**Line**: 172-174

**Buggy Code**:
```cpp
std::vector<Paper> PaperCrawlerAPI::getPapers(const std::string& keyword, int offset, int limit) {
    std::string sql = "SELECT * FROM cspaper WHERE type = '" +
                      DatabaseManager::getInstance().escape(keyword) + "'";
```

**Problem**: The `keyword` parameter is incorrectly used as the `type` field value instead of searching in the `title` field.

## Fix Applied

**File**: `e:\PaperCrawler\core\src\core\PaperCrawlerAPI.cpp`
**Lines**: 172-188

**Fixed Code**:
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

## Test Results Before Fix

```bash
# Test 1: Simple search
curl "http://localhost:8080/api/search?q=learning"
# Returns: Papers with title "Paper 1: Deep Learning for learning"

# Test 2: With offset and limit
curl "http://localhost:8080/api/search?q=learning&offset=3&limit=2"
# Returns: Papers with title "Paper 1: Deep Learning for learning&offset=3&limit=2"
# BUG: The entire query string is treated as the keyword!
```

## Expected Behavior After Fix

```bash
# Test 1: Simple search
curl "http://localhost:8080/api/search?q=learning"
# Should return: Papers where title contains "learning"

# Test 2: With offset and limit
curl "http://localhost:8080/api/search?q=learning&offset=3&limit=2"
# Should return: Papers 4-5 where title contains "learning"
```

## Build Instructions

To apply this fix, you need to rebuild the backend server:

### Option 1: Using MSVC (Visual Studio)

```bash
cd e:/PaperCrawler/core
mkdir -p build && cd build
cmake ..
cmake --build . --config Release
```

### Option 2: Using MinGW

```bash
cd e:/PaperCrawler/core
mkdir -p build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ ..
cmake --build .
```

### Option 3: Using the provided build scripts

```bash
cd e:/PaperCrawler/backend
# Edit build configuration if needed
./build.sh  # or build.bat on Windows
```

## Additional Changes Made

**File**: `e:\PaperCrawler\core\CMakeLists.txt`

Made OpenSSL optional to allow building without OpenSSL dependencies:

```cmake
# Changed from:
find_package(OpenSSL REQUIRED)

# To:
find_package(OpenSSL)  # Make OpenSSL optional for now

# And made linking conditional:
if(OPENSSL_FOUND)
    target_link_libraries(PaperCrawlerCore PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()
```

## Testing After Rebuild

1. Stop the current backend server
2. Replace the old binary with the newly built one
3. Start the backend server
4. Test pagination:
   ```bash
   curl "http://localhost:8080/api/search?q=test&offset=0&limit=5"
   curl "http://localhost:8080/api/search?q=test&offset=5&limit=5"
   curl "http://localhost:8080/api/search?q=test&offset=10&limit=5"
   ```
5. Verify that each request returns different papers (proper pagination)
6. Verify titles don't contain the URL parameters

## Status

- ✅ Bug identified and documented
- ✅ Fix implemented in source code
- ⏳ Rebuild pending (requires OpenSSL or build environment setup)
- ⏳ Testing pending (after rebuild)

## Notes

- The desktop client has been successfully compiled and tested
- The desktop client is ready to test pagination once the backend is fixed
- Consider setting up vcpkg or similar package manager for OpenSSL dependencies
- Alternatively, make OpenSSL completely optional if not used in the current codebase

## Related Files

- `e:\PaperCrawler\core\src\core\PaperCrawlerAPI.cpp` - Bug fix location
- `e:\PaperCrawler\core\CMakeLists.txt` - Build configuration changes
- `e:\PaperCrawler\backend\src\api_server.cpp` - API server (uses PaperCrawlerAPI)
- `e:\PaperCrawler\desktop\src\MainWindow.cpp` - Desktop client (ready for testing)
