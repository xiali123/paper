/**
 * Recommendations Store
 * 推荐系统状态管理
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { recommendationsApi } from '@/api/modules/recommendations'
import type { RecommendationResult, UserProfile } from '@/types/recommendation'
import type { Paper } from '@/types'

export const useRecommendationsStore = defineStore('recommendations', () => {
  // State
  const personalized = ref<RecommendationResult[]>([])
  const similar = ref<Paper[]>([])
  const trending = ref<Paper[]>([])
  const userProfile = ref<UserProfile | null>(null)
  const isLoading = ref(false)
  const lastUpdated = ref<Date | null>(null)

  // Computed
  const hasPersonalized = computed(() => personalized.value.length > 0)
  const hasTrending = computed(() => trending.value.length > 0)

  // Actions
  const loadPersonalized = async (userId: number, limit = 20) => {
    isLoading.value = true
    try {
      const results = await recommendationsApi.getPersonalized({
        userId,
        limit,
        algorithm: 'hybrid'
      })
      personalized.value = results
      lastUpdated.value = new Date()
      return results
    } catch (error) {
      console.error('Failed to load personalized recommendations:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const loadSimilar = async (paperId: number, limit = 10) => {
    isLoading.value = true
    try {
      const results = await recommendationsApi.getSimilar(paperId, limit)
      similar.value = results
      return results
    } catch (error) {
      console.error('Failed to load similar papers:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const loadTrending = async (limit = 20) => {
    isLoading.value = true
    try {
      const results = await recommendationsApi.getTrending(limit)
      trending.value = results
      return results
    } catch (error) {
      console.error('Failed to load trending papers:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const loadUserProfile = async (userId: number) => {
    try {
      const profile = await recommendationsApi.getUserProfile(userId)
      userProfile.value = profile
      return profile
    } catch (error) {
      console.error('Failed to load user profile:', error)
      throw error
    }
  }

  const submitFeedback = async (
    userId: number,
    paperId: number,
    liked: boolean,
    rating?: number
  ) => {
    try {
      await recommendationsApi.submitFeedback({
        userId,
        paperId,
        liked,
        rating
      })
      // 刷新个性化推荐
      await loadPersonalized(userId)
    } catch (error) {
      console.error('Failed to submit feedback:', error)
      throw error
    }
  }

  const explainRecommendation = async (userId: number, paperId: number) => {
    try {
      const explanation = await recommendationsApi.explainRecommendation(userId, paperId)
      return explanation
    } catch (error) {
      console.error('Failed to get recommendation explanation:', error)
      throw error
    }
  }

  const clearSimilar = () => {
    similar.value = []
  }

  return {
    // State
    personalized,
    similar,
    trending,
    userProfile,
    isLoading,
    lastUpdated,

    // Computed
    hasPersonalized,
    hasTrending,

    // Actions
    loadPersonalized,
    loadSimilar,
    loadTrending,
    loadUserProfile,
    submitFeedback,
    explainRecommendation,
    clearSimilar
  }
})
