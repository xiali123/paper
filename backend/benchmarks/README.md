# PaperCrawler Performance Benchmarking Suite

Comprehensive performance testing and benchmarking tools for PaperCrawler backend optimization.

## Overview

This benchmarking suite provides:

1. **Micro-benchmarks** - Low-level performance measurements (C++ Google Benchmark)
2. **Load testing** - Concurrent user simulation (Locust)
3. **Performance comparison** - Regression detection (Python)
4. **Profiling guides** - Performance optimization workflow

## Quick Start

### 1. Install Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install libbenchmark-dev python3-pip

# macOS
brew install google-benchmark pip

# Python dependencies
pip3 install locust matplotlib requests
```

### 2. Run Micro-benchmarks

```bash
# Compile benchmarks
cd backend/benchmarks
g++ -O3 -std=c++17 -pthread -I../include \
    performance_benchmark.cpp -o benchmark -lbenchmark -lpthread

# Run benchmarks
./benchmark --benchmark_out=baseline.json --benchmark_out_format=json

# View results
./benchmark --benchmark_filter=VirtualFunction
```

### 3. Run Load Tests

```bash
# Start PaperCrawler backend
cd backend
./papercrawler --port 8080

# In another terminal, run load tests
cd backend/benchmarks
locust -f load_test.py --host=http://localhost:8080 --users 100 --spawn-rate 10

# Or headless mode
locust -f load_test.py --host=http://localhost:8080 --users 100 --spawn-rate 10 --headless -t 60s
```

### 4. Compare Performance

```bash
# After implementing optimizations, compare results
python3 compare_benchmarks.py baseline.json current.json --threshold 5

# Generate detailed report
python3 compare_benchmarks.py baseline.json current.json --report PERFORMANCE_REPORT.md
```

## Benchmark Categories

### Micro-benchmarks (`performance_benchmark.cpp`)

#### 1. Virtual Function Overhead
- **Purpose**: Measure virtual function call overhead vs direct calls
- **Impact**: 5-10ns per call (5x slower than direct calls)
- **Benchmark**: `BM_VirtualFunctionCall`, `BM_DirectFunctionCall`

#### 2. Data Structure Performance
- **Purpose**: Compare std::map vs std::unordered_map
- **Impact**: O(log n) vs O(1) lookup, 2-3x faster with unordered_map
- **Benchmark**: `BM_MapLookup`, `BM_UnorderedMapLookup`

#### 3. Smart Pointer Overhead
- **Purpose**: Measure shared_ptr reference counting overhead
- **Impact**: 50-100ns per copy (atomic operations)
- **Benchmark**: `BM_SharedPtrCopy`, `BM_RawPtrCopy`

#### 4. Function Call Overhead
- **Purpose**: Compare std::function vs lambda vs function pointer
- **Impact**: 20-50ns per std::function call
- **Benchmark**: `BM_StdFunction`, `BM_Lambda`

#### 5. Synchronization Overhead
- **Purpose**: Measure mutex lock vs atomic operations
- **Impact**: 50-100ns per mutex lock
- **Benchmark**: `BM_MutexLock`, `BM_Atomic`

#### 6. String Operations
- **Purpose**: String copy vs move vs concatenation
- **Impact**: String concatenation is 10x slower than stringstream
- **Benchmark**: `BM_StringConcatenation`, `BM_StringStream`

#### 7. Memory Allocation
- **Purpose**: Heap vs stack allocation overhead
- **Impact**: Heap allocation is 100x slower than stack
- **Benchmark**: `BM_HeapAllocation_New`, `BM_StackAllocation`

#### 8. Cache Performance
- **Purpose**: Measure L1 cache hit vs miss
- **Impact**: Cache miss is 10-100x slower
- **Benchmark**: `BM_CacheHit_L1`, `BM_CacheMiss_Random`

### Load Testing (`load_test.py`)

#### User Types

**Regular User** (80% of traffic)
- Browse papers list (50%)
- View paper details (30%)
- Search papers (15%)
- Crawl papers (5%)

**Admin User** (20% of traffic)
- View statistics (50%)
- Manage users (30%)
- Manage templates (15%)
- Create templates (5%)

#### Performance Targets

| Endpoint | Median | P95 | RPS |
|----------|--------|-----|-----|
| GET /api/papers | < 200ms | < 500ms | 100 |
| GET /api/papers/{id} | < 100ms | < 300ms | 200 |
| POST /api/crawl | < 3000ms | < 5000ms | 10 |

## Performance Optimization Workflow

### 1. Baseline Measurement

```bash
# Establish baseline performance
./benchmark --benchmark_out=baseline.json
```

### 2. Profiling

```bash
# CPU profiling
perf record -F 99 -p $(pidof papercrawler) sleep 30
perf report

# Memory profiling
valgrind --tool=massif ./papercrawler
ms_print massif.out.<pid>
```

### 3. Optimization

Implement changes based on profiling results.

### 4. Regression Testing

```bash
# Run benchmarks after optimization
./benchmark --benchmark_out=current.json

# Compare with baseline
python3 compare_benchmarks.py baseline.json current.json
```

### 5. Load Validation

```bash
# Validate under load
locust -f load_test.py --users 1000 --spawn-rate 100 --headless -t 300s
```

## Performance Optimization Checklist

### HIGH PRIORITY (Immediate Impact)

- [ ] Replace std::map with std::unordered_map in template cache
- [ ] Implement async HTTP request batching
- [ ] Optimize database connection pool (remove shared_ptr)
- [ ] Add performance monitoring (Prometheus/Grafana)

**Expected Impact**: 2-3x throughput improvement

### MEDIUM PRIORITY (Significant Improvement)

- [ ] Implement object pooling for frequent allocations
- [ ] Replace mutex with lock-free queue for task queue
- [ ] Implement cache pre-warming for templates
- [ ] Add performance regression testing to CI/CD

**Expected Impact**: 30-40% additional improvement

### LOW PRIORITY (Long-term Optimization)

- [ ] Implement SIMD optimizations (simdjson)
- [ ] Implement coroutine-based async (C++20)
- [ ] Implement custom allocators (memory pools)
- [ ] Implement distributed caching (Redis cluster)

**Expected Impact**: 20-30% additional improvement

## CI/CD Integration

### GitHub Actions Workflow

```yaml
name: Performance Tests

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libbenchmark-dev

      - name: Compile benchmarks
        run: |
          cd backend/benchmarks
          g++ -O3 -std=c++17 -pthread -I../include \
            performance_benchmark.cpp -o benchmark -lbenchmark -lpthread

      - name: Run benchmarks
        run: |
          cd backend/benchmarks
          ./benchmark --benchmark_out=current.json --benchmark_out_format=json

      - name: Compare with baseline
        run: |
          python3 backend/benchmarks/compare_benchmarks.py \
            backend/benchmarks/baseline.json \
            backend/benchmarks/current.json \
            --threshold 5

      - name: Upload benchmark results
        uses: actions/upload-artifact@v2
        with:
          name: benchmark-results
          path: backend/benchmarks/current.json
```

## Performance Metrics Dashboard

### Key Metrics to Track

1. **Throughput Metrics**
   - Requests per second (RPS)
   - Papers crawled per second
   - Database queries per second

2. **Latency Metrics**
   - P50, P95, P99 response times
   - Database query latency
   - HTTP request latency

3. **Resource Utilization**
   - CPU usage (per core)
   - Memory usage (heap, RSS)
   - Thread pool utilization
   - Connection pool utilization

4. **Cache Performance**
   - Cache hit rate (L1, L2, L3)
   - Cache miss rate
   - Cache eviction rate

### Grafana Dashboard Example

```json
{
  "dashboard": {
    "title": "PaperCrawler Performance",
    "panels": [
      {
        "title": "Requests Per Second",
        "targets": [
          {
            "expr": "rate(http_requests_total[1m])"
          }
        ]
      },
      {
        "title": "Response Time (P95)",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, http_request_duration_seconds)"
          }
        ]
      },
      {
        "title": "Cache Hit Rate",
        "targets": [
          {
            "expr": "rate(cache_hits_total[1m]) / rate(cache_requests_total[1m])"
          }
        ]
      }
    ]
  }
}
```

## Troubleshooting

### Benchmark Results Vary Between Runs

**Problem**: Inconsistent benchmark results

**Solutions**:
1. Disable CPU frequency scaling
   ```bash
   sudo cpupower frequency-set -g performance
   ```

2. Disable turbo boost
   ```bash
   echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
   ```

3. Pin process to specific CPU
   ```bash
   taskset -c 0 ./benchmark
   ```

4. Run multiple iterations
   ```bash
   ./benchmark --benchmark_repetitions=5
   ```

### Load Test Shows High Failure Rate

**Problem**: Many requests failing under load

**Solutions**:
1. Check backend logs for errors
2. Increase thread pool size
3. Increase connection pool size
4. Check for memory leaks
5. Monitor system resources

### Memory Usage Increases Over Time

**Problem**: Memory leak detected

**Solutions**:
1. Run Valgrind massif
   ```bash
   valgrind --tool=massif ./papercrawler
   ms_print massif.out.<pid>
   ```

2. Check for circular references with shared_ptr
3. Implement object pooling
4. Add memory leak detection to CI/CD

## Performance Target Checklist

### Achieve Before Production Launch

- [ ] P50 latency < 100ms for all endpoints
- [ ] P95 latency < 500ms for all endpoints
- [ ] P99 latency < 2000ms for all endpoints
- [ ] 500+ RPS sustained throughput
- [ ] 1000+ concurrent users
- [ ] 99%+ success rate under load
- [ ] No memory leaks (24-hour endurance test)
- [ ] CPU utilization < 80% per core
- [ ] Memory utilization < 4GB RSS
- [ ] 95%+ cache hit rate

## Additional Resources

### Tools

- **Google Benchmark**: https://github.com/google/benchmark
- **Locust**: https://locust.io/
- **Perf**: https://perf.wiki.kernel.org/
- **Valgrind**: https://valgrind.org/

### Documentation

- **C++ Performance Guidelines**: https://isocpp.org/blog/2019/06/quick-bench-performance
- **Google C++ Style Guide**: https://google.github.io/styleguide/cppguide.html
- **Linux Performance Tools**: http://www.brendangregg.com/linuxperf.html

### Papers

- "Latency Is Everywhere And It Costs You Sales - How To Crush It"
- "The Hidden Costs of Microseconds"
- "Performance Optimization Patterns"

## Support

For performance issues or questions:
1. Check this README
2. Review performance analysis report
3. Consult profiling results
4. Open GitHub issue with benchmark data

---

**Last Updated**: 2026-04-03
**Maintained By**: Performance Benchmarker Agent
**Status**: Active Development
