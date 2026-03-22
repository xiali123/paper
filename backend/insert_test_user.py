#!/usr/bin/env python3
"""
Insert test user into PaperCrawler database
"""

import sqlite3
import hashlib
import os
import secrets

def generate_salt(length=32):
    """Generate a random salt for password hashing"""
    return secrets.token_hex(length)

def hash_password_pbkdf2(password, salt, iterations=100000):
    """
    Hash password using PBKDF2-HMAC-SHA256

    Note: This is a simplified version. The actual C++ implementation
    uses OpenSSL's PKCS5_PBKDF2_HMAC which should be identical.
    """
    # Convert salt from hex to bytes
    salt_bytes = bytes.fromhex(salt)

    # Use hashlib's pbkdf2_hmac
    password_hash = hashlib.pbkdf2_hmac(
        'sha256',
        password.encode('utf-8'),
        salt_bytes,
        iterations
    )

    # Return as hex string
    return password_hash.hex()

def insert_test_user():
    """Insert a test user into the database"""

    db_path = "e:/PaperCrawler/backend/papercrawler_test.db"

    if not os.path.exists(db_path):
        print(f"Database not found: {db_path}")
        print("Please run create_test_db.py first.")
        return False

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Enable foreign keys
    cursor.execute("PRAGMA foreign_keys = ON;")

    try:
        # Test user credentials
        username = "testuser"
        email = "test@example.com"
        password = "TestPass123!"
        full_name = "Test User"

        # Generate salt and hash password
        salt = generate_salt(32)
        password_hash = hash_password_pbkdf2(password, salt)

        # Get current timestamp
        import time
        current_time = int(time.time())

        # Insert user
        cursor.execute("""
            INSERT INTO users (
                username, email, password_hash, salt,
                full_name, role, is_active, is_verified,
                password_changed_at, created_at, updated_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            username, email, password_hash, salt,
            full_name, 'user', 1, 0,
            current_time, current_time, current_time
        ))

        user_id = cursor.lastrowid
        conn.commit()

        print("="*60)
        print("Test User Created Successfully!")
        print("="*60)
        print()
        print("User Details:")
        print(f"  ID:       {user_id}")
        print(f"  Username: {username}")
        print(f"  Email:    {email}")
        print(f"  Password: {password}")
        print(f"  Full Name: {full_name}")
        print()
        print("You can now login with these credentials:")
        print(f"  Email:    {email}")
        print(f"  Password: {password}")
        print()
        print("Login at: http://localhost:5173/login")
        print()
        print("="*60)

        return True

    except sqlite3.IntegrityError as e:
        if "UNIQUE constraint failed: users.email" in str(e):
            print("Test user already exists.")
            print()
            print("Existing credentials:")
            print(f"  Email:    {email}")
            print(f"  Password: {password}")
            return True
        else:
            print(f"Integrity error: {e}")
            return False

    except Exception as e:
        print(f"Error inserting test user: {e}")
        return False

    finally:
        conn.close()

if __name__ == "__main__":
    insert_test_user()
