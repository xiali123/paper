/**
 * Collaborative Writing API Module
 * 协作写作功能 - 对应后端 CollaborativeWritingModule
 * API 路径前缀: /api/writing
 */

import request from '@/utils/request'
import type {
  CollaborativeDocument,
  OTOperation,
  WritingSuggestion,
  DocumentVersion,
  DocumentComment,
  CreateDocumentRequest,
  UpdateDocumentRequest,
  GenerateSuggestionRequest,
  AddCommentRequest
} from '@/types/collaborative'

/**
 * 协作写作 API
 */
export const collaborativeApi = {
  // ==================== 文档管理 ====================

  /**
   * 创建文档
   * POST /api/writing/documents
   */
  async createDocument(data: CreateDocumentRequest): Promise<CollaborativeDocument> {
    return await request.post('/api/writing/documents', data)
  },

  /**
   * 获取文档列表
   * GET /api/writing/documents
   */
  async getDocuments(params?: {
    title?: string
    ownerId?: number
    docType?: string
  }): Promise<CollaborativeDocument[]> {
    const response = await request.get<{ documents?: CollaborativeDocument[]; total?: number }>('/api/writing/documents', { params })
    return Array.isArray(response) ? response : (response.documents || [])
  },

  /**
   * 获取单个文档
   * GET /api/writing/documents/:id
   */
  async getDocument(id: number): Promise<CollaborativeDocument> {
    return await request.get(`/api/writing/documents/${id}`)
  },

  /**
   * 更新文档
   * PUT /api/writing/documents/:id
   */
  async updateDocument(id: number, data: UpdateDocumentRequest): Promise<CollaborativeDocument> {
    return await request.put(`/api/writing/documents/${id}`, data)
  },

  /**
   * 删除文档 (待后端注册路由)
   * DELETE /api/writing/documents/:id
   */
  async deleteDocument(id: number): Promise<{ success: boolean }> {
    return await request.delete(`/api/writing/documents/${id}`)
  },

  // ==================== OT 操作 ====================

  /**
   * 应用 OT 操作
   * POST /api/writing/documents/:id/operations
   */
  async applyOperation(documentId: number, operation: OTOperation): Promise<{
    content: string
    word_count: number
  }> {
    return await request.post(`/api/writing/documents/${documentId}/operations`, {
      type: operation.type,
      position: operation.position || 0,
      length: operation.length || 0,
      content: operation.content || '',
      client_id: operation.client_id || 0,
      timestamp: operation.timestamp || Date.now()
    })
  },

  // ==================== AI 建议 ====================

  /**
   * 获取 AI 建议
   * GET /api/writing/documents/:id/suggestions
   */
  async getSuggestions(documentId: number): Promise<WritingSuggestion[]> {
    const response = await request.get<{
      suggestions?: WritingSuggestion[]
      total?: number
    }>(`/api/writing/documents/${documentId}/suggestions`)
    return Array.isArray(response) ? response : (response.suggestions || [])
  },

  /**
   * 生成 AI 建议
   * POST /api/writing/documents/:id/suggestions/generate
   */
  async generateSuggestion(documentId: number, data?: GenerateSuggestionRequest): Promise<WritingSuggestion> {
    return await request.post(`/api/writing/documents/${documentId}/suggestions/generate`, {
      suggestion_type: data?.suggestion_type || 'content',
      user_id: data?.user_id || 0,
      position_start: data?.position_start || 0,
      position_end: data?.position_end || 0
    })
  },

  /**
   * 接受建议 (待后端注册路由)
   * PUT /api/writing/suggestions/:id/accept
   */
  async acceptSuggestion(suggestionId: number): Promise<{ success: boolean }> {
    return await request.put(`/api/writing/suggestions/${suggestionId}/accept`)
  },

  /**
   * 拒绝建议 (待后端注册路由)
   * PUT /api/writing/suggestions/:id/reject
   */
  async rejectSuggestion(suggestionId: number): Promise<{ success: boolean }> {
    return await request.put(`/api/writing/suggestions/${suggestionId}/reject`)
  },

  // ==================== 版本历史 ====================

  /**
   * 获取版本历史
   * GET /api/writing/documents/:id/versions
   */
  async getVersions(documentId: number): Promise<DocumentVersion[]> {
    const response = await request.get<{
      versions?: DocumentVersion[]
      total?: number
    }>(`/api/writing/documents/${documentId}/versions`)
    return Array.isArray(response) ? response : (response.versions || [])
  },

  /**
   * 创建版本 (待后端注册路由)
   * POST /api/writing/documents/:id/versions
   */
  async createVersion(documentId: number, summary?: string): Promise<DocumentVersion> {
    return await request.post(`/api/writing/documents/${documentId}/versions`, {
      change_summary: summary || ''
    })
  },

  // ==================== 评论 ====================

  /**
   * 获取评论 (待后端注册路由)
   * GET /api/writing/documents/:id/comments
   */
  async getComments(documentId: number): Promise<DocumentComment[]> {
    return await request.get(`/api/writing/documents/${documentId}/comments`)
  },

  /**
   * 添加评论
   * POST /api/writing/documents/:id/comments
   */
  async addComment(documentId: number, data: AddCommentRequest): Promise<{ id: number }> {
    return await request.post(`/api/writing/documents/${documentId}/comments`, {
      user_id: data.user_id || 0,
      content: data.content,
      position_start: data.position_start ?? -1,
      position_end: data.position_end ?? -1,
      parent_id: data.parent_id ?? 0
    })
  },

  /**
   * 解决评论 (待后端注册路由)
   * PUT /api/writing/comments/:id/resolve
   */
  async resolveComment(commentId: number): Promise<{ success: boolean }> {
    return await request.put(`/api/writing/comments/${commentId}/resolve`)
  }
}

export default collaborativeApi
