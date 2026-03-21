export interface Paper {
  id: number
  title: string
  journal: {
    full: string
    short: string
  }
  year: string
  level: string
  authors: string
  urls?: {
    doi?: string
    journal?: string
  }
}

export interface SearchResult {
  papers: Paper[]
  total: number
  keyword: string
  duration: number
}

export interface Statistics {
  totalPapers: number
  totalJournals: number
  topTierPapers: number
  papersLastYear: number
  mostActiveJournal: string
}
