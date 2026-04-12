/**
 * LaTeX Adapter
 *
 * Handles transformation between backend LaTeX objects (C++/JSON) and frontend LaTeX objects (TypeScript).
 *
 * @module api/adapters/latexAdapter
 */

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * Backend LaTeX Document structure (matches C++ LatexDocument struct)
 */
interface BackendLatexDocument {
  id: number
  title: string
  content: string
  owner_id: string
  is_collaborative: boolean
  created_at: string  // ISO 8601 or Unix timestamp
  updated_at: string  // ISO 8601 or Unix timestamp
  last_auto_save: string  // ISO 8601 or Unix timestamp
  version: number
  is_compiled: boolean
  pdf_path: string
}

/**
 * Backend LaTeX Template structure (matches C++ LatexTemplate struct)
 */
interface BackendLatexTemplate {
  id: number
  name: string
  description: string
  category: string
  content: string
  icon: string
  is_built_in: boolean
}

/**
 * Backend LaTeX Compilation Result structure
 */
interface BackendLatexCompilationResult {
  success: boolean
  pdf_path?: string
  log?: string
  error?: string
  compile_time_ms?: number
}

/**
 * Backend LaTeX Statistics structure
 */
interface BackendLatexStats {
  total_documents: number
  compiled_documents: number
  collaborative_documents: number
  total_words: number
  total_characters: number
}

/**
 * Frontend LaTeX Document structure
 */
export interface FrontendLatexDocument {
  id: number
  title: string
  content: string
  ownerId: string
  isCollaborative: boolean
  createdAt: string
  updatedAt: string
  lastAutoSave: string
  version: number
  isCompiled: boolean
  pdfPath: string
}

/**
 * Frontend LaTeX Template structure
 */
export interface FrontendLatexTemplate {
  id: number
  name: string
  description: string
  category: string
  content: string
  icon: string
  isBuiltIn: boolean
}

/**
 * Frontend LaTeX Compilation Result structure
 */
export interface FrontendLatexCompilationResult {
  success: boolean
  pdfPath?: string
  log?: string
  error?: string
  compileTimeMs?: number
}

/**
 * Frontend LaTeX Statistics structure
 */
export interface FrontendLatexStats {
  totalDocuments: number
  compiledDocuments: number
  collaborativeDocuments: number
  totalWords: number
  totalCharacters: number
}

/**
 * Frontend LaTeX Document query parameters
 */
export interface LatexDocumentQueryParams {
  page?: number
  limit?: number
  ownerId?: string
}

/**
 * LaTeX Document creation request
 */
export interface CreateLatexDocumentRequest {
  title: string
  content: string
  ownerId?: string
  isCollaborative?: boolean
}

/**
 * LaTeX Document update request
 */
export interface UpdateLatexDocumentRequest {
  title?: string
  content?: string
}

/**
 * LaTeX Template creation from template request
 */
export interface CreateFromTemplateRequest {
  templateId: number
  title: string
  ownerId?: string
}

// ============================================================================
// Transformation Functions
// ============================================================================

/**
 * Transform backend LaTeX document to frontend format
 */
export function toFrontendLatexDocument(backend: BackendLatexDocument): FrontendLatexDocument {
  return {
    id: backend.id,
    title: backend.title,
    content: backend.content,
    ownerId: backend.owner_id,
    isCollaborative: backend.is_collaborative,
    createdAt: toISODateString(backend.created_at),
    updatedAt: toISODateString(backend.updated_at),
    lastAutoSave: toISODateString(backend.last_auto_save),
    version: backend.version,
    isCompiled: backend.is_compiled,
    pdfPath: backend.pdf_path
  }
}

/**
 * Transform frontend LaTeX document to backend format (for creation)
 */
export function toBackendLatexDocumentRequest(frontend: CreateLatexDocumentRequest): Record<string, any> {
  return {
    title: frontend.title,
    content: frontend.content,
    owner_id: frontend.ownerId || '',
    is_collaborative: frontend.isCollaborative || false
  }
}

/**
 * Transform frontend LaTeX document update to backend format
 */
export function toBackendLatexDocumentUpdate(frontend: UpdateLatexDocumentRequest): Record<string, any> {
  const result: Record<string, any> = {}
  if (frontend.title !== undefined) result.title = frontend.title
  if (frontend.content !== undefined) result.content = frontend.content
  return result
}

/**
 * Transform backend LaTeX template to frontend format
 */
export function toFrontendLatexTemplate(backend: BackendLatexTemplate): FrontendLatexTemplate {
  return {
    id: backend.id,
    name: backend.name,
    description: backend.description,
    category: backend.category,
    content: backend.content,
    icon: backend.icon,
    isBuiltIn: backend.is_built_in
  }
}

/**
 * Transform backend compilation result to frontend format
 */
export function toFrontendCompilationResult(backend: BackendLatexCompilationResult): FrontendLatexCompilationResult {
  return {
    success: backend.success,
    pdfPath: backend.pdf_path,
    log: backend.log,
    error: backend.error,
    compileTimeMs: backend.compile_time_ms
  }
}

/**
 * Transform backend statistics to frontend format
 */
export function toFrontendStats(backend: BackendLatexStats): FrontendLatexStats {
  return {
    totalDocuments: backend.total_documents,
    compiledDocuments: backend.compiled_documents,
    collaborativeDocuments: backend.collaborative_documents,
    totalWords: backend.total_words,
    totalCharacters: backend.total_characters
  }
}

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Convert backend timestamp to ISO date string
 */
function toISODateString(timestamp: string | number): string {
  // If it's already an ISO string, return as is
  if (typeof timestamp === 'string' && timestamp.includes('T')) {
    return timestamp
  }

  // If it's a Unix timestamp, convert to ISO string
  const date = new Date(typeof timestamp === 'number' ? timestamp * 1000 : timestamp)
  return date.toISOString()
}

/**
 * Transform template creation request to backend format
 */
export function toBackendCreateFromTemplateRequest(frontend: CreateFromTemplateRequest): Record<string, any> {
  return {
    template_id: frontend.templateId,
    title: frontend.title,
    owner_id: frontend.ownerId || ''
  }
}

// ============================================================================
// API Client Functions
// ============================================================================

import { apiClient } from '@/utils/http'

const API_BASE = '/api/latex'

/**
 * Get list of LaTeX documents
 */
export async function listLatexDocuments(params: LatexDocumentQueryParams = {}): Promise<{
  items: FrontendLatexDocument[]
  total: number
  page: number
  limit: number
}> {
  const queryParams = new URLSearchParams()
  if (params.page) queryParams.append('page', params.page.toString())
  if (params.limit) queryParams.append('limit', params.limit.toString())
  if (params.ownerId) queryParams.append('owner_id', params.ownerId)

  const response = await apiClient.get<{ success: boolean; data: { items: BackendLatexDocument[]; total: number; page: number; limit: number } }>(
    `${API_BASE}/documents?${queryParams.toString()}`
  )

  if (!response.data.success) {
    throw new Error(response.data.message || 'Failed to list documents')
  }

  return {
    items: response.data.data.items.map(toFrontendLatexDocument),
    total: response.data.data.total,
    page: response.data.data.page,
    limit: response.data.data.limit
  }
}

/**
 * Get LaTeX document by ID
 */
export async function getLatexDocument(id: number): Promise<FrontendLatexDocument> {
  const response = await apiClient.get<{ success: boolean; data: BackendLatexDocument }>(
    `${API_BASE}/documents/${id}`
  )

  if (!response.data.success) {
    throw new Error(response.data.message || 'Failed to get document')
  }

  return toFrontendLatexDocument(response.data.data)
}

/**
 * Create new LaTeX document
 */
export async function createLatexDocument(request: CreateLatexDocumentRequest): Promise<FrontendLatexDocument> {
  const response = await apiClient.post<{ success: boolean; data: BackendLatexDocument }>(
    `${API_BASE}/documents`,
    toBackendLatexDocumentRequest(request)
  )

  if (!response.data.success) {
    throw new Error(response.data.message || 'Failed to create document')
  }

  return toFrontendLatexDocument(response.data.data)
}

/**
 * Update LaTeX document
 */
export async function updateLatexDocument(id: number, request: UpdateLatexDocumentRequest): Promise<void> {
  const response = await apiClient.put<{ success: boolean }>(
    `${API_BASE}/documents/${id}`,
    toBackendLatexDocumentUpdate(request)
  )

  if (!response.data.success) {
    throw new Error(response.data.message || 'Failed to update document')
  }
}

/**
 * Delete LaTeX document
 */
export async function deleteLatexDocument(id: number): Promise<void> {
  const response = await apiClient.delete<{ success: boolean }>(
    `${API_BASE}/documents/${id}`
  )

  if (!response.data.success) {
    throw new Error(response.data.message || 'Failed to delete document')
  }
}

/**
 * Compile LaTeX document to PDF
 */
export async function compileLatexDocument(id: number): Promise<FrontendLatexCompilationResult> {
  const response = await apiClient.post<{ success: boolean; data?: BackendLatexCompilationResult; error?: string }>(
    `${API_BASE}/documents/${id}/compile`,
    {}
  )

  if (!response.data.success) {
    throw new Error(response.data.error || 'Failed to compile document')
  }

  return toFrontendCompilationResult(response.data.data || { success: false })
}

/**
 * Auto-save LaTeX document
 */
export async function autoSaveLatexDocument(id: number, content: string): Promise<void> {
  const response = await apiClient.post<{ success: boolean }>(
    `${API_BASE}/documents/${id}/autosave`,
    { content }
  )

  if (!response.data.success) {
    throw new Error(response.data.message || 'Failed to auto-save')
  }
}

/**
 * Get list of LaTeX templates
 */
export async function listLatexTemplates(category?: string): Promise<FrontendLatexTemplate[]> {
  const queryParams = category ? `?category=${encodeURIComponent(category)}` : ''

  const response = await apiClient.get<{ success: boolean; data: { items: BackendLatexTemplate[]; total: number } }>(
    `${API_BASE}/templates${queryParams}`
  )

  if (!response.data.success) {
    throw new Error(response.data.message || 'Failed to list templates')
  }

  return response.data.data.items.map(toFrontendLatexTemplate)
}

/**
 * Get LaTeX template by ID
 */
export async function getLatexTemplate(id: number): Promise<FrontendLatexTemplate> {
  const response = await apiClient.get<{ success: boolean; data: BackendLatexTemplate }>(
    `${API_BASE}/templates/${id}`
  )

  if (!response.data.success) {
    throw new Error(response.data.message || 'Failed to get template')
  }

  return toFrontendLatexTemplate(response.data.data)
}

/**
 * Create document from template
 */
export async function createFromTemplate(request: CreateFromTemplateRequest): Promise<FrontendLatexDocument> {
  const response = await apiClient.post<{ success: boolean; data: BackendLatexDocument }>(
    `${API_BASE}/templates`,
    toBackendCreateFromTemplateRequest(request)
  )

  if (!response.data.success) {
    throw new Error(response.data.message || 'Failed to create from template')
  }

  return toFrontendLatexDocument(response.data.data)
}

/**
 * Get LaTeX statistics
 */
export async function getLatexStats(): Promise<FrontendLatexStats> {
  const response = await apiClient.get<{ success: boolean; data: BackendLatexStats }>(
    `${API_BASE}/stats`
  )

  if (!response.data.success) {
    throw new Error(response.data.message || 'Failed to get statistics')
  }

  return toFrontendStats(response.data.data)
}

/**
 * Download PDF
 */
export async function downloadLatexPDF(id: number): Promise<string> {
  const response = await apiClient.get<{ success: boolean; data: { pdf_path: string } }>(
    `${API_BASE}/documents/${id}/pdf`
  )

  if (!response.data.success) {
    throw new Error(response.data.message || 'Failed to get PDF path')
  }

  return response.data.data.pdf_path
}
