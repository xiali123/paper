/**
 * Vitest 测试环境设置
 * 在所有测试运行前执行
 */

import { vi } from 'vitest'
import { config } from '@vue/test-utils'

// 全局测试配置
config.global.stubs = {
  transition: false,
  'transition-group': false
}

// Mock IntersectionObserver
global.IntersectionObserver = vi.fn().mockImplementation(() => ({
  observe: vi.fn(),
  unobserve: vi.fn(),
  disconnect: vi.fn()
}))

// Mock ResizeObserver
global.ResizeObserver = vi.fn().mockImplementation(() => ({
  observe: vi.fn(),
  unobserve: vi.fn(),
  disconnect: vi.fn()
}))

// Mock localStorage
const localStorageMock = (() => {
  let store: Record<string, string> = {}

  return {
    getItem: (key: string) => store[key] || null,
    setItem: (key: string, value: string) => {
      store[key] = value.toString()
    },
    removeItem: (key: string) => {
      delete store[key]
    },
    clear: () => {
      store = {}
    },
    get length() {
      return Object.keys(store).length
    },
    key: (index: number) => {
      const keys = Object.keys(store)
      return keys[index] || null
    }
  }
})()

global.localStorage = localStorageMock as Storage

// Mock sessionStorage
global.sessionStorage = localStorageMock as Storage

// Mock matchMedia
Object.defineProperty(window, 'matchMedia', {
  writable: true,
  value: vi.fn().mockImplementation(query => ({
    matches: false,
    media: query,
    onchange: null,
    addListener: vi.fn(),
    removeListener: vi.fn(),
    addEventListener: vi.fn(),
    removeEventListener: vi.fn(),
    dispatchEvent: vi.fn()
  }))
})

// Mock window.URL.createObjectURL
global.URL.createObjectURL = vi.fn(() => 'mock-url')
global.URL.revokeObjectURL = vi.fn()

// Mock Performance API
global.Performance.prototype.mark = vi.fn()
global.Performance.prototype.measure = vi.fn()

// Mock requestAnimationFrame
global.requestAnimationFrame = (callback: FrameRequestCallback) => {
  return setTimeout(callback, 16) as unknown as number
}

global.cancelAnimationFrame = (id: number) => {
  clearTimeout(id)
}

// Console mock for tests
const originalConsole = { ...console }

beforeEach(() => {
  // 恢复原始console
  Object.assign(console, originalConsole)

  // 清除localStorage
  localStorage.clear()
  sessionStorage.clear()
})

// 全局测试工具
declare global {
  namespace NodeJS {
    interface Global {
      // 添加自定义全局变量
    }
  }
}

export {}
