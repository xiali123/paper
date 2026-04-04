# API Adapters

This directory contains adapter modules that handle data transformation, validation, and error handling between the backend C++ API and the frontend TypeScript application.

## Available Adapters

### 1. Error Handling Adapter (`errorAdapter.ts`)

**Purpose**: Provides unified error handling and transformation for backend API errors with internationalization support.

**Key Features**:
- Transform various error formats (Axios, backend, network) into unified `ApiError` format
- Generate user-friendly error messages in Chinese and English
- Error type classification (NETWORK, AUTH, VALIDATION, SERVER, UNKNOWN)
- Error checking utilities (isAuthError, isNetworkError, isRetryableError, etc.)
- Request tracking and suggested user actions

**Quick Example**:
```typescript
import { transformApiError, isAuthError } from '@/api/adapters/errorAdapter'

try {
  await api.login(credentials)
} catch (error) {
  const apiError = transformApiError(error)
  console.log(apiError.userMessage)  // "用户名或密码错误"

  if (isAuthError(error)) {
    redirectToLogin()
  }
}
```

### 2. Validation Adapter (`validationAdapter.ts`)

**Purpose**: Provides runtime type validation and sanitization for backend API responses.

**Key Features**:
- Validate backend response structure for 7 data types (paper, paperList, paperDetail, stats, user, auth, search)
- Sanitize and normalize data (type conversion, trimming, null handling)
- Type guard functions for backend responses
- Detailed error paths for debugging
- Performance optimized (<5ms overhead)

**Quick Example**:
```typescript
import { validateBackendResponse, safeValidateBackendResponse } from '@/api/adapters/validationAdapter'

// Validate response
const result = validateBackendResponse<Paper>(backendData, 'paper')
if (result.valid) {
  const paper = result.data  // Type is Paper
} else {
  console.error('Validation errors:', result.errors)
}

// Safe validation with fallback
const paper = safeValidateBackendResponse<Paper>(backendData, 'paper')
if (paper) {
  displayPaper(paper)
}
```

### 3. Authentication Adapter (`authAdapter.ts`)

**Purpose**: Handles transformation between backend and frontend authentication data formats.

**Field Mappings**:
- `email` (frontend) ↔ `username` (backend)
- `fullName` (frontend) ↔ `full_name` (backend)
- Token format transformation
- User profile data normalization

**Quick Example**:
```typescript
import { transformLoginRequest, transformLoginResponse } from '@/api/adapters/authAdapter'

// Transform request
const backendRequest = transformLoginRequest({
  email: 'user@example.com',
  password: 'pass123'
})
// Result: { username: 'user@example.com', password: 'pass123' }

// Transform response
const frontendResponse = transformLoginResponse(backendResponse)
// Result: { user: {...}, tokens: {...} }
```

### 4. Paper Adapter (`paperAdapter.ts`)

Handles transformation between backend Paper objects (C++/JSON) and frontend Paper objects (TypeScript).

## Field Mappings

### Backend → Frontend

| Backend Field | Frontend Field | Type Conversion |
|--------------|---------------|-----------------|
| `journal` | `publication` | string → string |
| `is_favorite` | `isBookmarked` | boolean → boolean |
| `is_read` | `isRead` | boolean → boolean |
| `citation_count` | `citationCount` | number → number |
| `pdf_path` | `pdfPath` | string → string |
| `created_at` | `createdAt` | timestamp → ISO 8601 string |
| `updated_at` | `updatedAt` | timestamp → ISO 8601 string |
| `year` | `year` | int → string |
| `tags` | `tags` | string[] → comma-separated string |
| `keywords` | `keywords` | string[] → comma-separated string |

### Frontend → Backend

| Frontend Field | Backend Field | Type Conversion |
|---------------|---------------|-----------------|
| `publication` | `journal` | string → string |
| `isBookmarked` | `is_favorite` | boolean → boolean |
| `isRead` | `is_read` | boolean → boolean |
| `citationCount` | `citation_count` | number → number |
| `pdfPath` | `pdf_path` | string → string |
| `year` | `year` | string → int |
| `tags` | `tags` | comma-separated string → string[] |
| `keywords` | `keywords` | comma-separated string → string[] |

### Frontend-Specific Fields (Not Sent to Backend)

- `userId` - Managed by backend
- `source` - Not yet supported by backend
- `category` - Not yet supported by backend
- `readingProgress` - Not yet supported by backend

## API Reference

### Transformation Functions

#### `toFrontendPaper(backendPaper: BackendPaper): FrontendPaper`

Converts a backend Paper object to frontend format.

**Example:**
```typescript
const backendPaper = {
  id: 1,
  title: 'Deep Learning',
  journal: 'Nature',
  year: 2024,
  tags: ['AI', 'ML'],
  is_favorite: true,
  // ... other fields
}

const frontendPaper = toFrontendPaper(backendPaper)
// Result:
// {
//   id: 1,
//   title: 'Deep Learning',
//   publication: 'Nature',
//   year: '2024',
//   tags: 'AI, ML',
//   isBookmarked: true,
//   // ... other fields
// }
```

#### `transformPaperList(backendPapers: BackendPaper[]): FrontendPaper[]`

Converts an array of backend Papers to frontend format.

**Example:**
```typescript
const backendPapers = [/* ... */]
const frontendPapers = transformPaperList(backendPapers)
```

#### `toBackendPaper(frontendPaper: Partial<FrontendPaper>): BackendPaperRequest`

Converts a frontend Paper object to backend request format.

**Example:**
```typescript
const frontendPaper = {
  title: 'Deep Learning',
  publication: 'Nature',
  year: '2024',
  tags: 'AI, ML',
  isBookmarked: true
}

const backendRequest = toBackendPaper(frontendPaper)
// Result:
// {
//   title: 'Deep Learning',
//   journal: 'Nature',
//   year: 2024,
//   tags: ['AI', 'ML'],
//   is_favorite: true
// }
```

#### `transformCreateRequest(data: CreatePaperRequest): BackendPaperRequest`

Transforms a create paper request to backend format with validation.

**Example:**
```typescript
const createRequest = {
  title: 'New Paper',
  authors: 'John Doe',
  publication: 'IEEE Transactions'
}

const backendRequest = transformCreateRequest(createRequest)
// Throws error if title is missing or empty
```

#### `transformUpdateRequest(data: UpdatePaperRequest): BackendPaperRequest`

Transforms an update paper request to backend format.

**Example:**
```typescript
const updateRequest = {
  title: 'Updated Title',
  isRead: true
}

const backendRequest = transformUpdateRequest(updateRequest)
// Only includes defined fields
```

#### `transformQueryParams(params: PaperQueryParams): Record<string, any>`

Transforms frontend query parameters to backend format.

**Example:**
```typescript
const queryParams = {
  keyword: 'machine learning',
  isBookmarked: true,
  orderBy: 'createdAt',
  order: 'DESC',
  page: 1,
  pageSize: 20
}

const backendParams = transformQueryParams(queryParams)
// Result:
// {
//   query: 'machine learning',
//   is_favorite: true,
//   sort_by: 'created_at',
//   ascending: false,
//   page: 1,
//   limit: 20
// }
```

## Usage in papers.ts

The adapter is integrated into the papers API module:

```typescript
import {
  toFrontendPaper,
  transformPaperList,
  transformCreateRequest,
  transformUpdateRequest,
  transformQueryParams
} from '@/api/adapters/paperAdapter'

// Get papers with transformed query params and response
async getPapers(params: PaperQuery = {}): Promise<PaperListResponse> {
  const backendParams = transformQueryParams(params)
  const response = await request.get('/papers', { params: backendParams })
  return {
    ...response,
    papers: transformPaperList(response.papers)
  }
}

// Create paper with transformed request and response
async createPaper(data: CreatePaperRequest): Promise<Paper> {
  const backendRequest = transformCreateRequest(data)
  const backendPaper = await request.post('/papers', backendRequest)
  return toFrontendPaper(backendPaper)
}

// Update paper with transformed request and response
async updatePaper(id: number, data: UpdatePaperRequest): Promise<Paper> {
  const backendRequest = transformUpdateRequest(data)
  const backendPaper = await request.put(`/papers/${id}`, backendRequest)
  return toFrontendPaper(backendPaper)
}
```

## Data Validation

### Year Conversion
- Frontend → Backend: String to integer, invalid strings become `0`
- Backend → Frontend: Integer to string

### Array/String Conversion
- Frontend → Backend: Comma-separated string to array, empty string becomes `[]`
- Backend → Frontend: Array to comma-separated string, empty array becomes `''`
- Whitespace is trimmed and empty strings are filtered

### Timestamp Handling
- Backend timestamps can be Unix timestamps or ISO 8601 strings
- Frontend always receives ISO 8601 strings
- Invalid timestamps default to current time

### Required Fields
- `createPaper()` requires `title` field (throws error if missing or empty)
- All other fields are optional

## Testing

Run tests with:
```bash
npm test src/api/adapters/__tests__/paperAdapter.test.ts
```

Test coverage includes:
- Single and bulk transformations
- Field name mappings
- Type conversions
- Edge cases (empty strings, invalid data)
- Validation errors

## Architecture Benefits

1. **Separation of Concerns**: Data transformation logic is isolated from API calls
2. **Type Safety**: Full TypeScript support with proper type definitions
3. **Maintainability**: Easy to update mappings when backend schema changes
4. **Testability**: Pure functions that are easy to test
5. **Reusability**: Adapter functions can be used across different modules

## Future Enhancements

1. **Validation**: Add more comprehensive validation rules
2. **Sanitization**: Add HTML sanitization for text fields
3. **Normalization**: Add consistent string normalization (trim, lowercase, etc.)
4. **Backend Support**: Add support for `source`, `category`, and `readingProgress` when backend implements them
5. **Error Handling**: More detailed error messages for validation failures

## Related Files

- `E:\PaperCrawler\backend\include\business\PaperApiModule.hpp` - Backend Paper structure
- `E:\PaperCrawler\frontend\src\api\modules\papers.ts` - Papers API module
- `E:\PaperCrawler\frontend\src\api\adapters\__tests__\paperAdapter.test.ts` - Adapter tests
