#!/bin/bash
# Run all (or filtered) module API tests
# Usage:
#   ./run_all_tests.sh                              # All tests
#   ./run_all_tests.sh --ci                        # CI mode: auto-sync + JUnit + retry
#   ./run_all_tests.sh --module auth                # Only auth
#   ./run_all_tests.sh --filter "GET"               # Only GET routes
#   ./run_all_tests.sh --dry-run                    # Preview without running
#   ./run_all_tests.sh --junit results.xml          # JUnit XML output
#   ./run_all_tests.sh --json results.json          # JSON output
#   ./run_all_tests.sh --retry 2 --perf 500         # Retry + perf threshold
#   ./run_all_tests.sh --help

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/lib/test_framework.sh"

# ─── Parse args ─────────────────────────────────────────────────────
MODULE_FILTER=""
SYNC_MODE=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --module|-m)    MODULE_FILTER="$2"; shift 2 ;;
        --filter|-f)    TEST_FILTER="$2"; shift 2 ;;
        --dry-run|-d)   DRY_RUN=1; shift ;;
        --verbose|-v)   VERBOSE=1; shift ;;
        --retry|-r)     RETRY_COUNT="$2"; shift 2 ;;
        --timeout|-t)   TIMEOUT="$2"; shift 2 ;;
        --base-url)     BASE_URL="$2"; shift 2 ;;
        --junit|-j)     JUNIT_FILE="$2"; shift 2 ;;
        --json)         JSON_FILE="$2"; shift 2 ;;
        --perf|-p)      PERF_THRESHOLD_MS="$2"; shift 2 ;;
        --sync|-s)      SYNC_MODE="1"; shift ;;
        --sync-dry-run) SYNC_MODE="dry-run"; shift ;;
        --ci)
            SYNC_MODE="1"
            RETRY_COUNT="${RETRY_COUNT:-2}"
            JUNIT_FILE="${JUNIT_FILE:-test-results.xml}"
            JSON_FILE="${JSON_FILE:-test-results.json}"
            VERBOSE="${VERBOSE:-0}"
            PERF_THRESHOLD_MS="${PERF_THRESHOLD_MS:-3000}"
            shift ;;
        --list|-l)
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
            exit 0
            ;;
        --help|-h)
            echo "Usage: $0 [options]"
            echo ""
            echo "Test execution:"
            echo "  --module, -m <name>   Run single module"
            echo "  --filter, -f <regex>  Run only matching tests (grep on method+path+name)"
            echo "  --dry-run, -d         Preview tests without executing"
            echo "  --retry, -r <n>       Retry failed tests n times"
            echo "  --verbose, -v         Show response bodies on failure"
            echo ""
            echo "Output:"
            echo "  --junit, -j <file>    Write JUnit XML report"
            echo "  --json <file>         Write JSON report"
            echo ""
            echo "Config:"
            echo "  --base-url <url>      Override base URL"
            echo "  --timeout, -t <sec>   Request timeout (default: 10)"
            echo "  --perf, -p <ms>       Warn on tests slower than <ms>"
            echo "  --sync, -s            Sync routes from source before testing"
            echo "  --sync-dry-run        Show sync diff without modifying files"
            echo "  --ci                  CI mode: auto-sync + JUnit + retry + JSON"
            echo ""
            echo "Other:"
            echo "  --list, -l            List available modules"
            echo "  --help, -h            Show this help"
            echo ""
            echo "Examples:"
            echo "  $0                                    # Run all"
            echo "  $0 --ci                               # CI: sync + test + reports"
            echo "  $0 -m auth -r 1                      # Auth with retry"
            echo "  $0 -f 'POST' -v                      # Only POST, verbose"
            echo "  $0 --dry-run                          # Preview"
            echo "  $0 -j junit.xml --perf 500           # CI with JUnit + perf"
            exit 0
            ;;
        *) echo "Unknown option: $1. Run '$0 --help'."; exit 1 ;;
    esac
done

# ─── Sync ─────────────────────────────────────────────────────────────
if [ -n "$SYNC_MODE" ]; then
    SYNC_SCRIPT="$SCRIPT_DIR/../scripts/sync_routes.sh"
    if [ ! -f "$SYNC_SCRIPT" ]; then
        echo "ERROR: sync_routes.sh not found at $SYNC_SCRIPT"
        exit 1
    fi
    sync_args=""
    [ "$SYNC_MODE" = "dry-run" ] && sync_args="--dry-run"
    [ -n "$MODULE_FILTER" ] && sync_args="$sync_args --module $MODULE_FILTER"
    bash "$SYNC_SCRIPT" $sync_args
    [ "$SYNC_MODE" = "dry-run" ] && exit 0
    echo ""
fi

# ─── Run ─────────────────────────────────────────────────────────────
if [ -n "$MODULE_FILTER" ]; then
    echo "================================================================"
    echo "  PaperCrawler API Test: $MODULE_FILTER"
    echo "  $(date '+%Y-%m-%d %H:%M:%S')"
    echo "  Base URL: $BASE_URL | Retry: $RETRY_COUNT | Timeout: ${TIMEOUT}s"
    [ "$DRY_RUN" = "1" ] && echo "  Mode: DRY-RUN"
    [ -n "$TEST_FILTER" ] && echo "  Filter: $TEST_FILTER"
    echo "================================================================"
    run_single_module "$MODULE_FILTER"
else
    run_all_modules
fi

print_final_summary
_exit=$?
echo ""
exit $_exit
