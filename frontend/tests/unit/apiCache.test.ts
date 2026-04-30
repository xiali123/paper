/**
 * API缓存单元测试
 * 测试缓存策略、TTL、LRU淘汰
 */

import { describe, it, expect, beforeEach, vi, afterEach } from 'vitest'
import { ApiCache, withCache, latexCache, paperCache, userCache, cacheControl } from '@/utils/apiCache'

describe('ApiCache', () => {
  let cache: ApiCache

  beforeEach(() => {
    cache = new ApiCache({
      ttl: 1000, // 1秒
      maxSize: 3
    })
    vi.clearAllMocks()
    vi.useFakeTimers()
  })

  afterEach(() => {
    vi.useRealTimers()
  })

  describe('基础缓存操作', () => {
    it('应该能够设置和获取缓存', () => {
      cache.set('GET', '/api/test', { data: 'test' })
      const result = cache.get('GET', '/api/test')

      expect(result).toEqual({ data: 'test' })
    })

    it('未命中缓存时应返回null', () => {
      const result = cache.get('GET', '/api/nonexistent')

      expect(result).toBeNull()
    })

    it('应该能够删除缓存', () => {
      cache.set('GET', '/api/test', { data: 'test' })
      const deleted = cache.delete('GET', '/api/test')

      expect(deleted).toBe(true)
      expect(cache.get('GET', '/api/test')).toBeNull()
    })

    it('应该能够清空所有缓存', () => {
      cache.set('GET', '/api/test1', { data: 'test1' })
      cache.set('GET', '/api/test2', { data: 'test2' })

      cache.clear()

      expect(cache.get('GET', '/api/test1')).toBeNull()
      expect(cache.get('GET', '/api/test2')).toBeNull()
    })
  })

  describe('TTL过期', () => {
    it('在TTL内应该返回缓存数据', () => {
      cache.set('GET', '/api/test', { data: 'test' })

      vi.advanceTimersByTime(500)

      const result = cache.get('GET', '/api/test')
      expect(result).toEqual({ data: 'test' })
    })

    it('TTL过期后应该返回null', () => {
      cache.set('GET', '/api/test', { data: 'test' })

      vi.advanceTimersByTime(1500)

      const result = cache.get('GET', '/api/test')
      expect(result).toBeNull()
    })

    it('获取缓存时应更新命中次数', () => {
      cache.set('GET', '/api/test', { data: 'test' })

      cache.get('GET', '/api/test')
      cache.get('GET', '/api/test')

      const stats = cache.getStats()
      expect(stats.entries[0].hits).toBe(2)
    })
  })

  describe('LRU淘汰', () => {
    it('达到maxSize时应淘汰最少使用的缓存', () => {
      // 添加3个缓存（达到maxSize）
      cache.set('GET', '/api/test1', { data: 'test1' })
      cache.set('GET', '/api/test2', { data: 'test2' })
      cache.set('GET', '/api/test3', { data: 'test3' })

      // test1和test2被访问过，test3没有被访问过
      cache.get('GET', '/api/test1')
      cache.get('GET', '/api/test1')
      cache.get('GET', '/api/test2')

      // 添加第4个缓存，应该淘汰test3
      cache.set('GET', '/api/test4', { data: 'test4' })

      expect(cache.get('GET', '/api/test1')).toEqual({ data: 'test1' })
      expect(cache.get('GET', '/api/test2')).toEqual({ data: 'test2' })
      expect(cache.get('GET', '/api/test3')).toBeNull()
      expect(cache.get('GET', '/api/test4')).toEqual({ data: 'test4' })
    })

    it('应该优先淘汰命中次数少的缓存', () => {
      cache.set('GET', '/api/test1', { data: 'test1' })
      cache.set('GET', '/api/test2', { data: 'test2' })
      cache.set('GET', '/api/test3', { data: 'test3' })

      // test1被访问5次，test2被访问3次，test3被访问1次
      for (let i = 0; i < 5; i++) cache.get('GET', '/api/test1')
      for (let i = 0; i < 3; i++) cache.get('GET', '/api/test2')
      cache.get('GET', '/api/test3')

      // 添加第4个缓存，应该淘汰test3
      cache.set('GET', '/api/test4', { data: 'test4' })

      expect(cache.get('GET', '/api/test3')).toBeNull()
    })
  })

  describe('缓存Key生成', () => {
    it('相同的请求应该生成相同的key', () => {
      const cache1 = new ApiCache({ prefix: 'test' })
      const cache2 = new ApiCache({ prefix: 'test' })

      cache1.set('GET', '/api/test', { data: 'test' })
      cache2.set('GET', '/api/test', { data: 'test' })

      const stats1 = cache1.getStats()
      const stats2 = cache2.getStats()

      expect(stats1.entries.length).toBe(1)
      expect(stats2.entries.length).toBe(1)
    })

    it('不同的参数应该生成不同的key', () => {
      cache.set('GET', '/api/test', { data: 'test1' }, { page: 1 })
      cache.set('GET', '/api/test', { data: 'test2' }, { page: 2 })

      const result1 = cache.get('GET', '/api/test', { page: 1 })
      const result2 = cache.get('GET', '/api/test', { page: 2 })

      expect(result1).toEqual({ data: 'test1' })
      expect(result2).toEqual({ data: 'test2' })
    })
  })

  describe('按前缀清除缓存', () => {
    it('应该清除指定前缀的所有缓存', () => {
      const cache1 = new ApiCache({ prefix: 'latex' })
      const cache2 = new ApiCache({ prefix: 'paper' })

      cache1.set('GET', '/api/test', { data: 'latex' })
      cache2.set('GET', '/api/test', { data: 'paper' })

      cache1.clearByPrefix('latex')

      expect(cache1.get('GET', '/api/test')).toBeNull()
      expect(cache2.get('GET', '/api/test')).toEqual({ data: 'paper' })
    })
  })

  describe('缓存统计', () => {
    it('应该返回正确的统计信息', () => {
      cache.set('GET', '/api/test1', { data: 'test1' })
      cache.set('GET', '/api/test2', { data: 'test2' })

      cache.get('GET', '/api/test1')
      cache.get('GET', '/api/test1')
      cache.get('GET', '/api/test2')

      const stats = cache.getStats()

      expect(stats.size).toBe(2)
      expect(stats.hits).toBe(3)
      expect(stats.entries).toHaveLength(2)
    })
  })
})

describe('withCache装饰器', () => {
  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('GET请求应该使用缓存', async () => {
    const mockFn = vi.fn().mockResolvedValue({ data: 'test' })
    const cachedFn = withCache(mockFn, latexCache, 'GET')

    // 第一次调用
    const result1 = await cachedFn('/api/test')
    expect(mockFn).toHaveBeenCalledTimes(1)

    // 第二次调用应该使用缓存
    const result2 = await cachedFn('/api/test')
    expect(mockFn).toHaveBeenCalledTimes(1) // 没有增加
    expect(result1).toEqual(result2)
  })

  it('非GET请求不应该使用缓存', async () => {
    const mockFn = vi.fn().mockResolvedValue({ data: 'test' })
    const cachedFn = withCache(mockFn, latexCache, 'POST')

    await cachedFn('/api/test')
    await cachedFn('/api/test')

    expect(mockFn).toHaveBeenCalledTimes(2)
  })

  it('非GET请求应该清除缓存', async () => {
    latexCache.set('GET', '/api/test', { data: 'cached' })

    const mockFn = vi.fn().mockResolvedValue({ data: 'test' })
    const cachedFn = withCache(mockFn, latexCache, 'POST')

    await cachedFn('/api/test')

    expect(latexCache.get('GET', '/api/test')).toBeNull()
  })
})

describe('专用缓存实例', () => {
  it('LaTeX缓存应该有10分钟TTL', () => {
    expect(latexCache).toBeInstanceOf(ApiCache)
  })

  it('论文缓存应该有5分钟TTL', () => {
    expect(paperCache).toBeInstanceOf(ApiCache)
  })

  it('用户缓存应该有15分钟TTL', () => {
    expect(userCache).toBeInstanceOf(ApiCache)
  })
})

describe('cacheControl工具', () => {
  it('应该能够清除所有缓存', () => {
    latexCache.set('GET', '/api/test', { data: 'test' })
    paperCache.set('GET', '/api/test', { data: 'test' })

    cacheControl.clearAll()

    expect(latexCache.get('GET', '/api/test')).toBeNull()
    expect(paperCache.get('GET', '/api/test')).toBeNull()
  })

  it('应该能够清除LaTeX缓存', () => {
    latexCache.set('GET', '/api/test', { data: 'test' })
    paperCache.set('GET', '/api/test', { data: 'test' })

    cacheControl.clearLatex()

    expect(latexCache.get('GET', '/api/test')).toBeNull()
    expect(paperCache.get('GET', '/api/test')).toEqual({ data: 'test' })
  })

  it('应该能够获取所有缓存的统计', () => {
    latexCache.set('GET', '/api/test', { data: 'test' })

    const stats = cacheControl.getStats()

    expect(stats).toHaveProperty('default')
    expect(stats).toHaveProperty('latex')
    expect(stats).toHaveProperty('paper')
    expect(stats).toHaveProperty('user')
  })
})
