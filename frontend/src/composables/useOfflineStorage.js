/**
 * PaperCrawler - Offline Storage Composable for Vue 3
 *
 * Provides offline-first data persistence using IndexedDB:
 * - Store papers for offline access
 * - Queue operations for sync when online
 * - Manage download state
 * - Track synchronization status
 */

import { ref, computed, watch, onMounted, onUnmounted } from 'vue'
import { useCache } from './useCache'

// ============================================================================
// IndexedDB Database Manager
// ============================================================================

const DB_NAME = 'PaperCrawlerOffline'
const DB_VERSION = 1

const STORES = {
  papers: 'papers',
  journals: 'journals',
  searchHistory: 'searchHistory',
  syncQueue: 'syncQueue',
  downloads: 'downloads'
}

class OfflineDatabase {
  constructor() {
    this.db = null
  }

  async init() {
    return new Promise((resolve, reject) => {
      const request = indexedDB.open(DB_NAME, DB_VERSION)

      request.onerror = () => reject(request.error)
      request.onsuccess = () => {
        this.db = request.result
        resolve()
      }

      request.onupgradeneeded = (event) => {
        const db = event.target.result

        // Papers store
        if (!db.objectStoreNames.contains(STORES.papers)) {
          const paperStore = db.createObjectStore(STORES.papers, { keyPath: 'id' })
          paperStore.createIndex('serverId', 'server_id', { unique: false })
          paperStore.createIndex('title', 'title', { unique: false })
          paperStore.createIndex('year', 'year', { unique: false })
          paperStore.createIndex('level', 'level', { unique: false })
          paperStore.createIndex('syncStatus', 'sync_status', { unique: false })
          paperStore.createIndex('isBookmarked', 'is_bookmarked', { unique: false })
        }

        // Journals store
        if (!db.objectStoreNames.contains(STORES.journals)) {
          const journalStore = db.createObjectStore(STORES.journals, { keyPath: 'id' })
          journalStore.createIndex('serverId', 'server_id', { unique: false })
          journalStore.createIndex('name', 'name', { unique: false })
        }

        // Search history store
        if (!db.objectStoreNames.contains(STORES.searchHistory)) {
          const historyStore = db.createObjectStore(STORES.searchHistory, {
            keyPath: 'id',
            autoIncrement: true
          })
          historyStore.createIndex('keyword', 'keyword', { unique: false })
          historyStore.createIndex('createdAt', 'created_at', { unique: false })
        }

        // Sync queue store
        if (!db.objectStoreNames.contains(STORES.syncQueue)) {
          const syncStore = db.createObjectStore(STORES.syncQueue, {
            keyPath: 'id',
            autoIncrement: true
          })
          syncStore.createIndex('operation', 'operation', { unique: false })
          syncStore.createIndex('createdAt', 'created_at', { unique: false })
        }

        // Downloads store
        if (!db.objectStoreNames.contains(STORES.downloads)) {
          const downloadStore = db.createObjectStore(STORES.downloads, {
            keyPath: 'paperId'
          })
          downloadStore.createIndex('downloadedAt', 'downloaded_at', { unique: false })
        }
      }
    })
  }

  // Generic CRUD operations
  async getAll(storeName) {
    if (!this.db) await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction([storeName], 'readonly')
      const store = transaction.objectStore(storeName)
      const request = store.getAll()

      request.onsuccess = () => resolve(request.result)
      request.onerror = () => reject(request.error)
    })
  }

  async get(storeName, key) {
    if (!this.db) await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction([storeName], 'readonly')
      const store = transaction.objectStore(storeName)
      const request = store.get(key)

      request.onsuccess = () => resolve(request.result)
      request.onerror = () => reject(request.error)
    })
  }

  async put(storeName, data) {
    if (!this.db) await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction([storeName], 'readwrite')
      const store = transaction.objectStore(storeName)
      const request = store.put(data)

      request.onsuccess = () => resolve(request.result)
      request.onerror = () => reject(request.error)
    })
  }

  async delete(storeName, key) {
    if (!this.db) await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction([storeName], 'readwrite')
      const store = transaction.objectStore(storeName)
      const request = store.delete(key)

      request.onsuccess = () => resolve()
      request.onerror = () => reject(request.error)
    })
  }

  async clear(storeName) {
    if (!this.db) await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction([storeName], 'readwrite')
      const store = transaction.objectStore(storeName)
      const request = store.clear()

      request.onsuccess = () => resolve()
      request.onerror = () => reject(request.error)
    })
  }

  // Indexed queries
  async getIndex(storeName, indexName, value) {
    if (!this.db) await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction([storeName], 'readonly')
      const store = transaction.objectStore(storeName)
      const index = store.index(indexName)
      const request = index.getAll(value)

      request.onsuccess = () => resolve(request.result)
      request.onerror = () => reject(request.error)
    })
  }

  // Count operations
  async count(storeName) {
    if (!this.db) await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db.transaction([storeName], 'readonly')
      const store = transaction.objectStore(storeName)
      const request = store.count()

      request.onsuccess = () => resolve(request.result)
      request.onerror = () => reject(request.error)
    })
  }
}

// ============================================================================
// Main Composable
// ============================================================================

const db = new OfflineDatabase()

export function useOfflineStorage() {
  const isReady = ref(false)
  const isOnline = ref(navigator.onLine)
  const syncStatus = ref('idle')  // idle, syncing, error
  const lastSyncTime = ref(null)
  const pendingSyncCount = ref(0)

  // Initialize database
  onMounted(async () => {
    try {
      await db.init()
      isReady.value = true

      // Load last sync time from localStorage
      const saved = localStorage.getItem('lastSyncTime')
      if (saved) {
        lastSyncTime.value = new Date(saved)
      }

      // Listen for online/offline events
      window.addEventListener('online', handleOnline)
      window.addEventListener('offline', handleOffline)

      // Update pending sync count
      await updatePendingSyncCount()

    } catch (error) {
      console.error('Failed to initialize offline storage:', error)
    }
  })

  // Cleanup event listeners on unmount
  onUnmounted(() => {
    window.removeEventListener('online', handleOnline)
    window.removeEventListener('offline', handleOffline)
  })

  // Event handlers
  function handleOnline() {
    isOnline.value = true
    console.log('Device is online')
    // Trigger sync
    sync()
  }

  function handleOffline() {
    isOnline.value = false
    console.log('Device is offline')
  }

  async function updatePendingSyncCount() {
    try {
      const queue = await db.getAll(STORES.syncQueue)
      pendingSyncCount.value = queue.length
    } catch (error) {
      console.error('Failed to update pending sync count:', error)
    }
  }

  // ========== Paper Operations ==========

  async function savePaper(paper) {
    const paperData = {
      id: paper.id,
      server_id: paper.serverId || paper.id,
      title: paper.title,
      authors: paper.authors || paper.author,
      year: paper.year,
      journal_full: paper.journalFull || paper.journal,
      journal_short: paper.journalShort,
      level: paper.level,
      doi_url: paper.doiUrl,
      journal_url: paper.journalUrl,
      type: paper.type,
      abstract: paper.abstract,
      keywords: paper.keywords,
      is_bookmarked: paper.isBookmarked || false,
      is_read: paper.isRead || false,
      user_notes: paper.userNotes || '',
      user_rating: paper.userRating || 0,
      sync_status: 'synced',
      sync_version: 1,
      created_at: paper.createdAt || Date.now(),
      updated_at: paper.updatedAt || Date.now()
    }

    await db.put(STORES.papers, paperData)

    // Invalidate cache
    const cache = useCache()
    await cache.invalidate(`pc:paper:${paper.id}`)
  }

  async function getPapers(filters = {}) {
    let papers = await db.getAll(STORES.papers)

    // Apply filters
    if (filters.year) {
      papers = papers.filter(p => p.year === filters.year)
    }

    if (filters.level) {
      papers = papers.filter(p => p.level === filters.level)
    }

    if (filters.bookmarked !== undefined) {
      papers = papers.filter(p => p.is_bookmarked === filters.bookmarked)
    }

    if (filters.search) {
      const searchLower = filters.search.toLowerCase()
      papers = papers.filter(p =>
        p.title.toLowerCase().includes(searchLower) ||
        p.authors.toLowerCase().includes(searchLower)
      )
    }

    // Sort
    if (filters.sortBy === 'year') {
      papers.sort((a, b) => b.year - a.year)
    } else if (filters.sortBy === 'title') {
      papers.sort((a, b) => a.title.localeCompare(b.title))
    }

    // Pagination
    const offset = filters.offset || 0
    const limit = filters.limit || 20

    return {
      papers: papers.slice(offset, offset + limit),
      total: papers.length
    }
  }

  async function getPaper(id) {
    return db.get(STORES.papers, id)
  }

  async function deletePaper(id) {
    await db.delete(STORES.papers, id)

    // Add to sync queue
    await addToSyncQueue({
      operation: 'delete',
      tableName: 'papers',
      data: { id }
    })
  }

  async function updatePaper(id, updates) {
    const paper = await getPaper(id)
    if (!paper) return null

    const updated = {
      ...paper,
      ...updates,
      updated_at: Date.now(),
      sync_status: 'pending'
    }

    await db.put(STORES.papers, updated)

    // Add to sync queue
    await addToSyncQueue({
      operation: 'update',
      tableName: 'papers',
      data: updated
    })

    return updated
  }

  // ========== Journal Operations ==========

  async function saveJournal(journal) {
    const journalData = {
      id: journal.id,
      server_id: journal.serverId || journal.id,
      name: journal.name,
      name_short: journal.nameShort,
      full_name: journal.fullName,
      publisher: journal.publisher,
      level: journal.level,
      issn: journal.issn,
      impact_factor: journal.impactFactor,
      h_index: journal.hIndex,
      sync_status: 'synced',
      created_at: Date.now(),
      updated_at: Date.now()
    }

    await db.put(STORES.journals, journalData)
  }

  async function getJournals() {
    return db.getAll(STORES.journals)
  }

  async function getJournal(id) {
    return db.get(STORES.journals, id)
  }

  // ========== Search History ==========

  async function recordSearch(keyword, searchType = 'paper', resultCount = 0, durationMs = 0) {
    const entry = {
      keyword,
      search_type: searchType,
      result_count: resultCount,
      search_duration_ms: durationMs,
      created_at: Date.now()
    }

    await db.put(STORES.searchHistory, entry)
  }

  async function getRecentSearches(limit = 10) {
    const all = await db.getAll(STORES.searchHistory)

    return all
      .sort((a, b) => b.created_at - a.created_at)
      .slice(0, limit)
      .map(s => s.keyword)
  }

  async function getPopularSearches(limit = 10) {
    const all = await db.getAll(STORES.searchHistory)

    const counts = {}
    all.forEach(s => {
      counts[s.keyword] = (counts[s.keyword] || 0) + 1
    })

    return Object.entries(counts)
      .sort((a, b) => b[1] - a[1])
      .slice(0, limit)
      .map(([keyword, count]) => ({ keyword, count }))
  }

  // ========== Sync Queue ==========

  async function addToSyncQueue(item) {
    const queueItem = {
      operation: item.operation,
      table_name: item.tableName,
      data: item.data,
      created_at: Date.now(),
      retry_count: 0
    }

    await db.put(STORES.syncQueue, queueItem)
    await updatePendingSyncCount()
  }

  async function getSyncQueue() {
    return db.getAll(STORES.syncQueue)
  }

  async function clearSyncQueue() {
    await db.clear(STORES.syncQueue)
    await updatePendingSyncCount()
  }

  async function removeFromSyncQueue(id) {
    await db.delete(STORES.syncQueue, id)
    await updatePendingSyncCount()
  }

  // ========== Synchronization ==========

  async function sync() {
    if (!isOnline.value) {
      console.log('Cannot sync while offline')
      return false
    }

    if (syncStatus.value === 'syncing') {
      console.log('Sync already in progress')
      return false
    }

    syncStatus.value = 'syncing'

    try {
      // Get pending sync items
      const queue = await getSyncQueue()

      if (queue.length === 0) {
        console.log('No pending sync items')
        syncStatus.value = 'idle'
        return true
      }

      console.log(`Syncing ${queue.length} items...`)

      // Process sync queue
      for (const item of queue) {
        try {
          await processSyncItem(item)
          await removeFromSyncQueue(item.id)
        } catch (error) {
          console.error('Failed to sync item:', error)

          // Increment retry count
          item.retry_count++
          if (item.retry_count < 3) {
            await db.put(STORES.syncQueue, item)
          } else {
            // Max retries reached, remove from queue
            await removeFromSyncQueue(item.id)
          }
        }
      }

      // Fetch updates from server
      await fetchServerUpdates()

      // Update last sync time
      lastSyncTime.value = new Date()
      localStorage.setItem('lastSyncTime', lastSyncTime.value.toISOString())

      syncStatus.value = 'idle'
      console.log('Sync completed')
      return true

    } catch (error) {
      console.error('Sync failed:', error)
      syncStatus.value = 'error'
      return false
    }
  }

  async function processSyncItem(item) {
    // This would make API calls to sync with server
    // For now, just simulate the operation
    console.log('Processing sync item:', item.operation, item.table_name)

    // In production:
    // switch (item.operation) {
    //   case 'insert':
    //     await api.post('/papers', item.data)
    //     break
    //   case 'update':
    //     await api.put(`/papers/${item.data.id}`, item.data)
    //     break
    //   case 'delete':
    //     await api.delete(`/papers/${item.data.id}`)
    //     break
    // }
  }

  async function fetchServerUpdates() {
    // Fetch updates from server since last sync
    const lastSync = lastSyncTime.value || new Date(0)
    const since = Math.floor(lastSync.getTime() / 1000)

    // In production:
    // const updates = await api.get(`/sync/updates?since=${since}`)
    // for (const paper of updates.papers) {
    //   await savePaper(paper)
    // }

    console.log('Fetching server updates since:', since)
  }

  // ========== Downloads ==========

  async function markAsDownloaded(paperId, filePath = null) {
    await db.put(STORES.downloads, {
      paperId,
      file_path: filePath,
      downloaded_at: Date.now()
    })
  }

  async function getDownloadedPapers() {
    const downloads = await db.getAll(STORES.downloads)
    const papers = []

    for (const download of downloads) {
      const paper = await getPaper(download.paper_id)
      if (paper) {
        papers.push({ ...paper, filePath: download.file_path })
      }
    }

    return papers
  }

  async function isDownloaded(paperId) {
    const download = await db.get(STORES.downloads, paperId)
    return !!download
  }

  // ========== Statistics ==========

  async function getStats() {
    const paperCount = await db.count(STORES.papers)
    const journalCount = await db.count(STORES.journals)
    const searchCount = await db.count(STORES.searchHistory)

    const papers = await db.getAll(STORES.papers)
    const bookmarked = papers.filter(p => p.is_bookmarked).length

    return {
      totalPapers: paperCount,
      totalJournals: journalCount,
      totalSearches: searchCount,
      bookmarkedPapers: bookmarked,
      lastSyncTime: lastSyncTime.value,
      pendingSyncCount: pendingSyncCount.value
    }
  }

  // ========== Storage Management ==========

  async function clearAll() {
    await db.clear(STORES.papers)
    await db.clear(STORES.journals)
    await db.clear(STORES.searchHistory)
    await db.clear(STORES.syncQueue)
    await db.clear(STORES.downloads)

    // Clear cache
    const cache = useCache()
    await cache.clear()

    await updatePendingSyncCount()
  }

  async function exportData() {
    const data = {
      papers: await db.getAll(STORES.papers),
      journals: await db.getAll(STORES.journals),
      searchHistory: await db.getAll(STORES.searchHistory),
      exportedAt: new Date().toISOString()
    }

    return JSON.stringify(data, null, 2)
  }

  async function importData(jsonString) {
    try {
      const data = JSON.parse(jsonString)

      if (data.papers) {
        for (const paper of data.papers) {
          await db.put(STORES.papers, paper)
        }
      }

      if (data.journals) {
        for (const journal of data.journals) {
          await db.put(STORES.journals, journal)
        }
      }

      if (data.searchHistory) {
        for (const entry of data.searchHistory) {
          await db.put(STORES.searchHistory, entry)
        }
      }

      console.log('Data imported successfully')
      return true

    } catch (error) {
      console.error('Import failed:', error)
      return false
    }
  }

  // ========== Computed Properties ==========

  const canSync = computed(() => isOnline.value && syncStatus.value !== 'syncing')

  return {
    // State
    isReady,
    isOnline,
    syncStatus,
    lastSyncTime,
    pendingSyncCount,
    canSync,

    // Paper operations
    savePaper,
    getPapers,
    getPaper,
    deletePaper,
    updatePaper,

    // Journal operations
    saveJournal,
    getJournals,
    getJournal,

    // Search history
    recordSearch,
    getRecentSearches,
    getPopularSearches,

    // Sync
    sync,
    getSyncQueue,
    clearSyncQueue,

    // Downloads
    markAsDownloaded,
    getDownloadedPapers,
    isDownloaded,

    // Statistics
    getStats,

    // Storage management
    clearAll,
    exportData,
    importData
  }
}
