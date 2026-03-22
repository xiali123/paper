# PaperCrawler Frontend Type System

Comprehensive TypeScript type definitions for the PaperCrawler frontend application.

## Overview

This type system provides complete type safety for the PaperCrawler frontend, ensuring type consistency between the frontend and backend C++ API. It includes:

- Core domain types (Paper, Statistics, etc.)
- API request/response types
- Type guards and validation functions
- Utility types for common patterns
- Comprehensive JSDoc documentation

## File Structure

```
src/types/
├── index.ts         # Main type definitions and exports
├── validation.ts    # Runtime validation functions
└── README.md        # This file
```

## Core Types

### Paper Types

#### `Paper`
Core paper interface representing academic paper data.

```typescript
interface Paper {
  id: number | string
  title: string
  journal: JournalInfo | string
  year: string | number
  level: CCFLevel | string
  authors: string
  urls?: PaperUrls
  abstract?: string
  keywords?: string[]
  citations?: number
  references?: number
  downloadCount?: number
}
```

#### `PaperDetail`
Extended paper interface with additional details for detail views.

```typescript
interface PaperDetail extends Paper {
  citations: number
  references: number
  downloadCount: number
  relatedPapers?: Paper[]
  publicationDate?: string
  doi?: string
  pages?: string
  volume?: string
  issue?: string
  publisher?: string
}
```

#### `PaperListItem`
Simplified interface optimized for list views and tables.

```typescript
interface PaperListItem {
  id: number | string
  title: string
  journal: string
  year: string | number
  level: string
  authors: string
}
```

### Search Types

#### `SearchParams`
Parameters for paper search API.

```typescript
interface SearchParams {
  q: string                    // Search query (required)
  year?: string               // Filter by year
  level?: string              // Filter by CCF level
  offset?: number             // Pagination offset
  limit?: number              // Results per page
  sort?: string               // Sort field
  order?: SortOrder           // Sort direction
  journal?: string            // Filter by journal
  author?: string             // Filter by author
}
```

#### `SearchResult`
Search response containing papers and metadata.

```typescript
interface SearchResult {
  papers: Paper[]
  total: number
  keyword: string
  duration: number
  page?: number
  pageSize?: number
  offset?: number
  limit?: number
}
```

### Statistics Types

#### `Statistics`
Overview statistics for dashboard.

```typescript
interface Statistics {
  totalPapers: number
  totalJournals: number
  topTierPapers: number
  papersLastYear: number
  mostActiveJournal: string
  averagePapersPerYear: number
  latestUpdate?: string
  yearRange?: string
}
```

#### `JournalStats`
Per-journal publication statistics.

```typescript
interface JournalStats {
  journal: string
  count: number
  level: string
  percentage?: number
  rank?: number
}
```

#### `YearStats`
Per-year publication statistics.

```typescript
interface YearStats {
  year: string
  count: number
  aCount?: number
  bCount?: number
  cCount?: number
  growth?: number
}
```

#### `AuthorStats`
Per-author statistics.

```typescript
interface AuthorStats {
  author: string
  count: number
  totalCitations?: number
  avgLevel?: string
  topJournal?: string
}
```

### API Response Types

#### `ApiResponse<T>`
Standard API response wrapper.

```typescript
interface ApiResponse<T = any> {
  code: number
  message: string
  data: T
  timestamp?: number
}
```

#### `PaginatedResponse<T>`
Paginated collection response.

```typescript
interface PaginatedResponse<T> {
  items: T[]
  total: number
  page: number
  pageSize: number
  totalPages: number
  hasNext: boolean
  hasPrevious: boolean
}
```

#### `ApiError`
Error response structure.

```typescript
interface ApiError {
  success: false
  error: string
  code?: string
  statusCode?: number
  timestamp: number
  requestId?: string
}
```

### State Management Types

#### `LoadingState`
Async operation state.

```typescript
interface LoadingState {
  loading: boolean
  error: string | null
  lastUpdated?: number
}
```

#### `PaperListState`
Paper list with pagination.

```typescript
interface PaperListState extends LoadingState {
  papers: Paper[]
  total: number
  page: number
  pageSize: number
  hasMore: boolean
}
```

## Enums

### `CCFLevel`
CCF publication level classification.

```typescript
enum CCFLevel {
  A = 'A',  // Top-tier
  B = 'B',  // High-quality
  C = 'C'   // Standard
}
```

### `SortOrder`
Sort order for results.

```typescript
enum SortOrder {
  ASC = 'asc',
  DESC = 'desc'
}
```

### `ApiStatusCode`
HTTP status codes.

```typescript
enum ApiStatusCode {
  SUCCESS = 200,
  BAD_REQUEST = 400,
  NOT_FOUND = 404,
  INTERNAL_ERROR = 500
}
```

## Type Guards

Type guards provide runtime type checking:

```typescript
import { isPaper, isPaperDetail, isValidCCFLevel } from '@/types'

// Check if value is a Paper
if (isPaper(data)) {
  console.log(data.title) // TypeScript knows this is safe
}

// Check if value is a PaperDetail
if (isPaperDetail(data)) {
  console.log(data.citations) // Additional fields available
}

// Check CCF level
if (isValidCCFLevel(level)) {
  // level is typed as CCFLevel
}
```

## Validation Functions

Runtime validation for API responses and user input:

```typescript
import {
  validatePaper,
  validateSearchParams,
  validateStatistics
} from '@/types/validation'

// Validate paper data
const result = validatePaper(apiResponse)
if (!result.valid) {
  console.error('Validation errors:', result.errors)
  console.warn('Warnings:', result.warnings)
}

// Validate search parameters
const searchResult = validateSearchParams({ q: 'machine learning' })
if (searchResult.valid) {
  // Parameters are valid
}
```

### Validation Result

All validation functions return a `ValidationResult`:

```typescript
interface ValidationResult {
  valid: boolean           // True if validation passed
  errors: string[]         // Critical errors that must be fixed
  warnings: string[]       // Non-critical issues
}
```

## Utility Types

### Type Modifiers

```typescript
// Deep partial - makes all properties optional recursively
type DeepPartial<T> = { [P in keyof T]?: ... }

// Deep required - makes all properties required recursively
type DeepRequired<T> = { [P in keyof T]-?: ... }

// Mutable - converts readonly properties to mutable
type Mutable<T> = { -readonly [P in keyof T]: T[P] }

// ArrayElement - extracts array element type
type ArrayElement<T> = T extends (infer U)[] ? U : never
```

### Type Converters

Helper functions to convert between type formats:

```typescript
import {
  paperToListItem,
  journalToString,
  yearToString,
  searchParamsToQuery
} from '@/types'

// Convert Paper to PaperListItem
const listItem = paperToListItem(paper)

// Normalize journal info to string
const journalName = journalToString(paper.journal)

// Convert search params to query string
const queryString = searchParamsToQuery(params)
```

## Usage Examples

### Basic Usage

```typescript
import type { Paper, SearchParams, SearchResult } from '@/types'

// Type a variable
const paper: Paper = {
  id: '12345',
  title: 'Deep Learning',
  journal: { full: 'CVPR', short: 'CVPR' },
  year: '2024',
  level: CCFLevel.A,
  authors: 'John Doe, Jane Smith'
}

// Type API parameters
const params: SearchParams = {
  q: 'machine learning',
  year: '2024',
  level: CCFLevel.A,
  offset: 0,
  limit: 20
}
```

### With API Calls

```typescript
import { paperApi } from '@/api'
import type { Paper, SearchResult } from '@/types'

// Search papers
const searchResult: SearchResult = await paperApi.search({
  q: 'deep learning',
  limit: 10
})

// Get paper detail
const paper: Paper = await paperApi.getById('12345')
```

### With Validation

```typescript
import { validatePaper } from '@/types/validation'
import type { Paper } from '@/types'

// Validate API response
const paperData = await fetchPaper()
const validation = validatePaper(paperData)

if (!validation.valid) {
  console.error('Invalid paper data:', validation.errors)
  throw new Error('Invalid data')
}

// Now TypeScript knows it's a valid Paper
const paper: Paper = paperData
```

### With Type Guards

```typescript
import { isPaper, isPaperDetail } from '@/types'

function displayPaper(data: unknown) {
  if (isPaper(data)) {
    console.log(data.title) // Safe access

    if (isPaperDetail(data)) {
      console.log(data.citations) // Additional fields
    }
  }
}
```

## Best Practices

### 1. Use Specific Types

```typescript
// Good - specific type
function getPaper(id: string): Promise<Paper> { ... }

// Avoid - overly generic
function getPaper(id: string): Promise<any> { ... }
```

### 2. Leverage Type Guards

```typescript
// Good - type guard for runtime safety
function processPaper(data: unknown) {
  if (isPaper(data)) {
    console.log(data.title)
  }
}

// Avoid - assuming type
function processPaper(data: unknown) {
  console.log((data as Paper).title) // Unsafe
}
```

### 3. Validate External Data

```typescript
// Good - validate API responses
const data = await fetchData()
const result = validatePaper(data)
if (!result.valid) {
  throw new Error('Invalid data')
}

// Avoid - trusting API blindly
const paper = await fetchData() as Paper // Risky
```

### 4. Use Utility Functions

```typescript
// Good - use provided converters
const item = paperToListItem(paper)

// Avoid - manual conversion
const item = {
  id: paper.id,
  title: paper.title,
  // ... manual mapping
}
```

### 5. Document Complex Types

```typescript
// Good - documented interface
/**
 * Paper with extended citation information
 * Used for detail views and analytics
 */
interface PaperDetail extends Paper {
  /** Number of citations from Google Scholar */
  citations: number
  /** Number of references in bibliography */
  references: number
}

// Avoid - undocumented complex types
interface PaperDetail extends Paper {
  citations: number
  references: number
  // What do these numbers mean?
}
```

## Migration Guide

### From Old Types

If you're migrating from the old type system:

1. **Update imports**:
   ```typescript
   // Old
   import type { Paper } from '@/types/paper'

   // New
   import type { Paper } from '@/types'
   ```

2. **Update paper ID type**:
   ```typescript
   // Old - assumed string
   id: string

   // New - supports both
   id: number | string
   ```

3. **Update journal field**:
   ```typescript
   // Old - always string
   journal: string

   // New - can be detailed or simple
   journal: JournalInfo | string
   ```

4. **Add validation**:
   ```typescript
   // New - add runtime validation
   import { validatePaper } from '@/types/validation'

   const result = validatePaper(data)
   if (!result.valid) {
     // Handle errors
   }
   ```

## API Module Integration

The type system is fully integrated with API modules:

```typescript
// All API modules are fully typed
import { paperApi, statsApi, exportApi } from '@/api'

// TypeScript infers return types
const papers = await paperApi.search({ q: 'test' })
//    ^? SearchResult

const stats = await statsApi.getOverview()
//    ^? Statistics
```

## Testing

The type system includes comprehensive validation for testing:

```typescript
import { validatePaper, validateSearchParams } from '@/types/validation'

describe('Paper validation', () => {
  it('should validate correct paper', () => {
    const result = validatePaper(validPaper)
    expect(result.valid).toBe(true)
  })

  it('should reject invalid paper', () => {
    const result = validatePaper(invalidPaper)
    expect(result.valid).toBe(false)
    expect(result.errors).toContain('Paper title is required')
  })
})
```

## Contributing

When adding new types:

1. Add comprehensive JSDoc comments
2. Create validation functions in `validation.ts`
3. Add type guards if needed
4. Include usage examples in documentation
5. Update this README

## License

MIT License - see project root for details.
