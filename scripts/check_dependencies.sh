#!/bin/bash
# PaperCrawler 依赖关系检查脚本
# 用于验证模块依赖是否符合架构规范

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BACKEND_INCLUDE="$PROJECT_ROOT/backend/include"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 错误计数
ERRORS=0
WARNINGS=0

echo "================================================"
echo "PaperCrawler 依赖关系检查工具"
echo "================================================"
echo ""

# 函数：打印错误
print_error() {
    echo -e "${RED}❌ ERROR: $1${NC}"
    ((ERRORS++))
}

# 函数：打印警告
print_warning() {
    echo -e "${YELLOW}⚠️  WARNING: $1${NC}"
    ((WARNINGS++))
}

# 函数：打印成功
print_success() {
    echo -e "${GREEN}✅ PASS: $1${NC}"
}

echo "================================================"
echo "1. 检查业务层依赖规范"
echo "================================================"

# 检查：业务层不应直接包含features/头文件
echo "检查: business/ 不应直接依赖 features/ ..."
BUSINESS_TO_FEATURES=$(grep -r "features/" "$BACKEND_INCLUDE/business/" 2>/dev/null | grep -v "interfaces/" || true)
if [ -n "$BUSINESS_TO_FEATURES" ]; then
    print_error "业务层直接依赖中间件层具体实现"
    echo "$BUSINESS_TO_FEATURES"
    echo ""
else
    print_success "业务层不依赖中间件层"
fi

# 检查：业务层不应直接包含其他业务模块头文件
echo "检查: business/ 模块间不应直接依赖 ..."
BUSINESS_TO_BUSINESS=$(grep -r "#include.*business/" "$BACKEND_INCLUDE/business/" 2>/dev/null | grep -v "ModuleBase" | grep -v "ModuleExports" || true)
if [ -n "$BUSINESS_TO_BUSINESS" ]; then
    print_warning "业务模块之间存在直接依赖"
    echo "$BUSINESS_TO_BUSINESS"
    echo ""
else
    print_success "业务模块间无直接依赖"
fi

# 检查：业务层应通过interfaces/依赖其他模块
echo "检查: business/ 应使用 interfaces/ ..."
INTERFACE_USAGE=$(find "$BACKEND_INCLUDE/interfaces" -name "*.hpp" 2>/dev/null | wc -l)
if [ "$INTERFACE_USAGE" -eq 0 ]; then
    print_warning "未找到interfaces/目录，建议创建抽象接口层"
else
    print_success "发现interfaces/目录（$INTERFACE_USAGE个接口文件）"
fi

echo ""
echo "================================================"
echo "2. 检查中间件层依赖规范"
echo "================================================"

# 检查：中间件层不应依赖业务层
echo "检查: features/ 不应依赖 business/ ..."
FEATURES_TO_BUSINESS=$(grep -r "business/" "$BACKEND_INCLUDE/features/" 2>/dev/null || true)
if [ -n "$FEATURES_TO_BUSINESS" ]; then
    print_error "中间件层依赖业务层（违反分层原则）"
    echo "$FEATURES_TO_BUSINESS"
    echo ""
else
    print_success "中间件层不依赖业务层"
fi

# 检查：中间件层不应依赖modules/具体实现
echo "检查: features/ 不应依赖 modules/ ..."
FEATURES_TO_MODULES=$(grep -r "modules/" "$BACKEND_INCLUDE/features/" 2>/dev/null || true)
if [ -n "$FEATURES_TO_MODULES" ]; then
    print_warning "中间件层依赖模块层具体实现"
    echo "$FEATURES_TO_MODULES"
    echo ""
else
    print_success "中间件层不依赖模块层"
fi

echo ""
echo "================================================"
echo "3. 检查数据访问层依赖规范"
echo "================================================"

# 检查：data/不应依赖business/
echo "检查: data/ 不应依赖 business/ ..."
DATA_TO_BUSINESS=$(grep -r "business/" "$BACKEND_INCLUDE/data/" 2>/dev/null || true)
if [ -n "$DATA_TO_BUSINESS" ]; then
    print_error "数据访问层依赖业务层（严重违反分层原则）"
    echo "$DATA_TO_BUSINESS"
    echo ""
else
    print_success "数据访问层不依赖业务层"
fi

# 检查：MySqlConnection不应依赖DatabaseModule
echo "检查: MySqlConnection 不应依赖 DatabaseModule ..."
if [ -f "$BACKEND_INCLUDE/data/MySqlConnection.hpp" ]; then
    if grep -q "DatabaseModule" "$BACKEND_INCLUDE/data/MySqlConnection.hpp"; then
        print_warning "MySqlConnection依赖DatabaseModule（建议使用IConnection接口）"
    else
        print_success "MySqlConnection不依赖DatabaseModule"
    fi
fi

# 检查：RedisConnection不应依赖DatabaseModule
echo "检查: RedisConnection 不应依赖 DatabaseModule ..."
if [ -f "$BACKEND_INCLUDE/data/RedisConnection.hpp" ]; then
    if grep -q "DatabaseModule" "$BACKEND_INCLUDE/data/RedisConnection.hpp"; then
        print_error "RedisConnection依赖DatabaseModule（语义错误，Redis不是数据库模块）"
    else
        print_success "RedisConnection不依赖DatabaseModule"
    fi
fi

echo ""
echo "================================================"
echo "4. 检查核心层依赖规范"
echo "================================================"

# 检查：core/应保持零依赖或最小依赖
echo "检查: core/ 依赖数量 ..."
CORE_DEPENDS=$(find "$BACKEND_INCLUDE/core" -name "*.hpp" -exec grep -h "^#include" {} \; 2>/dev/null | sort -u | wc -l)
if [ "$CORE_DEPENDS" -gt 20 ]; then
    print_warning "core/层依赖较多（$CORE_DEPENDS个），建议简化"
else
    print_success "core/层依赖合理（$CORE_DEPENDS个）"
fi

echo ""
echo "================================================"
echo "5. 检查循环依赖"
echo "================================================"

# 检查：ServiceLayer循环依赖风险
echo "检查: ServiceLayer 循环依赖风险 ..."
if [ -f "$BACKEND_INCLUDE/business/ServiceLayer.hpp" ]; then
    SERVICE_LAYER_DEPS=$(grep "#include" "$BACKEND_INCLUDE/business/ServiceLayer.hpp" | grep "business/" | wc -l)
    if [ "$SERVICE_LAYER_DEPS" -gt 3 ]; then
        print_warning "ServiceLayer依赖过多业务模块（$SERVICE_LAYER_DEPS个），存在循环依赖风险"
    else
        print_success "ServiceLayer依赖合理"
    fi
fi

# 检查：ResponseHandlerModule依赖ResponseQueueModule
echo "检查: ResponseHandlerModule 循环依赖 ..."
if [ -f "$BACKEND_INCLUDE/features/operations/ResponseHandlerModule.hpp" ]; then
    if grep -q "ResponseQueueModule" "$BACKEND_INCLUDE/features/operations/ResponseHandlerModule.hpp"; then
        print_warning "ResponseHandlerModule依赖ResponseQueueModule（建议引入中间抽象）"
    else
        print_success "ResponseHandlerModule无循环依赖"
    fi
fi

echo ""
echo "================================================"
echo "6. 检查模块接口使用"
echo "================================================"

# 检查：是否使用了IDatabase接口
echo "检查: 业务模块使用 IDatabase 接口 ..."
BUSINESS_MODULES=$(find "$BACKEND_INCLUDE/business" -name "*ApiModule.hpp" 2>/dev/null)
USING_IDATABASE=0
TOTAL_MODULES=0

for module in $BUSINESS_MODULES; do
    ((TOTAL_MODULES++))
    if grep -q "IDatabase" "$module"; then
        ((USING_IDATABASE++))
    fi
done

if [ $TOTAL_MODULES -gt 0 ]; then
    RATIO=$((USING_IDATABASE * 100 / TOTAL_MODULES))
    echo "业务模块使用IDatabase接口: $USING_IDATABASE/$TOTAL_MODULES ($RATIO%)"
    if [ $RATIO -lt 50 ]; then
        print_warning "超过一半的业务模块未使用IDatabase接口"
    else
        print_success "大部分业务模块使用IDatabase接口"
    fi
fi

echo ""
echo "================================================"
echo "7. 检查硬编码依赖"
echo "================================================"

# 检查：CrawlerApiModule硬编码依赖
echo "检查: CrawlerApiModule 硬编码依赖 ..."
if [ -f "$BACKEND_INCLUDE/business/CrawlerApiModule.hpp" ]; then
    if grep -q "TemplateCrawlerModule" "$BACKEND_INCLUDE/business/CrawlerApiModule.hpp"; then
        print_error "CrawlerApiModule硬编码依赖TemplateCrawlerModule（应使用ICrawler接口）"
    else
        print_success "CrawlerApiModule无硬编码依赖"
    fi
fi

# 检查：CollaborativeWritingModule硬编码依赖
echo "检查: CollaborativeWritingModule 硬编码依赖 ..."
if [ -f "$BACKEND_INCLUDE/business/CollaborativeWritingModule.hpp" ]; then
    if grep -q "WebSocketModule" "$BACKEND_INCLUDE/business/CollaborativeWritingModule.hpp"; then
        print_error "CollaborativeWritingModule硬编码依赖WebSocketModule（应使用IWebSocket接口）"
    else
        print_success "CollaborativeWritingModule无硬编码依赖"
    fi
fi

# 检查：UnifiedAIWorkflow硬编码依赖
echo "检查: UnifiedAIWorkflow 硬编码依赖 ..."
if [ -f "$BACKEND_INCLUDE/business/UnifiedAIWorkflow.hpp" ]; then
    if grep -q "CacheModule" "$BACKEND_INCLUDE/business/UnifiedAIWorkflow.hpp"; then
        print_error "UnifiedAIWorkflow硬编码依赖CacheModule（应使用ICache接口）"
    else
        print_success "UnifiedAIWorkflow无硬编码依赖"
    fi
fi

echo ""
echo "================================================"
echo "8. 生成依赖关系图"
echo "================================================"

# 生成简单的依赖关系文本图
echo "生成依赖关系图..."
cat > "$PROJECT_ROOT/dependency_graph.txt" << 'EOF'
PaperCrawler 模块依赖关系图
============================

核心层 (Core)
  IM oudle
    ↓
  ModuleBase
    ↓
  ServiceContainer
    ↓
  Router
    ↓
  EventBus

网络层 (Network)
  HttpServerModule → IM oudle
  WebSocketModule → IM oudle
  HttpClient (零依赖)

数据访问层 (Data)
  IDatabase (接口)
    ↓
  DatabaseModule → ModuleBase + IDatabase
  CacheModule → IM oudle
  MySqlConnection → DatabaseModule (需修复)
  RedisConnection → DatabaseModule (需修复)

中间件层 (Middleware)
  ConfigModule → IM oudle
  LoggingModule → IM oudle
  MetricsModule → IM oudle
  SessionModule → IM oudle
  SecurityModule → IM oudle
  CompressionModule → IM oudle
  AsyncTaskModule → IM oudle
  CircuitBreakerModule → IM oudle
  SchedulerModule → IM oudle
  ValidationModule → IM oudle
  ProxyModule → IM oudle

模块层 (Modules)
  CrawlerModule → IM oudle
  TemplateCrawlerModule → IM oudle + CrawlerModule + HttpClient + IDatabase
  DistributedTaskModule → IM oudle + WebSocketModule

业务层 (Business)
  PaperApiModule → ModuleBase + IDatabase
  AuthApiModule → ModuleBase + IDatabase
  UserApiModule → ModuleBase + IDatabase
  SearchApiModule → ModuleBase
  CrawlerApiModule → ModuleBase + TemplateCrawlerModule (需修复) + DistributedTaskModule (需修复)
  AiApiModule → IM oudle
  RecommendationApiModule → IM oudle
  ExportApiModule → IM oudle
  StatsApiModule → IM oudle + ModuleRegistry (需修复)
  AnalyticsIntelligenceModule → ModuleBase
  AiCoPilotModule → ModuleBase
  CollaborativeWritingModule → ModuleBase + WebSocketModule (需修复)
  UnifiedAIWorkflow → IM oudle + CacheModule (需修复)
  ServiceLayer → PaperApiModule (需修复)

图例:
  → 依赖关系
  (需修复) 需要解耦的强依赖
EOF

echo "依赖关系图已生成: $PROJECT_ROOT/dependency_graph.txt"
print_success "依赖关系图生成完成"

echo ""
echo "================================================"
echo "检查总结"
echo "================================================"

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}🎉 所有检查通过！架构健康。${NC}"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo -e "${YELLOW}⚠️  发现 $WARNINGS 个警告，建议修复。${NC}"
    exit 0
else
    echo -e "${RED}❌ 发现 $ERRORS 个错误和 $WARNINGS 个警告，必须修复！${NC}"
    exit 1
fi
