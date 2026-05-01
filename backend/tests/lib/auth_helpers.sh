#!/bin/bash
# Auth helpers for API test framework
# Usage:
#   source lib/test_framework.sh
#   source lib/auth_helpers.sh
#   TOKEN=$(get_auth_token)
#   # Then in route definitions: "GET|/api/auth/me||200|Get me|Authorization: Bearer {{TOKEN}}"

# ─── Acquire auth token via login ────────────────────────────────────
# Args: [username] [password]
# Sets TOKEN env var for {{TOKEN}} substitution in route headers
get_auth_token() {
    local username="${1:-testuser}"
    local password="${2:-Pass1234}"

    local resp
    resp=$(curl -s -X POST "${BASE_URL}/api/auth/login" \
        -H "Content-Type: application/json" \
        -d "{\"username\":\"${username}\",\"password\":\"${password}\"}" \
        --max-time 5 2>&1)

    # Try extracting token from common JSON patterns
    local token=""
    # Pattern 1: "token":"xxx"
    token=$(echo "$resp" | grep -o '"token":"[^"]*"' | head -1 | cut -d'"' -f4)
    # Pattern 2: "access_token":"xxx"
    [ -z "$token" ] && token=$(echo "$resp" | grep -o '"access_token":"[^"]*"' | head -1 | cut -d'"' -f4)
    # Pattern 3: nested "data":{"token":"xxx"}
    [ -z "$token" ] && token=$(echo "$resp" | grep -o '"data"[[:space:]]*:[[:space:]]*{[^}]*"token":"[^"]*"' | grep -o '"token":"[^"]*"' | head -1 | cut -d'"' -f4)

    if [ -n "$token" ]; then
        export TOKEN="$token"
        echo "$token"
    else
        echo "" >&2
    fi
}

# ─── Register a test user then login ────────────────────────────────
# Args: [username] [password] [email]
# Returns token string
register_and_login() {
    local username="${1:-testuser}"
    local password="${2:-Pass1234}"
    local email="${3:-test@test.com}"

    # Try register (ignore errors — user may already exist)
    curl -s -X POST "${BASE_URL}/api/auth/register" \
        -H "Content-Type: application/json" \
        -d "{\"username\":\"${username}\",\"password\":\"${password}\",\"email\":\"${email}\"}" \
        --max-time 5 > /dev/null 2>&1

    # Login
    get_auth_token "$username" "$password"
}

# ─── Convenience: set TOKEN for all subsequent tests ─────────────────
setup_auth() {
    local username="${1:-testuser}"
    local password="${2:-Pass1234}"

    TOKEN=$(register_and_login "$username" "$password")
    export TOKEN

    if [ -z "$TOKEN" ]; then
        echo -e "  ${YELLOW}AUTH: Could not acquire token. Authenticated routes may fail.${NC}"
    fi
}
