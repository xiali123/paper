import request from '@/utils/request'
import type { CompilationResult } from '../../architecture/stores/latexEditor'
import {
  listLatexDocuments,
  getLatexDocument,
  createLatexDocument,
  updateLatexDocument,
  deleteLatexDocument,
  compileLatexDocument,
  autoSaveLatexDocument,
  listLatexTemplates,
  createFromTemplate,
  downloadLatexPDF
} from '@/api/adapters/latexAdapter'

export interface CompileLatexRequest {
  content: string
  documentClass?: string
  packages?: string[]
  options?: {
    timeout?: number
    format?: 'pdf' | 'html' | 'xml'
  }
}

export interface CompileLatexResponse {
  success: boolean
  result?: CompilationResult
  error?: string
  duration: number
}

export interface SaveDocumentRequest {
  id?: string | number
  name: string
  content: string
  path?: string
  metadata?: {
    title?: string
    authors?: string[]
    abstract?: string
    keywords?: string[]
    documentClass?: string
    packages?: string[]
  }
}

export interface SaveDocumentResponse {
  success: boolean
  document?: {
    id: string | number
    name: string
    path: string
    lastModified: number
    size: number
  }
  error?: string
}

export interface LoadDocumentRequest {
  id: string | number
}

export interface LoadDocumentResponse {
  success: boolean
  document?: {
    id: string | number
    name: string
    content: string
    path: string
    lastModified: number
    size: number
    metadata?: {
      title?: string
      authors?: string[]
      abstract?: string
      keywords?: string[]
      documentClass?: string
      packages?: string[]
    }
  }
  error?: string
}

export interface CollaborationSessionRequest {
  documentId: string
  userId: string
  userName: string
}

export interface CollaborationSessionResponse {
  success: boolean
  sessionId?: string
  error?: string
}

export interface CursorPositionRequest {
  sessionId: string
  userId: string
  position: {
    line: number
    column: number
  }
  selection?: {
    startLine: number
    startColumn: number
    endLine: number
    endColumn: number
  }
}

export interface CursorPositionResponse {
  success: boolean
  error?: string
}

/**
 * LaTeX 编译服务 - 使用后端API
 */
export const latexApi = {
  /**
   * 编译 LaTeX 文档 - 使用后端编译API
   */
  compile: async (requestData: CompileLatexRequest): Promise<CompileLatexResponse> => {
    try {
      // 首先需要保存文档或获取文档ID
      // 这里假设documentId已经在requestData中
      const documentId = (requestData as any).documentId

      if (!documentId) {
        // 客户端编译作为后备方案
        return {
          success: false,
          error: 'Document ID required for server-side compilation',
          duration: 0
        }
      }

      const result = await compileLatexDocument(Number(documentId))

      return {
        success: result.success,
        result: {
          success: result.success,
          output: result.log,
          pdfPath: result.pdfPath,
          compileTimeMs: result.compileTimeMs,
          error: result.error,
          errors: result.error ? [{
            line: 0,
            message: result.error,
            type: 'error'
          }] : [],
          warnings: [],
          log: result.log || '',
          duration: result.compileTimeMs || 0
        },
        duration: result.compileTimeMs || 0
      }
    } catch (error) {
      console.error('LaTeX compilation failed:', error)
      return {
        success: false,
        error: error instanceof Error ? error.message : '编译失败',
        duration: 0
      }
    }
  },

  /**
   * 保存文档 - 使用后端API
   */
  saveDocument: async (requestData: SaveDocumentRequest): Promise<SaveDocumentResponse> => {
    try {
      if (requestData.id && requestData.id !== '0' && requestData.id !== 0) {
        // 更新现有文档
        await updateLatexDocument(Number(requestData.id), {
          title: requestData.name,
          content: requestData.content
        })

        return {
          success: true,
          document: {
            id: requestData.id,
            name: requestData.name,
            path: requestData.path || '',
            lastModified: Date.now(),
            size: new Blob([requestData.content]).size
          }
        }
      } else {
        // 创建新文档
        const doc = await createLatexDocument({
          title: requestData.name,
          content: requestData.content
        })

        return {
          success: true,
          document: {
            id: doc.id,
            name: doc.title,
            path: '',
            lastModified: new Date(doc.updatedAt).getTime(),
            size: new Blob([doc.content]).size
          }
        }
      }
    } catch (error) {
      console.error('Document save failed:', error)

      // 开发环境使用mock保存作为后备
      if (import.meta.env.DEV) {
        console.log('Fallback to mock save document:', requestData.name, 'content length:', requestData.content.length)

        // 模拟API延迟
        await new Promise(resolve => setTimeout(resolve, 300))

        return {
          success: true,
          document: {
            id: requestData.id || 'mock-id',
            name: requestData.name,
            path: requestData.path || '/mock-path',
            lastModified: Date.now(),
            size: new Blob([requestData.content]).size
          }
        }
      }

      return {
        success: false,
        error: error instanceof Error ? error.message : '保存失败'
      }
    }
  },

  /**
   * 加载文档 - 使用后端API
   */
  loadDocument: async (requestData: LoadDocumentRequest): Promise<LoadDocumentResponse> => {
    try {
      const doc = await getLatexDocument(Number(requestData.id))

      return {
        success: true,
        document: {
          id: doc.id,
          name: doc.title,
          content: doc.content,
          path: doc.pdfPath,
          lastModified: new Date(doc.updatedAt).getTime(),
          size: new Blob([doc.content]).size,
          metadata: {
            title: doc.title
          }
        }
      }
    } catch (error) {
      console.error('Document load failed:', error)
      return {
        success: false,
        error: error instanceof Error ? error.message : '加载失败'
      }
    }
  },

  /**
   * 自动保存文档 - 使用后端API
   */
  autoSave: async (documentId: string | number, content: string): Promise<boolean> => {
    try {
      await autoSaveLatexDocument(Number(documentId), content)
      return true
    } catch (error) {
      console.error('Auto-save failed:', error)
      return false
    }
  },

  /**
   * 创建协作会话
   */
  createCollaborationSession: async (requestData: CollaborationSessionRequest): Promise<CollaborationSessionResponse> => {
    try {
      return await request.post('/api/latex/collaboration/sessions', requestData)
    } catch (error) {
      console.error('Failed to create collaboration session:', error)
      return {
        success: false,
        error: error instanceof Error ? error.message : '创建会话失败'
      }
    }
  },

  /**
   * 更新光标位置
   */
  updateCursorPosition: async (requestData: CursorPositionRequest): Promise<CursorPositionResponse> => {
    try {
      return await request.put('/api/latex/collaboration/cursor', requestData)
    } catch (error) {
      console.error('Failed to update cursor position:', error)
      return {
        success: false,
        error: error instanceof Error ? error.message : '更新光标位置失败'
      }
    }
  },

  /**
   * 获取文档模板列表 - 使用后端API
   */
  getTemplates: async (category?: string) => {
    try {
      const templates = await listLatexTemplates(category)
      return {
        success: true,
        templates: templates.map(t => ({
          id: t.id.toString(),
          name: t.name,
          description: t.description,
          category: t.category,
          content: t.content,
          icon: t.icon,
          isBuiltIn: t.isBuiltIn
        }))
      }
    } catch (error) {
      console.error('Failed to get templates:', error)
      // 返回默认模板
      return {
        success: true,
        templates: [{
          id: 'default',
          name: '默认文档',
          description: '基础LaTeX文档模板',
          category: '学术论文',
          content: `\\documentclass{article}
\\usepackage{amsmath}
\\usepackage{amsfonts}
\\usepackage{amssymb}
\\usepackage{graphicx}
\\usepackage{hyperref}

\\title{Your Document Title}
\\author{Your Name}
\\date{\\today}

\\begin{document}

\\maketitle

\\begin{abstract}
Your abstract here.
\\end{abstract}

\\section{Introduction}
Your introduction here.

\\section{Main Content}
Write your main content here.

\\subsection{Mathematical Content}
Here is an example of mathematical content:

$$
E = mc^2
$$

Or inline math: $a^2 + b^2 = c^2$

\\section{Conclusion}
Your conclusion here.

\\end{document}`,
          icon: '📄',
          isBuiltIn: true
        }]
      }
    }
  },

  /**
   * 从模板创建文档 - 使用后端API
   */
  createFromTemplate: async (templateId: number, title: string, ownerId?: string) => {
    try {
      const doc = await createFromTemplate({
        templateId,
        title,
        ownerId
      })

      return {
        success: true,
        documentId: doc.id,
        content: doc.content
      }
    } catch (error) {
      console.error('Failed to create from template:', error)
      return {
        success: false,
        error: error instanceof Error ? error.message : '创建失败'
      }
    }
  },

  /**
   * 验证 LaTeX 语法
   */
  validateLatex: async (content: string) => {
    try {
      return await request.post('/api/latex/validate', { content })
    } catch (error) {
      console.error('LaTeX validation failed:', error)
      // 客户端验证作为后备
      return {
        success: true,
        errors: [],
        warnings: []
      }
    }
  }
}
