/**
 * WebSocket 实时同步管理器
 * 提供自动重连、心跳检测、消息队列等功能
 */

import { ref, reactive, onUnmounted, watch, type Ref } from 'vue';
import {
  ConnectionState,
  type WSMessageUnion,
  type WebSocketConfig,
  type WebSocketHandlers,
  type ConnectionStats,
  type QueuedMessage,
  MessageType
} from '../types/websocket';

// 默认配置
const DEFAULT_CONFIG: WebSocketConfig = {
  url: import.meta.env.VITE_WS_URL || 'ws://localhost:8087/ws',
  reconnectInterval: 3000,
  maxReconnectAttempts: 10,
  heartbeatInterval: 30000,
  messageQueueSize: 100
};

export function useWebSocket(
  userConfig: Partial<WebSocketConfig> = {},
  userHandlers: Partial<WebSocketHandlers> = {}
) {
  // 合并配置
  const config = { ...DEFAULT_CONFIG, ...userConfig };

  // 响应式状态
  const connectionState = ref<ConnectionState>(ConnectionState.DISCONNECTED);
  const ws = ref<WebSocket | null>(null);
  const reconnectAttempts = ref(0);
  const reconnectTimer = ref<number | null>(null);
  const heartbeatTimer = ref<number | null>(null);
  const messageQueue = ref<QueuedMessage[]>([]);

  // 连接统计
  const stats = reactive<ConnectionStats>({
    connectedAt: null,
    messagesReceived: 0,
    messagesSent: 0,
    reconnectCount: 0,
    lastMessageAt: null,
    lastHeartbeatAt: null
  });

  // 事件处理器
  const handlers: WebSocketHandlers = {
    onMessage: userHandlers.onMessage || (() => {}),
    onStateChange: userHandlers.onStateChange || (() => {}),
    onError: userHandlers.onError || (() => {}),
    onPaperUpdate: userHandlers.onPaperUpdate,
    onPaperDelete: userHandlers.onPaperDelete,
    onPaperNew: userHandlers.onPaperNew,
    onStatsUpdate: userHandlers.onStatsUpdate
  };

  // 更新连接状态
  const setState = (newState: ConnectionState) => {
    if (connectionState.value !== newState) {
      connectionState.value = newState;
      handlers.onStateChange(newState);
    }
  };

  // 处理收到的消息
  const handleMessage = (event: MessageEvent) => {
    try {
      const message: WSMessageUnion = JSON.parse(event.data);
      stats.messagesReceived++;
      stats.lastMessageAt = new Date();

      // 处理心跳
      if (message.type === MessageType.HEARTBEAT) {
        stats.lastHeartbeatAt = new Date();
        return; // 不传递心跳消息给处理器
      }

      // 调用通用处理器
      handlers.onMessage(message);

      // 调用特定类型的处理器
      switch (message.type) {
        case MessageType.PAPER_UPDATE:
          handlers.onPaperUpdate?.(message.data.paper);
          break;
        case MessageType.PAPER_DELETE:
          handlers.onPaperDelete?.(message.data.paper_id);
          break;
        case MessageType.PAPER_NEW:
          handlers.onPaperNew?.(message.data.paper);
          break;
        case MessageType.STATS_UPDATE:
          handlers.onStatsUpdate?.(message.data);
          break;
        case MessageType.ERROR:
          console.error('[WebSocket] Server error:', message.data);
          break;
      }
    } catch (error) {
      console.error('[WebSocket] Failed to parse message:', error);
    }
  };

  // 处理连接打开
  const handleOpen = () => {
    setState(ConnectionState.CONNECTED);
    reconnectAttempts.value = 0;
    stats.connectedAt = new Date();

    // 发送队列中的消息
    flushMessageQueue();

    // 启动心跳
    startHeartbeat();

    console.log('[WebSocket] Connected to', config.url);
  };

  // 处理连接关闭
  const handleClose = (event: CloseEvent) => {
    stopHeartbeat();

    if (reconnectAttempts.value < config.maxReconnectAttempts) {
      setState(ConnectionState.RECONNECTING);
      scheduleReconnect();
    } else {
      setState(ConnectionState.DISCONNECTED);
      console.error('[WebSocket] Max reconnect attempts reached');
    }
  };

  // 处理错误
  const handleError = (error: Event) => {
    console.error('[WebSocket] Error:', error);
    setState(ConnectionState.ERROR);
    handlers.onError(error);
  };

  // 安排重连
  const scheduleReconnect = () => {
    if (reconnectTimer.value !== null) {
      return;
    }

    reconnectAttempts.value++;
    stats.reconnectCount++;

    console.log(`[WebSocket] Reconnecting... Attempt ${reconnectAttempts.value}`);

    reconnectTimer.value = window.setTimeout(() => {
      reconnectTimer.value = null;
      connect();
    }, config.reconnectInterval);
  };

  // 启动心跳
  const startHeartbeat = () => {
    stopHeartbeat();

    heartbeatTimer.value = window.setInterval(() => {
      send({
        type: MessageType.HEARTBEAT,
        timestamp: new Date().toISOString(),
        id: generateMessageId()
      });
    }, config.heartbeatInterval);
  };

  // 停止心跳
  const stopHeartbeat = () => {
    if (heartbeatTimer.value !== null) {
      clearInterval(heartbeatTimer.value);
      heartbeatTimer.value = null;
    }
  };

  // 生成消息ID
  const generateMessageId = (): string => {
    return `${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
  };

  // 发送消息
  const send = (message: any): boolean => {
    if (ws.value?.readyState !== WebSocket.OPEN) {
      // 添加到队列
      if (messageQueue.value.length < config.messageQueueSize) {
        messageQueue.value.push({
          message,
          timestamp: Date.now(),
          retryCount: 0
        });
      } else {
        console.warn('[WebSocket] Message queue full, discarding message');
      }
      return false;
    }

    try {
      ws.value.send(JSON.stringify(message));
      stats.messagesSent++;
      return true;
    } catch (error) {
      console.error('[WebSocket] Failed to send message:', error);
      return false;
    }
  };

  // 清空消息队列
  const flushMessageQueue = () => {
    while (messageQueue.value.length > 0 && ws.value?.readyState === WebSocket.OPEN) {
      const queued = messageQueue.value.shift();
      if (queued) {
        try {
          ws.value.send(JSON.stringify(queued.message));
          stats.messagesSent++;
        } catch (error) {
          console.error('[WebSocket] Failed to send queued message:', error);
          // 重新入队
          if (queued.retryCount < 3) {
            queued.retryCount++;
            messageQueue.value.unshift(queued);
          }
        }
      }
    }
  };

  // 连接WebSocket
  const connect = () => {
    if (ws.value?.readyState === WebSocket.OPEN ||
        ws.value?.readyState === WebSocket.CONNECTING) {
      return;
    }

    setState(ConnectionState.CONNECTING);

    try {
      ws.value = new WebSocket(config.url);

      ws.value.onopen = handleOpen;
      ws.value.onclose = handleClose;
      ws.value.onerror = handleError;
      ws.value.onmessage = handleMessage;
    } catch (error) {
      console.error('[WebSocket] Failed to create connection:', error);
      setState(ConnectionState.ERROR);
      scheduleReconnect();
    }
  };

  // 断开连接
  const disconnect = () => {
    stopHeartbeat();

    if (reconnectTimer.value !== null) {
      clearTimeout(reconnectTimer.value);
      reconnectTimer.value = null;
    }

    if (ws.value) {
      ws.value.close(1000, 'Client disconnect');
      ws.value = null;
    }

    setState(ConnectionState.DISCONNECTED);
    messageQueue.value = [];
  };

  // 手动重连
  const reconnect = () => {
    disconnect();
    reconnectAttempts.value = 0;
    connect();
  };

  // 清理
  onUnmounted(() => {
    disconnect();
  });

  return {
    // 状态
    connectionState,
    stats,
    isConnected: () => connectionState.value === ConnectionState.CONNECTED,

    // 方法
    connect,
    disconnect,
    reconnect,
    send,

    // 工具方法
    updateConfig: (newConfig: Partial<WebSocketConfig>) => {
      Object.assign(config, newConfig);
      if (newConfig.heartbeatInterval) {
        startHeartbeat();
      }
    }
  };
}

// 导出类型
export type { ConnectionState, ConnectionStats };
