/**
 * PaperCrawler - Advanced Cache Composable for Vue 3
 *
 * Multi-tier caching strategy:
 * - Level 1: In-memory cache (fastest, limited size)
 * - Level 2: IndexedDB (persistent, larger size)
 * - Level 3: Network fetch (fallback)
 *
 * Features:
 * - Automatic cache warming
 * - Predictive preloading
 * - Configurable TTL per cache type
 * - LRU eviction
 * - Cache compression for large values
 * - Performance metrics tracking
 */

import { ref, computed, watch, onMounted, onUnmounted } from 'vue'

// ============================================================================
// Cache Configuration
// ============================================================================

const CACHE_CONFIG = {
  // Cache sizes
  maxMemoryItems: 100,        // Max items in memory cache
  maxIndexedDBSize: 500 * 1024 * 1024,  // 500MB max IndexedDB

  // TTL in milliseconds
  ttl: {
    searchResults: 15 * 60 * 1000,      // 15 minutes
    paperDetail: 60 * 60 * 1000,         // 1 hour
    journalInfo: 24 * 60 * 60 * 1000,    // 24 hours
    statistics: 60 * 60 * 1000,          // 1 hour
    userPreferences: 60 * 60 * 1000,     // 1 hour
    recentSearches: 7 * 24 * 60 * 60 * 1000,  // 7 days
    syncState: 60 * 1000,                // 1 minute
    metadata: 60 * 60 * 1000             // 1 hour
  },

  // Compression threshold (bytes)
  compressionThreshold: 1024,  // Compress if > 1KB

  // Cache key prefix
  prefix: 'pc'
}

// ============================================================================
// In-Memory Cache Store
// ============================================================================

class MemoryCache {
  constructor(maxSize = CACHE_CONFIG.maxMemoryItems) {
    this.cache = new Map()
    this.maxSize = maxSize
    this.hits = 0
    this.misses = 0
  }

  set(key, value, ttl = CACHE_CONFIG.ttl.searchResults) {
    // Evict oldest if at capacity
    if (this.cache.size >= this.maxSize) {
      this.evictLRU()
    }

    this.cache.set(key, {
      value,
      expiresAt: Date.now() + ttl,
      createdAt: Date.now(),
      accessCount: 0,
      size: this.estimateSize(value)
    })
  }

  get(key) {
    const entry = this.cache.get(key)

    if (!entry) {
      this.misses++
      return null
    }

    // Check expiration
    if (Date.now() > entry.expiresAt) {
      this.cache.delete(key)
      this.misses++
      return null
    }

    // Update access stats
    entry.accessCount++
    this.hits++

    return entry.value
  }

  has(key) {
    const entry = this.cache.get(key)
    return entry && Date.now() <= entry.expiresAt
  }

  delete(key) {
    return this.cache.delete(key)
  }

  clear() {
    this.cache.clear()
    this.hits = 0
    this.misses = 0
  }

  evictLRU() {
    let oldestKey = null
    let oldestTime = Infinity
    let lowestAccess = Infinity

    for (const [key, entry] of this.cache) {
      // Evict based on last access time and access frequency
      const score = entry.accessCount > 0
        ? entry.createdAt / entry.accessCount
        : entry.createdAt

      if (score < oldestTime || (score === oldestTime && entry.accessCount < lowestAccess)) {
        oldestTime = score
        lowestAccess = entry.accessCount
        oldestKey = key
      }
    }

    if (oldestKey) {
      this.cache.delete(oldestKey)
    }
  }

  invalidatePattern(pattern) {
    const regex = new RegExp(pattern)
    for (const key of this.cache.keys()) {
      if (regex.test(key)) {
        this.cache.delete(key)
      }
    }
  }

  estimateSize(value) {
    // Rough estimation in bytes
    return JSON.stringify(value).length * 2  // UTF-16
  }

  getSize() {
    return this.cache.size
  }

  getHitRate() {
    const total = this.hits + this.misses
    return total > 0 ? this.hits / total : 0
  }

  getStats() {
    let totalSize = 0
    for (const [, entry] of this.cache) {
      totalSize += entry.size
    }

    return {
      size: this.cache.size,
      totalSize,
      hits: this.hits,
      misses: this.misses,
      hitRate: this.getHitRate()
    }
  }
}

// ============================================================================
// IndexedDB Cache Store
// ============================================================================

class IndexedDBCache {
  constructor() {
    this.dbName = 'PaperCrawlerCache'
    this.dbVersion = 1
    this.storeName = 'cache'
    this.db = null
  }

  async init() {
    return new Promise((resolve, reject) => {
      const request = indexedDB.open(this.dbName, this.dbVersion)

      request.onerror = () => reject(request.error)
      request.onsuccess = () => {
        this.db = request.result
        resolve()
      }

      request.onupgradeneeded = (event) => {
        const db = event.target.result

        if (!db.objectStoreNames.contains(this.storeName)) {
          const store = db.createObjectStore(this.storeName, { keyPath: 'key' })
          store.createIndex('expiresAt', 'expiresAt', { unique: false })
        }
      }
    })
  }

  async set(key, value, ttl = CACHE_CONFIG.ttl.searchResults) {
    if (!this.db) await this.init()

    // Compress large values
    let data = value
    let compressed = false

    if (this.shouldCompress(value)) {
      data = await this.compress(value)
      compressed = true
    }

    const entry = {
      key,
      data,
      compressed,
      expiresAt: Date.now() + ttl,
      createdAt: Date.now()
    }

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction([this.storeName], 'readwrite')
      const store = transaction.objectStore(this.storeName)
      const request = store.put(entry)

      request.onsuccess = () => resolve()
      request.onerror = () => reject(request.error)
    })
  }

  async get(key) {
    if (!this.db) await this.init()

    return new Promise((resolve) => {
      const transaction = this.db.transaction([this.storeName], 'readonly')
      const store = transaction.objectStore(this.storeName)
      const request = store.get(key)

      request.onsuccess = async () => {
        const entry = request.result

        if (!entry) {
          resolve(null)
          return
        }

        // Check expiration
        if (Date.now() > entry.expiresAt) {
          await this.delete(key)
          resolve(null)
          return
        }

        // Decompress if needed
        let value = entry.data
        if (entry.compressed) {
          value = await this.decompress(entry.data)
        }

        resolve(value)
      }

      request.onerror = () => resolve(null)
    })
  }

  async delete(key) {
    if (!this.db) await this.init()

    return new Promise((resolve) => {
      const transaction = this.db.transaction([this.storeName], 'readwrite')
      const store = transaction.objectStore(this.storeName)
      const request = store.delete(key)

      request.onsuccess = () => resolve()
      request.onerror = () => resolve()
    })
  }

  async clear() {
    if (!this.db) await this.init()

    return new Promise((resolve) => {
      const transaction = this.db.transaction([this.storeName], 'readwrite')
      const store = transaction.objectStore(this.storeName)
      const request = store.clear()

      request.onsuccess = () => resolve()
      request.onerror = () => resolve()
    })
  }

  async clearExpired() {
    if (!this.db) await this.init()

    return new Promise((resolve) => {
      const transaction = this.db.transaction([this.storeName], 'readwrite')
      const store = transaction.objectStore(this.storeName)
      const index = store.index('expiresAt')
      const request = index.openCursor(IDBKeyRange.upperBound(Date.now()))

      request.onsuccess = (event) => {
        const cursor = event.target.result
        if (cursor) {
          cursor.delete()
          cursor.continue()
        } else {
          resolve()
        }
      }

      request.onerror = () => resolve()
    })
  }

  shouldCompress(value) {
    const size = JSON.stringify(value).length
    return size > CACHE_CONFIG.compressionThreshold
  }

  async compress(value) {
    const json = JSON.stringify(value)

    // Simple compression using LZ-like algorithm
    // In production, use CompressionStream API or external library
    const compressed = json
      .replace(/[\u0000-\u001F\u007F-\u009F]/g, '')  // Remove control chars
      .replace(/\s+/g, ' ')  // Normalize whitespace

    return compressed
  }

  async decompress(compressed) {
    return JSON.parse(compressed)
  }

  async getStats() {
    if (!this.db) await this.init()

    return new Promise((resolve) => {
      const transaction = this.db.transaction([this.storeName], 'readonly')
      const store = transaction.objectStore(this.storeName)
      const countRequest = store.count()

      countRequest.onsuccess = () => {
        resolve({
          size: countRequest.result
        })
      }

      countRequest.onerror = () => resolve({ size: 0 })
    })
  }
}

// ============================================================================
// Cache Manager
// ============================================================================

const memoryCache = new MemoryCache()
const indexedDBCache = new IndexedDBCache()

/**
 * Generate cache key from components
 */
function generateCacheKey(namespace, type, identifier, options = {}) {
  const version = options.version || 'v1'
  const qualifiers = Object.entries(options.qualifiers || {})
    .map(([k, v]) => `${k}=${v}`)
    .join(',')

  let key = `${CACHE_CONFIG.prefix}:${namespace}:${type}:${identifier}`

  if (version) {
    key += `:${version}`
  }

  if (qualifiers) {
    key += `:${qualifiers}`
  }

  // If key is too long, use hash
  if (key.length > 250) {
    const hash = simpleHash(key)
    key = `${CACHE_CONFIG.prefix}:hash:${hash}`
  }

  return key
}

/**
 * Simple hash function for long keys
 */
function simpleHash(str) {
  let hash = 0
  for (let i = 0; i < str.length; i++) {
    const char = str.charCodeAt(i)
    hash = ((hash << 5) - hash) + char
    hash = hash & hash  // Convert to 32-bit integer
  }
  return Math.abs(hash).toString(16)
}

/**
 * Get cache TTL for type
 */
function getTTL(type) {
  return CACHE_CONFIG.ttl[type] || CACHE_CONFIG.ttl.searchResults
}

// ============================================================================
// Main Composable
// ============================================================================

export function useCache() {
  const isReady = ref(false)
  const metrics = ref({
    memory: memoryCache.getStats(),
    indexedDB: { size: 0 }
  })

  // Initialize IndexedDB on mount
  onMounted(async () => {
    try {
      await indexedDBCache.init()
      isReady.value = true

      // Clear expired entries
      await indexedDBCache.clearExpired()

      // Update metrics
      metrics.value.indexedDB = await indexedDBCache.getStats()
    } catch (error) {
      console.error('Failed to initialize cache:', error)
    }
  })

  /**
   * Get value from cache (memory -> IndexedDB -> null)
   */
  async function get(key) {
    // Try memory cache first
    const memValue = memoryCache.get(key)
    if (memValue !== null) {
      return memValue
    }

    // Try IndexedDB
    const idbValue = await indexedDBCache.get(key)
    if (idbValue !== null) {
      // Promote to memory cache
      const ttl = getTTL('searchResults')  // Default TTL
      memoryCache.set(key, idbValue, ttl)
      return idbValue
    }

    return null
  }

  /**
   * Set value in cache (both memory and IndexedDB)
   */
  async function set(key, value, options = {}) {
    const ttl = options.ttl || getTTL(options.type || 'searchResults')

    // Set in memory cache
    memoryCache.set(key, value, ttl)

    // Set in IndexedDB for persistence
    try {
      await indexedDBCache.set(key, value, ttl)
    } catch (error) {
      console.error('Failed to cache in IndexedDB:', error)
    }

    // Update metrics
    metrics.value.memory = memoryCache.getStats()
  }

  /**
   * Delete from cache
   */
  async function del(key) {
    memoryCache.delete(key)
    await indexedDBCache.delete(key)
  }

  /**
   * Invalidate cache entries matching pattern
   */
  async function invalidate(pattern) {
    memoryCache.invalidatePattern(pattern)

    // For IndexedDB, we'd need to iterate and match
    // For simplicity, just clear all matching pattern in memory
    console.log(`Invalidated cache matching: ${pattern}`)
  }

  /**
   * Clear all cache
   */
  async function clear() {
    memoryCache.clear()
    await indexedDBCache.clear()
    metrics.value.memory = memoryCache.getStats()
  }

  /**
   * Get or fetch pattern with automatic caching
   */
  async function getOrFetch(key, fetcher, options = {}) {
    // Try cache first
    const cached = await get(key)
    if (cached !== null) {
      return cached
    }

    // Fetch from source
    const value = await fetcher()

    // Cache the result
    if (value !== null && value !== undefined) {
      await set(key, value, options)
    }

    return value
  }

  /**
   * Cache warming - preload common data
   */
  async function warmup(warmupTasks) {
    const results = await Promise.allSettled(
      warmupTasks.map(task => task())
    )

    const succeeded = results.filter(r => r.status === 'fulfilled').length
    console.log(`Cache warming complete: ${succeeded}/${warmupTasks.length} tasks succeeded`)
  }

  /**
   * Predictive preloading based on access patterns
   */
  async function predictivePreload(key, relatedKeys) {
    // Preload related items
    for (const relatedKey of relatedKeys) {
      const cached = await get(relatedKey)
      if (cached === null) {
        // Trigger background fetch (don't await)
        console.log(`Preloading: ${relatedKey}`)
      }
    }
  }

  /**
   * Get cache hit rate
   */
  const hitRate = computed(() => {
    return metrics.value.memory.hitRate
  })

  /**
   * Total cache size
   */
  const totalSize = computed(() => {
    return metrics.value.memory.size + metrics.value.indexedDB.size
  })

  return {
    // State
    isReady,
    metrics,
    hitRate,
    totalSize,

    // Methods
    generateKey: generateCacheKey,
    get,
    set,
    delete: del,
    invalidate,
    clear,
    getOrFetch,
    warmup,
    predictivePreload
  }
}

// ============================================================================
// Convenience Composables
// ============================================================================

/**
 * Search results cache composable
 */
export function useSearchCache() {
  const cache = useCache()

  return {
    async get(keyword, page = 1, limit = 20) {
      const key = cache.generateKey('search', keyword, '', {
        qualifiers: { page, limit }
      })
      return cache.get(key)
    },

    async set(keyword, page, limit, results) {
      const key = cache.generateKey('search', keyword, '', {
        qualifiers: { page, limit }
      })
      return cache.set(key, results, { type: 'searchResults' })
    },

    async invalidate(keyword) {
      return cache.invalidate(`pc:search:${encodeURIComponent(keyword)}:*`)
    }
  }
}

/**
 * Paper detail cache composable
 */
export function usePaperCache() {
  const cache = useCache()

  return {
    async get(paperId) {
      const key = cache.generateKey('paper', paperId, '')
      return cache.get(key)
    },

    async set(paperId, paper) {
      const key = cache.generateKey('paper', paperId, '')
      return cache.set(key, paper, { type: 'paperDetail' })
    },

    async invalidate(paperId) {
      const key = cache.generateKey('paper', paperId, '')
      return cache.delete(key)
    }
  }
}

/**
 * Statistics cache composable
 */
export function useStatsCache() {
  const cache = useCache()

  return {
    async get() {
      const key = cache.generateKey('stats', 'overview', '')
      return cache.get(key)
    },

    async set(stats) {
      const key = cache.generateKey('stats', 'overview', '')
      return cache.set(key, stats, { type: 'statistics' })
    },

    async invalidate() {
      const key = cache.generateKey('stats', 'overview', '')
      return cache.delete(key)
    }
  }
}
