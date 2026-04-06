#!/usr/bin/env python3
"""
Quick health check for PaperCrawler authentication system
"""

import sqlite3
import os
import sys
import json

def check_database():
    """Check database status"""
    db_path = "papercrawler_test.db"

    if not os.path.exists(db_path):
        return False, "Database not found"

    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()

        # Check tables
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
        tables = {row[0] for row in cursor.fetchall()}

        required_tables = {'users', 'user_sessions', 'login_attempts'}
        missing = required_tables - tables

        if missing:
            return False, f"Missing tables: {missing}"

        # Check test user
        cursor.execute("SELECT COUNT(*) FROM users WHERE email='test@example.com';")
        user_count = cursor.fetchone()[0]

        conn.close()

        if user_count > 0:
            return True, f"Database OK, {user_count} test user(s) found"
        else:
            return True, "Database OK, but no test user found. Run insert_test_user.py"

    except Exception as e:
        return False, f"Database error: {e}"

def check_backend_binary():
    """Check if backend is compiled"""
    if os.path.exists("build/PaperCrawlerServer.exe"):
        return True, "Backend compiled"
    else:
        return False, "Backend not compiled (run: cd build && cmake .. && make)"

def check_config():
    """Check configuration file"""
    config_path = "config.json"

    if not os.path.exists(config_path):
        return False, "Config file not found"

    try:
        with open(config_path, 'r') as f:
            config = json.load(f)

        # Check required keys
        if 'authentication' not in config:
            return False, "Authentication config missing"

        auth = config['authentication']
        if not auth.get('enabled', False):
            return False, "Authentication disabled in config"

        return True, f"Config OK (JWT secret: {auth.get('jwtSecret', 'N/A')[:20]}...)"

    except Exception as e:
        return False, f"Config error: {e}"

def main():
    """Run all health checks"""
    print("="*60)
    print("PaperCrawler System Health Check")
    print("="*60)
    print()

    checks = [
        ("Database", check_database),
        ("Backend Binary", check_backend_binary),
        ("Configuration", check_config),
    ]

    all_pass = True

    for name, check_func in checks:
        try:
            success, message = check_func()
            status = "[PASS]" if success else "[FAIL]"
            print(f"{status} {name}")
            print(f"      {message}")
            print()

            if not success:
                all_pass = False
        except Exception as e:
            print(f"[ERROR] {name}: {e}")
            print()
            all_pass = False

    print("="*60)

    if all_pass:
        print("[SUCCESS] All checks passed!")
        print()
        print("You can now start the system:")
        print("  1. Run: start-test-env.bat")
        print("  2. Or: start-backend.bat (and start-frontend.bat in another window)")
        print("  3. Open: http://localhost:5173")
        print("  4. Login with: test@example.com / TestPass123!")
        return 0
    else:
        print("[FAILED] Some checks failed. Please fix the issues above.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
