/**
 * WebSocket Manager for Real-time Communication
 *
 * Manages WebSocket connections for:
 * - Real-time collaborative editing
 * - Live AI generation updates
 * - Progress tracking
 * - Multi-user synchronization
 */

import { ref, computed } from 'vue'

export type WebSocketStatus = 'connecting' | 'connected' | 'disconnected' | 'error'
export type WebSocketEventType =
  | 'connection'
  | 'progress'
  | 'message'
  | 'error'
  | 'collaboration'
  | 'ai_update'

export interface WebSocketMessage {
  type: WebSocketEventType
  payload: any
  timestamp: number
  sessionId?: string
}

export interface WebSocketOptions {
  url: string
  reconnectInterval?: number
  maxReconnectAttempts?: number
  onMessage?: (message: WebSocketMessage) => void
  onStatusChange?: (status: WebSocketStatus) => void
  onError?: (error: Error) => void
}

class WebSocketManager {
  private ws: WebSocket | null = null
  private reconnectTimer: number | null = null
  private reconnectAttempts = 0
  private options: Required<WebSocketOptions>
  private messageQueue: WebSocketMessage[] = []

  status = ref<WebSocketStatus>('disconnected')
  isReady = computed(() => this.status.value === 'connected')

  constructor(options: WebSocketOptions) {
    this.options = {
      url: options.url,
      reconnectInterval: options.reconnectInterval || 3000,
      maxReconnectAttempts: options.maxReconnectAttempts || 5,
      onMessage: options.onMessage || (() => {}),
      onStatusChange: options.onStatusChange || (() => {}),
      onError: options.onError || (() => {})
    }
  }

  /**
   * Connect to WebSocket server
   */
  connect(): void {
    if (this.ws?.readyState === WebSocket.OPEN) {
      console.log('[WebSocket] Already connected')
      return
    }

    this.setStatus('connecting')

    try {
      this.ws = new WebSocket(this.options.url)

      this.ws.onopen = () => {
        console.log('[WebSocket] Connected')
        this.setStatus('connected')
        this.reconnectAttempts = 0

        // Send queued messages
        this.flushMessageQueue()
      }

      this.ws.onmessage = (event) => {
        try {
          const message: WebSocketMessage = JSON.parse(event.data)
          this.options.onMessage(message)
        } catch (error) {
          console.error('[WebSocket] Failed to parse message:', error)
        }
      }

      this.ws.onerror = (error) => {
        console.error('[WebSocket] Error:', error)
        this.setStatus('error')
        this.options.onError(new Error('WebSocket connection error'))
      }

      this.ws.onclose = () => {
        console.log('[WebSocket] Connection closed')
        this.setStatus('disconnected')
        this.scheduleReconnect()
      }
    } catch (error) {
      console.error('[WebSocket] Failed to connect:', error)
      this.setStatus('error')
      this.options.onError(error as Error)
    }
  }

  /**
   * Disconnect from WebSocket server
   */
  disconnect(): void {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer)
      this.reconnectTimer = null
    }

    if (this.ws) {
      this.ws.close()
      this.ws = null
    }

    this.setStatus('disconnected')
  }

  /**
   * Send message to WebSocket server
   */
  send(type: WebSocketEventType, payload: any): void {
    const message: WebSocketMessage = {
      type,
      payload,
      timestamp: Date.now()
    }

    if (this.isReady.value && this.ws) {
      try {
        this.ws.send(JSON.stringify(message))
      } catch (error) {
        console.error('[WebSocket] Failed to send message:', error)
        this.messageQueue.push(message)
      }
    } else {
      console.log('[WebSocket] Queuing message (not connected)')
      this.messageQueue.push(message)
    }
  }

  /**
   * Send progress update
   */
  sendProgress(taskId: string, progress: number, status: string): void {
    this.send('progress', {
      taskId,
      progress,
      status,
      timestamp: Date.now()
    })
  }

  /**
   * Send AI generation update
   */
  sendAIUpdate(
    taskId: string,
    stage: string,
    progress: number,
    data?: any
  ): void {
    this.send('ai_update', {
      taskId,
      stage,
      progress,
      data,
      timestamp: Date.now()
    })
  }

  /**
   * Send collaborative editing operation
   */
  sendCollaboration(
    documentId: string,
    operation: {
      type: 'insert' | 'delete' | 'retain'
      position: number
      content?: string
      length?: number
    },
    userId: number
  ): void {
    this.send('collaboration', {
      documentId,
      operation,
      userId,
      timestamp: Date.now()
    })
  }

  /**
   * Set connection status
   */
  private setStatus(status: WebSocketStatus): void {
    this.status.value = status
    this.options.onStatusChange(status)
  }

  /**
   * Schedule reconnection attempt
   */
  private scheduleReconnect(): void {
    if (this.reconnectAttempts >= this.options.maxReconnectAttempts) {
      console.error('[WebSocket] Max reconnection attempts reached')
      return
    }

    this.reconnectAttempts++

    console.log(
      `[WebSocket] Scheduling reconnection (${this.reconnectAttempts}/${this.options.maxReconnectAttempts})`
    )

    this.reconnectTimer = window.setTimeout(() => {
      console.log('[WebSocket] Reconnecting...')
      this.connect()
    }, this.options.reconnectInterval)
  }

  /**
   * Send all queued messages
   */
  private flushMessageQueue(): void {
    while (this.messageQueue.length > 0 && this.isReady.value) {
      const message = this.messageQueue.shift()
      if (message && this.ws) {
        try {
          this.ws.send(JSON.stringify(message))
        } catch (error) {
          console.error('[WebSocket] Failed to send queued message:', error)
          // Put message back at front of queue
          this.messageQueue.unshift(message)
          break
        }
      }
    }
  }
}

/**
 * Create WebSocket connection for AI features
 */
export function createAIWebSocket(): WebSocketManager {
  const wsUrl = import.meta.env.VITE_WS_URL || 'ws://localhost:8080/ws'

  return new WebSocketManager({
    url: wsUrl,
    reconnectInterval: 3000,
    maxReconnectAttempts: 5,
    onMessage: (message) => {
      console.log('[WebSocket] Message received:', message)
    },
    onStatusChange: (status) => {
      console.log('[WebSocket] Status changed:', status)
    },
    onError: (error) => {
      console.error('[WebSocket] Error:', error)
    }
  })
}

/**
 * Create WebSocket connection for collaborative editing
 */
export function createCollaborationWebSocket(documentId: string): WebSocketManager {
  const wsUrl = `${
    import.meta.env.VITE_WS_URL || 'ws://localhost:8080/ws'
  }/collaborate/${documentId}`

  return new WebSocketManager({
    url: wsUrl,
    reconnectInterval: 2000,
    maxReconnectAttempts: 10,
    onMessage: (message) => {
      console.log('[Collaboration WS] Message received:', message)
    },
    onStatusChange: (status) => {
      console.log('[Collaboration WS] Status changed:', status)
    },
    onError: (error) => {
      console.error('[Collaboration WS] Error:', error)
    }
  })
}

export default WebSocketManager
