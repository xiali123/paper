/**
 * Search Utilities
 *
 * Helper functions for search functionality including
 * query parsing, result formatting, and export utilities
 */

import type {
  AdvancedSearchQuery,
  SearchSuggestion,
  Paper,
  ExportFormat
} from '@/types/search'

// ============================================================================
// QUERY BUILDING UTILITIES
// ============================================================================

/**
 * Build search query string from advanced search parameters
 */
export function buildSearchQuery(params: AdvancedSearchQuery): string {
  const parts: string[] = []

  // Main query
  if (params.query?.trim()) {
    parts.push(params.query.trim())
  }

  // Field-specific searches
  const fieldMappings: Record<string, string> = {
    title: 'title',
    author: 'author',
    authors: 'author',
    abstract: 'abstract',
    keywords: 'keywords',
    journal: 'journal',
    doi: 'doi'
  }

  Object.entries(fieldMappings).forEach(([paramKey, searchField]) => {
    const value = (params as any)[paramKey]
    if (value?.trim()) {
      parts.push(`${searchField}:"${value.trim()}"`)
    }
  })

  return parts.join(' ')
}

/**
 * Parse search query string into components
 */
export function parseSearchQuery(queryString: string): {
  terms: string[]
  phrases: string[]
  fields: Record<string, string>
} {
  const terms: string[] = []
  const phrases: string[] = []
  const fields: Record<string, string> = {}

  // Match field-specific searches like title:"machine learning"
  const fieldRegex = /(\w+):"([^"]+)"/g
  let match

  while ((match = fieldRegex.exec(queryString)) !== null) {
    const [, field, value] = match
    fields[field] = value
    queryString = queryString.replace(match[0], '')
  }

  // Match quoted phrases
  const phraseRegex = /"([^"]+)"/g
  while ((match = phraseRegex.exec(queryString)) !== null) {
    phrases.push(match[1])
    queryString = queryString.replace(match[0], '')
  }

  // Extract remaining terms
  terms.push(
    ...queryString
      .split(/\s+/)
      .map(t => t.trim())
      .filter(t => t.length > 0)
  )

  return { terms, phrases, fields }
}

/**
 * Validate search query
 */
export function validateSearchQuery(query: string): {
  valid: boolean
  errors: string[]
} {
  const errors: string[] = []

  if (!query || query.trim().length === 0) {
    errors.push('Search query cannot be empty')
  }

  if (query.length > 500) {
    errors.push('Search query is too long (maximum 500 characters)')
  }

  // Check for unbalanced quotes
  const quoteCount = (query.match(/"/g) || []).length
  if (quoteCount % 2 !== 0) {
    errors.push('Unbalanced quotes in search query')
  }

  return {
    valid: errors.length === 0,
    errors
  }
}

// ============================================================================
// RESULT FORMATTING UTILITIES
// ============================================================================

/**
 * Highlight search terms in text
 */
export function highlightSearchTerms(
  text: string,
  searchTerms: string[],
  maxLength: number = 200
): string {
  if (!text) return ''

  // Truncate text if too long
  let processedText = text
  if (text.length > maxLength) {
    const truncateAt = text.indexOf(' ', maxLength)
    processedText = text.substring(0, truncateAt > 0 ? truncateAt : maxLength) + '...'
  }

  // Highlight search terms
  searchTerms.forEach(term => {
    if (term.length < 2) return
    const regex = new RegExp(`(${escapeRegex(term)})`, 'gi')
    processedText = processedText.replace(regex, '<mark>$1</mark>')
  })

  return processedText
}

/**
 * Extract relevant snippet from text containing search terms
 */
export function extractRelevantSnippet(
  text: string,
  searchTerms: string[],
  snippetLength: number = 300
): string {
  if (!text) return ''

  // Find best position to start snippet
  let bestPosition = 0
  let maxScore = 0

  searchTerms.forEach(term => {
    const position = text.toLowerCase().indexOf(term.toLowerCase())
    if (position !== -1) {
      const score = term.length / (position + 1)
      if (score > maxScore) {
        maxScore = score
        bestPosition = Math.max(0, position - 50)
      }
    }
  })

  // Extract snippet
  const start = Math.max(0, bestPosition)
  const end = Math.min(text.length, start + snippetLength)

  let snippet = text.substring(start, end)

  // Add ellipsis if needed
  if (start > 0) snippet = '...' + snippet
  if (end < text.length) snippet = snippet + '...'

  return snippet
}

/**
 * Format paper for search result display
 */
export function formatPaperForSearch(paper: Paper): {
  id: number
  title: string
  authors: string
  year: number
  venue: string
  snippet: string
  relevanceScore: number
} {
  return {
    id: paper.id,
    title: paper.title,
    authors: formatAuthors(paper.authors, 3),
    year: paper.year,
    venue: paper.journal?.full || paper.journal?.short || 'Unknown Venue',
    snippet: paper.abstract ? extractRelevantSnippet(paper.abstract, [], 200) : '',
    relevanceScore: calculateRelevanceScore(paper)
  }
}

/**
 * Calculate relevance score for paper
 */
export function calculateRelevanceScore(paper: Paper): number {
  let score = 0

  // Citation score (normalized to 0-100)
  if (paper.citation_count) {
    score += Math.min(100, Math.log10(paper.citation_count + 1) * 20)
  }

  // Recency score (newer papers get higher scores)
  const currentYear = new Date().getFullYear()
  const yearsSincePublication = currentYear - paper.year
  const recencyScore = Math.max(0, 100 - yearsSincePublication * 5)
  score += recencyScore

  // CCF level score
  if (paper.ccf_level) {
    const levelScores: Record<string, number> = {
      'A': 30,
      'B': 20,
      'C': 10
    }
    score += levelScores[paper.ccf_level] || 0
  }

  return Math.round(score)
}

// ============================================================================
// EXPORT UTILITIES
// ============================================================================

/**
 * Export papers to CSV format
 */
export function exportToCSV(papers: Paper[], filename: string = 'papers.csv'): void {
  const headers = [
    'ID',
    'Title',
    'Authors',
    'Year',
    'Journal',
    'CCF Level',
    'Citations',
    'DOI',
    'URL'
  ]

  const rows = papers.map(paper => [
    paper.id,
    `"${paper.title.replace(/"/g, '""')}"`,
    `"${paper.authors.join('; ')}"`,
    paper.year,
    `"${paper.journal?.full || paper.journal?.short || ''}"`,
    paper.ccf_level || '',
    paper.citation_count,
    paper.doi || '',
    paper.url || ''
  ])

  const csv = [headers.join(','), ...rows.map(r => r.join(','))].join('\n')
  downloadFile(csv, filename, 'text/csv')
}

/**
 * Export papers to JSON format
 */
export function exportToJSON(papers: Paper[], filename: string = 'papers.json'): void {
  const json = JSON.stringify(papers, null, 2)
  downloadFile(json, filename, 'application/json')
}

/**
 * Export papers to BibTeX format
 */
export function exportToBibTeX(papers: Paper[], filename: string = 'papers.bib'): void {
  const bibtex = papers.map(paper => {
    const key = generateBibTeXKey(paper)
    return `@article{${key},
  title={${paper.title}},
  author={${paper.authors.join(' and ')}},
  journal={${paper.journal?.full || paper.journal?.short || ''}},
  year={${paper.year}},
  volume={${paper.journal?.volume || ''}},
  number={${paper.journal?.issue || ''}},
  pages={${paper.journal?.pages || ''}},
  doi={${paper.doi || ''}}
}`
  }).join('\n\n')

  downloadFile(bibtex, filename, 'text/plain')
}

/**
 * Generate BibTeX key from paper
 */
export function generateBibTeXKey(paper: Paper): string {
  const firstAuthor = paper.authors[0] || 'Unknown'
  const lastName = firstAuthor.split(' ').pop() || 'Unknown'
  const year = paper.year
  const firstWord = paper.title.split(/\s+/)[0] || 'Paper'

  return `${lastName}${year}${firstWord}`.replace(/[^a-zA-Z0-9]/g, '')
}

/**
 * Trigger file download
 */
function downloadFile(content: string, filename: string, mimeType: string): void {
  const blob = new Blob([content], { type: mimeType })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')

  link.href = url
  link.download = filename
  document.body.appendChild(link)
  link.click()

  document.body.removeChild(link)
  URL.revokeObjectURL(url)
}

// ============================================================================
// SEARCH SUGGESTION UTILITIES
// ============================================================================

/**
 * Generate search suggestions from partial input
 */
export function generateSearchSuggestions(
  partial: string,
  searchHistory: string[],
  maxSuggestions: number = 8
): SearchSuggestion[] {
  if (!partial || partial.length < 2) return []

  const suggestions: SearchSuggestion[] = []

  // Add history matches
  searchHistory.forEach(query => {
    if (query.toLowerCase().includes(partial.toLowerCase())) {
      suggestions.push({
        text: query,
        type: 'history'
      })
    }
  })

  // Sort by relevance and limit
  suggestions.sort((a, b) => {
    const aIndex = a.text.toLowerCase().indexOf(partial.toLowerCase())
    const bIndex = b.text.toLowerCase().indexOf(partial.toLowerCase())
    return aIndex - bIndex
  })

  return suggestions.slice(0, maxSuggestions)
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Format authors list
 */
function formatAuthors(authors: string[], maxAuthors: number = 3): string {
  if (authors.length === 0) return 'Unknown'
  if (authors.length <= maxAuthors) return authors.join(', ')

  return `${authors.slice(0, maxAuthors).join(', ')} et al.`
}

/**
 * Escape special regex characters
 */
function escapeRegex(string: string): string {
  return string.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
}

/**
 * Debounce function
 */
export function debounce<T extends (...args: any[]) => any>(
  func: T,
  wait: number
): (...args: Parameters<T>) => void {
  let timeout: NodeJS.Timeout | null = null

  return function executedFunction(...args: Parameters<T>) {
    const later = () => {
      timeout = null
      func(...args)
    }

    if (timeout) clearTimeout(timeout)
    timeout = setTimeout(later, wait)
  }
}

/**
 * Throttle function
 */
export function throttle<T extends (...args: any[]) => any>(
  func: T,
  limit: number
): (...args: Parameters<T>) => void {
  let inThrottle: boolean

  return function executedFunction(...args: Parameters<T>) {
    if (!inThrottle) {
      func(...args)
      inThrottle = true
      setTimeout(() => (inThrottle = false), limit)
    }
  }
}