/**
 * AI Store (Pinia)
 *
 * Manages AI functionality including:
 * - AI peer review
 * - Literature review generation
 * - Research plan generation
 * - AI usage history and statistics
 *
 * @module stores/aiStore
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { AIReviewResult, LiteratureReview, ResearchPlan } from '@/types'

export type AIFeatureType = 'review' | 'literature-review' | 'research-plan'

export interface AIGeneration {
  id: string
  type: AIFeatureType
  input: string
  result: AIReviewResult | LiteratureReview | ResearchPlan
  timestamp: number
  paperCount?: number
  tokensUsed?: number
  cost?: number
  duration?: number
}

export interface AIStats {
  totalGenerations: number
  generationsByType: Record<AIFeatureType, number>
  totalTokensUsed: number
  totalCost: number
  averageDuration: number
  recentGenerations: AIGeneration[]
}

export const useAIStore = defineStore(
  'ai',
  () => {
    // ========================================================================
    // State
    // ========================================================================

    /** AI generation history */
    const history = ref<AIGeneration[]>([])

    /** Current generation (in progress) */
    const currentGeneration = ref<AIGeneration | null>(null)

    /** Loading state */
    const loading = ref(false)

    /** Generation progress */
    const progress = ref(0)

    /** Error message */
    const error = ref<string | null>(null)

    /** AI statistics */
    const stats = ref<AIStats | null>(null)

    /** Maximum history items */
    const maxHistoryItems = 100

    // ========================================================================
    // Computed Properties
    // ========================================================================

    /** Recent generations (last 10) */
    const recentGenerations = computed(() =>
      history.value.slice(0, 10)
    )

    /** Generations by type */
    const generationsByType = computed(() => {
      const byType: Record<AIFeatureType, AIGeneration[]> = {
        review: [],
        'literature-review': [],
        'research-plan': []
      }

      history.value.forEach(gen => {
        byType[gen.type].push(gen)
      })

      return byType
    })

    /** Total generations */
    const totalGenerations = computed(() => history.value.length)

    /** Total tokens used */
    const totalTokensUsed = computed(() =>
      history.value.reduce((sum, gen) => sum + (gen.tokensUsed || 0), 0)
    )

    /** Total cost */
    const totalCost = computed(() =>
      history.value.reduce((sum, gen) => sum + (gen.cost || 0), 0)
    )

    /** Average duration */
    const averageDuration = computed(() => {
      const durations = history.value
        .filter(gen => gen.duration !== undefined)
        .map(gen => gen.duration!)

      if (durations.length === 0) return 0

      const sum = durations.reduce((a, b) => a + b, 0)
      return Math.round(sum / durations.length)
    })

    /** Most used feature */
    const mostUsedFeature = computed(() => {
      const counts = {
        review: generationsByType.value.review.length,
        'literature-review': generationsByType.value['literature-review'].length,
        'research-plan': generationsByType.value['research-plan'].length
      }

      return Object.entries(counts).sort((a, b) => b[1] - a[1])[0]?.[0] as AIFeatureType || 'review'
    })

    /** Has history */
    const hasHistory = computed(() => history.value.length > 0)

    // ========================================================================
    // Actions
    // ========================================================================

    /**
     * Generate AI peer review
     */
    async function generateReview(paperId: number, options?: any) {
      loading.value = true
      error.value = null
      progress.value = 0

      const startTime = Date.now()

      try {
        // Simulate progress
        const progressInterval = setInterval(() => {
          if (progress.value < 90) {
            progress.value += 10
          }
        }, 500)

        // TODO: Implement actual API call
        // const result = await aiApi.generateReview(paperId, options)

        // Mock result for now
        const result: AIReviewResult = {
          overallScore: 85,
          sections: [
            {
              name: 'Methodology',
              score: 90,
              strengths: ['Novel approach', 'Well-designed experiments'],
              weaknesses: ['Limited scalability analysis'],
              suggestions: ['Add scalability tests']
            },
            {
              name: 'Presentation',
              score: 80,
              strengths: ['Clear writing', 'Good organization'],
              weaknesses: ['Some figures are hard to read'],
              suggestions: ['Improve figure quality']
            }
          ],
          overallFeedback: 'This paper presents a novel approach with strong methodology and clear presentation.',
          recommendations: ['Accept with minor revisions']
        }

        clearInterval(progressInterval)
        progress.value = 100

        const generation: AIGeneration = {
          id: `ai_gen_${Date.now()}`,
          type: 'review',
          input: `Paper ID: ${paperId}`,
          result,
          timestamp: Date.now(),
          tokensUsed: 1500,
          cost: 0.03,
          duration: Date.now() - startTime
        }

        addToHistory(generation)
        currentGeneration.value = null

        return result
      } catch (err: any) {
        error.value = err.message || 'Failed to generate review'
        throw err
      } finally {
        loading.value = false
        progress.value = 0
      }
    }

    /**
     * Generate literature review
     */
    async function generateLiteratureReview(topic: string, paperIds: number[], options?: any) {
      loading.value = true
      error.value = null
      progress.value = 0

      const startTime = Date.now()

      try {
        // Simulate progress
        const progressInterval = setInterval(() => {
          if (progress.value < 90) {
            progress.value += 10
          }
        }, 500)

        // TODO: Implement actual API call
        // const result = await aiApi.generateLiteratureReview(topic, paperIds, options)

        // Mock result for now
        const result: LiteratureReview = {
          topic,
          introduction: 'This review examines recent advances in the field.',
          sections: [
            {
              title: 'Background',
              content: 'The field has evolved rapidly in recent years...',
              papers: [1, 2, 3]
            },
            {
              title: 'Recent Advances',
              content: 'Several groundbreaking approaches have emerged...',
              papers: [4, 5, 6]
            }
          ],
          conclusion: 'The field shows great promise for future development.',
          references: paperIds
        }

        clearInterval(progressInterval)
        progress.value = 100

        const generation: AIGeneration = {
          id: `ai_gen_${Date.now()}`,
          type: 'literature-review',
          input: topic,
          result,
          timestamp: Date.now(),
          paperCount: paperIds.length,
          tokensUsed: 3000,
          cost: 0.06,
          duration: Date.now() - startTime
        }

        addToHistory(generation)
        currentGeneration.value = null

        return result
      } catch (err: any) {
        error.value = err.message || 'Failed to generate literature review'
        throw err
      } finally {
        loading.value = false
        progress.value = 0
      }
    }

    /**
     * Generate research plan
     */
    async function generateResearchPlan(topic: string, objectives: string[], options?: any) {
      loading.value = true
      error.value = null
      progress.value = 0

      const startTime = Date.now()

      try {
        // Simulate progress
        const progressInterval = setInterval(() => {
          if (progress.value < 90) {
            progress.value += 10
          }
        }, 500)

        // TODO: Implement actual API call
        // const result = await aiApi.generateResearchPlan(topic, objectives, options)

        // Mock result for now
        const result: ResearchPlan = {
          topic,
          objectives,
          phases: [
            {
              name: 'Literature Review',
              duration: '4 weeks',
              tasks: ['Survey existing research', 'Identify gaps']
            },
            {
              name: 'Method Development',
              duration: '8 weeks',
              tasks: ['Design algorithms', 'Implement prototype']
            },
            {
              name: 'Experiments',
              duration: '6 weeks',
              tasks: ['Collect data', 'Run experiments', 'Analyze results']
            }
          ],
          timeline: '18 weeks total',
          resources: ['Computing resources', 'Dataset access'],
          milestones: [
            { week: 4, milestone: 'Complete literature review' },
            { week: 12, milestone: 'Finish implementation' },
            { week: 18, milestone: 'Submit paper' }
          ]
        }

        clearInterval(progressInterval)
        progress.value = 100

        const generation: AIGeneration = {
          id: `ai_gen_${Date.now()}`,
          type: 'research-plan',
          input: topic,
          result,
          timestamp: Date.now(),
          tokensUsed: 2000,
          cost: 0.04,
          duration: Date.now() - startTime
        }

        addToHistory(generation)
        currentGeneration.value = null

        return result
      } catch (err: any) {
        error.value = err.message || 'Failed to generate research plan'
        throw err
      } finally {
        loading.value = false
        progress.value = 0
      }
    }

    /**
     * Regenerate from history
     */
    async function regenerate(id: string) {
      const generation = history.value.find(g => g.id === id)
      if (!generation) {
        throw new Error('Generation not found')
      }

      switch (generation.type) {
        case 'review':
          return await generateReview(parseInt(generation.input.split(': ')[1]))
        case 'literature-review':
          return await generateLiteratureReview(generation.input, [])
        case 'research-plan':
          return await generateResearchPlan(generation.input, [])
      }
    }

    /**
     * Delete from history
     */
    function deleteFromHistory(id: string) {
      history.value = history.value.filter(g => g.id !== id)
      persistHistory()
    }

    /**
     * Clear history
     */
    function clearHistory() {
      history.value = []
      persistHistory()
    }

    /**
     * Add to history
     */
    function addToHistory(generation: AIGeneration) {
      history.value.unshift(generation)

      // Limit history size
      if (history.value.length > maxHistoryItems) {
        history.value = history.value.slice(0, maxHistoryItems)
      }

      persistHistory()
    }

    /**
     * Fetch AI statistics
     */
    async function fetchStats() {
      loading.value = true
      error.value = null

      try {
        // TODO: Implement actual API call
        // const data = await aiApi.getStats()
        // stats.value = data

        // Calculate from history
        stats.value = {
          totalGenerations: totalGenerations.value,
          generationsByType: {
            review: generationsByType.value.review.length,
            'literature-review': generationsByType.value['literature-review'].length,
            'research-plan': generationsByType.value['research-plan'].length
          },
          totalTokensUsed: totalTokensUsed.value,
          totalCost: totalCost.value,
          averageDuration: averageDuration.value,
          recentGenerations: recentGenerations.value
        }

        return stats.value
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch AI statistics'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Reset state
     */
    function reset() {
      history.value = []
      currentGeneration.value = null
      loading.value = false
      progress.value = 0
      error.value = null
      stats.value = null
    }

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * Persist history to localStorage
     */
    function persistHistory() {
      try {
        localStorage.setItem('ai-history', JSON.stringify(history.value))
      } catch (err) {
        console.error('Failed to persist AI history:', err)
      }
    }

    /**
     * Load history from localStorage
     */
    function loadHistory() {
      try {
        const stored = localStorage.getItem('ai-history')
        if (stored) {
          history.value = JSON.parse(stored)
        }
      } catch (err) {
        console.error('Failed to load AI history:', err)
      }
    }

    // Initialize on store creation
    loadHistory()

    // ========================================================================
    // Return
    // ========================================================================

    return {
      // State
      history,
      currentGeneration,
      loading,
      progress,
      error,
      stats,

      // Computed
      recentGenerations,
      generationsByType,
      totalGenerations,
      totalTokensUsed,
      totalCost,
      averageDuration,
      mostUsedFeature,
      hasHistory,

      // Actions
      generateReview,
      generateLiteratureReview,
      generateResearchPlan,
      regenerate,
      deleteFromHistory,
      clearHistory,
      fetchStats,
      reset
    }
  },
  {
    persist: {
      key: 'ai-store',
      storage: localStorage
    }
  }
)
