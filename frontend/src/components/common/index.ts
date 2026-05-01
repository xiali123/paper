/**
 * PaperCrawler Common Components Library
 *
 * A comprehensive collection of reusable Vue 3 components built with TypeScript,
 * Composition API, and modern best practices for accessibility and performance.
 *
 * @module common/components
 * @version 1.0.0
 */

// Export component types
export type { Paper } from './PaperCard.vue'
export type { Column } from './DataTable.vue'
export type { CheckboxOption, CheckboxFilter, DateRangeFilter, SliderFilter } from './FilterPanel.vue'
export type { SearchSuggestion } from './SearchBar.vue'

// Import components
import PaperCard from './PaperCard.vue'
import SearchBar from './SearchBar.vue'
import DataTable from './DataTable.vue'
import FilterPanel from './FilterPanel.vue'
import StatusBadge from './StatusBadge.vue'
import EmptyState from './EmptyState.vue'
import LoadingSpinner from './LoadingSpinner.vue'
import ErrorBoundary from './ErrorBoundary.vue'

/**
 * Individual component exports for named imports
 */
export {
  PaperCard,
  SearchBar,
  DataTable,
  FilterPanel,
  StatusBadge,
  EmptyState,
  LoadingSpinner,
  ErrorBoundary
}

/**
 * Default export object containing all components
 * Useful for registering all components at once
 */
export default {
  PaperCard,
  SearchBar,
  DataTable,
  FilterPanel,
  StatusBadge,
  EmptyState,
  LoadingSpinner,
  ErrorBoundary
}

/**
 * Component registration helper
 *
 * Usage in main.ts:
 * ```typescript
 * import { createApp } from 'vue'
 * import App from './App.vue'
 * import { registerComponents } from '@/components/common'
 *
 * const app = createApp(App)
 * registerComponents(app)
 * ```
 */
export function registerComponents(app: any) {
  app.component('PaperCard', PaperCard)
  app.component('SearchBar', SearchBar)
  app.component('DataTable', DataTable)
  app.component('FilterPanel', FilterPanel)
  app.component('StatusBadge', StatusBadge)
  app.component('EmptyState', EmptyState)
  app.component('LoadingSpinner', LoadingSpinner)
  app.component('ErrorBoundary', ErrorBoundary)
}

/**
 * TypeScript interfaces export
 *
 * This section re-exports all TypeScript interfaces for type checking
 */

/**
 * Paper interface for academic paper data
 * @interface Paper
 */
export interface Paper {
  /** Unique identifier */
  id: string
  /** Paper title */
  title: string
  /** Author list */
  authors: string
  /** Publication year */
  year: number
  /** Publication venue */
  venue: string
  /** Number of citations */
  citation_count: number
  /** CCF rating level (optional) */
  ccf_level?: 'A' | 'B' | 'C'
  /** Paper abstract (optional) */
  abstract?: string
}

/**
 * DataTable column definition
 * @interface Column
 */
export interface Column {
  /** Unique key for the column */
  key: string
  /** Display label */
  label: string
  /** Column width (optional) */
  width?: string
  /** Whether column is sortable */
  sortable?: boolean
  /** Cell value formatter (optional) */
  formatter?: (value: any) => string
}

/**
 * FilterPanel interfaces
 */
export interface CheckboxOption {
  value: string
  label: string
  count?: number
}

export interface CheckboxFilter {
  key: string
  label: string
  options: CheckboxOption[]
}

export interface DateRangeFilter {
  key: string
  label: string
  startPlaceholder?: string
  endPlaceholder?: string
}

export interface SliderFilter {
  key: string
  label: string
  min: number
  max: number
  step?: number
}

/**
 * SearchBar suggestion interface
 */
export interface SearchSuggestion {
  text: string
  count: number
}
