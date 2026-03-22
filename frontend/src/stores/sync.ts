/**
 * PaperCrawler Frontend Sync Store
 * Manages real-time synchronization with backend
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { io, Socket } from 'socket.io-client'
import type { Paper, SearchStats } from '@/types'

interface SyncState {
  connected: boolean
  connecting: boolean
  lastSyncTime: number
  lastSyncVersion: number
  pendingChanges: number
  conflicts: ConflictData[]
}

interface ConflictData {
  id: string
  localData: any
  remoteData: any
  timestamp: number
}

interface SyncMessage {
  type: 'paper_create' | 'paper_update' | 'paper_delete' | 'stats_update'
  data: any
  version: number
  timestamp: number
}

const SYNC_CONFIG = {
  url: import.meta.env.VITE_SYNC_URL || 'wss://api.papercrawler.com/sync',
  reconnection: true,
  reconnectionDelay: 1000,
  reconnectionAttempts: 10,
  reconnectionDelayMax: 5000,
  timeout: 10000,
}

export const useSyncStore = defineStore('sync', () => {
  // State
  const socket = ref<Socket | null>(null)
  const state = ref<SyncState>({
    connected: false,
    connecting: false,
    lastSyncTime: 0,
    lastSyncVersion: 0,
    pendingChanges: 0,
    conflicts: [],
  })

  // Computed
  const isConnected = computed(() => state.value.connected)
  const hasConflicts = computed(() => state.value.conflicts.length > 0)
  const syncStatus = computed(() => {
    if (state.value.connecting) return 'connecting'
    if (state.value.connected) return 'connected'
    return 'disconnected'
  })

  /**
   * Initialize WebSocket connection
   */
  function connect() {
    if (socket.value?.connected) {
      console.log('[Sync] Already connected')
      return
    }

    state.value.connecting = true
    console.log('[Sync] Connecting to', SYNC_CONFIG.url)

    socket.value = io(SYNC_CONFIG.url, {
      reconnection: SYNC_CONFIG.reconnection,
      reconnectionDelay: SYNC_CONFIG.reconnectionDelay,
      reconnectionAttempts: SYNC_CONFIG.reconnectionAttempts,
      reconnectionDelayMax: SYNC_CONFIG.reconnectionDelayMax,
      timeout: SYNC_CONFIG.timeout,
      transports: ['websocket', 'polling'],
    })

    // Connection established
    socket.value.on('connect', () => {
      console.log('[Sync] Connected', socket.value?.id)
      state.value.connected = true
      state.value.connecting = false

      // Subscribe to channels
      subscribeToChannels()

      // Request initial sync
      requestInitialSync()
    })

    // Connection closed
    socket.value.on('disconnect', (reason) => {
      console.log('[Sync] Disconnected:', reason)
      state.value.connected = false
      state.value.connecting = false
    })

    // Connection error
    socket.value.on('connect_error', (error) => {
      console.error('[Sync] Connection error:', error)
      state.value.connecting = false
    })

    // Paper updates
    socket.value.on('paper_update', handlePaperUpdate)
    socket.value.on('paper_create', handlePaperCreate)
    socket.value.on('paper_delete', handlePaperDelete)

    // Statistics updates
    socket.value.on('stats_update', handleStatsUpdate)

    // Conflict detected
    socket.value.on('conflict_detected', handleConflictDetected)

    // Sync progress
    socket.value.on('sync_progress', handleSyncProgress)
  }

  /**
   * Disconnect from sync server
   */
  function disconnect() {
    if (socket.value) {
      socket.value.disconnect()
      socket.value = null
      state.value.connected = false
      console.log('[Sync] Disconnected')
    }
  }

  /**
   * Subscribe to data channels
   */
  function subscribeToChannels() {
    socket.value?.emit('subscribe', {
      channels: ['papers', 'stats'],
      lastVersion: state.value.lastSyncVersion,
    })
  }

  /**
   * Request initial sync after connection
   */
  function requestInitialSync() {
    socket.value?.emit('sync_request', {
      lastSyncVersion: state.value.lastSyncVersion,
      deviceId: getDeviceId(),
      clientVersion: '2.0.0',
    })
  }

  /**
   * Handle paper update event
   */
  function handlePaperUpdate(data: Paper) {
    console.log('[Sync] Paper updated:', data.id)

    // Update local cache
    const cache = useCacheStore()
    cache.updatePaper(data.id, data)

    // Notify UI components
    const eventBus = useEventBus()
    eventBus.emit('paper:updated', data)

    // Update last sync version
    state.value.lastSyncVersion = data.version || state.value.lastSyncVersion + 1
    state.value.lastSyncTime = Date.now()
  }

  /**
   * Handle paper create event
   */
  function handlePaperCreate(data: Paper) {
    console.log('[Sync] Paper created:', data.id)

    const cache = useCacheStore()
    cache.addPaper(data)

    const eventBus = useEventBus()
    eventBus.emit('paper:created', data)

    state.value.lastSyncVersion = data.version || state.value.lastSyncVersion + 1
    state.value.lastSyncTime = Date.now()
  }

  /**
   * Handle paper delete event
   */
  function handlePaperDelete(data: { id: number }) {
    console.log('[Sync] Paper deleted:', data.id)

    const cache = useCacheStore()
    cache.removePaper(data.id)

    const eventBus = useEventBus()
    eventBus.emit('paper:deleted', data.id)

    state.value.lastSyncVersion += 1
    state.value.lastSyncTime = Date.now()
  }

  /**
   * Handle statistics update event
   */
  function handleStatsUpdate(data: SearchStats) {
    console.log('[Sync] Statistics updated')

    const cache = useCacheStore()
    cache.updateStats(data)

    const eventBus = useEventBus()
    eventBus.emit('stats:updated', data)
  }

  /**
   * Handle conflict detected event
   */
  function handleConflictDetected(data: ConflictData) {
    console.warn('[Sync] Conflict detected:', data.id)

    state.value.conflicts.push(data)

    const eventBus = useEventBus()
    eventBus.emit('conflict:detected', data)
  }

  /**
   * Handle sync progress event
   */
  function handleSyncProgress(data: { received: number; total: number }) {
    console.log('[Sync] Progress:', data.received, '/', data.total)
  }

  /**
   * Push local changes to server
   */
  async function pushChanges() {
    if (!state.value.connected) {
      console.warn('[Sync] Cannot push changes: not connected')
      return
    }

    if (state.value.pendingChanges === 0) {
      console.log('[Sync] No changes to push')
      return
    }

    try {
      const changes = await getLocalChanges()

      socket.value?.emit('push_changes', {
        changes,
        device_id: getDeviceId(),
        client_version: state.value.lastSyncVersion,
      })

      state.value.pendingChanges = 0
      console.log('[Sync] Pushed changes:', changes.length)
    } catch (error) {
      console.error('[Sync] Push failed:', error)
    }
  }

  /**
   * Resolve a conflict
   */
  function resolveConflict(
    conflictId: string,
    strategy: 'local' | 'remote' | 'merge'
  ) {
    const conflict = state.value.conflicts.find((c) => c.id === conflictId)
    if (!conflict) {
      console.warn('[Sync] Conflict not found:', conflictId)
      return
    }

    socket.value?.emit('resolve_conflict', {
      entity_id: conflictId,
      strategy,
      local_data: conflict.localData,
      remote_data: conflict.remoteData,
    })

    // Remove from conflicts
    state.value.conflicts = state.value.conflicts.filter(
      (c) => c.id !== conflictId
    )
  }

  /**
   * Get sync statistics
   */
  function getSyncStats() {
    return {
      connected: state.value.connected,
      lastSyncTime: state.value.lastSyncTime,
      lastSyncVersion: state.value.lastSyncVersion,
      pendingChanges: state.value.pendingChanges,
      conflicts: state.value.conflicts.length,
    }
  }

  return {
    // State
    state,
    socket,

    // Computed
    isConnected,
    hasConflicts,
    syncStatus,

    // Actions
    connect,
    disconnect,
    pushChanges,
    resolveConflict,
    getSyncStats,
  }
})

/**
 * Get or generate device ID
 */
function getDeviceId(): string {
  let deviceId = localStorage.getItem('papercrawler_device_id')
  if (!deviceId) {
    deviceId = `web_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`
    localStorage.setItem('papercrawler_device_id', deviceId)
  }
  return deviceId
}

/**
 * Get local changes from IndexedDB
 */
async function getLocalChanges() {
  // TODO: Implement IndexedDB query for local changes
  return []
}

// Import dependencies (to be implemented)
function useCacheStore() {
  return {
    updatePaper: (id: number, data: any) => {},
    addPaper: (data: any) => {},
    removePaper: (id: number) => {},
    updateStats: (data: any) => {},
  }
}

function useEventBus() {
  return {
    emit: (event: string, data: any) => {},
  }
}
