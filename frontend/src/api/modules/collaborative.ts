/**
 * Collaborative Writing API Module
 * 实时协作写作功能
 */

import { request } from '../utils/request'
import type {
  CollaborativeDocument,
  OTOperation,
  WritingSuggestion,
  DocumentVersion
} from '../types'

/**
 * 创建文档请求
 */
export interface CreateDocumentRequest {
  title: string
  content?: string
  documentType: string
  templateId?: number
}

/**
 * 应用操作请求
 */
export interface ApplyOperationRequest {
  documentId: number
  operation: OTOperation
}

/**
 * 生成建议请求
 */
export interface GenerateSuggestionRequest {
  documentId: number
  suggestionType: 'grammar' | 'style' | 'structure' | 'citation'
  positionStart: number
  positionEnd: number
}

/**
 * 协作写作API
 */
export const collaborativeApi = {
  /**
   * 创建协作文档
   * POST /api/collaborative/documents
   */
  async createDocument(request: CreateDocumentRequest): Promise<CollaborativeDocument> {
    return await request.post('/api/collaborative/documents', request)
  },

  /**
   * 获取文档详情
   * GET /api/collaborative/documents/:id
   */
  async getDocument(documentId: number): Promise<CollaborativeDocument> {
    return await request.get(`/api/collaborative/documents/${documentId}`)
  },

  /**
   * 更新文档
   * PUT /api/collaborative/documents/:id
   */
  async updateDocument(
    documentId: number,
    updates: Partial<CollaborativeDocument>
  ): Promise<CollaborativeDocument> {
    return await request.put(`/api/collaborative/documents/${documentId}`, updates)
  },

  /**
   * 应用OT操作
   * POST /api/collaborative/documents/:id/operations
   */
  async applyOperation(request: ApplyOperationRequest): Promise<{
    success: boolean
    newContent: string
  }> {
    return await request.post(
      `/api/collaborative/documents/${request.documentId}/operations`,
      request
    )
  },

  /**
   * 获取AI写作建议
   * GET /api/collaborative/documents/:id/suggestions
   */
  async getSuggestions(documentId: number): Promise<WritingSuggestion[]> {
    return await request.get(`/api/collaborative/documents/${documentId}/suggestions`)
  },

  /**
   * 生成AI建议
   * POST /api/collaborative/documents/:id/suggestions/generate
   */
  async generateSuggestion(request: GenerateSuggestionRequest): Promise<WritingSuggestion> {
    return await request.post(
      `/api/collaborative/documents/${request.documentId}/suggestions/generate`,
      request
    )
  },

  /**
   * 获取版本历史
   * GET /api/collaborative/documents/:id/versions
   */
  async getVersions(documentId: number): Promise<DocumentVersion[]> {
    return await request.get(`/api/collaborative/documents/${documentId}/versions`)
  },

  /**
   * 添加评论
   * POST /api/collaborative/documents/:id/comments
   */
  async addComment(documentId: number, comment: {
    position: number
    text: string
    authorId: number
  }): Promise<{ success: boolean; commentId: number }> {
    return await request.post(`/api/collaborative/documents/${documentId}/comments`, comment)
  },

  /**
   * WebSocket连接URL
   */
  getWebSocketUrl(documentId: number, token: string): string {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
    const host = window.location.host
    return `${protocol}//${host}/api/collaborative/documents/${documentId}/ws?token=${token}`
  }
}

export default collaborativeApi
