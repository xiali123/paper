/**
 * Error Handling Adapter
 *
 * Provides unified error handling and transformation for backend API errors.
 * Converts different error formats into a consistent frontend error structure
 * and provides user-friendly error messages with internationalization support.
 *
 * @module api/adapters/errorAdapter
 */

// ============================================================================
// ERROR TYPE DEFINITIONS
// ============================================================================

/**
 * Backend error response format (C++ backend)
 */
interface BackendErrorResponse {
  success: false
  error: string
  code?: string
  message?: string
  details?: Record<string, unknown>
  timestamp?: number
  requestId?: string
}

/**
 * Axios error structure
 */
interface AxiosErrorData {
  response?: {
    data?: BackendErrorResponse | Record<string, unknown>
    status?: number
    statusText?: string
    headers?: Record<string, string>
  }
  code?: string
  message?: string
  config?: {
    url?: string
    method?: string
    headers?: Record<string, string>
  }
}

/**
 * Frontend unified error format
 */
export interface ApiError {
  /** Success flag (always false for errors) */
  success: false

  /** Error type classification */
  type: ErrorType

  /** Original error message from backend */
  message: string

  /** User-friendly error message (localized) */
  userMessage: string

  /** Error code from backend */
  code?: string

  /** HTTP status code */
  statusCode?: number

  /** Error timestamp */
  timestamp: number

  /** Request ID for tracking */
  requestId?: string

  /** Original error details for debugging */
  details?: Record<string, unknown>

  /** Request URL that caused the error */
  url?: string

  /** HTTP method */
  method?: string
}

/**
 * Error classification types
 */
export enum ErrorType {
  /** Network-level errors (timeout, connection refused) */
  NETWORK = 'NETWORK',

  /** Authentication and authorization errors */
  AUTH = 'AUTH',

  /** Input validation errors */
  VALIDATION = 'VALIDATION',

  /** Server-side errors (5xx) */
  SERVER = 'SERVER',

  /** Unknown or unclassified errors */
  UNKNOWN = 'UNKNOWN'
}

/**
 * Supported locales for error messages
 */
type Locale = 'zh-CN' | 'en-US'

// ============================================================================
// ERROR CODE MAPPINGS
// ============================================================================

/**
 * Error code to user message mappings (Chinese)
 */
const ERROR_MESSAGES_ZH_CN: Record<string, string> = {
  // Authentication errors (AUTH_xxx)
  'AUTH_001': '用户名或密码错误',
  'AUTH_002': '登录已过期，请重新登录',
  'AUTH_003': '访问令牌无效',
  'AUTH_004': '刷新令牌无效',
  'AUTH_005': '账户已被禁用',
  'AUTH_006': '权限不足，无法访问此资源',
  'AUTH_007': '账户未激活，请先验证邮箱',
  'AUTH_008': '会话已失效，请重新登录',
  'AUTH_009': '密码错误次数过多，账户已临时锁定',
  'AUTH_010': '两次密码输入不一致',

  // Validation errors (VALIDATION_xxx)
  'VALIDATION_001': '输入数据格式错误',
  'VALIDATION_002': '必填字段不能为空',
  'VALIDATION_003': '邮箱格式不正确',
  'VALIDATION_004': '密码强度不足',
  'VALIDATION_005': '用户名已存在',
  'VALIDATION_006': '邮箱已被注册',
  'VALIDATION_007': '参数值超出允许范围',
  'VALIDATION_008': '日期格式不正确',
  'VALIDATION_009': '文件大小超过限制',
  'VALIDATION_010': '不支持的文件格式',

  // Network errors (NETWORK_xxx)
  'NETWORK_001': '网络连接超时',
  'NETWORK_002': '无法连接到服务器',
  'NETWORK_003': '网络连接中断',
  'NETWORK_004': 'DNS解析失败',
  'NETWORK_005': '请求被取消',

  // Server errors (SERVER_xxx)
  'SERVER_500': '服务器内部错误',
  'SERVER_502': '网关错误',
  'SERVER_503': '服务暂时不可用',
  'SERVER_504': '网关超时',
  'SERVER_001': '数据库操作失败',
  'SERVER_002': '文件系统错误',
  'SERVER_003': '外部服务调用失败',

  // Resource errors
  'NOT_FOUND': '请求的资源不存在',
  'CONFLICT': '数据冲突，请刷新后重试',
  'RATE_LIMIT': '请求过于频繁，请稍后再试',
  'MAINTENANCE': '系统维护中，请稍后再试'
}

/**
 * Error code to user message mappings (English)
 */
const ERROR_MESSAGES_EN_US: Record<string, string> = {
  // Authentication errors
  'AUTH_001': 'Invalid username or password',
  'AUTH_002': 'Session expired, please login again',
  'AUTH_003': 'Invalid access token',
  'AUTH_004': 'Invalid refresh token',
  'AUTH_005': 'Account has been disabled',
  'AUTH_006': 'Insufficient permissions to access this resource',
  'AUTH_007': 'Account not activated, please verify your email',
  'AUTH_008': 'Session invalid, please login again',
  'AUTH_009': 'Too many failed password attempts, account temporarily locked',
  'AUTH_010': 'Passwords do not match',

  // Validation errors
  'VALIDATION_001': 'Invalid input format',
  'VALIDATION_002': 'Required fields cannot be empty',
  'VALIDATION_003': 'Invalid email format',
  'VALIDATION_004': 'Password is too weak',
  'VALIDATION_005': 'Username already exists',
  'VALIDATION_006': 'Email already registered',
  'VALIDATION_007': 'Parameter value out of range',
  'VALIDATION_008': 'Invalid date format',
  'VALIDATION_009': 'File size exceeds limit',
  'VALIDATION_010': 'Unsupported file format',

  // Network errors
  'NETWORK_001': 'Network connection timeout',
  'NETWORK_002': 'Cannot connect to server',
  'NETWORK_003': 'Network connection interrupted',
  'NETWORK_004': 'DNS resolution failed',
  'NETWORK_005': 'Request cancelled',

  // Server errors
  'SERVER_500': 'Internal server error',
  'SERVER_502': 'Bad gateway',
  'SERVER_503': 'Service temporarily unavailable',
  'SERVER_504': 'Gateway timeout',
  'SERVER_001': 'Database operation failed',
  'SERVER_002': 'File system error',
  'SERVER_003': 'External service call failed',

  // Resource errors
  'NOT_FOUND': 'Requested resource not found',
  'CONFLICT': 'Data conflict, please refresh and try again',
  'RATE_LIMIT': 'Too many requests, please try again later',
  'MAINTENANCE': 'System under maintenance, please try again later'
}

/**
 * Default error messages (fallback)
 */
const DEFAULT_MESSAGES: Record<Locale, Record<ErrorType, string>> = {
  'zh-CN': {
    NETWORK: '网络错误，请检查网络连接',
    AUTH: '认证失败，请重新登录',
    VALIDATION: '数据验证失败，请检查输入',
    SERVER: '服务器错误，请稍后再试',
    UNKNOWN: '未知错误，请稍后再试'
  },
  'en-US': {
    NETWORK: 'Network error, please check your connection',
    AUTH: 'Authentication failed, please login again',
    VALIDATION: 'Validation failed, please check your input',
    SERVER: 'Server error, please try again later',
    UNKNOWN: 'Unknown error, please try again later'
  }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Detect current locale from browser or default to zh-CN
 */
function detectLocale(): Locale {
  if (typeof window === 'undefined' || !window.navigator) {
    return 'zh-CN'
  }

  const browserLang = window.navigator.language || 'zh-CN'
  return browserLang.startsWith('zh') ? 'zh-CN' : 'en-US'
}

/**
 * Get error messages for current locale
 */
function getErrorMessages(locale?: Locale): Record<string, string> {
  const currentLocale = locale || detectLocale()
  return currentLocale === 'zh-CN' ? ERROR_MESSAGES_ZH_CN : ERROR_MESSAGES_EN_US
}

/**
 * Extract error code from various error formats
 */
function extractErrorCode(error: unknown): string | undefined {
  if (typeof error === 'object' && error !== null) {
    const err = error as Record<string, unknown>

    // Try backend error format
    if (typeof err.code === 'string') {
      return err.code
    }

    // Try HTTP status code mapping
    if (typeof err.statusCode === 'number') {
      return mapStatusCodeToErrorCode(err.statusCode)
    }
  }

  return undefined
}

/**
 * Map HTTP status codes to error codes
 */
function mapStatusCodeToErrorCode(status: number): string {
  const statusMap: Record<number, string> = {
    400: 'VALIDATION_001',
    401: 'AUTH_001',
    403: 'AUTH_006',
    404: 'NOT_FOUND',
    409: 'CONFLICT',
    429: 'RATE_LIMIT',
    500: 'SERVER_500',
    502: 'SERVER_502',
    503: 'SERVER_503',
    504: 'SERVER_504'
  }

  return statusMap[status] || `SERVER_${status}`
}

/**
 * Classify error type from error object
 */
function classifyError(error: unknown): ErrorType {
  // Network errors (no response, timeout, connection refused)
  if (isNetworkErrorObject(error)) {
    return ErrorType.NETWORK
  }

  // Extract status code if available
  if (typeof error === 'object' && error !== null) {
    const err = error as Record<string, unknown>

    if (typeof err.statusCode === 'number') {
      const status = err.statusCode

      // Authentication errors
      if (status === 401 || status === 403) {
        return ErrorType.AUTH
      }

      // Validation errors
      if (status === 400 || status === 422) {
        return ErrorType.VALIDATION
      }

      // Server errors
      if (status >= 500) {
        return ErrorType.SERVER
      }
    }

    // Check error code for type hints
    if (typeof err.code === 'string') {
      const code = err.code.toUpperCase()

      if (code.startsWith('AUTH_')) {
        return ErrorType.AUTH
      }
      if (code.startsWith('VALIDATION_')) {
        return ErrorType.VALIDATION
      }
      if (code.startsWith('NETWORK_')) {
        return ErrorType.NETWORK
      }
      if (code.startsWith('SERVER_')) {
        return ErrorType.SERVER
      }
    }
  }

  return ErrorType.UNKNOWN
}

// ============================================================================
// TYPE GUARD FUNCTIONS
// ============================================================================

/**
 * Check if error is an Axios error with response data
 */
function isNetworkErrorObject(error: unknown): error is { code?: string; message?: string } {
  if (typeof error !== 'object' || error === null) {
    return false
  }

  const err = error as Record<string, unknown>

  // Check for Axios error codes
  const networkErrorCodes = ['ECONNABORTED', 'ETIMEDOUT', 'ENETDOWN', 'ECONNREFUSED', 'ECONNRESET']

  return (
    !('response' in err) ||
    typeof err.code === 'string' && networkErrorCodes.includes(err.code) ||
    err.message === 'Network Error'
  )
}

/**
 * Check if value is a backend error response
 */
function isBackendErrorResponse(data: unknown): data is BackendErrorResponse {
  if (typeof data !== 'object' || data === null) {
    return false
  }

  const err = data as Record<string, unknown>

  return (
    err.success === false &&
    (typeof err.error === 'string' || typeof err.message === 'string')
  )
}

// ============================================================================
// PUBLIC API - ERROR TRANSFORMATION
// ============================================================================

/**
 * Transform various error formats into unified ApiError format
 *
 * @param error - Error from axios, backend, or unknown source
 * @param locale - Optional locale for user messages (defaults to browser locale)
 * @returns Unified ApiError object
 *
 * @example
 * ```typescript
 * try {
 *   await api.login(credentials)
 * } catch (err) {
 *   const apiError = transformApiError(err)
 *   console.log(apiError.userMessage) // User-friendly error message
 *   console.log(apiError.type) // Error classification
 * }
 * ```
 */
export function transformApiError(error: unknown, locale?: Locale): ApiError {
  const timestamp = Date.now()
  const errorType = classifyError(error)
  const errorCode = extractErrorCode(error)

  // Extract basic error information
  let message = 'Unknown error'
  let statusCode: number | undefined
  let requestId: string | undefined
  let details: Record<string, unknown> | undefined
  let url: string | undefined
  let method: string | undefined

  // Handle Axios errors with response
  if (typeof error === 'object' && error !== null) {
    const err = error as AxiosErrorData

    // Extract request metadata
    url = err.config?.url
    method = err.config?.method?.toUpperCase()

    // Extract response data
    if (err.response) {
      statusCode = err.response.status

      if (err.response.data) {
        const data = err.response.data

        // Backend error format
        if (isBackendErrorResponse(data)) {
          message = data.error || data.message || message
          requestId = data.requestId
          details = data.details
        } else if (typeof data === 'object') {
          // Generic error object
          message = (data as Record<string, unknown>).error as string ||
                    (data as Record<string, unknown>).message as string ||
                    message
          details = data as Record<string, unknown>
        } else if (typeof data === 'string') {
          message = data
        }
      } else {
        message = err.response.statusText || message
      }
    } else {
      // Network error without response
      message = err.message || 'Network error'
    }
  } else if (error instanceof Error) {
    // Standard JavaScript Error
    message = error.message
  } else if (typeof error === 'string') {
    // Simple string error
    message = error
  }

  // Generate user-friendly message
  const userMessage = createUserFriendlyMessage(errorCode, errorType, locale)

  // Construct unified error object
  return {
    success: false,
    type: errorType,
    message,
    userMessage,
    code: errorCode,
    statusCode,
    timestamp,
    requestId,
    details,
    url,
    method
  }
}

/**
 * Create user-friendly error message with internationalization
 *
 * @param code - Error code from backend
 * @param type - Error type classification
 * @param locale - Optional locale (defaults to browser locale)
 * @returns Localized user-friendly error message
 *
 * @example
 * ```typescript
 * const message = createUserFriendlyMessage('AUTH_001', ErrorType.AUTH, 'zh-CN')
 * // Returns: "用户名或密码错误"
 * ```
 */
export function createUserFriendlyMessage(
  code: string | undefined,
  type: ErrorType,
  locale?: Locale
): string {
  const currentLocale = locale || detectLocale()
  const messages = getErrorMessages(currentLocale)

  // Try to get message from error code
  if (code && messages[code]) {
    return messages[code]
  }

  // Fall back to default message for error type
  const defaultMessages = DEFAULT_MESSAGES[currentLocale]
  return defaultMessages[type] || defaultMessages[ErrorType.UNKNOWN]
}

/**
 * Check if error is a network error
 *
 * @param error - Error to check
 * @returns True if error is network-related
 *
 * @example
 * ```typescript
 * try {
 *   await api.getData()
 * } catch (err) {
 *   if (isNetworkError(err)) {
 *     // Show network error UI
 *   }
 * }
 * ```
 */
export function isNetworkError(error: unknown): boolean {
  const apiError = transformApiError(error)
  return apiError.type === ErrorType.NETWORK
}

/**
 * Check if error is an authentication error
 *
 * @param error - Error to check
 * @returns True if error is authentication-related
 *
 * @example
 * ```typescript
 * try {
 *   await api.getProfile()
 * } catch (err) {
 *   if (isAuthError(err)) {
 *     // Redirect to login page
 *   }
 * }
 * ```
 */
export function isAuthError(error: unknown): boolean {
  const apiError = transformApiError(error)
  return apiError.type === ErrorType.AUTH
}

/**
 * Check if error is a validation error
 *
 * @param error - Error to check
 * @returns True if error is validation-related
 */
export function isValidationError(error: unknown): boolean {
  const apiError = transformApiError(error)
  return apiError.type === ErrorType.VALIDATION
}

/**
 * Check if error is a server error
 *
 * @param error - Error to check
 * @returns True if error is server-related (5xx)
 */
export function isServerError(error: unknown): boolean {
  const apiError = transformApiError(error)
  return apiError.type === ErrorType.SERVER
}

/**
 * Check if error should trigger a token refresh
 *
 * @param error - Error to check
 * @returns True if error indicates expired token
 */
export function shouldRefreshToken(error: unknown): boolean {
  const apiError = transformApiError(error)
  return (
    apiError.type === ErrorType.AUTH &&
    apiError.statusCode === 401 &&
    (apiError.code === 'AUTH_002' || apiError.code === 'AUTH_003')
  )
}

/**
 * Check if error is retryable
 *
 * @param error - Error to check
 * @returns True if error might succeed on retry
 */
export function isRetryableError(error: unknown): boolean {
  const apiError = transformApiError(error)

  // Network errors and server errors are retryable
  if (apiError.type === ErrorType.NETWORK || apiError.type === ErrorType.SERVER) {
    return true
  }

  // Rate limit errors are retryable after delay
  if (apiError.code === 'RATE_LIMIT') {
    return true
  }

  return false
}

/**
 * Extract request ID from error for tracking
 *
 * @param error - Error to extract from
 * @returns Request ID or undefined
 */
export function getRequestId(error: unknown): string | undefined {
  const apiError = transformApiError(error)
  return apiError.requestId
}

/**
 * Get suggested user action based on error type
 *
 * @param error - Error to analyze
 * @param locale - Optional locale
 * @returns Suggested action message
 */
export function getSuggestedAction(error: unknown, locale?: Locale): string {
  const apiError = transformApiError(error)
  const currentLocale = locale || detectLocale()

  const actions: Record<Locale, Record<ErrorType, string>> = {
    'zh-CN': {
      NETWORK: '请检查网络连接后重试',
      AUTH: '请重新登录',
      VALIDATION: '请检查输入数据',
      SERVER: '请稍后再试或联系技术支持',
      UNKNOWN: '请刷新页面重试'
    },
    'en-US': {
      NETWORK: 'Please check your connection and try again',
      AUTH: 'Please login again',
      VALIDATION: 'Please check your input',
      SERVER: 'Please try again later or contact support',
      UNKNOWN: 'Please refresh the page and try again'
    }
  }

  return actions[currentLocale][apiError.type]
}

// ============================================================================
// EXPORTS
// ============================================================================

export default {
  transformApiError,
  createUserFriendlyMessage,
  isNetworkError,
  isAuthError,
  isValidationError,
  isServerError,
  shouldRefreshToken,
  isRetryableError,
  getRequestId,
  getSuggestedAction,
  ErrorType
}
