import mysql.connector

conn = mysql.connector.connect(host='localhost', port=3306, user='root', password='123456', database='papercrawler')
cursor = conn.cursor()

print('=== Users表结构 ===')
cursor.execute('DESCRIBE users')
results = cursor.fetchall()
for row in results:
    print(row)

print('\n=== Users表数据 ===')
cursor.execute('SELECT * FROM users')
users = cursor.fetchall()
for user in users:
    print(user)

conn.close()
