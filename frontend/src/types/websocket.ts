/**
 * WebSocket 实时同步协议类型定义
 */

// 消息类型枚举
export enum MessageType {
  PAPER_UPDATE = 'paper_update',      // 论文更新
  PAPER_DELETE = 'paper_delete',      // 论文删除
  PAPER_NEW = 'paper_new',            // 新论文
  STATS_UPDATE = 'stats_update',      // 统计数据更新
  HEARTBEAT = 'heartbeat',            // 心跳
  SYNC_COMPLETE = 'sync_complete',    // 同步完成
  ERROR = 'error'                     // 错误消息
}

// 连接状态枚举
export enum ConnectionState {
  CONNECTING = 'connecting',
  CONNECTED = 'connected',
  DISCONNECTED = 'disconnected',
  RECONNECTING = 'reconnecting',
  ERROR = 'error'
}

// 论文数据接口（简化版本，实际应该与主类型保持一致）
export interface PaperData {
  id: number;
  title: string;
  authors: string;
  year: number;
  venue?: string;
  abstract?: string;
  keywords?: string[];
  pdf_url?: string;
  citation_count?: number;
  created_at?: string;
  updated_at?: string;
}

// 统计数据接口
export interface StatsData {
  total_papers: number;
  total_citations: number;
  papers_by_year: Record<number, number>;
  top_venues: Array<{ venue: string; count: number }>;
  recent_additions: number;
  last_updated: string;
}

// WebSocket消息基础接口
export interface WSMessage {
  type: MessageType;
  timestamp: string;
  id: string;
}

// 论文更新消息
export interface PaperUpdateMessage extends WSMessage {
  type: MessageType.PAPER_UPDATE;
  data: {
    paper: PaperData;
    changes: string[];  // 变更的字段列表
  };
}

// 论文删除消息
export interface PaperDeleteMessage extends WSMessage {
  type: MessageType.PAPER_DELETE;
  data: {
    paper_id: number;
  };
}

// 新论文消息
export interface PaperNewMessage extends WSMessage {
  type: MessageType.PAPER_NEW;
  data: {
    paper: PaperData;
  };
}

// 统计更新消息
export interface StatsUpdateMessage extends WSMessage {
  type: MessageType.STATS_UPDATE;
  data: StatsData;
}

// 心跳消息
export interface HeartbeatMessage extends WSMessage {
  type: MessageType.HEARTBEAT;
  data: {
    server_time: string;
  };
}

// 同步完成消息
export interface SyncCompleteMessage extends WSMessage {
  type: MessageType.SYNC_COMPLETE;
  data: {
    papers_count: number;
    duration_ms: number;
  };
}

// 错误消息
export interface ErrorMessage extends WSMessage {
  type: MessageType.ERROR;
  data: {
    code: string;
    message: string;
    details?: any;
  };
}

// 联合类型
export type WSMessageUnion =
  | PaperUpdateMessage
  | PaperDeleteMessage
  | PaperNewMessage
  | StatsUpdateMessage
  | HeartbeatMessage
  | SyncCompleteMessage
  | ErrorMessage;

// WebSocket配置接口
export interface WebSocketConfig {
  url: string;
  reconnectInterval: number;
  maxReconnectAttempts: number;
  heartbeatInterval: number;
  messageQueueSize: number;
}

// WebSocket事件处理器
export interface WebSocketHandlers {
  onMessage: (message: WSMessageUnion) => void;
  onStateChange: (state: ConnectionState) => void;
  onError: (error: Event) => void;
  onPaperUpdate?: (paper: PaperData) => void;
  onPaperDelete?: (paperId: number) => void;
  onPaperNew?: (paper: PaperData) => void;
  onStatsUpdate?: (stats: StatsData) => void;
}

// 连接统计信息
export interface ConnectionStats {
  connectedAt: Date | null;
  messagesReceived: number;
  messagesSent: number;
  reconnectCount: number;
  lastMessageAt: Date | null;
  lastHeartbeatAt: Date | null;
}

// 消息队列项
export interface QueuedMessage {
  message: any;
  timestamp: number;
  retryCount: number;
}
