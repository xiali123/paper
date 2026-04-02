#!/bin/bash

# PaperCrawler Backend Performance Benchmark Script
# This script performs comprehensive performance testing of the PaperCrawler backend

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
BACKEND_URL="http://localhost:8080"
OUTPUT_DIR="benchmark_results"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
RESULT_FILE="${OUTPUT_DIR}/benchmark_${TIMESTAMP}.json"
SUMMARY_FILE="${OUTPUT_DIR}/summary_${TIMESTAMP}.txt"

# Create output directory
mkdir -p "${OUTPUT_DIR}"

echo -e "${GREEN}PaperCrawler Performance Benchmark${NC}"
echo "=========================================="
echo "Backend URL: ${BACKEND_URL}"
echo "Output Directory: ${OUTPUT_DIR}"
echo "Timestamp: ${TIMESTAMP}"
echo ""

# Function to check backend health
check_health() {
    echo -e "${YELLOW}Checking backend health...${NC}"
    response=$(curl -s -o /dev/null -w "%{http_code}" "${BACKEND_URL}/health" || echo "000")

    if [ "$response" == "200" ]; then
        echo -e "${GREEN}✓ Backend is healthy${NC}"
        return 0
    else
        echo -e "${RED}✗ Backend is not healthy (HTTP ${response})${NC}"
        return 1
    fi
}

# Function to benchmark endpoint
benchmark_endpoint() {
    local endpoint=$1
    local method=${2:-GET}
    local data=${3:-}
    local concurrent_requests=${4:-10}
    local total_requests=${5:-1000}

    echo -e "${YELLOW}Benchmarking: ${method} ${endpoint}${NC}"
    echo "  Concurrent Requests: ${concurrent_requests}"
    echo "  Total Requests: ${total_requests}"

    # Prepare ab command
    if [ "$method" == "GET" ]; then
        ab_command="ab -n ${total_requests} -c ${concurrent_requests} -g ${OUTPUT_DIR}/gnuplot_${endpoint//\//_}.txt ${BACKEND_URL}${endpoint}"
    elif [ "$method" == "POST" ]; then
        ab_command="ab -n ${total_requests} -c ${concurrent_requests} -p ${data} -T application/json -g ${OUTPUT_DIR}/gnuplot_${endpoint//\//_}.txt ${BACKEND_URL}${endpoint}"
    fi

    # Run benchmark
    result=$(${ab_command} 2>&1)

    # Parse results
    rps=$(echo "$result" | grep "Requests per second" | awk '{print $4}')
    mean=$(echo "$result" | grep "Time per request.*mean" | awk '{print $4}')
    p95=$(echo "$result" | grep "95%" | awk '{print $2}')
    p99=$(echo "$result" | grep "99%" | awk '{print $2}')
    failed=$(echo "$result" | grep "Failed requests" | awk '{print $3}')

    # Output results
    echo "  Results:"
    echo "    Requests/sec: ${rps}"
    echo "    Mean Latency: ${mean}ms"
    echo "    P95 Latency: ${p95}ms"
    echo "    P99 Latency: ${p99}ms"
    echo "    Failed Requests: ${failed}"
    echo ""

    # Save to JSON
    cat >> ${RESULT_FILE} << EOF
{
  "endpoint": "${endpoint}",
  "method": "${method}",
  "concurrent_requests": ${concurrent_requests},
  "total_requests": ${total_requests},
  "requests_per_second": ${rps},
  "mean_latency_ms": ${mean},
  "p95_latency_ms": ${p95},
  "p99_latency_ms": ${p99},
  "failed_requests": ${failed}
},
EOF
}

# Function to benchmark search performance
benchmark_search() {
    echo -e "${YELLOW}Benchmarking Search Performance${NC}"

    # Test different query types
    declare -a queries=(
        "machine+learning"
        "deep+learning+neural+networks"
        "quantum+computing+algorithms"
        "bioinformatics+genomic+analysis"
        "computer+vision+image+processing"
    )

    for query in "${queries[@]}"; do
        echo "  Testing query: ${query}"
        benchmark_endpoint "/api/papers/search?q=${query}" "GET" "" 10 500
    done
}

# Function to stress test
stress_test() {
    echo -e "${YELLOW}Running Stress Test...${NC}"

    declare -a concurrency_levels=(10 25 50 100 200)

    for level in "${concurrency_levels[@]}"; do
        echo "  Concurrency Level: ${level}"
        benchmark_endpoint "/api/papers" "GET" "" ${level} 1000
    done
}

# Function to test memory usage
test_memory() {
    echo -e "${YELLOW}Testing Memory Usage...${NC}"

    # Get initial memory
    initial_mem=$(ps aux | grep PaperCrawlerServer | grep -v grep | awk '{sum+=$4} END {print sum}')

    # Run load test
    echo "  Running load test..."
    ab -n 10000 -c 50 ${BACKEND_URL}/api/papers > /dev/null 2>&1

    # Wait for stabilization
    sleep 5

    # Get final memory
    final_mem=$(ps aux | grep PaperCrawlerServer | grep -v grep | awk '{sum+=$4} END {print sum}')

    echo "  Initial Memory: ${initial_mem}%"
    echo "  Final Memory: ${final_mem}%"
    echo "  Memory Growth: $(echo "$final_mem - $initial_mem" | bc)%"
    echo ""
}

# Function to test database pool
test_database_pool() {
    echo -e "${YELLOW}Testing Database Connection Pool...${NC}"

    # Get pool statistics
    pool_stats=$(curl -s "${BACKEND_URL}/api/database/stats" || echo "{}")

    echo "  Pool Statistics:"
    echo "${pool_stats}" | jq '.' 2>/dev/null || echo "${pool_stats}"
    echo ""
}

# Function to test cache efficiency
test_cache() {
    echo -e "${YELLOW}Testing Cache Efficiency...${NC}"

    # Get cache statistics
    cache_stats=$(curl -s "${BACKEND_URL}/api/cache/stats" || echo "{}")

    echo "  Cache Statistics:"
    echo "${cache_stats}" | jq '.' 2>/dev/null || echo "${cache_stats}"
    echo ""

    # Test cache hit rate
    echo "  Testing cache hit rate..."

    # First call (cache miss)
    start_time=$(date +%s%N)
    curl -s "${BACKEND_URL}/api/papers/search?q=test" > /dev/null
    first_call_time=$((($(date +%s%N) - start_time) / 1000000))

    # Second call (cache hit)
    start_time=$(date +%s%N)
    curl -s "${BACKEND_URL}/api/papers/search?q=test" > /dev/null
    second_call_time=$((($(date +%s%N) - start_time) / 1000000))

    echo "    First Call (Cache Miss): ${first_call_time}ms"
    echo "    Second Call (Cache Hit): ${second_call_time}ms"
    echo "    Speedup: $(echo "scale=2; $first_call_time / $second_call_time" | bc)x"
    echo ""
}

# Function to generate summary report
generate_summary() {
    echo -e "${YELLOW}Generating Summary Report...${NC}"

    cat > ${SUMMARY_FILE} << EOF
PaperCrawler Performance Benchmark Summary
==========================================
Date: $(date)
Backend URL: ${BACKEND_URL}

EXECUTIVE SUMMARY
-----------------

1. HEALTH CHECK
EOF

    if check_health; then
        echo "✓ Backend is healthy and responsive" >> ${SUMMARY_FILE}
    else
        echo "✗ Backend health check failed" >> ${SUMMARY_FILE}
    fi

    cat >> ${SUMMARY_FILE} << EOF

2. PERFORMANCE METRICS
EOF

    # Parse JSON results and add to summary
    if [ -f "${RESULT_FILE}" ]; then
        echo "JSON results saved to: ${RESULT_FILE}" >> ${SUMMARY_FILE}
    fi

    cat >> ${SUMMARY_FILE} << EOF

3. RESOURCE UTILIZATION
EOF

    # Add system resource information
    echo "CPU Cores: $(nproc)" >> ${SUMMARY_FILE}
    echo "Total Memory: $(free -h | awk '/^Mem:/ {print $2}')" >> ${SUMMARY_FILE}
    echo "Available Memory: $(free -h | awk '/^Mem:/ {print $7}')" >> ${SUMMARY_FILE}

    cat >> ${SUMMARY_FILE} << EOF

4. RECOMMENDATIONS
EOF

    # Add performance recommendations
    echo "- Review detailed results in: ${RESULT_FILE}" >> ${SUMMARY_FILE}
    echo "- Compare with previous benchmarks to track performance trends" >> ${SUMMARY_FILE}
    echo "- Implement critical optimizations from performance analysis report" >> ${SUMMARY_FILE}

    echo ""
    echo -e "${GREEN}✓ Summary report saved to: ${SUMMARY_FILE}${NC}"
}

# Main execution
main() {
    echo "Starting comprehensive performance benchmark..."
    echo ""

    # Check if backend is running
    if ! check_health; then
        echo -e "${RED}Error: Backend is not available. Please start the backend first.${NC}"
        exit 1
    fi

    echo "Starting benchmark tests..."
    echo ""

    # Initialize JSON results file
    echo "[" > ${RESULT_FILE}

    # Run benchmark tests
    benchmark_endpoint "/health" "GET" "" 10 1000
    benchmark_endpoint "/api/papers" "GET" "" 10 1000
    benchmark_endpoint "/api/papers?page=1&limit=20" "GET" "" 10 500
    benchmark_search
    stress_test
    test_memory
    test_database_pool
    test_cache

    # Close JSON results file
    echo "{}]" >> ${RESULT_FILE}

    # Generate summary
    generate_summary

    echo -e "${GREEN}Benchmark complete!${NC}"
    echo "Results saved to: ${OUTPUT_DIR}"
    echo "Summary: ${SUMMARY_FILE}"
    echo "Detailed Results: ${RESULT_FILE}"
}

# Run main function
main "$@"