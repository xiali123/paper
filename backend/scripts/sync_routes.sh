#!/bin/bash
# sync_routes.sh - 对比源码路由与测试路由，自动同步
#
# 用法:
#   bash scripts/sync_routes.sh                    # 同步（实际修改）
#   bash scripts/sync_routes.sh --dry-run          # 仅显示差异，不修改
#   bash scripts/sync_routes.sh --module admin     # 仅同步指定模块
#   bash scripts/sync_routes.sh --help

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TESTS_DIR="$(cd "$SCRIPT_DIR/../tests" && pwd)"
ROUTES_DIR="$TESTS_DIR/routes"
EXTRACT_SCRIPT="$SCRIPT_DIR/extract_routes.sh"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
DIM='\033[2m'
NC='\033[0m'

# ─── 参数 ─────────────────────────────────────────────────────────
DRY_RUN=0
MODULE_FILTER=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run|-d) DRY_RUN=1; shift ;;
        --module|-m)  MODULE_FILTER="$2"; shift 2 ;;
        --help|-h)
            echo "用法: $0 [选项]"
            echo ""
            echo "  --dry-run, -d       仅显示差异，不修改文件"
            echo "  --module, -m <name>  仅同步指定模块"
            echo "  --help, -h          显示帮助"
            exit 0 ;;
        *) echo "未知参数: $1"; exit 1 ;;
    esac
done

# ─── 模块名 → 路由文件 映射 ───────────────────────────────────────
declare -A MODULE_ROUTE_FILE=(
    [StatsApi]="stats_routes.sh"
    [AuthApi]="auth_routes.sh"
    [AdminApi]="admin_routes.sh"
    [LatexApi]="latex_routes.sh"
    [PaperApi]="paper_routes.sh"
    [UserApi]="user_routes.sh"
    [SearchApi]="search_routes.sh"
    [ExportApi]="export_routes.sh"
    [CrawlerApi]="crawler_routes.sh"
    [AiApi]="ai_routes.sh"
    [RecommendationApi]="recommendation_routes.sh"
    [AiCoPilot]="aicopilot_routes.sh"
    [CollaborativeWriting]="collaborative_routes.sh"
    [AnalyticsIntelligence]="analytics_routes.sh"
    [Core]="core_routes.sh"
)

# 模块中文描述
declare -A MODULE_DESC=(
    [StatsApi]="统计分析"
    [AuthApi]="认证授权"
    [AdminApi]="管理后台"
    [LatexApi]="LaTeX编辑器"
    [PaperApi]="论文管理"
    [UserApi]="用户管理"
    [SearchApi]="搜索过滤"
    [ExportApi]="导出下载"
    [CrawlerApi]="爬虫系统"
    [AiApi]="AI功能"
    [RecommendationApi]="推荐引擎"
    [AiCoPilot]="AI副驾驶"
    [CollaborativeWriting]="协同写作"
    [AnalyticsIntelligence]="分析智能"
    [Core]="系统核心"
)

# 测试文件 MODULE_NAME → 源码模块名 映射
declare -A TEST_NAME_TO_MODULE=(
    [StatsApi]="StatsApi"
    [AuthApi]="AuthApi"
    [AdminApi]="AdminApi"
    [LatexApi]="LatexApi"
    [PaperApi]="PaperApi"
    [UserApi]="UserApi"
    [SearchApi]="SearchApi"
    [ExportApi]="ExportApi"
    [CrawlerApi]="CrawlerApi"
    [AiApi]="AiApi"
    [RecommendationApi]="RecommendationApi"
    [AiCoPilot]="AiCoPilot"
    [CollaborativeWriting]="CollaborativeWriting"
    [AnalyticsIntelligence]="AnalyticsIntelligence"
    ["Core (System)"]="Core"
)

# ─── 从源码提取路由 ───────────────────────────────────────────────
echo "================================================================"
echo "  路由同步工具 — 源码 vs 测试"
echo "================================================================"
echo ""

if [ ! -f "$EXTRACT_SCRIPT" ]; then
    echo -e "${RED}错误: extract_routes.sh 不存在${NC}"
    exit 1
fi

echo -e "${CYAN}[1/3] 从源码提取路由...${NC}"
extract_args=""
[ -n "$MODULE_FILTER" ] && extract_args="--module $MODULE_FILTER"
SOURCE_ROUTES=$(bash "$EXTRACT_SCRIPT" $extract_args 2>/dev/null)
SOURCE_COUNT=$(echo "$SOURCE_ROUTES" | grep -c '.' )

echo "  源码路由: ${SOURCE_COUNT} 条"

# 按模块分组源码路由
declare -A SRC_BY_MODULE=()
while IFS= read -r line; do
    [ -z "$line" ] && continue
    mod="${line%% *}"
    rest="${line#* }"
    SRC_BY_MODULE["$mod"]+="${rest}"$'\n'
done <<< "$SOURCE_ROUTES"

# ─── 从测试文件提取路由 ──────────────────────────────────────────
echo -e "${CYAN}[2/3] 扫描测试路由文件...${NC}"

declare -A TEST_BY_MODULE=()

for route_file in "$ROUTES_DIR"/*_routes.sh; do
    [ -f "$route_file" ] || continue
    raw_name=$(grep '^MODULE_NAME=' "$route_file" | head -1 | cut -d'"' -f2)
    [ -z "$raw_name" ] && continue

    # 映射到源码模块名（处理 Core (System) → Core 等）
    local_name="${TEST_NAME_TO_MODULE[$raw_name]:-$raw_name}"

    # 跳过过滤不匹配的模块
    if [ -n "$MODULE_FILTER" ] && ! echo "$local_name" | grep -qi "$MODULE_FILTER"; then
        continue
    fi

    # 提取 METHOD|PATH 行
    while IFS= read -r route_line; do
        [[ "$route_line" =~ ^[[:space:]]*# ]] && continue
        [[ -z "${route_line// /}" ]] && continue

        # 去掉前后引号
        route_line=$(echo "$route_line" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//' | sed 's/^"//;s/"$//')
        [ -z "$route_line" ] && continue
        [[ "$route_line" =~ ^(GET|POST|PUT|DELETE|PATCH|OPTIONS)\| ]] || continue

        method=$(echo "$route_line" | cut -d'|' -f1)
        path=$(echo "$route_line" | cut -d'|' -f2)
        # 剥离 query params: /api/search?query=test → /api/search
        path=$(echo "$path" | cut -d'?' -f1)
        [ -z "$method" ] || [ -z "$path" ] && continue

        TEST_BY_MODULE["$local_name"]+="${method} ${path}"$'\n'
    done < <(grep -E '^\s*"' "$route_file" 2>/dev/null)
done

# ─── 对比+同步 ────────────────────────────────────────────────────
echo ""
echo -e "${CYAN}[3/3] 对比差异...${NC}"
echo ""

total_new=0
total_stale=0
total_match=0
total_modules=0
updated_files=()

# 路径参数归一化: :id, :name 等替换为测试中使用的固定值
normalize_path() {
    local p="$1"
    p=$(echo "$p" | sed 's/:[a-zA-Z_][a-zA-Z0-9_]*(/1(/g; s/:[a-zA-Z_][a-zA-Z0-9_]*$/1/g; s/:[a-zA-Z_][a-zA-Z0-9_]*\//1\//g')
    echo "$p"
}

# 路径参数归一化: 反向，将固定值转为模式匹配
denormalize_for_match() {
    local p="$1"
    # 将 :xxx 参数位置替换为通配
    echo "$p"
}

for module in $(echo "${!SRC_BY_MODULE[@]}" | tr ' ' '\n' | sort); do
    total_modules=$((total_modules + 1))

    src_routes="${SRC_BY_MODULE[$module]}"
    test_routes="${TEST_BY_MODULE[$module]}"

    route_file="${MODULE_ROUTE_FILE[$module]}"
    [ -z "$route_file" ] && continue

    file_path="$ROUTES_DIR/$route_file"

    # 收集新增路由（源码有，测试没有）
    new_routes=()
    while IFS= read -r src_line; do
        [ -z "$src_line" ] && continue
        src_method=$(echo "$src_line" | awk '{print $1}' | tr '[:lower:]' '[:upper:]')
        src_path=$(echo "$src_line" | awk '{print $2}')

        # 检查测试中是否存在（考虑路径参数）
        found=0
        while IFS= read -r test_line; do
            [ -z "$test_line" ] && continue
            test_method=$(echo "$test_line" | awk '{print $1}' | tr '[:lower:]' '[:upper:]')
            test_path=$(echo "$test_line" | awk '{print $2}')

            [ "$src_method" != "$test_method" ] && continue

            # 精确匹配
            if [ "$src_path" = "$test_path" ]; then
                found=1
                break
            fi

            # 路径参数匹配: /api/xxx/:id 匹配 /api/xxx/1
            src_pattern=$(echo "$src_path" | sed 's|:[a-zA-Z_][a-zA-Z0-9_]*|[^/]*|g')
            if echo "$test_path" | grep -qE "^${src_pattern}$"; then
                found=1
                break
            fi

            # 子路径匹配: 源码 /saved 匹配测试 /saved/1（测试用了参数值，源码无参数）
            if [[ "$test_path" == "${src_path}/"* ]]; then
                found=1
                break
            fi
            # 反向: 测试 /saved 匹配源码 /saved/:id（测试未用参数值）
            if [[ "$src_path" == "${test_path}/"* ]]; then
                found=1
                break
            fi
        done <<< "$test_routes"

        if [ "$found" -eq 0 ]; then
            new_routes+=("$src_method $src_path")
        fi
    done <<< "$src_routes"

    # 收集过期路由（测试有，源码没有）— 仅报告，不自动删除
    stale_routes=()
    while IFS= read -r test_line; do
        [ -z "$test_line" ] && continue
        test_method=$(echo "$test_line" | awk '{print $1}' | tr '[:lower:]' '[:upper:]')
        test_path=$(echo "$test_line" | awk '{print $2}')

        found=0
        while IFS= read -r src_line; do
            [ -z "$src_line" ] && continue
            src_method=$(echo "$src_line" | awk '{print $1}' | tr '[:lower:]' '[:upper:]')
            src_path=$(echo "$src_line" | awk '{print $2}')

            [ "$src_method" != "$test_method" ] && continue

            if [ "$src_path" = "$test_path" ]; then
                found=1
                break
            fi

            src_pattern=$(echo "$src_path" | sed 's|:[a-zA-Z_][a-zA-Z0-9_]*|[^/]*|g')
            if echo "$test_path" | grep -qE "^${src_pattern}$"; then
                found=1
                break
            fi

            if [[ "$test_path" == "${src_path}/"* ]]; then
                found=1
                break
            fi
            if [[ "$src_path" == "${test_path}/"* ]]; then
                found=1
                break
            fi
        done <<< "$src_routes"

        if [ "$found" -eq 0 ]; then
            stale_routes+=("$test_method $test_path")
        fi
    done <<< "$test_routes"

    # 计算匹配数
    src_count=$(echo "$src_routes" | grep -c '.')
    match_count=$((src_count - ${#new_routes[@]}))

    # 打印模块报告
    mod_desc="${MODULE_DESC[$module]:-$module}"
    echo -e "  ${CYAN}${module}${NC} (${mod_desc})"
    echo -e "    源码: ${src_count} | 测试: $(echo "$test_routes" | grep -c '.') | 匹配: ${match_count}"

    if [ ${#new_routes[@]} -gt 0 ]; then
        echo -e "    ${GREEN}新增 (+${#new_routes[@]}):${NC}"
        for r in "${new_routes[@]}"; do
            echo -e "      ${GREEN}+ $r${NC}"
        done
        total_new=$((total_new + ${#new_routes[@]}))
    fi

    if [ ${#stale_routes[@]} -gt 0 ]; then
        echo -e "    ${YELLOW}过期 (-${#stale_routes[@]}):${NC}"
        for r in "${stale_routes[@]}"; do
            echo -e "      ${YELLOW}- $r${NC}"
        done
        total_stale=$((total_stale + ${#stale_routes[@]}))
    fi

    if [ ${#new_routes[@]} -eq 0 ] && [ ${#stale_routes[@]} -eq 0 ]; then
        echo -e "    ${GREEN}完全同步 ✓${NC}"
    fi

    total_match=$((total_match + match_count))
    echo ""

    # ─── 自动同步：追加新路由 ──────────────────────────────────────
    if [ ${#new_routes[@]} -eq 0 ]; then
        continue
    fi

    if [ "$DRY_RUN" = "1" ]; then
        echo -e "    ${DIM}(dry-run: 不修改文件)${NC}"
        echo ""
        continue
    fi

    # 路径参数替换为测试固定值
    test_path_for() {
        local p="$1"
        p=$(echo "$p" | sed 's/:id/1/g; s/:name/test_name/g; s/:userId/1/g; s/:paperId/1/g; s/:tag/test_tag/g; s/:date/2025-01-01/g; s/:sid/1/g; s/:roleid/1/g; s/:type/pdf/g')
        echo "$p"
    }

    # 生成路由条目
    new_entries=""
    for r in "${new_routes[@]}"; do
        r_method=$(echo "$r" | awk '{print $1}')
        r_path=$(echo "$r" | awk '{print $2}')
        test_p=$(test_path_for "$r_path")

        # 构造 body
        body=""
        case "$r_method" in
            POST|PUT)
                case "$r_path" in
                    */login*) body='{"username":"testuser","password":"Pass1234"}' ;;
                    */register*) body='{"username":"testuser","password":"Pass1234","email":"test@test.com"}' ;;
                    */chat*) body='{"message":"hello"}' ;;
                    */summarize*|*/keywords*|*/contributions*|*/compare*) body='{"paperId":1}' ;;
                    */review*) body='{"paperId":1}' ;;
                    */generate*) body='{"topic":"machine learning"}' ;;
                    */favorite*|*/read*) body='{"paperId":1}' ;;
                    */tags) body='{"tags":["test"]}' ;;
                    */feedback*) body='{"paperId":1,"rating":5}' ;;
                    */save*) body='{"query":"test"}' ;;
                    */send*) body='{"title":"test","content":"test","recipients":[]}' ;;
                    */trigger*) body='' ;;
                    */enable|*/disable|*/activate|*/deactivate|*/suspend|*/unlock|*/reload|*/clear) body='' ;;
                    */check) body='{"text":"test"}' ;;
                    */compile) body='{}' ;;
                    */upload*) body='' ;;
                    */import) body='{}' ;;
                    */merge|*/branch|*/restore) body='{}' ;;
                    */export*) body='{"paperIds":[1]}' ;;
                    */lock*|*/password*|*/change-password*|*/reset-password*) body='{"password":"NewPass1234"}' ;;
                    *) body='{}' ;;
                esac ;;
        esac

        # 构建 name
        short_path=$(echo "$r_path" | sed 's|/api/[^/]*/||')
        [ -z "$short_path" ] && short_path="(root)"
        entry_name="Auto: ${r_method} ${short_path}"

        new_entries+="    \"${r_method}|${test_p}|${body}|200,201,404|${entry_name}\""$'\n'
    done

    if [ ! -f "$file_path" ]; then
        # 创建新模块路由文件
        mod_desc="${MODULE_DESC[$module]:-$module}"
        {
            echo '#!/bin/bash'
            echo "# Route definitions for ${module} module (${mod_desc})"
            echo "# Add new routes: append to ROUTES array"
            echo '# Format: "METHOD|/path|body_json|expected_codes|test_name"'
            echo ''
            echo "MODULE_NAME=\"${module}\""
            echo 'ROUTES=('
            echo -n "$new_entries"
            echo ')'
        } > "$file_path"
        echo -e "    ${GREEN}创建: $route_file${NC}"
        updated_files+=("$route_file")
    else
        # 追加到现有文件
        # 在最后的 ) 前插入
        if grep -q '^)' "$file_path"; then
            # 移除末尾的 )
            sed -i '/^)/d' "$file_path"
            # 追加新路由
            echo "" >> "$file_path"
            echo "    # Auto-synced $(date '+%Y-%m-%d')" >> "$file_path"
            echo -n "$new_entries" >> "$file_path"
            echo ")" >> "$file_path"
        else
            # 文件格式不标准，直接追加到ROUTES数组
            echo "" >> "$file_path"
            echo "# Auto-synced $(date '+%Y-%m-%d')" >> "$file_path"
            while IFS= read -r entry; do
                [ -z "$entry" ] && continue
                echo "ROUTES+=(" >> "$file_path"
                echo "    $entry" >> "$file_path"
                echo ")" >> "$file_path"
            done <<< "$new_entries"
        fi
        echo -e "    ${GREEN}更新: $route_file (+${#new_routes[@]} 路由)${NC}"
        updated_files+=("$route_file")
    fi
    echo ""
done

# ─── 总结 ─────────────────────────────────────────────────────────
echo "================================================================"
echo "  同步报告"
echo "================================================================"
echo "  模块: ${total_modules}"
echo "  匹配: ${total_match}"
echo -e "  ${GREEN}新增: ${total_new}${NC}"
echo -e "  ${YELLOW}过期: ${total_stale}${NC}"

if [ ${#updated_files[@]} -gt 0 ]; then
    echo ""
    echo "  已更新文件:"
    for f in "${updated_files[@]}"; do
        echo -e "    ${GREEN}$f${NC}"
    done
fi

if [ "$DRY_RUN" = "1" ]; then
    echo ""
    echo -e "  ${DIM}(dry-run 模式: 未修改任何文件)${NC}"
fi

echo "================================================================"

# 返回状态
if [ "$total_new" -gt 0 ] && [ "$DRY_RUN" != "1" ]; then
    echo ""
    echo "  提示: 新增路由使用默认 expected=200,201,404"
    echo "  可手动调整 tests/routes/ 中对应文件"
fi
