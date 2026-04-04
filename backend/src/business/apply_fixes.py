#!/usr/bin/env python3
"""
系统化修复CrawlerApiModule.cpp的所有问题
"""
import re

def read_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        return f.read()

def write_file(filepath, content):
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

def fix_all(content):
    # 1. 修复所有handle方法签名
    content = re.sub(
        r'std::string CrawlerApiModule::handle([A-Za-z]+)\(const std::string& body\)',
        r'HttpResponse CrawlerApiModule::handle\1(const HttpRequest& req)',
        content
    )
    content = re.sub(
        r'std::string CrawlerApiModule::handle([A-Za-z]+)\(const std::map<std::string, std::string>& params\)',
        r'HttpResponse CrawlerApiModule::handle\1(const HttpRequest& req)',
        content
    )
    content = re.sub(
        r'std::string CrawlerApiModule::handle([A-Za-z]+)\(',
        r'HttpResponse CrawlerApiModule::handle\1(const HttpRequest& req)',
        content
    )

    # 2. 修复buildJsonResponse调用 - 4参数改为3参数
    content = re.sub(
        r'buildJsonResponse\((true|false),\s*"([^"]*)",\s*\{([^}]+)\},\s*([^)]+)\)',
        r'buildJsonResponse(\1, "\2")',
        content
    )

    # 3. 修复body -> req.body
    content = re.sub(r'JsonUtils::parse\(body\)', r'JsonUtils::parse(req.body)', content)
    content = re.sub(r'JsonUtils::parse\("([^"]+)"\)', r'JsonUtils::parse(req.body)', content)

    # 4. 修复params -> req.queryParams
    content = re.sub(r'params\.count\(', r'req.queryParams.count(', content)
    content = re.sub(r'params\.at\(', r'req.queryParams.at(', content)

    # 5. 修复pathParams
    content = re.sub(
        r'extractPathParam\(params\.at\(":id"\)',
        r'req.pathParams.at("id"',
        content
    )

    return content

if __name__ == '__main__':
    filepath = 'CrawlerApiModule.cpp'
    content = read_file(filepath)
    content = fix_all(content)
    write_file(filepath, content)
    print("Fixed! Please review and compile.")
