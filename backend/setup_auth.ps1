# PowerShell script to create users table for authentication

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "PaperCrawler Authentication System - Setup" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host ""

# MySQL configuration
$MYSQL_HOST = "127.0.0.1"
$MYSQL_PORT = "3306"
$MYSQL_USER = "root"
$MYSQL_PASS = "123456"
$MYSQL_DB = "papercrawler_db"

# Find MySQL executable
$MYSQL_BIN = ""
$possiblePaths = @(
    "C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe",
    "C:\xampp\mysql\bin\mysql.exe",
    "C:\wamp64\bin\mysql\mysql8.0.31\bin\mysql.exe",
    "C:\laragon\bin\mysql\mysql-8.0.30\bin\mysql.exe"
)

foreach ($path in $possiblePaths) {
    if (Test-Path $path) {
        $MYSQL_BIN = $path
        break
    }
}

if ($MYSQL_BIN -eq "") {
    Write-Host "ERROR: MySQL executable not found!" -ForegroundColor Red
    Write-Host "Please ensure MySQL is installed in one of these locations:" -ForegroundColor Yellow
    foreach ($path in $possiblePaths) {
        Write-Host "  - $path" -ForegroundColor Gray
    }
    Write-Host ""
    Write-Host "Or modify the paths in this script." -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "[OK] Found MySQL at: $MYSQL_BIN" -ForegroundColor Green
Write-Host ""

# Test connection
Write-Host "[1/3] Testing MySQL connection..." -ForegroundColor Cyan
& $MYSQL_BIN -h $MYSQL_HOST -P $MYSQL_PORT -u $MYSQL_USER -p$MYSQL_PASS -e "SELECT VERSION();" 2>$null
if ($LASTEXITCODE -eq 0) {
    Write-Host "[OK] Connected to MySQL successfully" -ForegroundColor Green
} else {
    Write-Host "[ERROR] Failed to connect to MySQL" -ForegroundColor Red
    Write-Host "Please check:" -ForegroundColor Yellow
    Write-Host "  1. MySQL is running" -ForegroundColor Gray
    Write-Host "  2. Password is: $MYSQL_PASS" -ForegroundColor Gray
    Write-Host "  3. User '$MYSQL_USER' has privileges" -ForegroundColor Gray
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host ""

# Execute SQL file
Write-Host "[2/3] Creating users table..." -ForegroundColor Cyan
$SQL_CONTENT = Get-Content create_users_table.sql -Raw
$SQL_CONTENT | & $MYSQL_BIN -h $MYSQL_HOST -P $MYSQL_PORT -u $MYSQL_USER -p$MYSQL_PASS $MYSQL_DB 2>$null
if ($LASTEXITCODE -eq 0) {
    Write-Host "[OK] SQL script executed successfully" -ForegroundColor Green
} else {
    Write-Host "[ERROR] Failed to execute SQL script" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host ""

# Verify tables
Write-Host "[3/3] Verifying tables..." -ForegroundColor Cyan
$RESULT = & $MYSQL_BIN -h $MYSQL_HOST -P $MYSQL_PORT -u $MYSQL_USER -p$MYSQL_PASS $MYSQL_DB -e "SHOW TABLES LIKE 'user%';" 2>$null | Select-Object -Skip 1
if ($RESULT) {
    Write-Host "[OK] Found tables:" -ForegroundColor Green
    foreach ($TABLE in $RESULT) {
        Write-Host "  - $TABLE" -ForegroundColor Gray
    }
} else {
    Write-Host "[WARNING] No user tables found" -ForegroundColor Yellow
}
Write-Host ""

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "Setup completed successfully!" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "You can now test the registration API:" -ForegroundColor Cyan
Write-Host "  curl -X POST http://localhost:8080/api/auth/register \" -ForegroundColor Gray
Write-Host "    -H 'Content-Type: application/json' \" -ForegroundColor Gray
Write-Host "    -d '{\"email\":\"test@example.com\",\"password\":\"test123\",\"name\":\"Test User"}'" -ForegroundColor Gray
Write-Host ""
Write-Host "Or restart the backend server if it's already running." -ForegroundColor Yellow
Write-Host ""

Read-Host "Press Enter to exit"
