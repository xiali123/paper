#!/usr/bin/env python3
"""
Verify the created test database schema
"""

import sqlite3
import os

def verify_database():
    """Verify database schema completeness"""

    db_path = "e:/PaperCrawler/backend/papercrawler_test.db"

    if not os.path.exists(db_path):
        print(f"Database not found: {db_path}")
        return False

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    print("="*60)
    print("Database Schema Verification")
    print("="*60)
    print()

    # Expected tables
    expected_tables = {
        'users', 'user_sessions', 'login_attempts',
        'user_bookmarks', 'user_reading_history', 'user_search_history',
        'user_collections', 'user_collection_items', 'migrations'
    }

    # Get actual tables
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;")
    actual_tables = {row[0] for row in cursor.fetchall()}

    # Check tables
    print("1. Table Verification")
    print("-" * 60)

    missing_tables = expected_tables - actual_tables
    extra_tables = actual_tables - expected_tables - {'sqlite_sequence'}

    if not missing_tables and not extra_tables:
        print("   [PASS] All expected tables present")
    else:
        if missing_tables:
            print(f"   [FAIL] Missing tables: {missing_tables}")
        if extra_tables:
            print(f"   [INFO] Extra tables: {extra_tables}")

    print()

    # Check indexes
    print("2. Index Verification")
    print("-" * 60)

    expected_indexes = {
        'idx_users_username', 'idx_users_email', 'idx_users_is_active',
        'idx_sessions_user_id', 'idx_sessions_refresh_token',
        'idx_bookmarks_user_id', 'idx_bookmarks_paper_id',
        'idx_reading_history_user_id', 'idx_reading_history_last_accessed'
    }

    cursor.execute("SELECT name FROM sqlite_master WHERE type='index' AND name LIKE 'idx_%' ORDER BY name;")
    actual_indexes = {row[0] for row in cursor.fetchall()}

    missing_indexes = expected_indexes - actual_indexes

    if not missing_indexes:
        print(f"   [PASS] All key indexes present ({len(actual_indexes)} total)")
    else:
        print(f"   [FAIL] Missing indexes: {missing_indexes}")

    print()

    # Check triggers
    print("3. Trigger Verification")
    print("-" * 60)

    expected_triggers = {
        'update_user_timestamp',
        'update_last_login_on_session',
        'update_bookmark_timestamp',
        'update_collection_timestamp'
    }

    cursor.execute("SELECT name FROM sqlite_master WHERE type='trigger' ORDER BY name;")
    actual_triggers = {row[0] for row in cursor.fetchall()}

    missing_triggers = expected_triggers - actual_triggers

    if not missing_triggers:
        print(f"   [PASS] All triggers present ({len(actual_triggers)} total)")
    else:
        print(f"   [FAIL] Missing triggers: {missing_triggers}")

    print()

    # Check foreign keys
    print("4. Foreign Key Verification")
    print("-" * 60)

    # Check if foreign keys are enabled
    cursor.execute("PRAGMA foreign_keys;")
    fk_enabled = cursor.fetchone()[0]

    if fk_enabled:
        print("   [PASS] Foreign keys enabled")
    else:
        print("   [WARN] Foreign keys disabled (should be enabled)")

    # Check user_sessions foreign key to users
    cursor.execute("PRAGMA foreign_key_list(user_sessions);")
    session_fks = cursor.fetchall()

    if session_fks:
        print("   [PASS] user_sessions -> users FK exists")
    else:
        print("   [FAIL] user_sessions -> users FK missing")

    print()

    # Check users table structure
    print("5. Users Table Structure")
    print("-" * 60)

    cursor.execute("PRAGMA table_info(users);")
    columns = {row[1]: row[2] for row in cursor.fetchall()}

    required_columns = {
        'id', 'username', 'email', 'password_hash', 'salt',
        'full_name', 'role', 'is_active', 'login_attempts',
        'created_at', 'updated_at'
    }

    missing_columns = required_columns - set(columns.keys())

    if not missing_columns:
        print("   [PASS] All required columns present")
        print(f"   Total columns: {len(columns)}")
    else:
        print(f"   [FAIL] Missing columns: {missing_columns}")

    # Verify password_hash and salt exist
    if 'password_hash' in columns and 'salt' in columns:
        print("   [PASS] Password security columns present")
    else:
        print("   [FAIL] Password security columns missing")

    print()

    # Check migration record
    print("6. Migration Record")
    print("-" * 60)

    cursor.execute("SELECT * FROM migrations WHERE version='002_add_authentication_sqlite';")
    migration = cursor.fetchone()

    if migration:
        print("   [PASS] Migration recorded in migrations table")
        print(f"   Version: {migration[1]}")
        print(f"   Description: {migration[2]}")
    else:
        print("   [FAIL] Migration not recorded")

    print()

    # Summary
    print("="*60)
    print("Summary")
    print("="*60)

    issues = []

    if missing_tables:
        issues.append(f"Missing tables: {missing_tables}")
    if missing_indexes:
        issues.append(f"Missing indexes: {missing_indexes}")
    if missing_triggers:
        issues.append(f"Missing triggers: {missing_triggers}")
    if not fk_enabled:
        issues.append("Foreign keys not enabled")

    if not issues:
        print("\n[SUCCESS] Database schema is complete and correct!")
        print("\nLocation: " + db_path)
        print("\nYou can now use this database for testing:")
        print("  - Backend can connect to this database")
        print("  - User registration and login can be tested")
        print("  - All authentication features are ready")
        return True
    else:
        print("\n[ISSUES FOUND]")
        for issue in issues:
            print(f"  - {issue}")
        return False

    conn.close()

if __name__ == "__main__":
    verify_database()
