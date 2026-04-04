# PaperCrawler Search Feature Guide

## Overview

The PaperCrawler search functionality provides comprehensive paper search capabilities with both basic and advanced search modes, real-time suggestions, search history, and export features.

## Features

### Basic Search
- Large search bar with auto-complete
- Real-time search suggestions
- Search history display
- Trending searches
- Quick filters (year, CCF level)
- Sort options (relevance, date, citations)
- Grid/list view toggle
- Export results

### Advanced Search
- Multi-field search query builder
- Boolean operators (AND, OR, NOT)
- Field-specific searches (title, authors, abstract, keywords, journal, year)
- Citation range filtering
- Year range filtering
- CCF level filtering
- Journal/conference filtering
- Paper type filtering
- Language filtering
- Save and load search queries
- Search query preview

## File Structure

```
src/
├── views/
│   └── search/
│       ├── SearchView.vue           # Main search page
│       └── AdvancedSearchView.vue   # Advanced search modal
├── components/
│   └── common/
│       ├── SearchBar.vue            # Reusable search bar component
│       ├── PaperCard.vue            # Paper display card
│       ├── LoadingSpinner.vue       # Loading indicator
│       └── EmptyState.vue           # Empty state display
├── stores/
│   └── searchStore.ts              # Search state management
├── composables/
│   ├── useSearch.ts                # Basic search composable
│   └── useAdvancedSearch.ts        # Advanced search composable
├── api/
│   └── modules/
│       └── search.ts               # Search API calls
├── types/
│   └── search.ts                   # Search type definitions
└── utils/
    └── searchUtils.ts              # Search utility functions
```

## Usage

### Router Integration

Add the search routes to your router configuration:

```typescript
// src/router/index.ts
import { createRouter, createWebHistory } from 'vue-router'
import SearchView from '@/views/search/SearchView.vue'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/search',
      name: 'search',
      component: SearchView,
      props: (route) => ({
        query: route.query.q,
        advanced: route.query.advanced === 'true'
      })
    }
  ]
})
```

### Basic Search Usage

```vue
<template>
  <SearchView />
</template>

<script setup lang="ts">
import SearchView from '@/views/search/SearchView.vue'
</script>
```

### Programmatic Search

```typescript
import { useRouter } from 'vue-router'
import { useSearchStore } from '@/stores/searchStore'

const router = useRouter()
const searchStore = useSearchStore()

// Navigate to search with query
router.push({
  name: 'search',
  query: { q: 'machine learning' }
})

// Or use the search store directly
await searchStore.search('deep learning')
```

### Advanced Search Usage

```vue
<template>
  <button @click="showAdvancedSearch = true">Advanced Search</button>

  <div v-if="showAdvancedSearch" class="modal">
    <AdvancedSearchView
      @search="handleAdvancedSearch"
      @cancel="showAdvancedSearch = false"
    />
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import AdvancedSearchView from '@/views/search/AdvancedSearchView.vue'

const showAdvancedSearch = ref(false)

const handleAdvancedSearch = async (params) => {
  console.log('Advanced search params:', params)
  // Handle search results
}
</script>
```

### Using the Search Composable

```typescript
import { useSearch } from '@/composables/useSearch'

const {
  query,
  results,
  total,
  loading,
  error,
  performSearch,
  searchRealtime,
  resetSearch
} = useSearch()

// Perform search
await performSearch('neural networks')

// Real-time search (with debouncing)
searchRealtime('machine learning')

// Reset search state
resetSearch()
```

### Using the Advanced Search Composable

```typescript
import { useAdvancedSearch } from '@/composables/useAdvancedSearch'

const {
  query,
  results,
  validateQuery,
  performAdvancedSearch,
  exportResults
} = useAdvancedSearch()

// Validate query
const validation = validateQuery({
  title: 'deep learning',
  yearFrom: 2020,
  ccfLevels: ['A', 'B']
})

if (validation.valid) {
  await performAdvancedSearch({
    title: 'deep learning',
    yearFrom: 2020,
    ccfLevels: ['A', 'B']
  })
}

// Export results
await exportResults('csv')
```

## API Integration

The search functionality integrates with the following API endpoints:

- `GET /api/search` - Basic search
- `POST /api/search/advanced` - Advanced search
- `GET /api/search/suggestions` - Search suggestions
- `GET /api/search/trending` - Trending searches
- `GET /api/search/history` - Search history
- `POST /api/search/saved` - Save search
- `GET /api/search/saved` - Get saved searches
- `DELETE /api/search/saved/:name` - Delete saved search
- `GET /api/search/export` - Export results

## Search Query Syntax

### Basic Queries
```
machine learning
"neural networks"
author:"Hinton"
title:"attention mechanism"
```

### Boolean Operators
```
machine learning AND deep learning
(classification OR regression) NOT "neural networks"
```

### Field-Specific Searches
```
title:"transformer architecture"
authors:"Geoffrey Hinton"
abstract:"reinforcement learning"
keywords:"computer vision"
journal:"Nature"
year:2023
doi:"10.1038/s41586"
```

### Complex Queries
```
(title:"deep learning" OR abstract:"neural network")
AND year:[2020 TO 2023]
AND citations:>100
```

## Export Formats

The search results can be exported in multiple formats:

### CSV
```typescript
import { exportToCSV } from '@/utils/searchUtils'

exportToCSV(papers, 'research-papers.csv')
```

### JSON
```typescript
import { exportToJSON } from '@/utils/searchUtils'

exportToJSON(papers, 'research-papers.json')
```

### BibTeX
```typescript
import { exportToBibTeX } from '@/utils/searchUtils'

exportToBibTeX(papers, 'references.bib')
```

## Customization

### Custom Search Filters

Add custom filters to the search query:

```typescript
const customFilters = {
  customField: 'value',
  anotherField: ['value1', 'value2']
}

await searchStore.advancedSearch({
  title: 'machine learning',
  filters: customFilters
})
```

### Custom Result Display

Customize how results are displayed:

```vue
<template>
  <PaperCard
    v-for="paper in results"
    :key="paper.id"
    :paper="paper"
    :highlight-keyword="searchQuery"
    :interactive="true"
    @click="handlePaperClick"
    @favorite="handleFavorite"
  />
</template>
```

### Custom Search Suggestions

Provide custom suggestions:

```typescript
import { generateSearchSuggestions } from '@/utils/searchUtils'

const suggestions = generateSearchSuggestions(
  'machine',
  searchHistory,
  10
)
```

## Performance Optimization

### Debouncing

Search is automatically debounced to prevent excessive API calls:

```typescript
import { debounce } from '@/utils/searchUtils'

const debouncedSearch = debounce(performSearch, 300)
```

### Result Caching

Results are cached in the search store:

```typescript
// Check if results are cached
if (searchStore.results.length > 0) {
  // Use cached results
} else {
  // Fetch new results
}
```

### Lazy Loading

Implement infinite scroll or pagination:

```typescript
const loadMoreResults = async () => {
  if (searchStore.hasNextPage) {
    await searchStore.nextPage()
  }
}
```

## Accessibility

The search components follow WCAG 2.1 AA guidelines:

- Proper ARIA labels and roles
- Keyboard navigation support
- Screen reader compatibility
- Focus management
- Error announcements

## Browser Support

- Chrome 90+
- Firefox 88+
- Safari 14+
- Edge 90+

## Troubleshooting

### Search Not Working
1. Check API connection
2. Verify search query format
3. Check browser console for errors
4. Ensure search store is initialized

### No Results Returned
1. Verify search terms are not too restrictive
2. Check filter combinations
3. Try broader search terms
4. Check if papers exist in database

### Export Failing
1. Verify export format is supported
2. Check if papers have required fields
3. Ensure browser supports file downloads
4. Check CORS settings for file URLs

## Future Enhancements

- Semantic search with embeddings
- Search result clustering
- Advanced faceted search
- Search analytics
- Collaborative filtering
- Search result recommendations
- Multi-language search support
- Real-time search result streaming

## Contributing

When contributing to the search functionality:

1. Follow the existing code structure
2. Update type definitions in `types/search.ts`
3. Add API methods to `api/modules/search.ts`
4. Update store actions for state changes
5. Add utility functions to `utils/searchUtils.ts`
6. Test with various search queries
7. Ensure accessibility compliance
8. Update documentation

## Support

For issues or questions:
1. Check the API documentation
2. Review the type definitions
3. Examine the search store
4. Look at utility functions
5. Check browser console for errors