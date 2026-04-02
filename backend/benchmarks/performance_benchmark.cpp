/**
 * PaperCrawler Performance Benchmark Suite
 *
 * Compile: g++ -O3 -std=c++17 -pthread -I../include performance_benchmark.cpp -o benchmark -lbenchmark -lpthread
 * Run: ./benchmark --benchmark_out=baseline.json --benchmark_out_format=json
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <mutex>
#include <functional>
#include <future>

// Mock implementations for benchmarking
class MockDatabase {
public:
    std::vector<std::map<std::string, std::string>> query(const std::string& sql) {
        std::this_thread::sleep_for(std::chrono::microseconds(100)); // Simulate 100μs query
        return { {{"id", "1"}, {"title", "Paper 1"}} };
    }

    bool execute(const std::string& sql) {
        std::this_thread::sleep_for(std::chrono::microseconds(50)); // Simulate 50μs execute
        return true;
    }
};

// ============================================================================
// Benchmark 1: Virtual Function Overhead
// ============================================================================

class InterfaceBase {
public:
    virtual ~InterfaceBase() = default;
    virtual std::string getName() const = 0;
    virtual int process(int value) = 0;
};

class Implementation : public InterfaceBase {
public:
    std::string getName() const override { return "Implementation"; }
    int process(int value) override { return value * 2; }
};

class DirectCall {
public:
    std::string getName() const { return "DirectCall"; }
    int process(int value) { return value * 2; }
};

static void BM_VirtualFunctionCall(benchmark::State& state) {
    InterfaceBase* obj = new Implementation();
    int result = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result += obj->process(42));
    }
    delete obj;
}
BENCHMARK(BM_VirtualFunctionCall);

static void BM_DirectFunctionCall(benchmark::State& state) {
    DirectCall obj;
    int result = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result += obj.process(42));
    }
}
BENCHMARK(BM_DirectFunctionCall);

// ============================================================================
// Benchmark 2: std::map vs std::unordered_map
// ============================================================================

static void BM_MapLookup(benchmark::State& state) {
    std::map<std::string, int> map;
    for (int i = 0; i < 1000; ++i) {
        map["key_" + std::to_string(i)] = i;
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find("key_500"));
    }
}
BENCHMARK(BM_MapLookup);

static void BM_UnorderedMapLookup(benchmark::State& state) {
    std::unordered_map<std::string, int> map;
    map.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        map["key_" + std::to_string(i)] = i;
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find("key_500"));
    }
}
BENCHMARK(BM_UnorderedMapLookup);

static void BM_MapInsert(benchmark::State& state) {
    std::map<std::string, int> map;
    int i = 0;
    for (auto _ : state) {
        map["key_" + std::to_string(i++)] = i;
    }
}
BENCHMARK(BM_MapInsert);

static void BM_UnorderedMapInsert(benchmark::State& state) {
    std::unordered_map<std::string, int> map;
    map.reserve(1000);
    int i = 0;
    for (auto _ : state) {
        map["key_" + std::to_string(i++)] = i;
    }
}
BENCHMARK(BM_UnorderedMapInsert);

// ============================================================================
// Benchmark 3: Smart Pointer Overhead
// ============================================================================

static void BM_RawPtrCopy(benchmark::State& state) {
    int* ptr = new int(42);
    int* copy;
    for (auto _ : state) {
        copy = ptr;
        benchmark::DoNotOptimize(copy);
    }
    delete ptr;
}
BENCHMARK(BM_RawPtrCopy);

static void BM_SharedPtrCopy(benchmark::State& state) {
    auto ptr = std::make_shared<int>(42);
    std::shared_ptr<int> copy;
    for (auto _ : state) {
        copy = ptr;
        benchmark::DoNotOptimize(copy);
    }
}
BENCHMARK(BM_SharedPtrCopy);

static void BM_UniquePtrMove(benchmark::State& state) {
    auto ptr = std::make_unique<int>(42);
    std::unique_ptr<int> copy;
    for (auto _ : state) {
        copy = std::move(ptr);
        benchmark::DoNotOptimize(copy);
        ptr = std::move(copy);
    }
}
BENCHMARK(BM_UniquePtrMove);

// ============================================================================
// Benchmark 4: std::function Overhead
// ============================================================================

static int directFunction(int x) {
    return x * 2;
}

static void BM_DirectFunctionCall_Static(benchmark::State& state) {
    int result = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result += directFunction(42));
    }
}
BENCHMARK(BM_DirectFunctionCall_Static);

static void BM_StdFunction(benchmark::State& state) {
    std::function<int(int)> func = [](int x) { return x * 2; };
    int result = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result += func(42));
    }
}
BENCHMARK(BM_StdFunction);

static void BM_FunctionPointer(benchmark::State& state) {
    int (*func)(int) = [](int x) { return x * 2; };
    int result = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result += func(42));
    }
}
BENCHMARK(BM_FunctionPointer);

static void BM_Lambda(benchmark::State& state) {
    auto func = [](int x) { return x * 2; };
    int result = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result += func(42));
    }
}
BENCHMARK(BM_Lambda);

// ============================================================================
// Benchmark 5: Mutex Lock Overhead
// ============================================================================

static void BM_NoLock(benchmark::State& state) {
    int counter = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(++counter);
    }
}
BENCHMARK(BM_NoLock);

static void BM_MutexLock(benchmark::State& state) {
    std::mutex mutex;
    int counter = 0;
    for (auto _ : state) {
        std::lock_guard<std::mutex> lock(mutex);
        benchmark::DoNotOptimize(++counter);
    }
}
BENCHMARK(BM_MutexLock);

static void BM_Atomic(benchmark::State& state) {
    std::atomic<int> counter{0};
    for (auto _ : state) {
        benchmark::DoNotOptimize(++counter);
    }
}
BENCHMARK(BM_Atomic);

// ============================================================================
// Benchmark 6: String Operations
// ============================================================================

static void BM_StringCopy(benchmark::State& state) {
    std::string source = "This is a test string for benchmarking";
    for (auto _ : state) {
        std::string copy = source;
        benchmark::DoNotOptimize(copy);
    }
}
BENCHMARK(BM_StringCopy);

static void BM_StringMove(benchmark::State& state) {
    std::string source = "This is a test string for benchmarking";
    for (auto _ : state) {
        std::string copy = std::move(source);
        benchmark::DoNotOptimize(copy);
        source = std::move(copy);
    }
}
BENCHMARK(BM_StringMove);

static void BM_StringConcatenation(benchmark::State& state) {
    std::string result;
    for (auto _ : state) {
        result = "SELECT * FROM papers WHERE id = " + std::to_string(42);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_StringConcatenation);

static void BM_StringStream(benchmark::State& state) {
    for (auto _ : state) {
        std::ostringstream oss;
        oss << "SELECT * FROM papers WHERE id = " << 42;
        std::string result = oss.str();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_StringStream);

// ============================================================================
// Benchmark 7: Database Query Simulation
// ============================================================================

static void BM_DatabaseQuery_Sync(benchmark::State& state) {
    MockDatabase db;
    for (auto _ : state) {
        auto results = db.query("SELECT * FROM papers");
        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(BM_DatabaseQuery_Sync);

static void BM_DatabaseQuery_Async(benchmark::State& state) {
    MockDatabase db;
    for (auto _ : state) {
        auto future = std::async(std::launch::async, [&db]() {
            return db.query("SELECT * FROM papers");
        });
        auto results = future.get();
        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(BM_DatabaseQuery_Async);

// ============================================================================
// Benchmark 8: Thread Pool Task Submission
// ============================================================================

class SimpleThreadPool {
public:
    SimpleThreadPool(size_t threads) : running_(true) {
        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this]() { workerThread(); });
        }
    }

    ~SimpleThreadPool() {
        running_ = false;
        condition_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }

    void submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.push(std::move(task));
        }
        condition_.notify_one();
    }

private:
    void workerThread() {
        while (running_) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this]() { return !tasks_.empty() || !running_; });
                if (!running_) break;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> running_;
};

static void BM_ThreadPoolSubmission(benchmark::State& state) {
    SimpleThreadPool pool(4);
    for (auto _ : state) {
        pool.submit([]() { /* empty task */ });
    }
}
BENCHMARK(BM_ThreadPoolSubmission);

// ============================================================================
// Benchmark 9: Cache Performance
// ============================================================================

static void BM_CacheHit_L1(benchmark::State& state) {
    int data[1024];
    for (auto _ : state) {
        for (int i = 0; i < 1024; ++i) {
            benchmark::DoNotOptimize(data[i] *= 2);
        }
    }
}
BENCHMARK(BM_CacheHit_L1);

static void BM_CacheMiss_Random(benchmark::State& state) {
    int data[1024];
    int indices[1024];
    for (int i = 0; i < 1024; ++i) indices[i] = rand() % 1024;

    for (auto _ : state) {
        for (int i = 0; i < 1024; ++i) {
            benchmark::DoNotOptimize(data[indices[i]] *= 2);
        }
    }
}
BENCHMARK(BM_CacheMiss_Random);

// ============================================================================
// Benchmark 10: Memory Allocation
// ============================================================================

static void BM_HeapAllocation_New(benchmark::State& state) {
    for (auto _ : state) {
        int* ptr = new int(42);
        benchmark::DoNotOptimize(ptr);
        delete ptr;
    }
}
BENCHMARK(BM_HeapAllocation_New);

static void BM_HeapAllocation_MakeShared(benchmark::State& state) {
    for (auto _ : state) {
        auto ptr = std::make_shared<int>(42);
        benchmark::DoNotOptimize(ptr);
    }
}
BENCHMARK(BM_HeapAllocation_MakeShared);

static void BM_StackAllocation(benchmark::State& state) {
    for (auto _ : state) {
        int value = 42;
        benchmark::DoNotOptimize(value);
    }
}
BENCHMARK(BM_StackAllocation);

// ============================================================================
// Benchmark 11: Vector Operations
// ============================================================================

static void BM_VectorPushBack_NoReserve(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int> vec;
        for (int i = 0; i < 1000; ++i) {
            vec.push_back(i);
        }
        benchmark::DoNotOptimize(vec);
    }
}
BENCHMARK(BM_VectorPushBack_NoReserve);

static void BM_VectorPushBack_WithReserve(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int> vec;
        vec.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            vec.push_back(i);
        }
        benchmark::DoNotOptimize(vec);
    }
}
BENCHMARK(BM_VectorPushBack_WithReserve);

static void BM_VectorIteration(benchmark::State& state) {
    std::vector<int> vec(1000);
    for (auto _ : state) {
        for (auto& val : vec) {
            benchmark::DoNotOptimize(val *= 2);
        }
    }
}
BENCHMARK(BM_VectorIteration);

// ============================================================================
// Benchmark 12: JSON Parsing Simulation
// ============================================================================

static void BM_JSON_Parse_Small(benchmark::State& state) {
    std::string json = R"({"id": 1, "title": "Paper Title", "authors": "Author Name"})";
    for (auto _ : state) {
        // Simulate JSON parsing
        size_t pos = 0;
        while ((pos = json.find("\"", pos)) != std::string::npos) {
            benchmark::DoNotOptimize(pos);
            pos += 1;
        }
    }
}
BENCHMARK(BM_JSON_Parse_Small);

static void BM_JSON_Parse_Large(benchmark::State& state) {
    std::string json;
    for (int i = 0; i < 100; ++i) {
        json += R"({"id": )" + std::to_string(i) +
                R"(, "title": "Paper Title )" + std::to_string(i) +
                R"(", "authors": "Author Name )" + std::to_string(i) + R"("},)";
    }

    for (auto _ : state) {
        size_t pos = 0;
        while ((pos = json.find("\"", pos)) != std::string::npos) {
            benchmark::DoNotOptimize(pos);
            pos += 1;
        }
    }
}
BENCHMARK(BM_JSON_Parse_Large);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
