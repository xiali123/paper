# PaperCrawler Common Components Library

A comprehensive collection of reusable Vue 3 components built with TypeScript, Composition API, and modern best practices for accessibility and performance.

## Features

- **TypeScript Support**: Full type definitions for all components and props
- **Composition API**: Built with Vue 3 Composition API for better logic reuse
- **Accessibility**: WCAG 2.1 AA compliant with proper ARIA labels and keyboard navigation
- **Responsive Design**: Mobile-first approach with responsive breakpoints
- **Customizable**: Extensive props, slots, and theming support
- **Performance Optimized**: Efficient rendering with computed properties and proper reactivity

## Installation

The components are automatically available in your PaperCrawler frontend application.

```typescript
// Import individual components
import { PaperCard, DataTable, SearchBar } from '@/components/common'

// Or register all components at once
import { registerComponents } from '@/components/common'
```

## Components

### PaperCard

Academic paper card component with rich metadata display and interactive features.

**Features:**
- Paper title, authors, venue, and citation count
- CCF level badges (A, B, C)
- Favorite functionality
- Citation copy and BibTeX export
- Keyword highlighting
- Auto-generated tags
- Click-to-view details

**Usage:**

```vue
<template>
  <PaperCard
    :paper="paperData"
    :interactive="true"
    :highlight-keyword="searchTerm"
    @click="handlePaperClick"
    @favorite="handleFavorite"
    @view="handleViewDetails"
  />
</template>

<script setup lang="ts">
import { PaperCard } from '@/components/common'
import type { Paper } from '@/components/common'

const paperData: Paper = {
  id: '1',
  title: 'Deep Learning for Software Testing',
  authors: 'John Doe, Jane Smith',
  year: 2024,
  venue: 'ICSE 2024',
  citation_count: 42,
  ccf_level: 'A'
}

const handlePaperClick = (paper: Paper) => {
  console.log('Clicked paper:', paper.title)
}

const handleFavorite = (paper: Paper, isFavorite: boolean) => {
  console.log(`${paper.title} favorite: ${isFavorite}`)
}

const handleViewDetails = (paper: Paper) => {
  // Navigate to paper details
}
</script>
```

**Props:**

| Prop | Type | Default | Description |
|------|------|---------|-------------|
| `paper` | `Paper` | required | Paper data object |
| `interactive` | `boolean` | `true` | Whether card is clickable |
| `highlightKeyword` | `string` | `''` | Keyword to highlight in title |
| `isSelected` | `boolean` | `false` | Whether card is selected |

**Events:**

| Event | Payload | Description |
|-------|---------|-------------|
| `click` | `(paper: Paper)` | Card clicked |
| `favorite` | `(paper: Paper, isFavorite: boolean)` | Favorite toggled |
| `view` | `(paper: Paper)` | View details clicked |

---

### SearchBar

Advanced search input with autocomplete, search history, and keyboard navigation.

**Features:**
- Debounced input with configurable delay
- Autocomplete suggestions with counts
- Search history with local storage persistence
- Keyboard navigation (Arrow keys, Enter, Escape)
- Advanced search toggle
- Clear button
- Mobile responsive

**Usage:**

```vue
<template>
  <SearchBar
    placeholder="Search papers..."
    :debounce-time="300"
    :show-advanced-toggle="true"
    @search="handleSearch"
    @suggest="handleSuggest"
    @toggle-advanced="handleToggleAdvanced"
  >
    <template #advanced>
      <div class="advanced-search">
        <!-- Advanced search filters -->
      </div>
    </template>
  </SearchBar>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { SearchBar } from '@/components/common'
import type { SearchSuggestion } from '@/components/common'

const handleSearch = (query: string) => {
  console.log('Searching for:', query)
  // Perform search
}

const handleSuggest = async (query: string) => {
  // Fetch suggestions
  const suggestions: SearchSuggestion[] = [
    { text: 'machine learning', count: 150 },
    { text: 'deep learning', count: 89 }
  ]
  // Update suggestions via ref or exposed method
}

const handleToggleAdvanced = (show: boolean) => {
  console.log('Advanced search:', show)
}
</script>
```

**Props:**

| Prop | Type | Default | Description |
|------|------|---------|-------------|
| `placeholder` | `string` | `'Search papers...'` | Input placeholder |
| `debounceTime` | `number` | `300` | Input debounce delay (ms) |
| `maxSuggestions` | `number` | `8` | Maximum suggestions to show |
| `maxHistoryItems` | `number` | `10` | Max history items to store |
| `showAdvancedToggle` | `boolean` | `false` | Show advanced search button |

**Events:**

| Event | Payload | Description |
|-------|---------|-------------|
| `search` | `(query: string)` | Search triggered |
| `suggest` | `(query: string)` | Suggestions requested |
| `toggleAdvanced` | `(show: boolean)` | Advanced panel toggled |

**Exposed Methods:**

| Method | Parameters | Description |
|--------|------------|-------------|
| `focus()` | - | Focus the input |
| `clear()` | - | Clear input and hide dropdowns |
| `setValue(value)` | `string` | Set input value |
| `getValue()` | Returns `string` | Get current value |
| `setSuggestions(suggestions)` | `SearchSuggestion[]` | Update suggestions |

---

### DataTable

Full-featured data table with sorting, pagination, and row selection.

**Features:**
- Column sorting (ascending/descending)
- Row selection (single/multiple)
- Pagination with customizable page size
- Loading and empty states
- Custom cell rendering via slots
- Responsive design
- Accessible keyboard navigation

**Usage:**

```vue
<template>
  <DataTable
    :columns="columns"
    :data="papers"
    :selectable="true"
    :pagination="true"
    :items-per-page="20"
    :loading="isLoading"
    @row-click="handleRowClick"
    @sort="handleSort"
    @selection-change="handleSelectionChange"
  >
    <template #filters>
      <input v-model="filterText" placeholder="Filter..." />
    </template>

    <template #cell-title="{ row, value }">
      <router-link :to="`/papers/${row.id}`">
        {{ value }}
      </router-link>
    </template>

    <template #cell-venue="{ value }">
      <StatusBadge :text="value" type="info" size="sm" />
    </template>

    <template #empty>
      <EmptyState
        title="No papers found"
        description="Try adjusting your search criteria"
        icon-type="no-results"
      />
    </template>
  </DataTable>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { DataTable, StatusBadge, EmptyState } from '@/components/common'
import type { Column, Paper } from '@/components/common'

const isLoading = ref(false)
const filterText = ref('')

const columns: Column[] = [
  { key: 'title', label: 'Title', sortable: true },
  { key: 'authors', label: 'Authors', width: '250px' },
  { key: 'year', label: 'Year', sortable: true, width: '80px' },
  { key: 'venue', label: 'Venue', width: '150px' },
  {
    key: 'citation_count',
    label: 'Citations',
    sortable: true,
    width: '100px',
    formatter: (val) => `${val} citations`
  }
]

const papers: Paper[] = [
  // Your paper data
]

const handleRowClick = (row: Paper) => {
  console.log('Row clicked:', row.title)
}

const handleSort = (key: string, order: 'asc' | 'desc') => {
  console.log(`Sort ${key} ${order}`)
}

const handleSelectionChange = (selectedRows: Paper[]) => {
  console.log('Selected:', selectedRows.length, 'rows')
}
</script>
```

**Props:**

| Prop | Type | Default | Description |
|------|------|---------|-------------|
| `columns` | `Column[]` | required | Column definitions |
| `data` | `T[]` | required | Table data (generic) |
| `rowKey` | `keyof T` | `'id'` | Unique row identifier |
| `selectable` | `boolean` | `false` | Enable row selection |
| `pagination` | `boolean` | `false` | Enable pagination |
| `itemsPerPage` | `number` | `20` | Items per page |
| `loading` | `boolean` | `false` | Show loading state |

**Events:**

| Event | Payload | Description |
|-------|---------|-------------|
| `rowClick` | `(row: T)` | Row clicked |
| `sort` | `(key: string, order: 'asc' | 'desc')` | Sort changed |
| `selectionChange` | `(selectedRows: T[])` | Selection changed |

**Slots:**

| Slot | Props | Description |
|------|-------|-------------|
| `filters` | - | Toolbar filter area |
| `actions` | - | Toolbar actions area |
| `cell-{key}` | `{ row, value, index }` | Custom cell content |
| `empty` | - | Empty state content |

**Exposed Methods:**

| Method | Returns | Description |
|--------|---------|-------------|
| `getSelectedRows()` | `T[]` | Get selected rows |
| `clearSelection()` | - | Clear all selections |
| `selectAll()` | - | Select all rows |
| `refresh()` | - | Reset to first page |

---

### FilterPanel

Versatile filtering component supporting checkbox groups, date ranges, and sliders.

**Features:**
- Checkbox filters with counts
- Date range pickers
- Slider controls
- Collapsible panel
- Active filter count
- Clear all filters
- Custom filter slots

**Usage:**

```vue
<template>
  <FilterPanel
    :checkbox-filters="checkboxFilters"
    :date-range-filters="dateFilters"
    :slider-filters="sliderFilters"
    @change="handleFilterChange"
    @clear="handleClearFilters"
  >
    <template #custom-filters>
      <!-- Custom filter components -->
    </template>
  </FilterPanel>
</template>

<script setup lang="ts">
import { FilterPanel } from '@/components/common'
import type { CheckboxFilter, DateRangeFilter, SliderFilter } from '@/components/common'

const checkboxFilters: CheckboxFilter[] = [
  {
    key: 'ccf_level',
    label: 'CCF Level',
    options: [
      { value: 'A', label: 'CCF A', count: 45 },
      { value: 'B', label: 'CCF B', count: 78 },
      { value: 'C', label: 'CCF C', count: 123 }
    ]
  }
]

const dateFilters: DateRangeFilter[] = [
  {
    key: 'publication_date',
    label: 'Publication Date',
    startPlaceholder: 'From',
    endPlaceholder: 'To'
  }
]

const sliderFilters: SliderFilter[] = [
  {
    key: 'min_citations',
    label: 'Minimum Citations',
    min: 0,
    max: 100,
    step: 5
  }
]

const handleFilterChange = (filters: Record<string, any>) => {
  console.log('Filters changed:', filters)
}

const handleClearFilters = () => {
  console.log('Filters cleared')
}
</script>
```

**Props:**

| Prop | Type | Default | Description |
|------|------|---------|-------------|
| `checkboxFilters` | `CheckboxFilter[]` | `[]` | Checkbox filter definitions |
| `dateRangeFilters` | `DateRangeFilter[]` | `[]` | Date range filters |
| `sliderFilters` | `SliderFilter[]` | `[]` | Slider filters |
| `initiallyCollapsed` | `boolean` | `false` | Start collapsed |

**Events:**

| Event | Payload | Description |
|-------|---------|-------------|
| `change` | `(filters: Record<string, any>)` | Filters changed |
| `clear` | - | Clear all filters |

**Slots:**

| Slot | Description |
|------|-------------|
| `custom-filters` | Custom filter content |

**Exposed Methods:**

| Method | Returns | Description |
|--------|---------|-------------|
| `getFilters()` | `Record<string, any>` | Get current filters |
| `setFilters(filters)` | - | Set filters programmatically |
| `clearFilters()` | - | Clear all filters |
| `isActive()` | `boolean` | Whether any filters active |

---

### StatusBadge

Flexible status badge component with multiple variants and sizes.

**Features:**
- Multiple type variants (success, info, warning, error, neutral, primary, secondary)
- Three sizes (sm, md, lg)
- Optional icons
- Dismissible variant
- Outlined and dotted styles
- Custom slots

**Usage:**

```vue
<template>
  <!-- Basic usage -->
  <StatusBadge text="Active" type="success" />
  <StatusBadge text="Pending" type="warning" />
  <StatusBadge text="Error" type="error" />

  <!-- With sizes -->
  <StatusBadge text="Small" type="info" size="sm" />
  <StatusBadge text="Medium" type="info" size="md" />
  <StatusBadge text="Large" type="info" size="lg" />

  <!-- Dismissible -->
  <StatusBadge
    text="Notification"
    type="primary"
    :dismissible="true"
    @dismiss="handleDismiss"
  />

  <!-- Custom content -->
  <StatusBadge type="success" size="lg">
    <template #icon>
      <svg><!-- custom icon --></svg>
    </template>
    <span>Custom badge text</span>
  </StatusBadge>

  <!-- Outlined variant -->
  <StatusBadge text="Outlined" type="secondary" :outlined="true" />

  <!-- Dotted variant -->
  <StatusBadge text="Dotted" type="info" :dotted="true" />
</template>

<script setup lang="ts">
import { StatusBadge } from '@/components/common'

const handleDismiss = () => {
  console.log('Badge dismissed')
}
</script>
```

**Props:**

| Prop | Type | Default | Description |
|------|------|---------|-------------|
| `type` | `BadgeType` | `'neutral'` | Badge type variant |
| `size` | `'sm' | 'md' | 'lg'` | `'md'` | Badge size |
| `text` | `string` | `''` | Badge text |
| `dismissible` | `boolean` | `false` | Show dismiss button |
| `dotted` | `boolean` | `false` | Use dotted border |
| `outlined` | `boolean` | `false` | Use outlined style |
| `role` | `string` | `'status'` | ARIA role |
| `ariaLabel` | `string` | - | Custom ARIA label |

**Events:**

| Event | Payload | Description |
|-------|---------|-------------|
| `dismiss` | - | Dismiss button clicked |

**Slots:**

| Slot | Description |
|------|-------------|
| `icon` | Custom icon content |
| `default` | Badge text content |

---

### EmptyState

Attractive empty state component for when no data is available.

**Features:**
- Built-in icons for common scenarios
- Customizable size (sm, md, lg)
- Optional action button
- Custom icon and content slots
- Animated effects

**Usage:**

```vue
<template>
  <!-- With built-in icon -->
  <EmptyState
    title="No papers found"
    description="We couldn't find any papers matching your search."
    action-text="Clear filters"
    icon-type="no-results"
    size="lg"
    @action="handleClearFilters"
  />

  <!-- Custom content -->
  <EmptyState size="md">
    <template #icon>
      <svg><!-- custom icon --></svg>
    </template>
    <template #title>
      <h3>Custom Title</h3>
    </template>
    <template #description>
      <p>Custom description text</p>
    </template>
    <template #actions>
      <button @click="handleAction">Custom Action</button>
      <button @click="handleSecondary">Secondary</button>
    </template>
  </EmptyState>
</template>

<script setup lang="ts">
import { EmptyState } from '@/components/common'

const handleClearFilters = () => {
  console.log('Clear filters')
}
</script>
```

**Props:**

| Prop | Type | Default | Description |
|------|------|---------|-------------|
| `title` | `string` | `'No Data'` | Empty state title |
| `description` | `string` | `''` | Description text |
| `actionText` | `string` | `''` | Action button text |
| `iconType` | `IconType` | `'no-data'` | Built-in icon type |
| `size` | `'sm' | 'md' | 'lg'` | `'md'` | Component size |
| `centered` | `boolean` | `true` | Center content |

**Events:**

| Event | Payload | Description |
|-------|---------|-------------|
| `action` | - | Action button clicked |

**Slots:**

| Slot | Description |
|------|-------------|
| `icon` | Custom icon |
| `title` | Custom title |
| `description` | Custom description |
| `actions` | Custom action buttons |

---

### LoadingSpinner

Animated loading spinner with text support.

**Features:**
- Multiple size variants
- Color variants (primary, secondary, white)
- Optional loading text
- Smooth animations
- Performance optimized

**Usage:**

```vue
<template>
  <!-- Basic usage -->
  <LoadingSpinner />

  <!-- With text -->
  <LoadingSpinner text="Loading papers..." />

  <!-- Different sizes -->
  <LoadingSpinner size="small" />
  <LoadingSpinner size="medium" />
  <LoadingSpinner size="large" />

  <!-- Different variants -->
  <LoadingSpinner variant="primary" />
  <LoadingSpinner variant="secondary" />
  <LoadingSpinner variant="white" />
</template>

<script setup lang="ts">
import { LoadingSpinner } from '@/components/common'
</script>
```

**Props:**

| Prop | Type | Default | Description |
|------|------|---------|-------------|
| `size` | `'small' | 'medium' | 'large'` | `'medium'` | Spinner size |
| `variant` | `'primary' | 'secondary' | 'white'` | `'primary'` | Color variant |
| `text` | `string` | - | Loading text |

---

## Global Registration

Register all components globally in your `main.ts`:

```typescript
import { createApp } from 'vue'
import App from './App.vue'
import { registerComponents } from '@/components/common'

const app = createApp(App)
registerComponents(app)
app.mount('#app')
```

Then use components without importing:

```vue
<template>
  <div>
    <PaperCard :paper="paper" />
    <SearchBar @search="handleSearch" />
    <DataTable :data="data" :columns="columns" />
  </div>
</template>
```

## TypeScript Support

All components export their TypeScript interfaces:

```typescript
import type {
  Paper,
  Column,
  CheckboxFilter,
  DateRangeFilter,
  SliderFilter,
  SearchSuggestion
} from '@/components/common'
```

## Accessibility

All components follow WCAG 2.1 AA guidelines:

- Proper ARIA labels and roles
- Keyboard navigation support
- Focus indicators
- Screen reader compatibility
- Semantic HTML structure

## Performance

Components are optimized for performance:

- Efficient reactivity with computed properties
- Proper key usage in lists
- Lazy rendering where appropriate
- Minimal re-renders
- Optimized bundle size

## Browser Support

- Chrome/Edge (latest)
- Firefox (latest)
- Safari (latest)
- Mobile browsers (iOS Safari, Chrome Mobile)

## Contributing

When adding new components:

1. Use TypeScript with proper type definitions
2. Implement Composition API
3. Add ARIA attributes for accessibility
4. Include comprehensive props documentation
5. Provide usage examples
6. Export from `index.ts`

## License

MIT License - See project root for details.
