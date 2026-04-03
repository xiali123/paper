# PaperCrawler::Core

## Introduction

**PaperCrawler::Core** is a high-performance, production-ready C++ backend framework that provides essential infrastructure for building scalable, maintainable C++ applications.

### Key Features

- **🔧 Modular Architecture** - Plugin-based module system with lifecycle management
- **💉 Dependency Injection** - Powerful IoC container with auto-wiring
- **📡 Event-Driven** - Unified event bus for pub/sub messaging
- **⚙️ Configuration Management** - Multi-format config with hot-reload
- **🛡️ Error Handling** - Structured exception handling with recovery strategies
- **🧵 Thread Pool** - Dynamic thread pool with task scheduling
- **📝 Logging** - Structured logging with context support
- **🔨 Utilities** - Comprehensive string, time, file, and type utilities

### Quick Start

```cpp
#include <PaperCrawler/Core>

using namespace PaperCrawler::Core;

class MyModule : public ModuleBase {
public:
    bool initialize() override {
        getLogger()->info("Module initialized!");
        return true;
    }
};

int main() {
    auto container = std::make_shared<ServiceContainer>();
    container->registerModule<MyModule>("myModule");

    auto module = container->getModule<MyModule>("myModule");
    module->initialize();
    module->start();

    return 0;
}
```

### Performance

| Feature | Performance |
|---------|-------------|
| Virtual function overhead | <5ns |
| Event latency | <1ms |
| Thread pool throughput | 10M+ tasks/sec |
| Memory footprint | ~2MB |

### Documentation

- [API Reference](modules.html)
- [Class Index](annotated.html)
- [File Index](files.html)
- [Examples](examples.html)

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Components](#core-components)
3. [Examples](#examples)
4. [Building](#building)
5. [Contributing](#contributing)

---

## Architecture Overview

PaperCrawler::Core follows a modular, event-driven architecture:

```
┌─────────────────────────────────────────┐
│         Application Layer               │
├─────────────────────────────────────────┤
│  ┌─────────┐ ┌──────────┐ ┌─────────┐  │
│  │Modules  │ │Services  │ │Events   │  │
│  └────┬────┘ └────┬─────┘ └────┬────┘  │
├───────┼────────────┼────────────┼────────┤
│       │            │            │        │
│  ┌────▼────────────▼────────────▼────┐  │
│  │     Core Framework Layer          │  │
│  │ ModuleBase | ServiceContainer     │  │
│  │ EventBus | ConfigManager         │  │
│  │ ThreadPool | ErrorHandler        │  │
│  └───────────────────────────────────┘  │
├─────────────────────────────────────────┤
│         Utility Layer                   │
│  Logger | String | Time | File | Type  │
└─────────────────────────────────────────┘
```

### Design Principles

1. **Header-Only Library** - Easy integration, no compilation required
2. **Zero-Cost Abstractions** - Modern C++ with minimal overhead
3. **RAII** - Automatic resource management
4. **Thread Safety** - Multi-threaded by design
5. **Extensibility** - Plugin architecture for custom modules

---

## Core Components

### ModuleBase

Module lifecycle management with state machine.

```cpp
class MyModule : public ModuleBase {
public:
    std::string getName() const override { return "MyModule"; }
    std::string getVersion() const override { return "1.0.0"; }

protected:
    bool initialize() override {
        // Initialization logic
        return true;
    }

    bool start() override {
        // Start logic
        return true;
    }

    bool stop() override {
        // Stop logic
        return true;
    }

    void cleanup() override {
        // Cleanup resources
    }
};
```

### ServiceContainer

Dependency injection container with auto-wiring.

```cpp
auto container = std::make_shared<ServiceContainer>();

// Register services
container->registerService<IDatabase, MySQLDatabase>(ServiceLifetime::Singleton);
container->registerService<ICache, RedisCache>(ServiceLifetime::Transient);

// Resolve services
auto db = container->getService<IDatabase>();
auto cache = container->getService<ICache>();

// Auto dependency injection
container->registerService<UserRepository, UserRepository>();
auto repo = container->getService<UserRepository>(); // DB injected automatically
```

### EventBus

Publish-subscribe event system.

```cpp
auto eventBus = std::make_shared<EventBus>();

// Subscribe to events
eventBus->subscribe("user.created", [](const EventData& data) {
    int userId = data.get<int>("userId");
    std::cout << "User created: " << userId << std::endl;
});

// Publish events
eventBus->publish("user.created", {{"userId", 123}});
```

### ConfigManager

Configuration management with hot-reload.

```cpp
auto& config = ConfigManager::getInstance();

// Load configuration
config.loadFromFile("config.json");

// Get values
std::string dbHost = config.getString("database.host");
int dbPort = config.getInt("database.port", 3306);

// Watch for changes
config.watch("database.password", [](const std::string& newValue) {
    std::cout << "Password changed" << std::endl;
});
```

---

## Examples

### Example 1: Minimal Module

@see MinimalModuleExample.cpp

### Example 2: Dependency Injection

@see DependencyInjectionExample.cpp

### Example 3: Event-Driven Architecture

@see EventDrivenExample.cpp

### Example 4: Complete Application

@see CompleteApplicationExample.cpp

---

## Building

### Requirements

- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+
- spdlog
- fmt

### Build from Source

```bash
git clone https://github.com/PaperCrawler/Core.git
cd Core
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### Using CMake

```cmake
find_package(PaperCrawlerCore REQUIRED)
target_link_libraries(myapp PRIVATE PaperCrawlerCore::PaperCrawlerCore)
```

### Using Docker

```bash
docker build -t papercrawler-core .
docker run papercrawler-core
```

---

## Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Development Setup

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Coding Standards

- Follow C++ Core Guidelines
- Use clang-format for code formatting
- Write unit tests for new features
- Update documentation

---

## License

MIT License - see [LICENSE](../LICENSE) for details.

---

## Contact

- Website: https://papercrawler.io
- Documentation: https://docs.papercrawler.io
- Issues: https://github.com/PaperCrawler/Core/issues

---

*Generated by Doxygen*
