export interface Paper {
  id: number
  title: string
  authors: string[]
  abstract: string
  keywords: string[]
  publishDate: string
  journal?: string
  volume?: string
  issue?: string
  pages?: string
  doi?: string
  url?: string
  pdfUrl?: string
  citations?: number
  downloads?: number
  status: 'pending' | 'processing' | 'completed' | 'failed'
  createdAt: string
  updatedAt: string
}

export interface PaperDetail extends Paper {
  fullText?: string
  references?: PaperReference[]
  metrics?: PaperMetrics
}

export interface PaperReference {
  id: number
  title: string
  authors: string[]
  year: number
  source: string
}

export interface PaperMetrics {
  citations: number
  downloads: number
  views: number
  hIndex?: number
  impactFactor?: number
}

export interface PaperQuery {
  page?: number
  pageSize?: number
  keyword?: string
  author?: string
  journal?: string
  startDate?: string
  endDate?: string
  sortBy?: 'createdAt' | 'publishDate' | 'citations' | 'downloads'
  sortOrder?: 'asc' | 'desc'
  status?: string
}

export interface PaperListResponse {
  items: Paper[]
  total: number
  page: number
  pageSize: number
}
