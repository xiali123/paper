/**
 * Recommendation Store (Pinia)
 *
 * Manages recommendation system including:
 * - Personalized recommendations
 * - Similar papers
 * - Trending papers
 * - User feedback and preferences
 *
 * @module stores/recommendationStore
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { Paper } from '@/api/modules/papers'
import type { RecommendationResult, UserProfile, RecommendationFeedback } from '@/types'

export interface RecommendationItem extends RecommendationResult {
  paper: Paper
  score: number
  reason: string
}

export interface SimilarPaperItem {
  paper: Paper
  similarity: number
  commonKeywords: string[]
  commonAuthors: string[]
}

export interface TrendingPaperItem {
  paper: Paper
  trend: 'rising' | 'stable' | 'declining'
  views: number
  citations: number
  period: string
}

export const useRecommendationStore = defineStore(
  'recommendation',
  () => {
    // ========================================================================
    // State
    // ========================================================================

    /** Personalized recommendations */
    const personalized = ref<RecommendationItem[]>([])

    /** Similar papers (for a given paper) */
    const similarPapers = ref<SimilarPaperItem[]>([])

    /** Trending papers */
    const trending = ref<TrendingPaperItem[]>([])

    /** User profile for recommendations */
    const userProfile = ref<UserProfile | null>(null)

    /** Feedback history */
    const feedbackHistory = ref<RecommendationFeedback[]>([])

    /** Loading state */
    const loading = ref(false)

    /** Error message */
    const error = ref<string | null>(null)

    /** Last update timestamp */
    const lastUpdate = ref<number>(0)

    /** Refresh interval (milliseconds) */
    const refreshInterval = 24 * 60 * 60 * 1000 // 24 hours

    // ========================================================================
    // Computed Properties
    // ========================================================================

    /** Has personalized recommendations */
    const hasPersonalized = computed(() => personalized.value.length > 0)

    /** Has similar papers */
    const hasSimilarPapers = computed(() => similarPapers.value.length > 0)

    /** Has trending papers */
    const hasTrending = computed(() => trending.value.length > 0)

    /** Top recommendations (score > 0.8) */
    const topRecommendations = computed(() =>
      personalized.value.filter(r => r.score > 0.8).slice(0, 10)
    )

    /** Highly cited recommendations */
    const highlyCited = computed(() =>
      [...personalized.value]
        .sort((a, b) => (b.paper.citations || 0) - (a.paper.citations || 0))
        .slice(0, 10)
    )

    /** Recent recommendations */
    const recentRecommendations = computed(() =>
      personalized.value.slice(0, 20)
    )

    /** Rising trending papers */
    const risingPapers = computed(() =>
      trending.value.filter(t => t.trend === 'rising')
    )

    /** Feedback by type */
    const feedbackByType = computed(() => {
      const byType: Record<string, number> = {
        helpful: 0,
        not_helpful: 0,
        irrelevant: 0,
        saved: 0
      }

      feedbackHistory.value.forEach(f => {
        byType[f.feedbackType]++
      })

      return byType
    })

    /** Is stale data */
    const isStale = computed(() => {
      if (lastUpdate.value === 0) return true
      return Date.now() - lastUpdate.value > refreshInterval
    })

    // ========================================================================
    // Actions
    // ========================================================================

    /**
     * Fetch personalized recommendations
     */
    async function fetchPersonalized(limit = 20) {
      loading.value = true
      error.value = null

      try {
        // TODO: Implement actual API call
        // const response = await recommendationApi.getPersonalized({ limit })
        // personalized.value = response.recommendations

        // Mock data for now
        personalized.value = []
        lastUpdate.value = Date.now()

        return personalized.value
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch recommendations'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch similar papers
     */
    async function fetchSimilar(paperId: number, limit = 10) {
      loading.value = true
      error.value = null

      try {
        // TODO: Implement actual API call
        // const response = await recommendationApi.getSimilar(paperId, { limit })
        // similarPapers.value = response.similarPapers

        // Mock data for now
        similarPapers.value = []
        return similarPapers.value
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch similar papers'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch trending papers
     */
    async function fetchTrending(period = 'week', limit = 20) {
      loading.value = true
      error.value = null

      try {
        // TODO: Implement actual API call
        // const response = await recommendationApi.getTrending({ period, limit })
        // trending.value = response.trending

        // Mock data for now
        trending.value = []
        return trending.value
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch trending papers'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch user profile
     */
    async function fetchUserProfile() {
      loading.value = true
      error.value = null

      try {
        // TODO: Implement actual API call
        // const profile = await recommendationApi.getProfile()
        // userProfile.value = profile

        // Mock data for now
        userProfile.value = {
          interests: ['Machine Learning', 'Computer Vision', 'NLP'],
          preferredJournals: ['CVPR', 'ICCV', 'NeurIPS'],
          preferredAuthors: ['Geoffrey Hinton', 'Yann LeCun'],
          readingHistory: [1, 2, 3, 4, 5],
          bookmarkedPapers: [1, 2, 3],
          feedbackHistory: feedbackHistory.value
        }

        return userProfile.value
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch user profile'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Submit feedback
     */
    async function submitFeedback(
      recommendationId: string,
      feedbackType: 'helpful' | 'not_helpful' | 'irrelevant' | 'saved',
      comment?: string
    ) {
      loading.value = true
      error.value = null

      try {
        const feedback: RecommendationFeedback = {
          id: `feedback_${Date.now()}`,
          recommendationId,
          feedbackType,
          comment,
          timestamp: Date.now()
        }

        // TODO: Implement actual API call
        // await recommendationApi.submitFeedback(feedback)

        // Add to history
        feedbackHistory.value.push(feedback)
        persistFeedback()

        return feedback
      } catch (err: any) {
        error.value = err.message || 'Failed to submit feedback'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Update user interests
     */
    async function updateInterests(interests: string[]) {
      loading.value = true
      error.value = null

      try {
        if (!userProfile.value) {
          userProfile.value = {} as UserProfile
        }

        userProfile.value.interests = interests

        // TODO: Implement actual API call
        // await recommendationApi.updateProfile({ interests })

        persistProfile()
      } catch (err: any) {
        error.value = err.message || 'Failed to update interests'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Add paper to reading history
     */
    function addToReadingHistory(paperId: number) {
      if (!userProfile.value) {
        userProfile.value = {} as UserProfile
      }

      if (!userProfile.value.readingHistory) {
        userProfile.value.readingHistory = []
      }

      // Remove if already exists
      userProfile.value.readingHistory = userProfile.value.readingHistory.filter(id => id !== paperId)

      // Add to beginning
      userProfile.value.readingHistory.unshift(paperId)

      // Limit history size
      if (userProfile.value.readingHistory.length > 100) {
        userProfile.value.readingHistory = userProfile.value.readingHistory.slice(0, 100)
      }

      persistProfile()
    }

    /**
     * Add paper to bookmarks
     */
    function addBookmark(paperId: number) {
      if (!userProfile.value) {
        userProfile.value = {} as UserProfile
      }

      if (!userProfile.value.bookmarkedPapers) {
        userProfile.value.bookmarkedPapers = []
      }

      if (!userProfile.value.bookmarkedPapers.includes(paperId)) {
        userProfile.value.bookmarkedPapers.push(paperId)
        persistProfile()
      }
    }

    /**
     * Remove paper from bookmarks
     */
    function removeBookmark(paperId: number) {
      if (userProfile.value?.bookmarkedPapers) {
        userProfile.value.bookmarkedPapers = userProfile.value.bookmarkedPapers.filter(id => id !== paperId)
        persistProfile()
      }
    }

    /**
     * Clear similar papers
     */
    function clearSimilarPapers() {
      similarPapers.value = []
    }

    /**
     * Refresh recommendations
     */
    async function refresh() {
      await Promise.all([
        fetchPersonalized(),
        fetchTrending()
      ])
    }

    /**
     * Reset state
     */
    function reset() {
      personalized.value = []
      similarPapers.value = []
      trending.value = []
      userProfile.value = null
      feedbackHistory.value = []
      loading.value = false
      error.value = null
      lastUpdate.value = 0
    }

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * Persist profile to localStorage
     */
    function persistProfile() {
      try {
        if (userProfile.value) {
          localStorage.setItem('recommendation-profile', JSON.stringify(userProfile.value))
        }
      } catch (err) {
        console.error('Failed to persist profile:', err)
      }
    }

    /**
     * Load profile from localStorage
     */
    function loadProfile() {
      try {
        const stored = localStorage.getItem('recommendation-profile')
        if (stored) {
          userProfile.value = JSON.parse(stored)
        }
      } catch (err) {
        console.error('Failed to load profile:', err)
      }
    }

    /**
     * Persist feedback to localStorage
     */
    function persistFeedback() {
      try {
        localStorage.setItem('recommendation-feedback', JSON.stringify(feedbackHistory.value))
      } catch (err) {
        console.error('Failed to persist feedback:', err)
      }
    }

    /**
     * Load feedback from localStorage
     */
    function loadFeedback() {
      try {
        const stored = localStorage.getItem('recommendation-feedback')
        if (stored) {
          feedbackHistory.value = JSON.parse(stored)
        }
      } catch (err) {
        console.error('Failed to load feedback:', err)
      }
    }

    // Initialize on store creation
    loadProfile()
    loadFeedback()

    // ========================================================================
    // Return
    // ========================================================================

    return {
      // State
      personalized,
      similarPapers,
      trending,
      userProfile,
      feedbackHistory,
      loading,
      error,
      lastUpdate,

      // Computed
      hasPersonalized,
      hasSimilarPapers,
      hasTrending,
      topRecommendations,
      highlyCited,
      recentRecommendations,
      risingPapers,
      feedbackByType,
      isStale,

      // Actions
      fetchPersonalized,
      fetchSimilar,
      fetchTrending,
      fetchUserProfile,
      submitFeedback,
      updateInterests,
      addToReadingHistory,
      addBookmark,
      removeBookmark,
      clearSimilarPapers,
      refresh,
      reset
    }
  },
  {
    persist: {
      key: 'recommendation-store',
      storage: localStorage,
      paths: ['userProfile']
    }
  }
)
