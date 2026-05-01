#!/bin/bash
# PaperCrawler Test Framework v3
# Data-driven route testing: retry, auth, validation, filter, dry-run, JUnit
#
# Route format: "METHOD|PATH|BODY|EXPECTED|NAME|HEADERS"
#   Fields 1-4 split on first 4 pipes; field 5+ = test name
#   BODY: JSON, empty, "null", or @path/to/file.json
#   HEADERS: "Key: Val; Key2: Val2", supports {{VAR}} substitution
#
# Per-module optional:
#   RESPONSE_CHECKS=(["GET /api/users"]="has_field:users")
#   ROUTE_META=(["/api/ai/chat"]="timeout=30" ["/skip"]="skip")

# ─── Config ──────────────────────────────────────────────────────────
BASE_URL="${BASE_URL:-http://localhost:8080}"
TIMEOUT="${TIMEOUT:-10}"
VERBOSE="${VERBOSE:-0}"
RETRY_COUNT="${RETRY_COUNT:-0}"
DRY_RUN="${DRY_RUN:-0}"
TEST_FILTER="${TEST_FILTER:-}"
PERF_THRESHOLD_MS="${PERF_THRESHOLD_MS:-0}"
JUNIT_FILE="${JUNIT_FILE:-}"
JSON_FILE="${JSON_FILE:-}"

# ─── Colors ──────────────────────────────────────────────────────────
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
DIM='\033[2m'
NC='\033[0m'

# ─── Global state ────────────────────────────────────────────────────
TOTAL=0
PASSED=0
FAILED=0
SKIPPED=0
MODULE_RESULTS=()
declare -A RESPONSE_CHECKS
declare -A ROUTE_META

# JUnit / JSON collectors
_JUNIT_TESTS=""
_JSON_TESTS=""

# ─── Ctrl+C handler ─────────────────────────────────────────────────
_cleanup() {
    echo -e "\n${YELLOW}Interrupted. Cleaning up...${NC}"
    jobs -p 2>/dev/null | xargs kill 2>/dev/null
    exit 130
}
trap _cleanup INT TERM

# ─── Reset counters ──────────────────────────────────────────────────
reset_counters() {
    TOTAL=0
    PASSED=0
    FAILED=0
    SKIPPED=0
}

# ─── Resolve @file body ─────────────────────────────────────────────
resolve_body() {
    local body="$1"
    local routes_dir
    routes_dir="$(cd "$(dirname "$0")" && pwd)/routes"
    if [[ "$body" == @* ]]; then
        local filepath="${body#@}"
        if [ -f "$routes_dir/$filepath" ]; then
            cat "$routes_dir/$filepath"
        elif [ -f "$filepath" ]; then
            cat "$filepath"
        else
            echo "ERROR: Fixture not found: $filepath" >&2
            echo ""
        fi
    else
        echo "$body"
    fi
}

# ─── Substitute {{VAR}} templates ────────────────────────────────────
subst_templates() {
    local str="$1"
    while [[ "$str" =~ \{\{([A-Z_][A-Z0-9_]*)\}\} ]]; do
        local var="${BASH_REMATCH[1]}"
        local val="${!var}"
        str="${str//\{\{${var}\}\}/${val}}"
    done
    echo "$str"
}

# ─── Validate response body ─────────────────────────────────────────
validate_response() {
    local method="$1" path="$2" body="$3"
    local key="${method} ${path}"
    local checks="${RESPONSE_CHECKS[$key]}"
    [ -z "$checks" ] && return 0

    local all_ok=1
    IFS=';' read -ra parts <<< "$checks"
    for part in "${parts[@]}"; do
        local check_type="${part%%:*}"
        local check_val="${part#*:}"
        case "$check_type" in
            has_field)
                if ! echo "$body" | grep -q "\"${check_val}\""; then
                    [ "$DRY_RUN" != "1" ] && echo -e "       ${DIM}validation: missing '${check_val}'${NC}"
                    all_ok=0
                fi
                ;;
            not_empty)
                if [ -z "$body" ] || [ "$body" = "[]" ] || [ "$body" = "{}" ]; then
                    [ "$DRY_RUN" != "1" ] && echo -e "       ${DIM}validation: response empty${NC}"
                    all_ok=0
                fi
                ;;
        esac
    done
    return $((1 - all_ok))
}

# ─── XML escape helper ──────────────────────────────────────────────
xml_escape() {
    local s="$1"
    s="${s//&/&amp;}"
    s="${s//</&lt;}"
    s="${s//>/&gt;}"
    s="${s//\"/&quot;}"
    s="${s//\'/&apos;}"
    echo "$s"
}

# ─── Print module header ────────────────────────────────────────────
print_module_header() {
    local name="$1" count="$2"
    echo ""
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${CYAN}  $name ($count endpoints)${NC}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

# ─── Single endpoint test ───────────────────────────────────────────
test_endpoint() {
    local method="$1" path="$2" body="$3" expected="$4" name="$5" extra_headers="$6"

    # ROUTE_META: skip
    local meta="${ROUTE_META[$path]}"
    if [[ "$meta" == *"skip"* ]]; then
        ((TOTAL++))
        ((SKIPPED++))
        echo -e "  ${DIM}SKIP${NC} [$TOTAL] $method $path"
        return 0
    fi

    # Filter: skip if TEST_FILTER set and no match
    if [ -n "$TEST_FILTER" ]; then
        local combined="${method} ${path} ${name}"
        if ! echo "$combined" | grep -qE "$TEST_FILTER" 2>/dev/null; then
            ((SKIPPED++))
            return 0
        fi
    fi

    # Per-route timeout
    local route_timeout="$TIMEOUT"
    if [[ "$meta" =~ timeout=([0-9]+) ]]; then
        route_timeout="${BASH_REMATCH[1]}"
    fi

    # Resolve body & headers
    body=$(resolve_body "$body")
    extra_headers=$(subst_templates "$extra_headers")

    # Dry-run: just print what would run
    if [ "$DRY_RUN" = "1" ]; then
        ((TOTAL++))
        ((PASSED++))
        echo -e "  ${DIM}DRY${NC}  [$TOTAL] $method $path → $expected  ($name)"
        return 0
    fi

    local url="${BASE_URL}${path}"
    local attempt=0
    local max_attempts=$((RETRY_COUNT + 1))
    local num=""

    # Increment counter ONCE before retry loop
    ((TOTAL++))
    num="$TOTAL"

    while [ $attempt -lt $max_attempts ]; do
        local start_time end_time duration
        start_time=$(date +%s%N)

        local response status resp_body
        local curl_args=(-s -w "\n%{http_code}" -X "$method" --max-time "$route_timeout")

        # Extra headers
        if [ -n "$extra_headers" ]; then
            IFS=';' read -ra hdr_pairs <<< "$extra_headers"
            for hdr in "${hdr_pairs[@]}"; do
                hdr=$(echo "$hdr" | xargs)
                [ -n "$hdr" ] && curl_args+=(-H "$hdr")
            done
        fi

        if [ -n "$body" ] && [ "$body" != "null" ]; then
            curl_args+=(-H "Content-Type: application/json")
            curl_args+=(-d "$body")
        fi

        response=$(curl "${curl_args[@]}" "$url" 2>&1)
        end_time=$(date +%s%N)
        duration=$(( (end_time - start_time) / 1000000 ))

        # Parse response
        local line_count
        line_count=$(echo "$response" | wc -l)
        if [ "$line_count" -le 1 ]; then
            status=$(echo "$response" | tr -d '\r')
            resp_body=""
        else
            status=$(echo "$response" | tail -n 1 | tr -d '\r')
            resp_body=$(echo "$response" | head -n -1)
        fi

        # Status match
        local match=0
        IFS=',' read -ra codes <<< "$expected"
        for code in "${codes[@]}"; do
            code=$(echo "$code" | tr -d ' ')
            if [ "$status" = "$code" ]; then
                match=1
                break
            fi
        done

        if [ "$match" -eq 1 ]; then
            # Response validation
            local validation_ok=1
            if [ -n "$resp_body" ]; then
                validate_response "$method" "$path" "$resp_body" || validation_ok=0
            fi

            # Perf threshold check
            local perf_warn=""
            if [ "$PERF_THRESHOLD_MS" -gt 0 ] && [ "$duration" -gt "$PERF_THRESHOLD_MS" ]; then
                perf_warn=" ${RED}SLOW>${PERF_THRESHOLD_MS}ms${NC}"
            fi

            if [ "$validation_ok" -eq 1 ]; then
                echo -e "  ${GREEN}PASS${NC} [$num] $method $path ${YELLOW}${duration}ms${NC}${perf_warn} (HTTP $status)"
            else
                echo -e "  ${YELLOW}WARN${NC} [$num] $method $path ${YELLOW}${duration}ms${NC} (HTTP $status, validation failed)"
            fi
            ((PASSED++))

            # Collect for JUnit / JSON
            _collect_result "$method" "$path" "$name" "pass" "$status" "$duration" "$resp_body"
            return 0
        fi

        # Failed — retry with backoff
        ((attempt++))
        if [ $attempt -lt $max_attempts ]; then
            sleep $((2 ** (attempt - 1)))
        fi
    done

    # Final failure
    echo -e "  ${RED}FAIL${NC} [$num] $method $path ${YELLOW}${duration}ms${NC} (HTTP $status, expected: $expected)"
    ((FAILED++))
    if [ "$VERBOSE" = "1" ] && [ -n "$resp_body" ]; then
        local truncated=""
        [ ${#resp_body} -gt 500 ] && truncated="..."
        echo "       Response: $(echo "$resp_body" | head -c 500)${truncated}"
    fi

    _collect_result "$method" "$path" "$name" "fail" "$status" "$duration" "$resp_body"
}

# ─── Collect result for JUnit/JSON output ────────────────────────────
_collect_result() {
    local method="$1" path="$2" name="$3" result="$4"
    local status="$5" duration="$6" body="$7"

    # JUnit
    if [ -n "$JUNIT_FILE" ]; then
        local classname="$MODULE_NAME"
        local esc_name=$(xml_escape "$method $path — $name")
        if [ "$result" = "pass" ]; then
            _JUNIT_TESTS+="    <testcase classname=\"$classname\" name=\"$esc_name\" time=\"0.$duration\" />"$'\n'
        else
            _JUNIT_TESTS+="    <testcase classname=\"$classname\" name=\"$esc_name\" time=\"0.$duration\">"$'\n'
            _JUNIT_TESTS+="      <failure message=\"HTTP $status\">Expected: $expected, Got: $status</failure>"$'\n'
            _JUNIT_TESTS+="    </testcase>"$'\n'
        fi
    fi

    # JSON
    if [ -n "$JSON_FILE" ]; then
        local esc_body=$(echo "$body" | head -c 200 | sed 's/"/\\"/g' | tr '\n' ' ')
        _JSON_TESTS+="  {\"method\":\"$method\",\"path\":\"$path\",\"name\":\"$name\",\"result\":\"$result\",\"http_status\":\"$status\",\"duration_ms\":$duration},\n"
    fi
}

# ─── Smart split parser ─────────────────────────────────────────────
parse_and_run_route() {
    local entry="$1"

    local method="${entry%%|*}"; entry="${entry#*|}"
    local path="${entry%%|*}"; entry="${entry#*|}"
    local body="${entry%%|*}"; entry="${entry#*|}"
    local expected="${entry%%|*}"; entry="${entry#*|}"
    local rest="$entry"

    local name="$rest" headers=""
    if echo "$rest" | grep -q '|'; then
        name="${rest%%|*}"; rest="${rest#*|}"
        headers="$rest"
    fi

    if [ -z "$method" ] || [ -z "$path" ]; then
        echo -e "  ${RED}ERROR${NC}: Invalid route: $1" >&2
        return 1
    fi

    [ -z "$expected" ] && expected="200,201,404"
    [ -z "$name" ] && name="$method $path"

    test_endpoint "$method" "$path" "$body" "$expected" "$name" "$headers"
}

# ─── Run module tests ───────────────────────────────────────────────
run_module_tests() {
    local module_name="${MODULE_NAME:-Unknown}"
    local route_count="${#ROUTES[@]}"

    reset_counters
    RESPONSE_CHECKS=()
    ROUTE_META=()

    print_module_header "$module_name" "$route_count"

    if declare -f module_setup >/dev/null 2>&1; then
        module_setup
    fi

    for route in "${ROUTES[@]}"; do
        [[ "$route" =~ ^[[:space:]]*# ]] && continue
        [[ -z "${route// /}" ]] && continue
        parse_and_run_route "$route"
    done

    if declare -f module_custom_tests >/dev/null 2>&1; then
        module_custom_tests
    fi

    print_module_summary "$module_name"
}

# ─── Module summary ─────────────────────────────────────────────────
print_module_summary() {
    local name="$1"
    local active=$((PASSED + FAILED))
    local pass_rate=0
    [ "$active" -gt 0 ] && pass_rate=$((PASSED * 100 / active))

    echo ""
    local skip_info=""
    [ "$SKIPPED" -gt 0 ] && skip_info=" (${SKIPPED} filtered)"
    echo -e "  ${name}: ${GREEN}$PASSED${NC}/$active passed (${pass_rate}%)${skip_info}"

    if [ "$FAILED" -eq 0 ]; then
        MODULE_RESULTS+=("PASS|${name}|${PASSED}/${active}${skip_info}")
    elif [ "$pass_rate" -ge 95 ]; then
        MODULE_RESULTS+=("WARN|${name}|${PASSED}/${active} (${pass_rate}%)")
    else
        MODULE_RESULTS+=("FAIL|${name}|${PASSED}/${active} (${pass_rate}%)")
    fi
}

# ─── Final summary ──────────────────────────────────────────────────
print_final_summary() {
    echo ""
    echo "================================================================"
    echo "  FINAL SUMMARY"
    echo "================================================================"

    local _pass=0 _warn=0 _fail=0
    for result in "${MODULE_RESULTS[@]}"; do
        IFS='|' read -r status name detail <<< "$result"
        local color=""
        case "$status" in
            PASS) color="$GREEN"; ((_pass++)) ;;
            WARN) color="$YELLOW"; ((_warn++)) ;;
            FAIL) color="$RED"; ((_fail++)) ;;
        esac
        printf "  %-25s %b%s%b (%s)\n" "$name" "$color" "$status" "$NC" "$detail"
    done

    local mod_count="${#MODULE_RESULTS[@]}"
    echo ""
    echo "  Modules: $mod_count | ${GREEN}$((_pass)) pass${NC} | ${YELLOW}$((_warn)) warn${NC} | ${RED}$((_fail)) fail${NC}"
    echo "================================================================"

    # Write JUnit XML
    if [ -n "$JUNIT_FILE" ]; then
        _write_junit "$_fail"
        echo "  JUnit XML: $JUNIT_FILE"
    fi

    # Write JSON
    if [ -n "$JSON_FILE" ]; then
        _write_json
        echo "  JSON: $JSON_FILE"
    fi

    return $_fail
}

# ─── Write JUnit XML ────────────────────────────────────────────────
_write_junit() {
    local failures="$1"
    local tests="$((PASSED + FAILED))"
    {
        echo '<?xml version="1.0" encoding="UTF-8"?>'
        echo "<testsuite name=\"PaperCrawler\" tests=\"$tests\" failures=\"$failures\" time=\"0\">"
        echo -n "$_JUNIT_TESTS"
        echo "</testsuite>"
    } > "$JUNIT_FILE"
}

# ─── Write JSON results ─────────────────────────────────────────────
_write_json() {
    local tests="$((PASSED + FAILED))"
    {
        echo "{"
        echo "  \"timestamp\": \"$(date -Iseconds)\","
        echo "  \"base_url\": \"$BASE_URL\","
        echo "  \"total\": $tests,"
        echo "  \"passed\": $PASSED,"
        echo "  \"failed\": $FAILED,"
        echo "  \"modules\": ["
        local first=1
        for result in "${MODULE_RESULTS[@]}"; do
            IFS='|' read -r status name detail <<< "$result"
            [ "$first" = "1" ] && first=0 || echo ","
            echo -n "    {\"name\":\"$name\",\"status\":\"$status\",\"detail\":\"$detail\"}"
        done
        echo ""
        echo "  ],"
        echo "  \"results\": ["
        echo -ne "$_JSON_TESTS" | sed '$ s/,$//'
        echo ""
        echo "  ]"
        echo "}"
    } > "$JSON_FILE"
}

# ─── Discover route files ───────────────────────────────────────────
discover_route_files() {
    local routes_dir
    routes_dir="$(cd "$(dirname "$0")" && pwd)/routes"
    [ ! -d "$routes_dir" ] && { echo "ERROR: routes/ not found" >&2; return 1; }
    find "$routes_dir" -maxdepth 1 -name "*_routes.sh" -type f | sort
}

# ─── Run specific module ────────────────────────────────────────────
run_single_module() {
    local module_name="$1"
    local script_dir
    script_dir="$(cd "$(dirname "$0")" && pwd)"
    local route_file=""

    if [ -f "$script_dir/routes/${module_name}_routes.sh" ]; then
        route_file="$script_dir/routes/${module_name}_routes.sh"
    else
        for f in "$script_dir/routes/"*_routes.sh; do
            [ -f "$f" ] || continue
            local base
            base=$(basename "$f" _routes.sh)
            if [[ "$base" == *"${module_name}"* ]] || [[ "${module_name}" == *"$base"* ]]; then
                route_file="$f"
                break
            fi
        done
    fi

    if [ -z "$route_file" ] || [ ! -f "$route_file" ]; then
        echo "ERROR: No route file for '$module_name'" >&2
        echo "Available:"
        for f in "$script_dir/routes/"*_routes.sh; do
            [ -f "$f" ] || continue
            echo "  $(basename "$f" _routes.sh)"
        done
        return 1
    fi

    ROUTES=()
    MODULE_NAME=""
    source "$route_file"
    run_module_tests
}

# ─── Run all modules ────────────────────────────────────────────────
run_all_modules() {
    local script_dir
    script_dir="$(cd "$(dirname "$0")" && pwd)"

    echo "================================================================"
    echo "  PaperCrawler API Test Suite"
    echo "  $(date '+%Y-%m-%d %H:%M:%S')"
    echo "  Base URL: $BASE_URL | Retry: $RETRY_COUNT | Timeout: ${TIMEOUT}s"
    [ "$DRY_RUN" = "1" ] && echo "  Mode: DRY-RUN"
    [ -n "$TEST_FILTER" ] && echo "  Filter: $TEST_FILTER"
    [ "$PERF_THRESHOLD_MS" -gt 0 ] && echo "  Perf threshold: ${PERF_THRESHOLD_MS}ms"
    echo "================================================================"

    if [ "$DRY_RUN" != "1" ]; then
        if ! curl -s --max-time 3 "$BASE_URL/api/health" > /dev/null 2>&1; then
            echo ""
            echo -e "${YELLOW}WARNING: Server at $BASE_URL not responding.${NC}"
            echo -e "${YELLOW}Tests may fail. Ensure server is running.${NC}"
            echo ""
        fi
    fi

    for route_file in $(discover_route_files); do
        ROUTES=()
        MODULE_NAME=""
        source "$route_file"
        run_module_tests
    done

    print_final_summary
    return $?
}
