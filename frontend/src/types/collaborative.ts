/**
 * 协作写作类型定义
 * 匹配后端 CollaborativeWritingModule API
 */

/**
 * 文档状态
 */
export type DocumentStatus = 'active' | 'archived' | 'deleted' | 'draft' | 'published'

/**
 * 文档类型
 */
export type DocumentType = 'paper' | 'report' | 'note' | 'thesis' | 'other'

/**
 * AI 建议类型
 */
export type SuggestionType = 'grammar' | 'style' | 'structure' | 'citation' | 'content'

/**
 * 建议状态
 */
export type SuggestionStatus = 'pending' | 'accepted' | 'rejected'

/**
 * OT 操作类型
 */
export type OTOperationType = 'INSERT' | 'DELETE' | 'RETAIN' | 'FORMAT'

/**
 * 协作文档
 */
export interface CollaborativeDocument {
  id: number
  title: string
  content: string
  document_type: DocumentType | string
  owner_id: number
  template_id?: number
  status: DocumentStatus | string
  word_count: number
  last_modified_by?: number
  created_at: string
  updated_at: string
}

/**
 * OT 操作
 */
export interface OTOperation {
  type: OTOperationType | number
  position: number
  length?: number
  content?: string
  client_id?: number
  timestamp?: number
}

/**
 * 写作建议
 */
export interface WritingSuggestion {
  id: number
  document_id: number
  user_id?: number
  suggestion_type: SuggestionType | string
  position_start: number
  position_end: number
  original_text: string
  suggested_text: string
  confidence_score: number
  explanation: string
  status: SuggestionStatus | string
  created_at?: string
  updated_at?: string
}

/**
 * 文档版本
 */
export interface DocumentVersion {
  id: number
  document_id: number
  version_number: number
  content: string
  change_summary?: string
  word_count: number
  created_by: number
  is_auto_save: boolean
  created_at: string
}

/**
 * 文档评论
 */
export interface DocumentComment {
  id: number
  document_id: number
  user_id: number
  parent_id?: number
  position_start: number
  position_end: number
  content: string
  is_resolved: boolean
  created_at: string
  updated_at?: string
}

/**
 * 创建文档请求
 */
export interface CreateDocumentRequest {
  title: string
  content?: string
  document_type?: DocumentType | string
  owner_id?: number
  template_id?: number
}

/**
 * 更新文档请求
 */
export interface UpdateDocumentRequest {
  title?: string
  content?: string
  status?: DocumentStatus | string
}

/**
 * 生成建议请求
 */
export interface GenerateSuggestionRequest {
  suggestion_type?: SuggestionType | string
  user_id?: number
  position_start?: number
  position_end?: number
}

/**
 * 添加评论请求
 */
export interface AddCommentRequest {
  content: string
  user_id?: number
  position_start?: number
  position_end?: number
  parent_id?: number
}
