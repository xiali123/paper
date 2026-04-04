/**
 * Collaborative Writing API Module
 * 实时协作写作功能 - 对应后端CollaborativeWritingModule
 */

import request from '@/utils/request'
import type {
  CollaborativeDocument,
  OTOperation,
  WritingSuggestion,
  DocumentVersion,
  DocumentComment
} from '@/types/collaborative'

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
 * 协作会话信息
 */
export interface CollaborationSession {
  documentId: number
  activeUsers: Array<{
    userId: number
    userName: string
    cursorPosition?: number
    lastActiveAt: string
  }>
  connectedAt: string
}

/**
 * 协作写作API
 */
export const collaborativeApi = {
  /**
   * 创建协作文档
   * POST /api/collab/documents
   */
  async createDocument(req: CreateDocumentRequest): Promise<CollaborativeDocument> {
    return await request.post('/api/collab/documents', req)
  },

  /**
   * 获取文档详情
   * GET /api/collab/documents/:id
   */
  async getDocument(documentId: number): Promise<CollaborativeDocument> {
    return await request.get(`/api/collab/documents/${documentId}`)
  },

  /**
   * 更新文档元数据
   * PUT /api/collab/documents/:id
   */
  async updateDocument(
    documentId: number,
    updates: Partial<CollaborativeDocument>
  ): Promise<CollaborativeDocument> {
    return await request.put(`/api/collab/documents/${documentId}`, updates)
  },

  /**
   * 删除文档
   * DELETE /api/collab/documents/:id
   */
  async deleteDocument(documentId: number): Promise<{ success: boolean }> {
    return await request.delete(`/api/collab/documents/${documentId}`)
  },

  /**
   * 应用OT操作
   * POST /api/collab/documents/:id/operations
   */
  async applyOperation(req: ApplyOperationRequest): Promise<{
    success: boolean
    newContent: string
    operationId: string
  }> {
    return await request.post(`/api/collab/documents/${req.documentId}/operations`, req)
  },

  /**
   * 批量应用操作
   * POST /api/collab/documents/:id/operations/batch
   */
  async applyOperationsBatch(documentId: number, operations: OTOperation[]): Promise<{
    success: boolean
    appliedCount: number
    finalContent: string
  }> {
    return await request.post(`/api/collab/documents/${documentId}/operations/batch`, {
      operations
    })
  },

  /**
   * 获取AI写作建议
   * GET /api/collab/documents/:id/suggestions
   */
  async getSuggestions(documentId: number): Promise<WritingSuggestion[]> {
    return await request.get(`/api/collab/documents/${documentId}/suggestions`)
  },

  /**
   * 生成AI建议
   * POST /api/collab/documents/:id/suggestions/generate
   */
  async generateSuggestion(req: GenerateSuggestionRequest): Promise<WritingSuggestion> {
    return await request.post(
      `/api/collab/documents/${req.documentId}/suggestions/generate`,
      req
    )
  },

  /**
   * 批量生成建议
   * POST /api/collab/documents/:id/suggestions/generate-batch
   */
  async generateSuggestionsBatch(documentId: number): Promise<WritingSuggestion[]> {
    return await request.post(`/api/collab/documents/${documentId}/suggestions/generate-batch`)
  },

  /**
   * 接受建议
   * POST /api/collab/documents/suggestions/:id/accept
   */
  async acceptSuggestion(suggestionId: number): Promise<{ success: boolean }> {
    return await request.post(`/api/collab/documents/suggestions/${suggestionId}/accept`)
  },

  /**
   * 拒绝建议
   * POST /api/collab/documents/suggestions/:id/reject
   */
  async rejectSuggestion(suggestionId: number): Promise<{ success: boolean }> {
    return await request.post(`/api/collab/documents/suggestions/${suggestionId}/reject`)
  },

  /**
   * 获取版本历史
   * GET /api/collab/documents/:id/versions
   */
  async getVersions(documentId: number): Promise<DocumentVersion[]> {
    return await request.get(`/api/collab/documents/${documentId}/versions`)
  },

  /**
   * 恢复到特定版本
   * POST /api/collab/documents/:id/versions/:versionId/restore
   */
  async restoreVersion(documentId: number, versionId: number): Promise<CollaborativeDocument> {
    return await request.post(`/api/collab/documents/${documentId}/versions/${versionId}/restore`)
  },

  /**
   * 比较两个版本
   * GET /api/collab/documents/:id/versions/compare
   */
  async compareVersions(
    documentId: number,
    versionId1: number,
    versionId2: number
  ): Promise<{
    version1: DocumentVersion
    version2: DocumentVersion
    diff: string
  }> {
    return await request.get(`/api/collab/documents/${documentId}/versions/compare`, {
      params: { versionId1, versionId2 }
    })
  },

  /**
   * 添加评论
   * POST /api/collab/documents/:id/comments
   */
  async addComment(documentId: number, comment: {
    position: number
    text: string
    authorId: number
  }): Promise<{ success: boolean; commentId: number }> {
    return await request.post(`/api/collab/documents/${documentId}/comments`, comment)
  },

  /**
   * 获取文档所有评论
   * GET /api/collab/documents/:id/comments
   */
  async getComments(documentId: number): Promise<DocumentComment[]> {
    return await request.get(`/api/collab/documents/${documentId}/comments`)
  },

  /**
   * 解决评论
   * PUT /api/collab/documents/comments/:id/resolve
   */
  async resolveComment(commentId: number): Promise<{ success: boolean }> {
    return await request.put(`/api/collab/documents/comments/${commentId}/resolve`)
  },

  /**
   * 删除评论
   * DELETE /api/collab/documents/comments/:id
   */
  async deleteComment(commentId: number): Promise<{ success: boolean }> {
    return await request.delete(`/api/collab/documents/comments/${commentId}`)
  },

  /**
   * 获取协作文档列表
   * GET /api/collab/documents
   */
  async getDocuments(params?: {
    ownerId?: number
    status?: 'active' | 'archived' | 'deleted'
    page?: number
    limit?: number
  }): Promise<{
    documents: CollaborativeDocument[]
    total: number
    page: number
  }> {
    return await request.get('/api/collab/documents', { params })
  },

  /**
   * 获取协作会话信息
   * GET /api/collab/documents/:id/session
   */
  async getSession(documentId: number): Promise<CollaborationSession> {
    return await request.get(`/api/collab/documents/${documentId}/session`)
  },

  /**
   * WebSocket连接URL
   */
  getWebSocketUrl(documentId: number, token: string): string {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
    const host = window.location.host
    return `${protocol}//${host}/api/collab/documents/${documentId}/ws?token=${token}`
  },

  /**
   * 获取协作统计信息
   * GET /api/collab/stats
   */
  async getStats(): Promise<{
    totalDocuments: number
    activeUsers: number
    totalOperations: number
    totalSuggestions: number
    avgSessionDuration: number
  }> {
    return await request.get('/api/collab/stats')
  }
}

export default collaborativeApi
