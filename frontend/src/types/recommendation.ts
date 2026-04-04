/**
 * 推荐系统类型定义
 */

/**
 * 推荐结果
 */
export interface RecommendationResult {
  paperId: number
  score: number
  reason: string
  category: string
  confidence: number
  paper: {
    id: number
    title: string
    authors: string[]
    year: number
    abstract: string
    citationCount: number
  }
}

/**
 * 用户画像
 */
export interface UserProfile {
  userId: number
  interests: { [key: string]: number }
  expertise: string[]
  recentlyViewed: number[]
  favoriteTopics: string[]
  readingLevel: string
  updateAt: string
}

/**
 * 推荐反馈
 */
export interface RecommendationFeedback {
  userId: number
  paperId: number
  liked: boolean
  rating?: number
  timestamp?: string
}
