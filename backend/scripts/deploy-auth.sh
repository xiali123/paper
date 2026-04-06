#!/bin/bash

# ============================================================================
# PaperCrawler Authentication System Deployment Script
# ============================================================================
#
# This script automates the deployment of the authentication system
# including database migrations, configuration, and verification
#
# Usage:
#   ./deploy-auth.sh [environment]
#
# Environments: dev, staging, production
#
# ============================================================================

set -e  # Exit on error

# ============================================================================
# Configuration
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
ENVIRONMENT=${1:-dev}

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ============================================================================
# Helper Functions
# ============================================================================

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_command() {
    if ! command -v $1 &> /dev/null; then
        log_error "Required command '$1' not found"
        exit 1
    fi
}

# ============================================================================
# Pre-flight Checks
# ============================================================================

preflight_checks() {
    log_info "Running pre-flight checks..."

    # Check required commands
    check_command mysql
    check_command sqlite3
    check_command npm

    # Check if backend build exists
    if [ ! -f "$PROJECT_ROOT/backend/build/api_server" ]; then
        log_warning "Backend server not built. Run 'cmake --build build' first."
    fi

    # Check if frontend dependencies installed
    if [ ! -d "$PROJECT_ROOT/frontend/node_modules" ]; then
        log_warning "Frontend dependencies not installed. Run 'npm install' first."
    fi

    log_success "Pre-flight checks completed"
}

# ============================================================================
# Database Migration
# ============================================================================

migrate_database() {
    log_info "Running database migrations..."

    DB_TYPE=${DB_TYPE:-mysql}
    DB_HOST=${DB_HOST:-localhost}
    DB_NAME=${DB_NAME:-papercrawler}
    DB_USER=${DB_USER:-root}
    DB_PASSWORD=${DB_PASSWORD:-}

    if [ "$DB_TYPE" = "mysql" ]; then
        log_info "Migrating MySQL database..."

        # Run migration script
        mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASSWORD" "$DB_NAME" \
            < "$SCRIPT_DIR/../migrations/002_add_authentication.sql"

        log_success "MySQL migration completed"

    elif [ "$DB_TYPE" = "sqlite" ]; then
        log_info "Migrating SQLite database..."

        sqlite3 papercrawler.db \
            < "$SCRIPT_DIR/../migrations/002_add_authentication_sqlite.sql"

        log_success "SQLite migration completed"

    else
        log_error "Unknown database type: $DB_TYPE"
        exit 1
    fi

    # Verify migration
    log_info "Verifying migration..."

    if [ "$DB_TYPE" = "mysql" ]; then
        TABLE_COUNT=$(mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASSWORD" "$DB_NAME" -N -B -e \
            "SELECT COUNT(*) FROM information_schema.tables
             WHERE table_schema = '$DB_NAME'
             AND table_name IN ('users', 'user_sessions', 'login_attempts')")
 || echo "0"

        if [ "$TABLE_COUNT" -eq 3 ]; then
            log_success "All authentication tables created successfully"
        else
            log_error "Migration verification failed. Expected 3 tables, found $TABLE_COUNT"
            exit 1
        fi
    fi
}

# ============================================================================
# Backend Configuration
# ============================================================================

configure_backend() {
    log_info "Configuring backend authentication..."

    CONFIG_FILE="$PROJECT_ROOT/backend/config.json"

    if [ ! -f "$CONFIG_FILE" ]; then
        log_error "Config file not found: $CONFIG_FILE"
        exit 1
    fi

    # Generate JWT secret if not exists
    if ! grep -q '"jwtSecret"' "$CONFIG_FILE"; then
        JWT_SECRET=$(openssl rand -base64 32)
        log_info "Generated new JWT secret"

        # Add to config (using jq or sed)
        if command -v jq &> /dev/null; then
            tmp=$(mktemp)
            jq --arg secret "$JWT_SECRET" '.authentication.jwtSecret = $secret' "$CONFIG_FILE" > "$tmp"
            mv "$tmp" "$CONFIG_FILE"
        else
            log_warning "jq not found. Please manually add jwtSecret to config"
        fi
    fi

    log_success "Backend configuration completed"
}

# ============================================================================
# Frontend Build
# ============================================================================

build_frontend() {
    log_info "Building frontend..."

    cd "$PROJECT_ROOT/frontend"

    # Install dependencies if needed
    if [ ! -d "node_modules" ]; then
        log_info "Installing frontend dependencies..."
        npm install
    fi

    # Build for environment
    if [ "$ENVIRONMENT" = "production" ]; then
        npm run build
    else
        npm run build:dev || npm run build
    fi

    log_success "Frontend build completed"
}

# ============================================================================
# Verification Tests
# ============================================================================

run_tests() {
    log_info "Running verification tests..."

    # Test backend authentication (if server is running)
    if curl -s http://localhost:8080/health > /dev/null; then
        log_info "Backend server is running. Testing authentication endpoints..."

        # Test registration
        REGISTER_RESPONSE=$(curl -s -X POST http://localhost:8080/api/auth/register \
            -H "Content-Type: application/json" \
            -d '{"username":"testuser","email":"test@example.com","password":"TestPass123!"}')

        if echo "$REGISTER_RESPONSE" | grep -q '"success":true'; then
            log_success "Registration endpoint working"
        else
            log_error "Registration endpoint failed"
            echo "Response: $REGISTER_RESPONSE"
        fi

        # Test login
        LOGIN_RESPONSE=$(curl -s -X POST http://localhost:8080/api/auth/login \
            -H "Content-Type: application/json" \
            -d '{"email":"test@example.com","password":"TestPass123!"}')

        if echo "$LOGIN_RESPONSE" | grep -q '"success":true'; then
            log_success "Login endpoint working"
        else
            log_error "Login endpoint failed"
            echo "Response: $LOGIN_RESPONSE"
        fi
    else
        log_warning "Backend server not running. Skipping endpoint tests."
    fi
}

# ============================================================================
# Deployment Steps
# ============================================================================

main() {
    log_info "Starting PaperCrawler Authentication System deployment..."
    log_info "Environment: $ENVIRONMENT"
    echo ""

    # Step 1: Pre-flight checks
    preflight_checks
    echo ""

    # Step 2: Database migration
    migrate_database
    echo ""

    # Step 3: Backend configuration
    configure_backend
    echo ""

    # Step 4: Frontend build
    build_frontend
    echo ""

    # Step 5: Verification
    run_tests
    echo ""

    log_success "Deployment completed successfully!"
    echo ""
    log_info "Next steps:"
    echo "  1. Start the backend server: cd backend && ./build/api_server"
    echo "  2. Start the frontend: cd frontend && npm run dev"
    echo "  3. Open browser: http://localhost:5173"
    echo ""
    log_info "Test credentials:"
    echo "  Email: test@example.com"
    echo "  Password: TestPass123!"
}

# ============================================================================
# Run Main
# ============================================================================

main "$@"
