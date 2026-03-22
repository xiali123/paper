/**
 * Type Validation Utilities
 *
 * Provides runtime validation functions for TypeScript types
 * Used to ensure API responses and user input match expected types
 *
 * @module types/validation
 */

import type {
  Paper,
  PaperDetail,
  SearchParams,
  Statistics,
  JournalStats,
  YearStats,
  CCFLevel
} from './index'

// ============================================================================
// VALIDATION RESULT TYPES
// ============================================================================

/**
 * Result of a validation operation
 */
export interface ValidationResult {
  /** Whether validation passed */
  valid: boolean

  /** Array of error messages */
  errors: string[]

  /** Array of warning messages (non-critical issues) */
  warnings: string[]
}

/**
 * Creates a successful validation result
 */
function success(warnings: string[] = []): ValidationResult {
  return { valid: true, errors: [], warnings }
}

/**
 * Creates a failed validation result
 */
function failure(errors: string[], warnings: string[] = []): ValidationResult {
  return { valid: false, errors, warnings }
}

// ============================================================================
// PAPER VALIDATION
// ============================================================================

/**
 * Validates a Paper object
 */
export function validatePaper(data: unknown): ValidationResult {
  const errors: string[] = []
  const warnings: string[] = []

  if (!data || typeof data !== 'object') {
    return failure(['Paper data must be an object'])
  }

  const paper = data as Record<string, unknown>

  // Required fields
  if (!paper.id) {
    errors.push('Paper ID is required')
  } else if (typeof paper.id !== 'string' && typeof paper.id !== 'number') {
    errors.push('Paper ID must be a string or number')
  }

  if (!paper.title) {
    errors.push('Paper title is required')
  } else if (typeof paper.title !== 'string') {
    errors.push('Paper title must be a string')
  } else if (paper.title.trim().length === 0) {
    errors.push('Paper title cannot be empty')
  } else if (paper.title.length > 500) {
    warnings.push('Paper title is unusually long (>500 characters)')
  }

  if (!paper.journal) {
    errors.push('Journal information is required')
  } else if (typeof paper.journal !== 'string' && typeof paper.journal !== 'object') {
    errors.push('Journal must be a string or object')
  }

  if (!paper.year) {
    errors.push('Publication year is required')
  } else if (typeof paper.year !== 'string' && typeof paper.year !== 'number') {
    errors.push('Year must be a string or number')
  } else {
    const yearNum = typeof paper.year === 'string' ? parseInt(paper.year) : paper.year
    if (isNaN(yearNum) || yearNum < 1900 || yearNum > 2100) {
      errors.push('Year must be a valid year between 1900 and 2100')
    }
  }

  if (!paper.level) {
    errors.push('CCF level is required')
  } else if (!['A', 'B', 'C'].includes(paper.level as string)) {
    errors.push('CCF level must be A, B, or C')
  }

  if (!paper.authors) {
    errors.push('Authors are required')
  } else if (typeof paper.authors !== 'string') {
    errors.push('Authors must be a string')
  } else if (paper.authors.trim().length === 0) {
    errors.push('Authors cannot be empty')
  }

  // Optional fields
  if (paper.abstract !== undefined) {
    if (typeof paper.abstract !== 'string') {
      errors.push('Abstract must be a string')
    } else if (paper.abstract.length > 10000) {
      warnings.push('Abstract is unusually long (>10000 characters)')
    }
  }

  if (paper.keywords !== undefined) {
    if (!Array.isArray(paper.keywords)) {
      errors.push('Keywords must be an array')
    } else if (!paper.keywords.every(k => typeof k === 'string')) {
      errors.push('All keywords must be strings')
    }
  }

  if (paper.urls !== undefined) {
    if (typeof paper.urls !== 'object') {
      errors.push('URLs must be an object')
    } else {
      const urls = paper.urls as Record<string, unknown>
      if (urls.doi !== undefined && typeof urls.doi !== 'string') {
        errors.push('DOI must be a string')
      }
      if (urls.journal !== undefined && typeof urls.journal !== 'string') {
        errors.push('Journal URL must be a string')
      }
      if (urls.pdf !== undefined && typeof urls.pdf !== 'string') {
        errors.push('PDF URL must be a string')
      }
    }
  }

  if (paper.citations !== undefined && typeof paper.citations !== 'number') {
    errors.push('Citations must be a number')
  }

  if (paper.references !== undefined && typeof paper.references !== 'number') {
    errors.push('References must be a number')
  }

  return errors.length > 0 ? failure(errors, warnings) : success(warnings)
}

/**
 * Validates a PaperDetail object
 */
export function validatePaperDetail(data: unknown): ValidationResult {
  const result = validatePaper(data)

  if (!result.valid) {
    return result
  }

  if (!data || typeof data !== 'object') {
    return failure(['Paper detail data must be an object'])
  }

  const detail = data as Record<string, unknown>
  const errors: string[] = []
  const warnings: string[] = [...result.warnings]

  // Additional required fields for PaperDetail
  if (detail.citations === undefined) {
    errors.push('Citations count is required for paper detail')
  } else if (typeof detail.citations !== 'number' || detail.citations < 0) {
    errors.push('Citations must be a non-negative number')
  }

  if (detail.references === undefined) {
    errors.push('References count is required for paper detail')
  } else if (typeof detail.references !== 'number' || detail.references < 0) {
    errors.push('References must be a non-negative number')
  }

  if (detail.downloadCount === undefined) {
    errors.push('Download count is required for paper detail')
  } else if (typeof detail.downloadCount !== 'number' || detail.downloadCount < 0) {
    errors.push('Download count must be a non-negative number')
  }

  // Optional detail fields
  if (detail.relatedPapers !== undefined) {
    if (!Array.isArray(detail.relatedPapers)) {
      errors.push('Related papers must be an array')
    } else {
      // Validate each related paper
      detail.relatedPapers.forEach((paper, index) => {
        const paperResult = validatePaper(paper)
        if (!paperResult.valid) {
          errors.push(`Related paper at index ${index} is invalid: ${paperResult.errors.join(', ')}`)
        }
      })
    }
  }

  return errors.length > 0 ? failure(errors, warnings) : success(warnings)
}

// ============================================================================
// SEARCH PARAMS VALIDATION
// ============================================================================

/**
 * Validates search parameters
 */
export function validateSearchParams(data: unknown): ValidationResult {
  const errors: string[] = []
  const warnings: string[] = []

  if (!data || typeof data !== 'object') {
    return failure(['Search parameters must be an object'])
  }

  const params = data as Record<string, unknown>

  // Required fields
  if (!params.q) {
    errors.push('Search query (q) is required')
  } else if (typeof params.q !== 'string') {
    errors.push('Search query must be a string')
  } else if (params.q.trim().length === 0) {
    errors.push('Search query cannot be empty')
  } else if (params.q.length > 500) {
    warnings.push('Search query is unusually long (>500 characters)')
  }

  // Optional fields
  if (params.year !== undefined) {
    if (typeof params.year !== 'string' && typeof params.year !== 'number') {
      errors.push('Year must be a string or number')
    } else {
      const yearNum = typeof params.year === 'string' ? parseInt(params.year) : params.year
      if (isNaN(yearNum) || yearNum < 1900 || yearNum > 2100) {
        errors.push('Year must be valid (1900-2100)')
      }
    }
  }

  if (params.level !== undefined) {
    if (typeof params.level !== 'string') {
      errors.push('Level must be a string')
    } else if (!['A', 'B', 'C'].includes(params.level)) {
      errors.push('Level must be A, B, or C')
    }
  }

  if (params.offset !== undefined) {
    const offset = typeof params.offset === 'string' ? parseInt(params.offset) : params.offset
    if (typeof offset !== 'number' || isNaN(offset) || offset < 0) {
      errors.push('Offset must be a non-negative number')
    }
  }

  if (params.limit !== undefined) {
    const limit = typeof params.limit === 'string' ? parseInt(params.limit) : params.limit
    if (typeof limit !== 'number' || isNaN(limit) || limit < 1) {
      errors.push('Limit must be a positive number')
    } else if (limit > 100) {
      warnings.push('Limit exceeds maximum of 100, will be capped')
    } else if (limit > 50) {
      warnings.push('Large limit may impact performance')
    }
  }

  if (params.sort !== undefined) {
    if (typeof params.sort !== 'string') {
      errors.push('Sort field must be a string')
    } else if (!['year', 'citations', 'title', 'journal'].includes(params.sort)) {
      warnings.push('Unknown sort field, will use default')
    }
  }

  if (params.order !== undefined) {
    if (typeof params.order !== 'string') {
      errors.push('Order must be a string')
    } else if (!['asc', 'desc'].includes(params.order.toLowerCase())) {
      errors.push('Order must be "asc" or "desc"')
    }
  }

  if (params.journal !== undefined && typeof params.journal !== 'string') {
    errors.push('Journal filter must be a string')
  }

  if (params.author !== undefined && typeof params.author !== 'string') {
    errors.push('Author filter must be a string')
  }

  return errors.length > 0 ? failure(errors, warnings) : success(warnings)
}

// ============================================================================
// STATISTICS VALIDATION
// ============================================================================

/**
 * Validates overview statistics
 */
export function validateStatistics(data: unknown): ValidationResult {
  const errors: string[] = []
  const warnings: string[] = []

  if (!data || typeof data !== 'object') {
    return failure(['Statistics data must be an object'])
  }

  const stats = data as Record<string, unknown>

  // Required fields
  if (stats.totalPapers === undefined) {
    errors.push('totalPapers is required')
  } else if (typeof stats.totalPapers !== 'number' || stats.totalPapers < 0) {
    errors.push('totalPapers must be a non-negative number')
  }

  if (stats.totalJournals === undefined) {
    errors.push('totalJournals is required')
  } else if (typeof stats.totalJournals !== 'number' || stats.totalJournals < 0) {
    errors.push('totalJournals must be a non-negative number')
  }

  if (stats.topTierPapers === undefined) {
    errors.push('topTierPapers is required')
  } else if (typeof stats.topTierPapers !== 'number' || stats.topTierPapers < 0) {
    errors.push('topTierPapers must be a non-negative number')
  }

  if (stats.papersLastYear === undefined) {
    errors.push('papersLastYear is required')
  } else if (typeof stats.papersLastYear !== 'number' || stats.papersLastYear < 0) {
    errors.push('papersLastYear must be a non-negative number')
  }

  if (stats.mostActiveJournal === undefined) {
    errors.push('mostActiveJournal is required')
  } else if (typeof stats.mostActiveJournal !== 'string') {
    errors.push('mostActiveJournal must be a string')
  }

  if (stats.averagePapersPerYear === undefined) {
    errors.push('averagePapersPerYear is required')
  } else if (typeof stats.averagePapersPerYear !== 'number' || stats.averagePapersPerYear < 0) {
    errors.push('averagePapersPerYear must be a non-negative number')
  }

  // Optional fields
  if (stats.latestUpdate !== undefined && typeof stats.latestUpdate !== 'string') {
    errors.push('latestUpdate must be a string')
  }

  if (stats.yearRange !== undefined) {
    if (typeof stats.yearRange !== 'string') {
      errors.push('yearRange must be a string')
    } else if (!/^\d{4}-\d{4}$/.test(stats.yearRange)) {
      warnings.push('yearRange should be in format "YYYY-YYYY"')
    }
  }

  return errors.length > 0 ? failure(errors, warnings) : success(warnings)
}

/**
 * Validates journal statistics array
 */
export function validateJournalStats(data: unknown): ValidationResult {
  const errors: string[] = []
  const warnings: string[] = []

  if (!Array.isArray(data)) {
    return failure(['Journal statistics must be an array'])
  }

  data.forEach((item, index) => {
    if (!item || typeof item !== 'object') {
      errors.push(`Journal stats at index ${index} must be an object`)
      return
    }

    const stat = item as Record<string, unknown>

    if (!stat.journal) {
      errors.push(`Journal stats at index ${index} missing journal name`)
    } else if (typeof stat.journal !== 'string') {
      errors.push(`Journal stats at index ${index} journal must be a string`)
    }

    if (stat.count === undefined) {
      errors.push(`Journal stats at index ${index} missing count`)
    } else if (typeof stat.count !== 'number' || stat.count < 0) {
      errors.push(`Journal stats at index ${index} count must be non-negative number`)
    }

    if (!stat.level) {
      errors.push(`Journal stats at index ${index} missing level`)
    } else if (!['A', 'B', 'C'].includes(stat.level as string)) {
      errors.push(`Journal stats at index ${index} level must be A, B, or C`)
    }

    if (stat.percentage !== undefined) {
      if (typeof stat.percentage !== 'number') {
        errors.push(`Journal stats at index ${index} percentage must be a number`)
      } else if (stat.percentage < 0 || stat.percentage > 100) {
        warnings.push(`Journal stats at index ${index} percentage should be 0-100`)
      }
    }
  })

  return errors.length > 0 ? failure(errors, warnings) : success(warnings)
}

/**
 * Validates year statistics array
 */
export function validateYearStats(data: unknown): ValidationResult {
  const errors: string[] = []
  const warnings: string[] = []

  if (!Array.isArray(data)) {
    return failure(['Year statistics must be an array'])
  }

  data.forEach((item, index) => {
    if (!item || typeof item !== 'object') {
      errors.push(`Year stats at index ${index} must be an object`)
      return
    }

    const stat = item as Record<string, unknown>

    if (!stat.year) {
      errors.push(`Year stats at index ${index} missing year`)
    } else if (typeof stat.year !== 'string' && typeof stat.year !== 'number') {
      errors.push(`Year stats at index ${index} year must be string or number`)
    } else {
      const yearNum = typeof stat.year === 'string' ? parseInt(stat.year) : stat.year
      if (isNaN(yearNum) || yearNum < 1900 || yearNum > 2100) {
        errors.push(`Year stats at index ${index} year must be valid (1900-2100)`)
      }
    }

    if (stat.count === undefined) {
      errors.push(`Year stats at index ${index} missing count`)
    } else if (typeof stat.count !== 'number' || stat.count < 0) {
      errors.push(`Year stats at index ${index} count must be non-negative number`)
    }

    if (stat.aCount !== undefined) {
      if (typeof stat.aCount !== 'number' || stat.aCount < 0) {
        errors.push(`Year stats at index ${index} aCount must be non-negative number`)
      }
    }

    if (stat.bCount !== undefined) {
      if (typeof stat.bCount !== 'number' || stat.bCount < 0) {
        errors.push(`Year stats at index ${index} bCount must be non-negative number`)
      }
    }

    if (stat.cCount !== undefined) {
      if (typeof stat.cCount !== 'number' || stat.cCount < 0) {
        errors.push(`Year stats at index ${index} cCount must be non-negative number`)
      }
    }

    if (stat.growth !== undefined) {
      if (typeof stat.growth !== 'number') {
        errors.push(`Year stats at index ${index} growth must be a number`)
      } else if (stat.growth < -100 || stat.growth > 1000) {
        warnings.push(`Year stats at index ${index} growth seems unusual`)
      }
    }
  })

  return errors.length > 0 ? failure(errors, warnings) : success(warnings)
}

// ============================================================================
// API RESPONSE VALIDATION
// ============================================================================

/**
 * Validates API response structure
 */
export function validateApiResponse(data: unknown): ValidationResult {
  const errors: string[] = []

  if (!data || typeof data !== 'object') {
    return failure(['API response must be an object'])
  }

  const response = data as Record<string, unknown>

  if (response.code === undefined) {
    errors.push('API response missing status code')
  } else if (typeof response.code !== 'number') {
    errors.push('API response status code must be a number')
  } else if (response.code < 100 || response.code >= 600) {
    errors.push('API response status code is invalid')
  }

  if (response.message === undefined) {
    errors.push('API response missing message')
  } else if (typeof response.message !== 'string') {
    errors.push('API response message must be a string')
  }

  if (response.data === undefined) {
    errors.push('API response missing data field')
  }

  if (response.timestamp !== undefined && typeof response.timestamp !== 'number') {
    errors.push('API response timestamp must be a number')
  }

  return errors.length > 0 ? failure(errors) : success()
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Validates multiple items and returns combined result
 */
export function validateMultiple<T>(
  items: unknown[],
  validator: (item: unknown) => ValidationResult
): ValidationResult {
  const allErrors: string[] = []
  const allWarnings: string[] = []

  items.forEach((item, index) => {
    const result = validator(item)
    if (!result.valid) {
      allErrors.push(...result.errors.map(e => `[${index}] ${e}`))
    }
    allWarnings.push(...result.warnings.map(w => `[${index}] ${w}`))
  })

  return allErrors.length > 0 ? failure(allErrors, allWarnings) : success(allWarnings)
}

/**
 * Asserts that data is valid, throws if not
 */
export function assertValid<T>(
  data: unknown,
  validator: (data: unknown) => ValidationResult,
  errorMessage?: string
): asserts data is T {
  const result = validator(data)
  if (!result.valid) {
    throw new Error(errorMessage || `Validation failed: ${result.errors.join(', ')}`)
  }
}

/**
 * Safely validates data and returns typed result or null
 */
export function safeValidate<T>(
  data: unknown,
  validator: (data: unknown) => ValidationResult
): T | null {
  const result = validator(data)
  return result.valid ? (data as T) : null
}

// ============================================================================
// EXPORTS
// ============================================================================

export default {
  // Paper validation
  validatePaper,
  validatePaperDetail,

  // Search validation
  validateSearchParams,

  // Statistics validation
  validateStatistics,
  validateJournalStats,
  validateYearStats,

  // API response validation
  validateApiResponse,

  // Utilities
  validateMultiple,
  assertValid,
  safeValidate
}
