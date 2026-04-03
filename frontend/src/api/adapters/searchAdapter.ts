/**
 * 搜索数据适配器
 * 转换后端搜索数据到前端格式
 */

import type { SearchResult, SearchResultItem } from '../../types/search'

/**
 * 转换搜索结果
 */
export function adaptSearchResult(data: any): SearchResult {
  return {
    items: adaptSearchItems(data.items || data.results || []),
    page: data.page || 1,
    limit: data.limit || 20,
    total: data.total || 0,
    totalPages: data.total_pages || data.totalPages || 0,
    searchTimeMs: data.search_time_ms || data.searchTimeMs || 0,
    query: data.query || '',
    suggestions: data.suggestions || []
  }
}

/**
 * 转换搜索结果项
 */
function adaptSearchItems(items: any[]): SearchResultItem[] {
  return items.map(item => ({
    id: item.id || 0,
    type: item.type || 'paper',
    title: item.title || '',
    description: item.description || item.abstract || '',
    relevanceScore: item.relevance_score || item.relevanceScore || 0,
    highlights: item.highlights || {},
    url: item.url || `/papers/${item.id}`
  }))
}
