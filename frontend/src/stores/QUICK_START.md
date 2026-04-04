# Pinia Stores Quick Start Guide

## Installation

The stores are already set up in the application. Just import and use:

```typescript
import { useAuthStore, usePaperStore, useSearchStore } from '@/stores'
```

## Store Reference

### 📦 Core Stores

#### `useAuthStore()`
Authentication and user management
```typescript
const authStore = useAuthStore()

// Login
await authStore.login({ email, password })

// Check auth
if (authStore.isAuthenticated) {
  console.log(authStore.user)
}

// Logout
await authStore.logout()
```

#### `usePaperStore()`
Paper management with filtering and sorting
```typescript
const paperStore = usePaperStore()

// Fetch papers
await paperStore.fetchPapers({ page: 1, pageSize: 20 })

// Filter papers
paperStore.setFilters({ category: 'AI', year: '2024' })

// Sort papers
paperStore.setSort('year', 'desc')

// Batch operations
paperStore.selectAll()
await paperStore.batchMarkAsRead(true)

// Individual operations
await paperStore.toggleBookmark(123)
await paperStore.markAsRead(123, true)
```

#### `useSearchStore()`
Search with history and saved searches
```typescript
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

// Load saved search
searchStore.loadSavedSearch('search_id')
```

### 🎯 Feature Stores

#### `useCrawlerStore()`
Web crawler task management
```typescript
const crawlerStore = useCrawlerStore()

// Start crawler
const task = await crawlerStore.startTask({
  query: 'machine learning',
  source: 'arxiv',
  limit: 100
})

// Monitor progress
crawlerStore.fetchTaskStatus(task.id)

// Control task
await crawlerStore.pauseTask(task.id)
await crawlerStore.resumeTask(task.id)
await crawlerStore.cancelTask(task.id)
```

#### `useExportStore()`
Paper export functionality
```typescript
const exportStore = useExportStore()

// Export to CSV
await exportStore.exportToCSV(papers)

// Export to BibTeX
await exportStore.exportToBibTeX(papers)

// Custom export
await exportStore.exportPapers(papers, {
  format: 'json',
  includeAbstract: true,
  includeNotes: true
})

// Create template
exportStore.createTemplate('My Template', 'csv', {
  includeAbstract: true,
  includeTags: true
})
```

#### `useStatsStore()`
Statistics and chart data
```typescript
const statsStore = useStatsStore()

// Fetch all stats
await statsStore.fetchAllStats()

// Chart data
const chartData = statsStore.papersByYearChart

// Computed stats
const growth = statsStore.growthRate
const isStale = statsStore.isStale

// Refresh
await statsStore.refresh()
```

#### `useAIStore()`
AI features
```typescript
const aiStore = useAIStore()

// Generate review
const review = await aiStore.generateReview(paperId)

// Generate literature review
const litReview = await aiStore.generateLiteratureReview(
  'Machine Learning',
  [1, 2, 3, 4, 5]
)

// Generate research plan
const plan = await aiStore.generateResearchPlan(
  'Deep Learning for NLP',
  ['Objective 1', 'Objective 2']
)

// View history
console.log(aiStore.recentGenerations)
```

#### `useRecommendationStore()`
Recommendations and user preferences
```typescript
const recStore = useRecommendationStore()

// Get recommendations
await recStore.fetchPersonalized(20)

// Get similar papers
await recStore.fetchSimilar(paperId, 10)

// Get trending papers
await recStore.fetchTrending('week', 20)

// Submit feedback
await recStore.submitFeedback('rec_id', 'helpful', 'Great recommendation!')

// Update interests
await recStore.updateInterests(['AI', 'Machine Learning', 'NLP'])
```

#### `useUIStore()`
UI preferences and notifications
```typescript
const uiStore = useUIStore()

// Theme
uiStore.setTheme('dark') // 'light' | 'dark' | 'system'

// Sidebar
uiStore.toggleSidebar()
uiStore.collapseSidebar()

// Notifications
uiStore.showNotification('success', 'Success', 'Paper saved!')

// Loading
uiStore.showLoading('Loading...')
// ... do work
uiStore.hideLoading()

// Modals
uiStore.openModal('modal-id')
uiStore.closeModal('modal-id')
```

## Common Patterns

### Using Store in Components

```vue
<script setup lang="ts">
import { ref, computed } from 'vue'
import { storeToRefs } from 'pinia'
import { usePaperStore } from '@/stores'

const paperStore = usePaperStore()

// Use storeToRefs for reactivity
const { papers, loading, error } = storeToRefs(paperStore)

// Actions don't need storeToRefs
const fetchPapers = async () => {
  await paperStore.fetchPapers({ page: 1, pageSize: 20 })
}

// Computed
const paperCount = computed(() => papers.value.length)

// Initialize
onMounted(() => {
  fetchPapers()
})
</script>
```

### Error Handling

```typescript
try {
  await paperStore.fetchPapers()
  if (paperStore.error) {
    console.error('Error:', paperStore.error)
  }
} catch (error) {
  console.error('Failed to fetch papers:', error)
}
```

### Watch Store Changes

```typescript
import { watch } from 'vue'
import { usePaperStore } from '@/stores'

const paperStore = usePaperStore()

watch(
  () => paperStore.papers,
  (newPapers) => {
    console.log('Papers updated:', newPapers.length)
  },
  { deep: true }
)
```

### Combining Stores

```typescript
import { usePaperStore, useExportStore } from '@/stores'

const paperStore = usePaperStore()
const exportStore = useExportStore()

// Fetch papers and export
await paperStore.fetchPapers()
await exportStore.exportToCSV(paperStore.papers)
```

## Best Practices

1. **Use `storeToRefs`** for reactive state in components
2. **Handle errors** after actions
3. **Clean up** when done (call reset if needed)
4. **Use computed properties** for derived state
5. **Batch operations** when possible

## File Locations

All stores are in `/e/PaperCrawler/frontend/src/stores/`:
- `auth.ts` - Authentication
- `paperStore.ts` - Paper management
- `searchStore.ts` - Search functionality
- `crawlerStore.ts` - Crawler tasks
- `exportStore.ts` - Export functionality
- `statsStore.ts` - Statistics
- `aiStore.ts` - AI features
- `recommendationStore.ts` - Recommendations
- `uiStore.ts` - UI preferences

## TypeScript Types

All stores export their types:

```typescript
import type {
  Paper,
  PaperFilters,
  ExportOptions,
  AIGeneration
} from '@/stores'
```

## Need Help?

- See `/e/PaperCrawler/frontend/src/stores/README.md` for detailed documentation
- Check individual store files for JSDoc comments
- Use Vue DevTools to inspect store state
