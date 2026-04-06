#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import mysql.connector
import sys

if sys.platform == 'win32':
    import codecs
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout.buffer, 'strict')

def check_users_table():
    print("=== 查询users表结构 ===")

    try:
        connection = mysql.connector.connect(
            host='localhost',
            port=3306,
            user='root',
            password='123456',
            database='papercrawler'
        )

        cursor = connection.cursor()

        # 查询users表结构
        print("\nUsers表结构:")
        cursor.execute("DESCRIBE users")
        columns = cursor.fetchall()

        print("字段名\t\t类型\t\t\t\t\t\t\tNull\tKey\t默认值\t\tExtra")
        print("-" * 100)
        for col in columns:
            field = col[0]
            type_info = col[1]
            null_info = col[2]
            key_info = col[3]
            default_info = col[4] if col[4] else "NULL"
            extra_info = col[5]
            print(f"{field:<20}\t{type_info:<20}\t{null_info:<5}\t{key_info:<5}\t{default_info:<15}\t{extra_info}")

        # 查询users表数据
        print("\n\nUsers表数据:")
        cursor.execute("SELECT * FROM users")
        users = cursor.fetchall()

        if users:
            # 获取列名
            cursor.execute("DESCRIBE users")
            columns = [col[0] for col in cursor.fetchall()]

            print(f"找到 {len(users)} 个用户:")
            for user in users:
                user_dict = dict(zip(columns, user))
                print(f"\n  User: {user_dict}")
        else:
            print("users表为空")

        cursor.close()
        connection.close()

    except Exception as e:
        print(f"错误: {e}")

if __name__ == "__main__":
    check_users_table()
