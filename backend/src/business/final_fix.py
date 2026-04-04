#!/usr/bin/env python3
"""
最终修复CrawlerApiModule.cpp的剩余问题
"""
import re

def read_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        return f.read()

def write_file(filepath, content):
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

def fix_final_issues(content):
    # 1. 修复buildJsonResponse 4参数调用
    content = re.sub(
        r'buildJsonResponse\((true|false),\s*"([^"]*)",\s*\{([^}]+)\},\s*([^\)]+)\)',
        r'buildJsonResponse(\1, "\2")',
        content
    )

    # 2. 修复LoggingModule::error调用 - 需要添加模块名参数
    content = re.sub(
        r'logging->error\("([^"]+)"\)',
        r'logging->error("CrawlerApi", "\1")',
        content
    )

    # 3. 修复extractPathParam调用
    content = re.sub(
        r'extractPathParam\(req\.queryParams\.at\(":id"\)',
        r'req.pathParams.at("id"',
        content
    )

    return content

if __name__ == '__main__':
    filepath = 'CrawlerApiModule.cpp'
    content = read_file(filepath)
    content = fix_final_issues(content)
    write_file(filepath, content)
    print("Final fixes applied! Please compile.")
