/**
 * API Type Definitions
 *
 * Centralized TypeScript types for all API requests and responses
 * across all 9 backend modules (94 endpoints)
 */

// ============================================================================
// COMMON TYPES
// ============================================================================

/**
 * Standard API response wrapper
 */
export interface ApiResponse<T = any> {
  success: boolean
  message?: string
  data: T
  error?: string
  code?: string
}

/**
 * Pagination parameters
 */
export interface PaginationParams {
  page?: number
  pageSize?: number
  sortBy?: string
  sortOrder?: 'asc' | 'desc'
}

/**
 * Paginated response
 */
export interface PaginatedResponse<T> {
  items: T[]
  total: number
  page: number
  pageSize: number
  totalPages: number
  hasMore: boolean
}

/**
 * Generic ID parameter
 */
export interface IdParam {
  id: number | string
}

/**
 * Date range filter
 */
export interface DateRange {
  startDate?: string | Date
  endDate?: string | Date
}

/**
 * Generic filter interface
 */
export interface FilterOptions {
  search?: string
  tags?: string[]
  dateRange?: DateRange
  [key: string]: any
}

// ============================================================================
// AUTH API TYPES
// ============================================================================

/**
 * Login request
 */
export interface LoginRequest {
  username: string
  password: string
  rememberMe?: boolean
}

/**
 * Register request
 */
export interface RegisterRequest {
  username: string
  email: string
  password: string
  fullName?: string
}

/**
 * Auth response with tokens
 */
export interface AuthResponse {
  accessToken: string
  refreshToken: string
  user: User
}

/**
 * Token refresh request
 */
export interface RefreshTokenRequest {
  refreshToken: string
}

/**
 * Password reset request
 */
export interface ForgotPasswordRequest {
  email: string
}

/**
 * Reset password request
 */
export interface ResetPasswordRequest {
  token: string
  newPassword: string
}

/**
 * Verify email request
 */
export interface VerifyEmailRequest {
  token: string
}

// ============================================================================
// USER API TYPES
// ============================================================================

/**
 * User profile
 */
export interface User {
  id: number
  username: string
  email: string
  fullName?: string
  role: 'admin' | 'user'
  avatar?: string
  createdAt: string
  updatedAt: string
  settings?: UserSettings
}

/**
 * User settings
 */
export interface UserSettings {
  theme: 'light' | 'dark' | 'system'
  language: 'en' | 'zh'
  notifications: {
    email: boolean
    push: boolean
    crawler: boolean
    recommendations: boolean
  }
  privacy: {
    profileVisible: boolean
    activityVisible: boolean
  }
  preferences: {
    itemsPerPage: number
    defaultSort: string
    defaultView: 'list' | 'grid' | 'card'
  }
}

/**
 * Update user request
 */
export interface UpdateUserRequest {
  fullName?: string
  email?: string
  avatar?: string
}

/**
 * Change password request
 */
export interface ChangePasswordRequest {
  currentPassword: string
  newPassword: string
}

/**
 * User statistics
 */
export interface UserStats {
  totalPapers: number
  readPapers: number
  bookmarkedPapers: number
  totalCitations: number
  joinDate: string
  lastActive: string
}

/**
 * User activity log
 */
export interface UserActivity {
  id: number
  userId: number
  action: string
  entityType: string
  entityId?: number
  description: string
  timestamp: string
  metadata?: any
}

// ============================================================================
// PAPER API TYPES
// ============================================================================

/**
 * Paper entity
 */
export interface Paper {
  id: number
  title: string
  authors: string[]
  abstract?: string
  year?: number
  publication?: string
  volume?: string
  issue?: string
  pages?: string
  doi?: string
  url?: string
  pdfUrl?: string
  citations?: number
  tags?: string[]
  keywords?: string[]
  isRead?: boolean
  isBookmarked?: boolean
  notes?: string
  readingProgress?: number
  createdAt: string
  updatedAt: string
}

/**
 * Paper query filters
 */
export interface PaperQuery extends PaginationParams {
  search?: string
  authors?: string[]
  year?: number
  yearRange?: { start: number; end: number }
  publication?: string
  tags?: string[]
  keywords?: string[]
  isRead?: boolean
  isBookmarked?: boolean
  minCitations?: number
}

/**
 * Create paper request
 */
export interface CreatePaperRequest {
  title: string
  authors: string[]
  abstract?: string
  year?: number
  publication?: string
  volume?: string
  issue?: string
  pages?: string
  doi?: string
  url?: string
  pdfUrl?: string
  tags?: string[]
  keywords?: string[]
  notes?: string
}

/**
 * Update paper request
 */
export interface UpdatePaperRequest extends Partial<CreatePaperRequest> {
  isRead?: boolean
  isBookmarked?: boolean
  readingProgress?: number
  notes?: string
}

/**
 * Paper statistics
 */
export interface PaperStats {
  totalPapers: number
  readPapers: number
  unreadPapers: number
  bookmarkedPapers: number
  papersByYear: Record<number, number>
  papersByJournal: Record<string, number>
  topAuthors: Array<{ name: string; count: number }>
  totalCitations: number
  avgCitationsPerPaper: number
}

// ============================================================================
// SEARCH API TYPES
// ============================================================================

/**
 * Search query
 */
export interface SearchQuery extends PaginationParams {
  q: string
  fields?: ('title' | 'authors' | 'abstract' | 'keywords' | 'publication')[]
  year?: { from?: number; to?: number }
  publications?: string[]
  tags?: string[]
  hasAbstract?: boolean
  hasPdf?: boolean
}

/**
 * Advanced search request
 */
export interface AdvancedSearchRequest {
  title?: string
  authors?: string[]
  abstract?: string
  keywords?: string[]
  publication?: string
  yearRange?: { from: number; to: number }
  tags?: string[]
  operator?: 'AND' | 'OR'
}

/**
 * Search result
 */
export interface SearchResult {
  paper: Paper
  relevance: number
  highlights: {
    title?: string[]
    abstract?: string[]
    authors?: string[]
  }
}

/**
 * Search suggestions
 */
export interface SearchSuggestions {
  queries: string[]
  authors: string[]
  publications: string[]
  keywords: string[]
}

/**
 * Search statistics
 */
export interface SearchStats {
  totalSearches: number
  topQueries: Array<{ query: string; count: number }>
  recentSearches: Array<{ query: string; timestamp: string }>
}

// ============================================================================
// CRAWLER API TYPES
// ============================================================================

/**
 * Crawler template
 */
export interface CrawlerTemplate {
  id: number
  name: string
  description?: string
  source: string
  sourceType: 'arxiv' | 'pubmed' | 'ieee' | 'springer' | 'elsevier' | 'custom'
  config: CrawlerConfig
  schedule?: CrawlerSchedule
  isActive: boolean
  createdAt: string
  updatedAt: string
}

/**
 * Crawler configuration
 */
export interface CrawlerConfig {
  baseUrl: string
  startUrl?: string
  searchPath?: string
  maxPapers?: number
  delay: number
  retryAttempts: number
  timeout: number
  headers?: Record<string, string>
  proxy?: string
  fields: CrawlerField[]
}

/**
 * Crawler field mapping
 */
export interface CrawlerField {
  name: string
  selector: string
  attribute?: string
  transform?: string
  required: boolean
}

/**
 * Crawler schedule
 */
export interface CrawlerSchedule {
  enabled: boolean
  frequency: 'once' | 'daily' | 'weekly' | 'monthly'
  time?: string
  dayOfWeek?: number
  dayOfMonth?: number
}

/**
 * Crawler task
 */
export interface CrawlerTask {
  id: number
  templateId: number
  name: string
  status: 'pending' | 'running' | 'paused' | 'completed' | 'failed' | 'cancelled'
  progress: number
  totalPapers: number
  crawledPapers: number
  failedPapers: number
  startTime?: string
  endTime?: string
  error?: string
  results?: CrawlerResult[]
  createdAt: string
  updatedAt: string
}

/**
 * Crawler result
 */
export interface CrawlerResult {
  id: number
  taskId: number
  title: string
  authors: string[]
  abstract?: string
  year?: number
  publication?: string
  url: string
  status: 'success' | 'failed'
  error?: string
  paperId?: number
}

/**
 * Create crawler task request
 */
export interface CreateCrawlerTaskRequest {
  templateId: number
  name?: string
  maxPapers?: number
  searchQuery?: string
  yearRange?: { from: number; to: number }
}

/**
 * Crawler statistics
 */
export interface CrawlerStats {
  totalTemplates: number
  activeTemplates: number
  totalTasks: number
  runningTasks: number
  completedTasks: number
  failedTasks: number
  totalPapersCrawled: number
}

// ============================================================================
// EXPORT API TYPES
// ============================================================================

/**
 * Export format
 */
export type ExportFormat = 'csv' | 'json' | 'bibtex' | 'endnote' | 'xml'

/**
 * Export request
 */
export interface ExportRequest {
  format: ExportFormat
  paperIds?: number[]
  filters?: PaperQuery
  includeAbstract?: boolean
  includeNotes?: boolean
  includeTags?: boolean
  filename?: string
}

/**
 * Export job
 */
export interface ExportJob {
  id: number
  format: ExportFormat
  status: 'pending' | 'processing' | 'completed' | 'failed'
  progress: number
  fileUrl?: string
  error?: string
  paperCount: number
  createdAt: string
  completedAt?: string
}

/**
 * Export formats available
 */
export interface ExportFormatInfo {
  format: ExportFormat
  name: string
  extension: string
  mimeType: string
  description: string
}

// ============================================================================
// STATS API TYPES
// ============================================================================

/**
 * System statistics
 */
export interface SystemStats {
  uptime: number
  version: string
  environment: string
  startTime: string
  timezone: string
}

/**
 * Resource statistics
 */
export interface ResourceStats {
  cpu: {
    usage: number
    cores: number
  }
  memory: {
    used: number
    total: number
    percentage: number
  }
  disk: {
    used: number
    total: number
    percentage: number
  }
  network: {
    bytesReceived: number
    bytesSent: number
  }
}

/**
 * Module statistics
 */
export interface ModuleStats {
  name: string
  status: 'loaded' | 'unloaded' | 'error'
  endpoints: number
  lastError?: string
  loadTime: number
}

/**
 * Performance statistics
 */
export interface PerformanceStats {
  avgResponseTime: number
  requestsPerSecond: number
  errorRate: number
  slowestEndpoints: Array<{
    endpoint: string
    avgTime: number
  }>
}

/**
 * Real-time statistics
 */
export interface RealtimeStats {
  timestamp: string
  activeConnections: number
  requestsInFlight: number
  queueLength: number
}

// ============================================================================
// AI API TYPES
// ============================================================================

/**
 * AI review request
 */
export interface AIReviewRequest {
  paperId?: number
  title: string
  abstract?: string
  content?: string
  criteria?: string[]
}

/**
 * AI review response
 */
export interface AIReviewResponse {
  id: number
  paperId?: number
  review: AIReview
  status: 'pending' | 'processing' | 'completed' | 'failed'
  createdAt: string
  completedAt?: string
}

/**
 * AI review content
 */
export interface AIReview {
  overall: {
    score: number
    summary: string
    strengths: string[]
    weaknesses: string[]
  }
  sections: Array<{
    name: string
    score: number
    comments: string
  }>
  recommendations: string[]
}

/**
 * Literature review request
 */
export interface LiteratureReviewRequest {
  topic: string
  paperIds?: number[]
  maxPapers?: number
  yearRange?: { from: number; to: number }
  sections?: string[]
}

/**
 * Literature review response
 */
export interface LiteratureReviewResponse {
  id: number
  topic: string
  review: string
  references: Array<{
    paperId: number
    citation: string
  }>
  status: 'pending' | 'processing' | 'completed' | 'failed'
  createdAt: string
  completedAt?: string
}

/**
 * Research plan request
 */
export interface ResearchPlanRequest {
  topic: string
  objectives: string[]
  duration?: string
  resources?: string[]
}

/**
 * Research plan response
 */
export interface ResearchPlanResponse {
  id: number
  topic: string
  plan: ResearchPlan
  status: 'pending' | 'processing' | 'completed' | 'failed'
  createdAt: string
  completedAt?: string
}

/**
 * Research plan content
 */
export interface ResearchPlan {
  phases: Array<{
    name: string
    duration: string
    objectives: string[]
    tasks: string[]
    milestones: string[]
  }>
  resources: Array<{
    type: string
    name: string
    description: string
  }>
  timeline: string
}

/**
 * AI keywords extraction request
 */
export interface AIKeywordsRequest {
  paperId?: number
  title: string
  abstract?: string
  content?: string
  maxKeywords?: number
}

/**
 * AI keywords response
 */
export interface AIKeywordsResponse {
  keywords: Array<{
    word: string
    relevance: number
  }>
}

/**
 * AI similar papers request
 */
export interface AISimilarPapersRequest {
  paperId: number
  maxPapers?: number
}

/**
 * AI similar papers response
 */
export interface AISimilarPapersResponse {
  paperId: number
  similarPapers: Array<{
    paper: Paper
    similarity: number
    reasons: string[]
  }>
}

/**
 * AI generation history
 */
export interface AIGenerationHistory {
  id: number
  type: 'review' | 'literature_review' | 'research_plan' | 'keywords' | 'similar_papers'
  paperId?: number
  status: 'pending' | 'processing' | 'completed' | 'failed'
  createdAt: string
  completedAt?: string
  error?: string
}

/**
 * AI usage statistics
 */
export interface AIUsageStats {
  totalGenerations: number
  generationsByType: Record<string, number>
  totalTokensUsed: number
  totalCost: number
  avgProcessingTime: number
}

/**
 * AI service status
 */
export interface AIServiceStatus {
  available: boolean
  service: string
  model: string
  rateLimit: {
    remaining: number
    reset: string
  }
}

// ============================================================================
// RECOMMENDATION API TYPES
// ============================================================================

/**
 * Recommendation type
 */
export type RecommendationType = 'personalized' | 'trending' | 'similar' | 'collaborative'

/**
 * Recommendation
 */
export interface Recommendation {
  paper: Paper
  type: RecommendationType
  score: number
  reasons: string[]
  metadata?: any
}

/**
 * Personalized recommendations request
 */
export interface PersonalizedRecommendationsRequest {
  limit?: number
  excludeRead?: boolean
  excludeBookmarked?: boolean
}

/**
 * Similar papers request
 */
export interface SimilarPapersRequest {
  paperId: number
  limit?: number
}

/**
 * Trending papers request
 */
export interface TrendingPapersRequest {
  period?: 'day' | 'week' | 'month'
  limit?: number
}

/**
 * User recommendations request
 */
export interface UserRecommendationsRequest {
  userId: number
  limit?: number
}

/**
 * Recommendation feedback
 */
export interface RecommendationFeedback {
  recommendationId: number
  feedback: 'helpful' | 'not_helpful' | 'neutral'
  reason?: string
}

/**
 * Dismiss recommendation request
 */
export interface DismissRecommendationRequest {
  recommendationId: number
  reason?: string
}

/**
 * Recommendation history
 */
export interface RecommendationHistory {
  id: number
  userId: number
  paperId: number
  type: RecommendationType
  feedback?: 'helpful' | 'not_helpful' | 'neutral'
  dismissed: boolean
  createdAt: string
}

// ============================================================================
// BATCH OPERATION TYPES
// ============================================================================

/**
 * Batch operation request
 */
export interface BatchOperationRequest {
  paperIds: number[]
  operation: 'read' | 'unread' | 'bookmark' | 'unbookmark' | 'delete' | 'tag' | 'export'
  data?: any
}

/**
 * Batch operation result
 */
export interface BatchOperationResult {
  successful: number[]
  failed: Array<{
    id: number
    error: string
  }>
  total: number
  successCount: number
  failureCount: number
}

// ============================================================================
// ERROR TYPES
// ============================================================================

/**
 * API error response
 */
export interface ApiError {
  type: 'NETWORK' | 'AUTH' | 'VALIDATION' | 'SERVER' | 'UNKNOWN'
  message: string
  userMessage: {
    en: string
    zh: string
  }
  statusCode?: number
  details?: any
  isRetryable: boolean
}

// ============================================================================
// UTILITY TYPES
// ============================================================================

/**
 * Optional type utility
 */
export type Partial<T> = {
  [P in keyof T]?: T[P]
}

/**
 * Required keys utility
 */
export type RequiredKeys<T, K extends keyof T> = T & Required<Pick<T, K>>

/**
 * Deep partial utility
 */
export type DeepPartial<T> = {
  [P in keyof T]?: T[P] extends object ? DeepPartial<T[P]> : T[P]
}
