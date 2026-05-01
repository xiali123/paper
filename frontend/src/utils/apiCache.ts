/**
 * API响应缓存工具
 * 支持TTL、LRU、缓存key生成
 */

interface CacheEntry<T> {
  data: T
  timestamp: number
  hits: number
}

interface CacheConfig {
  ttl?: number // 缓存生存时间（毫秒）
  maxSize?: number // 最大缓存条目数
  prefix?: string // 缓存key前缀
}

class ApiCache {
  private cache: Map<string, CacheEntry<any>> = new Map()
  private config: Required<CacheConfig>

  constructor(config: CacheConfig = {}) {
    this.config = {
      ttl: config.ttl || 5 * 60 * 1000, // 默认5分钟
      maxSize: config.maxSize || 100, // 默认100条
      prefix: config.prefix || 'api_cache'
    }
  }

  /**
   * 生成缓存key
   */
  private generateKey(method: string, url: string, params?: any): string {
    const paramsStr = params ? JSON.stringify(params) : ''
    // Simple hash to avoid btoa Unicode crash
    let hash = 0
    for (let i = 0; i < paramsStr.length; i++) {
      const char = paramsStr.charCodeAt(i)
      hash = ((hash << 5) - hash) + char
      hash |= 0
    }
    return `${this.config.prefix}:${method}:${url}:${hash}`
  }

  /**
   * 获取缓存数据
   */
  get<T>(method: string, url: string, params?: any): T | null {
    const key = this.generateKey(method, url, params)
    const entry = this.cache.get(key)

    if (!entry) {
      return null
    }

    // 检查是否过期
    const now = Date.now()
    if (now - entry.timestamp > this.config.ttl) {
      this.cache.delete(key)
      return null
    }

    // Re-insert to move to end (most recently used) for O(1) LRU
    this.cache.delete(key)
    entry.hits++
    this.cache.set(key, entry)
    return entry.data as T
  }

  /**
   * 设置缓存数据
   */
  set<T>(method: string, url: string, data: T, params?: any): void {
    // 检查缓存大小限制
    if (this.cache.size >= this.config.maxSize) {
      this.evictLRU()
    }

    const key = this.generateKey(method, url, params)
    this.cache.set(key, {
      data,
      timestamp: Date.now(),
      hits: 0
    })
  }

  /**
   * 删除缓存
   */
  delete(method: string, url: string, params?: any): boolean {
    const key = this.generateKey(method, url, params)
    return this.cache.delete(key)
  }

  /**
   * 清空所有缓存
   */
  clear(): void {
    this.cache.clear()
  }

  /**
   * 按前缀清除缓存
   */
  clearByPrefix(prefix: string): void {
    const keysToDelete: string[] = []

    for (const key of this.cache.keys()) {
      if (key.startsWith(prefix)) {
        keysToDelete.push(key)
      }
    }

    keysToDelete.forEach(key => this.cache.delete(key))
  }

  /**
   * LRU淘汰策略 - O(1) using Map insertion order
   * Map.keys() returns keys in insertion order; first key = least recently used
   */
  private evictLRU(): void {
    const firstKey = this.cache.keys().next().value
    if (firstKey !== undefined) {
      this.cache.delete(firstKey)
    }
  }

  /**
   * 获取缓存统计
   */
  getStats(): { size: number; hits: number; entries: Array<{ key: string; hits: number }> } {
    const entries = Array.from(this.cache.entries()).map(([key, entry]) => ({
      key,
      hits: entry.hits
    }))

    const totalHits = entries.reduce((sum, e) => sum + e.hits, 0)

    return {
      size: this.cache.size,
      hits: totalHits,
      entries
    }
  }
}

// 默认缓存实例
const defaultCache = new ApiCache()

// 专用缓存实例
export const latexCache = new ApiCache({ prefix: 'latex', ttl: 10 * 60 * 1000 }) // LaTeX缓存10分钟
export const paperCache = new ApiCache({ prefix: 'paper', ttl: 5 * 60 * 1000 }) // 论文缓存5分钟
export const userCache = new ApiCache({ prefix: 'user', ttl: 15 * 60 * 1000 }) // 用户缓存15分钟

/**
 * 带缓存的API请求装饰器
 */
export function withCache<T extends (...args: any[]) => Promise<any>>(
  fn: T,
  cache: ApiCache = defaultCache,
  method: string = 'GET'
): T {
  return (async (...args: any[]) => {
    // 尝试从缓存获取
    const [url, params] = args
    if (method === 'GET') {
      const cached = cache.get<any>(method, url, params)
      if (cached) {
        console.log(`[API Cache] HIT: ${method} ${url}`)
        return cached
      }
    }

    // 调用原始函数
    console.log(`[API Cache] MISS: ${method} ${url}`)
    const result = await fn(...args)

    // 缓存结果
    if (method === 'GET') {
      cache.set(method, url, result, params)
    } else {
      // 非GET请求清除相关缓存
      cache.clearByPrefix(cache['config']?.prefix || 'api_cache')
    }

    return result
  }) as T
}

/**
 * 缓存控制工具
 */
export const cacheControl = {
  /**
   * 清除所有缓存
   */
  clearAll: () => {
    defaultCache.clear()
    latexCache.clear()
    paperCache.clear()
    userCache.clear()
  },

  /**
   * 清除LaTeX相关缓存
   */
  clearLatex: () => {
    latexCache.clear()
  },

  /**
   * 清除论文相关缓存
   */
  clearPaper: () => {
    paperCache.clear()
  },

  /**
   * 获取缓存统计
   */
  getStats: () => ({
    default: defaultCache.getStats(),
    latex: latexCache.getStats(),
    paper: paperCache.getStats(),
    user: userCache.getStats()
  })
}

export default ApiCache
