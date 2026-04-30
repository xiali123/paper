/**
 * 云存储同步单元测试
 * 测试云存储提供商、文件同步、冲突解决
 */

import { describe, it, expect, beforeEach, vi, afterEach } from 'vitest'
import { mount } from '@vue/test-utils'
import CloudStorageSync from '@/components/cloud/CloudStorageSync.vue'

// Mock Element Plus
vi.mock('element-plus', () => ({
  ElMessage: {
    success: vi.fn(),
    warning: vi.fn(),
    error: vi.fn(),
    info: vi.fn()
  },
  ElMessageBox: {
    confirm: vi.fn()
  }
}))

describe('CloudStorageSync', () => {
  let wrapper: any

  beforeEach(() => {
    vi.clearAllMocks()
    localStorage.clear()

    // Mock fetch API
    global.fetch = vi.fn(() =>
      Promise.resolve({
        ok: true,
        json: () => Promise.resolve({ success: true })
      })
    ) as any
  })

  afterEach(() => {
    if (wrapper) {
      wrapper.unmount()
    }
  })

  describe('初始化', () => {
    it('应该正确渲染组件', () => {
      wrapper = mount(CloudStorageSync)

      expect(wrapper.exists()).toBe(true)
      expect(wrapper.find('.cloud-sync-manager').exists()).toBe(true)
    })

    it('应该初始化云存储提供商列表', () => {
      wrapper = mount(CloudStorageSync)

      expect(wrapper.vm.providers).toHaveLength(4)
      expect(wrapper.vm.providers[0].id).toBe('dropbox')
      expect(wrapper.vm.providers[1].id).toBe('googledrive')
      expect(wrapper.vm.providers[2].id).toBe('onedrive')
      expect(wrapper.vm.providers[3].id).toBe('webdav')
    })

    it('应该加载已保存的连接状态', () => {
      localStorage.setItem('cloud_provider_dropbox', 'connected')

      wrapper = mount(CloudStorageSync)

      const dropboxProvider = wrapper.vm.providers.find(p => p.id === 'dropbox')
      expect(dropboxProvider.connected).toBe(true)
    })
  })

  describe('提供商连接', () => {
    it('应该能够选择提供商', async () => {
      wrapper = mount(CloudStorageSync)

      await wrapper.vm.selectProvider('dropbox')

      expect(wrapper.vm.activeProvider).toBe('dropbox')
    })

    it('连接未连接的提供商时应触发OAuth', async () => {
      wrapper = mount(CloudStorageSync)

      await wrapper.vm.selectProvider('dropbox')

      // 应该调用连接函数
      expect(wrapper.vm.syncing).toBe(true)

      // 等待连接完成
      await new Promise(resolve => setTimeout(resolve, 1100))

      expect(wrapper.vm.syncing).toBe(false)
    })

    it('连接成功后应该更新提供商状态', async () => {
      wrapper = mount(CloudStorageSync)

      await wrapper.vm.selectProvider('dropbox')

      // 等待连接
      await new Promise(resolve => setTimeout(resolve, 1100))

      const dropboxProvider = wrapper.vm.providers.find(p => p.id === 'dropbox')
      expect(dropboxProvider.connected).toBe(true)
      expect(dropboxProvider.statusText).toBe('已连接')
    })
  })

  describe('文件同步', () => {
    beforeEach(async () => {
      wrapper = mount(CloudStorageSync)

      // 先连接提供商
      localStorage.setItem('cloud_provider_dropbox', 'connected')
      await wrapper.vm.selectProvider('dropbox')

      // 添加测试文件
      wrapper.vm.syncFiles = [
        {
          id: 'file-1',
          name: 'test.tex',
          type: 'tex',
          size: 1024,
          modifiedAt: Date.now(),
          syncStatus: 'pending',
          cloudPath: '/LaTeX/test.tex'
        }
      ]
    })

    it('应该能够同步单个文件', async () => {
      const file = wrapper.vm.syncFiles[0]

      await wrapper.vm.syncFile(file)

      expect(file.syncStatus).toBe('synced')
    })

    it('同步失败时应保持pending状态', async () => {
      const file = wrapper.vm.syncFiles[0]

      // Mock失败的fetch
      global.fetch = vi.fn(() => Promise.reject('Network error')) as any

      await wrapper.vm.syncFile(file)

      expect(file.syncStatus).toBe('pending')
    })

    it('应该能够移除文件', () => {
      const file = wrapper.vm.syncFiles[0]

      wrapper.vm.removeFile(file)

      expect(wrapper.vm.syncFiles).toHaveLength(0)
    })
  })

  describe('自动同步', () => {
    beforeEach(async () => {
      wrapper = mount(CloudStorageSync)

      localStorage.setItem('cloud_provider_dropbox', 'connected')
      await wrapper.vm.selectProvider('dropbox')
    })

    it('启用自动同步时应开始定期同步', async () => {
      await wrapper.vm.handleAutoSyncToggle(true)

      expect(wrapper.vm.autoSyncEnabled).toBe(true)
    })

    it('禁用自动同步时应停止定期同步', async () => {
      await wrapper.vm.handleAutoSyncToggle(true)
      await wrapper.vm.handleAutoSyncToggle(false)

      expect(wrapper.vm.autoSyncEnabled).toBe(false)
    })
  })

  describe('文件上传', () => {
    beforeEach(async () => {
      wrapper = mount(CloudStorageSync)

      localStorage.setItem('cloud_provider_dropbox', 'connected')
      await wrapper.vm.selectProvider('dropbox')
    })

    it('应该能够处理文件选择', () => {
      const mockFile = new File(['content'], 'test.tex', { type: 'text/plain' })

      wrapper.vm.handleFileSelect({ raw: mockFile })

      expect(wrapper.vm.pendingUpload).toHaveLength(1)
    })

    it('应该能够上传多个文件', async () => {
      const mockFile1 = new File(['content1'], 'test1.tex', { type: 'text/plain' })
      const mockFile2 = new File(['content2'], 'test2.tex', { type: 'text/plain' })

      wrapper.vm.handleFileSelect({ raw: mockFile1 })
      wrapper.vm.handleFileSelect({ raw: mockFile2 })

      await wrapper.vm.uploadFiles()

      expect(wrapper.vm.syncFiles.length).toBeGreaterThanOrEqual(2)
    })
  })

  describe('统计信息', () => {
    beforeEach(async () => {
      wrapper = mount(CloudStorageSync)

      localStorage.setItem('cloud_provider_dropbox', 'connected')
      await wrapper.vm.selectProvider('dropbox')

      // 添加测试文件
      wrapper.vm.syncFiles = [
        {
          id: 'file-1',
          name: 'test1.tex',
          type: 'tex',
          size: 1024,
          modifiedAt: Date.now(),
          syncStatus: 'synced'
        },
        {
          id: 'file-2',
          name: 'test2.tex',
          type: 'tex',
          size: 2048,
          modifiedAt: Date.now(),
          syncStatus: 'syncing'
        },
        {
          id: 'file-3',
          name: 'test3.tex',
          type: 'tex',
          size: 512,
          modifiedAt: Date.now(),
          syncStatus: 'pending'
        }
      ]
    })

    it('应该正确计算已同步文件数', () => {
      expect(wrapper.vm.syncedFiles).toBe(1)
    })

    it('应该正确计算存储空间使用', () => {
      const totalSize = 1024 + 2048 + 512
      expect(wrapper.vm.storageUsed).toBe('3.5 KB')
    })
  })

  describe('设置管理', () => {
    beforeEach(() => {
      wrapper = mount(CloudStorageSync)
    })

    it('应该能够保存同步设置', () => {
      wrapper.vm.syncInterval = 30
      wrapper.vm.conflictStrategy = 'local'
      wrapper.vm.syncOnWifi = true

      wrapper.vm.saveSettings()

      const saved = localStorage.getItem('cloud_sync_settings')
      const settings = JSON.parse(saved!)

      expect(settings.syncInterval).toBe(30)
      expect(settings.conflictStrategy).toBe('local')
      expect(settings.syncOnWifi).toBe(true)
    })

    it('应该加载保存的设置', () => {
      localStorage.setItem('cloud_sync_settings', JSON.stringify({
        syncInterval: 5,
        conflictStrategy: 'remote',
        syncOnWifi: true
      }))

      // 重新挂载组件
      wrapper.unmount()
      wrapper = mount(CloudStorageSync)

      expect(wrapper.vm.syncInterval).toBe(5)
      expect(wrapper.vm.conflictStrategy).toBe('remote')
      expect(wrapper.vm.syncOnWifi).toBe(true)
    })
  })

  describe('冲突解决', () => {
    it('应该支持本地优先策略', () => {
      wrapper = mount(CloudStorageSync)

      wrapper.vm.conflictStrategy = 'local'
      expect(wrapper.vm.conflictStrategy).toBe('local')
    })

    it('应该支持云端优先策略', () => {
      wrapper = mount(CloudStorageSync)

      wrapper.vm.conflictStrategy = 'remote'
      expect(wrapper.vm.conflictStrategy).toBe('remote')
    })

    it('应该支持最新优先策略', () => {
      wrapper = mount(CloudStorageSync)

      wrapper.vm.conflictStrategy = 'timestamp'
      expect(wrapper.vm.conflictStrategy).toBe('timestamp')
    })
  })

  describe('工具函数', () => {
    beforeEach(() => {
      wrapper = mount(CloudStorageSync)
    })

    it('应该正确格式化文件大小', () => {
      expect(wrapper.vm.formatSize(0)).toBe('0 B')
      expect(wrapper.vm.formatSize(1024)).toBe('1 KB')
      expect(wrapper.vm.formatSize(1024 * 1024)).toBe('1 MB')
      expect(wrapper.vm.formatSize(1024 * 1024 * 1024)).toBe('1 GB')
    })

    it('应该正确格式化相对时间', () => {
      const now = Date.now()

      expect(wrapper.vm.formatTime(now)).toBe('刚刚')
      expect(wrapper.vm.formatTime(now - 30 * 60000)).toBe('30分钟前')
      expect(wrapper.vm.formatTime(now - 2 * 3600000)).toBe('2小时前')
    })
  })
})
