# PaperCrawler Pinia State Management - Complete Implementation

## Overview

This document provides a comprehensive overview of the complete Pinia state management system for PaperCrawler, implemented with TypeScript, persistence, error handling, and API integration.

## Architecture

### Store Categories

1. **Core Stores** - Essential application state
2. **Feature Stores** - Domain-specific functionality
3. **UI Store** - User interface preferences

## Available Stores

### Core Stores

#### 1. Auth Store (`auth.ts`)
**Purpose**: User authentication and session management

**State**:
- `user`: Current user information
- `tokens`: Authentication tokens (access + refresh)
- `loading`: Loading state
- `error`: Error messages

**Actions**:
- `login(credentials)`: User login
- `register(data)`: User registration
- `logout()`: User logout
- `refreshAccessToken()`: Token refresh
- `fetchCurrentUser()`: Get user info
- `updateProfile(data)`: Update profile
- `changePassword(old, new)`: Change password

**Computed**:
- `isAuthenticated`: Auth status
- `isAdmin`: Admin role check
- `displayName`: User display name

**Persistence**: localStorage (user data, tokens in separate storage)

---

#### 2. Paper Store (`paperStore.ts`)
**Purpose**: Paper management, filtering, and batch operations

**State**:
- `papers`: Paper list
- `currentPaper`: Selected paper
- `filters`: Active filters
- `sort`: Sort configuration
- `selectedIds`: Selected paper IDs

**Actions**:
- `fetchPapers(params)`: Get papers with pagination
- `fetchPaper(id)`: Get single paper
- `createPaper(data)`: Create paper
- `updatePaper(id, data)`: Update paper
- `deletePaper(id)`: Delete paper
- `toggleBookmark(id)`: Toggle bookmark
- `markAsRead(id, isRead)`: Mark read/unread
- `batchDelete()`: Batch delete
- `batchMarkAsRead(isRead)`: Batch mark read

**Computed**:
- `filteredPapers`: Filtered papers
- `sortedPapers`: Sorted papers
- `bookmarkedPapers`: Bookmarked only
- `readPapers`: Read only

**Persistence**: localStorage (filters, sort, pageSize)

---

#### 3. Search Store (`searchStore.ts`)
**Purpose**: Search functionality with history and advanced search

**State**:
- `query`: Search query
- `results`: Search results
- `history`: Search history
- `advancedParams`: Advanced search parameters
- `savedSearches`: Saved searches

**Actions**:
- `search(query)`: Perform search
- `advancedSearch(params)`: Advanced search
- `fetchSuggestions(input)`: Get suggestions
- `saveSearch(name)`: Save search
- `loadSavedSearch(id)`: Load saved search

**Computed**:
- `hasResults`: Has results
- `recentSearches`: Recent searches
- `hasAdvancedFilters`: Has active filters

**Persistence**: localStorage (history, saved searches, advanced params)

---

### Feature Stores

#### 4. Crawler Store (`crawlerStore.ts`)
**Purpose**: Web crawler task management

**State**:
- `tasks`: Crawler tasks
- `templates`: Crawler templates
- `history`: Crawl history
- `config`: Crawler configuration
- `connectionStatus`: Source connection status

**Actions**:
- `startTask(request)`: Start crawler task
- `fetchTaskStatus(id)`: Get task status
- `cancelTask(id)`: Cancel task
- `pauseTask(id)`: Pause task
- `resumeTask(id)`: Resume task
- `savePapers(papers)`: Save crawled papers
- `testConnection(source)`: Test source connection

**Computed**:
- `activeTasks`: Active tasks
- `totalProgress`: Overall progress
- `availableSources`: Available sources

**Persistence**: localStorage (templates, config)

---

#### 5. Export Store (`exportStore.ts`)
**Purpose**: Paper export functionality

**State**:
- `history`: Export history
- `templates`: Export templates
- `options`: Export options
- `progress`: Export progress

**Actions**:
- `exportPapers(papers, options)`: Export papers
- `exportToCSV(papers)`: Export to CSV
- `exportToJSON(papers)`: Export to JSON
- `exportToBibTeX(papers)`: Export to BibTeX
- `createTemplate(name, format, options)`: Create template

**Computed**:
- `recentExports`: Recent exports
- `exportsByFormat`: By format
- `mostUsedFormat`: Most used format

**Persistence**: localStorage (history, templates, options)

---

#### 6. Statistics Store (`statsStore.ts`)
**Purpose**: Statistics and analytics data

**State**:
- `overview`: Overview statistics
- `journals`: Journal statistics
- `years`: Year statistics
- `authors`: Author statistics
- `readingProgress`: Reading progress
- `citationTrends`: Citation trends

**Actions**:
- `fetchAllStats()`: Fetch all statistics
- `fetchOverview()`: Fetch overview
- `fetchJournals()`: Fetch journal stats
- `fetchYears()`: Fetch year stats
- `refresh()`: Refresh all stats

**Computed**:
- `papersByYearChart`: Chart data for papers by year
- `papersByJournalChart`: Chart data for papers by journal
- `readingProgressChart`: Reading progress chart
- `growthRate`: Growth rate

**Persistence**: None (real-time data)

---

#### 7. AI Store (`aiStore.ts`)
**Purpose**: AI features (review, literature review, research plan)

**State**:
- `history`: AI generation history
- `currentGeneration`: Current generation
- `stats`: AI statistics
- `progress`: Generation progress

**Actions**:
- `generateReview(paperId)`: Generate peer review
- `generateLiteratureReview(topic, papers)`: Generate literature review
- `generateResearchPlan(topic, objectives)`: Generate research plan
- `regenerate(id)`: Regenerate from history

**Computed**:
- `recentGenerations`: Recent generations
- `totalGenerations`: Total count
- `totalTokensUsed`: Total tokens
- `totalCost`: Total cost

**Persistence**: localStorage (history, stats)

---

#### 8. Recommendation Store (`recommendationStore.ts`)
**Purpose**: Paper recommendations and user preferences

**State**:
- `personalized`: Personalized recommendations
- `similarPapers`: Similar papers
- `trending`: Trending papers
- `userProfile`: User profile
- `feedbackHistory`: Feedback history

**Actions**:
- `fetchPersonalized(limit)`: Get recommendations
- `fetchSimilar(paperId)`: Get similar papers
- `fetchTrending(period)`: Get trending
- `submitFeedback(id, type, comment)`: Submit feedback
- `updateInterests(interests)`: Update interests

**Computed**:
- `topRecommendations`: Top recommendations
- `highlyCited`: Highly cited papers
- `risingPapers`: Rising trending papers

**Persistence**: localStorage (user profile, feedback)

---

#### 9. UI Store (`uiStore.ts`)
**Purpose**: User interface preferences and state

**State**:
- `settings`: UI settings
- `notificationSettings`: Notification settings
- `loadingOverlay`: Loading overlay
- `modalStack`: Open modals
- `notificationQueue`: Active notifications

**Actions**:
- `setTheme(theme)`: Set theme (light/dark/system)
- `setLanguage(lang)`: Set language
- `toggleSidebar()`: Toggle sidebar
- `showLoading(msg)`: Show loading
- `showNotification(type, title, msg)`: Show notification
- `openModal(id)`: Open modal

**Computed**:
- `currentTheme`: Current theme (resolved)
- `isDarkMode`: Dark mode status
- `densitySpacing`: Spacing values

**Persistence**: localStorage (settings, notification settings)

---

## Usage Examples

### Basic Store Usage

```typescript
import { useAuthStore } from '@/stores'
import { storeToRefs } from 'pinia'

// In component setup
const authStore = useAuthStore()
const { user, isAuthenticated } = storeToRefs(authStore)

// Actions
await authStore.login({ email, password })

// Computed
if (isAuthenticated.value) {
  console.log(user.value)
}
```

### Paper Store Usage

```typescript
import { usePaperStore } from '@/stores'

const paperStore = usePaperStore()

// Fetch papers
await paperStore.fetchPapers({ page: 1, pageSize: 20 })

// Filter and sort
paperStore.setFilters({ category: 'AI', year: '2024' })
paperStore.setSort('year', 'desc')

// Batch operations
paperStore.selectAll()
await paperStore.batchDelete()
```

### Search Store Usage

```typescript
import { useSearchStore } from '@/stores'

const searchStore = useSearchStore()

// Simple search
await searchStore.search('machine learning')

// Advanced search
await searchStore.advancedSearch({
  title: 'deep learning',
  authors: 'Hinton',
  yearFrom: '2020',
  yearTo: '2024'
})

// Save search
searchStore.saveSearch('My Research')
```

### UI Store Usage

```typescript
import { useUIStore } from '@/stores'

const uiStore = useUIStore()

// Theme
uiStore.setTheme('dark')

// Notifications
uiStore.showNotification('success', 'Success', 'Paper saved successfully')

// Loading
uiStore.showLoading('Loading papers...')
// ... do work
uiStore.hideLoading()
```

---

## TypeScript Support

All stores are fully typed with TypeScript:

```typescript
// Import types
import type {
  Paper,
  PaperFilters,
  ExportOptions,
  AIGeneration
} from '@/stores'

// Use in components
interface Props {
  paper: Paper
  filters: PaperFilters
}
```

---

## Persistence Strategy

### localStorage
- User authentication data
- User preferences (theme, language)
- Application settings
- History data

### sessionStorage
- Temporary filters
- Form drafts
- Search session data

### Memory-only
- Real-time WebSocket data
- Sensitive data
- Temporary state

---

## Error Handling

All stores implement consistent error handling:

```typescript
try {
  await store.action()
} catch (error) {
  // Error is stored in store.error
  // Can be accessed via store.error
  console.error(error.message)
}
```

---

## Testing

Each store can be tested independently:

```typescript
import { setActivePinia, createPinia } from 'pinia'
import { useAuthStore } from '@/stores'

describe('Auth Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('should login user', async () => {
    const store = useAuthStore()
    await store.login({ email: 'test@test.com', password: 'pass' })
    expect(store.isAuthenticated).toBe(true)
  })
})
```

---

## Migration Notes

### Legacy Stores

The following legacy stores are kept for compatibility:
- `usePapersStore`
- `useLegacyStatsStore`
- `useUserStore`
- `useSyncStore`

These will be deprecated in future versions.

### Migration Guide

**From legacy to new stores**:

```typescript
// Old
const papersStore = usePapersStore()

// New
const paperStore = usePaperStore()
```

---

## Performance Optimization

1. **Lazy Loading**: Stores load data only when needed
2. **Computed Properties**: Efficient derived state
3. **Selective Persistence**: Only essential data is persisted
4. **Debouncing**: Search and filter operations are debounced
5. **Pagination**: Large datasets are paginated

---

## Best Practices

1. **Use `storeToRefs`** when destructuring in components
2. **Avoid direct mutation** - use actions
3. **Handle errors** - check store.error after actions
4. **Clean up** - call reset() when done
5. **Use composables** - extract reusable logic

---

## Future Enhancements

1. **Offline Support**: Service worker integration
2. **Real-time Updates**: WebSocket integration
3. **Data Validation**: Schema validation
4. **Optimistic Updates**: Immediate UI feedback
5. **Request Cancellation**: AbortController support

---

## Files Structure

```
stores/
├── index.ts              # Main export file
├── auth.ts              # Authentication store (existing)
├── paperStore.ts        # Paper management (NEW)
├── searchStore.ts       # Search functionality (NEW)
├── crawlerStore.ts      # Crawler management (NEW)
├── exportStore.ts       # Export functionality (NEW)
├── statsStore.ts        # Statistics & analytics (NEW)
├── aiStore.ts           # AI features (NEW)
├── recommendationStore.ts # Recommendations (NEW)
├── uiStore.ts           # UI preferences (NEW)
├── papers.ts            # Legacy paper store
├── stats.ts             # Legacy stats store
├── app.ts               # App store
├── user.ts              # User store
└── sync.ts              # Sync store
```

---

## Summary

This complete Pinia state management system provides:

- **9 New Stores** with full TypeScript support
- **Composition API** style for modern Vue 3
- **Persistence** with pinia-plugin-persistedstate
- **Error Handling** with consistent patterns
- **API Integration** with backend services
- **Computed Properties** for derived state
- **Actions** for state mutations
- **Development Tools** with logging

All stores follow best practices and are production-ready for the PaperCrawler application.

**Status**: ✅ Complete Implementation
**Version**: 1.0.0
**Last Updated**: 2026-04-04
