# AI PDF解析模块实现指南

**版本**: v1.0.0
**日期**: 2026-03-22
**状态**: 实现指南

---

## 📚 目录

1. [环境搭建](#环境搭建)
2. [后端实现](#后端实现)
3. [前端实现](#前端实现)
4. [测试策略](#测试策略)
5. [部署方案](#部署方案)

---

## 🚀 环境搭建

### 1. Python环境配置

```bash
# 创建虚拟环境
cd backend
python -m venv venv_pdf
source venv_pdf/bin/activate  # Linux/Mac
# 或
venv_pdf\Scripts\activate  # Windows

# 安装依赖
pip install -r requirements_pdf.txt
```

#### requirements_pdf.txt

```txt
# PDF解析
PyMuPDF==1.23.8
pdfplumber==0.10.3
pdfminer.six==20231228
PyPDF2==3.0.1

# 公式识别
pix2tex==0.1.1
pillow==10.1.0

# Web框架
fastapi==0.109.0
uvicorn[standard]==0.27.0
python-multipart==0.0.6

# 数据库
pymysql==1.1.0
redis==5.0.1
sqlalchemy==2.0.25

# 任务队列
celery==5.3.4

# AI集成
anthropic==0.18.0
aiohttp==3.9.1

# 工具
python-dotenv==1.0.0
pydantic==2.5.3
pydantic-settings==2.1.0

# 导出
markdown==3.5.1
python-docx==1.1.0
```

---

### 2. 配置文件

#### config_pdf.yaml

```yaml
# PDF解析服务配置
server:
  host: "127.0.0.1"
  port: 8001
  workers: 4

# PDF处理
pdf:
  max_file_size: 104857600  # 100MB
  upload_path: "/var/papercrawler/pdf/uploads"
  parsed_path: "/var/papercrawler/pdf/parsed"
  export_path: "/var/papercrawler/pdf/exports"

# Claude API
anthropic:
  api_key: "${ANTHROPIC_API_KEY}"
  max_tokens: 4096
  timeout: 60

# Redis
redis:
  host: "localhost"
  port: 6379
  db: 0
  password: null

# MySQL
mysql:
  host: "localhost"
  port: 3306
  user: "root"
  password: "${DB_PASSWORD}"
  database: "papercrawler"

# Celery
celery:
  broker: "redis://localhost:6379/1"
  backend: "redis://localhost:6379/2"

# 用户配额
quotas:
  free:
    max_file_size: 10485760  # 10MB
    max_files_per_month: 10
    max_tokens_per_month: 100000

  premium:
    max_file_size: 104857600  # 100MB
    max_files_per_month: 100
    max_tokens_per_month: 1000000
```

---

## 🔧 后端实现

### 1. FastAPI主服务

#### main.py

```python
from fastapi import FastAPI, UploadFile, File, HTTPException, Depends
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import StreamingResponse
from contextlib import asynccontextmanager
import uvicorn

from services.pdf_processor import PDFProcessor
from services.claude_service import ClaudeService
from services.cache_manager import CacheManager
from api.routes import pdf_router

@asynccontextmanager
async def lifespan(app: FastAPI):
    """应用生命周期管理"""
    # 启动时初始化
    print("启动PDF解析服务...")
    yield
    # 关闭时清理
    print("关闭PDF解析服务...")

app = FastAPI(
    title="PaperCrawler PDF API",
    description="AI驱动的PDF解析服务",
    version="1.0.0",
    lifespan=lifespan
)

# CORS配置
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173", "http://localhost:8080"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# 注册路由
app.include_router(pdf_router, prefix="/api/pdf", tags=["PDF"])

@app.get("/health")
async def health_check():
    """健康检查"""
    return {
        "status": "healthy",
        "service": "PDF Parser",
        "version": "1.0.0"
    }

if __name__ == "__main__":
    uvicorn.run(
        "main:app",
        host="127.0.0.1",
        port=8001,
        reload=True,
        workers=1
    )
```

---

### 2. PDF处理服务

#### services/pdf_processor.py

```python
import fitz  # PyMuPDF
import pdfplumber
import json
import os
from pathlib import Path
from typing import Dict, List, Any
from PIL import Image
import io

class PDFProcessor:
    """PDF处理器 - 混合引擎策略"""

    def __init__(self, config: Dict):
        self.upload_path = config["pdf"]["upload_path"]
        self.parsed_path = config["pdf"]["parsed_path"]

        # 确保目录存在
        os.makedirs(self.upload_path, exist_ok=True)
        os.makedirs(self.parsed_path, exist_ok=True)

    async def process_pdf(self, task_id: str, file_path: str) -> Dict[str, Any]:
        """
        处理PDF文件
        返回: 解析结果
        """
        result = {
            "task_id": task_id,
            "pages": [],
            "metadata": {},
            "tables": [],
            "figures": [],
            "formulas": [],
            "statistics": {}
        }

        try:
            # 1. 提取元数据和基础信息
            doc = fitz.open(file_path)
            result["metadata"] = self._extract_metadata(doc)

            # 2. 逐页处理
            for page_num in range(len(doc)):
                page_data = await self._process_page(doc, page_num, task_id)
                result["pages"].append(page_data)

            # 3. 提取表格（使用pdfplumber）
            result["tables"] = await self._extract_tables(file_path)

            # 4. 计算统计信息
            result["statistics"] = self._calculate_statistics(result)

            doc.close()

            # 5. 保存解析结果
            await self._save_result(task_id, result)

            return result

        except Exception as e:
            raise Exception(f"PDF处理失败: {str(e)}")

    def _extract_metadata(self, doc: fitz.Document) -> Dict:
        """提取PDF元数据"""
        metadata = doc.metadata
        return {
            "title": metadata.get("title", ""),
            "author": metadata.get("author", ""),
            "subject": metadata.get("subject", ""),
            "keywords": metadata.get("keywords", ""),
            "creator": metadata.get("creator", ""),
            "producer": metadata.get("producer", ""),
            "page_count": len(doc),
            "encrypted": doc.isEncrypted
        }

    async def _process_page(self, doc: fitz.Document, page_num: int, task_id: str) -> Dict:
        """处理单个页面"""
        page = doc[page_num]
        page_data = {
            "page_number": page_num + 1,
            "text": "",
            "formulas": [],
            "figures": [],
            "size": page.rect
        }

        # 1. 提取文本
        page_data["text"] = page.get_text("text")

        # 2. 提取图像
        image_list = page.get_images()
        for img_index, img in enumerate(image_list):
            xref = img[0]
            base_image = doc.extract_image(xref)

            if base_image:
                image_bytes = base_image["image"]
                image_ext = base_image["ext"]

                # 保存图像
                image_filename = f"{task_id}_p{page_num + 1}_img{img_index + 1}.{image_ext}"
                image_path = os.path.join(self.parsed_path, task_id, image_filename)

                os.makedirs(os.path.dirname(image_path), exist_ok=True)

                with open(image_path, "wb") as f:
                    f.write(image_bytes)

                page_data["figures"].append({
                    "filename": image_filename,
                    "path": image_path,
                    "xref": xref,
                    "bbox": page.get_image_bbox(img)
                })

        # 3. 检测公式（简单启发式）
        page_data["formulas"] = self._detect_formulas(page)

        return page_data

    def _detect_formulas(self, page: fitz.Page) -> List[Dict]:
        """检测页面中的公式"""
        formulas = []

        # 简单启发式：查找LaTeX模式
        text = page.get_text("text")
        lines = text.split('\n')

        for i, line in enumerate(lines):
            # 检测LaTeX公式标记
            if any(marker in line for marker in ['$$', '\\(', '\\[', '\\begin{equation']):
                formulas.append({
                    "line": i + 1,
                    "content": line.strip(),
                    "type": "latex"
                })

        return formulas

    async def _extract_tables(self, file_path: str) -> List[Dict]:
        """提取表格"""
        tables = []

        with pdfplumber.open(file_path) as pdf:
            for page_num, page in enumerate(pdf.pages):
                table_extraction = page.extract_tables()

                if table_extraction:
                    for table_idx, table in enumerate(table_extraction):
                        if table:
                            tables.append({
                                "page": page_num + 1,
                                "index": table_idx + 1,
                                "rows": len(table),
                                "cols": len(table[0]) if table else 0,
                                "data": table
                            })

        return tables

    def _calculate_statistics(self, result: Dict) -> Dict:
        """计算统计信息"""
        total_text = sum(len(page["text"]) for page in result["pages"])
        total_formulas = sum(len(page["formulas"]) for page in result["pages"])
        total_figures = sum(len(page["figures"]) for page in result["pages"])

        return {
            "total_pages": len(result["pages"]),
            "total_text_length": total_text,
            "total_formulas": total_formulas,
            "total_figures": total_figures,
            "total_tables": len(result["tables"])
        }

    async def _save_result(self, task_id: str, result: Dict):
        """保存解析结果"""
        output_path = os.path.join(self.parsed_path, f"{task_id}.json")

        os.makedirs(os.path.dirname(output_path), exist_ok=True)

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(result, f, ensure_ascii=False, indent=2)
```

---

### 3. Claude AI服务

#### services/claude_service.py

```python
import anthropic
import asyncio
import aiohttp
import json
from typing import AsyncIterator, Dict, Any
import hashlib

class ClaudeService:
    """Claude API集成服务"""

    def __init__(self, config: Dict):
        self.client = anthropic.AsyncAnthropic(
            api_key=config["anthropic"]["api_key"]
        )
        self.max_tokens = config["anthropic"]["max_tokens"]

    async def summarize_pdf(
        self,
        content: str,
        style: str = "academic",
        language: str = "zh-CN"
    ) -> AsyncIterator[Dict[str, Any]]:
        """
        生成PDF总结
        返回: 流式响应
        """
        system_prompt = self._build_system_prompt()
        user_prompt = self._build_summary_prompt(content, style, language)

        try:
            async with self.client.messages.stream(
                model="claude-3-5-sonnet-20241022",
                max_tokens=self.max_tokens,
                system=system_prompt,
                messages=[{"role": "user", "content": user_prompt}]
            ) as stream:

                async for text in stream.text_stream:
                    yield {
                        "type": "content",
                        "content": text
                    }

                # 获取最终统计
                response = await stream.get_final_message()
                yield {
                    "type": "done",
                    "tokens": response.usage.input_tokens + response.usage.output_tokens,
                    "cost": self._calculate_cost(response.usage)
                }

        except Exception as e:
            yield {
                "type": "error",
                "message": str(e)
            }

    async def chat(
        self,
        question: str,
        pdf_content: str,
        chat_history: List[Dict] = None
    ) -> AsyncIterator[Dict[str, Any]]:
        """
        AI问答
        返回: 流式响应
        """
        system_prompt = self._build_system_prompt()
        user_prompt = self._build_chat_prompt(question, pdf_content, chat_history)

        try:
            async with self.client.messages.stream(
                model="claude-3-5-sonnet-20241022",
                max_tokens=self.max_tokens,
                system=system_prompt,
                messages=[{"role": "user", "content": user_prompt}]
            ) as stream:

                async for text in stream.text_stream:
                    yield {
                        "type": "content",
                        "content": text
                    }

                response = await stream.get_final_message()
                yield {
                    "type": "done",
                    "tokens": response.usage.input_tokens + response.usage.output_tokens,
                    "cost": self._calculate_cost(response.usage)
                }

        except Exception as e:
            yield {
                "type": "error",
                "message": str(e)
            }

    def _build_system_prompt(self) -> str:
        """构建系统提示词"""
        return """你是一位专业的学术文档分析助手。

# 核心能力
- 准确理解学术文档内容
- 提取关键信息（方法、结果、结论）
- 生成结构化摘要
- 回答文档相关问题

# 输出格式
- 使用Markdown格式
- 结构清晰，层次分明
- 准确引用原文
- 标注图表和公式

# 语言风格
- 学术化、专业化
- 简洁明了
- 客观中立
- 避免冗复"""

    def _build_summary_prompt(self, content: str, style: str, language: str) -> str:
        """构建总结提示词"""
        # 截断过长的内容（保留前20000字符）
        truncated_content = content[:20000] if len(content) > 20000 else content

        return f"""请为以下学术论文生成{style}风格的摘要。

# 文档内容
{truncated_content}

# 要求
1. 语言: {language}
2. 风格: {style}
3. 包含部分: 摘要、研究方法、主要结果、结论、关键词

# 输出格式
## 摘要
[200-300字概述]

## 研究背景
[研究背景和动机]

## 研究方法
[方法论概述]

## 主要结果
[关键发现]

## 结论与创新点
[主要贡献]

## 关键词
[3-5个关键词]"""

    def _build_chat_prompt(self, question: str, pdf_content: str, chat_history: List[Dict]) -> str:
        """构建问答提示词"""
        # 截断上下文
        truncated_content = pdf_content[:15000] if len(pdf_content) > 15000 else pdf_content

        prompt = f"""基于以下学术论文内容回答问题。

# 文档内容
{truncated_content}

# 用户问题
{question}

# 回答要求
1. 准确基于文档内容
2. 引用具体章节
3. 使用Markdown格式
4. 可包含公式和图表引用

## 答案
"""

        # 添加对话历史（最近3轮）
        if chat_history:
            history = chat_history[-6:]  # 最近3轮（用户+助手）
            prompt += "\n\n# 对话历史\n"
            for msg in history:
                role = "用户" if msg["role"] == "user" else "助手"
                prompt += f"{role}: {msg['content']}\n"

        return prompt

    def _calculate_cost(self, usage: Any) -> float:
        """计算成本（美元）"""
        # Claude Sonnet定价: 输入 $3/1M tokens, 输出 $15/1M tokens
        input_cost = usage.input_tokens * 3.0 / 1_000_000
        output_cost = usage.output_tokens * 15.0 / 1_000_000
        return input_cost + output_cost
```

---

### 4. API路由

#### api/routes/pdf_router.py

```python
from fastapi import APIRouter, UploadFile, File, HTTPException, Depends, Header
from fastapi.responses import StreamingResponse
from typing import Optional
import uuid
import os

from services.pdf_processor import PDFProcessor
from services.claude_service import ClaudeService
from services.cache_manager import CacheManager
from services.auth_service import verify_token
from models.schemas import (
    UploadResponse,
    TaskStatusResponse,
    SummarizeRequest,
    ChatRequest
)

router = APIRouter()

# 依赖注入
async def get_pdf_processor():
    return PDFProcessor(config)

async def get_claude_service():
    return ClaudeService(config)

async def get_cache_manager():
    return CacheManager(config)

@router.post("/upload", response_model=UploadResponse)
async def upload_pdf(
    file: UploadFile = File(...),
    title: Optional[str] = None,
    tags: Optional[str] = None,
    auto_summarize: bool = False,
    authorization: str = Header(...),
    pdf_processor: PDFProcessor = Depends(get_pdf_processor),
    claude_service: ClaudeService = Depends(get_claude_service)
):
    """上传PDF文件"""
    # 1. 验证JWT
    user = await verify_token(authorization)

    # 2. 权限检查
    if file.size > user.max_file_size:
        raise HTTPException(400, f"文件大小超过限制 ({user.max_file_size / 1024 / 1024}MB)")

    # 3. 生成任务ID
    task_id = str(uuid.uuid4())

    # 4. 保存文件
    file_path = os.path.join(config["pdf"]["upload_path"], f"{task_id}.pdf")
    with open(file_path, "wb") as f:
        content = await file.read()
        f.write(content)

    # 5. 创建数据库记录
    await create_pdf_task(
        task_id=task_id,
        user_id=user.id,
        filename=file.filename,
        file_path=file_path,
        file_size=len(content),
        title=title or file.filename,
        tags=tags.split(",") if tags else []
    )

    # 6. 异步处理
    asyncio.create_task(process_pdf_async(task_id, file_path))

    return UploadResponse(
        task_id=task_id,
        filename=file.filename,
        file_size=len(content),
        status="processing",
        estimated_time=30
    )

async def process_pdf_async(task_id: str, file_path: str):
    """异步处理PDF"""
    try:
        # 更新状态: 解析中
        await update_task_status(task_id, "parsing", 10)

        # 解析PDF
        pdf_processor = PDFProcessor(config)
        result = await pdf_processor.process_pdf(task_id, file_path)

        # 更新状态: 分析完成
        await update_task_status(task_id, "analyzing", 70)

        # 保存到数据库
        await save_pdf_result(task_id, result)

        # 更新状态: 完成
        await update_task_status(task_id, "completed", 100)

    except Exception as e:
        await update_task_status(task_id, "failed", 0, error_message=str(e))

@router.get("/tasks/{task_id}", response_model=TaskStatusResponse)
async def get_task_status(
    task_id: str,
    authorization: str = Header(...),
    cache_manager: CacheManager = Depends(get_cache_manager)
):
    """获取任务状态"""
    # 验证JWT
    user = await verify_token(authorization)

    # 检查缓存
    cached = await cache_manager.get_task_status(task_id)
    if cached:
        return cached

    # 查询数据库
    task = await get_task_from_db(task_id)

    if not task or task.user_id != user.id:
        raise HTTPException(404, "任务不存在")

    response = TaskStatusResponse(
        task_id=task.task_id,
        status=task.status,
        progress=task.progress,
        result=task.result,
        summary=task.summary,
        created_at=task.created_at,
        completed_at=task.completed_at
    )

    # 缓存结果
    await cache_manager.cache_task_status(task_id, response)

    return response

@router.post("/summarize/{task_id}")
async def summarize_pdf(
    task_id: str,
    request: SummarizeRequest,
    authorization: str = Header(...),
    claude_service: ClaudeService = Depends(get_claude_service)
):
    """生成AI总结（流式）"""
    # 验证JWT
    user = await verify_token(authorization)

    # 获取任务
    task = await get_task_from_db(task_id)
    if not task or task.user_id != user.id:
        raise HTTPException(404, "任务不存在")

    # 检查权限
    if not user.ai_enabled:
        raise HTTPException(403, "AI功能需要VIP权限")

    # 检查令牌预算
    if not await check_token_budget(user.id, 50000):
        raise HTTPException(403, "令牌额度不足")

    # 获取PDF内容
    pdf_content = await get_pdf_content(task_id)

    # 流式响应
    async def generate():
        full_summary = ""
        async for chunk in claude_service.summarize_pdf(
            pdf_content,
            request.style,
            request.language
        ):
            if chunk["type"] == "content":
                full_summary += chunk["content"]
                yield f"data: {json.dumps(chunk)}\n\n"

            elif chunk["type"] == "done":
                # 保存总结
                await save_summary(task_id, full_summary)

                # 记录令牌使用
                await record_token_usage(user.id, chunk["tokens"], chunk["cost"])

                yield f"data: {json.dumps(chunk)}\n\n"

    return StreamingResponse(generate(), media_type="text/event-stream")

@router.post("/chat/{task_id}")
async def chat_with_pdf(
    task_id: str,
    request: ChatRequest,
    authorization: str = Header(...),
    claude_service: ClaudeService = Depends(get_claude_service),
    cache_manager: CacheManager = Depends(get_cache_manager)
):
    """AI问答（流式）"""
    # 验证JWT
    user = await verify_token(authorization)

    # 获取任务
    task = await get_task_from_db(task_id)
    if not task or task.user_id != user.id:
        raise HTTPException(404, "任务不存在")

    # 检查缓存
    cache_key = f"chat:{task_id}:{hashlib.md5(request.question.encode()).hexdigest()}"
    cached_response = await cache_manager.get(cache_key)
    if cached_response:
        async def return_cached():
            yield f"data: {json.dumps({'type': 'content', 'content': cached_response})}\n\n"
            yield f"data: {json.dumps({'type': 'done', 'cached': True})}\n\n"
        return StreamingResponse(return_cached(), media_type="text/event-stream")

    # 获取PDF内容
    pdf_content = await get_pdf_content(task_id)
    chat_history = await get_chat_history(task_id)

    # 流式响应
    async def generate():
        full_response = ""
        async for chunk in claude_service.chat(
            request.question,
            pdf_content,
            chat_history
        ):
            if chunk["type"] == "content":
                full_response += chunk["content"]
                yield f"data: {json.dumps(chunk)}\n\n"

            elif chunk["type"] == "done":
                # 缓存响应
                await cache_manager.set(cache_key, full_response, ex=86400)

                # 保存对话
                await save_chat_message(task_id, user.id, "user", request.question)
                await save_chat_message(
                    task_id, user.id, "assistant", full_response,
                    chunk["tokens"], chunk["cost"]
                )

                yield f"data: {json.dumps(chunk)}\n\n"

    return StreamingResponse(generate(), media_type="text/event-stream")
```

---

## 🎨 前端实现

### 1. Vue组件

#### components/PDFUploader.vue

```vue
<template>
  <div class="pdf-uploader">
    <el-upload
      ref="uploadRef"
      class="upload-demo"
      drag
      :action="uploadUrl"
      :headers="uploadHeaders"
      :before-upload="beforeUpload"
      :on-success="onSuccess"
      :on-error="onError"
      :show-file-list="false"
      accept=".pdf"
    >
      <el-icon class="el-icon--upload"><upload-filled /></el-icon>
      <div class="el-upload__text">
        拖拽PDF到此处 或 <em>点击上传</em>
      </div>
      <template #tip>
        <div class="el-upload__tip">
          支持PDF格式，最大{{ uploadLimit }}MB
        </div>
      </template>
    </el-upload>

    <div v-if="uploadProgress > 0" class="upload-progress">
      <el-progress :percentage="uploadProgress" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { ElMessage } from 'element-plus'
import { UploadFilled } from '@element-plus/icons-vue'
import { useAuthStore } from '@/stores/auth'
import { uploadPDF } from '@/api/modules/pdf'

const authStore = useAuthStore()
const uploadRef = ref()
const uploadProgress = ref(0)

const uploadUrl = computed(() => `${import.meta.env.VITE_API_URL}/api/pdf/upload`)
const uploadHeaders = computed(() => ({
  Authorization: `Bearer ${authStore.tokens?.accessToken}`
}))

const uploadLimit = computed(() => {
  const tier = authStore.user?.role || 'free'
  const limits = {
    free: 10,
    premium: 100,
    admin: 100
  }
  return limits[tier] || 10
})

const beforeUpload = (file: File) => {
  // 权限检查
  const tier = authStore.user?.role || 'free'
  const maxSize = (tier === 'premium' || tier === 'admin') ? 100 * 1024 * 1024 : 10 * 1024 * 1024

  if (file.size > maxSize) {
    ElMessage.error(`文件大小超过限制 (${maxSize / 1024 / 1024}MB)`)
    return false
  }

  uploadProgress.value = 0
  return true
}

const onSuccess = async (response: any) => {
  uploadProgress.value = 100

  ElMessage.success('上传成功，正在解析...')

  // 跳转到任务详情页
  router.push(`/pdf/tasks/${response.taskId}`)
}

const onError = (error: any) => {
  ElMessage.error(error.message || '上传失败')
  uploadProgress.value = 0
}
</script>

<style scoped>
.pdf-uploader {
  max-width: 600px;
  margin: 0 auto;
}

.upload-progress {
  margin-top: 20px;
}
</style>
```

---

#### components/PDFChat.vue

```vue
<template>
  <div class="pdf-chat">
    <div class="chat-messages" ref="messagesRef">
      <div
        v-for="(msg, index) in chatHistory"
        :key="index"
        :class="['message', msg.role]"
      >
        <div class="message-content">
          <div class="message-text" v-html="renderMarkdown(msg.content)"></div>
          <div v-if="msg.tokens" class="message-meta">
            {{ msg.tokens }} tokens · ${{ msg.cost?.toFixed(4) }}
          </div>
        </div>
      </div>

      <div v-if="streamingMessage" class="message assistant streaming">
        <div class="message-content">
          <div class="message-text" v-html="renderMarkdown(streamingMessage)"></div>
          <el-icon class="loading-icon"><loading /></el-icon>
        </div>
      </div>
    </div>

    <div class="chat-input">
      <el-input
        v-model="questionInput"
        type="textarea"
        :rows="2"
        placeholder="输入您的问题..."
        @keydown.enter.prevent="sendMessage"
        :disabled="loading"
      />
      <el-button
        type="primary"
        @click="sendMessage"
        :loading="loading"
        :disabled="!questionInput.trim()"
      >
        发送
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, nextTick } from 'vue'
import { ElMessage } from 'element-plus'
import { Loading } from '@element-plus/icons-vue'
import { marked } from 'marked'
import { useAuthStore } from '@/stores/auth'

const props = defineProps<{
  taskId: string
}>()

const authStore = useAuthStore()
const chatHistory = ref<any[]>([])
const questionInput = ref('')
const loading = ref(false)
const streamingMessage = ref('')
const messagesRef = ref<HTMLElement>()

const renderMarkdown = (text: string) => {
  return marked(text)
}

const scrollToBottom = () => {
  nextTick(() => {
    if (messagesRef.value) {
      messagesRef.value.scrollTop = messagesRef.value.scrollHeight
    }
  })
}

const sendMessage = async () => {
  if (!questionInput.value.trim() || loading.value) return

  const question = questionInput.value
  questionInput.value = ''

  // 添加用户消息
  chatHistory.value.push({
    role: 'user',
    content: question,
    timestamp: Date.now()
  })

  loading.value = true
  scrollToBottom()

  try {
    const response = await fetch(`${import.meta.env.VITE_API_URL}/api/pdf/chat/${props.taskId}`, {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${authStore.tokens?.accessToken}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({ question, stream: true })
    })

    if (!response.ok) {
      throw new Error('请求失败')
    }

    const reader = response.body!.getReader()
    const decoder = new TextDecoder()

    streamingMessage.value = ''

    while (true) {
      const { done, value } = await reader.read()
      if (done) break

      const chunk = decoder.decode(value)
      const lines = chunk.split('\n')

      for (const line of lines) {
        if (line.startsWith('data: ')) {
          try {
            const data = JSON.parse(line.slice(6))

            if (data.type === 'content') {
              streamingMessage.value += data.content
              scrollToBottom()
            } else if (data.type === 'done') {
              chatHistory.value.push({
                role: 'assistant',
                content: streamingMessage.value,
                tokens: data.tokens,
                cost: data.cost,
                timestamp: Date.now()
              })
              streamingMessage.value = ''

              // 更新用户令牌使用
              if (authStore.user) {
                authStore.user.monthlyTokenUsage =
                  (authStore.user.monthlyTokenUsage || 0) + data.tokens
              }
            } else if (data.type === 'error') {
              ElMessage.error(data.message)
            }
          } catch (e) {
            console.error('解析SSE数据失败:', e)
          }
        }
      }
    }

  } catch (error: any) {
    ElMessage.error(error.message || '发送失败')
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
.pdf-chat {
  display: flex;
  flex-direction: column;
  height: 600px;
}

.chat-messages {
  flex: 1;
  overflow-y: auto;
  padding: 20px;
}

.message {
  margin-bottom: 16px;
  display: flex;
}

.message.user {
  justify-content: flex-end;
}

.message.assistant {
  justify-content: flex-start;
}

.message-content {
  max-width: 70%;
  padding: 12px 16px;
  border-radius: 8px;
}

.message.user .message-content {
  background-color: #409eff;
  color: white;
}

.message.assistant .message-content {
  background-color: #f0f0f0;
  color: #333;
}

.message-text {
  word-wrap: break-word;
}

.message-meta {
  font-size: 12px;
  opacity: 0.7;
  margin-top: 8px;
}

.chat-input {
  padding: 16px;
  border-top: 1px solid #e0e0e0;
  display: flex;
  gap: 8px;
}

.chat-input .el-input {
  flex: 1;
}
</style>
```

---

### 2. API模块

#### api/modules/pdf.ts

```typescript
import request from '@/utils/request'
import type { UploadFile } from 'element-plus'

export interface PDFTask {
  taskId: string
  filename: string
  fileSize: number
  status: 'pending' | 'processing' | 'completed' | 'failed'
  progress: number
  summary?: string
  createdAt: number
  completedAt?: number
}

export interface ChatMessage {
  role: 'user' | 'assistant'
  content: string
  tokens?: number
  cost?: number
  timestamp: number
}

export async function uploadPDF(file: File, options?: {
  title?: string
  tags?: string[]
  autoSummarize?: boolean
}) {
  const formData = new FormData()
  formData.append('file', file)
  if (options?.title) formData.append('title', options.title)
  if (options?.tags) formData.append('tags', JSON.stringify(options.tags))
  if (options?.autoSummarize) formData.append('auto_summarize', 'true')

  return request.post<PDFTask>('/api/pdf/upload', formData, {
    headers: { 'Content-Type': 'multipart/form-data' },
    timeout: 60000
  })
}

export async function getTaskStatus(taskId: string) {
  return request.get<PDFTask>(`/api/pdf/tasks/${taskId}`)
}

export async function getTaskList(params?: {
  offset?: number
  limit?: number
  status?: string
}) {
  return request.get<{ tasks: PDFTask[], total: number }>('/api/pdf/tasks', { params })
}

export async function summarizePDF(taskId: string, options?: {
  style?: 'academic' | 'brief' | 'detailed'
  language?: 'zh-CN' | 'en-US'
}) {
  return request.post(`/api/pdf/summarize/${taskId}`, options)
}

export async function exportPDF(taskId: string, format: 'markdown' | 'json' | 'docx' | 'txt') {
  return request.get(`/api/pdf/export/${taskId}`, {
    params: { format },
    responseType: 'blob'
  })
}
```

---

## 🧪 测试策略

### 1. 单元测试

#### tests/test_pdf_processor.py

```python
import pytest
from services.pdf_processor import PDFProcessor

@pytest.fixture
def pdf_processor():
    config = {
        "pdf": {
            "upload_path": "/tmp/uploads",
            "parsed_path": "/tmp/parsed"
        }
    }
    return PDFProcessor(config)

@pytest.mark.asyncio
async def test_process_pdf(pdf_processor):
    """测试PDF处理"""
    # 使用测试PDF文件
    result = await pdf_processor.process_pdf(
        task_id="test-123",
        file_path="tests/fixtures/sample.pdf"
    )

    assert result["task_id"] == "test-123"
    assert len(result["pages"]) > 0
    assert result["statistics"]["total_pages"] > 0
    assert result["statistics"]["total_text_length"] > 0

def test_extract_metadata(pdf_processor):
    """测试元数据提取"""
    import fitz
    doc = fitz.open("tests/fixtures/sample.pdf")
    metadata = pdf_processor._extract_metadata(doc)

    assert "page_count" in metadata
    assert metadata["page_count"] > 0

    doc.close()
```

---

### 2. 集成测试

#### tests/test_api_integration.py

```python
import pytest
from fastapi.testclient import TestClient
from main import app

client = TestClient(app)

def test_upload_pdf():
    """测试PDF上传"""
    with open("tests/fixtures/sample.pdf", "rb") as f:
        response = client.post(
            "/api/pdf/upload",
            files={"file": ("sample.pdf", f, "application/pdf")},
            headers={"Authorization": "Bearer test-token"}
        )

    assert response.status_code == 200
    data = response.json()
    assert "taskId" in data
    assert data["status"] == "processing"

def test_get_task_status():
    """测试获取任务状态"""
    response = client.get(
        "/api/pdf/tasks/test-123",
        headers={"Authorization": "Bearer test-token"}
    )

    assert response.status_code == 200
    data = response.json()
    assert "status" in data
```

---

### 3. 负载测试

#### tests/load_test.py

```python
import asyncio
import aiohttp
import time

async def upload_pdf(session, file_path, token):
    """异步上传PDF"""
    with open(file_path, 'rb') as f:
        async with session.post(
            "http://localhost:8001/api/pdf/upload",
            data={"file": f},
            headers={"Authorization": f"Bearer {token}"}
        ) as response:
            return await response.json()

async def load_test(num_requests=100):
    """负载测试"""
    async with aiohttp.ClientSession() as session:
        start_time = time.time()

        tasks = [
            upload_pdf(session, "tests/fixtures/sample.pdf", "test-token")
            for _ in range(num_requests)
        ]

        results = await asyncio.gather(*tasks)

        end_time = time.time()
        duration = end_time - start_time

        print(f"完成 {num_requests} 个请求")
        print(f"总耗时: {duration:.2f}秒")
        print(f"平均耗时: {duration / num_requests:.2f}秒/请求")
        print(f"吞吐量: {num_requests / duration:.2f} 请求/秒")

if __name__ == "__main__":
    asyncio.run(load_test(100))
```

---

## 🚀 部署方案

### 1. Docker部署

#### Dockerfile

```dockerfile
FROM python:3.11-slim

WORKDIR /app

# 安装系统依赖
RUN apt-get update && apt-get install -y \
    libpoppler-cpp-dev \
    tesseract-ocr \
    tesseract-ocr-chi-sim \
    && rm -rf /var/lib/apt/lists/*

# 安装Python依赖
COPY requirements_pdf.txt .
RUN pip install --no-cache-dir -r requirements_pdf.txt

# 复制代码
COPY . .

# 创建目录
RUN mkdir -p /var/papercrawler/pdf/{uploads,parsed,exports}

# 暴露端口
EXPOSE 8001

# 启动命令
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8001", "--workers", "4"]
```

---

#### docker-compose.yml

```yaml
version: '3.8'

services:
  pdf-service:
    build: .
    ports:
      - "8001:8001"
    environment:
      - ANTHROPIC_API_KEY=${ANTHROPIC_API_KEY}
      - DB_PASSWORD=${DB_PASSWORD}
    volumes:
      - ./data/pdf:/var/papercrawler/pdf
    depends_on:
      - redis
      - mysql
    restart: unless-stopped

  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"
    volumes:
      - redis_data:/data
    restart: unless-stopped

  mysql:
    image: mysql:8.0
    environment:
      - MYSQL_ROOT_PASSWORD=${DB_PASSWORD}
      - MYSQL_DATABASE=papercrawler
    volumes:
      - mysql_data:/var/lib/mysql
    restart: unless-stopped

  celery-worker:
    build: .
    command: celery -A tasks worker --loglevel=info
    environment:
      - ANTHROPIC_API_KEY=${ANTHROPIC_API_KEY}
    depends_on:
      - redis
    restart: unless-stopped

volumes:
  redis_data:
  mysql_data:
```

---

### 2. 生产部署检查清单

- [ ] 配置环境变量
- [ ] 设置数据库迁移
- [ ] 配置HTTPS证书
- [ ] 设置日志轮转
- [ ] 配置监控告警
- [ ] 设置自动备份
- [ ] 配置负载均衡
- [ ] 压力测试
- [ ] 成本监控设置
- [ ] 用户配额配置

---

**文档版本**: v1.0.0
**最后更新**: 2026-03-22
**作者**: Claude (AI Engineer Agent)
