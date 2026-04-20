/**
 * PDF本地存储服务
 * 使用IndexedDB存储PDF文件，支持离线查看和缓存
 */

interface PdfMetadata {
  id: string
  name: string
  size: number
  timestamp: number
  documentId?: string
  projectId?: string
  url?: string // 原始URL
}

interface PdfRecord extends PdfMetadata {
  blob: Blob
}

class PdfStorageService {
  private readonly DB_NAME = 'LatexEditorPdfCache'
  private readonly STORE_NAME = 'pdfs'
  private readonly VERSION = 1
  private db: IDBDatabase | null = null

  /**
   * 初始化IndexedDB
   */
  async init(): Promise<void> {
    if (this.db) return

    return new Promise((resolve, reject) => {
      const request = indexedDB.open(this.DB_NAME, this.VERSION)

      request.onerror = () => {
        reject(new Error('Failed to open IndexedDB'))
      }

      request.onsuccess = () => {
        this.db = request.result
        resolve()
      }

      request.onupgradeneeded = (event) => {
        const db = (event.target as IDBOpenDBRequest).result

        // 创建对象存储
        if (!db.objectStoreNames.contains(this.STORE_NAME)) {
          const store = db.createObjectStore(this.STORE_NAME, { keyPath: 'id' })

          // 创建索引
          store.createIndex('timestamp', 'timestamp', { unique: false })
          store.createIndex('documentId', 'documentId', { unique: false })
          store.createIndex('projectId', 'projectId', { unique: false })
        }
      }
    })
  }

  /**
   * 保存PDF到IndexedDB
   */
  async savePdf(id: string, blob: Blob, metadata: Omit<PdfMetadata, 'id' | 'size'>): Promise<void> {
    await this.init()

    const record: PdfRecord = {
      id,
      blob,
      size: blob.size,
      ...metadata
    }

    return new Promise((resolve, reject) => {
      const transaction = this.db!.transaction([this.STORE_NAME], 'readwrite')
      const store = transaction.objectStore(this.STORE_NAME)
      const request = store.put(record)

      request.onsuccess = () => resolve()
      request.onerror = () => reject(new Error('Failed to save PDF'))
    })
  }

  /**
   * 从IndexedDB获取PDF
   */
  async getPdf(id: string): Promise<PdfRecord | null> {
    await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db!.transaction([this.STORE_NAME], 'readonly')
      const store = transaction.objectStore(this.STORE_NAME)
      const request = store.get(id)

      request.onsuccess = () => {
        resolve(request.result || null)
      }
      request.onerror = () => reject(new Error('Failed to get PDF'))
    })
  }

  /**
   * 从URL下载并保存PDF
   */
  async downloadAndSavePdf(
    url: string,
    id: string,
    metadata: Omit<PdfMetadata, 'id' | 'size'>
  ): Promise<Blob> {
    // 添加缓存破坏参数
    const urlWithCacheBust = url.includes('?t=') ? url : `${url}?t=${Date.now()}`

    const response = await fetch(urlWithCacheBust)
    if (!response.ok) {
      throw new Error(`Failed to download PDF: ${response.statusText}`)
    }

    const blob = await response.blob()

    // 保存到IndexedDB
    await this.savePdf(id, blob, { ...metadata, url })

    return blob
  }

  /**
   * 获取PDF的Blob URL（优先从本地，如果不存在则下载）
   */
  async getPdfUrl(
    id: string,
    sourceUrl: string,
    metadata: Omit<PdfMetadata, 'id' | 'size'>
  ): Promise<string> {
    // 先尝试从本地获取
    const localPdf = await this.getPdf(id)

    if (localPdf) {
      console.log('[PdfStorage] Using cached PDF:', id)
      return URL.createObjectURL(localPdf.blob)
    }

    // 本地没有，下载并保存
    console.log('[PdfStorage] Downloading PDF:', id)
    const blob = await this.downloadAndSavePdf(sourceUrl, id, metadata)
    return URL.createObjectURL(blob)
  }

  /**
   * 列出所有缓存的PDF
   */
  async listPdfs(): Promise<PdfMetadata[]> {
    await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db!.transaction([this.STORE_NAME], 'readonly')
      const store = transaction.objectStore(this.STORE_NAME)
      const request = store.getAll()

      request.onsuccess = () => {
        const records = request.result as PdfRecord[]
        // 移除blob字段，只返回元数据
        const metadata: PdfMetadata[] = records.map(({ blob, ...rest }) => rest)
        resolve(metadata)
      }
      request.onerror = () => reject(new Error('Failed to list PDFs'))
    })
  }

  /**
   * 删除指定的PDF
   */
  async deletePdf(id: string): Promise<void> {
    await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db!.transaction([this.STORE_NAME], 'readwrite')
      const store = transaction.objectStore(this.STORE_NAME)
      const request = store.delete(id)

      request.onsuccess = () => resolve()
      request.onerror = () => reject(new Error('Failed to delete PDF'))
    })
  }

  /**
   * 清空所有缓存的PDF
   */
  async clearAll(): Promise<void> {
    await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db!.transaction([this.STORE_NAME], 'readwrite')
      const store = transaction.objectStore(this.STORE_NAME)
      const request = store.clear()

      request.onsuccess = () => resolve()
      request.onerror = () => reject(new Error('Failed to clear PDFs'))
    })
  }

  /**
   * 获取缓存大小
   */
  async getCacheSize(): Promise<number> {
    await this.init()

    return new Promise((resolve, reject) => {
      const transaction = this.db!.transaction([this.STORE_NAME], 'readonly')
      const store = transaction.objectStore(this.STORE_NAME)
      const request = store.getAll()

      request.onsuccess = () => {
        const records = request.result as PdfRecord[]
        const totalSize = records.reduce((sum, record) => sum + record.size, 0)
        resolve(totalSize)
      }
      request.onerror = () => reject(new Error('Failed to get cache size'))
    })
  }

  /**
   * 清理旧缓存（删除超过指定天数的PDF）
   */
  async cleanOldCache(maxAgeDays: number = 7): Promise<number> {
    await this.init()

    const maxAge = maxAgeDays * 24 * 60 * 60 * 1000
    const now = Date.now()
    const records = await this.listPdfs()
    const toDelete: string[] = []

    for (const record of records) {
      if (now - record.timestamp > maxAge) {
        toDelete.push(record.id)
      }
    }

    for (const id of toDelete) {
      await this.deletePdf(id)
    }

    return toDelete.length
  }

  /**
   * 导出PDF为文件
   */
  async exportToFile(id: string, filename?: string): Promise<void> {
    const record = await this.getPdf(id)
    if (!record) {
      throw new Error('PDF not found in cache')
    }

    const url = URL.createObjectURL(record.blob)
    const a = document.createElement('a')
    a.href = url
    a.download = filename || `${record.name}.pdf`
    document.body.appendChild(a)
    a.click()
    document.body.removeChild(a)
    URL.revokeObjectURL(url)
  }
}

// 导出单例
export const pdfStorage = new PdfStorageService()

// 辅助函数：格式化文件大小
export function formatFileSize(bytes: number): string {
  if (bytes === 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB', 'GB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i]
}

// 辅助函数：格式化时间
export function formatTimestamp(timestamp: number): string {
  return new Date(timestamp).toLocaleString('zh-CN')
}
