#!/usr/bin/env python3
"""
Quick database schema validation for PaperCrawler authentication
"""

import os

def validate_sql_schema(filepath):
    """Validate SQL schema file"""
    print(f"Validating: {os.path.basename(filepath)}")

    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    checks = {
        'users_table': 'CREATE TABLE.*users' in content,
        'user_sessions_table': 'CREATE TABLE.*user_sessions' in content,
        'login_attempts_table': 'CREATE TABLE.*login_attempts' in content,
        'user_bookmarks_table': 'CREATE TABLE.*user_bookmarks' in content,
        'password_hashing': 'password_hash' in content.lower(),
        'jwt_tokens': 'refresh_token' in content.lower(),
        'rate_limiting': 'login_attempts' in content.lower(),
    }

    passed = 0
    total = len(checks)

    for check_name, result in checks.items():
        status = "✓" if result else "✗"
        print(f"  {status} {check_name}")
        if result:
            passed += 1

    print(f"  Score: {passed}/{total}\n")
    return passed == total

def main():
    print("🔍 PaperCrawler Authentication Schema Validation")
    print("=" * 60)
    print()

    base_path = "e:/PaperCrawler/backend/migrations"

    schemas = [
        ("MySQL Authentication", f"{base_path}/002_add_authentication.sql"),
        ("SQLite Authentication", f"{base_path}/002_add_authentication_sqlite.sql"),
    ]

    total_passed = 0
    total_checks = len(schemas) * 7  # 7 checks per schema

    for name, path in schemas:
        if os.path.exists(path):
            if validate_sql_schema(path):
                total_passed += 7
            else:
                print(f"⚠️  {name} has issues\n")
        else:
            print(f"✗ File not found: {path}\n")

    print("=" * 60)
    print(f"\n📊 Overall: {total_passed}/{total_checks} checks passed")

    if total_passed == total_checks:
        print("\n🎉 Database schemas are valid and ready for use!")
        print("\n💡 To create a test database:")
        print("   sqlite3 papercrawler.db < migrations/002_add_authentication_sqlite.sql")
    else:
        print("\n⚠️  Some schema issues detected. Please review.")

if __name__ == "__main__":
    main()
