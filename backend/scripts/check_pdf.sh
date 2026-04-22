#!/bin/bash
# PDF下载问题诊断脚本

echo "==================================="
echo "PDF下载问题诊断工具"
echo "==================================="
echo ""

# 检查后端服务是否运行
echo "1. 检查后端服务..."
if curl -s http://localhost:8080/api/health > /dev/null; then
    echo "   ✅ 后端服务运行中"
else
    echo "   ❌ 后端服务未运行"
    echo "   请启动后端: cd backend/build/Release && ./PaperCrawlerServerHotPlug.exe"
    exit 1
fi
echo ""

# 检查PDF目录
echo "2. 检查PDF目录..."
if [ -d "backend/output/pdfs" ]; then
    echo "   ✅ PDF目录存在: backend/output/pdfs"
    pdf_count=$(ls -1 backend/output/pdfs/*.pdf 2>/dev/null | wc -l)
    echo "   📄 PDF文件数量: $pdf_count"
else
    echo "   ❌ PDF目录不存在"
fi
echo ""

# 检查缓存目录
echo "3. 检查缓存目录..."
if [ -d "backend/cache/pdf" ]; then
    echo "   ✅ 缓存目录存在: backend/cache/pdf"
    cache_count=$(ls -1 backend/cache/pdf/*.pdf 2>/dev/null | wc -l)
    echo "   📄 缓存PDF数量: $cache_count"
    cache_size=$(du -sh backend/cache/pdf 2>/dev/null | cut -f1)
    echo "   💾 缓存大小: $cache_size"
else
    echo "   ❌ 缓存目录不存在"
fi
echo ""

# 测试PDF端点
echo "4. 测试PDF API端点..."
echo "   测试文档PDF (ID=1):"
response=$(curl -s -w "\n%{http_code}" "http://localhost:8080/api/latex/documents/1/pdf" -o /dev/null 2>&1)
status_code=$(echo "$response" | tail -n 1)
echo "   HTTP状态码: $status_code"

if [ "$status_code" = "200" ]; then
    echo "   ✅ 文档PDF下载成功"
elif [ "$status_code" = "404" ]; then
    echo "   ⚠️  文档不存在 (正常，需要先创建)"
elif [ "$status_code" = "500" ]; then
    echo "   ❌ 服务器错误，检查后端日志"
else
    echo "   ⚠️  意外状态码: $status_code"
fi
echo ""

# 测试调试端点
echo "5. 获取PDF调试信息..."
debug_info=$(curl -s "http://localhost:8080/api/latex/debug/pdf/document/1" 2>/dev/null)
if [ -n "$debug_info" ]; then
    echo "   📋 调试信息:"
    echo "$debug_info" | jq '.' 2>/dev/null || echo "$debug_info"
fi
echo ""

# 检查LaTeX编译工具
echo "6. 检查LaTeX编译工具..."
if command -v xelatex &> /dev/null; then
    echo "   ✅ xelatex: $(which xelatex)"
elif command -v pdflatex &> /dev/null; then
    echo "   ✅ pdflatex: $(which pdflatex)"
else
    echo "   ❌ 未找到LaTeX编译工具"
    echo "   请安装: sudo apt-get install texlive-full"
fi
echo ""

# 显示后端日志
echo "7. 最近的后端日志 (PDF相关):"
if [ -f "backend/output/logs/api.log" ]; then
    echo "   PDF相关日志:"
    grep -i "pdf\|latex\|compile" backend/output/logs/api.log | tail -n 5 2>/dev/null || echo "   (无PDF相关日志)"
else
    echo "   ⚠️  日志文件不存在: backend/output/logs/api.log"
fi
echo ""

echo "==================================="
echo "诊断完成"
echo "==================================="
echo ""
echo "💡 常见解决方案:"
echo "   1. 如果文档不存在，先创建文档"
echo "   2. 如果500错误，检查后端日志获取详细错误"
echo "   3. 如果LaTeX未安装，安装 texlive-full"
echo "   4. 浏览器访问 http://<局域网IP>:5173 (不要用localhost)"
