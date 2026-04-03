/**
 * 搜索类型定义
 */

/**
 * 搜索类型
 */
export enum SearchType {
  PAPERS = 'papers',
  AUTHORS = 'authors',
  KEYWORDS = 'keywords',
  FULLTEXT = 'fulltext',
  ADVANCED = 'advanced'
}

/**
 * 排序方式
 */
export enum SortOrder {
  RELEVANCE = 'relevance',
  DATE_DESC = 'date_desc',
  DATE_ASC = 'date_asc',
  CITATION_DESC = 'citation_desc',
  TITLE_ASC = 'title_asc'
}

/**
 * 搜索结果项
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
 * 搜索结果
 */
export interface SearchResult {
  items: SearchResultItem[]
  page: number
  limit: number
  total: number
  totalPages: number
  searchTimeMs: number
  query: string
  suggestions: string[]
}

/**
 * 高级搜索查询
 */
export interface AdvancedSearchQuery {
  query: string
  title?: string
  author?: string
  abstract?: string
  journal?: string
  keywords?: string
  doi?: string
  yearFrom?: number
  yearTo?: number
  citationsMin?: number
  sortBy?: SortOrder
}

/**
 * 搜索建议
 */
export interface SearchSuggestion {
  text: string
  frequency: number
  type: string
}
