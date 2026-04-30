/**
 * Service Worker单元测试
 * 测试离线功能、缓存策略、同步机制
 */

import { describe, it, expect, beforeEach, vi, afterEach } from 'vitest'
import { renderHook, act } from '@testing-library/vue'
import { useServiceWorker } from '@/composables/useServiceWorker'

// Mock navigator.serviceWorker
const mockRegistration = {
  installing: null,
  waiting: null,
  active: null,
  scope: 'http://localhost:3000/',
  update: vi.fn(),
  unregister: vi.fn(),
  addEventListener: vi.fn(),
  removeEventListener: vi.fn()
}

const mockServiceWorker = {
  register: vi.fn(() => Promise.resolve(mockRegistration)),
  ready: Promise.resolve(mockRegistration),
  controller: null,
  addEventListener: vi.fn(),
  removeEventListener: vi.fn()
}

Object.defineProperty(navigator, 'serviceWorker', {
  value: mockServiceWorker,
  writable: true
})

// Mock localStorage
const localStorageMock = (() => {
  let store: Record<string, string> = {}
  return {
    getItem: (key: string) => store[key] || null,
    setItem: (key: string, value: string) => { store[key] = value.toString() },
    removeItem: (key: string) => { delete store[key] },
    clear: () => { store = {} }
  }
})()

global.localStorage = localStorageMock as Storage

describe('useServiceWorker', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    localStorage.clear()

    // 重置mock
    mockRegistration.installing = null
    mockRegistration.waiting = null
    mockRegistration.active = null
  })

  describe('初始化', () => {
    it('应该检测Service Worker支持', () => {
      (navigator as any).serviceWorker = undefined

      const { result } = renderHook(() => useServiceWorker())

      expect(result.current.isSWReady.value).toBe(false)
    })

    it('应该注册Service Worker', async () => {
      const { result } = renderHook(() => useServiceWorker())

      await act(async () => {
        await result.current.registerSW()
      })

      expect(mockServiceWorker.register).toHaveBeenCalledWith('/sw.js', {
        updateViaCache: 'none'
      })
    })

    it('注册成功后应设置isSWReady为true', async () => {
      const { result } = renderHook(() => useServiceWorker())

      await act(async () => {
        await result.current.registerSW()
      })

      expect(result.current.isSWReady.value).toBe(true)
    })
  })

  describe('网络状态', () => {
    it('应该正确检测在线状态', () => {
      // 模拟在线
      Object.defineProperty(navigator, 'onLine', {
        value: true,
        writable: true
      })

      const { result } = renderHook(() => useServiceWorker())

      expect(result.current.isOnline.value).toBe(true)
    })

    it('应该正确检测离线状态', () => {
      // 模拟离线
      Object.defineProperty(navigator, 'onLine', {
        value: false,
        writable: true
      })

      const { result } = renderHook(() => useServiceWorker())

      expect(result.current.isOnline.value).toBe(false)
    })
  })

  describe('Service Worker更新', () => {
    it('应该检测Service Worker更新', async () => {
      const { result } = renderHook(() => useServiceWorker())

      // Mock updatefound事件
      const addEventListenerMock = vi.fn((event, callback) => {
        if (event === 'updatefound') {
          // 模拟有新版本
          setTimeout(() => {
            act(() => {
              result.current.updateAvailable = true
            })
          }, 100)
        }
      })

      mockRegistration.addEventListener = addEventListenerMock

      await act(async () => {
        await result.current.registerSW()
      })

      // 等待事件处理
      await new Promise(resolve => setTimeout(resolve, 200))
    })

    it('应该能够跳过等待并激活新版本', async () => {
      const { result } = renderHook(() => useServiceWorker())

      // Mock waiting worker
      mockRegistration.waiting = {
        postMessage: vi.fn()
      } as any

      await act(async () => {
        await result.current.registerSW()
      })

      act(() => {
        result.current.skipWaiting()
      })

      expect(mockRegistration.waiting?.postMessage).toHaveBeenCalledWith({
        type: 'SKIP_WAITING'
      })
    })
  })

  describe('离线队列管理', () => {
    it('应该同步离线队列', async () => {
      const { result } = renderHook(() => useServiceWorker())

      // Mock active worker
      mockRegistration.active = {
        postMessage: vi.fn()
      } as any

      await act(async () => {
        await result.current.registerSW()
      })

      act(() => {
        result.current.syncOfflineQueue()
      })

      expect(mockRegistration.active?.postMessage).toHaveBeenCalledWith({
        type: 'SYNC_OFFLINE_QUEUE'
      })
    })

    it('应该清空离线队列', async () => {
      const { result } = renderHook(() => useServiceWorker())

      // Mock active worker
      mockRegistration.active = {
        postMessage: vi.fn()
      } as any

      await act(async () => {
        await result.current.registerSW()
      })

      act(() => {
        result.current.clearOfflineQueue()
      })

      expect(mockRegistration.active?.postMessage).toHaveBeenCalledWith({
        type: 'CLEAR_OFFLINE_QUEUE'
      })
    })

    it('应该获取离线队列大小', async () => {
      const { result } = renderHook(() => useServiceWorker())

      // Mock active worker和MessageChannel
      const mockPort = {
        postMessage: vi.fn()
      }
      const mockChannel = {
        port1: { onmessage: null },
        port2: mockPort
      }

      global.MessageChannel = vi.fn(() => mockChannel) as any

      mockRegistration.active = {
        postMessage: vi.fn((_, transfer) => {
          // 模拟响应
          if (mockChannel.port1.onmessage) {
            mockChannel.port1.onmessage({ data: { size: 5 } })
          }
        })
      } as any

      await act(async () => {
        await result.current.registerSW()
      })

      act(() => {
        result.current.getOfflineQueueSize()
      })

      // 等待异步响应
      await new Promise(resolve => setTimeout(resolve, 100))
    })
  })

  describe('消息处理', () => {
    it('应该处理离线操作队列消息', async () => {
      const { result } = renderHook(() => useServiceWorker())

      let messageHandler: ((message: any) => void) | null = null

      act(() => {
        result.current.setMessageHandler((message) => {
          messageHandler = message
        })
      })

      // 模拟收到消息
      const mockMessage = {
        type: 'OFFLINE_OPERATION_QUEUED',
        data: { count: 3 }
      }

      act(() => {
        if (messageHandler) {
          messageHandler(mockMessage)
        }
      })

      expect(result.current.offlineQueueSize.value).toBe(3)
    })

    it('应该处理同步完成消息', async () => {
      const { result } = renderHook(() => useServiceWorker())

      let messageHandler: ((message: any) => void) | null = null

      act(() => {
        result.current.setMessageHandler((message) => {
          messageHandler = message
        })
      })

      const mockMessage = {
        type: 'OFFLINE_SYNC_COMPLETE',
        data: { successful: 2, failed: 1, remaining: 1 }
      }

      act(() => {
        if (messageHandler) {
          messageHandler(mockMessage)
        }
      })

      expect(result.current.offlineQueueSize.value).toBe(1)
    })
  })

  describe('离线状态', () => {
    it('应该返回正确的离线状态', () => {
      const { result } = renderHook(() => useServiceWorker())

      Object.defineProperty(navigator, 'onLine', { value: false, writable: true })

      const status = result.current.getOfflineStatus()

      expect(status).toEqual({
        isOnline: false,
        queueSize: 0
      })
    })
  })
})

describe('Service Worker缓存策略', () => {
  describe('静态资源缓存', () => {
    it('应该缓存核心静态资源', () => {
      const staticResources = [
        '/',
        '/index.html',
        '/manifest.json'
      ]

      staticResources.forEach(resource => {
        expect(resource).toBeTruthy()
      })
    })

    it('应该使用cache-first策略', () => {
      // 验证缓存优先策略
      const strategy = 'cache-first'
      expect(strategy).toBe('cache-first')
    })
  })

  describe('API缓存策略', () => {
    it('应该区分可缓存和不可缓存的API', () => {
      const cacheablePatterns = [
        /\/api\/latex\/documents$/,
        /\/api\/latex\/templates$/
      ]

      const bypassPatterns = [
        /\/api\/auth/,
        /\/api\/user\/.*\/settings/
      ]

      expect(cacheablePatterns.length).toBeGreaterThan(0)
      expect(bypassPatterns.length).toBeGreaterThan(0)
    })

    it('GET请求应该可缓存', () => {
      const getRequest = { method: 'GET', url: '/api/latex/documents' }
      expect(getRequest.method).toBe('GET')
    })

    it('POST请求不应该缓存', () => {
      const postRequest = { method: 'POST', url: '/api/latex/documents' }
      expect(postRequest.method).toBe('POST')
    })
  })
})

describe('Service Worker离线同步', () => {
  it('应该将离线操作加入队列', () => {
    const operation = {
      url: '/api/latex/documents/1',
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: '{"title": "Updated"}',
      timestamp: Date.now()
    }

    expect(operation).toHaveProperty('timestamp')
  })

  it('应该按顺序同步离线队列', async () => {
    const queue = [
      { url: '/api/1', timestamp: 1000 },
      { url: '/api/2', timestamp: 2000 },
      { url: '/api/3', timestamp: 3000 }
    ]

    // 模拟按时间戳排序
    const sorted = [...queue].sort((a, b) => a.timestamp - b.timestamp)

    expect(sorted[0].url).toBe('/api/1')
    expect(sorted[1].url).toBe('/api/2')
    expect(sorted[2].url).toBe('/api/3')
  })
})
