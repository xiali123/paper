#!/bin/bash
# ============================================================================
# PaperCrawler Backend - Module Development Standards Compliance Checker
# ============================================================================
#
# This script checks if a module conforms to the development standards defined
# in MODULE_DEVELOPMENT_STANDARDS.md
#
# Usage:
#   bash scripts/check_module_standards.sh <ModuleName>
#
# Example:
#   bash scripts/check_module_standards.sh SearchApiModule
#
# Exit codes:
#   0 - All checks passed
#   1 - One or more checks failed
#
# ============================================================================

# Don't exit on error - we want to continue checking even if one check fails

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Parse arguments
MODULE_NAME=$1

if [ -z "$MODULE_NAME" ]; then
    echo -e "${RED}❌ Error: Module name not provided${NC}"
    echo ""
    echo "Usage: $0 <ModuleName>"
    echo ""
    echo "Example: $0 SearchApiModule"
    exit 1
fi

# ============================================================================
# Configuration
# ============================================================================

HEADER_FILE="include/business/${MODULE_NAME}.hpp"
SOURCE_FILE="src/business/${MODULE_NAME}.cpp"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}🔍 PaperCrawler Module Standards Compliance Checker${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Module: ${MODULE_NAME}"
echo -e "Project Root: ${PROJECT_ROOT}"
echo ""

# Change to project root
cd "$PROJECT_ROOT" || {
    echo -e "${RED}❌ Error: Cannot change to project root: ${PROJECT_ROOT}${NC}"
    exit 1
}

# ============================================================================
# Check Counter
# ============================================================================

CHECKS_PASSED=0
CHECKS_FAILED=0
CHECKS_WARNING=0

# Helper functions
check_passed() {
    echo -e "${GREEN}✅ PASS:${NC} $1"
    ((CHECKS_PASSED++))
}

check_failed() {
    echo -e "${RED}❌ FAIL:${NC} $1"
    ((CHECKS_FAILED++))
}

check_warning() {
    echo -e "${YELLOW}⚠️  WARN:${NC} $1"
    ((CHECKS_WARNING++))
}

# ============================================================================
# Check 1: Header File Exists
# ============================================================================

echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Check 1: Header File Existence${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

if [ -f "$HEADER_FILE" ]; then
    check_passed "Header file exists: ${HEADER_FILE}"
else
    check_failed "Header file not found: ${HEADER_FILE}"
    echo ""
    echo -e "${RED}❌ Module '${MODULE_NAME}' does not exist or is not in the correct location${NC}"
    echo ""
    echo "Expected locations:"
    echo "  - Header: ${HEADER_FILE}"
    echo "  - Source: ${SOURCE_FILE}"
    echo ""
    echo "Please ensure the module files are in the correct directories."
    exit 1
fi

# ============================================================================
# Check 2: Source File Exists
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Check 2: Source File Existence${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

if [ -f "$SOURCE_FILE" ]; then
    check_passed "Source file exists: ${SOURCE_FILE}"
else
    check_failed "Source file not found: ${SOURCE_FILE}"
fi

# ============================================================================
# Check 3: Default Constructor Declaration
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Check 3: Default Constructor Declaration${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

if grep -q "${MODULE_NAME}()" "$HEADER_FILE"; then
    check_passed "Default constructor declared in header file"
else
    check_failed "Default constructor not declared in header file"
    echo ""
    echo "Required declaration:"
    echo "  class ${MODULE_NAME} : public BusinessModuleBase {"
    echo "  public:"
    echo "      ${MODULE_NAME}();  // <-- Required!"
    echo "      ~${MODULE_NAME}() override;"
fi

# ============================================================================
# Check 4: Base Class Inheritance
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Check 4: Base Class Inheritance${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

if grep -q ": public BusinessModuleBase" "$HEADER_FILE"; then
    check_passed "Module inherits from BusinessModuleBase"
else
    check_failed "Module does not inherit from BusinessModuleBase"
    echo ""
    echo "Required declaration:"
    echo "  class ${MODULE_NAME} : public BusinessModuleBase { ... };"
fi

# ============================================================================
# Check 5: Destructor Override
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Check 5: Destructor Override${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

if grep -q "~${MODULE_NAME}() override" "$HEADER_FILE"; then
    check_passed "Destructor declared with override keyword"
else
    check_warning "Destructor should be declared with 'override' keyword"
    echo ""
    echo "Recommended declaration:"
    echo "  ~${MODULE_NAME}() override;"
fi

# ============================================================================
# Check 6: Virtual Methods (Should NOT be reimplemented)
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Check 6: Prohibited Base Class Method Reimplementation${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

PROHIBITED_METHODS="initialize|start|stop|cleanup"
HAS_PROHIBITED=false

for METHOD in initialize start stop cleanup; do
    if grep -q "bool ${MODULE_NAME}::${METHOD}()" "$SOURCE_FILE" 2>/dev/null; then
        check_failed "Prohibited method reimplemented: ${METHOD}()"
        echo ""
        echo "⚠️  The following methods are already implemented in BusinessModuleBase"
        echo "   and should NOT be reimplemented in ${MODULE_NAME}:"
        echo "   - initialize()"
        echo "   - start()"
        echo "   - stop()"
        echo "   - cleanup()"
        HAS_PROHIBITED=true
    fi
done

if [ "$HAS_PROHIBITED" = false ]; then
    check_passed "No prohibited base class methods reimplemented"
fi

# ============================================================================
# Check 7: Default Constructor Implementation
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Check 7: Default Constructor Implementation${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

if grep -q "${MODULE_NAME}::${MODULE_NAME}()" "$SOURCE_FILE"; then
    check_passed "Default constructor implemented in source file"
else
    check_failed "Default constructor not implemented in source file"
    echo ""
    echo "Required implementation:"
    echo "  ${MODULE_NAME}::${MODULE_NAME}()"
    echo "      : ${MODULE_NAME}(nullptr) {"
    echo "      std::cout << \"[${MODULE_NAME}] Default constructor\" << std::endl;"
    echo "  }"
fi

# ============================================================================
# Check 8: DLL Export Functions
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Check 8: DLL Export Functions${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

EXPORT_FUNCTIONS_OK=true

if ! grep -q "createModule" "$SOURCE_FILE"; then
    check_failed "DLL export function 'createModule' not found"
    EXPORT_FUNCTIONS_OK=false
fi

if ! grep -q "destroyModule" "$SOURCE_FILE"; then
    check_failed "DLL export function 'destroyModule' not found"
    EXPORT_FUNCTIONS_OK=false
fi

if ! grep -q "getModuleVersion" "$SOURCE_FILE"; then
    check_failed "DLL export function 'getModuleVersion' not found"
    EXPORT_FUNCTIONS_OK=false
fi

if [ "$EXPORT_FUNCTIONS_OK" = true ]; then
    check_passed "All required DLL export functions present"
else
    echo ""
    echo "Required DLL export functions (at end of .cpp file):"
    echo ""
    echo "  #define EXPORT __declspec(dllexport)"
    echo ""
    echo "  extern \"C\" {"
    echo ""
    echo "  EXPORT void* createModule() {"
    echo "      return new PaperCrawler::${MODULE_NAME}();"
    echo "  }"
    echo ""
    echo "  EXPORT void destroyModule(void* ptr) {"
    echo "      delete static_cast<PaperCrawler::${MODULE_NAME}*>(ptr);"
    echo "  }"
    echo ""
    echo "  EXPORT const char* getModuleVersion() {"
    echo "      return \"1.0.0\";"
    echo "  }"
    echo ""
    echo "  }"
fi

# ============================================================================
# Check 9: Dependencies in Correct Location
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Check 9: Third-Party Dependencies${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

# Check for common dependencies
DEPENDENCIES=""

if grep -q "HttpClient" "$SOURCE_FILE" 2>/dev/null; then
    DEPENDENCIES="${DEPENDENCIES} libcurl-x64.dll"
fi

if grep -q "IDatabase" "$SOURCE_FILE" 2>/dev/null; then
    DEPENDENCIES="${DEPENDENCIES} libmysql.dll"
fi

if [ -n "$DEPENDENCIES" ]; then
    echo ""
    echo -e "${YELLOW}📦 Detected dependencies:${NC}${DEPENDENCIES}"

    DEPS_OK=true
    for DEP in $DEPENDENCIES; do
        if [ -f "dependencies/runtime/${DEP}" ]; then
            echo -e "  ${GREEN}✅${NC} ${DEP} found in dependencies/runtime/"
        else
            echo -e "  ${RED}❌${NC} ${DEP} NOT found in dependencies/runtime/"
            DEPS_OK=false
        fi
    done

    if [ "$DEPS_OK" = true ]; then
        check_passed "All dependencies in correct location"
    else
        check_failed "Some dependencies missing from dependencies/runtime/"
        echo ""
        echo "⚠️  Please place the required DLL files in: dependencies/runtime/"
    fi
else
    check_passed "No third-party dependencies detected (or not using standard patterns)"
fi

# ============================================================================
# Check 10: Module Metadata
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Check 10: Module Metadata${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

if grep -q "getName()" "$HEADER_FILE"; then
    check_passed "Module getName() method declared"
else
    check_warning "Module getName() method not found"
fi

if grep -q "getVersion()" "$HEADER_FILE"; then
    check_passed "Module getVersion() method declared"
else
    check_warning "Module getVersion() method not found"
fi

if grep -q "getDescription()" "$HEADER_FILE"; then
    check_passed "Module getDescription() method declared"
else
    check_warning "Module getDescription() method not found"
fi

# ============================================================================
# Summary
# ============================================================================

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}📊 Check Summary${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "  Checks Passed: ${GREEN}${CHECKS_PASSED}${NC}"
echo -e "  Checks Failed: ${RED}${CHECKS_FAILED}${NC}"
echo -e "  Warnings:      ${YELLOW}${CHECKS_WARNING}${NC}"
echo ""

# ============================================================================
# Final Result
# ============================================================================

if [ $CHECKS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✅✅✅ All checks PASSED! Module '${MODULE_NAME}' conforms to standards.${NC}"
    echo ""
    exit 0
else
    echo -e "${RED}❌❌❌ Some checks FAILED! Module '${MODULE_NAME}' does NOT conform to standards.${NC}"
    echo ""
    echo -e "${YELLOW}📚 Please refer to: backend/MODULE_DEVELOPMENT_STANDARDS.md${NC}"
    echo ""
    exit 1
fi
