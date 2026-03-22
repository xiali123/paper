#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PaperCrawler Authentication System Validation
"""

import os

def main():
    print("="*60)
    print("PaperCrawler Authentication System Validation")
    print("="*60)
    print()

    base_path = "e:/PaperCrawler"

    # Check database schemas
    mysql_schema = f"{base_path}/backend/migrations/002_add_authentication.sql"
    sqlite_schema = f"{base_path}/backend/migrations/002_add_authentication_sqlite.sql"

    if os.path.exists(mysql_schema):
        print("[OK] MySQL schema exists")
        with open(mysql_schema, 'r', encoding='utf-8') as f:
            content = f.read()
            if 'CREATE TABLE IF NOT EXISTS users' in content:
                print("[OK]   - users table defined")
            if 'CREATE TABLE IF NOT EXISTS user_sessions' in content:
                print("[OK]   - user_sessions table defined")
            if 'CREATE TABLE IF NOT EXISTS login_attempts' in content:
                print("[OK]   - login_attempts table defined")
    else:
        print("[FAIL] MySQL schema not found")

    print()

    if os.path.exists(sqlite_schema):
        print("[OK] SQLite schema exists")
        with open(sqlite_schema, 'r', encoding='utf-8') as f:
            content = f.read()
            if 'CREATE TABLE IF NOT EXISTS users' in content:
                print("[OK]   - users table defined")
            if 'CREATE TABLE IF NOT EXISTS local_sessions' in content:
                print("[OK]   - local_sessions table defined")
    else:
        print("[FAIL] SQLite schema not found")

    print()
    print("="*60)
    print("Core Components Check")
    print("="*60)
    print()

    # Check backend
    components = [
        ("AuthManager.hpp", f"{base_path}/include/auth/AuthManager.hpp"),
        ("JwtUtils.hpp", f"{base_path}/include/auth/JwtUtils.hpp"),
        ("PasswordHasher.hpp", f"{base_path}/include/auth/PasswordHasher.hpp"),
        ("RateLimiter.hpp", f"{base_path}/include/auth/RateLimiter.hpp"),
        ("auth_handlers.cpp", f"{base_path}/backend/src/auth_handlers.cpp"),
        ("auth_middleware.cpp", f"{base_path}/backend/src/auth_middleware.cpp"),
    ]

    for name, path in components:
        if os.path.exists(path):
            print(f"[OK] {name}")
        else:
            print(f"[MISSING] {name}")

    print()

    # Check frontend
    frontend_components = [
        ("auth.ts (API)", f"{base_path}/frontend/src/api/modules/auth.ts"),
        ("auth.ts (Store)", f"{base_path}/frontend/src/stores/auth.ts"),
        ("Login.vue", f"{base_path}/frontend/src/views/Login.vue"),
        ("Register.vue", f"{base_path}/frontend/src/views/Register.vue"),
        ("guards.ts", f"{base_path}/frontend/src/router/guards.ts"),
    ]

    print("Frontend:")
    for name, path in frontend_components:
        if os.path.exists(path):
            print(f"[OK]   {name}")
        else:
            print(f"[MISSING] {name}")

    print()
    print("="*60)
    print("Validation Complete!")
    print("="*60)

if __name__ == "__main__":
    main()
