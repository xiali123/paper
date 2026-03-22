#!/usr/bin/env python3
"""
Create SQLite test database for PaperCrawler authentication
"""

import sqlite3
import os

def create_database():
    """Create test database from migration script"""

    db_path = "e:/PaperCrawler/backend/papercrawler_test.db"
    schema_path = "e:/PaperCrawler/backend/migrations/002_add_authentication_sqlite.sql"

    # Remove existing database if it exists
    if os.path.exists(db_path):
        os.remove(db_path)
        print(f"Removed existing database: {db_path}")

    # Read the migration schema
    with open(schema_path, 'r', encoding='utf-8') as f:
        sql_script = f.read()

    # Connect to database and execute schema
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    try:
        # Enable foreign keys
        cursor.execute("PRAGMA foreign_keys = ON;")

        # Execute the entire script at once (handles triggers correctly)
        cursor.executescript(sql_script)

        conn.commit()

        # List all tables
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;")
        tables = cursor.fetchall()

        print("\n" + "="*60)
        print("Database Created Successfully!")
        print("="*60)
        print(f"\nLocation: {db_path}")
        print("\nTables created:")

        for table in tables:
            table_name = table[0]
            print(f"  [OK] {table_name}")

            # Show table schema
            cursor.execute(f"PRAGMA table_info({table_name});")
            columns = cursor.fetchall()
            print(f"    Columns: {', '.join([col[1] for col in columns])}")

        print("\n" + "="*60)
        print("Test database is ready for use!")
        print("="*60)

        return True

    except Exception as e:
        print(f"Error creating database: {e}")
        return False

    finally:
        conn.close()

if __name__ == "__main__":
    create_database()
