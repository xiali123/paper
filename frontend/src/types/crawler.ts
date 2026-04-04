export interface CrawlerConfig {
  id: number
  name: string
  description?: string
  source: 'arxiv' | 'ieee' | 'springer' | 'elsevier' | 'custom'
  enabled: boolean
  schedule?: string
  keywords: string[]
  categories: string[]
  maxPapers: number
  depth: number
  proxy?: string
  headers?: Record<string, string>
  createdAt: string
  updatedAt: string
}

export interface CrawlerTask {
  id: string
  configId: number
  configName: string
  status: 'pending' | 'running' | 'completed' | 'failed' | 'cancelled'
  progress: number
  totalPapers: number
  successPapers: number
  failedPapers: number
  startTime: string
  endTime?: string
  error?: string
}

export interface CrawlerStats {
  totalTasks: number
  runningTasks: number
  completedTasks: number
  failedTasks: number
  totalPapers: number
  successPapers: number
  failedPapers: number
  avgDuration: number
}

export interface CrawlerLog {
  id: string
  taskId: string
  level: 'info' | 'warning' | 'error' | 'debug'
  message: string
  timestamp: string
}
