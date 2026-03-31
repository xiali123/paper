/**
 * @file crawler.ts
 * @brief Crawler API module for fetching papers from academic sources
 */

import request from '@/utils/request'

export type CrawlerSource = 'arxiv' | 'pubmed' | 'scholar'

export interface CrawlerSearchRequest {
  query: string
  limit?: number
  max_retries?: number
  delay?: number
  source?: CrawlerSource
}

export interface Paper {
  title: string
  authors: string
  abstract: string
  year: string
  url: string
  pdfUrl: string
  arxivId?: string
  pmid?: string
  source: string
}

export interface CrawlerSearchResponse {
  success: boolean
  query: string
  source: string
  total: number
  retries: number
  papers: Paper[]
  error?: string
  status?: number
  message?: string
  info?: {
    api?: string
    documentation?: string
    note?: string
    alternatives?: string[]
    status: string
  }
}

/**
 * Search arXiv for papers (GET method)
 */
export function searchArXiv(params: {
  q: string
  limit?: number
  max_retries?: number
  delay?: number
}): Promise<CrawlerSearchResponse> {
  return request({
    url: '/crawler/arxiv',
    method: 'GET',
    params
  })
}

/**
 * Search PubMed for papers (GET method)
 * Note: Currently under development
 */
export function searchPubMed(params: {
  q: string
  limit?: number
}): Promise<CrawlerSearchResponse> {
  return request({
    url: '/crawler/pubmed',
    method: 'GET',
    params
  })
}

/**
 * Search Google Scholar for papers (GET method)
 * Note: Currently under development (no official API available)
 */
export function searchScholar(params: {
  q: string
  limit?: number
}): Promise<CrawlerSearchResponse> {
  return request({
    url: '/crawler/scholar',
    method: 'GET',
    params
  })
}

/**
 * Search arXiv for papers (POST method)
 */
export function searchCrawler(data: CrawlerSearchRequest): Promise<CrawlerSearchResponse> {
  return request({
    url: '/crawler/search',
    method: 'POST',
    data
  })
}

/**
 * Get crawler status and health
 */
export function getCrawlerStatus(): Promise<{ status: string; timestamp: string }> {
  return request({
    url: '/health',
    method: 'GET'
  })
}

/**
 * Save crawled papers to database
 */
export function savePapers(papers: Paper[]): Promise<{
  success: boolean
  message: string
  saved: number
  updated: number
  failed: number
  total: number
  source: string
}> {
  return request({
    url: '/crawler/save',
    method: 'POST',
    data: { papers }
  })
}

/**
 * Save single paper to database
 */
export function savePaper(paper: Paper): Promise<{
  success: boolean
  message: string
  saved: number
  updated: number
  failed: number
  total: number
  source: string
}> {
  return savePapers([paper])
}
