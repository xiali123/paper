/**
 * Type Validation Adapter
 *
 * Provides runtime type validation and sanitization for backend API responses.
 * Ensures data integrity between backend and frontend with comprehensive type guards
 * and validation utilities.
 *
 * @module api/adapters/validationAdapter
 */

import type { Paper, PaperDetail, Statistics, JournalStats, YearStats, SearchResult } from '@/types'
import type { User } from '../modules/auth'

// ============================================================================
// VALIDATION RESULT TYPES
// ============================================================================

/**
 * Validation result with optional sanitized data
 */
export interface ValidationResult<T = unknown> {
  /** Whether validation passed */
  valid: boolean

  /** Array of error messages with field paths */
  errors: ValidationError[]

  /** Array of warning messages (non-critical issues) */
  warnings: ValidationWarning[]

  /** Sanitized data if validation passed */
  data?: T

  /** Original input data */
  original: unknown
}

/**
 * Validation error with field path
 */
export interface ValidationError {
  /** Error message */
  message: string

  /** Path to the invalid field (e.g., "data.papers[0].title") */
  path: string

  /** Expected type */
  expected: string

  /** Actual value received */
  received: string
}

/**
 * Validation warning for non-critical issues
 */
export interface ValidationWarning {
  /** Warning message */
  message: string

  /** Path to the field */
  path: string

  /** Severity level */
  severity: 'low' | 'medium' | 'high'
}

// ============================================================================
// SANITIZATION SCHEMA TYPES
// ============================================================================

/**
 * Schema for data sanitization
 */
export interface SanitizationSchema {
  /** Field type */
  type: 'string' | 'number' | 'boolean' | 'array' | 'object' | 'date'

  /** Default value if field is missing/null */
  default?: unknown

  /** Whether to trim strings */
  trim?: boolean

  /** Whether to convert empty strings to null */
  emptyStringToNull?: boolean

  /** Whether to convert undefined to null */
  undefinedToNull?: boolean

  /** Number conversion options */
  numberOptions?: {
    /** Parse string to number */
    parseString?: boolean
    /** Default value if NaN */
    nanDefault?: number
  }

  /** Date conversion options */
  dateOptions?: {
    /** Output format */
    format: 'timestamp' | 'iso' | 'date'
    /** Default value if invalid */
    invalidDefault?: number | string
  }

  /** Array item schema (for arrays) */
  itemSchema?: SanitizationSchema

  /** Object property schemas (for objects) */
  properties?: Record<string, SanitizationSchema>

  /** Whether field is required */
  required?: boolean

  /** Custom validator function */
  validator?: (value: unknown) => boolean | { valid: boolean; message?: string }
}

// ============================================================================
// VALIDATION RESULT BUILDERS
// ============================================================================

/**
 * Create a successful validation result
 */
function success<T>(data: T, original: unknown, warnings: ValidationWarning[] = []): ValidationResult<T> {
  return {
    valid: true,
    errors: [],
    warnings,
    data,
    original
  }
}

/**
 * Create a failed validation result
 */
function failure(
  errors: ValidationError[],
  warnings: ValidationWarning[] = [],
  original: unknown
): ValidationResult {
  return {
    valid: false,
    errors,
    warnings,
    original
  }
}

/**
 * Create a validation error
 */
function createError(
  path: string,
  message: string,
  expected: string,
  received: string
): ValidationError {
  return { message, path, expected, received }
}

/**
 * Create a validation warning
 */
function createWarning(
  path: string,
  message: string,
  severity: 'low' | 'medium' | 'high' = 'low'
): ValidationWarning {
  return { message, path, severity }
}

// ============================================================================
// TYPE CHECKING UTILITIES
// ============================================================================

/**
 * Check if value is defined (not null or undefined)
 */
function isDefined(value: unknown): boolean {
  return value !== null && value !== undefined
}

/**
 * Get value type name
 */
function getTypeName(value: unknown): string {
  if (value === null) return 'null'
  if (value === undefined) return 'undefined'
  if (Array.isArray(value)) return 'array'
  return typeof value
}

/**
 * Check if value is a plain object (not array, null, or Date)
 */
function isPlainObject(value: unknown): value is Record<string, unknown> {
  return (
    typeof value === 'object' &&
    value !== null &&
    !Array.isArray(value) &&
    !(value instanceof Date)
  )
}

// ============================================================================
// TYPE GUARDS - BACKEND RESPONSES
// ============================================================================

/**
 * Check if data is a backend paper response
 */
export function isBackendPaperResponse(data: unknown): data is { success: boolean; data: Paper } {
  if (!isPlainObject(data)) return false

  const response = data as Record<string, unknown>
  return (
    response.success === true &&
    isPlainObject(response.data) &&
    typeof (response.data as Paper).title === 'string' &&
    typeof (response.data as Paper).id !== 'undefined'
  )
}

/**
 * Check if data is a backend paper list response
 */
export function isBackendPaperListResponse(data: unknown): data is { success: boolean; data: Paper[] } {
  if (!isPlainObject(data)) return false

  const response = data as Record<string, unknown>
  return (
    response.success === true &&
    Array.isArray(response.data) &&
    response.data.length > 0 &&
    isPlainObject(response.data[0]) &&
    typeof (response.data[0] as Paper).title === 'string'
  )
}

/**
 * Check if data is a backend auth response
 */
export function isBackendAuthResponse(data: unknown): data is {
  success: boolean
  access_token: string
  refresh_token: string
  expires_in: number
  user: unknown
} {
  if (!isPlainObject(data)) return false

  const response = data as Record<string, unknown>
  return (
    response.success === true &&
    typeof response.access_token === 'string' &&
    typeof response.refresh_token === 'string' &&
    typeof response.expires_in === 'number'
  )
}

/**
 * Check if data is a backend stats response
 */
export function isBackendStatsResponse(data: unknown): data is { success: boolean; data: Statistics } {
  if (!isPlainObject(data)) return false

  const response = data as Record<string, unknown>
  if (response.success !== true || !isPlainObject(response.data)) return false

  const stats = response.data as Record<string, unknown>
  return (
    typeof stats.totalPapers === 'number' &&
    typeof stats.totalJournals === 'number' &&
    typeof stats.topTierPapers === 'number'
  )
}

// ============================================================================
// VALIDATION FUNCTIONS
// ============================================================================

/**
 * Validate backend response data structure
 *
 * @param data - Backend response data
 * @param expectedType - Expected data type
 * @returns Validation result with typed data if valid
 *
 * @example
 * ```typescript
 * const result = validateBackendResponse(backendData, 'paper')
 * if (result.valid) {
 *   const paper = result.data // Type is Paper
 * } else {
 *   console.error(result.errors)
 * }
 * ```
 */
export function validateBackendResponse<T>(
  data: unknown,
  expectedType: 'paper' | 'paperList' | 'paperDetail' | 'stats' | 'user' | 'auth' | 'search'
): ValidationResult<T> {
  const errors: ValidationError[] = []
  const warnings: ValidationWarning[] = []

  // Validate response wrapper
  if (!isPlainObject(data)) {
    errors.push(createError(
      '',
      'Response must be an object',
      'object',
      getTypeName(data)
    ))
    return failure(errors, warnings, data)
  }

  const response = data as Record<string, unknown>

  // Check success flag
  if (response.success !== true) {
    errors.push(createError(
      'success',
      'Response success flag must be true',
      'true',
      String(response.success)
    ))
  }

  // Check data field exists
  if (!isDefined(response.data)) {
    errors.push(createError(
      'data',
      'Response data field is required',
      'defined',
      'undefined'
    ))
    return failure(errors, warnings, data)
  }

  // Validate based on expected type
  const validationResult = validateDataByType(response.data, expectedType, 'data')
  errors.push(...validationResult.errors)
  warnings.push(...validationResult.warnings)

  if (errors.length > 0) {
    return failure(errors, warnings, data)
  }

  return success(response.data as T, data, warnings)
}

/**
 * Validate data based on type
 */
function validateDataByType(
  data: unknown,
  type: string,
  path: string
): { errors: ValidationError[]; warnings: ValidationWarning[] } {
  const errors: ValidationError[] = []
  const warnings: ValidationWarning[] = []

  switch (type) {
    case 'paper':
      validatePaper(data, path, errors, warnings)
      break

    case 'paperList':
      validatePaperList(data, path, errors, warnings)
      break

    case 'paperDetail':
      validatePaperDetail(data, path, errors, warnings)
      break

    case 'stats':
      validateStats(data, path, errors, warnings)
      break

    case 'user':
      validateUser(data, path, errors, warnings)
      break

    case 'auth':
      validateAuthData(data, path, errors, warnings)
      break

    case 'search':
      validateSearchResult(data, path, errors, warnings)
      break

    default:
      errors.push(createError(path, `Unknown type: ${type}`, type, getTypeName(data)))
  }

  return { errors, warnings }
}

/**
 * Validate paper object
 */
function validatePaper(
  data: unknown,
  path: string,
  errors: ValidationError[],
  warnings: ValidationWarning[]
): void {
  if (!isPlainObject(data)) {
    errors.push(createError(path, 'Paper must be an object', 'object', getTypeName(data)))
    return
  }

  const paper = data as Record<string, unknown>

  // Required fields
  if (typeof paper.id !== 'string' && typeof paper.id !== 'number') {
    errors.push(createError(`${path}.id`, 'Paper ID must be string or number', 'string|number', getTypeName(paper.id)))
  }

  if (typeof paper.title !== 'string') {
    errors.push(createError(`${path}.title`, 'Paper title must be a string', 'string', getTypeName(paper.title)))
  } else if (paper.title.length === 0) {
    errors.push(createError(`${path}.title`, 'Paper title cannot be empty', 'non-empty string', 'empty string'))
  } else if (paper.title.length > 1000) {
    warnings.push(createWarning(`${path}.title`, 'Paper title is unusually long (>1000 chars)', 'medium'))
  }

  if (typeof paper.journal !== 'string' && !isPlainObject(paper.journal)) {
    errors.push(createError(`${path}.journal`, 'Journal must be string or object', 'string|object', getTypeName(paper.journal)))
  }

  if (typeof paper.year !== 'string' && typeof paper.year !== 'number') {
    errors.push(createError(`${path}.year`, 'Year must be string or number', 'string|number', getTypeName(paper.year)))
  }

  if (typeof paper.level !== 'string') {
    errors.push(createError(`${path}.level`, 'CCF level must be a string', 'string', getTypeName(paper.level)))
  } else if (!['A', 'B', 'C'].includes(paper.level)) {
    warnings.push(createWarning(`${path}.level`, `Unknown CCF level: ${paper.level}`, 'low'))
  }

  if (typeof paper.authors !== 'string') {
    errors.push(createError(`${path}.authors`, 'Authors must be a string', 'string', getTypeName(paper.authors)))
  }

  // Optional fields
  if (isDefined(paper.abstract) && typeof paper.abstract !== 'string') {
    errors.push(createError(`${path}.abstract`, 'Abstract must be a string', 'string', getTypeName(paper.abstract)))
  }

  if (isDefined(paper.keywords) && !Array.isArray(paper.keywords)) {
    errors.push(createError(`${path}.keywords`, 'Keywords must be an array', 'array', getTypeName(paper.keywords)))
  }

  if (isDefined(paper.urls) && !isPlainObject(paper.urls)) {
    errors.push(createError(`${path}.urls`, 'URLs must be an object', 'object', getTypeName(paper.urls)))
  }
}

/**
 * Validate paper list
 */
function validatePaperList(
  data: unknown,
  path: string,
  errors: ValidationError[],
  warnings: ValidationWarning[]
): void {
  if (!Array.isArray(data)) {
    errors.push(createError(path, 'Paper list must be an array', 'array', getTypeName(data)))
    return
  }

  // Validate first few papers (performance optimization)
  const validateCount = Math.min(data.length, 10)

  for (let i = 0; i < validateCount; i++) {
    validatePaper(data[i], `${path}[${i}]`, errors, warnings)
  }

  // Check total count
  if (data.length > validateCount) {
    warnings.push(createWarning(
      path,
      `Only validated first ${validateCount} papers out of ${data.length}`,
      'low'
    ))
  }
}

/**
 * Validate paper detail
 */
function validatePaperDetail(
  data: unknown,
  path: string,
  errors: ValidationError[],
  warnings: ValidationWarning[]
): void {
  // First validate as paper
  validatePaper(data, path, errors, warnings)

  if (!isPlainObject(data)) {
    return
  }

  const detail = data as Record<string, unknown>

  // Additional required fields for detail
  if (typeof detail.citations !== 'number') {
    errors.push(createError(`${path}.citations`, 'Citations must be a number', 'number', getTypeName(detail.citations)))
  } else if (detail.citations < 0) {
    errors.push(createError(`${path}.citations`, 'Citations cannot be negative', 'non-negative number', String(detail.citations)))
  }

  if (typeof detail.references !== 'number') {
    errors.push(createError(`${path}.references`, 'References must be a number', 'number', getTypeName(detail.references)))
  } else if (detail.references < 0) {
    errors.push(createError(`${path}.references`, 'References cannot be negative', 'non-negative number', String(detail.references)))
  }

  if (typeof detail.downloadCount !== 'number') {
    errors.push(createError(`${path}.downloadCount`, 'Download count must be a number', 'number', getTypeName(detail.downloadCount)))
  }
}

/**
 * Validate statistics object
 */
function validateStats(
  data: unknown,
  path: string,
  errors: ValidationError[],
  warnings: ValidationWarning[]
): void {
  if (!isPlainObject(data)) {
    errors.push(createError(path, 'Statistics must be an object', 'object', getTypeName(data)))
    return
  }

  const stats = data as Record<string, unknown>

  if (typeof stats.totalPapers !== 'number') {
    errors.push(createError(`${path}.totalPapers`, 'totalPapers must be a number', 'number', getTypeName(stats.totalPapers)))
  }

  if (typeof stats.totalJournals !== 'number') {
    errors.push(createError(`${path}.totalJournals`, 'totalJournals must be a number', 'number', getTypeName(stats.totalJournals)))
  }

  if (typeof stats.topTierPapers !== 'number') {
    errors.push(createError(`${path}.topTierPapers`, 'topTierPapers must be a number', 'number', getTypeName(stats.topTierPapers)))
  }

  if (typeof stats.papersLastYear !== 'number') {
    errors.push(createError(`${path}.papersLastYear`, 'papersLastYear must be a number', 'number', getTypeName(stats.papersLastYear)))
  }

  if (typeof stats.mostActiveJournal !== 'string') {
    errors.push(createError(`${path}.mostActiveJournal`, 'mostActiveJournal must be a string', 'string', getTypeName(stats.mostActiveJournal)))
  }
}

/**
 * Validate user object
 */
function validateUser(
  data: unknown,
  path: string,
  errors: ValidationError[],
  warnings: ValidationWarning[]
): void {
  if (!isPlainObject(data)) {
    errors.push(createError(path, 'User must be an object', 'object', getTypeName(data)))
    return
  }

  const user = data as Record<string, unknown>

  if (typeof user.id !== 'string' && typeof user.id !== 'number') {
    errors.push(createError(`${path}.id`, 'User ID must be string or number', 'string|number', getTypeName(user.id)))
  }

  if (typeof user.username !== 'string') {
    errors.push(createError(`${path}.username`, 'Username must be a string', 'string', getTypeName(user.username)))
  }

  if (typeof user.email !== 'string') {
    errors.push(createError(`${path}.email`, 'Email must be a string', 'string', getTypeName(user.email)))
  } else if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(user.email)) {
    warnings.push(createWarning(`${path}.email`, 'Email format looks invalid', 'medium'))
  }

  if (isDefined(user.role) && typeof user.role !== 'string') {
    errors.push(createError(`${path}.role`, 'Role must be a string', 'string', getTypeName(user.role)))
  }
}

/**
 * Validate auth response data
 */
function validateAuthData(
  data: unknown,
  path: string,
  errors: ValidationError[],
  warnings: ValidationWarning[]
): void {
  if (!isPlainObject(data)) {
    errors.push(createError(path, 'Auth data must be an object', 'object', getTypeName(data)))
    return
  }

  const auth = data as Record<string, unknown>

  if (typeof auth.access_token !== 'string') {
    errors.push(createError(`${path}.access_token`, 'Access token must be a string', 'string', getTypeName(auth.access_token)))
  }

  if (typeof auth.refresh_token !== 'string') {
    errors.push(createError(`${path}.refresh_token`, 'Refresh token must be a string', 'string', getTypeName(auth.refresh_token)))
  }

  if (typeof auth.expires_in !== 'number') {
    errors.push(createError(`${path}.expires_in`, 'Expires in must be a number', 'number', getTypeName(auth.expires_in)))
  }

  if (isDefined(auth.user)) {
    validateUser(auth.user, `${path}.user`, errors, warnings)
  }
}

/**
 * Validate search result
 */
function validateSearchResult(
  data: unknown,
  path: string,
  errors: ValidationError[],
  warnings: ValidationWarning[]
): void {
  if (!isPlainObject(data)) {
    errors.push(createError(path, 'Search result must be an object', 'object', getTypeName(data)))
    return
  }

  const result = data as Record<string, unknown>

  if (!Array.isArray(result.papers)) {
    errors.push(createError(`${path}.papers`, 'Papers must be an array', 'array', getTypeName(result.papers)))
  } else {
    // Validate first few papers
    const validateCount = Math.min(result.papers.length, 5)
    for (let i = 0; i < validateCount; i++) {
      validatePaper(result.papers[i], `${path}.papers[${i}]`, errors, warnings)
    }
  }

  if (typeof result.total !== 'number') {
    errors.push(createError(`${path}.total`, 'Total must be a number', 'number', getTypeName(result.total)))
  }

  if (typeof result.keyword !== 'string') {
    errors.push(createError(`${path}.keyword`, 'Keyword must be a string', 'string', getTypeName(result.keyword)))
  }
}

// ============================================================================
// SANITIZATION FUNCTIONS
// ============================================================================

/**
 * Sanitize and normalize backend data according to schema
 *
 * @param data - Raw backend data
 * @param schema - Sanitization schema
 * @returns Sanitized data
 *
 * @example
 * ```typescript
 * const sanitized = sanitizeBackendData(backendPaper, {
 *   type: 'object',
 *   properties: {
 *     id: { type: 'number' },
 *     title: { type: 'string', trim: true },
 *     year: { type: 'number', numberOptions: { parseString: true } }
 *   }
 * })
 * ```
 */
export function sanitizeBackendData<T = Record<string, unknown>>(
  data: unknown,
  schema: SanitizationSchema
): T {
  // Skip validation in production for performance
  if (import.meta.env.PROD && import.meta.env.VITE_ENABLE_VALIDATION !== 'true') {
    return data as T
  }

  return sanitizeValue(data, schema) as T
}

/**
 * Sanitize a single value according to schema
 */
function sanitizeValue(value: unknown, schema: SanitizationSchema): unknown {
  const { type } = schema

  // Handle undefined/null
  if (!isDefined(value)) {
    if (schema.required) {
      return schema.default
    }
    if (schema.undefinedToNull) {
      return null
    }
    return value
  }

  switch (type) {
    case 'string':
      return sanitizeString(value, schema)

    case 'number':
      return sanitizeNumber(value, schema)

    case 'boolean':
      return sanitizeBoolean(value)

    case 'array':
      return sanitizeArray(value, schema)

    case 'object':
      return sanitizeObject(value, schema)

    case 'date':
      return sanitizeDate(value, schema)

    default:
      return value
  }
}

/**
 * Sanitize string value
 */
function sanitizeString(value: unknown, schema: SanitizationSchema): string {
  let str = String(value)

  if (schema.trim) {
    str = str.trim()
  }

  if (schema.emptyStringToNull && str.length === 0) {
    return null as unknown as string
  }

  return str
}

/**
 * Sanitize number value
 */
function sanitizeNumber(value: unknown, schema: SanitizationSchema): number {
  if (typeof value === 'number') {
    return value
  }

  if (schema.numberOptions?.parseString && typeof value === 'string') {
    const num = parseFloat(value)
    if (!isNaN(num)) {
      return num
    }
  }

  return (schema.numberOptions?.nanDefault ?? 0) as number
}

/**
 * Sanitize boolean value
 */
function sanitizeBoolean(value: unknown): boolean {
  if (typeof value === 'boolean') {
    return value
  }

  if (typeof value === 'string') {
    return value.toLowerCase() === 'true'
  }

  if (typeof value === 'number') {
    return value !== 0
  }

  return Boolean(value)
}

/**
 * Sanitize array value
 */
function sanitizeArray(value: unknown, schema: SanitizationSchema): unknown[] {
  if (!Array.isArray(value)) {
    return schema.default as unknown[] ?? []
  }

  if (!schema.itemSchema) {
    return value
  }

  return value.map(item => sanitizeValue(item, schema.itemSchema!))
}

/**
 * Sanitize object value
 */
function sanitizeObject(value: unknown, schema: SanitizationSchema): Record<string, unknown> {
  if (!isPlainObject(value)) {
    return schema.default as Record<string, unknown> ?? {}
  }

  const obj = value as Record<string, unknown>
  const result: Record<string, unknown> = {}

  if (schema.properties) {
    // Sanitize according to property schemas
    for (const [key, propSchema] of Object.entries(schema.properties)) {
      result[key] = sanitizeValue(obj[key], propSchema)
    }
  } else {
    // Deep copy without schema
    for (const [key, val] of Object.entries(obj)) {
      if (isPlainObject(val)) {
        result[key] = sanitizeObject(val, {})
      } else if (Array.isArray(val)) {
        result[key] = [...val]
      } else {
        result[key] = val
      }
    }
  }

  return result
}

/**
 * Sanitize date value
 */
function sanitizeDate(value: unknown, schema: SanitizationSchema): number | string | Date {
  const dateOptions = schema.dateOptions!

  // Try to parse date
  let date: Date

  if (value instanceof Date) {
    date = value
  } else if (typeof value === 'number') {
    date = new Date(value)
  } else if (typeof value === 'string') {
    date = new Date(value)
  } else {
    return dateOptions.invalidDefault ?? Date.now()
  }

  // Check if valid
  if (isNaN(date.getTime())) {
    return dateOptions.invalidDefault ?? Date.now()
  }

  // Format according to options
  switch (dateOptions.format) {
    case 'timestamp':
      return date.getTime()
    case 'iso':
      return date.toISOString()
    case 'date':
      return date
    default:
      return date.getTime()
  }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Safely validate and sanitize backend response
 *
 * @param data - Backend response data
 * @param expectedType - Expected data type
 * @param schema - Optional sanitization schema
 * @returns Sanitized data or null if validation fails
 */
export function safeValidateBackendResponse<T>(
  data: unknown,
  expectedType: 'paper' | 'paperList' | 'paperDetail' | 'stats' | 'user' | 'auth' | 'search',
  schema?: SanitizationSchema
): T | null {
  const result = validateBackendResponse<T>(data, expectedType)

  if (!result.valid) {
    console.error('Backend response validation failed:', result.errors)
    return null
  }

  if (schema) {
    return sanitizeBackendData<T>(result.data, schema)
  }

  return result.data
}

/**
 * Assert that backend response is valid, throw if not
 *
 * @param data - Backend response data
 * @param expectedType - Expected data type
 * @throws Error if validation fails
 */
export function assertValidBackendResponse<T>(
  data: unknown,
  expectedType: 'paper' | 'paperList' | 'paperDetail' | 'stats' | 'user' | 'auth' | 'search'
): asserts data is { success: boolean; data: T } {
  const result = validateBackendResponse<T>(data, expectedType)

  if (!result.valid) {
    const errorMessages = result.errors.map(e => `${e.path}: ${e.message}`).join('\n')
    throw new Error(`Backend response validation failed:\n${errorMessages}`)
  }
}

// ============================================================================
// PREDEFINED SCHEMAS
// ============================================================================

/**
 * Common sanitization schemas
 */
export const Schemas = {
  /** Paper sanitization schema */
  paper: {
    type: 'object',
    properties: {
      id: { type: 'number' },
      title: { type: 'string', trim: true },
      journal: { type: 'string', trim: true },
      year: { type: 'number', numberOptions: { parseString: true } },
      level: { type: 'string', trim: true },
      authors: { type: 'string', trim: true },
      abstract: { type: 'string', emptyStringToNull: true },
      keywords: { type: 'array' },
      urls: { type: 'object' }
    }
  } as SanitizationSchema,

  /** User sanitization schema */
  user: {
    type: 'object',
    properties: {
      id: { type: 'number' },
      username: { type: 'string', trim: true },
      email: { type: 'string', trim: true },
      fullName: { type: 'string', trim: true, emptyStringToNull: true },
      role: { type: 'string', trim: true },
      isActive: { type: 'boolean' },
      createdAt: { type: 'date', dateOptions: { format: 'timestamp' } }
    }
  } as SanitizationSchema,

  /** Statistics sanitization schema */
  stats: {
    type: 'object',
    properties: {
      totalPapers: { type: 'number' },
      totalJournals: { type: 'number' },
      topTierPapers: { type: 'number' },
      papersLastYear: { type: 'number' },
      mostActiveJournal: { type: 'string', trim: true }
    }
  } as SanitizationSchema
}

// ============================================================================
// EXPORTS
// ============================================================================

export default {
  // Validation functions
  validateBackendResponse,
  safeValidateBackendResponse,
  assertValidBackendResponse,

  // Sanitization functions
  sanitizeBackendData,

  // Type guards
  isBackendPaperResponse,
  isBackendPaperListResponse,
  isBackendAuthResponse,
  isBackendStatsResponse,

  // Utilities
  ValidationResult,
  ValidationError,
  ValidationWarning,
  SanitizationSchema,

  // Predefined schemas
  Schemas
}
