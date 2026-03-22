/**
 * Type Utility Functions
 *
 * Helper functions for common type operations and transformations
 *
 * @module types/utils
 */

import type {
  Paper,
  PaperDetail,
  PaperListItem,
  JournalInfo,
  SearchParams,
  CCFLevel
} from './index'

// ============================================================================
// PAPER UTILITIES
// ============================================================================

/**
 * Convert Paper to PaperListItem for list views
 */
export function toListItem(paper: Paper): PaperListItem {
  return {
    id: paper.id,
    title: truncateText(paper.title, 100),
    journal: extractJournalName(paper.journal),
    year: normalizeYear(paper.year),
    level: paper.level.toString(),
    authors: truncateAuthors(paper.authors, 50)
  }
}

/**
 * Convert multiple Papers to PaperListItems
 */
export function toListItems(papers: Paper[]): PaperListItem[] {
  return papers.map(toListItem)
}

/**
 * Check if a paper is a PaperDetail
 */
export function isPaperDetail(paper: Paper): paper is PaperDetail {
  return 'citations' in paper &&
         'references' in paper &&
         'downloadCount' in paper
}

/**
 * Extract journal name from journal field
 */
export function extractJournalName(journal: JournalInfo | string): string {
  if (typeof journal === 'string') {
    return journal
  }
  return journal.short || journal.full
}

/**
 * Extract full journal name
 */
export function extractFullJournalName(journal: JournalInfo | string): string {
  if (typeof journal === 'string') {
    return journal
  }
  return journal.full
}

/**
 * Normalize year to string
 */
export function normalizeYear(year: string | number): string {
  return year.toString()
}

/**
 * Normalize year to number
 */
export function normalizeYearNumber(year: string | number): number {
  if (typeof year === 'number') {
    return year
  }
  return parseInt(year, 10)
}

/**
 * Check if year is valid
 */
export function isValidYear(year: string | number): boolean {
  const yearNum = normalizeYearNumber(year)
  const currentYear = new Date().getFullYear()
  return yearNum >= 1900 && yearNum <= currentYear + 1
}

/**
 * Extract CCF level as enum
 */
export function extractCCFLevel(level: string | CCFLevel): CCFLevel {
  const levelStr = level.toString().toUpperCase()
  if (levelStr === 'A' || levelStr === 'B' || levelStr === 'C') {
    return levelStr as CCFLevel
  }
  throw new Error(`Invalid CCF level: ${level}`)
}

/**
 * Format authors for display
 */
export function formatAuthors(authors: string, maxCount: number = 3): string {
  const authorList = authors.split(/[,;]/).map(a => a.trim()).filter(Boolean)

  if (authorList.length <= maxCount) {
    return authorList.join(', ')
  }

  return `${authorList.slice(0, maxCount).join(', ')} et al.`
}

/**
 * Truncate author list to fit character limit
 */
export function truncateAuthors(authors: string, maxLength: number = 50): string {
  if (authors.length <= maxLength) {
    return authors
  }

  // Try to split on comma and keep first author
  const commaIndex = authors.indexOf(',')
  if (commaIndex > 0 && commaIndex < maxLength - 5) {
    return `${authors.substring(0, commaIndex)} et al.`
  }

  // Just truncate
  return `${authors.substring(0, maxLength - 4)}...`
}

/**
 * Truncate text with ellipsis
 */
export function truncateText(text: string, maxLength: number): string {
  if (text.length <= maxLength) {
    return text
  }
  return `${text.substring(0, maxLength - 3)}...`
}

/**
 * Extract DOI from paper
 */
export function extractDOI(paper: Paper): string | undefined {
  if (typeof paper.urls?.doi === 'string') {
    return paper.urls.doi
  }

  // Try to extract DOI from abstract if present
  if (paper.abstract) {
    const doiMatch = paper.abstract.match(/doi\.org\/(10\.\d{4,}\/[^\s]+)/i)
    if (doiMatch) {
      return doiMatch[1]
    }
  }

  return undefined
}

/**
 * Get paper URL (DOI or direct URL)
 */
export function getPaperUrl(paper: Paper): string | undefined {
  return paper.urls?.doi || paper.urls?.journal || paper.urls?.pdf
}

/**
 * Format citation count
 */
export function formatCitationCount(count: number | undefined): string {
  if (count === undefined) {
    return 'N/A'
  }

  if (count >= 1000) {
    return `${(count / 1000).toFixed(1)}k`
  }

  return count.toString()
}

/**
 * Check if paper has keywords
 */
export function hasKeywords(paper: Paper): boolean {
  return Array.isArray(paper.keywords) && paper.keywords.length > 0
}

/**
 * Check if paper has abstract
 */
export function hasAbstract(paper: Paper): boolean {
  return typeof paper.abstract === 'string' && paper.abstract.length > 0
}

/**
 * Calculate paper "age" in years
 */
export function calculatePaperAge(paper: Paper): number {
  const year = normalizeYearNumber(paper.year)
  const currentYear = new Date().getFullYear()
  return Math.max(0, currentYear - year)
}

/**
 * Sort papers by year (newest first)
 */
export function sortByYear(papers: Paper[]): Paper[] {
  return [...papers].sort((a, b) => {
    const yearA = normalizeYearNumber(a.year)
    const yearB = normalizeYearNumber(b.year)
    return yearB - yearA
  })
}

/**
 * Sort papers by citations (most cited first)
 */
export function sortByCitations(papers: (Paper | PaperDetail)[]): (Paper | PaperDetail)[] {
  return [...papers].sort((a, b) => {
    const citationsA = isPaperDetail(a) ? a.citations : 0
    const citationsB = isPaperDetail(b) ? b.citations : 0
    return citationsB - citationsA
  })
}

/**
 * Filter papers by CCF level
 */
export function filterByLevel(papers: Paper[], level: CCFLevel): Paper[] {
  const levelStr = level.toString()
  return papers.filter(paper => paper.level.toString() === levelStr)
}

/**
 * Filter papers by year range
 */
export function filterByYearRange(papers: Paper[], startYear: number, endYear: number): Paper[] {
  return papers.filter(paper => {
    const year = normalizeYearNumber(paper.year)
    return year >= startYear && year <= endYear
  })
}

/**
 * Filter papers by journal
 */
export function filterByJournal(papers: Paper[], journal: string): Paper[] {
  const journalLower = journal.toLowerCase()
  return papers.filter(paper => {
    const journalName = extractJournalName(paper.journal).toLowerCase()
    return journalName.includes(journalLower)
  })
}

/**
 * Search papers by keyword in title, abstract, or keywords
 */
export function searchPapers(papers: Paper[], query: string): Paper[] {
  const queryLower = query.toLowerCase()

  return papers.filter(paper => {
    // Search in title
    if (paper.title.toLowerCase().includes(queryLower)) {
      return true
    }

    // Search in abstract
    if (paper.abstract && paper.abstract.toLowerCase().includes(queryLower)) {
      return true
    }

    // Search in keywords
    if (paper.keywords && paper.keywords.some(kw => kw.toLowerCase().includes(queryLower))) {
      return true
    }

    // Search in authors
    if (paper.authors.toLowerCase().includes(queryLower)) {
      return true
    }

    return false
  })
}

// ============================================================================
// SEARCH PARAM UTILITIES
// ============================================================================

/**
 * Create default search params
 */
export function createDefaultParams(): SearchParams {
  return {
    q: '',
    offset: 0,
    limit: 20
  }
}

/**
 * Clone search params
 */
export function cloneParams(params: SearchParams): SearchParams {
  return { ...params }
}

/**
 * Update search params safely
 */
export function updateParams(params: SearchParams, updates: Partial<SearchParams>): SearchParams {
  return { ...params, ...updates }
}

/**
 * Convert search params to URL query string
 */
export function paramsToQueryString(params: SearchParams): string {
  const queryParams = new URLSearchParams()

  if (params.q) queryParams.append('q', params.q)
  if (params.year) queryParams.append('year', params.year)
  if (params.level) queryParams.append('level', params.level)
  if (params.offset !== undefined) queryParams.append('offset', params.offset.toString())
  if (params.limit !== undefined) queryParams.append('limit', params.limit.toString())
  if (params.sort) queryParams.append('sort', params.sort)
  if (params.order) queryParams.append('order', params.order)
  if (params.journal) queryParams.append('journal', params.journal)
  if (params.author) queryParams.append('author', params.author)

  return queryParams.toString()
}

/**
 * Calculate page from offset and limit
 */
export function calculatePage(offset: number, limit: number): number {
  return Math.floor(offset / limit) + 1
}

/**
 * Calculate offset from page and limit
 */
export function calculateOffset(page: number, limit: number): number {
  return (page - 1) * limit
}

/**
 * Convert page-based pagination to offset-based
 */
export function pageToOffset(page: number, pageSize: number): { offset: number; limit: number } {
  return {
    offset: calculateOffset(page, pageSize),
    limit: pageSize
  }
}

/**
 * Convert offset-based pagination to page-based
 */
export function offsetToPage(offset: number, limit: number): { page: number; pageSize: number } {
  return {
    page: calculatePage(offset, limit),
    pageSize: limit
  }
}

// ============================================================================
// COLLECTION UTILITIES
// ============================================================================

/**
 * Group papers by year
 */
export function groupByYear(papers: Paper[]): Map<number, Paper[]> {
  const grouped = new Map<number, Paper[]>()

  for (const paper of papers) {
    const year = normalizeYearNumber(paper.year)
    if (!grouped.has(year)) {
      grouped.set(year, [])
    }
    grouped.get(year)!.push(paper)
  }

  return grouped
}

/**
 * Group papers by CCF level
 */
export function groupByLevel(papers: Paper[]): Map<CCFLevel, Paper[]> {
  const grouped = new Map<CCFLevel, Paper[]>()
  grouped.set('A', [])
  grouped.set('B', [])
  grouped.set('C', [])

  for (const paper of papers) {
    const level = extractCCFLevel(paper.level)
    grouped.get(level)!.push(paper)
  }

  return grouped
}

/**
 * Group papers by journal
 */
export function groupByJournal(papers: Paper[]): Map<string, Paper[]> {
  const grouped = new Map<string, Paper[]>()

  for (const paper of papers) {
    const journal = extractJournalName(paper.journal)
    if (!grouped.has(journal)) {
      grouped.set(journal, [])
    }
    grouped.get(journal)!.push(paper)
  }

  return grouped
}

/**
 * Count papers by year
 */
export function countByYear(papers: Paper[]): Map<number, number> {
  const counts = new Map<number, number>()

  for (const paper of papers) {
    const year = normalizeYearNumber(paper.year)
    counts.set(year, (counts.get(year) || 0) + 1)
  }

  return counts
}

/**
 * Count papers by CCF level
 */
export function countByLevel(papers: Paper[]): Map<CCFLevel, number> {
  const counts = new Map<CCFLevel, number>()
  counts.set('A', 0)
  counts.set('B', 0)
  counts.set('C', 0)

  for (const paper of papers) {
    const level = extractCCFLevel(paper.level)
    counts.set(level, counts.get(level)! + 1)
  }

  return counts
}

/**
 * Get unique journals from papers
 */
export function getUniqueJournals(papers: Paper[]): string[] {
  const journals = new Set<string>()

  for (const paper of papers) {
    journals.add(extractJournalName(paper.journal))
  }

  return Array.from(journals).sort()
}

/**
 * Get year range from papers
 */
export function getYearRange(papers: Paper[]): { min: number; max: number } {
  if (papers.length === 0) {
    return { min: 0, max: 0 }
  }

  let min = Infinity
  let max = -Infinity

  for (const paper of papers) {
    const year = normalizeYearNumber(paper.year)
    min = Math.min(min, year)
    max = Math.max(max, year)
  }

  return { min, max }
}

/**
 * Get unique years from papers (sorted)
 */
export function getUniqueYears(papers: Paper[]): number[] {
  const years = new Set<number>()

  for (const paper of papers) {
    years.add(normalizeYearNumber(paper.year))
  }

  return Array.from(years).sort((a, b) => b - a) // Descending
}

// ============================================================================
// STATISTICS UTILITIES
// ============================================================================

/**
 * Calculate average papers per year
 */
export function calculateAveragePapersPerYear(papers: Paper[]): number {
  const yearRange = getYearRange(papers)
  const yearSpan = yearRange.max - yearRange.min + 1

  if (yearSpan <= 0) {
    return 0
  }

  return papers.length / yearSpan
}

/**
 * Calculate percentage of papers at a level
 */
export function calculateLevelPercentage(papers: Paper[], level: CCFLevel): number {
  const counts = countByLevel(papers)
  const levelCount = counts.get(level) || 0

  if (papers.length === 0) {
    return 0
  }

  return (levelCount / papers.length) * 100
}

/**
 * Find most productive year
 */
export function findMostProductiveYear(papers: Paper[]): { year: number; count: number } | null {
  const counts = countByYear(papers)

  let maxCount = 0
  let maxYear: number | null = null

  for (const [year, count] of counts.entries()) {
    if (count > maxCount) {
      maxCount = count
      maxYear = year
    }
  }

  return maxYear !== null ? { year: maxYear, count: maxCount } : null
}

// ============================================================================
// PAGINATION UTILITIES
// ============================================================================

/**
 * Calculate total pages from total count and page size
 */
export function calculateTotalPages(total: number, pageSize: number): number {
  return Math.ceil(total / pageSize)
}

/**
 * Check if there's a next page
 */
export function hasNextPage(currentPage: number, totalPages: number): boolean {
  return currentPage < totalPages
}

/**
 * Check if there's a previous page
 */
export function hasPreviousPage(currentPage: number): boolean {
  return currentPage > 1
}

/**
 * Get page numbers to show in pagination (with ellipsis)
 */
export function getPaginationPages(
  currentPage: number,
  totalPages: number,
  maxVisible: number = 7
): (number | string)[] {
  if (totalPages <= maxVisible) {
    return Array.from({ length: totalPages }, (_, i) => i + 1)
  }

  const pages: (number | string)[] = []
  const sideCount = Math.floor((maxVisible - 3) / 2) // Pages on each side

  // Always show first page
  pages.push(1)

  // Add ellipsis if needed
  if (currentPage > sideCount + 2) {
    pages.push('...')
  }

  // Add pages around current
  const start = Math.max(2, currentPage - sideCount)
  const end = Math.min(totalPages - 1, currentPage + sideCount)

  for (let i = start; i <= end; i++) {
    pages.push(i)
  }

  // Add ellipsis if needed
  if (currentPage < totalPages - sideCount - 1) {
    pages.push('...')
  }

  // Always show last page
  pages.push(totalPages)

  return pages
}

// ============================================================================
// EXPORT UTILITIES
// ============================================================================

/**
 * Convert papers to CSV format
 */
export function papersToCSV(papers: Paper[]): string {
  const headers = ['ID', 'Title', 'Authors', 'Journal', 'Year', 'Level', 'DOI']
  const rows = papers.map(paper => [
    paper.id,
    `"${paper.title.replace(/"/g, '""')}"`,
    `"${paper.authors}"`,
    extractJournalName(paper.journal),
    normalizeYear(paper.year),
    paper.level,
    extractDOI(paper) || ''
  ])

  return [headers.join(','), ...rows.map(row => row.join(','))].join('\n')
}

/**
 * Download data as file
 */
export function downloadAsFile(content: string, filename: string, mimeType: string = 'text/plain'): void {
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

/**
 * Download papers as CSV file
 */
export function downloadPapersAsCSV(papers: Paper[], filename: string = 'papers.csv'): void {
  const csv = papersToCSV(papers)
  downloadAsFile(csv, filename, 'text/csv')
}

// ============================================================================
// DEFAULT EXPORT
// ============================================================================

export default {
  // Paper utilities
  toListItem,
  toListItems,
  isPaperDetail,
  extractJournalName,
  extractFullJournalName,
  normalizeYear,
  normalizeYearNumber,
  isValidYear,
  extractCCFLevel,
  formatAuthors,
  truncateAuthors,
  truncateText,
  extractDOI,
  getPaperUrl,
  formatCitationCount,
  hasKeywords,
  hasAbstract,
  calculatePaperAge,
  sortByYear,
  sortByCitations,
  filterByLevel,
  filterByYearRange,
  filterByJournal,
  searchPapers,

  // Search param utilities
  createDefaultParams,
  cloneParams,
  updateParams,
  paramsToQueryString,
  calculatePage,
  calculateOffset,
  pageToOffset,
  offsetToPage,

  // Collection utilities
  groupByYear,
  groupByLevel,
  groupByJournal,
  countByYear,
  countByLevel,
  getUniqueJournals,
  getYearRange,
  getUniqueYears,

  // Statistics utilities
  calculateAveragePapersPerYear,
  calculateLevelPercentage,
  findMostProductiveYear,

  // Pagination utilities
  calculateTotalPages,
  hasNextPage,
  hasPreviousPage,
  getPaginationPages,

  // Export utilities
  papersToCSV,
  downloadAsFile,
  downloadPapersAsCSV
}
