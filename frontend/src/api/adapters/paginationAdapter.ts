/**
 * 分页数据适配器
 * 处理前后端分页数据的格式转换
 *
 * 主要转换：
 * - pageSize <-> limit (每页数量)
 * - sortBy <-> sort_by (排序字段)
 * - sortOrder <-> sort_order (排序方向)
 * - 计算分页元数据（总页数、是否有下一页等）
 *
 * @module api/adapters/paginationAdapter
 */

// ============================================================================
// 类型定义
// ============================================================================

/**
 * 前端分页参数格式
 */
export interface FrontendPaginationParams {
  /** 页码（从1开始） */
  page?: number

  /** 每页数量 */
  pageSize?: number

  /** 排序字段（camelCase） */
  sortBy?: string

  /** 排序方向 */
  sortOrder?: 'ASC' | 'DESC' | 'asc' | 'desc'

  /** 搜索关键词 */
  keyword?: string

  /** 分类过滤 */
  category?: string

  /** 标签过滤 */
  tags?: string

  /** 来源过滤 */
  source?: string

  /** 是否已读 */
  isRead?: boolean

  /** 是否收藏 */
  isBookmarked?: boolean
}

/**
 * 后端分页参数格式（snake_case）
 */
export interface BackendPaginationParams {
  /** 页码（从1开始） */
  page?: number

  /** 每页数量（后端使用 limit） */
  limit?: number

  /** 排序字段（snake_case） */
  sort_by?: string

  /** 排序方向 */
  sort_order?: 'ASC' | 'DESC'

  /** 搜索关键词 */
  keyword?: string

  /** 分类过滤 */
  category?: string

  /** 标签过滤 */
  tags?: string

  /** 来源过滤 */
  source?: string

  /** 是否已读 */
  is_read?: boolean

  /** 是否收藏 */
  is_bookmarked?: boolean
}

/**
 * 前端分页元数据
 */
export interface FrontendPaginationMeta {
  /** 当前页码 */
  page: number

  /** 每页数量 */
  pageSize: number

  /** 总记录数 */
  total: number

  /** 总页数 */
  totalPages: number

  /** 是否有上一页 */
  hasPrevious: boolean

  /** 是否有下一页 */
  hasNext: boolean

  /** 上一页页码 */
  previousPage?: number

  /** 下一页页码 */
  nextPage?: number
}

/**
 * 后端分页响应格式
 */
export interface BackendPaginationResponse<T> {
  /** 数据列表 */
  data: T[]

  /** 总记录数 */
  total: number

  /** 当前页码 */
  page: number

  /** 每页数量（后端使用 limit） */
  limit: number
}

/**
 * 前端分页响应格式
 */
export interface FrontendPaginationResponse<T> {
  /** 数据列表 */
  data: T[]

  /** 分页元数据 */
  meta: FrontendPaginationMeta
}

// ============================================================================
// 请求转换函数（前端 -> 后端）
// ============================================================================

/**
 * 前端分页参数转后端格式
 *
 * 转换规则：
 * - `pageSize` → `limit`
 * - `sortBy` → `sort_by` (camelCase 转 snake_case)
 * - `sortOrder` → `sort_order` (camelCase 转 snake_case)
 * - `isRead` → `is_read`
 * - `isBookmarked` → `is_bookmarked`
 *
 * @param frontendParams - 前端分页参数
 * @returns 后端分页参数
 *
 * @example
 * ```typescript
 * // 前端参数
 * const frontendParams = {
 *   page: 1,
 *   pageSize: 20,
 *   sortBy: 'createdAt',
 *   sortOrder: 'DESC',
 *   isRead: false
 * }
 *
 * // 转换后
 * const backendParams = transformPaginationParams(frontendParams)
 * // {
 * //   page: 1,
 * //   limit: 20,
 * //   sort_by: 'created_at',
 * //   sort_order: 'DESC',
 * //   is_read: false
 * // }
 * ```
 */
export const transformPaginationParams = (
  frontendParams: FrontendPaginationParams
): BackendPaginationParams => {
  const backendParams: BackendPaginationParams = {}

  // 基础分页参数
  if (frontendParams.page !== undefined) {
    backendParams.page = frontendParams.page
  }

  if (frontendParams.pageSize !== undefined) {
    backendParams.limit = frontendParams.pageSize
  }

  // 排序参数转换
  if (frontendParams.sortBy !== undefined) {
    backendParams.sort_by = camelToSnake(frontendParams.sortBy)
  }

  if (frontendParams.sortOrder !== undefined) {
    backendParams.sort_order = frontendParams.sortOrder.toUpperCase() as 'ASC' | 'DESC'
  }

  // 过滤参数
  if (frontendParams.keyword !== undefined) {
    backendParams.keyword = frontendParams.keyword
  }

  if (frontendParams.category !== undefined) {
    backendParams.category = frontendParams.category
  }

  if (frontendParams.tags !== undefined) {
    backendParams.tags = frontendParams.tags
  }

  if (frontendParams.source !== undefined) {
    backendParams.source = frontendParams.source
  }

  // 布尔值过滤参数转换
  if (frontendParams.isRead !== undefined) {
    backendParams.is_read = frontendParams.isRead
  }

  if (frontendParams.isBookmarked !== undefined) {
    backendParams.is_bookmarked = frontendParams.isBookmarked
  }

  return backendParams
}

// ============================================================================
// 响应转换函数（后端 -> 前端）
// ============================================================================

/**
 * 后端分页响应转前端格式
 *
 * 转换规则：
 * - `limit` → `pageSize`
 * - 自动计算分页元数据（总页数、是否有上下页等）
 * - 统一响应格式为 { data, meta }
 *
 * @param backendResponse - 后端分页响应
 * @returns 前端分页响应
 *
 * @example
 * ```typescript
 * // 后端响应
 * const backendResponse = {
 *   data: [...],
 *   total: 100,
 *   page: 2,
 *   limit: 20
 * }
 *
 * // 转换后
 * const frontendResponse = transformPaginationResponse(backendResponse)
 * // {
 * //   data: [...],
 * //   meta: {
 * //     page: 2,
 * //     pageSize: 20,
 * //     total: 100,
 * //     totalPages: 5,
 * //     hasPrevious: true,
 * //     hasNext: true,
 * //     previousPage: 1,
 * //     nextPage: 3
 * //   }
 * // }
 * ```
 */
export const transformPaginationResponse = <T>(
  backendResponse: BackendPaginationResponse<T>
): FrontendPaginationResponse<T> => {
  const { data, total, page, limit } = backendResponse

  // 计算总页数
  const totalPages = calculateTotalPages(total, limit)

  // 构建分页元数据
  const meta: FrontendPaginationMeta = {
    page,
    pageSize: limit,
    total,
    totalPages,
    hasPrevious: page > 1,
    hasNext: page < totalPages,
    previousPage: page > 1 ? page - 1 : undefined,
    nextPage: page < totalPages ? page + 1 : undefined
  }

  return {
    data,
    meta
  }
}

/**
 * 转换单个分页元数据对象（用于已有数据的转换）
 *
 * @param total - 总记录数
 * @param page - 当前页码
 * @param limit - 每页数量
 * @returns 分页元数据
 *
 * @example
 * ```typescript
 * const meta = calculatePaginationMeta(100, 2, 20)
 * // {
 * //   page: 2,
 * //   pageSize: 20,
 * //   total: 100,
 * //   totalPages: 5,
 * //   hasPrevious: true,
 * //   hasNext: true,
 * //   previousPage: 1,
 * //   nextPage: 3
 * // }
 * ```
 */
export const calculatePaginationMeta = (
  total: number,
  page: number,
  limit: number
): FrontendPaginationMeta => {
  const totalPages = calculateTotalPages(total, limit)

  return {
    page,
    pageSize: limit,
    total,
    totalPages,
    hasPrevious: page > 1,
    hasNext: page < totalPages,
    previousPage: page > 1 ? page - 1 : undefined,
    nextPage: page < totalPages ? page + 1 : undefined
  }
}

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * camelCase 转 snake_case
 *
 * @param str - camelCase 字符串
 * @returns snake_case 字符串
 *
 * @example
 * ```typescript
 * camelToSnake('sortBy') // 'sort_by'
 * camelToSnake('createdAt') // 'created_at'
 * camelToSnake('isRead') // 'is_read'
 * camelToSnake('userID') // 'user_id'
 * ```
 */
const camelToSnake = (str: string): string => {
  return str
    .replace(/([A-Z])/g, '_$1')
    .toLowerCase()
    .replace(/^_/, '') // 移除开头的下划线（如果第一个字符是大写）
}

/**
 * 计算总页数
 *
 * @param total - 总记录数
 * @param limit - 每页数量
 * @returns 总页数
 *
 * @example
 * ```typescript
 * calculateTotalPages(100, 20) // 5
 * calculateTotalPages(101, 20) // 6
 * calculateTotalPages(0, 20)   // 0
 * ```
 */
const calculateTotalPages = (total: number, limit: number): number => {
  if (total <= 0 || limit <= 0) {
    return 0
  }
  return Math.ceil(total / limit)
}

/**
 * 构建后端查询参数字符串（用于 URL 查询）
 *
 * @param params - 前端分页参数
 * @returns URL 查询字符串
 *
 * @example
 * ```typescript
 * const queryString = buildQueryString({
 *   page: 1,
 *   pageSize: 20,
 *   sortBy: 'createdAt',
 *   sortOrder: 'DESC'
 * })
 * // '?page=1&limit=20&sort_by=created_at&sort_order=DESC'
 * ```
 */
export const buildQueryString = (params: FrontendPaginationParams): string => {
  const backendParams = transformPaginationParams(params)
  const searchParams = new URLSearchParams()

  Object.entries(backendParams).forEach(([key, value]) => {
    if (value !== undefined && value !== null && value !== '') {
      searchParams.append(key, String(value))
    }
  })

  const queryString = searchParams.toString()
  return queryString ? `?${queryString}` : ''
}

/**
 * 从 URL 查询字符串解析分页参数
 *
 * @param queryString - URL 查询字符串（包含或不包含 ?）
 * @returns 前端分页参数
 *
 * @example
 * ```typescript
 * const params = parseQueryString('?page=1&limit=20&sort_by=created_at')
 * // {
 * //   page: 1,
 * //   pageSize: 20,
 * //   sortBy: 'createdAt'
 * // }
 * ```
 */
export const parseQueryString = (queryString: string): FrontendPaginationParams => {
  // 移除开头的 ?
  const cleanQueryString = queryString.startsWith('?')
    ? queryString.slice(1)
    : queryString

  const searchParams = new URLSearchParams(cleanQueryString)
  const frontendParams: FrontendPaginationParams = {}

  // 转换基础分页参数
  const page = searchParams.get('page')
  if (page) {
    frontendParams.page = parseInt(page, 10)
  }

  const limit = searchParams.get('limit')
  if (limit) {
    frontendParams.pageSize = parseInt(limit, 10)
  }

  // 转换排序参数
  const sortBy = searchParams.get('sort_by')
  if (sortBy) {
    frontendParams.sortBy = snakeToCamel(sortBy)
  }

  const sortOrder = searchParams.get('sort_order')
  if (sortOrder) {
    frontendParams.sortOrder = sortOrder as 'ASC' | 'DESC'
  }

  // 转换过滤参数
  const keyword = searchParams.get('keyword')
  if (keyword) {
    frontendParams.keyword = keyword
  }

  const category = searchParams.get('category')
  if (category) {
    frontendParams.category = category
  }

  const tags = searchParams.get('tags')
  if (tags) {
    frontendParams.tags = tags
  }

  const source = searchParams.get('source')
  if (source) {
    frontendParams.source = source
  }

  // 转换布尔值过滤参数
  const isRead = searchParams.get('is_read')
  if (isRead) {
    frontendParams.isRead = isRead === 'true'
  }

  const isBookmarked = searchParams.get('is_bookmarked')
  if (isBookmarked) {
    frontendParams.isBookmarked = isBookmarked === 'true'
  }

  return frontendParams
}

/**
 * snake_case 转 camelCase（用于解析后端响应）
 *
 * @param str - snake_case 字符串
 * @returns camelCase 字符串
 *
 * @example
 * ```typescript
 * snakeToCamel('sort_by')   // 'sortBy'
 * snakeToCamel('created_at') // 'createdAt'
 * snakeToCamel('is_read')   // 'isRead'
 * snakeToCamel('user_id')   // 'userId'
 * ```
 */
const snakeToCamel = (str: string): string => {
  return str
    .split('_')
    .map((word, index) => {
      if (index === 0) {
        return word
      }
      return word.charAt(0).toUpperCase() + word.slice(1)
    })
    .join('')
}

// ============================================================================
// 默认值常量
// ============================================================================

/**
 * 默认分页参数
 */
export const DEFAULT_PAGINATION_PARAMS: FrontendPaginationParams = {
  page: 1,
  pageSize: 20,
  sortBy: 'createdAt',
  sortOrder: 'DESC'
}

/**
 * 默认每页数量选项
 */
export const DEFAULT_PAGE_SIZE_OPTIONS = [10, 20, 50, 100] as const

/**
 * 最大每页数量
 */
export const MAX_PAGE_SIZE = 100

/**
 * 最小每页数量
 */
export const MIN_PAGE_SIZE = 1
