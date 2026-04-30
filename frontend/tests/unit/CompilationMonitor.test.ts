/**
 * 编译性能监控单元测试
 * 测试编译统计、历史记录、报告导出
 */

import { describe, it, expect, beforeEach, vi, afterEach } from 'vitest'
import { mount } from '@vue/test-utils'
import CompilationMonitor from '@/components/latex/CompilationMonitor.vue'

// Mock Element Plus components
vi.mock('element-plus', () => ({
  ElMessage: {
    success: vi.fn(),
    warning: vi.fn(),
    error: vi.fn(),
    info: vi.fn()
  },
  ElTag: {
    name: 'ElTag',
    template: '<div class="el-tag"><slot /></div>',
    props: ['type', 'size']
  }
}))

describe('CompilationMonitor', () => {
  let wrapper: any

  beforeEach(() => {
    vi.clearAllMocks()
    // 清空localStorage
    localStorage.clear()
  })

  afterEach(() => {
    if (wrapper) {
      wrapper.unmount()
    }
  })

  describe('初始化', () => {
    it('应该正确渲染组件', () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc-123'
        }
      })

      expect(wrapper.exists()).toBe(true)
      expect(wrapper.find('.compilation-monitor').exists()).toBe(true)
    })

    it('应该初始化为空闲状态', () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc-123'
        }
      })

      expect(wrapper.vm.status).toBe('idle')
    })

    it('应该加载保存的编译历史', () => {
      // 模拟保存的历史记录
      const mockHistory = [
        {
          documentId: 'test-doc',
          status: 'success',
          compileTime: 1500,
          timestamp: Date.now(),
          incremental: false,
          errors: []
        }
      ]
      localStorage.setItem('latex_compilation_history', JSON.stringify(mockHistory))

      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      expect(wrapper.vm.compilationHistory).toHaveLength(1)
    })
  })

  describe('编译记录管理', () => {
    it('应该添加编译记录', async () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      const record = {
        documentId: 'test-doc',
        documentName: 'Test Document',
        status: 'success' as const,
        compileTime: 1200,
        timestamp: Date.now(),
        incremental: false,
        log: 'Compilation successful'
      }

      await wrapper.vm.addRecord(record)

      expect(wrapper.vm.compilationHistory).toHaveLength(1)
      expect(wrapper.vm.compilationHistory[0].status).toBe('success')
    })

    it('应该限制历史记录数量为100', async () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      // 添加超过100条记录
      for (let i = 0; i < 105; i++) {
        await wrapper.vm.addRecord({
          documentId: 'test-doc',
          status: 'success' as const,
          compileTime: 1000,
          timestamp: Date.now() + i,
          incremental: false
        })
      }

      expect(wrapper.vm.compilationHistory.length).toBeLessThanOrEqual(100)
    })

    it('应该清空历史记录', async () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      await wrapper.vm.addRecord({
        documentId: 'test-doc',
        status: 'success' as const,
        compileTime: 1000,
        timestamp: Date.now(),
        incremental: false
      })

      await wrapper.vm.clearHistory()

      expect(wrapper.vm.compilationHistory).toHaveLength(0)
    })
  })

  describe('统计计算', () => {
    it('应该正确计算平均编译时间', async () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      await wrapper.vm.addRecord({
        documentId: 'test-doc',
        status: 'success' as const,
        compileTime: 1000,
        timestamp: Date.now(),
        incremental: false
      })

      await wrapper.vm.addRecord({
        documentId: 'test-doc',
        status: 'success' as const,
        compileTime: 2000,
        timestamp: Date.now(),
        incremental: false
      })

      expect(wrapper.vm.averageTime).toBe(1500)
    })

    it('应该正确计算成功率', async () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      await wrapper.vm.addRecord({
        documentId: 'test-doc',
        status: 'success' as const,
        compileTime: 1000,
        timestamp: Date.now(),
        incremental: false
      })

      await wrapper.vm.addRecord({
        documentId: 'test-doc',
        status: 'error' as const,
        compileTime: 500,
        timestamp: Date.now(),
        incremental: false
      })

      await wrapper.vm.addRecord({
        documentId: 'test-doc',
        status: 'success' as const,
        compileTime: 1000,
        timestamp: Date.now(),
        incremental: false
      })

      expect(wrapper.vm.successRate).toBe(67) // 2/3 = 66.67% → 67%
    })

    it('应该正确计算总错误数', async () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      await wrapper.vm.addRecord({
        documentId: 'test-doc',
        status: 'error' as const,
        compileTime: 500,
        timestamp: Date.now(),
        incremental: false,
        errors: [
          { line: 10, message: 'Error 1', type: 'error' },
          { line: 20, message: 'Error 2', type: 'error' }
        ]
      })

      expect(wrapper.vm.totalErrors).toBe(2)
    })
  })

  describe('状态管理', () => {
    it('应该更新状态', async () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      wrapper.vm.setStatus('compiling')

      expect(wrapper.vm.status).toBe('compiling')
    })

    it('应该根据状态返回正确的标签类型', () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      expect(wrapper.vm.getStatusType('idle')).toBe('info')
      expect(wrapper.vm.getStatusType('compiling')).toBe('warning')
      expect(wrapper.vm.getStatusType('success')).toBe('success')
      expect(wrapper.vm.getStatusType('error')).toBe('danger')
    })
  })

  describe('报告导出', () => {
    it('应该导出编译报告', async () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      await wrapper.vm.addRecord({
        documentId: 'test-doc',
        status: 'success' as const,
        compileTime: 1000,
        timestamp: Date.now(),
        incremental: false
      })

      // Mock Blob和URL
      const mockBlob = { content: '' }
      global.URL.createObjectURL = vi.fn(() => 'mock-url')
      global.URL.revokeObjectURL = vi.fn()

      const createElementSpy = vi.spyOn(document, 'createElement')
      const mockLink = {
        href: '',
        download: '',
        click: vi.fn()
      }
      createElementSpy.mockReturnValue(mockLink as any)

      await wrapper.vm.exportReport()

      expect(createElementSpy).toHaveBeenCalledWith('a')
      expect(mockLink.click).toHaveBeenCalled()
    })
  })

  describe('历史记录筛选', () => {
    beforeEach(async () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      await wrapper.vm.addRecord({
        documentId: 'test-doc',
        status: 'success' as const,
        compileTime: 1000,
        timestamp: Date.now(),
        incremental: false
      })

      await wrapper.vm.addRecord({
        documentId: 'test-doc',
        status: 'error' as const,
        compileTime: 500,
        timestamp: Date.now(),
        incremental: false
      })

      await wrapper.vm.addRecord({
        documentId: 'test-doc',
        status: 'warning' as const,
        compileTime: 800,
        timestamp: Date.now(),
        incremental: false
      })
    })

    it('应该筛选成功的编译', () => {
      wrapper.vm.historyFilter = 'success'

      expect(wrapper.vm.filteredHistory).toHaveLength(1)
      expect(wrapper.vm.filteredHistory[0].status).toBe('success')
    })

    it('应该筛选失败的编译', () => {
      wrapper.vm.historyFilter = 'error'

      expect(wrapper.vm.filteredHistory).toHaveLength(1)
      expect(wrapper.vm.filteredHistory[0].status).toBe('error')
    })

    it('应该显示所有记录', () => {
      wrapper.vm.historyFilter = 'all'

      expect(wrapper.vm.filteredHistory).toHaveLength(3)
    })
  })

  describe('详情查看', () => {
    it('应该显示编译详情', async () => {
      wrapper = mount(CompilationMonitor, {
        props: {
          documentId: 'test-doc'
        }
      })

      const record = {
        documentId: 'test-doc',
        documentName: 'Test Document',
        status: 'error' as const,
        compileTime: 1500,
        timestamp: Date.now(),
        incremental: false,
        log: 'Error on line 10',
        errors: [
          { line: 10, message: 'Undefined control sequence', type: 'error' }
        ]
      }

      await wrapper.vm.addRecord(record)

      wrapper.vm.viewDetails(record)

      expect(wrapper.vm.selectedRecord).toEqual(record)
      expect(wrapper.vm.detailVisible).toBe(true)
    })
  })
})
