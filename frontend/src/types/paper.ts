// 论文基础接口
export interface Paper {
  id: number
  title: string
  journal: {
    full: string
    short: string
  }
  year: string
  level: 'A' | 'B' | 'C' | null
  authors: string | string[]
  urls?: {
    doi?: string
    journal?: string
  }
  abstract?: string
  keywords?: string[]
}

// 论文详情接口
export interface PaperDetail extends Paper {
  citations?: number
  references?: number
  downloadCount?: number
  relatedPapers?: Paper[]
}

// 搜索结果接口
export interface SearchResult {
  papers: Paper[]
  total: number
  keyword: string
  duration: number  // 搜索耗时（毫秒）
  page?: number
  pageSize?: number
}

// 统计信息接口
export interface Statistics {
  totalPapers: number
  totalJournals: number
  topTierPapers: number
  papersLastYear: number
  mostActiveJournal: string
  latestUpdate?: string
}

// 期刊统计接口
export interface JournalStats {
  journal: string
  count: number
  level: string
  percentage: number
}

// 年度统计接口
export interface YearStats {
  year: string
  count: number
  growth?: number
}

// 搜索参数接口（重新导出）
export interface SearchParams {
  q: string
  year?: string
  level?: string
  offset?: number
  limit?: number
}

// API 响应包装接口
export interface ApiResponse<T = any> {
  code: number
  message: string
  data: T
}

// 分页参数接口
export interface PaginationParams {
  page: number
  pageSize: number
}

// 分页响应接口
export interface PaginatedResponse<T> {
  items: T[]
  total: number
  page: number
  pageSize: number
  totalPages: number
}

// 加载状态接口
export interface LoadingState {
  loading: boolean
  error: string | null
}

// 论文列表项接口（用于列表展示）
export interface PaperListItem {
  id: number
  title: string
  journal: string
  year: string
  level: string | null
  authors: string | string[]
}
