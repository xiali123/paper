#!/bin/bash
# Run tests for a single module
# Usage:
#   ./run_module_test.sh auth             # Run auth tests
#   ./run_module_test.sh auth -f login    # Filter to login tests
#   ./run_module_test.sh auth --dry-run   # Preview only
#   ./run_module_test.sh --list           # List modules
#   ./run_module_test.sh --help

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib/test_framework.sh"

# ─── List ────────────────────────────────────────────────────────────
if [ "$1" = "--list" ] || [ "$1" = "-l" ]; then
    echo "Available test modules:"
    echo ""
    printf "  %-20s %5s  %s\n" "MODULE" "ROUTES" "NAME"
    printf "  %-20s %5s  %s\n" "------" "------" "----"
    for f in "$SCRIPT_DIR/routes/"*_routes.sh; do
        [ -f "$f" ] || continue
        ROUTES=(); MODULE_NAME=""
        source "$f"
        printf "  %-20s %5d  %s\n" "$(basename "$f" _routes.sh)" "${#ROUTES[@]}" "${MODULE_NAME:-unknown}"
    done
    echo ""
    echo "Usage: $0 <module> [options]"
    exit 0
fi

# ─── Parse flags (module name is first non-flag arg) ─────────────────
MODULE_NAME_ARG=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --filter|-f)    TEST_FILTER="$2"; shift 2 ;;
        --dry-run|-d)   DRY_RUN=1; shift ;;
        --verbose|-v)   VERBOSE=1; shift ;;
        --retry|-r)     RETRY_COUNT="$2"; shift 2 ;;
        --timeout|-t)   TIMEOUT="$2"; shift 2 ;;
        --base-url)     BASE_URL="$2"; shift 2 ;;
        --junit|-j)     JUNIT_FILE="$2"; shift 2 ;;
        --json)         JSON_FILE="$2"; shift 2 ;;
        --perf|-p)      PERF_THRESHOLD_MS="$2"; shift 2 ;;
        --help|-h)
            echo "Usage: $0 <module> [options]"
            echo ""
            echo "  --filter, -f <regex>  Filter tests"
            echo "  --dry-run, -d         Preview without running"
            echo "  --verbose, -v         Show response bodies"
            echo "  --retry, -r <n>       Retry count"
            echo "  --timeout, -t <sec>   Request timeout"
            echo "  --junit, -j <file>    JUnit XML output"
            echo "  --json <file>         JSON output"
            echo "  --perf, -p <ms>       Warn if slower than <ms>"
            echo "  --list, -l            List modules"
            echo ""
            echo "Examples:"
            echo "  $0 auth"
            echo "  $0 auth -f login -v"
            echo "  $0 paper --dry-run"
            echo "  $0 admin -j results.xml"
            exit 0
            ;;
        -*)
            echo "Unknown flag: $1"
            exit 1
            ;;
        *)
            MODULE_NAME_ARG="$1"
            shift
            ;;
    esac
done

if [ -z "$MODULE_NAME_ARG" ]; then
    echo "Usage: $0 <module> [options]"
    echo "       $0 --list"
    echo "       $0 --help"
    exit 1
fi

echo "================================================================"
echo "  PaperCrawler Module Test: $MODULE_NAME_ARG"
echo "  $(date '+%Y-%m-%d %H:%M:%S')"
echo "  Base URL: $BASE_URL | Retry: $RETRY_COUNT | Timeout: ${TIMEOUT}s"
[ "$DRY_RUN" = "1" ] && echo "  Mode: DRY-RUN"
[ -n "$TEST_FILTER" ] && echo "  Filter: $TEST_FILTER"
echo "================================================================"

run_single_module "$MODULE_NAME_ARG"

print_final_summary
_exit=$?
echo ""
exit $_exit
