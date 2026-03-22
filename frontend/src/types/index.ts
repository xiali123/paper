/**
 * PaperCrawler Frontend Type Definitions
 *
 * This file contains comprehensive TypeScript type definitions for the PaperCrawler frontend.
 * All types are aligned with the backend C++ API structure and include JSDoc documentation.
 *
 * @module types
 * @version 2.0.0
 */

// ============================================================================
// ENUMERATIONS
// ============================================================================

/**
 * CCF (China Computer Federation) publication level classification
 */
export enum CCFLevel {
  /** Top-tier conferences and journals */
  A = 'A',
  /** High-quality conferences and journals */
  B = 'B',
  /** Standard conferences and journals */
  C = 'C'
}

/**
 * API response status codes
 */
export enum ApiStatusCode {
  /** Successful request */
  SUCCESS = 200,
  /** Bad request - invalid parameters */
  BAD_REQUEST = 400,
  /** Not found - resource doesn't exist */
  NOT_FOUND = 404,
  /** Internal server error */
  INTERNAL_ERROR = 500
}

/**
 * Sort order for search results
 */
export enum SortOrder {
  /** Ascending order */
  ASC = 'asc',
  /** Descending order */
  DESC = 'desc'
}

// ============================================================================
// CORE PAPER TYPES
// ============================================================================

/**
 * Journal information with full and abbreviated names
 */
export interface JournalInfo {
  /** Full journal name */
  full: string
  /** Abbreviated journal name */
  short: string
}

/**
 * URL information for a paper
 */
export interface PaperUrls {
  /** Digital Object Identifier */
  doi?: string
  /** Direct journal URL */
  journal?: string
  /** PDF download URL */
  pdf?: string
  /** Supplementary materials URL */
  supplementary?: string
}

/**
 * Core paper interface representing academic paper data
 *
 * @example
 * ```typescript
 * const paper: Paper = {
 *   id: '12345',
 *   title: 'Deep Learning for Computer Vision',
 *   journal: { full: 'CVPR', short: 'CVPR' },
 *   year: '2024',
 *   level: CCFLevel.A,
 *   authors: 'John Doe, Jane Smith',
 *   urls: { doi: '10.1109/CVPR.2024.12345' }
 * }
 * ```
 */
export interface Paper {
  /** Unique paper identifier (database ID) */
  id: number | string

  /** Paper title */
  title: string

  /** Journal/conference information */
  journal: JournalInfo | string

  /** Publication year */
  year: string | number

  /** CCF classification level */
  level: CCFLevel | string

  /** Author list (comma-separated string) */
  authors: string

  /** URLs to paper resources */
  urls?: PaperUrls

  /** Paper abstract text */
  abstract?: string

  /** Keywords/tags array */
  keywords?: string[]

  /** Citation count (for detailed views) */
  citations?: number

  /** Reference count (for detailed views) */
  references?: number

  /** Download count (for detailed views) */
  downloadCount?: number
}

/**
 * Extended paper interface with additional details
 * Used for detail views and comprehensive paper information
 */
export interface PaperDetail extends Paper {
  /** Number of times this paper has been cited */
  citations: number

  /** Number of references in the bibliography */
  references: number

  /** Download count */
  downloadCount: number

  /** Related papers based on similarity */
  relatedPapers?: Paper[]

  /** Publication date (full date if available) */
  publicationDate?: string

  /** DOI identifier */
  doi?: string

  /** Pages in the journal/conference */
  pages?: string

  /** Volume number */
  volume?: string

  /** Issue number */
  issue?: string

  /** Publisher name */
  publisher?: string
}

/**
 * Simplified paper interface for list views
 * Optimized for rendering in tables and grids
 */
export interface PaperListItem {
  /** Unique paper identifier */
  id: number | string

  /** Paper title (possibly truncated) */
  title: string

  /** Journal name (short form) */
  journal: string

  /** Publication year */
  year: string | number

  /** CCF level */
  level: string

  /** Authors (truncated if too long) */
  authors: string
}

// ============================================================================
// SEARCH AND QUERY TYPES
// ============================================================================

/**
 * Search parameters for paper search API
 *
 * @example
 * ```typescript
 * const params: SearchParams = {
 *   q: 'machine learning',
 *   year: '2024',
 *   level: CCFLevel.A,
 *   offset: 0,
 *   limit: 20
 * }
 * ```
 */
export interface SearchParams {
  /** Search query string */
  q: string

  /** Filter by publication year */
  year?: string

  /** Filter by CCF level */
  level?: string

  /** Pagination offset (number of items to skip) */
  offset?: number

  /** Maximum number of results to return */
  limit?: number

  /** Sort field (e.g., 'year', 'citations', 'title') */
  sort?: string

  /** Sort order */
  order?: SortOrder

  /** Filter by journal name */
  journal?: string

  /** Filter by author name */
  author?: string
}

/**
 * Search result response
 */
export interface SearchResult {
  /** Array of matching papers */
  papers: Paper[]

  /** Total number of matching results */
  total: number

  /** Search keyword used */
  keyword: string

  /** Search duration in milliseconds */
  duration: number

  /** Current page number (if using page-based pagination) */
  page?: number

  /** Page size (if using page-based pagination) */
  pageSize?: number

  /** Current offset (if using offset-based pagination) */
  offset?: number

  /** Current limit (if using offset-based pagination) */
  limit?: number
}

// ============================================================================
// STATISTICS TYPES
// ============================================================================

/**
 * Overview statistics for the dashboard
 */
export interface Statistics {
  /** Total number of papers in the database */
  totalPapers: number

  /** Total number of unique journals/conferences */
  totalJournals: number

  /** Number of top-tier (CCF A) papers */
  topTierPapers: number

  /** Number of papers published in the last year */
  papersLastYear: number

  /** Name of the most active journal */
  mostActiveJournal: string

  /** Average papers published per year */
  averagePapersPerYear?: number

  /** Timestamp of the latest data update */
  latestUpdate?: string

  /** Year range covered (e.g., "2010-2024") */
  yearRange?: string
}

/**
 * Per-journal statistics
 */
export interface JournalStats {
  /** Journal/conference name */
  journal: string

  /** Number of papers from this journal */
  count: number

  /** CCF level of the journal */
  level: string

  /** Percentage of total papers */
  percentage?: number

  /** Ranking among all journals */
  rank?: number
}

/**
 * Per-year statistics
 */
export interface YearStats {
  /** Publication year */
  year: string

  /** Total papers published this year */
  count: number

  /** Number of CCF A papers */
  aCount?: number

  /** Number of CCF B papers */
  bCount?: number

  /** Number of CCF C papers */
  cCount?: number

  /** Growth rate compared to previous year */
  growth?: number
}

/**
 * Author statistics
 */
export interface AuthorStats {
  /** Author name */
  author: string

  /** Number of papers by this author */
  count: number

  /** Total citations for all papers */
  totalCitations?: number

  /** Average CCF level */
  avgLevel?: string

  /** Most frequent journal */
  topJournal?: string
}

/**
 * Real-time search statistics for sync updates
 */
export interface SearchStats {
  /** Total number of papers */
  totalPapers: number

  /** Total number of journals */
  totalJournals: number

  /** Number of CCF A papers */
  aPapers: number

  /** Number of CCF B papers */
  bPapers: number

  /** Number of CCF C papers */
  cPapers: number

  /** Last update timestamp */
  lastUpdate: string

  /** Update version number */
  version: number
}

// ============================================================================
// API RESPONSE TYPES
// ============================================================================

/**
 * Standard API response wrapper
 *
 * @template T - Type of data payload
 */
export interface ApiResponse<T = any> {
  /** HTTP status code */
  code: number

  /** Response message */
  message: string

  /** Response data payload */
  data: T

  /** Response timestamp */
  timestamp?: number
}

/**
 * Alternative API response format with success flag
 *
 * @template T - Type of data payload
 */
export interface ApiResponseV2<T = any> {
  /** Success flag */
  success: boolean

  /** Response data payload */
  data: T

  /** Optional error message */
  message?: string

  /** Response timestamp */
  timestamp: number
}

/**
 * API error response
 */
export interface ApiError {
  /** Success flag (always false) */
  success: false

  /** Error message */
  error: string

  /** Error code */
  code?: string

  /** HTTP status code */
  statusCode?: number

  /** Error timestamp */
  timestamp: number

  /** Request ID for tracking */
  requestId?: string
}

/**
 * Pagination parameters
 */
export interface PaginationParams {
  /** Page number (1-indexed) */
  page: number

  /** Number of items per page */
  pageSize: number
}

/**
 * Pagination parameters using offset/limit
 */
export interface OffsetPaginationParams {
  /** Number of items to skip */
  offset: number

  /** Maximum number of items to return */
  limit: number
}

/**
 * Paginated response wrapper
 *
 * @template T - Type of items in the collection
 */
export interface PaginatedResponse<T> {
  /** Array of items */
  items: T[]

  /** Total number of items (across all pages) */
  total: number

  /** Current page number */
  page: number

  /** Number of items per page */
  pageSize: number

  /** Total number of pages */
  totalPages: number

  /** Whether there's a next page */
  hasNext: boolean

  /** Whether there's a previous page */
  hasPrevious: boolean
}

// ============================================================================
// STATE MANAGEMENT TYPES
// ============================================================================

/**
 * Loading state for async operations
 */
export interface LoadingState {
  /** Loading flag */
  loading: boolean

  /** Error message if any */
  error: string | null

  /** Last update timestamp */
  lastUpdated?: number
}

/**
 * Paper list state with loading and pagination
 */
export interface PaperListState extends LoadingState {
  /** Array of papers */
  papers: Paper[]

  /** Total number of papers */
  total: number

  /** Current page */
  page: number

  /** Page size */
  pageSize: number

  /** Has more pages */
  hasMore: boolean
}

/**
 * Statistics state
 */
export interface StatisticsState extends LoadingState {
  /** Overview statistics */
  overview: Statistics | null

  /** Journal statistics */
  journals: JournalStats[]

  /** Year statistics */
  years: YearStats[]

  /** Author statistics */
  authors: AuthorStats[]
}

// ============================================================================
// UTILITY TYPES
// ============================================================================

/**
 * Makes all properties in T optional recursively
 */
export type DeepPartial<T> = {
  [P in keyof T]?: T[P] extends object ? DeepPartial<T[P]> : T[P]
}

/**
 * Makes all properties in T required recursively
 */
export type DeepRequired<T> = {
  [P in keyof T]-?: T[P] extends object ? DeepRequired<T[P]> : T[P]
}

/**
 * Picks only specified keys from T
 */
export type PickPartial<T, K extends keyof T> = {
  [P in K]?: T[P]
}

/**
 * Creates a type with only the required properties of T
 */
export type OnlyRequired<T> = {
  [P in keyof T]-?: {}
} extends { [P in keyof T]-?: T[P] } ? T : never

/**
 * Mutable version of Readonly type
 */
export type Mutable<T> = {
  -readonly [P in keyof T]: T[P]
}

/**
 * Array element type
 */
export type ArrayElement<T> = T extends (infer U)[] ? U : never

// ============================================================================
// TYPE GUARDS
// ============================================================================

/**
 * Type guard to check if a value is a valid Paper
 */
export function isPaper(value: unknown): value is Paper {
  return (
    typeof value === 'object' &&
    value !== null &&
    'id' in value &&
    'title' in value &&
    'journal' in value &&
    'year' in value &&
    'level' in value &&
    'authors' in value
  )
}

/**
 * Type guard to check if a value is a PaperDetail
 */
export function isPaperDetail(value: unknown): value is PaperDetail {
  return (
    isPaper(value) &&
    'citations' in value &&
    'references' in value &&
    'downloadCount' in value
  )
}

/**
 * Type guard to check if an API response is successful
 */
export function isSuccessResponse<T>(response: ApiResponse<T> | ApiError): response is ApiResponse<T> {
  return 'code' in response && response.code >= 200 && response.code < 300
}

/**
 * Type guard to check if a value is a valid CCF level
 */
export function isValidCCFLevel(value: string): value is CCFLevel {
  return Object.values(CCFLevel).includes(value as CCFLevel)
}

/**
 * Type guard to check if journal info is in detailed format
 */
export function isDetailedJournal(info: JournalInfo | string): info is JournalInfo {
  return typeof info === 'object' && 'full' in info && 'short' in info
}

// ============================================================================
// VALIDATION FUNCTIONS
// ============================================================================

/**
 * Validates search parameters
 */
export function validateSearchParams(params: SearchParams): { valid: boolean; errors: string[] } {
  const errors: string[] = []

  if (!params.q || params.q.trim().length === 0) {
    errors.push('Search query is required')
  }

  if (params.offset !== undefined && params.offset < 0) {
    errors.push('Offset must be non-negative')
  }

  if (params.limit !== undefined && (params.limit < 1 || params.limit > 100)) {
    errors.push('Limit must be between 1 and 100')
  }

  if (params.level && !isValidCCFLevel(params.level)) {
    errors.push('Invalid CCF level')
  }

  return {
    valid: errors.length === 0,
    errors
  }
}

/**
 * Validates paper data
 */
export function validatePaper(paper: Partial<Paper>): { valid: boolean; errors: string[] } {
  const errors: string[] = []

  if (!paper.title || paper.title.trim().length === 0) {
    errors.push('Paper title is required')
  }

  if (!paper.authors || paper.authors.trim().length === 0) {
    errors.push('Authors are required')
  }

  if (!paper.year) {
    errors.push('Publication year is required')
  }

  if (paper.level && !isValidCCFLevel(paper.level.toString())) {
    errors.push('Invalid CCF level')
  }

  return {
    valid: errors.length === 0,
    errors
  }
}

// ============================================================================
// TYPE CONVERTERS
// ============================================================================

/**
 * Converts paper to list item format
 */
export function paperToListItem(paper: Paper): PaperListItem {
  return {
    id: paper.id,
    title: paper.title,
    journal: typeof paper.journal === 'string' ? paper.journal : paper.journal.short,
    year: paper.year,
    level: paper.level.toString(),
    authors: paper.authors
  }
}

/**
 * Normalizes journal info to string
 */
export function journalToString(journal: JournalInfo | string): string {
  if (typeof journal === 'string') {
    return journal
  }
  return journal.full
}

/**
 * Normalizes year to string
 */
export function yearToString(year: string | number): string {
  return year.toString()
}

/**
 * Converts search params to query string
 */
export function searchParamsToQuery(params: SearchParams): string {
  const query = new URLSearchParams()

  if (params.q) query.append('q', params.q)
  if (params.year) query.append('year', params.year)
  if (params.level) query.append('level', params.level)
  if (params.offset !== undefined) query.append('offset', params.offset.toString())
  if (params.limit !== undefined) query.append('limit', params.limit.toString())
  if (params.sort) query.append('sort', params.sort)
  if (params.order) query.append('order', params.order)
  if (params.journal) query.append('journal', params.journal)
  if (params.author) query.append('author', params.author)

  return query.toString()
}

// ============================================================================
// EXPORTS
// ============================================================================

// Re-export enums for convenience
export { CCFLevel, ApiStatusCode, SortOrder }

// Type aliases for common patterns
export type PaperWithDetails = PaperDetail
export type PaperInList = PaperListItem
export type SearchQuery = SearchParams
export type StatsResponse = ApiResponse<Statistics>
export type PapersResponse = ApiResponse<PaginatedResponse<Paper>>
export type SearchResponse = ApiResponse<SearchResult>

// Export utility functions
export {
  toListItem,
  toListItems,
  extractJournalName,
  extractFullJournalName,
  normalizeYear,
  normalizeYearNumber,
  isValidYear,
  extractCCFLevel,
  formatAuthors,
  formatCitationCount,
  hasKeywords,
  hasAbstract,
  calculatePaperAge,
  sortByYear,
  sortByCitations,
  filterByLevel,
  filterByYearRange,
  filterByJournal,
  searchPapers,
  groupByYear,
  groupByLevel,
  groupByJournal,
  countByYear,
  countByLevel,
  getUniqueJournals,
  getYearRange,
  getUniqueYears,
  calculateAveragePapersPerYear,
  calculateLevelPercentage,
  findMostProductiveYear,
  papersToCSV,
  downloadAsFile,
  downloadPapersAsCSV
} from './utils'

// Export validation functions
export {
  validatePaper,
  validatePaperDetail,
  validateSearchParams,
  validateStatistics,
  validateJournalStats,
  validateYearStats,
  validateApiResponse,
  validateMultiple,
  assertValid,
  safeValidate
} from './validation'

// Default export containing all types
export default {
  // Enums
  CCFLevel,
  ApiStatusCode,
  SortOrder,

  // Core types
  Paper,
  PaperDetail,
  PaperListItem,
  JournalInfo,
  PaperUrls,

  // Search types
  SearchParams,
  SearchResult,

  // Statistics types
  Statistics,
  JournalStats,
  YearStats,
  AuthorStats,

  // API response types
  ApiResponse,
  ApiResponseV2,
  ApiError,
  PaginatedResponse,

  // State types
  LoadingState,
  PaperListState,
  StatisticsState,

  // Utilities
  isPaper,
  isPaperDetail,
  isSuccessResponse,
  isValidCCFLevel,
  validateSearchParams,
  validatePaper,
  paperToListItem,
  journalToString,
  yearToString,
  searchParamsToQuery
}
