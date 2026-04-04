/**
 * 搜索类型定义
 * Search Type Definitions
 */

// ============================================================================
// ENUMS
// ============================================================================

/**
 * 搜索类型 / Search Type
 */
export enum SearchType {
  PAPERS = 'papers',
  AUTHORS = 'authors',
  KEYWORDS = 'keywords',
  FULLTEXT = 'fulltext',
  ADVANCED = 'advanced'
}

/**
 * 排序方式 / Sort Order
 */
export enum SortOrder {
  RELEVANCE = 'relevance',
  DATE_DESC = 'date_desc',
  DATE_ASC = 'date_asc',
  CITATION_DESC = 'citation_desc',
  TITLE_ASC = 'title_asc'
}

// ============================================================================
// BASIC SEARCH TYPES
// ============================================================================

/**
 * 搜索参数 / Search Parameters
 */
export interface SearchParams {
  q?: string // Query string
  page?: number
  pageSize?: number
  limit?: number
  sortBy?: SortBy
  sortOrder?: 'asc' | 'desc'
  year?: string
  level?: string
  offset?: number
}

/**
 * 排序选项 / Sort By Options
 */
export type SortBy =
  | 'relevance'
  | 'date'
  | 'citations'
  | 'downloads'
  | 'title'

/**
 * 搜索结果项 / Search Result Item
 */
export interface SearchResultItem {
  id: number
  type: string
  title: string
  description: string
  relevanceScore: number
  highlights: { [key: string]: string }
  url: string
}

/**
 * 搜索结果 / Search Result
 */
export interface SearchResult {
  papers: Paper[]
  total: number
  page: number
  pageSize: number
  duration?: number
  facets?: SearchFacets
  items?: SearchResultItem[]
  limit?: number
  totalPages?: number
  searchTimeMs?: number
  query?: string
  suggestions?: string[]
}

/**
 * 搜索分面 / Search Facets
 */
export interface SearchFacets {
  years?: FacetItem[]
  ccfLevels?: FacetItem[]
  journals?: FacetItem[]
  authors?: FacetItem[]
}

/**
 * 分面项 / Facet Item
 */
export interface FacetItem {
  value: string
  count: number
  label?: string
}

// ============================================================================
// ADVANCED SEARCH TYPES
// ============================================================================

/**
 * 高级搜索查询 / Advanced Search Query
 */
export interface AdvancedSearchQuery {
  query?: string
  title?: string
  author?: string
  authors?: string
  abstract?: string
  journal?: string
  keywords?: string
  doi?: string
  yearFrom?: number
  yearTo?: number
  year?: string
  citationsMin?: number
  citationsFrom?: number
  citationsTo?: number
  ccfLevel?: string
  ccfLevels?: string[]
  sortBy?: SortOrder
  must?: SearchClause[]
  should?: SearchClause[]
  mustNot?: SearchClause[]
  filters?: SearchFilter[]
}

/**
 * 搜索子句 / Search Clause
 */
export interface SearchClause {
  field: SearchField
  operator: SearchOperator
  value: string
}

/**
 * 搜索字段 / Search Field
 */
export type SearchField =
  | 'title'
  | 'authors'
  | 'abstract'
  | 'keywords'
  | 'journal'
  | 'year'
  | 'doi'
  | 'fulltext'

/**
 * 搜索操作符 / Search Operator
 */
export type SearchOperator =
  | 'contains'
  | 'equals'
  | 'startsWith'
  | 'endsWith'
  | 'regex'

/**
 * 搜索过滤器 / Search Filter
 */
export interface SearchFilter {
  field: FilterField
  value: string | number | string[]
  operator?: FilterOperator
}

/**
 * 过滤字段 / Filter Field
 */
export type FilterField =
  | 'year'
  | 'yearFrom'
  | 'yearTo'
  | 'ccfLevel'
  | 'ccfLevels'
  | 'citations'
  | 'citationsFrom'
  | 'citationsTo'
  | 'journal'
  | 'paperType'
  | 'language'
  | 'source'

/**
 * 过滤操作符 / Filter Operator
 */
export type FilterOperator = 'eq' | 'ne' | 'gt' | 'lt' | 'gte' | 'lte' | 'in'

// ============================================================================
// SUGGESTION TYPES
// ============================================================================

/**
 * 搜索建议 / Search Suggestion
 */
export interface SearchSuggestion {
  text: string
  frequency?: number
  type?: string
  count?: number
}

// ============================================================================
// HISTORY TYPES
// ============================================================================

/**
 * 搜索历史项 / Search History Item
 */
export interface SearchHistoryItem {
  query: string
  timestamp: number
  resultCount: number
  params?: SearchParams
}

// ============================================================================
// TRENDING TYPES
// ============================================================================

/**
 * 热门搜索 / Trending Search
 */
export interface TrendingSearch {
  query: string
  count: number
  trend: 'up' | 'down' | 'stable'
  change?: number
}

// ============================================================================
// EXPORT TYPES
// ============================================================================

/**
 * 导出格式 / Export Format
 */
export type ExportFormat = 'csv' | 'json' | 'excel' | 'bibtex' | 'endnote'

/**
 * 导出结果 / Export Result
 */
export interface ExportResult {
  url: string
  filename: string
  format: ExportFormat
  size?: number
}

// ============================================================================
// PAPER TYPES (for search results)
// ============================================================================

/**
 * 论文信息 / Paper Information (Simplified for Search)
 */
export interface Paper {
  id: number
  title: string
  authors: string[]
  abstract?: string
  keywords?: string[]
  year: number
  journal: JournalInfo
  ccf_level?: 'A' | 'B' | 'C'
  citation_count: number
  downloads?: number
  doi?: string
  url?: string
  pdfUrl?: string
  source?: string
  paperType?: 'journal' | 'conference' | 'preprint'
  language?: string
  status?: string
  level?: string
  venue?: string
  publishDate?: string
  urls?: {
    doi?: string
    pdf?: string
  }
}

/**
 * 期刊信息 / Journal Information
 */
export interface JournalInfo {
  full?: string
  short?: string
  volume?: string
  issue?: string
  pages?: string
}
