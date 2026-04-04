<!--
  Example Usage of PaperCrawler Common Components

  This file demonstrates how to use the common component library
  in a typical PaperCrawler application scenario.
-->

<template>
  <div class="component-demo">
    <!-- Search Bar Example -->
    <section class="demo-section">
      <h2>Search Papers</h2>
      <SearchBar
        :debounce-time="300"
        :show-advanced-toggle="true"
        @search="handleSearch"
        @suggest="handleSuggest"
        @toggle-advanced="toggleAdvancedSearch"
      >
        <template #advanced>
          <div class="advanced-search">
            <h3>Advanced Filters</h3>
            <FilterPanel
              :checkbox-filters="checkboxFilters"
              :date-range-filters="dateFilters"
              :slider-filters="sliderFilters"
              @change="handleFilterChange"
            />
          </div>
        </template>
      </SearchBar>
    </section>

    <!-- Filter Panel Example -->
    <section class="demo-section">
      <h2>Filter Papers</h2>
      <FilterPanel
        :checkbox-filters="checkboxFilters"
        :date-range-filters="dateFilters"
        :slider-filters="sliderFilters"
        :initially-collapsed="false"
        @change="handleFilterChange"
        @clear="handleClearFilters"
      />
    </section>

    <!-- Status Badges Example -->
    <section class="demo-section">
      <h2>Status Indicators</h2>
      <div class="badge-demo">
        <StatusBadge text="Active" type="success" size="md" />
        <StatusBadge text="Processing" type="info" size="md" />
        <StatusBadge text="Warning" type="warning" size="md" />
        <StatusBadge text="Error" type="error" size="md" />
        <StatusBadge text="CCF A" type="primary" size="sm" :outlined="true" />
        <StatusBadge text="CCF B" type="secondary" size="sm" :dotted="true" />
      </div>
    </section>

    <!-- Paper Cards Example -->
    <section class="demo-section">
      <h2>Paper Results</h2>
      <div class="paper-grid">
        <PaperCard
          v-for="paper in papers"
          :key="paper.id"
          :paper="paper"
          :interactive="true"
          :highlight-keyword="searchQuery"
          :is-selected="selectedPaperId === paper.id"
          @click="handlePaperClick"
          @favorite="handleFavorite"
          @view="handleViewDetails"
        />
      </div>

      <!-- Empty State when no results -->
      <EmptyState
        v-if="papers.length === 0"
        title="No papers found"
        description="Try adjusting your search or filter criteria"
        action-text="Clear all filters"
        icon-type="no-results"
        size="lg"
        @action="handleClearFilters"
      />
    </section>

    <!-- Data Table Example -->
    <section class="demo-section">
      <h2>Paper Table</h2>
      <DataTable
        :columns="tableColumns"
        :data="papers"
        :selectable="true"
        :pagination="true"
        :items-per-page="10"
        :loading="isLoading"
        @row-click="handlePaperClick"
        @sort="handleSort"
        @selection-change="handleSelectionChange"
      >
        <template #filters>
          <input
            v-model="tableFilter"
            type="text"
            placeholder="Quick filter..."
            class="table-filter"
          />
        </template>

        <template #cell-title="{ row, value }">
          <router-link :to="`/papers/${row.id}`" class="paper-link">
            {{ value }}
          </router-link>
        </template>

        <template #cell-venue="{ value }">
          <StatusBadge :text="value" type="info" size="sm" />
        </template>

        <template #cell-citation_count="{ value }">
          <span :class="getCitationClass(value)">
            {{ value }}
          </span>
        </template>

        <template #empty>
          <EmptyState
            title="No data available"
            description="There are no papers to display"
            icon-type="no-data"
            size="md"
          />
        </template>
      </DataTable>
    </section>

    <!-- Loading States Example -->
    <section class="demo-section">
      <h2>Loading States</h2>
      <div class="loading-demo">
        <LoadingSpinner size="small" />
        <LoadingSpinner size="medium" text="Loading papers..." />
        <LoadingSpinner size="large" text="Please wait..." />
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import {
  PaperCard,
  SearchBar,
  DataTable,
  FilterPanel,
  StatusBadge,
  EmptyState,
  LoadingSpinner
} from '@/components/common'
import type {
  Paper,
  Column,
  CheckboxFilter,
  DateRangeFilter,
  SliderFilter
} from '@/components/common'

// State
const searchQuery = ref('')
const selectedPaperId = ref<string | null>(null)
const isLoading = ref(false)
const tableFilter = ref('')
const papers = ref<Paper[]>([])

// Filter definitions
const checkboxFilters: CheckboxFilter[] = [
  {
    key: 'ccf_level',
    label: 'CCF Level',
    options: [
      { value: 'A', label: 'CCF A', count: 45 },
      { value: 'B', label: 'CCF B', count: 78 },
      { value: 'C', label: 'CCF C', count: 123 }
    ]
  },
  {
    key: 'publication_type',
    label: 'Publication Type',
    options: [
      { value: 'journal', label: 'Journal', count: 89 },
      { value: 'conference', label: 'Conference', count: 156 },
      { value: 'workshop', label: 'Workshop', count: 34 }
    ]
  }
]

const dateFilters: DateRangeFilter[] = [
  {
    key: 'publication_date',
    label: 'Publication Date',
    startPlaceholder: 'From year',
    endPlaceholder: 'To year'
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

// Table columns
const tableColumns: Column[] = [
  { key: 'title', label: 'Title', sortable: true },
  { key: 'authors', label: 'Authors', width: '250px' },
  { key: 'year', label: 'Year', sortable: true, width: '80px' },
  { key: 'venue', label: 'Venue', width: '150px' },
  {
    key: 'citation_count',
    label: 'Citations',
    sortable: true,
    width: '100px'
  }
]

// Event handlers
const handleSearch = (query: string) => {
  searchQuery.value = query
  console.log('Searching for:', query)
  // Perform search API call
}

const handleSuggest = async (query: string) => {
  console.log('Fetching suggestions for:', query)
  // Fetch autocomplete suggestions
}

const toggleAdvancedSearch = (show: boolean) => {
  console.log('Advanced search:', show)
}

const handleFilterChange = (filters: Record<string, any>) => {
  console.log('Filters changed:', filters)
  // Apply filters to data
}

const handleClearFilters = () => {
  console.log('Clearing all filters')
  searchQuery.value = ''
  // Reset filters and data
}

const handlePaperClick = (paper: Paper) => {
  selectedPaperId.value = paper.id
  console.log('Paper clicked:', paper.title)
}

const handleFavorite = (paper: Paper, isFavorite: boolean) => {
  console.log(`${paper.title} favorite: ${isFavorite}`)
  // Update favorite status
}

const handleViewDetails = (paper: Paper) => {
  console.log('View details:', paper.title)
  // Navigate to paper details page
}

const handleSort = (key: string, order: 'asc' | 'desc') => {
  console.log(`Sort ${key} ${order}`)
  // Sort the data
}

const handleSelectionChange = (selectedRows: Paper[]) => {
  console.log('Selected papers:', selectedRows.length)
  // Handle selection
}

const getCitationClass = (count: number) => {
  if (count >= 100) return 'citation-very-high'
  if (count >= 50) return 'citation-high'
  if (count >= 10) return 'citation-medium'
  return 'citation-low'
}

// Initialize with sample data
papers.value = [
  {
    id: '1',
    title: 'Deep Learning for Software Testing: A Comprehensive Survey',
    authors: 'John Doe, Jane Smith, Robert Johnson',
    year: 2024,
    venue: 'ICSE 2024',
    citation_count: 42,
    ccf_level: 'A',
    abstract: 'This paper presents a comprehensive survey of deep learning applications in software testing...'
  },
  {
    id: '2',
    title: 'Automated Test Generation using Machine Learning',
    authors: 'Alice Williams, Bob Brown',
    year: 2023,
    venue: 'ASE 2023',
    citation_count: 28,
    ccf_level: 'A'
  },
  {
    id: '3',
    title: 'A Novel Approach to Regression Testing',
    authors: 'Charlie Davis, Diana Evans',
    year: 2024,
    venue: 'ISSTA 2024',
    citation_count: 15,
    ccf_level: 'B'
  }
]
</script>

<style scoped>
.component-demo {
  max-width: 1400px;
  margin: 0 auto;
  padding: var(--space-8);
}

.demo-section {
  margin-bottom: var(--space-12);
}

.demo-section h2 {
  margin-bottom: var(--space-4);
  font-size: var(--font-xl);
  font-weight: var(--font-semibold);
  color: var(--gray-900);
}

.advanced-search {
  padding: var(--space-4);
  background: var(--gray-50);
  border-radius: var(--radius-lg);
}

.advanced-search h3 {
  margin: 0 0 var(--space-4) 0;
  font-size: var(--font-base);
  font-weight: var(--font-semibold);
}

.badge-demo {
  display: flex;
  gap: var(--space-3);
  flex-wrap: wrap;
}

.paper-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(400px, 1fr));
  gap: var(--space-6);
}

.table-filter {
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--gray-300);
  border-radius: var(--radius-md);
  font-size: var(--font-sm);
}

.paper-link {
  color: var(--primary-600);
  text-decoration: none;
  font-weight: var(--font-medium);

  &:hover {
    text-decoration: underline;
  }
}

.citation-very-high {
  color: var(--success-600);
  font-weight: var(--font-semibold);
}

.citation-high {
  color: var(--info-600);
  font-weight: var(--font-semibold);
}

.citation-medium {
  color: var(--warning-600);
}

.citation-low {
  color: var(--gray-600);
}

.loading-demo {
  display: flex;
  gap: var(--space-6);
  align-items: center;
  flex-wrap: wrap;
}

@media (max-width: 768px) {
  .component-demo {
    padding: var(--space-4);
  }

  .paper-grid {
    grid-template-columns: 1fr;
  }
}
</style>
