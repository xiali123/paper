#!/usr/bin/env python3
"""
PaperCrawler Database Initialization Script

This script initializes the SQLite database with:
1. Schema creation
2. Test data insertion
3. Index building
"""

import sqlite3
import os
import sys
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

class DatabaseInitializer:
    def __init__(self, db_path="papercrawler.db"):
        self.db_path = db_path
        self.conn = None

    def connect(self):
        """Connect to database"""
        self.conn = sqlite3.connect(self.db_path)
        # Enable foreign keys
        self.conn.execute("PRAGMA foreign_keys = ON")
        # Performance optimizations
        self.conn.execute("PRAGMA journal_mode = WAL")
        self.conn.execute("PRAGMA synchronous = NORMAL")
        self.conn.execute("PRAGMA cache_size = -10000")
        print(f"[OK] Connected to database: {self.db_path}")

    def close(self):
        """Close database connection"""
        if self.conn:
            self.conn.close()
            print("[OK] Database connection closed")

    def execute_sql_file(self, sql_file):
        """Execute SQL from a file"""
        if not os.path.exists(sql_file):
            print(f"[ERROR] SQL file not found: {sql_file}")
            return False

        print(f"Executing: {sql_file}")
        with open(sql_file, 'r', encoding='utf-8') as f:
            sql = f.read()

        try:
            # Split by semicolon and execute each statement
            statements = [s.strip() for s in sql.split(';') if s.strip()]
            for statement in statements:
                if statement:
                    self.conn.execute(statement)

            self.conn.commit()
            print(f"[OK] Executed {sql_file}")
            return True
        except Exception as e:
            print(f"[ERROR] Error executing {sql_file}: {e}")
            self.conn.rollback()
            return False

    def create_tables(self):
        """Create database tables"""
        migrations_path = Path(__file__).parent.parent / "migrations"
        schema_file = migrations_path / "001_init_schema_sqlite.sql"
        auth_file = migrations_path / "002_add_authentication_sqlite.sql"
        superadmin_file = migrations_path / "003_add_superadmin_sqlite.sql"
        sync_file = migrations_path / "004_add_sync_support_sqlite.sql"

        print("\n=== Creating Database Tables ===")

        # Execute in order
        files = [
            ("001_init_schema_sqlite.sql", schema_file),
            ("002_add_authentication_sqlite.sql", auth_file),
            ("003_add_superadmin_sqlite.sql", superadmin_file),
            ("004_add_sync_support_sqlite.sql", sync_file),
        ]

        for name, file_path in files:
            if file_path.exists():
                self.execute_sql_file(str(file_path))
            else:
                print(f"[WARN] Migration file not found: {name}")

    def insert_test_data(self):
        """Insert test data"""
        test_data_file = Path(__file__).parent.parent / "migrations" / "test_data.sql"

        print("\n=== Inserting Test Data ===")
        if test_data_file.exists():
            self.execute_sql_file(str(test_data_file))
        else:
            print("[WARN] Test data file not found")

    def verify_data(self):
        """Verify data was inserted correctly"""
        print("\n=== Verifying Data ===")

        cursor = self.conn.cursor()

        # Check tables
        cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
        tables = [row[0] for row in cursor.fetchall()]
        print(f"[OK] Tables created: {len(tables)}")
        for table in tables:
            print(f"  - {table}")

        # Check papers
        cursor.execute("SELECT COUNT(*) FROM papers")
        paper_count = cursor.fetchone()[0]
        print(f"[OK] Papers inserted: {paper_count}")

        # Show some papers
        cursor.execute("SELECT id, title, year, publication FROM papers LIMIT 3")
        print("\nSample papers:")
        for row in cursor.fetchall():
            print(f"  [{row[0]}] {row[1]} ({row[2]}) - {row[3]}")

        # Check journals
        cursor.execute("SELECT COUNT(*) FROM journals")
        journal_count = cursor.fetchone()[0]
        print(f"[OK] Journals inserted: {journal_count}")

        # Check authors
        cursor.execute("SELECT COUNT(*) FROM authors")
        author_count = cursor.fetchone()[0]
        print(f"[OK] Authors inserted: {author_count}")

        # Check collections
        cursor.execute("SELECT COUNT(*) FROM collections")
        collection_count = cursor.fetchone()[0]
        print(f"[OK] Collections created: {collection_count}")

        # Check users (if authentication tables exist)
        try:
            cursor.execute("SELECT COUNT(*) FROM users")
            user_count = cursor.fetchone()[0]
            print(f"[OK] Users table ready: {user_count} users")
        except:
            print("[OK] Users table not yet created (authentication not initialized)")

    def run(self, drop_existing=False):
        """Run complete initialization"""
        print("=" * 60)
        print("PaperCrawler Database Initialization")
        print("=" * 60)

        # Drop existing database if requested
        if drop_existing and os.path.exists(self.db_path):
            print(f"\nRemoving existing database: {self.db_path}")
            os.remove(self.db_path)

        try:
            self.connect()

            if drop_existing:
                print("\n=== Fresh Start ===")
            else:
                print("\n=== Updating Existing Database ===")

            self.create_tables()
            self.insert_test_data()
            self.verify_data()

            print("\n" + "=" * 60)
            print("[OK] Database initialization complete!")
            print("=" * 60)

            return True

        except Exception as e:
            print(f"\n[ERROR] Initialization failed: {e}")
            import traceback
            traceback.print_exc()
            return False

        finally:
            self.close()

def main():
    import argparse

    parser = argparse.ArgumentParser(description="Initialize PaperCrawler database")
    parser.add_argument("--db", default="papercrawler.db", help="Database file path")
    parser.add_argument("--fresh", action="store_true", help="Drop existing database and start fresh")
    parser.add_argument("--test", action="store_true", help="Initialize test database")

    args = parser.parse_args()

    if args.test:
        args.db = "papercrawler_test.db"

    initializer = DatabaseInitializer(args.db)
    success = initializer.run(drop_existing=args.fresh)

    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
