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

import service from '@/utils/request'

// 创建 API 客户端函数 - 响应拦截器已经提取了 data
// 所以这些函数直接返回 data 类型，而不是 AxiosResponse
const apiClient = {
  get: <T>(url: string, config?: any): Promise<T> => service.get(url, config) as any,
  post: <T>(url: string, data?: any, config?: any): Promise<T> => service.post(url, data, config) as any,
  put: <T>(url: string, data?: any, config?: any): Promise<T> => service.put(url, data, config) as any,
  delete: <T>(url: string, config?: any): Promise<T> => service.delete(url, config) as any,
  patch: <T>(url: string, data?: any, config?: any): Promise<T> => service.patch(url, data, config) as any,
}

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

  // service 拦截器已经提取了 data 字段，response 就是 { items, total, page, limit }
  const response = await apiClient.get<{ items: BackendLatexDocument[]; total: number; page: number; limit: number }>(
    `${API_BASE}/documents?${queryParams.toString()}`
  )

  return {
    items: response.items.map(toFrontendLatexDocument),
    total: response.total,
    page: response.page,
    limit: response.limit
  }
}

/**
 * Get LaTeX document by ID
 */
export async function getLatexDocument(id: number): Promise<FrontendLatexDocument> {
  // service 拦截器已经提取了 data 字段，response 就是 BackendLatexDocument
  const response = await apiClient.get<BackendLatexDocument>(
    `${API_BASE}/documents/${id}`
  )

  return toFrontendLatexDocument(response)
}

/**
 * Create new LaTeX document
 */
export async function createLatexDocument(request: CreateLatexDocumentRequest): Promise<FrontendLatexDocument> {
  // service 拦截器已经提取了 data 字段，response 就是 BackendLatexDocument
  const response = await apiClient.post<BackendLatexDocument>(
    `${API_BASE}/documents`,
    toBackendLatexDocumentRequest(request)
  )

  return toFrontendLatexDocument(response)
}

/**
 * Update LaTeX document
 */
export async function updateLatexDocument(id: number, request: UpdateLatexDocumentRequest): Promise<void> {
  // service 拦截器已处理，直接发送请求即可
  await apiClient.put(
    `${API_BASE}/documents/${id}`,
    toBackendLatexDocumentUpdate(request)
  )
}

/**
 * Delete LaTeX document
 */
export async function deleteLatexDocument(id: number): Promise<void> {
  // service 拦截器已处理，直接发送请求即可
  await apiClient.delete(
    `${API_BASE}/documents/${id}`
  )
}

/**
 * Compile LaTeX document to PDF
 */
export async function compileLatexDocument(id: number): Promise<FrontendLatexCompilationResult> {
  // service 拦截器已提取 data 字段
  const response = await apiClient.post<BackendLatexCompilationResult>(
    `${API_BASE}/documents/${id}/compile`,
    {}
  )

  return toFrontendCompilationResult(response)
}

/**
 * Auto-save LaTeX document
 */
export async function autoSaveLatexDocument(id: number, content: string): Promise<void> {
  // service 拦截器已处理，直接发送请求即可
  await apiClient.post(
    `${API_BASE}/documents/${id}/autosave`,
    { content }
  )
}

/**
 * Get list of LaTeX templates
 */
export async function listLatexTemplates(category?: string): Promise<FrontendLatexTemplate[]> {
  const queryParams = category ? `?category=${encodeURIComponent(category)}` : ''

  // service 拦截器已提取 data 字段，response 就是 { items, total }
  const response = await apiClient.get<{ items: BackendLatexTemplate[]; total: number }>(
    `${API_BASE}/templates${queryParams}`
  )

  return response.items.map(toFrontendLatexTemplate)
}

/**
 * Get LaTeX template by ID
 */
export async function getLatexTemplate(id: number): Promise<FrontendLatexTemplate> {
  // service 拦截器已提取 data 字段
  const response = await apiClient.get<BackendLatexTemplate>(
    `${API_BASE}/templates/${id}`
  )

  return toFrontendLatexTemplate(response)
}

/**
 * Create document from template
 */
export async function createFromTemplate(request: CreateFromTemplateRequest): Promise<FrontendLatexDocument> {
  // service 拦截器已提取 data 字段
  const response = await apiClient.post<BackendLatexDocument>(
    `${API_BASE}/templates`,
    toBackendCreateFromTemplateRequest(request)
  )

  return toFrontendLatexDocument(response)
}

/**
 * Get LaTeX statistics
 */
export async function getLatexStats(): Promise<FrontendLatexStats> {
  // service 拦截器已提取 data 字段
  const response = await apiClient.get<BackendLatexStats>(
    `${API_BASE}/stats`
  )

  return toFrontendStats(response)
}

/**
 * Download PDF
 */
export async function downloadLatexPDF(id: number): Promise<string> {
  // service 拦截器已提取 data 字段
  const response = await apiClient.get<{ pdf_path: string }>(
    `${API_BASE}/documents/${id}/pdf`
  )

  return response.pdf_path
}

// ============================================================================
// 项目相关类型定义
// ============================================================================

/**
 * Backend Project File structure
 */
interface BackendLatexProjectFile {
  id: number
  project_id: number
  name: string
  path: string
  content: string
  type: string
  created_at: number
  updated_at: number
}

/**
 * Backend LaTeX Project structure
 */
interface BackendLatexProject {
  id: number
  name: string
  owner_id: string
  main_file: string
  description: string
  is_public: boolean
  created_at: number
  updated_at: number
  version: number
}

/**
 * Frontend Project File structure
 */
export interface FrontendLatexProjectFile {
  id: number
  projectId: number
  name: string
  path: string
  content: string
  type: 'main' | 'included' | 'bibliography' | 'image' | 'other'
  createdAt: number
  updatedAt: number
}

/**
 * Frontend LaTeX Project structure
 */
export interface FrontendLatexProject {
  id: number
  name: string
  ownerId: string
  mainFile: string
  description: string
  isPublic: boolean
  createdAt: number
  updatedAt: number
  version: number
  files: FrontendLatexProjectFile[]
}

/**
 * Create Project Request
 */
export interface CreateLatexProjectRequest {
  name: string
  mainFile?: string
  description?: string
  isPublic?: boolean
  ownerId?: string
}

/**
 * Update Project Request
 */
export interface UpdateLatexProjectRequest {
  name?: string
  mainFile?: string
  description?: string
  isPublic?: boolean
}

/**
 * Create Project File Request
 */
export interface CreateProjectFileRequest {
  projectId: number
  name: string
  path: string
  content: string
  type?: string
}

/**
 * Update Project File Request
 */
export interface UpdateProjectFileRequest {
  content?: string
}

/**
 * Project Query Parameters
 */
export interface LatexProjectQueryParams {
  page?: number
  limit?: number
  ownerId?: string
}

// ============================================================================
// Transformation Functions
// ============================================================================

function toFrontendProjectFile(backend: BackendLatexProjectFile): FrontendLatexProjectFile {
  return {
    id: backend.id,
    projectId: backend.project_id,
    name: backend.name,
    path: backend.path,
    content: backend.content,
    type: backend.type as any,
    createdAt: backend.created_at,
    updatedAt: backend.updated_at
  }
}

function toFrontendProject(backend: BackendLatexProject, files: FrontendLatexProjectFile[] = []): FrontendLatexProject {
  return {
    id: backend.id,
    name: backend.name,
    ownerId: backend.owner_id,
    mainFile: backend.main_file,
    description: backend.description,
    isPublic: backend.is_public,
    createdAt: backend.created_at,
    updatedAt: backend.updated_at,
    version: backend.version,
    files
  }
}

// ============================================================================
// 项目 API 客户端函数
// ============================================================================

/**
 * 获取项目列表
 */
export async function listLatexProjects(params: LatexProjectQueryParams = {}): Promise<{
  items: FrontendLatexProject[]
  total: number
  page: number
  limit: number
}> {
  const queryParams = new URLSearchParams()
  if (params.page) queryParams.append('page', params.page.toString())
  if (params.limit) queryParams.append('limit', params.limit.toString())
  if (params.ownerId) queryParams.append('owner_id', params.ownerId)

  const response = await apiClient.get<{ items: BackendLatexProject[]; total: number; page: number; limit: number }>(
    `${API_BASE}/projects?${queryParams.toString()}`
  )

  return {
    items: response.items.map(p => toFrontendProject(p)),
    total: response.total,
    page: response.page,
    limit: response.limit
  }
}

/**
 * 获取项目详情
 */
export async function getLatexProject(id: number): Promise<FrontendLatexProject> {
  const response = await apiClient.get<{
    id: number
    name: string
    owner_id: string
    main_file: string
    description: string
    is_public: boolean
    created_at: number
    updated_at: number
    version: number
    files: BackendLatexProjectFile[]
  }>(`${API_BASE}/projects/${id}`)

  const files = response.files.map(toFrontendProjectFile)
  return toFrontendProject(response, files)
}

/**
 * 创建项目
 */
export async function createLatexProject(request: CreateLatexProjectRequest): Promise<FrontendLatexProject> {
  const response = await apiClient.post<{ id: number; name: string; main_file: string }>(
    `${API_BASE}/projects`,
    request
  )

  // 创建后返回的只有基本信息，需要重新获取完整信息
  return await getLatexProject(response.id)
}

/**
 * 更新项目
 */
export async function updateLatexProject(id: number, request: UpdateLatexProjectRequest): Promise<void> {
  await apiClient.put(`${API_BASE}/projects/${id}`, request)
}

/**
 * 删除项目
 */
export async function deleteLatexProject(id: number): Promise<void> {
  await apiClient.del(`${API_BASE}/projects/${id}`)
}

/**
 * 编译项目
 */
export async function compileLatexProject(id: number): Promise<FrontendLatexCompilationResult> {
  const response = await apiClient.post<BackendLatexCompilationResult>(
    `${API_BASE}/projects/${id}/compile`,
    {}
  )

  return toFrontendCompilationResult(response)
}

/**
 * 添加项目文件
 */
export async function addProjectFile(request: CreateProjectFileRequest): Promise<FrontendLatexProjectFile> {
  const response = await apiClient.post<{ id: number; name: string; path: string }>(
    `${API_BASE}/projects/files`,
    {
      project_id: request.projectId,
      name: request.name,
      path: request.path,
      content: request.content || '',
      type: request.type || 'other'
    }
  )

  return toFrontendProjectFile({
    id: response.id,
    project_id: request.projectId,
    name: response.name,
    path: response.path,
    content: request.content || '',
    type: request.type || 'other',
    created_at: Date.now() / 1000,
    updated_at: Date.now() / 1000
  })
}

/**
 * 更新项目文件
 */
export async function updateProjectFile(fileId: number, content: string): Promise<void> {
  await apiClient.put(`${API_BASE}/projects/files/${fileId}`, { content })
}

/**
 * 删除项目文件
 */
export async function deleteProjectFile(fileId: number): Promise<void> {
  await apiClient.del(`${API_BASE}/projects/files/${fileId}`)
}

/**
 * 获取项目文件内容
 */
export async function getProjectFile(fileId: number): Promise<FrontendLatexProjectFile> {
  const response = await apiClient.get<BackendLatexProjectFile>(
    `${API_BASE}/projects/files/${fileId}`
  )

  return toFrontendProjectFile(response)
}
