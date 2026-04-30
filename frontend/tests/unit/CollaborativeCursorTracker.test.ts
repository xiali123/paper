/**
 * 协作光标追踪器单元测试
 * 测试光标同步、冲突检测、编辑锁
 */

import { describe, it, expect, beforeEach, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { ref } from 'vue'
import CollaborativeCursorTracker from '@/components/collaboration/CollaborativeCursorTracker.vue'

// Mock Monaco Editor
const mockMonacoEditor = {
  getPosition: vi.fn(),
  getSelection: vi.fn(),
  onDidChangeCursorPosition: vi.fn(),
  onDidChangeCursorSelection: vi.fn()
}

global.monacoEditor = mockMonacoEditor

describe('CollaborativeCursorTracker', () => {
  let wrapper: any

  const mockProps = {
    documentId: 'test-doc-123',
    currentUserId: 'user-1',
    editorLines: 100
  }

  beforeEach(() => {
    vi.clearAllMocks()

    // 重置mock
    mockMonacoEditor.getPosition.mockReturnValue({ lineNumber: 5, column: 10 })
    mockMonacoEditor.getSelection.mockReturnValue({
      startLineNumber: 1,
      startColumn: 1,
      endLineNumber: 1,
      endColumn: 1
    })
  })

  afterEach(() => {
    if (wrapper) {
      wrapper.unmount()
    }
  })

  describe('组件渲染', () => {
    it('应该正确渲染组件', () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      expect(wrapper.exists()).toBe(true)
      expect(wrapper.find('.cursor-tracker').exists()).toBe(true)
    })

    it('应该显示远程光标', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      // 模拟接收到远程光标更新
      await wrapper.vm.handleCursorUpdate({
        userId: 'user-2',
        userName: 'Alice',
        line: 10,
        column: 5
      })

      const cursors = wrapper.findAll('.remote-cursor')
      expect(cursors.length).toBeGreaterThan(0)
    })

    it('应该显示冲突警告', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      // 模拟另一个用户在同一位置
      await wrapper.vm.handleCursorUpdate({
        userId: 'user-2',
        userName: 'Bob',
        line: 5,
        column: 8
      })

      const hasConflict = wrapper.vm.hasConflict
      expect(hasConflict).toBe(true)
    })
  })

  describe('光标同步', () => {
    it('应该发送本地光标位置', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      const sendSpy = vi.spyOn(wrapper.vm.ws, 'send')

      // 触发光标变化
      mockMonacoEditor.onDidChangeCursorPosition.mockImplementation((callback) => {
        callback()
      })

      await wrapper.vm.setupEditorListeners()

      // 等待节流
      await new Promise(resolve => setTimeout(resolve, 150))

      expect(sendSpy).toHaveBeenCalled()
    })

    it('应该忽略自己的光标更新', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      await wrapper.vm.handleCursorUpdate({
        userId: 'user-1', // 当前用户
        userName: 'Me',
        line: 5,
        column: 10
      })

      expect(wrapper.vm.remoteCursors).toHaveLength(0)
    })

    it('应该自动清理过期的光标', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      await wrapper.vm.handleCursorUpdate({
        userId: 'user-2',
        userName: 'Alice',
        line: 10,
        column: 5
      })

      expect(wrapper.vm.remoteCursors).toHaveLength(1)

      // 等待5秒过期时间
      await new Promise(resolve => setTimeout(resolve, 6000))

      // 触发清理
      const now = Date.now()
      wrapper.vm.remoteCursors = wrapper.vm.remoteCursors.filter(
        c => now - c.timestamp < 5000
      )

      expect(wrapper.vm.remoteCursors).toHaveLength(0)
    })
  })

  describe('冲突检测', () => {
    it('应该在3行内检测到冲突', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      // 当前光标在第5行
      mockMonacoEditor.getPosition.mockReturnValue({ lineNumber: 5, column: 10 })

      // 另一个用户在第7行（距离2行）
      await wrapper.vm.handleCursorUpdate({
        userId: 'user-2',
        userName: 'Alice',
        line: 7,
        column: 5
      })

      expect(wrapper.vm.hasConflict).toBe(true)
    })

    it('应该返回冲突用户列表', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      mockMonacoEditor.getPosition.mockReturnValue({ lineNumber: 5, column: 10 })

      await wrapper.vm.handleCursorUpdate({
        userId: 'user-2',
        userName: 'Alice',
        line: 6,
        column: 5
      })

      await wrapper.vm.handleCursorUpdate({
        userId: 'user-3',
        userName: 'Bob',
        line: 7,
        column: 5
      })

      const conflictUsers = wrapper.vm.conflictUsers
      expect(conflictUsers).toContain('Alice')
      expect(conflictUsers).toContain('Bob')
    })

    it('应该在距离超过3行时不检测冲突', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      mockMonacoEditor.getPosition.mockReturnValue({ lineNumber: 5, column: 10 })

      await wrapper.vm.handleCursorUpdate({
        userId: 'user-2',
        userName: 'Alice',
        line: 9, // 距离4行
        column: 5
      })

      expect(wrapper.vm.hasConflict).toBe(false)
    })
  })

  describe('编辑锁', () => {
    it('应该显示编辑锁', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      await wrapper.vm.handleEditLock({
        line: 10,
        userId: 'user-2',
        userName: 'Alice'
      })

      expect(wrapper.vm.editLocks).toHaveLength(1)
      expect(wrapper.vm.editLocks[0].line).toBe(10)
    })

    it('应该忽略自己的编辑锁', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      await wrapper.vm.handleEditLock({
        line: 10,
        userId: 'user-1', // 当前用户
        userName: 'Me'
      })

      expect(wrapper.vm.editLocks).toHaveLength(0)
    })

    it('应该自动解锁过期的锁', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      await wrapper.vm.handleEditLock({
        line: 10,
        userId: 'user-2',
        userName: 'Alice'
      })

      expect(wrapper.vm.editLocks).toHaveLength(1)

      // 等待10秒解锁时间
      await new Promise(resolve => setTimeout(resolve, 11000))

      // 解锁
      await wrapper.vm.handleEditUnlock({
        line: 10
      })

      expect(wrapper.vm.editLocks).toHaveLength(0)
    })
  })

  describe('颜色生成', () => {
    it('应该为不同用户生成一致的颜色', () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      const color1 = wrapper.vm.stringToColor('user-123')
      const color2 = wrapper.vm.stringToColor('user-123')
      const color3 = wrapper.vm.stringToColor('user-456')

      expect(color1).toBe(color2)
      expect(color1).not.toBe(color3)
    })

    it('应该使用预定义的颜色池', () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      const colors = new Set()
      for (let i = 0; i < 20; i++) {
        colors.add(wrapper.vm.stringToColor(`user-${i}`))
      }

      // 应该只有8种预定义颜色
      expect(colors.size).toBeLessThanOrEqual(8)
    })
  })

  describe('WebSocket消息处理', () => {
    it('应该处理cursor_update消息', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      await wrapper.vm.handleWebSocketMessage({
        type: 'cursor_update',
        data: {
          userId: 'user-2',
          userName: 'Alice',
          line: 10,
          column: 5
        }
      })

      expect(wrapper.vm.remoteCursors).toHaveLength(1)
    })

    it('应该处理selection_update消息', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      await wrapper.vm.handleWebSocketMessage({
        type: 'selection_update',
        data: {
          userId: 'user-2',
          userName: 'Alice',
          startLine: 5,
          endLine: 10,
          startColumn: 0,
          endColumn: 0
        }
      })

      expect(wrapper.vm.remoteSelections).toHaveLength(1)
    })

    it('应该处理edit_lock消息', async () => {
      wrapper = mount(CollaborativeCursorTracker, {
        props: mockProps
      })

      await wrapper.vm.handleWebSocketMessage({
        type: 'edit_lock',
        data: {
          line: 10,
          userId: 'user-2',
          userName: 'Alice'
        }
      })

      expect(wrapper.vm.editLocks).toHaveLength(1)
    })
  })
})
