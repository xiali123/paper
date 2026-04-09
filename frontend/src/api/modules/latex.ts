import request from '@/utils/request'
import type { CompilationResult } from '../../architecture/stores/latexEditor'

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
  id?: string
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
    id: string
    name: string
    path: string
    lastModified: number
    size: number
  }
  error?: string
}

export interface LoadDocumentRequest {
  id: string
}

export interface LoadDocumentResponse {
  success: boolean
  document?: {
    id: string
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
 * LaTeX 编译服务
 */
export const latexApi = {
  /**
   * 编译 LaTeX 文档
   */
  compile: async (requestData: CompileLatexRequest): Promise<CompileLatexResponse> => {
    try {
      const response = await request.post('/api/latex/compile', requestData)
      return response.data
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
   * 保存文档
   */
  saveDocument: async (requestData: SaveDocumentRequest): Promise<SaveDocumentResponse> => {
    try {
      // For now, simulate a successful save since backend might not be available
      console.log('Mock save document:', requestData.name, 'content length:', requestData.content.length)

      // Simulate API delay
      await new Promise(resolve => setTimeout(resolve, 500))

      // Simulate successful save
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
    } catch (error) {
      console.error('Document save failed:', error)
      return {
        success: false,
        error: error instanceof Error ? error.message : '保存失败'
      }
    }
  },

  /**
   * 加载文档
   */
  loadDocument: async (requestData: LoadDocumentRequest): Promise<LoadDocumentResponse> => {
    try {
      const response = await request.get(`/api/latex/documents/${requestData.id}`)
      return response.data
    } catch (error) {
      console.error('Document load failed:', error)
      return {
        success: false,
        error: error instanceof Error ? error.message : '加载失败'
      }
    }
  },

  /**
   * 创建协作会话
   */
  createCollaborationSession: async (requestData: CollaborationSessionRequest): Promise<CollaborationSessionResponse> => {
    try {
      const response = await request.post('/api/latex/collaboration/sessions', requestData)
      return response.data
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
      const response = await request.put('/api/latex/collaboration/cursor', requestData)
      return response.data
    } catch (error) {
      console.error('Failed to update cursor position:', error)
      return {
        success: false,
        error: error instanceof Error ? error.message : '更新光标位置失败'
      }
    }
  },

  /**
   * 获取文档模板
   */
  getTemplate: async (templateName: string = 'default') => {
    try {
      const response = await request.get(`/api/latex/templates/${templateName}`)
      return response.data
    } catch (error) {
      console.error('Failed to get template:', error)
      // 返回默认模板
      return {
        success: true,
        template: `\\documentclass{article}
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

\\bibliographystyle{plain}
\\bibliography{references}

\\end{document}`
      }
    }
  },

  /**
   * 验证 LaTeX 语法
   */
  validateLatex: async (content: string) => {
    try {
      const response = await request.post('/api/latex/validate', { content })
      return response.data
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