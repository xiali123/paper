#!/usr/bin/env python3
"""
修复CrawlerApiModule.cpp的路由注册部分
"""
import re

def read_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        return f.read()

def write_file(filepath, content):
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

def fix_routes(content):
    # 修复所有路由注册lambda函数
    # 模式1: return handleXxx(req.body)
    content = re.sub(
        r'return (handle\w+)\(req\.body\)',
        r'return \1(req)',
        content
    )

    # 模式2: return handleXxx(params)
    content = re.sub(
        r'auto params = req\.queryParams;\s+return (handle\w+)\(params\)',
        r'return \1(req)',
        content
    )

    content = re.sub(
        r'std::map<std::string, std::string> params = req\.queryParams;\s+return (handle\w+)\(params\)',
        r'return \1(req)',
        content
    )

    # 模式3: return handleXxx(params, req.body)
    content = re.sub(
        r'std::map<std::string, std::string> params = req\.queryParams;\s+params\["id"\] = req\.getPathParam\("id"\);\s+return (handle\w+)\(params, req\.body\)',
        r'return \1(req)',
        content
    )

    content = re.sub(
        r'std::map<std::string, std::string> params = req\.queryParams;\s+params\["id"\] = req\.getPathParam\("id"\);\s+return (handle\w+)\(params\)',
        r'return \1(req)',
        content
    )

    return content

if __name__ == '__main__':
    filepath = 'CrawlerApiModule.cpp'
    content = read_file(filepath)
    content = fix_routes(content)
    write_file(filepath, content)
    print("Routes fixed! Please compile.")
