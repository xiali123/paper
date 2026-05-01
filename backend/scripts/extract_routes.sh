#!/bin/bash
# extract_routes.sh - 从C++源码提取路由清单
# 输出格式: MODULE_NAME METHOD PATH (每行一条)
#
# 用法:
#   bash scripts/extract_routes.sh              # 提取所有路由
#   bash scripts/extract_routes.sh --module auth # 仅提取指定模块
#   bash scripts/extract_routes.sh --json        # JSON输出
#   bash scripts/extract_routes.sh --help

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUSINESS_DIR="$SRC_DIR/src/business"
CORE_FILE="$SRC_DIR/src/core/main_refactored.cpp"

GREEN='\033[0;32m'
NC='\033[0m'

# ─── 模块文件 → 模块名 映射 ───────────────────────────────────────
MODULE_ALIAS=(
    "StatsApiModule|StatsApi"
    "AuthApiModule|AuthApi"
    "AdminApiModule|AdminApi"
    "LatexApiModule|LatexApi"
    "PaperApiModule|PaperApi"
    "UserApiModule|UserApi"
    "SearchApiModule|SearchApi"
    "ExportApiModule|ExportApi"
    "CrawlerApiModule|CrawlerApi"
    "AiApiModule|AiApi"
    "RecommendationApiModule|RecommendationApi"
    "AiCoPilotModule|AiCoPilot"
    "AiCoPilotModuleRoutes|AiCoPilot"
    "CollaborativeWritingModule|CollaborativeWriting"
    "AnalyticsIntelligenceModule|AnalyticsIntelligence"
)

# 已知前缀映射（权威来源，覆盖自动推导）
declare -A KNOWN_PREFIX=(
    [StatsApi]="/api/stats"
    [AuthApi]="/api/auth"
    [AdminApi]="/api/admin"
    [LatexApi]="/api/latex"
    [PaperApi]="/api/papers"
    [UserApi]="/api/users"
    [SearchApi]="/api/search"
    [ExportApi]="/api/export"
    [CrawlerApi]="/api/crawler"
    [AiApi]="/api/ai"
    [RecommendationApi]="/api/recommendations"
    [AiCoPilot]="/api/ai-co-pilot"
    [CollaborativeWriting]="/api/collaborative"
    [AnalyticsIntelligence]="/api/analytics"
    [Core]="/api"
)

# ─── 解析参数 ─────────────────────────────────────────────────────
MODULE_FILTER=""
OUTPUT_FORMAT="text"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --module|-m) MODULE_FILTER="$2"; shift 2 ;;
        --json|-j)   OUTPUT_FORMAT="json"; shift ;;
        --help|-h)
            echo "用法: $0 [选项]"
            echo ""
            echo "  --module, -m <name>  仅提取指定模块"
            echo "  --json, -j           JSON格式输出"
            echo "  --help, -h           显示帮助"
            exit 0 ;;
        *) echo "未知参数: $1"; exit 1 ;;
    esac
done

# ─── 获取模块简称 ─────────────────────────────────────────────────
get_module_alias() {
    local basename="$1"
    for entry in "${MODULE_ALIAS[@]}"; do
        local file_part="${entry%%|*}"
        local alias_part="${entry#*|}"
        if [ "$basename" = "$file_part" ]; then
            echo "$alias_part"
            return
        fi
    done
    echo "${basename%Module}"
}

# ─── 获取模块前缀 ─────────────────────────────────────────────────
get_prefix() {
    local file="$1"
    local basename="$2"
    local alias
    alias=$(get_module_alias "$basename")

    # 优先: 已知前缀映射表
    local known="${KNOWN_PREFIX[$alias]}"
    [ -n "$known" ] && { echo "$known"; return; }

    # 回退: .cpp中直接赋值 prefix = "/api/xxx"
    local direct
    direct=$(grep -E 'prefix\s*=\s*"/api/' "$file" | head -1 | grep -o '"/api/[^"]*"' | tr -d '"')
    [ -n "$direct" ] && { echo "$direct"; return; }

    # 从模块名推导
    local lower
    lower=$(echo "$alias" | tr '[:upper:]' '[:lower:]')
    lower="${lower//module/}"
    lower="${lower//api/}"
    echo "/api/$lower"
}

# ─── 全局去重 ─────────────────────────────────────────────────────
declare -a ALL_ROUTES=()

extract_from_file() {
    local file="$1"
    local module_name="$2"
    local prefix="$3"

    # 扫描别名变量: listPath = prefix, templatesPath = prefix + "/xxx"
    declare -A ALIAS_VARS=()
    while IFS= read -r vline; do
        # listPath = prefix;
        if [[ "$vline" =~ ([a-zA-Z_][a-zA-Z0-9_]*)[[:space:]]*=[[:space:]]*prefix[[:space:]]*\; ]]; then
            ALIAS_VARS["${BASH_REMATCH[1]}"]="$prefix"
        fi
        # xxxPath = prefix + "/sub";
        if [[ "$vline" =~ ([a-zA-Z_][a-zA-Z0-9_]*)[[:space:]]*=[[:space:]]*prefix[[:space:]]*\+[[:space:]]*\"([^\"]*)\"\; ]]; then
            ALIAS_VARS["${BASH_REMATCH[1]}"]="${prefix}${BASH_REMATCH[2]}"
        fi
    done < <(grep -E '[a-zA-Z_]+Path\s*=\s*prefix' "$file" 2>/dev/null)

    while IFS= read -r line; do
        local method
        method=$(echo "$line" | grep -oE 'router\.(get|post|put|del|patch)\(' | sed 's/router\.//;s/(//')
        [ -z "$method" ] && continue
        [ "$method" = "del" ] && method="DELETE"

        local path=""

        # prefix + "/subpath"
        if echo "$line" | grep -q 'prefix *+'; then
            local subpath
            subpath=$(echo "$line" | grep -oE 'prefix *\+ *"[^"]*"' | grep -o '"[^"]*"$' | tr -d '"')
            [ -n "$subpath" ] && path="${prefix}${subpath}"
        fi

        # router.get(prefix, ...) — 根路由
        if [ -z "$path" ] && echo "$line" | grep -qE "router\.\w+\(prefix[,)]"; then
            path="$prefix"
        fi

        # 别名变量: router.get(listPath, ...) 或 router.post(templatesPath, ...)
        if [ -z "$path" ]; then
            for var_name in "${!ALIAS_VARS[@]}"; do
                if echo "$line" | grep -qE "router\.\w+\(${var_name}[,)]"; then
                    path="${ALIAS_VARS[$var_name]}"
                    break
                fi
            done
        fi

        # 直接路径 "/api/xxx"（无prefix变量）
        if [ -z "$path" ]; then
            path=$(echo "$line" | grep -oE '"/api/[^"]*"' | head -1 | tr -d '"')
        fi

        [ -z "$path" ] && continue

        # 去重
        local key="${module_name}|${method}|${path}"
        if ! echo "${ALL_ROUTES[@]}" | grep -qF "$key" 2>/dev/null; then
            ALL_ROUTES+=("$key")
            echo "${module_name} ${method} ${path}"
        fi
    done < <(grep -E 'router\.(get|post|put|del|patch)\(' "$file" 2>/dev/null)
}

# ─── 主流程 ───────────────────────────────────────────────────────
main() {
    # 扫描 src/business/*.cpp
    for file in "$BUSINESS_DIR"/*Module.cpp "$BUSINESS_DIR"/*ModuleRoutes.cpp; do
        [ -f "$file" ] || continue

        fbname=$(basename "$file" .cpp)
        grep -q 'registerRoutes\|router\.\(get\|post\|put\|del\|patch\)(' "$file" 2>/dev/null || continue

        falias=$(get_module_alias "$fbname")
        fprefix=$(get_prefix "$file" "$fbname")

        [ -n "$MODULE_FILTER" ] && ! echo "$falias" | grep -qi "$MODULE_FILTER" && continue

        extract_from_file "$file" "$falias" "$fprefix"
    done

    # 扫描 src/core/main_refactored.cpp
    if [ -f "$CORE_FILE" ] && { [ -z "$MODULE_FILTER" ] || echo "core" | grep -qi "$MODULE_FILTER"; }; then
        extract_from_file "$CORE_FILE" "Core" "/api"
    fi

    echo "" >&2
    echo -e "${GREEN}提取完成: ${#ALL_ROUTES[@]} 条路由${NC}" >&2
}

main
