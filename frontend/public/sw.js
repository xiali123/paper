/**
 * Service Worker - 离线编辑支持
 * 版本: 1.0.0
 * 功能: 缓存静态资源、拦截API请求、同步离线修改
 */

const CACHE_VERSION = 'v1.0.0'
const CACHE_NAME = `paper-crawler-${CACHE_VERSION}`

// 需要缓存的静态资源
const STATIC_CACHE_URLS = [
  '/',
  '/index.html',
  '/manifest.json',
  '/assets/index.js',
  '/assets/index.css',
  // 添加其他需要离线访问的资源
]

// API缓存策略配置
const API_CACHE_PATTERNS = {
  // 可缓存的GET请求
  cacheable: [
    /\/api\/latex\/documents$/,
    /\/api\/latex\/templates$/,
    /\/api\/papers(\?.*)?$/
  ],
  // 不缓存的请求
  bypass: [
    /\/api\/auth/,
    /\/api\/user\/.*\/settings/
  ]
}

// 离线操作队列
let offlineQueue = []
let syncInProgress = false

/**
 * 安装Service Worker
 */
self.addEventListener('install', (event) => {
  console.log('[SW] Installing...')

  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => {
      console.log('[SW] Caching static resources')
      return cache.addAll(STATIC_CACHE_URLS)
    }).then(() => {
      // 强制激活新的Service Worker
      return self.skipWaiting()
    })
  )
})

/**
 * 激活Service Worker
 */
self.addEventListener('activate', (event) => {
  console.log('[SW] Activating...')

  event.waitUntil(
    caches.keys().then((cacheNames) => {
      return Promise.all(
        cacheNames.map((cacheName) => {
          // 删除旧版本的缓存
          if (cacheName !== CACHE_NAME && cacheName.startsWith('paper-crawler-')) {
            console.log('[SW] Deleting old cache:', cacheName)
            return caches.delete(cacheName)
          }
        })
      )
    }).then(() => {
      // 立即控制所有页面
      return self.clients.claim()
    })
  )
})

/**
 * 拦截网络请求
 */
self.addEventListener('fetch', (event) => {
  const { request } = event
  const url = new URL(request.url)

  // 跳过chrome扩展等非同源请求
  if (url.protocol !== 'http:' && url.protocol !== 'https:') {
    return
  }

  // API请求处理
  if (url.pathname.startsWith('/api/')) {
    event.respondWith(handleApiRequest(request))
    return
  }

  // 静态资源请求处理
  event.respondWith(handleStaticRequest(request))
})

/**
 * 处理API请求
 */
async function handleApiRequest(request) {
  const url = new URL(request.url)
  const method = request.method

  // 检查是否应该绕过缓存
  const shouldBypass = API_CACHE_PATTERNS.bypass.some(pattern => pattern.test(url.pathname))
  if (shouldBypass) {
    return fetch(request)
  }

  // GET请求 - 尝试网络优先，失败则使用缓存
  if (method === 'GET') {
    const isCacheable = API_CACHE_PATTERNS.cacheable.some(pattern => pattern.test(url.pathname))

    if (isCacheable) {
      try {
        // 网络优先策略
        const networkResponse = await fetch(request)

        if (networkResponse.ok) {
          // 缓存成功的响应
          const cache = await caches.open(CACHE_NAME)
          cache.put(request, networkResponse.clone())
        }

        return networkResponse
      } catch (error) {
        // 网络失败，尝试使用缓存
        const cachedResponse = await caches.match(request)
        if (cachedResponse) {
          console.log('[SW] Using cached response for:', request.url)
          return cachedResponse
        }

        // 返回离线响应
        return createOfflineResponse(request)
      }
    }
  }

  // POST/PUT/DELETE请求 - 在线时直接发送
  if (navigator.onLine) {
    return fetch(request)
  }

  // 离线时将修改操作加入队列
  if (['POST', 'PUT', 'DELETE'].includes(method)) {
    await queueOfflineRequest(request)
    return createQueuedResponse()
  }

  return fetch(request)
}

/**
 * 处理静态资源请求
 */
async function handleStaticRequest(request) {
  // 缓存优先策略
  const cachedResponse = await caches.match(request)

  if (cachedResponse) {
    // 在后台更新缓存
    fetch(request).then((networkResponse) => {
      if (networkResponse.ok) {
        const cache = await caches.open(CACHE_NAME)
        cache.put(request, networkResponse)
      }
    })

    return cachedResponse
  }

  // 缓存未命中，尝试网络
  try {
    const networkResponse = await fetch(request)

    if (networkResponse.ok) {
      const cache = await caches.open(CACHE_NAME)
      cache.put(request, networkResponse.clone())
    }

    return networkResponse
  } catch (error) {
    // 返回离线页面
    return caches.match('/offline.html') || new Response('Offline', { status: 503 })
  }
}

/**
 * 将离线请求加入队列
 */
async function queueOfflineRequest(request) {
  try {
    // 克隆请求以读取body
    const requestData = {
      url: request.url,
      method: request.method,
      headers: Object.fromEntries(request.headers.entries()),
      body: await request.text(),
      timestamp: Date.now()
    }

    offlineQueue.push(requestData)

    // 保存到IndexedDB
    await saveOfflineQueue(offlineQueue)

    // 通知所有客户端有离线操作
    const clients = await self.clients.matchAll()
    clients.forEach(client => {
      client.postMessage({
        type: 'OFFLINE_OPERATION_QUEUED',
        count: offlineQueue.length
      })
    })

    console.log('[SW] Offline operation queued:', requestData)
  } catch (error) {
    console.error('[SW] Failed to queue offline request:', error)
  }
}

/**
 * 创建离线响应
 */
function createOfflineResponse(request) {
  const url = new URL(request.url)

  // 针对不同API返回模拟数据
  if (url.pathname.startsWith('/api/latex/documents')) {
    const offlineData = {
      success: true,
      items: [],
      offline: true,
      message: '当前离线，显示缓存数据'
    }
    return new Response(JSON.stringify(offlineData), {
      status: 200,
      headers: { 'Content-Type': 'application/json' }
    })
  }

  return new Response(JSON.stringify({
    success: false,
    offline: true,
    message: '当前离线，请检查网络连接'
  }), {
    status: 503,
    headers: { 'Content-Type': 'application/json' }
  })
}

/**
 * 创建已排队响应
 */
function createQueuedResponse() {
  return new Response(JSON.stringify({
    success: true,
    queued: true,
    message: '操作已加入队列，将在恢复网络后同步'
  }), {
    status: 202,
    headers: { 'Content-Type': 'application/json' }
  })
}

/**
 * 同步离线队列
 */
async function syncOfflineQueue() {
  if (syncInProgress || offlineQueue.length === 0 || !navigator.onLine) {
    return
  }

  syncInProgress = true
  console.log('[SW] Syncing offline queue...')

  const successful = []
  const failed = []

  for (const operation of offlineQueue) {
    try {
      const response = await fetch(operation.url, {
        method: operation.method,
        headers: operation.headers,
        body: operation.body
      })

      if (response.ok) {
        successful.push(operation)
      } else {
        failed.push(operation)
      }
    } catch (error) {
      console.error('[SW] Sync failed for:', operation.url, error)
      failed.push(operation)
    }
  }

  // 更新队列
  offlineQueue = failed
  await saveOfflineQueue(offlineQueue)

  // 通知客户端同步结果
  const clients = await self.clients.matchAll()
  clients.forEach(client => {
    client.postMessage({
      type: 'OFFLINE_SYNC_COMPLETE',
      successful: successful.length,
      failed: failed.length,
      remaining: offlineQueue.length
    })
  })

  syncInProgress = false

  if (offlineQueue.length > 0) {
    // 5秒后重试失败的请求
    setTimeout(syncOfflineQueue, 5000)
  }
}

/**
 * 保存离线队列到IndexedDB
 */
async function saveOfflineQueue(queue) {
  // 简化版本：使用localStorage（生产环境应使用IndexedDB）
  try {
    localStorage.setItem('offline_queue', JSON.stringify(queue))
  } catch (error) {
    console.error('[SW] Failed to save offline queue:', error)
  }
}

/**
 * 从IndexedDB加载离线队列
 */
async function loadOfflineQueue() {
  try {
    const saved = localStorage.getItem('offline_queue')
    if (saved) {
      offlineQueue = JSON.parse(saved)
    }
  } catch (error) {
    console.error('[SW] Failed to load offline queue:', error)
  }
}

/**
 * 监听消息
 */
self.addEventListener('message', (event) => {
  const { type, data } = event.data

  switch (type) {
    case 'SYNC_OFFLINE_QUEUE':
      syncOfflineQueue()
      break
    case 'CLEAR_OFFLINE_QUEUE':
      offlineQueue = []
      saveOfflineQueue([])
      break
    case 'GET_OFFLINE_QUEUE_SIZE':
      event.ports[0].postMessage({ size: offlineQueue.length })
      break
    case 'SKIP_WAITING':
      self.skipWaiting()
      break
  }
})

/**
 * 监听网络状态变化
 */
self.addEventListener('online', () => {
  console.log('[SW] Network online, starting sync...')
  syncOfflineQueue()
})

/**
 * 定期同步离线队列
 */
setInterval(() => {
  if (navigator.onLine && offlineQueue.length > 0 && !syncInProgress) {
    syncOfflineQueue()
  }
}, 60000) // 每分钟检查一次

// 初始化时加载离线队列
loadOfflineQueue()
