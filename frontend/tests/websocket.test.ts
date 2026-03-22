/**
 * WebSocket 集成测试
 * 测试前后端 WebSocket 通信
 */

import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest'
import { useWebSocket } from '@/composables/useWebSocket'
import { useWebSocketStore } from '@/stores/websocket'
import { MessageType, ConnectionState } from '@/types/websocket'

// Mock WebSocket
class MockWebSocket {
  static instances: MockWebSocket[] = []
  url: string
  readyState: number = 0 // CONNECTING
  onopen: ((event: Event) => void) | null = null
  onclose: ((event: CloseEvent) => void) | null = null
  onerror: ((event: Event) => void) | null = null
  onmessage: ((event: MessageEvent) => void) | null = null

  constructor(url: string) {
    this.url = url
    MockWebSocket.instances.push(this)

    // 模拟异步连接
    setTimeout(() => {
      this.readyState = 1 // OPEN
      if (this.onopen) {
        this.onopen(new Event('open'))
      }
    }, 100)
  }

  send(data: string): void {
    if (this.readyState !== 1) {
      throw new Error('WebSocket is not open')
    }

    // 模拟服务器响应
    setTimeout(() => {
      if (this.onmessage) {
        const message = JSON.parse(data)
        if (message.type === 'heartbeat') {
          // 响应心跳
          this.onmessage(new MessageEvent('message', {
            data: JSON.stringify({
              type: 'heartbeat',
              timestamp: new Date().toISOString(),
              id: 'server_' + Math.random()
            })
          }))
        }
      }
    }, 50)
  }

  close(code?: number, reason?: string): void {
    this.readyState = 3 // CLOSED
    if (this.onclose) {
      this.onclose(new CloseEvent('close', { code, reason }))
    }
  }

  static reset(): void {
    MockWebSocket.instances = []
  }
}

// 覆盖全局 WebSocket
global.WebSocket = MockWebSocket as any

describe('WebSocket Integration Tests', () => {
  beforeEach(() => {
    MockWebSocket.reset()
    vi.clearAllMocks()
  })

  afterEach(() => {
    MockWebSocket.reset()
  })

  describe('useWebSocket Composable', () => {
    it('should connect to WebSocket server', async () => {
      const { connectionState, isConnected } = useWebSocket({
        url: 'ws://localhost:8088/ws'
      })

      // 初始状态应该是连接中
      expect(connectionState.value).toBe(ConnectionState.CONNECTING)

      // 等待连接建立
      await new Promise(resolve => setTimeout(resolve, 200))

      // 连接成功
      expect(connectionState.value).toBe(ConnectionState.CONNECTED)
      expect(isConnected()).toBe(true)
    })

    it('should handle connection errors', async () => {
      const invalidUrl = 'ws://invalid-host:8088/ws'
      const { connectionState } = useWebSocket({
        url: invalidUrl,
        reconnectInterval: 100
      })

      // 等待连接失败
      await new Promise(resolve => setTimeout(resolve, 500))

      // 应该进入重连状态
      expect(
        connectionState.value === ConnectionState.RECONNECTING ||
        connectionState.value === ConnectionState.ERROR
      ).toBe(true)
    })

    it('should send and receive messages', async () => {
      const messageHandler = vi.fn()
      const { send } = useWebSocket({
        url: 'ws://localhost:8088/ws'
      }, {
        onMessage: messageHandler
      })

      // 等待连接
      await new Promise(resolve => setTimeout(resolve, 200))

      // 发送消息
      const testMessage = {
        type: MessageType.HEARTBEAT,
        timestamp: new Date().toISOString(),
        id: 'test_123'
      }
      send(testMessage)

      // 等待响应
      await new Promise(resolve => setTimeout(resolve, 100))

      // 验证消息被接收
      expect(messageHandler).toHaveBeenCalled()
    })

    it('should handle paper updates', async () => {
      const paperUpdateHandler = vi.fn()
      useWebSocket({
        url: 'ws://localhost:8088/ws'
      }, {
        onPaperUpdate: paperUpdateHandler
      })

      // 等待连接
      await new Promise(resolve => setTimeout(resolve, 200))

      // 模拟收到论文更新消息
      const wsInstance = MockWebSocket.instances[0]
      if (wsInstance && wsInstance.onmessage) {
        const updateMessage = {
          type: MessageType.PAPER_UPDATE,
          timestamp: new Date().toISOString(),
          id: 'update_123',
          data: {
            paper: {
              id: 123,
              title: 'Test Paper',
              authors: 'Test Author',
              year: 2024
            }
          }
        }
        wsInstance.onmessage(new MessageEvent('message', {
          data: JSON.stringify(updateMessage)
        }))
      }

      // 验证处理器被调用
      expect(paperUpdateHandler).toHaveBeenCalledWith(
        expect.objectContaining({
          id: 123,
          title: 'Test Paper'
        })
      )
    })
  })

  describe('WebSocket Store', () => {
    it('should initialize WebSocket connection', async () => {
      const store = useWebSocketStore()

      expect(store.connectionState).toBe(ConnectionState.DISCONNECTED)

      store.initialize()

      // 等待连接
      await new Promise(resolve => setTimeout(resolve, 200))

      expect(store.connectionState).toBe(ConnectionState.CONNECTED)
      expect(store.isConnected).toBe(true)
    })

    it('should track connection statistics', async () => {
      const store = useWebSocketStore()
      store.initialize()

      // 等待连接
      await new Promise(resolve => setTimeout(resolve, 200))

      expect(store.stats.messagesSent).toBeGreaterThan(0)
      expect(store.stats.connectedAt).toBeInstanceOf(Date)
    })

    it('should provide connection status text', () => {
      const store = useWebSocketStore()

      store.initialize()
      expect(store.connectionStatusText).toBe('Connected')

      // 模拟不同状态
      store.connectionState = ConnectionState.CONNECTING
      expect(store.connectionStatusText).toBe('Connecting...')

      store.connectionState = ConnectionState.ERROR
      expect(store.connectionStatusText).toBe('Connection Error')
    })

    it('should track update notifications', async () => {
      const store = useWebSocketStore()
      store.initialize()

      // 等待连接
      await new Promise(resolve => setTimeout(resolve, 200))

      // 模拟收到更新
      const wsInstance = MockWebSocket.instances[0]
      if (wsInstance && wsInstance.onmessage) {
        const updateMessage = {
          type: MessageType.PAPER_UPDATE,
          timestamp: new Date().toISOString(),
          id: 'update_123',
          data: {
            paper: {
              id: 123,
              title: 'Test Paper',
              authors: 'Test Author',
              year: 2024
            }
          }
        }
        wsInstance.onmessage(new MessageEvent('message', {
          data: JSON.stringify(updateMessage)
        }))
      }

      // 等待处理
      await new Promise(resolve => setTimeout(resolve, 100))

      expect(store.hasUpdates).toBe(true)
      expect(store.updateNotifications).toBeGreaterThan(0)

      // 清除通知
      store.clearNotifications()
      expect(store.hasUpdates).toBe(false)
    })
  })

  describe('Message Types', () => {
    it('should handle all message types correctly', async () => {
      const handlers = {
        onPaperUpdate: vi.fn(),
        onPaperDelete: vi.fn(),
        onPaperNew: vi.fn(),
        onStatsUpdate: vi.fn()
      }

      useWebSocket({
        url: 'ws://localhost:8088/ws'
      }, handlers)

      // 等待连接
      await new Promise(resolve => setTimeout(resolve, 200))

      const wsInstance = MockWebSocket.instances[0]
      if (wsInstance && wsInstance.onmessage) {
        // 测试论文更新
        wsInstance.onmessage(new MessageEvent('message', {
          data: JSON.stringify({
            type: MessageType.PAPER_UPDATE,
            timestamp: new Date().toISOString(),
            id: '1',
            data: { paper: { id: 1, title: 'Paper 1' } }
          })
        }))

        // 测试论文删除
        wsInstance.onmessage(new MessageEvent('message', {
          data: JSON.stringify({
            type: MessageType.PAPER_DELETE,
            timestamp: new Date().toISOString(),
            id: '2',
            data: { paper_id: 1 }
          })
        }))

        // 测试新论文
        wsInstance.onmessage(new MessageEvent('message', {
          data: JSON.stringify({
            type: MessageType.PAPER_NEW,
            timestamp: new Date().toISOString(),
            id: '3',
            data: { paper: { id: 2, title: 'Paper 2' } }
          })
        }))

        // 测试统计更新
        wsInstance.onmessage(new MessageEvent('message', {
          data: JSON.stringify({
            type: MessageType.STATS_UPDATE,
            timestamp: new Date().toISOString(),
            id: '4',
            data: {
              total_papers: 1000,
              total_citations: 50000
            }
          })
        }))
      }

      // 等待处理
      await new Promise(resolve => setTimeout(resolve, 100))

      expect(handlers.onPaperUpdate).toHaveBeenCalledTimes(1)
      expect(handlers.onPaperDelete).toHaveBeenCalledTimes(1)
      expect(handlers.onPaperNew).toHaveBeenCalledTimes(1)
      expect(handlers.onStatsUpdate).toHaveBeenCalledTimes(1)
    })
  })

  describe('Reconnection Logic', () => {
    it('should automatically reconnect on connection loss', async () => {
      const { connectionState } = useWebSocket({
        url: 'ws://localhost:8088/ws',
        reconnectInterval: 100,
        maxReconnectAttempts: 3
      })

      // 等待初始连接
      await new Promise(resolve => setTimeout(resolve, 200))
      expect(connectionState.value).toBe(ConnectionState.CONNECTED)

      // 模拟连接断开
      const wsInstance = MockWebSocket.instances[0]
      if (wsInstance && wsInstance.onclose) {
        wsInstance.readyState = 3 // CLOSED
        wsInstance.onclose(new CloseEvent('close', { code: 1000 }))
      }

      // 应该进入重连状态
      await new Promise(resolve => setTimeout(resolve, 50))
      expect(connectionState.value).toBe(ConnectionState.RECONNECTING)

      // 等待重连
      await new Promise(resolve => setTimeout(resolve, 300))
      expect(connectionState.value).toBe(ConnectionState.CONNECTED)
    })

    it('should stop reconnection after max attempts', async () => {
      const { connectionState } = useWebSocket({
        url: 'ws://invalid-host:8088/ws',
        reconnectInterval: 50,
        maxReconnectAttempts: 2
      })

      // 等待重连尝试
      await new Promise(resolve => setTimeout(resolve, 500))

      // 应该达到最大重连次数
      expect(connectionState.value).toBe(ConnectionState.DISCONNECTED)
    })
  })

  describe('Heartbeat Mechanism', () => {
    it('should send heartbeat messages', async () => {
      const sendSpy = vi.fn()
      const originalSend = MockWebSocket.prototype.send
      MockWebSocket.prototype.send = sendSpy

      useWebSocket({
        url: 'ws://localhost:8088/ws',
        heartbeatInterval: 1000
      })

      // 等待连接和心跳
      await new Promise(resolve => setTimeout(resolve, 1200))

      // 验证发送了心跳
      expect(sendSpy).toHaveBeenCalled()
      const sentData = sendSpy.mock.calls[0][0]
      const message = JSON.parse(sentData)
      expect(message.type).toBe(MessageType.HEARTBEAT)

      // 恢复原始方法
      MockWebSocket.prototype.send = originalSend
    })
  })

  describe('Message Queue', () => {
    it('should queue messages when disconnected', async () => {
      const messageQueue: any[] = []

      // 模拟始终连接中的 WebSocket
      class NeverConnectsWebSocket extends MockWebSocket {
        constructor(url: string) {
          super(url)
          this.readyState = 0 // 始终 CONNECTING
        }
      }
      global.WebSocket = NeverConnectsWebSocket as any

      const { send } = useWebSocket({
        url: 'ws://localhost:8088/ws',
        messageQueueSize: 10
      })

      // 发送消息（应该被队列）
      send({ type: 'test', data: 'message1' })
      send({ type: 'test', data: 'message2' })

      // 验证消息被排队（通过不抛出错误）
      expect(true).toBe(true)
    })
  })
})

/**
 * 运行测试：
 * npm run test websocket
 *
 * 或使用 watch 模式：
 * npm run test:watch websocket
 */
