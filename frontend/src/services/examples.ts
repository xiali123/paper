/**
 * PaperCrawler API Service Usage Examples
 *
 * This file contains comprehensive examples demonstrating how to use
 * all API service modules in real-world scenarios.
 */

import {
  api,
  authApi,
  userApi,
  paperApi,
  searchApi,
  exportApi,
  statsApi,
  aiApi,
  recommendationApi,
  crawlerApi
} from '@/services'
import type {
  Paper,
  PaperQuery,
  SearchQuery,
  CreatePaperRequest
} from '@/types/api'

// ============================================================================
// AUTHENTICATION EXAMPLES
// ============================================================================

/**
 * Example 1: User Login Flow
 */
export async function loginExample() {
  try {
    // Login user
    const response = await authApi.login({
      username: 'user@example.com',
      password: 'securepassword',
      rememberMe: true
    })

    const { accessToken, refreshToken, user } = response.data

    // Store tokens (auto-handled by authApi)
    console.log('Login successful', user)
    return user
  } catch (error) {
    console.error('Login failed:', error)
    throw error
  }
}

/**
 * Example 2: User Registration
 */
export async function registerExample() {
  try {
    const response = await authApi.register({
      username: 'newuser@example.com',
      email: 'newuser@example.com',
      password: 'securepassword',
      fullName: 'John Doe'
    })

    console.log('Registration successful', response.data)
    return response.data
  } catch (error) {
    console.error('Registration failed:', error)
    throw error
  }
}

/**
 * Example 3: Token Refresh (auto-handled by interceptors)
 */
export async function tokenRefreshExample() {
  // Token refresh is automatically handled by axios interceptors
  // when a 401 error occurs. This is just for demonstration.

  try {
    const response = await authApi.refreshToken({
      refreshToken: localStorage.getItem('refresh_token') || ''
    })

    const { accessToken } = response.data
    console.log('Token refreshed successfully')
    return accessToken
  } catch (error) {
    console.error('Token refresh failed:', error)
    throw error
  }
}

// ============================================================================
// USER MANAGEMENT EXAMPLES
// ============================================================================

/**
 * Example 4: Get Current User Profile
 */
export async function getCurrentUserExample() {
  try {
    const response = await userApi.getMe()
    const user = response.data

    console.log('Current user:', user)
    return user
  } catch (error) {
    console.error('Failed to get user:', error)
    throw error
  }
}

/**
 * Example 5: Update User Profile
 */
export async function updateProfileExample() {
  try {
    const response = await userApi.updateMe({
      fullName: 'John Updated Doe',
      avatar: 'https://example.com/avatar.jpg'
    })

    console.log('Profile updated:', response.data)
    return response.data
  } catch (error) {
    console.error('Profile update failed:', error)
    throw error
  }
}

/**
 * Example 6: Get User Statistics
 */
export async function getUserStatsExample() {
  try {
    const response = await userApi.getStats()
    const stats = response.data

    console.log('User stats:', {
      totalPapers: stats.totalPapers,
      readPapers: stats.readPapers,
      bookmarkedPapers: stats.bookmarkedPapers
    })

    return stats
  } catch (error) {
    console.error('Failed to get stats:', error)
    throw error
  }
}

// ============================================================================
// PAPER MANAGEMENT EXAMPLES
// ============================================================================

/**
 * Example 7: Get All Papers with Filters
 */
export async function getPapersExample() {
  try {
    const params: PaperQuery = {
      page: 1,
      pageSize: 20,
      sortBy: 'createdAt',
      sortOrder: 'desc',
      search: 'machine learning',
      year: 2024,
      isRead: false,
      isBookmarked: true
    }

    const response = await paperApi.getAll(params)
    const { items, total, page, pageSize } = response.data

    console.log(`Found ${total} papers, showing page ${page}`)
    return items
  } catch (error) {
    console.error('Failed to get papers:', error)
    throw error
  }
}

/**
 * Example 8: Create New Paper
 */
export async function createPaperExample() {
  try {
    const paperData: CreatePaperRequest = {
      title: 'Deep Learning for Natural Language Processing',
      authors: ['John Doe', 'Jane Smith', 'Bob Johnson'],
      abstract: 'This paper presents a novel approach...',
      year: 2024,
      publication: 'Journal of AI Research',
      volume: '15',
      issue: '2',
      pages: '123-145',
      doi: '10.1234/example.doi',
      url: 'https://example.com/paper',
      tags: ['AI', 'Deep Learning', 'NLP'],
      keywords: ['deep learning', 'natural language processing', 'neural networks']
    }

    const response = await paperApi.create(paperData)
    console.log('Paper created:', response.data)
    return response.data
  } catch (error) {
    console.error('Failed to create paper:', error)
    throw error
  }
}

/**
 * Example 9: Update Paper Reading Progress
 */
export async function updateReadingProgressExample(paperId: number) {
  try {
    const response = await paperApi.update(paperId, {
      isRead: true,
      readingProgress: 100
    })

    console.log('Paper updated:', response.data)
    return response.data
  } catch (error) {
    console.error('Failed to update paper:', error)
    throw error
  }
}

/**
 * Example 10: Toggle Paper Bookmark
 */
export async function toggleBookmarkExample(paperId: number) {
  try {
    // First check if paper is bookmarked
    const paperResponse = await paperApi.getById(paperId)
    const paper = paperResponse.data

    if (paper.isBookmarked) {
      await paperApi.removeFavorite(paperId)
      console.log('Bookmark removed')
    } else {
      await paperApi.addFavorite(paperId)
      console.log('Bookmark added')
    }

    // Refresh paper data
    const updatedResponse = await paperApi.getById(paperId)
    return updatedResponse.data
  } catch (error) {
    console.error('Failed to toggle bookmark:', error)
    throw error
  }
}

// ============================================================================
// SEARCH EXAMPLES
// ============================================================================

/**
 * Example 11: Simple Search
 */
export async function simpleSearchExample() {
  try {
    const params: SearchQuery = {
      q: 'machine learning neural networks',
      page: 1,
      pageSize: 10
    }

    const response = await searchApi.search(params)
    const { items, total } = response.data

    console.log(`Found ${total} results`)
    return items
  } catch (error) {
    console.error('Search failed:', error)
    throw error
  }
}

/**
 * Example 12: Advanced Search
 */
export async function advancedSearchExample() {
  try {
    const response = await searchApi.advancedSearch({
      title: 'deep learning',
      authors: ['John Doe'],
      abstract: 'neural networks',
      yearRange: { from: 2020, to: 2024 },
      tags: ['AI', 'machine learning'],
      operator: 'AND',
      page: 1,
      pageSize: 20
    })

    console.log('Advanced search results:', response.data.items)
    return response.data.items
  } catch (error) {
    console.error('Advanced search failed:', error)
    throw error
  }
}

/**
 * Example 13: Get Search Suggestions
 */
export async function searchSuggestionsExample() {
  try {
    const response = await searchApi.suggest({
      q: 'machine learning',
      limit: 5
    })

    console.log('Suggestions:', response.data)
    return response.data
  } catch (error) {
    console.error('Failed to get suggestions:', error)
    throw error
  }
}

// ============================================================================
// EXPORT EXAMPLES
// ============================================================================

/**
 * Example 14: Export Papers to BibTeX
 */
export async function exportPapersExample() {
  try {
    const response = await exportApi.createExport({
      format: 'bibtex',
      filters: {
        tags: ['AI', 'machine learning'],
        year: 2024
      },
      includeAbstract: true,
      includeNotes: true,
      filename: 'ai_papers_2024.bib'
    })

    const exportJob = response.data
    console.log('Export job created:', exportJob)

    // Wait for export to complete (polling)
    while (exportJob.status !== 'completed') {
      await new Promise(resolve => setTimeout(resolve, 1000))
      const statusResponse = await exportApi.getById(exportJob.id)
      Object.assign(exportJob, statusResponse.data)
    }

    // Download exported file
    const blob = await exportApi.download(exportJob.id)
    const url = URL.createObjectURL(blob)

    // Trigger download
    const a = document.createElement('a')
    a.href = url
    a.download = exportJob.filename || 'export.bib'
    a.click()

    console.log('Export downloaded successfully')
    return exportJob
  } catch (error) {
    console.error('Export failed:', error)
    throw error
  }
}

// ============================================================================
// STATISTICS EXAMPLES
// ============================================================================

/**
 * Example 15: Get System Statistics
 */
export async function getSystemStatsExample() {
  try {
    const response = await statsApi.getSystemStats()
    console.log('System stats:', response.data)
    return response.data
  } catch (error) {
    console.error('Failed to get system stats:', error)
    throw error
  }
}

/**
 * Example 16: Get Resource Usage
 */
export async function getResourceStatsExample() {
  try {
    const response = await statsApi.getResourceStats()
    const { cpu, memory, disk } = response.data

    console.log('CPU Usage:', cpu.usage)
    console.log('Memory Usage:', memory.percentage)
    console.log('Disk Usage:', disk.percentage)

    return response.data
  } catch (error) {
    console.error('Failed to get resource stats:', error)
    throw error
  }
}

// ============================================================================
// AI FEATURES EXAMPLES
// ============================================================================

/**
 * Example 17: Generate AI Review
 */
export async function generateAIReviewExample(paperId: number) {
  try {
    const response = await aiApi.summarize({
      paperId,
      criteria: ['methodology', 'results', 'conclusions']
    })

    const review = response.data
    console.log('AI Review generated:', review.overall.summary)

    return review
  } catch (error) {
    console.error('Failed to generate AI review:', error)
    throw error
  }
}

/**
 * Example 18: Generate Literature Review
 */
export async function generateLiteratureReviewExample() {
  try {
    const response = await aiApi.chat({
      topic: 'Deep Learning in Natural Language Processing',
      maxPapers: 20,
      yearRange: { from: 2020, to: 2024 },
      sections: ['Introduction', 'Methodology', 'Applications', 'Future Work']
    })

    console.log('Literature review generated:', response.data.review)
    return response.data
  } catch (error) {
    console.error('Failed to generate literature review:', error)
    throw error
  }
}

/**
 * Example 19: Extract Keywords
 */
export async function extractKeywordsExample(paperId: number) {
  try {
    const response = await aiApi.extractKeywords({
      paperId,
      maxKeywords: 10
    })

    const keywords = response.data.keywords
    console.log('Extracted keywords:', keywords)

    return keywords
  } catch (error) {
    console.error('Failed to extract keywords:', error)
    throw error
  }
}

// ============================================================================
// RECOMMENDATION EXAMPLES
// ============================================================================

/**
 * Example 20: Get Personalized Recommendations
 */
export async function getRecommendationsExample() {
  try {
    const response = await recommendationApi.getPapers({
      limit: 10,
      excludeRead: false,
      excludeBookmarked: false
    })

    console.log('Recommendations:', response.data)
    return response.data
  } catch (error) {
    console.error('Failed to get recommendations:', error)
    throw error
  }
}

/**
 * Example 21: Get Trending Papers
 */
export async function getTrendingPapersExample() {
  try {
    const response = await recommendationApi.getTrending({
      period: 'week',
      limit: 20
    })

    console.log('Trending papers:', response.data)
    return response.data
  } catch (error) {
    console.error('Failed to get trending papers:', error)
    throw error
  }
}

// ============================================================================
// CRAWLER EXAMPLES
// ============================================================================

/**
 * Example 22: Create Crawler Template
 */
export async function createCrawlerTemplateExample() {
  try {
    const response = await crawlerApi.createTemplate({
      name: 'ArXiv AI Papers',
      description: 'Crawl AI-related papers from ArXiv',
      source: 'arxiv',
      sourceType: 'arxiv',
      config: {
        baseUrl: 'https://arxiv.org',
        searchPath: '/search/',
        maxPapers: 100,
        delay: 1000,
        retryAttempts: 3,
        timeout: 30000,
        fields: [
          {
            name: 'title',
            selector: 'h1.title',
            required: true
          },
          {
            name: 'authors',
            selector: 'div.authors',
            required: true
          }
        ]
      },
      isActive: true
    })

    console.log('Crawler template created:', response.data)
    return response.data
  } catch (error) {
    console.error('Failed to create template:', error)
    throw error
  }
}

/**
 * Example 23: Create and Run Crawler Task
 */
export async function runCrawlerTaskExample() {
  try {
    // Create task
    const response = await crawlerApi.createTask({
      templateId: 1,
      name: 'Daily AI Papers Crawl',
      maxPapers: 50,
      searchQuery: 'artificial intelligence',
      yearRange: { from: 2024, to: 2024 }
    })

    const task = response.data
    console.log('Crawler task created:', task)

    // Monitor progress (polling)
    while (task.status === 'running' || task.status === 'pending') {
      await new Promise(resolve => setTimeout(resolve, 2000))

      const statusResponse = await crawlerApi.getTaskById(task.id)
      Object.assign(task, statusResponse.data)

      console.log(`Progress: ${task.progress}% (${task.crawledPapers}/${task.totalPapers})`)
    }

    console.log('Crawler task completed:', task)
    return task
  } catch (error) {
    console.error('Failed to run crawler task:', error)
    throw error
  }
}

/**
 * Example 24: Get Crawler Statistics
 */
export async function getCrawlerStatsExample() {
  try {
    const response = await crawlerApi.getStats()
    console.log('Crawler stats:', response.data)
    return response.data
  } catch (error) {
    console.error('Failed to get crawler stats:', error)
    throw error
  }
}

// ============================================================================
// COMPLEX WORKFLOWS
// ============================================================================

/**
 * Example 25: Complete Paper Management Workflow
 */
export async function completePaperWorkflow() {
  try {
    // Step 1: Search for papers
    const searchResults = await simpleSearchExample()
    console.log(`Found ${searchResults.length} papers`)

    // Step 2: Get details of first paper
    const firstPaper = searchResults[0].paper
    const paperDetails = await paperApi.getById(firstPaper.id)
    console.log('Paper details:', paperDetails.data)

    // Step 3: Bookmark the paper
    await paperApi.addFavorite(firstPaper.id)
    console.log('Paper bookmarked')

    // Step 4: Get AI review
    const review = await generateAIReviewExample(firstPaper.id)
    console.log('AI review generated')

    // Step 5: Update reading progress
    await updateReadingProgressExample(firstPaper.id)
    console.log('Reading progress updated')

    // Step 6: Get recommendations based on this paper
    const recommendations = await recommendationApi.getPapers({ limit: 5 })
    console.log('Got recommendations')

    return {
      paper: paperDetails.data,
      review,
      recommendations: recommendations.data
    }
  } catch (error) {
    console.error('Workflow failed:', error)
    throw error
  }
}

/**
 * Example 26: Batch Paper Import and Export
 */
export async function batchImportExportWorkflow() {
  try {
    // Step 1: Create papers in batch
    const papersToCreate: CreatePaperRequest[] = [
      {
        title: 'Paper 1',
        authors: ['Author 1'],
        year: 2024
      },
      {
        title: 'Paper 2',
        authors: ['Author 2'],
        year: 2024
      }
    ]

    const batchResponse = await paperApi.batchCreate(papersToCreate)
    console.log('Batch create completed:', batchResponse.data)

    // Step 2: Export all papers
    const exportResponse = await exportApi.createExport({
      format: 'csv',
      includeAbstract: true,
      includeNotes: true
    })

    console.log('Export job created:', exportResponse.data)

    // Step 3: Download export (when ready)
    // Wait and download logic as shown in exportPapersExample()

    return exportResponse.data
  } catch (error) {
    console.error('Batch workflow failed:', error)
    throw error
  }
}

/**
 * Example 27: Real-time Crawler Monitoring
 */
export async function monitorCrawlerWorkflow() {
  try {
    // Start crawler
    const taskResponse = await crawlerApi.createTask({
      templateId: 1,
      name: 'Monitoring Example',
      maxPapers: 20
    })

    const task = taskResponse.data

    // Set up monitoring interval
    const monitoringInterval = setInterval(async () => {
      try {
        const statusResponse = await crawlerApi.getTaskById(task.id)
        const currentTask = statusResponse.data

        console.log(`Task ${currentTask.id}: ${currentTask.status}`)
        console.log(`Progress: ${currentTask.progress}%`)
        console.log(`Papers: ${currentTask.crawledPapers}/${currentTask.totalPapers}`)

        if (currentTask.status !== 'running' && currentTask.status !== 'pending') {
          clearInterval(monitoringInterval)
          console.log('Task completed!')
        }
      } catch (error) {
        console.error('Monitoring error:', error)
        clearInterval(monitoringInterval)
      }
    }, 3000) // Check every 3 seconds

    return task
  } catch (error) {
    console.error('Failed to monitor crawler:', error)
    throw error
  }
}

// ============================================================================
// ERROR HANDLING EXAMPLES
// ============================================================================

/**
 * Example 28: Proper Error Handling
 */
export async function errorHandlingExample() {
  try {
    const response = await paperApi.getById(999)

    if (!response.data) {
      throw new Error('Paper not found')
    }

    return response.data
  } catch (error: any) {
    // Error is automatically handled by axios interceptor
    // and displayed to user via ElMessage

    // Additional error handling if needed
    if (error.response?.status === 404) {
      console.error('Paper not found')
      // Handle 404 specifically
    } else if (error.response?.status === 401) {
      console.error('Unauthorized')
      // Handle 401 - redirect to login
    } else {
      console.error('Unexpected error:', error.message)
    }

    throw error
  }
}

/**
 * Example 29: Request Cancellation
 */
export async function requestCancellationExample() {
  const controller = new AbortController()

  try {
    // Make request with abort signal
    const response = await axiosInstance.get('/api/papers', {
      signal: controller.signal
    })

    return response.data
  } catch (error: any) {
    if (error.name === 'CanceledError') {
      console.log('Request was cancelled')
    } else {
      console.error('Request failed:', error)
    }
    throw error
  } finally {
    // Cancel request if component unmounts
    controller.abort()
  }
}
