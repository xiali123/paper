/**
 * Paper Adapter
 *
 * Handles transformation between backend Paper objects (C++/JSON) and frontend Paper objects (TypeScript).
 *
 * Key transformations:
 * - Field name mappings: journal↔publication, isFavorite↔isBookmarked
 * - Type conversions: year (int↔string), tags/keywords (array↔comma-separated string)
 * - Timestamp conversions: chrono::time_point↔ISO 8601 string
 * - Frontend-specific fields: userId, source, category, readingProgress
 *
 * @module api/adapters/paperAdapter
 */

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * Backend Paper structure (matches C++ Paper struct)
 */
interface BackendPaper {
  id: number
  title: string
  authors: string
  year: number
  abstract: string
  journal: string
  volume: string
  issue: string
  pages: string
  doi: string
  url: string
  pdf_path: string
  created_at: string  // ISO 8601 or Unix timestamp
  updated_at: string  // ISO 8601 or Unix timestamp
  tags: string[]      // Array of tags
  keywords: string[]  // Array of keywords
  citation_count: number
  is_read: boolean
  is_favorite: boolean
  notes: string
}

/**
 * Backend Paper request structure (for create/update operations)
 */
interface BackendPaperRequest {
  title?: string
  authors?: string
  year?: number
  abstract?: string
  journal?: string
  volume?: string
  issue?: string
  pages?: string
  doi?: string
  url?: string
  pdf_path?: string
  tags?: string[]
  keywords?: string[]
  citation_count?: number
  is_read?: boolean
  is_favorite?: boolean
  notes?: string
}

/**
 * Frontend Paper structure (from papers.ts)
 */
export interface FrontendPaper {
  id: number
  userId: number
  title: string
  authors: string
  abstract: string
  keywords: string
  doi: string
  publication: string
  year: string
  volume: string
  issue: string
  pages: string
  url: string
  pdfPath: string
  source: 'manual' | 'cnki' | 'ieee' | 'arxiv' | 'pubmed'
  category: string
  tags: string
  citationCount: number
  isRead: boolean
  isBookmarked: boolean
  readingProgress: number
  notes: string
  createdAt: string
  updatedAt: string
}

/**
 * Frontend Paper query parameters
 */
export interface PaperQueryParams {
  keyword?: string
  category?: string
  tags?: string
  source?: string
  isRead?: boolean
  isBookmarked?: boolean
  orderBy?: string
  order?: 'ASC' | 'DESC'
  page?: number
  pageSize?: number
}

/**
 * Frontend create paper request
 */
export interface CreatePaperRequest {
  title: string
  authors?: string
  abstract?: string
  keywords?: string
  doi?: string
  publication?: string
  year?: string
  volume?: string
  issue?: string
  pages?: string
  url?: string
  pdfPath?: string
  source?: string
  category?: string
  tags?: string
}

/**
 * Frontend update paper request
 */
export interface UpdatePaperRequest {
  title?: string
  authors?: string
  abstract?: string
  keywords?: string
  doi?: string
  publication?: string
  year?: string
  volume?: string
  issue?: string
  pages?: string
  url?: string
  pdfPath?: string
  source?: string
  category?: string
  tags?: string
  citationCount?: number
  isRead?: boolean
  isBookmarked?: boolean
  readingProgress?: number
  notes?: string
}

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Convert timestamp string to ISO 8601 format
 */
function toISO8601(timestamp: string | number | undefined | null): string {
  if (!timestamp) {
    return new Date().toISOString()  // Fallback to current time
  }
  if (typeof timestamp === 'number') {
    return new Date(timestamp * 1000).toISOString()
  }
  const date = new Date(timestamp)
  if (isNaN(date.getTime())) {
    return new Date().toISOString()  // Fallback to current time
  }
  return date.toISOString()
}

/**
 * Convert comma-separated string to array
 */
function stringToArray(str: string | undefined): string[] {
  if (!str || str.trim() === '') {
    return []
  }
  return str.split(',').map(s => s.trim()).filter(s => s.length > 0)
}

/**
 * Convert array to comma-separated string
 */
function arrayToString(arr: string[] | undefined): string {
  if (!arr || arr.length === 0) {
    return ''
  }
  return arr.join(', ')
}

/**
 * Convert year string to number
 */
function stringToNumber(year: string | undefined): number {
  if (!year) {
    return 0
  }
  const num = parseInt(year, 10)
  return isNaN(num) ? 0 : num
}

/**
 * Convert year number to string
 */
function numberToString(year: number | undefined): string {
  if (year === undefined || year === null) {
    return ''
  }
  return year.toString()
}

// ============================================================================
// Transformation Functions
// ============================================================================

/**
 * Transform backend Paper to frontend Paper
 *
 * @param backendPaper - Backend Paper object
 * @returns Frontend Paper object
 */
export function toFrontendPaper(backendPaper: any): FrontendPaper {
  if (!backendPaper) {
    throw new Error('toFrontendPaper: backendPaper is null or undefined')
  }

  // Debug logging
  console.log('[toFrontendPaper] Input:', backendPaper)

  return {
    id: backendPaper.id || 0,
    userId: 0,  // Backend doesn't provide userId, set to default
    title: backendPaper.title || '',
    authors: backendPaper.authors || '',
    abstract: backendPaper.abstract || '',
    keywords: arrayToString(backendPaper.keywords),
    doi: backendPaper.doi || '',
    publication: backendPaper.journal || '',
    year: numberToString(backendPaper.year),
    volume: backendPaper.volume || '',
    issue: backendPaper.issue || '',
    pages: backendPaper.pages || '',
    url: backendPaper.url || '',
    pdfPath: backendPaper.pdf_path || '',
    source: 'manual',  // Backend doesn't provide source, set to default
    category: '',  // Backend doesn't provide category, set to default
    tags: arrayToString(backendPaper.tags),
    citationCount: backendPaper.citation_count || 0,
    isRead: backendPaper.is_read || false,
    isBookmarked: backendPaper.is_favorite || false,
    readingProgress: 0,  // Backend doesn't provide readingProgress, set to default
    notes: backendPaper.notes || '',
    createdAt: toISO8601(backendPaper.created_at),
    updatedAt: toISO8601(backendPaper.updated_at)
  }
}

/**
 * Transform array of backend Papers to frontend Papers
 *
 * @param backendPapers - Array of backend Paper objects
 * @returns Array of frontend Paper objects
 */
export function transformPaperList(backendPapers: BackendPaper[]): FrontendPaper[] {
  return backendPapers.map(paper => toFrontendPaper(paper))
}

/**
 * Transform frontend Paper to backend Paper request format
 *
 * @param frontendPaper - Frontend Paper object (partial)
 * @returns Backend Paper request object
 */
export function toBackendPaper(frontendPaper: Partial<FrontendPaper>): BackendPaperRequest {
  const backendRequest: BackendPaperRequest = {}

  // Transform basic fields
  if (frontendPaper.title !== undefined) backendRequest.title = frontendPaper.title
  if (frontendPaper.authors !== undefined) backendRequest.authors = frontendPaper.authors
  if (frontendPaper.abstract !== undefined) backendRequest.abstract = frontendPaper.abstract

  // Transform year: string → number
  if (frontendPaper.year !== undefined) {
    backendRequest.year = stringToNumber(frontendPaper.year)
  }

  // Transform field name: publication → journal
  if (frontendPaper.publication !== undefined) {
    backendRequest.journal = frontendPaper.publication
  }

  // Transform other fields
  if (frontendPaper.volume !== undefined) backendRequest.volume = frontendPaper.volume
  if (frontendPaper.issue !== undefined) backendRequest.issue = frontendPaper.issue
  if (frontendPaper.pages !== undefined) backendRequest.pages = frontendPaper.pages
  if (frontendPaper.doi !== undefined) backendRequest.doi = frontendPaper.doi
  if (frontendPaper.url !== undefined) backendRequest.url = frontendPaper.url

  // Transform field name: pdfPath → pdf_path
  if (frontendPaper.pdfPath !== undefined) {
    backendRequest.pdf_path = frontendPaper.pdfPath
  }

  // Transform keywords: string → array
  if (frontendPaper.keywords !== undefined) {
    backendRequest.keywords = stringToArray(frontendPaper.keywords)
  }

  // Transform tags: string → array
  if (frontendPaper.tags !== undefined) {
    backendRequest.tags = stringToArray(frontendPaper.tags)
  }

  // Transform field name: citationCount → citation_count
  if (frontendPaper.citationCount !== undefined) {
    backendRequest.citation_count = frontendPaper.citationCount
  }

  // Transform field name: isRead → is_read
  if (frontendPaper.isRead !== undefined) {
    backendRequest.is_read = frontendPaper.isRead
  }

  // Transform field name: isBookmarked → is_favorite
  if (frontendPaper.isBookmarked !== undefined) {
    backendRequest.is_favorite = frontendPaper.isBookmarked
  }

  // Transform notes
  if (frontendPaper.notes !== undefined) {
    backendRequest.notes = frontendPaper.notes
  }

  // Omit frontend-specific fields: userId, source, category, readingProgress
  // These are not sent to the backend

  return backendRequest
}

/**
 * Transform create paper request to backend format
 *
 * @param data - Frontend create paper request
 * @returns Backend paper request object
 */
export function transformCreateRequest(data: CreatePaperRequest): BackendPaperRequest {
  // Title is required
  if (!data.title || data.title.trim() === '') {
    throw new Error('Title is required for creating a paper')
  }

  const backendRequest = toBackendPaper(data)

  // Ensure title is set (it's required)
  backendRequest.title = data.title.trim()

  return backendRequest
}

/**
 * Transform update paper request to backend format
 *
 * @param data - Frontend update paper request
 * @returns Backend paper request object
 */
export function transformUpdateRequest(data: UpdatePaperRequest): BackendPaperRequest {
  const backendRequest = toBackendPaper(data)

  // Remove undefined values to avoid overwriting with null/empty
  Object.keys(backendRequest).forEach(key => {
    if (backendRequest[key as keyof BackendPaperRequest] === undefined) {
      delete backendRequest[key as keyof BackendPaperRequest]
    }
  })

  return backendRequest
}

/**
 * Transform query parameters to backend format
 *
 * @param params - Frontend query parameters
 * @returns Backend query parameters object
 */
export function transformQueryParams(params: PaperQueryParams): Record<string, any> {
  const backendParams: Record<string, any> = {}

  // Transform keyword search - support both 'keyword' and 'q' fields
  const searchQuery = params.keyword ?? (params as any).q
  if (searchQuery !== undefined) {
    backendParams.q = searchQuery
  }

  // Transform basic filters
  if (params.category !== undefined) {
    backendParams.category = params.category
  }

  if (params.tags !== undefined) {
    backendParams.tags = stringToArray(params.tags)
  }

  if (params.source !== undefined) {
    backendParams.source = params.source
  }

  // Transform field name: isRead → is_read
  if (params.isRead !== undefined) {
    backendParams.is_read = params.isRead
  }

  // Transform field name: isBookmarked → is_favorite
  if (params.isBookmarked !== undefined) {
    backendParams.is_favorite = params.isBookmarked
  }

  // Transform sorting
  if (params.orderBy !== undefined) {
    // Convert camelCase to snake_case for backend
    const orderByMap: Record<string, string> = {
      'createdAt': 'created_at',
      'updatedAt': 'updated_at',
      'title': 'title',
      'year': 'year',
      'citationCount': 'citation_count'
    }
    backendParams.sort_by = orderByMap[params.orderBy] || params.orderBy
  }

  if (params.order !== undefined) {
    backendParams.ascending = params.order === 'ASC'
  }

  // Transform pagination
  if (params.page !== undefined) {
    backendParams.page = params.page
  }

  if (params.pageSize !== undefined) {
    backendParams.limit = params.pageSize
  }

  // Filter out undefined values
  Object.keys(backendParams).forEach(key => {
    if (backendParams[key] === undefined) {
      delete backendParams[key]
    }
  })

  return backendParams
}

// ============================================================================
// Re-exports for convenience
// ============================================================================

export type {
  BackendPaper,
  BackendPaperRequest,
  FrontendPaper,
  PaperQueryParams,
  CreatePaperRequest,
  UpdatePaperRequest
}
