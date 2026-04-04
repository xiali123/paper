/**
 * Crawler WebSocket Service
 *
 * Manages real-time WebSocket connection for crawler task updates
 * Handles connection lifecycle, message parsing, and event broadcasting
 *
 * @module services/crawlerWebSocket
 */

import { ref, reactive } from 'vue'
import { ElMessage } from 'element-plus'

export interface WebSocketMessage {
  type: 'task_update' | 'task_progress' | 'task_complete' | 'task_error' | 'node_status' | 'log'
  data: any
  timestamp: number
}

export interface TaskUpdateMessage extends WebSocketMessage {
  type: 'task_update'
  data: {
    taskId: string
    status: 'pending' | 'running' | 'completed' | 'failed' | 'cancelled'
    progress: number
    completedPapers: number
    totalPapers: number
    currentPaper?: any
  }
}

export interface NodeStatusMessage extends WebSocketMessage {
  type: 'node_status'
  data: {
    nodeId: string
    status: 'online' | 'offline' | 'error'
    activeTasks: number
    latency: number
  }
}

export interface LogMessage extends WebSocketMessage {
  type: 'log'
  data: {
    level: 'info' | 'warning' | 'error' | 'debug'
    message: string
    source: string
  }
}

type MessageHandler = (message: WebSocketMessage) => void
type ConnectionHandler = () => void
type ErrorHandler = (error: Event) => void

/**
 * Crawler WebSocket Service Class
 */
class CrawlerWebSocketService {
  private ws: WebSocket | null = null
  private url: string = ''
  private reconnectAttempts: number = 0
  private maxReconnectAttempts: number = 5
  private reconnectDelay: number = 3000
  private reconnectTimeout: number | null = null
  private heartbeatInterval: number | null = null
  private messageHandlers: Set<MessageHandler> = new Set()
  private connectionHandlers: Set<ConnectionHandler> = new Set()
  private disconnectionHandlers: Set<ConnectionHandler> = new Set()
  private errorHandlers: Set<ErrorHandler> = new Set()

  // Reactive state
  public connected = ref(false)
  public connecting = ref(false)
  public error = ref<string | null>(null)

  /**
   * Connect to WebSocket server
   */
  connect(wsUrl: string): Promise<void> {
    return new Promise((resolve, reject) => {
      if (this.ws?.readyState === WebSocket.OPEN) {
        resolve()
        return
      }

      this.url = wsUrl
      this.connecting.value = true
      this.error.value = null

      try {
        this.ws = new WebSocket(wsUrl)

        this.ws.onopen = () => {
          console.log('[CrawlerWebSocket] Connected')
          this.connected.value = true
          this.connecting.value = false
          this.reconnectAttempts = 0
          this.startHeartbeat()

          // Notify connection handlers
          this.connectionHandlers.forEach(handler => handler())

          resolve()
        }

        this.ws.onmessage = (event) => {
          this.handleMessage(event.data)
        }

        this.ws.onerror = (event) => {
          console.error('[CrawlerWebSocket] Error:', event)
          this.error.value = 'WebSocket connection error'

          // Notify error handlers
          this.errorHandlers.forEach(handler => handler(event))
        }

        this.ws.onclose = (event) => {
          console.log('[CrawlerWebSocket] Disconnected:', event.code, event.reason)
          this.connected.value = false
          this.connecting.value = false
          this.stopHeartbeat()

          // Notify disconnection handlers
          this.disconnectionHandlers.forEach(handler => handler())

          // Attempt to reconnect if not closed intentionally
          if (event.code !== 1000 && this.reconnectAttempts < this.maxReconnectAttempts) {
            this.scheduleReconnect()
          }
        }
      } catch (err: any) {
        this.connecting.value = false
        this.error.value = err.message
        reject(err)
      }
    })
  }

  /**
   * Disconnect from WebSocket server
   */
  disconnect(): void {
    if (this.reconnectTimeout) {
      clearTimeout(this.reconnectTimeout)
      this.reconnectTimeout = null
    }

    this.stopHeartbeat()

    if (this.ws) {
      this.ws.close(1000, 'Client disconnect')
      this.ws = null
    }

    this.connected.value = false
    this.connecting.value = false
  }

  /**
   * Send message to server
   */
  send(type: string, data: any): boolean {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      console.warn('[CrawlerWebSocket] Cannot send message: not connected')
      return false
    }

    try {
      const message = {
        type,
        data,
        timestamp: Date.now()
      }
      this.ws.send(JSON.stringify(message))
      return true
    } catch (err) {
      console.error('[CrawlerWebSocket] Send error:', err)
      return false
    }
  }

  /**
   * Subscribe to task updates
   */
  subscribeToTask(taskId: string): boolean {
    return this.send('subscribe_task', { taskId })
  }

  /**
   * Unsubscribe from task updates
   */
  unsubscribeFromTask(taskId: string): boolean {
    return this.send('unsubscribe_task', { taskId })
  }

  /**
   * Subscribe to all tasks
   */
  subscribeToAllTasks(): boolean {
    return this.send('subscribe_all_tasks', {})
  }

  /**
   * Subscribe to node status updates
   */
  subscribeToNodeStatus(): boolean {
    return this.send('subscribe_node_status', {})
  }

  /**
   * Subscribe to log stream
   */
  subscribeToLogs(level?: string): boolean {
    return this.send('subscribe_logs', { level })
  }

  /**
   * Add message handler
   */
  onMessage(handler: MessageHandler): () => void {
    this.messageHandlers.add(handler)
    return () => this.messageHandlers.delete(handler)
  }

  /**
   * Add connection handler
   */
  onConnect(handler: ConnectionHandler): () => void {
    this.connectionHandlers.add(handler)
    return () => this.connectionHandlers.delete(handler)
  }

  /**
   * Add disconnection handler
   */
  onDisconnect(handler: ConnectionHandler): () => void {
    this.disconnectionHandlers.add(handler)
    return () => this.disconnectionHandlers.delete(handler)
  }

  /**
   * Add error handler
   */
  onError(handler: ErrorHandler): () => void {
    this.errorHandlers.add(handler)
    return () => this.errorHandlers.delete(handler)
  }

  /**
   * Handle incoming message
   */
  private handleMessage(data: string): void {
    try {
      const message: WebSocketMessage = JSON.parse(data)

      // Handle heartbeat
      if (message.type === 'heartbeat') {
        this.send('heartbeat_response', {})
        return
      }

      // Notify all message handlers
      this.messageHandlers.forEach(handler => {
        try {
          handler(message)
        } catch (err) {
          console.error('[CrawlerWebSocket] Handler error:', err)
        }
      })
    } catch (err) {
      console.error('[CrawlerWebSocket] Message parse error:', err)
    }
  }

  /**
   * Schedule reconnection attempt
   */
  private scheduleReconnect(): void {
    if (this.reconnectTimeout) {
      clearTimeout(this.reconnectTimeout)
    }

    this.reconnectAttempts++
    const delay = this.reconnectDelay * Math.pow(2, this.reconnectAttempts - 1)

    console.log(`[CrawlerWebSocket] Scheduling reconnect in ${delay}ms (attempt ${this.reconnectAttempts})`)

    this.reconnectTimeout = window.setTimeout(() => {
      if (this.reconnectAttempts <= this.maxReconnectAttempts) {
        this.connect(this.url).catch(err => {
          console.error('[CrawlerWebSocket] Reconnect failed:', err)
        })
      }
    }, delay)

    // Show notification
    if (this.reconnectAttempts === 1) {
      ElMessage.warning('连接中断，正在尝试重新连接...')
    }
  }

  /**
   * Start heartbeat to keep connection alive
   */
  private startHeartbeat(): void {
    this.stopHeartbeat()

    this.heartbeatInterval = window.setInterval(() => {
      if (this.ws?.readyState === WebSocket.OPEN) {
        this.send('heartbeat', {})
      }
    }, 30000) // Send heartbeat every 30 seconds
  }

  /**
   * Stop heartbeat
   */
  private stopHeartbeat(): void {
    if (this.heartbeatInterval) {
      clearInterval(this.heartbeatInterval)
      this.heartbeatInterval = null
    }
  }

  /**
   * Get connection state
   */
  getState(): {
    connected: boolean
    connecting: boolean
    error: string | null
  } {
    return {
      connected: this.connected.value,
      connecting: this.connecting.value,
      error: this.error.value
    }
  }
}

// Create singleton instance
const crawlerWebSocketService = new CrawlerWebSocketService()

/**
 * Composable for using Crawler WebSocket in components
 */
export function useCrawlerWebSocket() {
  const {
    connected,
    connecting,
    error
  } = crawlerWebSocketService.getState()

  /**
   * Connect to crawler WebSocket
   */
  async function connect(baseURL: string = import.meta.env.VITE_WS_BASE_URL || 'ws://localhost:8080') {
    const wsUrl = `${baseURL}/api/crawler/ws`
    await crawlerWebSocketService.connect(wsUrl)
  }

  /**
   * Disconnect from WebSocket
   */
  function disconnect() {
    crawlerWebSocketService.disconnect()
  }

  /**
   * Subscribe to task updates
   */
  function subscribeToTask(taskId: string) {
    return crawlerWebSocketService.subscribeToTask(taskId)
  }

  /**
   * Subscribe to all tasks
   */
  function subscribeToAllTasks() {
    return crawlerWebSocketService.subscribeToAllTasks()
  }

  /**
   * Subscribe to node status
   */
  function subscribeToNodeStatus() {
    return crawlerWebSocketService.subscribeToNodeStatus()
  }

  /**
   * Subscribe to logs
   */
  function subscribeToLogs(level?: string) {
    return crawlerWebSocketService.subscribeToLogs(level)
  }

  /**
   * Listen to messages
   */
  function onMessage(handler: (message: WebSocketMessage) => void) {
    return crawlerWebSocketService.onMessage(handler)
  }

  /**
   * Listen to connection
   */
  function onConnect(handler: () => void) {
    return crawlerWebSocketService.onConnect(handler)
  }

  /**
   * Listen to disconnection
   */
  function onDisconnect(handler: () => void) {
    return crawlerWebSocketService.onDisconnect(handler)
  }

  /**
   * Listen to errors
   */
  function onError(handler: (error: Event) => void) {
    return crawlerWebSocketService.onError(handler)
  }

  return {
    // State
    connected,
    connecting,
    error,

    // Methods
    connect,
    disconnect,
    subscribeToTask,
    subscribeToAllTasks,
    subscribeToNodeStatus,
    subscribeToLogs,
    onMessage,
    onConnect,
    onDisconnect,
    onError
  }
}

export default crawlerWebSocketService
