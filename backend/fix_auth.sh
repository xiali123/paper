#!/bin/bash
# Fix authentication tables by running migration in correct database

MYSQL_USER="root"
MYSQL_PASS="123456"
MYSQL_HOST="127.0.0.1"
MYSQL_DB="papercrawler_db"

echo "==================================================================="
echo "PaperCrawler Authentication System - Database Migration"
echo "==================================================================="
echo ""
echo "Database: $MYSQL_DB"
echo "Host: $MYSQL_HOST"
echo ""

# Check if MySQL is accessible
echo "[1/4] Testing MySQL connection..."
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" -e "SELECT VERSION();" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to connect to MySQL"
    echo "Please check:"
    echo "  1. MySQL is running"
    echo "  2. Password is correct (currently: $MYSQL_PASS)"
    echo "  3. User '$MYSQL_USER' has privileges"
    exit 1
fi
echo "OK - MySQL connection successful"
echo ""

# Create database if not exists
echo "[2/4] Creating database if not exists..."
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" -e "CREATE DATABASE IF NOT EXISTS $MYSQL_DB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "OK - Database ready"
else
    echo "ERROR: Failed to create database"
    exit 1
fi
echo ""

# Run migration
echo "[3/4] Running authentication migration..."
mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$MYSQL_DB" < migrations/002_add_authentication.sql 2>/dev/null
if [ $? -eq 0 ]; then
    echo "OK - Migration completed"
else
    echo "ERROR: Migration failed"
    exit 1
fi
echo ""

# Verify tables
echo "[4/4] Verifying tables..."
TABLES=$(mysql -h "$MYSQL_HOST" -u "$MYSQL_USER" -p"$MYSQL_PASS" "$MYSQL_DB" -e "SHOW TABLES LIKE 'user%';" 2>/dev/null | tail -n +2)
if [ -n "$TABLES" ]; then
    echo "OK - Tables created:"
    echo "$TABLES"
else
    echo "WARNING: No user tables found"
fi
echo ""

echo "==================================================================="
echo "Migration completed successfully!"
echo "==================================================================="
echo ""
echo "You can now test registration:"
echo "  curl -X POST http://localhost:8080/api/auth/register \\"
echo "    -H 'Content-Type: application/json' \\"
echo "    -d '{\"email\":\"test@example.com\",\"password\":\"test123\",\"name\":\"Test User\"}'"
echo ""
