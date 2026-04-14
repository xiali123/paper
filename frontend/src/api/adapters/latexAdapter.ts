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

  // Validate date before calling toISOString()
  if (isNaN(date.getTime())) {
    console.warn('[toISODateString] Invalid timestamp:', timestamp)
    return new Date().toISOString() // Fallback to current time
  }

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
  del: <T>(url: string, config?: any): Promise<T> => service.delete(url, config) as any, // 别名，因为后端使用 del (delete 是 C++ 关键字)
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
 * @param id Document ID
 * @param userId Optional user ID for quota tracking
 */
export async function compileLatexDocument(id: number, userId?: string): Promise<FrontendLatexCompilationResult> {
  // service 拦截器已提取 data 字段
  const queryParams = userId ? `?user_id=${encodeURIComponent(userId)}` : ''
  const response = await apiClient.post<BackendLatexCompilationResult>(
    `${API_BASE}/documents/${id}/compile${queryParams}`,
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
 * @param id Project ID
 * @param userId Optional user ID for quota tracking
 */
export async function compileLatexProject(id: number, userId?: string): Promise<FrontendLatexCompilationResult> {
  const queryParams = userId ? `?user_id=${encodeURIComponent(userId)}` : ''
  const response = await apiClient.post<BackendLatexCompilationResult>(
    `${API_BASE}/projects/${id}/compile${queryParams}`,
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

// ============================================================================
// 用户配额管理类型定义
// ============================================================================

/**
 * Backend User Quota structure
 */
interface BackendLatexUserQuota {
  user_id: string
  daily_compile_limit: number
  monthly_compile_limit: number
  max_project_count: number
  can_use_advanced_features: boolean
  daily_compiles_used: number
  monthly_compiles_used: number
  project_count_used: number
  daily_reset: number
  monthly_reset: number
}

/**
 * Backend Compilation Record structure
 */
interface BackendLatexCompilationRecord {
  id: number
  user_id: string
  project_id: number
  document_id: string
  content_hash: string
  success: boolean
  error_message: string
  timestamp: number
}

/**
 * Frontend User Quota structure
 */
export interface FrontendLatexUserQuota {
  userId: string
  dailyCompileLimit: number
  monthlyCompileLimit: number
  maxProjectCount: number
  canUseAdvancedFeatures: boolean
  dailyCompilesUsed: number
  monthlyCompilesUsed: number
  projectCountUsed: number
  dailyReset: number
  monthlyReset: number
}

/**
 * Frontend Compilation Record structure
 */
export interface FrontendLatexCompilationRecord {
  id: number
  userId: string
  projectId: number
  documentId: string
  contentHash: string
  success: boolean
  errorMessage: string
  timestamp: number
}

/**
 * Initialize User Quota Request
 */
export interface InitializeUserQuotaRequest {
  userId: string
  tier?: 'free' | 'pro' | 'admin'
}

/**
 * Set User Quota Request
 */
export interface SetUserQuotaRequest {
  userId: string
  dailyCompileLimit?: number
  monthlyCompileLimit?: number
  maxProjectCount?: number
  canUseAdvancedFeatures?: boolean
  allowedPackages?: string[]
}

// ============================================================================
// 用户配额 API 客户端函数
// ============================================================================

/**
 * 获取用户配额信息
 */
export async function getUserQuota(userId: string): Promise<FrontendLatexUserQuota> {
  const response = await apiClient.get<BackendLatexUserQuota>(
    `${API_BASE}/quota/${userId}`
  )

  return {
    userId: response.user_id,
    dailyCompileLimit: response.daily_compile_limit,
    monthlyCompileLimit: response.monthly_compile_limit,
    maxProjectCount: response.max_project_count,
    canUseAdvancedFeatures: response.can_use_advanced_features,
    dailyCompilesUsed: response.daily_compiles_used,
    monthlyCompilesUsed: response.monthly_compiles_used,
    projectCountUsed: response.project_count_used,
    dailyReset: response.daily_reset,
    monthlyReset: response.monthly_reset
  }
}

/**
 * 设置用户配额
 */
export async function setUserQuota(request: SetUserQuotaRequest): Promise<void> {
  await apiClient.post(
    `${API_BASE}/quota`,
    {
      user_id: request.userId,
      daily_compile_limit: request.dailyCompileLimit,
      monthly_compile_limit: request.monthlyCompileLimit,
      max_project_count: request.maxProjectCount,
      can_use_advanced_features: request.canUseAdvancedFeatures,
      allowed_packages: request.allowedPackages
    }
  )
}

/**
 * 初始化用户配额
 */
export async function initializeUserQuota(request: InitializeUserQuotaRequest): Promise<void> {
  await apiClient.post(
    `${API_BASE}/quota/initialize`,
    {
      user_id: request.userId,
      tier: request.tier || 'free'
    }
  )
}

/**
 * 获取用户编译记录
 */
export async function getUserCompilationRecords(
  userId: string,
  limit: number = 100
): Promise<FrontendLatexCompilationRecord[]> {
  const response = await apiClient.get<{ records: BackendLatexCompilationRecord[]; total: number }>(
    `${API_BASE}/quota/${userId}/records?limit=${limit}`
  )

  return response.records.map(r => ({
    id: r.id,
    userId: r.user_id,
    projectId: r.project_id,
    documentId: r.document_id,
    contentHash: r.content_hash,
    success: r.success,
    errorMessage: r.error_message,
    timestamp: r.timestamp
  }))
}

// ============================================================================
// 版本控制类型定义
// ============================================================================

/**
 * Backend Version Node structure
 */
interface BackendLatexVersionNode {
  id: string
  parentId: string
  branchId: string
  branchName: string
  content: string
  summary: string
  timestamp: number
  author: string
  isAutoSave: boolean
  changeCount: number
  totalLines: number
  fileId: string
  projectId: string
  userId: string
  position: number
  depth: number
  isMerged: boolean
}

/**
 * Frontend Version Node structure
 */
export interface FrontendLatexVersionNode {
  id: string
  parentId: string
  branchId: string
  branchName: string
  content: string
  summary: string
  timestamp: Date
  author: string
  isAutoSave: boolean
  changeCount: number
  totalLines: number
  fileId: string
  projectId: string
  userId: string
  position: number
  depth: number
  isMerged: boolean
}

/**
 * Save Version Request
 */
export interface SaveLatexVersionRequest {
  fileId: number
  projectId: number
  userId: string
  content: string
  summary?: string
  isAutoSave?: boolean
}

/**
 * Restore Version Request
 */
export interface RestoreLatexVersionRequest {
  versionId: string
}

/**
 * Create Branch Request
 */
export interface CreateLatexVersionBranchRequest {
  parentVersionId: string
  branchName: string
}

/**
 * Merge Branch Request
 */
export interface MergeLatexVersionBranchRequest {
  branchId: string
}

/**
 * Version Comparison Result
 */
export interface LatexVersionComparison {
  version1: string
  version2: string
  timestamp1: number
  timestamp2: number
  summary1: string
  summary2: string
  additions: number
  deletions: number
  modifications: number
  totalChanges: number
  lineCount1: number
  lineCount2: number
}

// ============================================================================
// 版本控制 API 客户端函数
// ============================================================================

function toFrontendVersionNode(backend: BackendLatexVersionNode): FrontendLatexVersionNode {
  return {
    id: backend.id,
    parentId: backend.parentId,
    branchId: backend.branchId,
    branchName: backend.branchName,
    content: backend.content,
    summary: backend.summary,
    timestamp: new Date(backend.timestamp * 1000),
    author: backend.author,
    isAutoSave: backend.isAutoSave,
    changeCount: backend.changeCount,
    totalLines: backend.totalLines,
    fileId: backend.fileId,
    projectId: backend.projectId,
    userId: backend.userId,
    position: backend.position,
    depth: backend.depth,
    isMerged: backend.isMerged
  }
}

/**
 * 保存版本
 */
export async function saveLatexVersion(request: SaveLatexVersionRequest): Promise<FrontendLatexVersionNode> {
  const response = await apiClient.post<{ version: BackendLatexVersionNode }>(
    `${API_BASE}/versions/save`,
    {
      file_id: request.fileId,
      project_id: request.projectId,
      user_id: request.userId,
      content: request.content,
      summary: request.summary || '',
      is_auto_save: request.isAutoSave || false
    }
  )

  return toFrontendVersionNode(response.version)
}

/**
 * 获取版本历史
 */
export async function getLatexVersionHistory(
  fileId: number,
  projectId: number,
  userId: string
): Promise<FrontendLatexVersionNode[]> {
  const response = await apiClient.get<{ versions: BackendLatexVersionNode[]; total: number }>(
    `${API_BASE}/versions/history?file_id=${fileId}&project_id=${projectId}&user_id=${userId}`
  )

  return response.versions.map(toFrontendVersionNode)
}

/**
 * 获取版本树（用于分支可视化）
 */
export async function getLatexVersionTree(
  fileId: number,
  projectId: number,
  userId: string
): Promise<FrontendLatexVersionNode[]> {
  const response = await apiClient.get<{ tree: BackendLatexVersionNode[]; total: number }>(
    `${API_BASE}/versions/tree?file_id=${fileId}&project_id=${projectId}&user_id=${userId}`
  )

  return response.tree.map(toFrontendVersionNode)
}

/**
 * 恢复版本
 */
export async function restoreLatexVersion(request: RestoreLatexVersionRequest): Promise<FrontendLatexVersionNode> {
  const response = await apiClient.post<{ version: BackendLatexVersionNode }>(
    `${API_BASE}/versions/restore`,
    {
      version_id: request.versionId
    }
  )

  return toFrontendVersionNode(response.version)
}

/**
 * 创建分支
 */
export async function createLatexVersionBranch(request: CreateLatexVersionBranchRequest): Promise<FrontendLatexVersionNode> {
  const response = await apiClient.post<{ branch: BackendLatexVersionNode }>(
    `${API_BASE}/versions/branch`,
    {
      parent_version_id: request.parentVersionId,
      branch_name: request.branchName
    }
  )

  return toFrontendVersionNode(response.branch)
}

/**
 * 合并分支
 */
export async function mergeLatexVersionBranch(request: MergeLatexVersionBranchRequest): Promise<FrontendLatexVersionNode> {
  const response = await apiClient.post<{ version: BackendLatexVersionNode }>(
    `${API_BASE}/versions/merge`,
    {
      branch_id: request.branchId
    }
  )

  return toFrontendVersionNode(response.version)
}

/**
 * 删除版本
 */
export async function deleteLatexVersion(versionId: string): Promise<void> {
  await apiClient.del(`${API_BASE}/versions/${versionId}`)
}

/**
 * 比较两个版本
 */
export async function compareLatexVersions(versionId1: string, versionId2: string): Promise<LatexVersionComparison> {
  return await apiClient.get<LatexVersionComparison>(
    `${API_BASE}/versions/compare?version1=${versionId1}&version2=${versionId2}`
  )
}
