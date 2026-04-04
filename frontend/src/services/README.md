# PaperCrawler API Service Layer

Complete TypeScript API service layer for PaperCrawler frontend, connecting to 94 backend endpoints across 9 modules.

## Overview

This directory contains the complete API service layer implementation for the PaperCrawler frontend application. Each service module provides a clean, type-safe interface to communicate with the backend REST API.

## Architecture

```
src/services/
├── axios.ts                    # Axios HTTP client configuration
├── index.ts                    # Central export file
├── authApi.ts                  # Authentication service (9 endpoints)
├── userApi.ts                  # User management service (13 endpoints)
├── paperApi.ts                 # Paper CRUD service (11 endpoints)
├── searchApi.ts                # Search service (6 endpoints)
├── exportApi.ts                # Export service (6 endpoints)
├── statsApi.ts                 # Statistics service (7 endpoints)
├── aiApi.ts                    # AI features service (7 endpoints)
├── recommendationApi.ts        # Recommendation service (6 endpoints)
└── crawlerApi.ts               # Crawler service (29 endpoints)
```

## Features

### Core Infrastructure

- **Axios Instance**: Configured HTTP client with interceptors
- **Type Safety**: Full TypeScript support with comprehensive type definitions
- **Error Handling**: Centralized error handling with user-friendly messages
- **Token Management**: Automatic JWT token refresh and management
- **Request Cancellation**: Built-in support for cancelling requests
- **Request Tracking**: Unique request IDs for debugging

### API Services

1. **authApi** - Authentication & Authorization
   - User registration, login, logout
   - Token management and refresh
   - Password reset and email verification
   - Session management

2. **userApi** - User Management
   - User profile management
   - User settings and preferences
   - User statistics and activity
   - Saved papers management

3. **paperApi** - Paper Management
   - Paper CRUD operations
   - Paper search and filtering
   - Bookmark management
   - Reading progress tracking
   - Citations and references

4. **searchApi** - Search Functionality
   - Simple and advanced search
   - Search suggestions
   - Trending searches
   - Search history

5. **exportApi** - Export Features
   - Export in multiple formats (CSV, JSON, BibTeX, EndNote, XML)
   - Export job management
   - Export history tracking

6. **statsApi** - Statistics & Monitoring
   - System statistics
   - Resource usage
   - Module status
   - Performance metrics
   - Real-time monitoring

7. **aiApi** - AI Features
   - AI peer review
   - Literature review generation
   - Research planning
   - Keyword extraction
   - Similar papers analysis

8. **recommendationApi** - Recommendations
   - Personalized recommendations
   - Trending papers
   - Similar papers
   - Feedback management

9. **crawlerApi** - Web Crawler
   - Template management
   - Task management
   - Distributed crawling
   - Real-time progress tracking
   - Statistics and monitoring

## Usage Examples

### Basic Usage

```typescript
import { api } from '@/services'

// Authentication
const loginResponse = await api.auth.login({
  username: 'user@example.com',
  password: 'password'
})

// Get papers
const papers = await api.papers.getAll({
  page: 1,
  pageSize: 20,
  search: 'machine learning'
})

// Create paper
const newPaper = await api.papers.create({
  title: 'Deep Learning for NLP',
  authors: ['John Doe', 'Jane Smith'],
  abstract: 'This paper explores...',
  year: 2024
})
```

### Advanced Usage

```typescript
import { paperApi, searchApi, crawlerApi } from '@/services'

// Advanced search
const results = await searchApi.advancedSearch({
  title: 'machine learning',
  authors: ['John Doe'],
  yearRange: { from: 2020, to: 2024 },
  tags: ['AI', 'deep learning'],
  page: 1,
  pageSize: 10
})

// Create and run crawler task
const task = await crawlerApi.createTask({
  templateId: 1,
  name: 'ArXiv AI Papers',
  maxPapers: 100,
  searchQuery: 'artificial intelligence',
  yearRange: { from: 2023, to: 2024 }
})

// Export papers
const exportJob = await exportApi.createExport({
  format: 'bibtex',
  filters: { tags: ['AI'] },
  includeAbstract: true,
  includeNotes: true
})
```

### Error Handling

```typescript
import { authApi } from '@/services'
import { ElMessage } from 'element-plus'

try {
  const response = await authApi.login({
    username: 'user@example.com',
    password: 'password'
  })

  // Handle success
  console.log('Login successful', response.data)
} catch (error) {
  // Error is automatically handled by axios interceptor
  // Custom error handling if needed
  console.error('Login failed:', error)
}
```

### Token Management

```typescript
import { httpUtils } from '@/services'

// Set tokens (auto-handled by authApi.login)
httpUtils.setToken(accessToken, refreshToken)

// Check authentication status
if (httpUtils.isAuthenticated()) {
  // User is logged in
}

// Clear tokens (auto-handled by authApi.logout)
httpUtils.clearAuth()

// Get current token
const token = httpUtils.getToken()
```

## Type Definitions

All API types are defined in `src/types/api.ts`:

```typescript
import type {
  ApiResponse,
  PaginatedResponse,
  User,
  Paper,
  PaperQuery,
  SearchQuery,
  CrawlerTask,
  // ... many more types
} from '@/types/api'
```

## Configuration

### Environment Variables

Create `.env.development` and `.env.production` files:

```bash
# API Base URL
VITE_API_BASE_URL=http://localhost:8080

# API Timeout (ms)
VITE_API_TIMEOUT=30000
```

### Axios Configuration

The Axios instance in `axios.ts` can be customized:

- `baseURL`: API base URL
- `timeout`: Request timeout
- `headers`: Default headers
- `withCredentials`: Include credentials

## Error Handling

All API errors are handled by the global error handler in `axios.ts`:

- **401 Unauthorized**: Auto token refresh, redirect to login
- **403 Forbidden**: Permission denied message
- **404 Not Found**: Resource not found message
- **422 Validation Error**: Validation error details
- **500 Server Error**: Server error message

Error messages are internationalized (English/Chinese).

## Request/Response Interceptors

### Request Interceptor

- Adds JWT token to all requests
- Adds unique request ID
- Tracks request start time

### Response Interceptor

- Handles token refresh on 401 errors
- Transforms response data
- Calculates response times
- Handles errors globally

## Best Practices

1. **Always use the API services** instead of direct axios calls
2. **Leverage TypeScript types** for type safety
3. **Handle errors** with try-catch blocks
4. **Use pagination** for large datasets
5. **Implement loading states** in UI components
6. **Cache responses** when appropriate
7. **Cancel requests** when component unmounts

## Testing

```typescript
import { describe, it, expect } from 'vitest'
import { authApi, paperApi } from '@/services'

describe('API Services', () => {
  it('should login user', async () => {
    const response = await authApi.login({
      username: 'test@example.com',
      password: 'password'
    })
    expect(response.data).toBeDefined()
  })

  it('should get papers', async () => {
    const response = await paperApi.getAll()
    expect(response.data.items).toBeDefined()
  })
})
```

## Performance Optimization

- **Request Cancellation**: Cancel requests when navigating away
- **Debouncing**: Debounce search inputs
- **Caching**: Implement response caching for frequent requests
- **Pagination**: Use pagination for large datasets
- **Virtual Scrolling**: Use virtual scrolling for long lists

## Integration with Pinia Stores

```typescript
// stores/papers.ts
import { defineStore } from 'pinia'
import { paperApi } from '@/services'
import type { Paper, PaperQuery } from '@/types/api'

export const usePaperStore = defineStore('papers', {
  state: () => ({
    papers: [] as Paper[],
    loading: false
  }),

  actions: {
    async fetchPapers(params?: PaperQuery) {
      this.loading = true
      try {
        const response = await paperApi.getAll(params)
        this.papers = response.data.items
      } finally {
        this.loading = false
      }
    }
  }
})
```

## Documentation

For more information on:
- Backend API endpoints: See `API_TEST_COMPLETE_REPORT.md`
- Frontend architecture: See `docs/FRONTEND_ARCHITECTURE_UX_FOUNDATION.md`
- Type definitions: See `src/types/api.ts`

## Support

For issues or questions:
1. Check the API test report for endpoint status
2. Review type definitions in `src/types/api.ts`
3. Check browser console for error details
4. Verify backend API is running

## License

This code is part of the PaperCrawler project.
