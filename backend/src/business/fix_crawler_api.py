#!/usr/bin/env python3
"""
批量修复CrawlerApiModule.cpp中的方法签名
将所有返回std::string的handle方法改为返回HttpResponse
"""

import re
import sys

def fix_method_signature(content):
    """修复方法签名"""
    # 替换std::string为HttpResponse
    content = re.sub(
        r'std::string CrawlerApiModule::handle([A-Za-z]+)\(',
        r'HttpResponse CrawlerApiModule::handle\1(const HttpRequest& req)',
        content
    )

    # 替换参数类型
    content = re.sub(
        r'HttpResponse CrawlerApiModule::handle([A-Za-z]+)\(const std::string& body\)',
        r'HttpResponse CrawlerApiModule::handle\1(const HttpRequest& req)',
        content
    )

    content = re.sub(
        r'HttpResponse CrawlerApiModule::handle([A-Za-z]+)\(const std::map<std::string, std::string>& params\)',
        r'HttpResponse CrawlerApiModule::handle\1(const HttpRequest& req)',
        content
    )

    content = re.sub(
        r'HttpResponse CrawlerApiModule::handle([A-Za-z]+)\(',
        r'HttpResponse CrawlerApiModule::handle\1(const HttpRequest& req)',
        content
    )

    return content

def fix_build_json_response(content):
    """修复buildJsonResponse调用"""
    # 替换4参数调用为3参数
    content = re.sub(
        r'buildJsonResponse(([^,]+),\s*([^,]+),\s*\{([^}]+)\},\s*([^)]+\))',
        r'buildJsonResponse(\1, \2)',
        content
    )

    return content

def fix_query_builder_usage(content):
    """修复QueryBuilder用法"""
    # QueryBuilder::select需要vector参数
    content = re.sub(
        r'QueryBuilder\(database_\)\.select\("([^"]+)"\)',
        r'QueryBuilder(database_).select({"\1"})',
        content
    )

    return content

if __name__ == '__main__':
    input_file = 'CrawlerApiModule.cpp'
    output_file = 'CrawlerApiModule_fixed.cpp'

    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()

    # 应用所有修复
    content = fix_method_signature(content)
    content = fix_build_json_response(content)
    content = fix_query_builder_usage(content)

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(content)

    print(f"Fixed version saved to {output_file}")
