/**
 * @file crawler.ts
 * @brief Crawler API module for fetching papers from academic sources
 */

import { request } from '@/utils/request'

export interface CrawlerSearchRequest {
  query: string
  limit?: number
  max_retries?: number
  delay?: number
}

export interface Paper {
  title: string
  authors: string
  abstract: string
  year: string
  url: string
  pdfUrl: string
  arxivId: string
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
    url: '/api/crawler/arxiv',
    method: 'GET',
    params
  })
}

/**
 * Search arXiv for papers (POST method)
 */
export function searchCrawler(data: CrawlerSearchRequest): Promise<CrawlerSearchResponse> {
  return request({
    url: '/api/crawler/search',
    method: 'POST',
    data
  })
}

/**
 * Get crawler status and health
 */
export function getCrawlerStatus(): Promise<{ status: string; timestamp: string }> {
  return request({
    url: '/api/health',
    method: 'GET'
  })
}
