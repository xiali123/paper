/**
 * 前端-后端API路由映射测试
 * 验证前端API模块的URL路径与后端路由对应关系
 * 确保前端调用的每个API端点都使用正确的HTTP方法和路径前缀
 */

import { describe, it, expect, beforeEach, vi } from 'vitest'
import request from '@/utils/request'

// 使用 vi.hoisted 创建可在 vi.mock 工厂中引用的 mock 函数
const { mockGet, mockPost, mockPut, mockDelete, mockPatch, mockRequest } = vi.hoisted(() => {
  const spy = (urlOrConfig: any) => Promise.resolve({})
  const mockGet = Object.assign(spy, {
    get: spy,
    post: spy,
    put: spy,
    delete: spy,
    patch: spy
  })
  return {
    mockGet: vi.fn().mockResolvedValue({}),
    mockPost: vi.fn().mockResolvedValue({}),
    mockPut: vi.fn().mockResolvedValue({}),
    mockDelete: vi.fn().mockResolvedValue({}),
    mockPatch: vi.fn().mockResolvedValue({}),
    mockRequest: Object.assign(vi.fn().mockResolvedValue({}), {
      get: vi.fn().mockResolvedValue({}),
      post: vi.fn().mockResolvedValue({}),
      put: vi.fn().mockResolvedValue({}),
      delete: vi.fn().mockResolvedValue({}),
      patch: vi.fn().mockResolvedValue({})
    })
  }
})

vi.mock('@/utils/request', () => ({
  default: mockRequest
}))

// Mock import.meta.env
vi.stubGlobal('import.meta', { env: { DEV: false, VITE_APP_ENABLE_MOCK: 'false', BASE_URL: '/' } })

// ============================================================================
// Auth API 映射
// ============================================================================

describe('Auth API 路由映射', () => {
  let authApi: typeof import('@/api/modules/auth').authApi

  beforeEach(async () => {
    vi.clearAllMocks()
    const mod = await import('@/api/modules/auth')
    authApi = mod.authApi
  })

  it('getCurrentUser 应 GET /api/auth/me', async () => {
    await authApi.getCurrentUser()
    expect(request.get).toHaveBeenCalledWith('/api/auth/me')
  })

  it('changePassword 应 PUT /api/auth/password', async () => {
    await authApi.changePassword({ oldPassword: 'old', newPassword: 'new' })
    expect(request.put).toHaveBeenCalledWith('/api/auth/password', expect.any(Object))
  })

  it('requestPasswordReset 应 POST /api/auth/forgot-password', async () => {
    await authApi.requestPasswordReset('test@test.com')
    expect(request.post).toHaveBeenCalledWith('/api/auth/forgot-password', expect.any(Object))
  })

  it('resetPassword 应 POST /api/auth/reset-password', async () => {
    await authApi.resetPassword({ token: 'tok', newPassword: 'new' })
    expect(request.post).toHaveBeenCalledWith('/api/auth/reset-password', expect.any(Object))
  })

  it('updateProfile 应 PUT /api/auth/profile', async () => {
    await authApi.updateProfile({ fullName: 'Test User' })
    expect(request.put).toHaveBeenCalledWith('/api/auth/profile', expect.any(Object))
  })

  it('getSessions 应 GET /api/auth/sessions', async () => {
    await authApi.getSessions()
    expect(request.get).toHaveBeenCalledWith('/api/auth/sessions')
  })

  it('deleteAvatar 应 DELETE /api/auth/avatar', async () => {
    await authApi.deleteAvatar()
    expect(request.delete).toHaveBeenCalledWith('/api/auth/avatar')
  })
})

// ============================================================================
// Papers API 映射
// ============================================================================

describe('Papers API 路由映射', () => {
  let papersApi: typeof import('@/api/modules/papers').papersApi

  beforeEach(async () => {
    vi.clearAllMocks()
    const mod = await import('@/api/modules/papers')
    papersApi = mod.papersApi
  })

  it('getPapers 应 GET /api/papers', async () => {
    await papersApi.getPapers({ page: 1, limit: 20 })
    expect(request.get).toHaveBeenCalledWith('/api/papers', expect.objectContaining({ params: expect.any(Object) }))
  })

  it('getPaper 应 GET /api/papers/:id', async () => {
    await papersApi.getPaper(42)
    expect(request.get).toHaveBeenCalledWith('/api/papers/42')
  })

  it('createPaper 应 POST /api/papers', async () => {
    await papersApi.createPaper({ title: 'Test', authors: 'Author', year: 2024, venue: 'Test', abstract: '' })
    expect(request.post).toHaveBeenCalledWith('/api/papers', expect.any(Object))
  })

  it('updatePaper 应 PUT /api/papers/:id', async () => {
    await papersApi.updatePaper(42, { title: 'Updated' })
    expect(request.put).toHaveBeenCalledWith('/api/papers/42', expect.any(Object))
  })

  it('deletePaper 应 DELETE /api/papers/:id', async () => {
    await papersApi.deletePaper(42)
    expect(request.delete).toHaveBeenCalledWith('/api/papers/42')
  })

  it('getStats 应 GET /api/stats', async () => {
    await papersApi.getStats()
    expect(request.get).toHaveBeenCalledWith('/api/stats')
  })
})

// ============================================================================
// Crawler API 映射
// ============================================================================

describe('Crawler API 路由映射', () => {
  let crawlerApi: typeof import('@/api/modules/crawler').crawlerApi

  beforeEach(async () => {
    vi.clearAllMocks()
    const mod = await import('@/api/modules/crawler')
    crawlerApi = mod.crawlerApi
  })

  it('getDashboard 应 GET /api/crawler/dashboard', async () => {
    await crawlerApi.getDashboard()
    expect(request.get).toHaveBeenCalledWith('/api/crawler/dashboard')
  })

  it('createTask 应 POST /api/crawler/tasks', async () => {
    await crawlerApi.createTask({ url: 'http://test.com', source: 'test' } as any)
    expect(request.post).toHaveBeenCalledWith('/api/crawler/tasks', expect.any(Object))
  })

  it('getTaskStatus 应 GET /api/crawler/tasks/:id', async () => {
    await crawlerApi.getTaskStatus(99)
    expect(request.get).toHaveBeenCalledWith('/api/crawler/tasks/99')
  })

  it('cancelTask 应 DELETE /api/crawler/tasks/:id', async () => {
    await crawlerApi.cancelTask(99)
    expect(request.delete).toHaveBeenCalledWith('/api/crawler/tasks/99')
  })

  it('retryTask 应 POST /api/crawler/tasks/:id/retry', async () => {
    await crawlerApi.retryTask(99)
    expect(request.post).toHaveBeenCalledWith('/api/crawler/tasks/99/retry')
  })

  it('getTemplates 应 GET /api/crawler/templates', async () => {
    await crawlerApi.getTemplates()
    expect(request.get).toHaveBeenCalledWith('/api/crawler/templates')
  })
})

// ============================================================================
// AI API 映射
// ============================================================================

describe('AI API 路由映射', () => {
  let aiReviewApi: typeof import('@/api/modules/ai').aiReviewApi
  let literatureReviewApi: typeof import('@/api/modules/ai').literatureReviewApi
  let researchPlanApi: typeof import('@/api/modules/ai').researchPlanApi
  let aiChatApi: typeof import('@/api/modules/ai').aiChatApi

  beforeEach(async () => {
    vi.clearAllMocks()
    const mod = await import('@/api/modules/ai')
    aiReviewApi = mod.aiReviewApi
    literatureReviewApi = mod.literatureReviewApi
    researchPlanApi = mod.researchPlanApi
    aiChatApi = mod.aiChatApi
  })

  it('generateReview 应 POST /api/ai/review', async () => {
    await aiReviewApi.generateReview({ paperContent: 'test', reviewType: 'full' } as any)
    expect(request.post).toHaveBeenCalledWith('/api/ai/review', expect.any(Object))
  })

  it('getReviewHistory 应 GET /api/ai-co-pilot/reviews/:userId', async () => {
    await aiReviewApi.getReviewHistory(1)
    expect(request.get).toHaveBeenCalledWith('/api/ai-co-pilot/reviews/1', expect.any(Object))
  })

  it('getReview 应 GET /api/ai-co-pilot/review/:id', async () => {
    await aiReviewApi.getReview(5)
    expect(request.get).toHaveBeenCalledWith('/api/ai-co-pilot/review/5')
  })

  it('generateLiteratureReview 应 POST /api/ai/literature-review/generate', async () => {
    await literatureReviewApi.generateReview({ topic: 'test', papers: [] } as any)
    expect(request.post).toHaveBeenCalledWith('/api/ai/literature-review/generate', expect.any(Object))
  })

  it('generatePlan 应 POST /api/ai/research-plan/generate', async () => {
    await researchPlanApi.generatePlan({ topic: 'test' } as any)
    expect(request.post).toHaveBeenCalledWith('/api/ai/research-plan/generate', expect.any(Object))
  })

  it('chat 应 POST /api/ai/chat', async () => {
    await aiChatApi.chat(1, 'hello')
    expect(request.post).toHaveBeenCalledWith('/api/ai/chat', expect.any(Object))
  })
})

// ============================================================================
// Admin API 映射
// ============================================================================

describe('Admin API 路由映射', () => {
  let getAdminUsersFn: typeof import('@/api/modules/admin').getAdminUsers
  let getModulesFn: typeof import('@/api/modules/admin').getModules
  let getAuditLogsFn: typeof import('@/api/modules/admin').getAuditLogs

  beforeEach(async () => {
    vi.clearAllMocks()
    const mod = await import('@/api/modules/admin')
    getAdminUsersFn = mod.getAdminUsers
    getModulesFn = mod.getModules
    getAuditLogsFn = mod.getAuditLogs

    // Admin 模块用 request() 函数式调用，mock 需要返回适配器能处理的数据
    mockRequest.mockResolvedValue({ users: [], pagination: { page: 1, limit: 20, total: 0 } })
    mockGet.mockResolvedValue({ users: [], pagination: { page: 1, limit: 20, total: 0 } })
  })

  it('getAdminUsers 应调用 /api/admin/users', async () => {
    await getAdminUsersFn()
    expect(mockRequest).toHaveBeenCalledWith(expect.objectContaining({
      url: '/api/admin/users',
      method: 'GET'
    }))
  })

  it('getModules 应调用 /api/admin/modules', async () => {
    mockRequest.mockResolvedValueOnce([])
    await getModulesFn()
    expect(mockRequest).toHaveBeenCalledWith(expect.objectContaining({
      url: '/api/admin/modules',
      method: 'GET'
    }))
  })

  it('getAuditLogs 应调用 /api/admin/audit-logs', async () => {
    mockRequest.mockResolvedValueOnce({ logs: [], pagination: { page: 1, limit: 20, total: 0 } })
    await getAuditLogsFn()
    expect(mockRequest).toHaveBeenCalledWith(expect.objectContaining({
      url: '/api/admin/audit-logs',
      method: 'GET'
    }))
  })
})

// ============================================================================
// Export API 映射
// ============================================================================

describe('Export API 路由映射', () => {
  let exportApi: typeof import('@/api/modules/export').exportApi

  beforeEach(async () => {
    vi.clearAllMocks()
    const mod = await import('@/api/modules/export')
    exportApi = mod.exportApi
  })

  it('exportToCSV 应 GET /api/export/csv', async () => {
    await exportApi.exportToCSV()
    expect(request.get).toHaveBeenCalledWith('/api/export/csv', expect.any(Object))
  })

  it('exportToJSON 应 GET /api/export/json', async () => {
    await exportApi.exportToJSON()
    expect(request.get).toHaveBeenCalledWith('/api/export/json', expect.any(Object))
  })

  it('exportToExcel 应 GET /api/export/excel', async () => {
    await exportApi.exportToExcel()
    expect(request.get).toHaveBeenCalledWith('/api/export/excel', expect.any(Object))
  })
})

// ============================================================================
// Search API 映射
// ============================================================================

describe('Search API 路由映射', () => {
  let searchApi: typeof import('@/api/modules/search').searchApi

  beforeEach(async () => {
    vi.clearAllMocks()
    const mod = await import('@/api/modules/search')
    searchApi = mod.searchApi
  })

  it('advancedSearch 应 POST /api/search/advanced', async () => {
    await searchApi.advancedSearch({ query: 'test' })
    expect(request.post).toHaveBeenCalledWith('/api/search/advanced', expect.any(Object))
  })

  it('getSuggestions 应 GET /api/search/suggest', async () => {
    await searchApi.getSuggestions('test')
    expect(request.get).toHaveBeenCalledWith('/api/search/suggest', expect.any(Object))
  })
})

// ============================================================================
// Analytics API 映射
// ============================================================================

describe('Analytics API 路由映射', () => {
  let analyticsApi: typeof import('@/api/modules/analytics').analyticsApi

  beforeEach(async () => {
    vi.clearAllMocks()
    const mod = await import('@/api/modules/analytics')
    analyticsApi = mod.analyticsApi
  })

  it('getImpactMetrics 应 GET /api/analytics/impact/:userId', async () => {
    await analyticsApi.getImpactMetrics(1)
    expect(request.get).toHaveBeenCalledWith('/api/analytics/impact/1', expect.any(Object))
  })

  it('getResearchInterests 应 GET /api/analytics/interests/:userId', async () => {
    await analyticsApi.getResearchInterests(1)
    expect(request.get).toHaveBeenCalledWith('/api/analytics/interests/1')
  })

  it('getTrendingTopics 应 GET /api/analytics/trends', async () => {
    await analyticsApi.getTrendingTopics()
    expect(request.get).toHaveBeenCalledWith('/api/analytics/trends', expect.any(Object))
  })
})

// ============================================================================
// Health API 映射
// ============================================================================

describe('Health API 路由映射', () => {
  let healthApi: typeof import('@/api/modules/health').healthApi

  beforeEach(async () => {
    vi.clearAllMocks()
    const mod = await import('@/api/modules/health')
    healthApi = mod.healthApi
  })

  it('check 应 GET /api/health', async () => {
    await healthApi.check()
    expect(request.get).toHaveBeenCalledWith('/api/health')
  })

  it('detailedCheck 应 GET /api/health/detailed', async () => {
    await healthApi.detailedCheck()
    expect(request.get).toHaveBeenCalledWith('/api/health/detailed')
  })

  it('isReady 应 GET /api/health/ready', async () => {
    await healthApi.isReady()
    expect(request.get).toHaveBeenCalledWith('/api/health/ready')
  })

  it('isAlive 应 GET /api/health/live', async () => {
    await healthApi.isAlive()
    expect(request.get).toHaveBeenCalledWith('/api/health/live')
  })
})

// ============================================================================
// HTTP 方法语义正确性
// ============================================================================

describe('HTTP 方法语义正确性', () => {
  let papersApi: typeof import('@/api/modules/papers').papersApi

  beforeEach(async () => {
    vi.clearAllMocks()
    const mod = await import('@/api/modules/papers')
    papersApi = mod.papersApi
  })

  it('获取数据应使用 GET 方法', async () => {
    await papersApi.getPapers()
    expect(request.get).toHaveBeenCalled()
  })

  it('创建数据应使用 POST 方法', async () => {
    await papersApi.createPaper({ title: 'T', authors: 'A', year: 2024, venue: 'V', abstract: '' })
    expect(request.post).toHaveBeenCalled()
  })

  it('更新数据应使用 PUT 方法', async () => {
    await papersApi.updatePaper(1, { title: 'Updated' })
    expect(request.put).toHaveBeenCalled()
  })

  it('删除数据应使用 DELETE 方法', async () => {
    await papersApi.deletePaper(1)
    expect(request.delete).toHaveBeenCalled()
  })
})
