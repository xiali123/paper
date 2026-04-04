/**
 * 协作写作类型定义
 */

/**
 * 协作文档
 */
export interface CollaborativeDocument {
  id: number
  title: string
  content: string
  documentType: string
  ownerId: number
  templateId?: number
  status: 'active' | 'archived' | 'deleted'
  wordCount: number
  lastModifiedBy: number
  createdAt: string
  updatedAt: string
}

/**
 * OT操作类型
 */
export enum OperationType {
  INSERT = 'insert',
  DELETE = 'delete',
  RETAIN = 'retain'
}

/**
 * OT操作
 */
export interface OTOperation {
  id: string
  type: OperationType
  position: number
  length?: number
  content?: string
  clientId: string
  timestamp: number
}

/**
 * 写作建议
 */
export interface WritingSuggestion {
  id: number
  documentId: number
  suggestionType: 'grammar' | 'style' | 'structure' | 'citation'
  positionStart: number
  positionEnd: number
  originalText: string
  suggestedText: string
  reason: string
  confidence: number
  accepted?: boolean
}

/**
 * 文档版本
 */
export interface DocumentVersion {
  id: number
  documentId: number
  versionNumber: number
  content: string
  createdBy: number
  createdAt: string
  changeDescription: string
}

/**
 * 文档评论
 */
export interface DocumentComment {
  id: number
  documentId: number
  position: number
  text: string
  authorId: number
  authorName: string
  createdAt: string
  resolved: boolean
}
