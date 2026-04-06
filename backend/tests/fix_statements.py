#!/usr/bin/env python3
"""
修复DistributedTaskModule.cpp和CrawlerApiModule.cpp中的
PreparedStatement和QueryBuilder使用，转换为直接SQL执行
"""

import re
import sys

def fix_prepared_statement(content):
    """替换PreparedStatement用法为直接SQL执行"""

    # Pattern 1: 简单的INSERT/UPDATE语句
    # PreparedStatement stmt(database_, "SQL ? ? ?");
    # stmt.bind(1, val1);
    # stmt.bind(2, val2);
    # if (stmt.execute()) {

    pattern1 = r'''PreparedStatement\s+stmt\s*\(\s*database_\s*,\s*"([^"]+)"\s*\);

        ((?:\s+stmt\.bind\([^)]+\);)+)

        \s*if\s*\(\s*stmt\.execute\(\)\s*\)'''

    def replace_with_sql(match):
        sql_template = match.group(1)
        binds = match.group(2)

        # 提取所有bind值
        bind_values = []
        for bind_match in re.finditer(r'std::to_string\([^)]+\)|"[^"]*"|[^,\s]+', binds):
            val = bind_match.group(0).strip()
            if val and val != 'std::to_string()' and not val.startswith('//'):
                bind_values.append(val)

        # 构建替换的SQL
        # 替换 ? 为实际值（简化版本，实际需要注意类型转换）
        sql = sql_template
        for val in bind_values:
            sql = sql.replace('?', f"'{val}'", 1)

        return f'''// SQL: {sql_template}
        std::ostringstream sql;
        sql << "{sql_template}";
        // TODO: 替换 ? 为实际值

        if (database_->execute(sql.str()))'''

    content = re.sub(pattern1, replace_with_sql, content, flags=re.MULTILINE | re.DOTALL)

    return content

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python fix_statements.py <file.cpp>")
        sys.exit(1)

    filename = sys.argv[1]
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()

    fixed_content = fix_prepared_statement(content)

    with open(filename, 'w', encoding='utf-8') as f:
        f.write(fixed_content)

    print(f"Fixed {filename}")
