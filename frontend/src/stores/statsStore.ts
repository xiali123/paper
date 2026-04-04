/**
 * Statistics Store (Pinia)
 *
 * Manages statistics and analytics data including:
 * - Overview statistics
 * - Chart data (papers by year, journal, author)
 * - Reading progress
 * - Citation trends
 * - Real-time stats updates
 *
 * @module stores/statsStore
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type {
  Statistics,
  JournalStats,
  YearStats,
  AuthorStats
} from '@/types'

export interface ChartData {
  labels: string[]
  datasets: Array<{
    label: string
    data: number[]
    backgroundColor?: string | string[]
    borderColor?: string | string[]
    borderWidth?: number
  }>
}

export interface ReadingProgressStats {
  totalPapers: number
  readPapers: number
  unreadPapers: number
  inProgressPapers: number
  completionRate: number
  averageReadingTime: number
  totalReadingTime: number
}

export interface CitationTrends {
  byYear: Array<{
    year: string
    citations: number
    papers: number
    avgCitations: number
  }>
  topCited: Array<{
    paperId: number
    title: string
    citations: number
    year: string
  }>
}

export const useStatsStore = defineStore(
  'stats',
  () => {
    // ========================================================================
    // State
    // ========================================================================

    /** Overview statistics */
    const overview = ref<Statistics | null>(null)

    /** Journal statistics */
    const journals = ref<JournalStats[]>([])

    /** Year statistics */
    const years = ref<YearStats[]>([])

    /** Author statistics */
    const authors = ref<AuthorStats[]>([])

    /** Reading progress stats */
    const readingProgress = ref<ReadingProgressStats | null>(null)

    /** Citation trends */
    const citationTrends = ref<CitationTrends | null>(null)

    /** Loading state */
    const loading = ref(false)

    /** Error message */
    const error = ref<string | null>(null)

    /** Last update timestamp */
    const lastUpdate = ref<number>(0)

    /** Auto-refresh enabled */
    const autoRefresh = ref(true)

    /** Refresh interval in milliseconds */
    const refreshInterval = ref(60000) // 1 minute

    /** Timer for auto-refresh */
    let refreshTimer: ReturnType<typeof setInterval> | null = null

    // ========================================================================
    // Computed Properties
    // ========================================================================

    /** Papers by year chart data */
    const papersByYearChart = computed<ChartData>(() => {
      const sortedYears = [...years.value].sort((a, b) =>
        parseInt(a.year) - parseInt(b.year)
      )

      return {
        labels: sortedYears.map(y => y.year),
        datasets: [
          {
            label: 'CCF A',
            data: sortedYears.map(y => y.aCount || 0),
            backgroundColor: 'rgba(255, 99, 132, 0.6)',
            borderColor: 'rgba(255, 99, 132, 1)',
            borderWidth: 1
          },
          {
            label: 'CCF B',
            data: sortedYears.map(y => y.bCount || 0),
            backgroundColor: 'rgba(54, 162, 235, 0.6)',
            borderColor: 'rgba(54, 162, 235, 1)',
            borderWidth: 1
          },
          {
            label: 'CCF C',
            data: sortedYears.map(y => y.cCount || 0),
            backgroundColor: 'rgba(255, 206, 86, 0.6)',
            borderColor: 'rgba(255, 206, 86, 1)',
            borderWidth: 1
          }
        ]
      }
    })

    /** Papers by journal chart data */
    const papersByJournalChart = computed<ChartData>(() => {
      const topJournals = journals.value.slice(0, 10)

      return {
        labels: topJournals.map(j => j.journal),
        datasets: [
          {
            label: 'Papers',
            data: topJournals.map(j => j.count),
            backgroundColor: [
              'rgba(255, 99, 132, 0.6)',
              'rgba(54, 162, 235, 0.6)',
              'rgba(255, 206, 86, 0.6)',
              'rgba(75, 192, 192, 0.6)',
              'rgba(153, 102, 255, 0.6)',
              'rgba(255, 159, 64, 0.6)',
              'rgba(199, 199, 199, 0.6)',
              'rgba(83, 102, 255, 0.6)',
              'rgba(255, 99, 255, 0.6)',
              'rgba(99, 255, 132, 0.6)'
            ],
            borderWidth: 1
          }
        ]
      }
    })

    /** Papers by author chart data */
    const papersByAuthorChart = computed<ChartData>(() => {
      const topAuthors = authors.value.slice(0, 10)

      return {
        labels: topAuthors.map(a => a.author),
        datasets: [
          {
            label: 'Papers',
            data: topAuthors.map(a => a.count),
            backgroundColor: 'rgba(54, 162, 235, 0.6)',
            borderColor: 'rgba(54, 162, 235, 1)',
            borderWidth: 1
          }
        ]
      }
    })

    /** Reading progress chart data */
    const readingProgressChart = computed<ChartData>(() => {
      if (!readingProgress.value) {
        return { labels: [], datasets: [] }
      }

      return {
        labels: ['Read', 'In Progress', 'Unread'],
        datasets: [
          {
            label: 'Papers',
            data: [
              readingProgress.value.readPapers,
              readingProgress.value.inProgressPapers,
              readingProgress.value.unreadPapers
            ],
            backgroundColor: [
              'rgba(75, 192, 192, 0.6)',   // Green for read
              'rgba(255, 206, 86, 0.6)',   // Yellow for in progress
              'rgba(255, 99, 132, 0.6)'    // Red for unread
            ],
            borderWidth: 1
          }
        ]
      }
    })

    /** Citation trends chart data */
    const citationTrendsChart = computed<ChartData>(() => {
      if (!citationTrends.value) {
        return { labels: [], datasets: [] }
      }

      const sortedByYear = [...citationTrends.value.byYear].sort((a, b) =>
        parseInt(a.year) - parseInt(b.year)
      )

      return {
        labels: sortedByYear.map(t => t.year),
        datasets: [
          {
            label: 'Total Citations',
            data: sortedByYear.map(t => t.citations),
            backgroundColor: 'rgba(54, 162, 235, 0.2)',
            borderColor: 'rgba(54, 162, 235, 1)',
            borderWidth: 2
          },
          {
            label: 'Average Citations',
            data: sortedByYear.map(t => Math.round(t.avgCitations)),
            backgroundColor: 'rgba(255, 99, 132, 0.2)',
            borderColor: 'rgba(255, 99, 132, 1)',
            borderWidth: 2
          }
        ]
      }
    })

    /** CCF level distribution */
    const ccfDistribution = computed(() => {
      if (!overview.value) return []

      return [
        { level: 'CCF A', count: overview.value.topTierPapers, percentage: 0 },
        { level: 'CCF B', count: 0, percentage: 0 },
        { level: 'CCF C', count: 0, percentage: 0 }
      ]
    })

    /** Growth rate */
    const growthRate = computed(() => {
      if (years.value.length < 2) return 0

      const sorted = [...years.value].sort((a, b) =>
        parseInt(a.year) - parseInt(b.year)
      )

      const latest = sorted[sorted.length - 1]
      const previous = sorted[sorted.length - 2]

      if (!latest || !previous || previous.count === 0) return 0

      return ((latest.count - previous.count) / previous.count) * 100
    })

    /** Is stale data */
    const isStale = computed(() => {
      if (lastUpdate.value === 0) return true
      const staleTime = 5 * 60 * 1000 // 5 minutes
      return Date.now() - lastUpdate.value > staleTime
    })

    // ========================================================================
    // Actions
    // ========================================================================

    /**
     * Fetch all statistics
     */
    async function fetchAllStats() {
      loading.value = true
      error.value = null

      try {
        // Fetch all stats in parallel
        await Promise.all([
          fetchOverview(),
          fetchJournals(),
          fetchYears(),
          fetchAuthors(),
          fetchReadingProgress(),
          fetchCitationTrends()
        ])

        lastUpdate.value = Date.now()
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch statistics'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch overview statistics
     */
    async function fetchOverview() {
      try {
        // TODO: Implement actual API call
        // const data = await statsApi.getOverview()
        // overview.value = data

        // Mock data for now
        overview.value = {
          totalPapers: 1234,
          totalJournals: 56,
          topTierPapers: 234,
          papersLastYear: 89,
          mostActiveJournal: 'CVPR',
          yearRange: '2015-2024'
        }
      } catch (err: any) {
        console.error('Failed to fetch overview:', err)
        throw err
      }
    }

    /**
     * Fetch journal statistics
     */
    async function fetchJournals() {
      try {
        // TODO: Implement actual API call
        // const data = await statsApi.getJournals()
        // journals.value = data

        // Mock data for now
        journals.value = [
          { journal: 'CVPR', count: 45, level: 'A' },
          { journal: 'ICCV', count: 38, level: 'A' },
          { journal: 'ECCV', count: 32, level: 'A' },
          { journal: 'NeurIPS', count: 56, level: 'A' },
          { journal: 'ICML', count: 42, level: 'A' }
        ]
      } catch (err: any) {
        console.error('Failed to fetch journal stats:', err)
        throw err
      }
    }

    /**
     * Fetch year statistics
     */
    async function fetchYears() {
      try {
        // TODO: Implement actual API call
        // const data = await statsApi.getYears()
        // years.value = data

        // Mock data for now
        years.value = [
          { year: '2020', count: 120, aCount: 45, bCount: 50, cCount: 25 },
          { year: '2021', count: 145, aCount: 55, bCount: 60, cCount: 30 },
          { year: '2022', count: 168, aCount: 65, bCount: 70, cCount: 33 },
          { year: '2023', count: 189, aCount: 75, bCount: 80, cCount: 34 },
          { year: '2024', count: 89, aCount: 35, bCount: 36, cCount: 18 }
        ]
      } catch (err: any) {
        console.error('Failed to fetch year stats:', err)
        throw err
      }
    }

    /**
     * Fetch author statistics
     */
    async function fetchAuthors() {
      try {
        // TODO: Implement actual API call
        // const data = await statsApi.getAuthors()
        // authors.value = data

        // Mock data for now
        authors.value = [
          { author: 'John Doe', count: 12, totalCitations: 234 },
          { author: 'Jane Smith', count: 10, totalCitations: 189 },
          { author: 'Bob Johnson', count: 8, totalCitations: 156 },
          { author: 'Alice Williams', count: 7, totalCitations: 143 },
          { author: 'Charlie Brown', count: 6, totalCitations: 98 }
        ]
      } catch (err: any) {
        console.error('Failed to fetch author stats:', err)
        throw err
      }
    }

    /**
     * Fetch reading progress
     */
    async function fetchReadingProgress() {
      try {
        // TODO: Implement actual API call
        // const data = await statsApi.getReadingProgress()
        // readingProgress.value = data

        // Mock data for now
        readingProgress.value = {
          totalPapers: 500,
          readPapers: 234,
          unreadPapers: 200,
          inProgressPapers: 66,
          completionRate: 46.8,
          averageReadingTime: 45,
          totalReadingTime: 10530
        }
      } catch (err: any) {
        console.error('Failed to fetch reading progress:', err)
        throw err
      }
    }

    /**
     * Fetch citation trends
     */
    async function fetchCitationTrends() {
      try {
        // TODO: Implement actual API call
        // const data = await statsApi.getCitationTrends()
        // citationTrends.value = data

        // Mock data for now
        citationTrends.value = {
          byYear: [
            { year: '2020', citations: 1234, papers: 120, avgCitations: 10.28 },
            { year: '2021', citations: 1567, papers: 145, avgCitations: 10.81 },
            { year: '2022', citations: 1890, papers: 168, avgCitations: 11.25 },
            { year: '2023', citations: 2234, papers: 189, avgCitations: 11.82 },
            { year: '2024', citations: 890, papers: 89, avgCitations: 10.0 }
          ],
          topCited: [
            { paperId: 1, title: 'Deep Learning', citations: 234, year: '2020' },
            { paperId: 2, title: 'Attention Is All You Need', citations: 189, year: '2021' },
            { paperId: 3, title: 'BERT', citations: 156, year: '2022' }
          ]
        }
      } catch (err: any) {
        console.error('Failed to fetch citation trends:', err)
        throw err
      }
    }

    /**
     * Refresh statistics
     */
    async function refresh() {
      await fetchAllStats()
    }

    /**
     * Enable auto-refresh
     */
    function enableAutoRefresh() {
      autoRefresh.value = true
      setupAutoRefresh()
    }

    /**
     * Disable auto-refresh
     */
    function disableAutoRefresh() {
      autoRefresh.value = false
      if (refreshTimer) {
        clearInterval(refreshTimer)
        refreshTimer = null
      }
    }

    /**
     * Set refresh interval
     */
    function setRefreshInterval(interval: number) {
      refreshInterval.value = interval
      if (autoRefresh.value) {
        setupAutoRefresh()
      }
    }

    /**
     * Reset state
     */
    function reset() {
      overview.value = null
      journals.value = []
      years.value = []
      authors.value = []
      readingProgress.value = null
      citationTrends.value = null
      loading.value = false
      error.value = null
      lastUpdate.value = 0
    }

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * Setup auto-refresh timer
     */
    function setupAutoRefresh() {
      if (refreshTimer) {
        clearInterval(refreshTimer)
      }

      if (autoRefresh.value) {
        refreshTimer = setInterval(() => {
          refresh()
        }, refreshInterval.value)
      }
    }

    // Initialize auto-refresh if enabled
    if (autoRefresh.value) {
      setupAutoRefresh()
    }

    // ========================================================================
    // Return
    // ========================================================================

    return {
      // State
      overview,
      journals,
      years,
      authors,
      readingProgress,
      citationTrends,
      loading,
      error,
      lastUpdate,
      autoRefresh,
      refreshInterval,

      // Computed
      papersByYearChart,
      papersByJournalChart,
      papersByAuthorChart,
      readingProgressChart,
      citationTrendsChart,
      ccfDistribution,
      growthRate,
      isStale,

      // Actions
      fetchAllStats,
      fetchOverview,
      fetchJournals,
      fetchYears,
      fetchAuthors,
      fetchReadingProgress,
      fetchCitationTrends,
      refresh,
      enableAutoRefresh,
      disableAutoRefresh,
      setRefreshInterval,
      reset
    }
  }
)
