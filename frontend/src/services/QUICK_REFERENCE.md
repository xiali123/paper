# API Service Layer - Quick Reference Guide

## Import Options

```typescript
// Option 1: Import individual services
import { authApi, paperApi, searchApi } from '@/services'

// Option 2: Import all services as object
import { api } from '@/services'

// Option 3: Import types
import type { Paper, User, ApiResponse } from '@/services'
```

## Service Quick Reference

### Authentication (authApi)
```typescript
// Login
await authApi.login({ username, password, rememberMe })

// Register
await authApi.register({ username, email, password, fullName })

// Logout
await authApi.logout()

// Get current user
await authApi.me()

// Refresh token
await authApi.refreshToken({ refreshToken })
```

### Users (userApi)
```typescript
// Get current user
await userApi.getMe()

// Update profile
await userApi.updateMe({ fullName, avatar })

// Get statistics
await userApi.getStats()

// Change password
await userApi.changePassword(id, { currentPassword, newPassword })

// Get saved papers
await userApi.getSavedPapers(id, { page, pageSize })
```

### Papers (paperApi)
```typescript
// Get all papers
await paperApi.getAll({ page, pageSize, search, tags })

// Get paper by ID
await paperApi.getById(id)

// Create paper
await paperApi.create({ title, authors, abstract, year })

// Update paper
await paperApi.update(id, { isRead, readingProgress })

// Delete paper
await paperApi.delete(id)

// Toggle favorite
await paperApi.addFavorite(id)
await paperApi.removeFavorite(id)

// Get citations/references
await paperApi.getCitations(id)
await paperApi.getReferences(id)

// Get related papers
await paperApi.getRelated(id, { limit })
```

### Search (searchApi)
```typescript
// Simple search
await searchApi.search({ q, page, pageSize })

// Advanced search
await searchApi.advancedSearch({
  title, authors, yearRange, tags, operator
})

// Get suggestions
await searchApi.suggest({ q, limit })

// Get trending
await searchApi.trending({ limit })

// Get history
await searchApi.getHistory({ page, pageSize })
```

### Export (exportApi)
```typescript
// Create export
await exportApi.createExport({
  format, paperIds, filters, includeAbstract
})

// Get all exports
await exportApi.getAll({ page, pageSize })

// Get formats
await exportApi.getFormats()

// Download export
const blob = await exportApi.download(id)

// Delete export
await exportApi.delete(id)
```

### Statistics (statsApi)
```typescript
// System stats
await statsApi.getSystemStats()

// Resource usage
await statsApi.getResourceStats()

// Uptime
await statsApi.getUptime()

// Module status
await statsApi.getModules()
await statsApi.getModuleByName(name)

// Performance
await statsApi.getPerformance()

// Real-time
await statsApi.getRealtime()
```

### AI (aiApi)
```typescript
// Generate review
await aiApi.summarize({ paperId, title, abstract })

// Literature review
await aiApi.chat({ topic, paperIds, maxPapers })

// Extract keywords
await aiApi.extractKeywords({ paperId, maxKeywords })

// Similar papers
await aiApi.similarPapers({ paperId, maxPapers })

// Analyze citations
await aiApi.analyzeCitations({ paperId })

// Research plan
await aiApi.generateTitle({ topic, objectives })

// Service status
await aiApi.getStatus()

// Generation history
await aiApi.getHistory({ page, pageSize })

// Usage stats
await aiApi.getStats()
```

### Recommendations (recommendationApi)
```typescript
// Personalized
await recommendationApi.getPapers({ limit, excludeRead })

// Trending
await recommendationApi.getTrending({ period, limit })

// User recommendations
await recommendationApi.getUserRecommendations(userId, { limit })

// Submit feedback
await recommendationApi.submitFeedback(userId, {
  recommendationId, feedback, reason
})

// Dismiss
await recommendationApi.dismiss(userId, { recommendationId, reason })

// History
await recommendationApi.getHistory(userId, { page, pageSize })
```

### Crawler (crawlerApi)

#### Templates
```typescript
// Create template
await crawlerApi.createTemplate({ name, source, config })

// Get templates
await crawlerApi.getTemplates({ page, pageSize })

// Get template
await crawlerApi.getTemplateById(id)

// Update template
await crawlerApi.updateTemplate(id, data)

// Delete template
await crawlerApi.deleteTemplate(id)

// Test template
await crawlerApi.testTemplate(id, { maxPapers })

// Import/Export
await crawlerApi.importTemplate({ file, name })
await crawlerApi.exportTemplate(id, format)
```

#### Tasks
```typescript
// Create task
await crawlerApi.createTask({
  templateId, name, maxPapers, searchQuery
})

// Get tasks
await crawlerApi.getTasks({ page, pageSize, status })

// Get task
await crawlerApi.getTaskById(id)

// Update task
await crawlerApi.updateTask(id, data)

// Delete task
await crawlerApi.deleteTask(id)

// Control task
await crawlerApi.pauseTask(id)
await crawlerApi.resumeTask(id)
await crawlerApi.cancelTask(id)

// Get results
await crawlerApi.getTaskResults(id, { page, pageSize })

// Retry failed
await crawlerApi.retryTask(id)
```

#### Distributed
```typescript
// Get status
await crawlerApi.getDistributedStatus()

// Get nodes
await crawlerApi.getNodes({ page, pageSize })

// Get node
await crawlerApi.getNodeById(id)

// Add node
await crawlerApi.addNode({ address, port })

// Remove node
await crawlerApi.removeNode(id)

// Distribute task
await crawlerApi.distributeTask(taskId, { nodeIds })

// Get progress
await crawlerApi.getDistributedTaskProgress(taskId)
```

#### Statistics
```typescript
// General stats
await crawlerApi.getStats()

// Task stats
await crawlerApi.getTaskStats({ startDate, endDate })

// Template stats
await crawlerApi.getTemplateStats()
```

## Common Patterns

### Pagination
```typescript
const response = await paperApi.getAll({
  page: 1,
  pageSize: 20,
  sortBy: 'createdAt',
  sortOrder: 'desc'
})

const { items, total, page, pageSize } = response.data
```

### Error Handling
```typescript
try {
  const response = await paperApi.getById(id)
  // Handle success
} catch (error) {
  // Error auto-handled by axios interceptor
  // Additional handling if needed
}
```

### Request Cancellation
```typescript
const controller = new AbortController()

try {
  await paperApi.getAll({}, { signal: controller.signal })
} finally {
  controller.abort()
}
```

### File Download
```typescript
const blob = await exportApi.download(id)
const url = URL.createObjectURL(blob)
const a = document.createElement('a')
a.href = url
a.download = 'export.bib'
a.click()
```

## Type Reference

### Common Types
```typescript
// API Response
interface ApiResponse<T> {
  success: boolean
  message?: string
  data: T
}

// Paginated Response
interface PaginatedResponse<T> {
  items: T[]
  total: number
  page: number
  pageSize: number
  totalPages: number
  hasMore: boolean
}
```

### Main Entities
```typescript
// User
interface User {
  id: number
  username: string
  email: string
  fullName?: string
  role: 'admin' | 'user'
}

// Paper
interface Paper {
  id: number
  title: string
  authors: string[]
  abstract?: string
  year?: number
  publication?: string
  isRead?: boolean
  isBookmarked?: boolean
}

// Crawler Task
interface CrawlerTask {
  id: number
  status: 'pending' | 'running' | 'completed'
  progress: number
  totalPapers: number
  crawledPapers: number
}
```

## Environment Variables

```bash
# .env.development
VITE_API_BASE_URL=http://localhost:8080
VITE_API_TIMEOUT=30000

# .env.production
VITE_API_BASE_URL=https://api.papercrawler.com
VITE_API_TIMEOUT=30000
```

## Utility Functions

```typescript
import { httpUtils } from '@/services'

// Check authentication
if (httpUtils.isAuthenticated()) {
  // User is logged in
}

// Get token
const token = httpUtils.getToken()

// Set tokens
httpUtils.setToken(accessToken, refreshToken)

// Clear auth
httpUtils.clearAuth()

// Create abort controller
const controller = httpUtils.createController()

// Cancel request
httpUtils.cancelRequest(controller)
```

## Best Practices

1. **Always use services** - Never use axios directly
2. **Handle errors** - Use try-catch blocks
3. **Type safety** - Use TypeScript types
4. **Loading states** - Show loading indicators
5. **Pagination** - Use for large datasets
6. **Cancellation** - Cancel requests on unmount
7. **Caching** - Cache responses when appropriate

## Quick Examples

### Login Flow
```typescript
const response = await authApi.login({ username, password })
const { accessToken, user } = response.data
// Token auto-stored
```

### Get Papers with Filters
```typescript
const papers = await paperApi.getAll({
  search: 'AI',
  year: 2024,
  tags: ['machine learning'],
  page: 1,
  pageSize: 20
})
```

### Search and Export
```typescript
// Search
const results = await searchApi.search({ q: 'deep learning' })

// Export results
const job = await exportApi.createExport({
  format: 'bibtex',
  paperIds: results.data.items.map(r => r.paper.id)
})
```

### Crawler Workflow
```typescript
// Create task
const task = await crawlerApi.createTask({
  templateId: 1,
  maxPapers: 100
})

// Monitor progress
while (task.status === 'running') {
  await new Promise(r => setTimeout(r, 2000))
  const status = await crawlerApi.getTaskById(task.id)
  Object.assign(task, status.data)
  console.log(`Progress: ${task.progress}%`)
}
```

## Support

For detailed examples, see `examples.ts`
For full documentation, see `README.md`
For type definitions, see `types/api.ts`
