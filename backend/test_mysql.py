#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PaperCrawler MySQL数据库连接测试脚本
用法: python test_mysql.py
"""

import mysql.connector
from mysql.connector import Error
import sys

# Windows兼容性设置
if sys.platform == 'win32':
    import codecs
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout.buffer, 'strict')

def test_mysql_connection():
    print("=" * 50)
    print("PaperCrawler MySQL 数据库连接测试")
    print("=" * 50)

    # 数据库配置
    config = {
        'host': 'localhost',
        'port': 3306,
        'user': 'root',
        'password': '123456',  # 用户提供的密码
        'database': 'papercrawler',
        'charset': 'utf8mb4',
        'collation': 'utf8mb4_unicode_ci'
    }

    print("\n连接配置:")
    print(f"  Host: {config['host']}:{config['port']}")
    print(f"  User: {config['user']}")
    print(f"  Database: {config['database']}")
    print(f"  Password: *******")

    connection = None
    try:
        # 1. 测试连接
        print("\n[1/6] 正在连接MySQL...")
        connection = mysql.connector.connect(**config)
        if connection.is_connected():
            print("[OK] 连接成功！")
        else:
            print("[ERROR] 连接失败！")
            return False

        # 2. 获取MySQL版本
        print("\n[2/6] 获取MySQL版本信息...")
        cursor = connection.cursor()
        cursor.execute("SELECT VERSION()")
        version = cursor.fetchone()
        print(f"[OK] MySQL Version: {version[0]}")

        # 3. 查看当前数据库
        print("\n[3/6] 查询当前数据库...")
        cursor.execute("SELECT DATABASE()")
        current_db = cursor.fetchone()
        print(f"[OK] Current Database: {current_db[0]}")

        # 4. 查看所有表
        print("\n[4/6] 查询所有表...")
        cursor.execute("SHOW TABLES")
        tables = cursor.fetchall()
        print(f"[OK] 找到 {len(tables)} 个表:")
        for table in tables:
            print(f"   - {table[0]}")

        # 5. 查询users表数据
        print("\n[5/6] 查询 users 表...")
        try:
            cursor.execute("SELECT id, username, email, role, active FROM users LIMIT 5")
            users = cursor.fetchall()
            if users:
                print(f"[OK] 找到 {len(users)} 个用户:")
                for user in users:
                    print(f"   ID: {user[0]}, Username: {user[1]}, Email: {user[2]}, Role: {user[3]}, Active: {user[4]}")
            else:
                print("[INFO] users表为空")
        except Error as e:
            print(f"[WARN] users表查询失败: {e}")

        # 6. 查询papers表数据
        print("\n[6/6] 查询 papers 表...")
        try:
            cursor.execute("SELECT id, title, authors, year, citation_count FROM papers LIMIT 5")
            papers = cursor.fetchall()
            if papers:
                print(f"[OK] 找到 {len(papers)} 篇论文:")
                for paper in papers:
                    print(f"   ID: {paper[0]}, Title: {paper[1]}, Authors: {paper[2]}, Year: {paper[3]}, Citations: {paper[4]}")
            else:
                print("[INFO] papers表为空")
        except Error as e:
            print(f"[WARN] papers表查询失败: {e}")

        # 7. 数据库统计
        print("\n[EXTRA] 数据库统计...")
        try:
            cursor.execute("""
                SELECT table_name, table_rows
                FROM information_schema.tables
                WHERE table_schema = 'papercrawler'
                ORDER BY table_rows DESC
            """)
            stats = cursor.fetchall()
            if stats:
                print("表名和行数（按行数降序）:")
                for stat in stats:
                    print(f"   {stat[0]}: {stat[1]} 行")
        except Error as e:
            print(f"[WARN] 统计查询失败: {e}")

        # 8. 测试插入功能
        print("\n[EXTRA] 测试插入功能...")
        try:
            # 检查是否已有测试用户
            cursor.execute("SELECT id FROM users WHERE username = 'test_python_user'")
            if cursor.fetchone() is None:
                print("插入测试用户...")
                cursor.execute("""
                    INSERT INTO users (username, email, password_hash, full_name, role, active, created_at)
                    VALUES ('test_python_user', 'python@example.com', 'hashed_password', 'Python Test User', 'user', 1, NOW())
                """)
                connection.commit()
                insert_id = cursor.lastrowid
                print(f"[OK] 测试用户插入成功！ID: {insert_id}")

                # 查询刚插入的用户
                cursor.execute(f"SELECT * FROM users WHERE id = {insert_id}")
                new_user = cursor.fetchone()
                if new_user:
                    print(f"[OK] 验证查询成功: {new_user[1]}")
            else:
                print("[INFO] 测试用户已存在")
        except Error as e:
            print(f"[WARN] 插入测试失败: {e}")

        cursor.close()

        print("\n" + "=" * 50)
        print("[SUCCESS] 所有测试通过！数据库连接正常。")
        print("=" * 50)

        # 9. 提供下一步建议
        print("\n下一步操作建议:")
        print("1. 如果数据库为空，导入schema:")
        print("   mysql -u root -p123456 papercrawler < database/complete-schema-mysql.sql")
        print("2. 启动PaperCrawler服务器:")
        print("   cd backend/build/Release")
        print("   ./PaperCrawlerServer.exe")
        print("3. 测试API端点:")
        print("   curl http://localhost:8080/api/papers")

        return True

    except Error as e:
        print(f"\n[ERROR] 数据库连接失败: {e}")
        print("\n可能的原因:")
        print("1. MySQL服务未启动")
        print("2. 用户名或密码错误")
        print("3. 数据库 'papercrawler' 不存在")
        print("4. 防火墙阻止连接")
        print("\n解决方法:")
        print("1. 启动MySQL服务: net start MySQL80")
        print("2. 创建数据库: CREATE DATABASE papercrawler CHARACTER SET utf8mb4;")
        print("3. 导入schema: mysql -u root -p123456 papercrawler < database/complete-schema-mysql.sql")
        return False

    finally:
        if connection and connection.is_connected():
            connection.close()
            print("\n数据库连接已关闭")

if __name__ == "__main__":
    try:
        success = test_mysql_connection()
        exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\n测试被用户中断")
        exit(1)
    except Exception as e:
        print(f"\n[ERROR] 未知错误: {e}")
        exit(1)
