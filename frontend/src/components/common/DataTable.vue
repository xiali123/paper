<template>
  <div class="data-table">
    <!-- Table toolbar -->
    <div class="data-table__toolbar">
      <div class="data-table__filters">
        <slot name="filters" />
      </div>

      <div class="data-table__actions">
        <slot name="actions" />
      </div>
    </div>

    <!-- Table container -->
    <div class="data-table__container">
      <table class="data-table__table">
        <thead class="data-table__thead">
          <tr>
            <th v-if="selectable" class="data-table__checkbox">
              <input
                type="checkbox"
                :checked="allSelected"
                @change="toggleAll"
                aria-label="Select all rows"
              />
            </th>
            <th
              v-for="column in columns"
              :key="column.key"
              class="data-table__th"
              :class="{
                'data-table__th--sortable': column.sortable,
                'data-table__th--sorted': sortKey === column.key
              }"
              :style="{ width: column.width }"
              @click="column.sortable && sort(column.key)"
              :aria-sort="sortKey === column.key ? (sortOrder === 'asc' ? 'ascending' : 'descending') : 'none'"
            >
              <div class="data-table__th-content">
                <span>{{ column.label }}</span>
                <svg
                  v-if="column.sortable"
                  class="data-table__sort-icon"
                  width="16"
                  height="16"
                  viewBox="0 0 24 24"
                  fill="currentColor"
                >
                  <path
                    :d="sortKey === column.key
                      ? (sortOrder === 'asc'
                        ? 'M3 18h6v-2H3v2zM3 6v2h18V6H3zm0 7h12v-2H3v2z'
                        : 'M3 18h6v-2H3v2zM3 6v2h18V6H3zm0 7h12v-2H3v2z')
                      : 'M3 18h6v-2H3v2zM3 6v2h18V6H3zm0 7h12v-2H3v2z'"
                  />
                </svg>
              </div>
            </th>
          </tr>
        </thead>

        <tbody class="data-table__tbody">
          <tr
            v-for="(row, index) in paginatedData"
            :key="getRowKey(row, index)"
            class="data-table__row"
            :class="{
              'data-table__row--selected': isSelected(row),
              'data-table__row--hovered': hoveredRow === index
            }"
            @click="handleRowClick(row)"
            @mouseenter="hoveredRow = index"
            @mouseleave="hoveredRow = -1"
            :aria-selected="isSelected(row)"
          >
            <td v-if="selectable" class="data-table__checkbox">
              <input
                type="checkbox"
                :checked="isSelected(row)"
                @change="toggleRow(row)"
                @click.stop
                :aria-label="`Select row ${index + 1}`"
              />
            </td>
            <td
              v-for="column in columns"
              :key="column.key"
              class="data-table__td"
            >
              <slot
                v-if="$slots[`cell-${column.key}`]"
                :name="`cell-${column.key}`"
                :row="row"
                :value="row[column.key]"
                :index="index"
              />
              <span v-else>{{ formatCellValue(row[column.key], column) }}</span>
            </td>
          </tr>
        </tbody>
      </table>

      <!-- Loading state -->
      <div v-if="loading" class="data-table__loading">
        <div class="data-table__loading-spinner"></div>
        <p>Loading data...</p>
      </div>

      <!-- Empty state -->
      <div v-if="!loading && sortedData.length === 0" class="data-table__empty">
        <slot name="empty">
          <div class="data-table__empty-content">
            <svg width="64" height="64" viewBox="0 0 24 24" fill="currentColor">
              <path d="M19 5v14H5V5h14m0-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm-4.86 8.86l-3 3.87L9 13.14 6 17h12l-3.86-5.14z" />
            </svg>
            <p>No data available</p>
          </div>
        </slot>
      </div>
    </div>

    <!-- Pagination -->
    <div v-if="pagination && !loading && sortedData.length > 0" class="data-table__pagination">
      <div class="data-table__pagination-info">
        Showing {{ paginationStart }}-{{ paginationEnd }} of {{ totalItems }} items
      </div>

      <div class="data-table__pagination-controls">
        <button
          class="data-table__pagination-btn"
          :disabled="currentPage === 1"
          @click="changePage(currentPage - 1)"
          aria-label="Previous page"
        >
          Previous
        </button>

        <div class="data-table__pagination-pages">
          <button
            v-for="page in visiblePages"
            :key="page"
            class="data-table__pagination-page"
            :class="{ 'data-table__pagination-page--active': page === currentPage }"
            @click="changePage(page)"
            :aria-label="`Go to page ${page}`"
            :aria-current="page === currentPage ? 'page' : undefined"
          >
            {{ page }}
          </button>
        </div>

        <button
          class="data-table__pagination-btn"
          :disabled="currentPage === totalPages"
          @click="changePage(currentPage + 1)"
          aria-label="Next page"
        >
          Next
        </button>
      </div>

      <select
        v-model="pageSize"
        class="data-table__pagination-size"
        @change="handlePageSizeChange"
        aria-label="Items per page"
      >
        <option :value="10">10 / page</option>
        <option :value="20">20 / page</option>
        <option :value="50">50 / page</option>
        <option :value="100">100 / page</option>
      </select>
    </div>
  </div>
</template>

<script setup lang="ts" generic="T extends Record<string, any> ">
import { ref, computed, watch } from 'vue'

/**
 * Column definition interface
 */
interface Column {
  /** Unique key for the column */
  key: string
  /** Display label for the column */
  label: string
  /** Optional width for the column */
  width?: string
  /** Whether the column is sortable */
  sortable?: boolean
  /** Optional formatter function for cell values */
  formatter?: (value: any) => string
}

/**
 * Component props for DataTable
 */
interface Props<T> {
  /** Array of column definitions */
  columns: Column[]
  /** Data to display in the table */
  data: T[]
  /** Key property to use for row identification */
  rowKey?: keyof T
  /** Whether rows can be selected */
  selectable?: boolean
  /** Whether to show pagination */
  pagination?: boolean
  /** Number of items per page */
  itemsPerPage?: number
  /** Whether data is currently loading */
  loading?: boolean
}

const props = withDefaults(defineProps<Props<T>>(), {
  rowKey: 'id' as keyof T,
  selectable: false,
  pagination: false,
  itemsPerPage: 20,
  loading: false
})

/**
 * Component events
 */
const emit = defineEmits<{
  /** Fired when a row is clicked */
  rowClick: [row: T]
  /** Fired when sorting changes */
  sort: [key: string, order: 'asc' | 'desc']
  /** Fired when selection changes */
  selectionChange: [selectedRows: T[]]
}>()

const selectedRows = ref<Set<any>>(new Set())
const hoveredRow = ref(-1)
const sortKey = ref<string>('')
const sortOrder = ref<'asc' | 'desc'>('asc')
const currentPage = ref(1)
const pageSize = ref(props.itemsPerPage)

/**
 * Computed sorted data
 */
const sortedData = computed(() => {
  if (!sortKey.value) return props.data

  return [...props.data].sort((a, b) => {
    const aVal = a[sortKey.value as keyof T]
    const bVal = b[sortKey.value as keyof T]

    if (aVal < bVal) return sortOrder.value === 'asc' ? -1 : 1
    if (aVal > bVal) return sortOrder.value === 'asc' ? 1 : -1
    return 0
  })
})

/**
 * Computed paginated data
 */
const paginatedData = computed(() => {
  if (!props.pagination) return sortedData.value

  const start = (currentPage.value - 1) * pageSize.value
  const end = start + pageSize.value
  return sortedData.value.slice(start, end)
})

/**
 * Total number of pages
 */
const totalPages = computed(() => {
  return Math.ceil(props.data.length / pageSize.value)
})

/**
 * Total number of items
 */
const totalItems = computed(() => props.data.length)

/**
 * Start index for pagination display
 */
const paginationStart = computed(() => {
  return (currentPage.value - 1) * pageSize.value + 1
})

/**
 * End index for pagination display
 */
const paginationEnd = computed(() => {
  return Math.min(currentPage.value * pageSize.value, totalItems.value)
})

/**
 * Visible page numbers for pagination
 */
const visiblePages = computed(() => {
  const pages: number[] = []
  const maxVisible = 5

  if (totalPages.value <= maxVisible) {
    for (let i = 1; i <= totalPages.value; i++) {
      pages.push(i)
    }
  } else {
    let start = Math.max(1, currentPage.value - 2)
    let end = Math.min(totalPages.value, start + maxVisible - 1)

    if (end - start < maxVisible - 1) {
      start = Math.max(1, end - maxVisible + 1)
    }

    for (let i = start; i <= end; i++) {
      pages.push(i)
    }
  }

  return pages
})

/**
 * Whether all rows are selected
 */
const allSelected = computed(() => {
  return paginatedData.value.length > 0 &&
    paginatedData.value.every(row => isSelected(row))
})

/**
 * Get unique key for a row
 */
const getRowKey = (row: T, index: number) => {
  return row[props.rowKey] ?? index
}

/**
 * Check if a row is selected
 */
const isSelected = (row: T) => {
  return selectedRows.value.has(row[props.rowKey])
}

/**
 * Toggle row selection
 */
const toggleRow = (row: T) => {
  const key = row[props.rowKey]
  if (selectedRows.value.has(key)) {
    selectedRows.value.delete(key)
  } else {
    selectedRows.value.add(key)
  }
  emitSelectionChange()
}

/**
 * Toggle all rows selection
 */
const toggleAll = () => {
  if (allSelected.value) {
    paginatedData.value.forEach(row => {
      selectedRows.value.delete(row[props.rowKey])
    })
  } else {
    paginatedData.value.forEach(row => {
      selectedRows.value.add(row[props.rowKey])
    })
  }
  emitSelectionChange()
}

/**
 * Emit selection change event
 */
const emitSelectionChange = () => {
  const selected = props.data.filter(row => selectedRows.value.has(row[props.rowKey]))
  emit('selectionChange', selected)
}

/**
 * Sort data by column
 */
const sort = (key: string) => {
  if (sortKey.value === key) {
    sortOrder.value = sortOrder.value === 'asc' ? 'desc' : 'asc'
  } else {
    sortKey.value = key
    sortOrder.value = 'asc'
  }
  emit('sort', key, sortOrder.value)
}

/**
 * Handle row click
 */
const handleRowClick = (row: T) => {
  emit('rowClick', row)
}

/**
 * Format cell value
 */
const formatCellValue = (value: any, column: Column) => {
  if (column.formatter) {
    return column.formatter(value)
  }
  return value
}

/**
 * Change current page
 */
const changePage = (page: number) => {
  if (page >= 1 && page <= totalPages.value) {
    currentPage.value = page
  }
}

/**
 * Handle page size change
 */
const handlePageSizeChange = () => {
  currentPage.value = 1
}

/**
 * Watch for data changes to reset pagination
 */
watch(() => props.data, () => {
  currentPage.value = 1
})

/**
 * Expose methods for parent components
 */
defineExpose({
  getSelectedRows: () => props.data.filter(row => selectedRows.value.has(row[props.rowKey])),
  clearSelection: () => {
    selectedRows.value.clear()
    emitSelectionChange()
  },
  selectAll: () => {
    props.data.forEach(row => {
      selectedRows.value.add(row[props.rowKey])
    })
    emitSelectionChange()
  },
  refresh: () => {
    currentPage.value = 1
  }
})
</script>

<style scoped lang="scss">
.data-table {
  display: flex;
  flex-direction: column;
  gap: var(--space-4);
}

.data-table__toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: var(--space-4);
}

.data-table__filters {
  display: flex;
  gap: var(--space-3);
  flex: 1;
}

.data-table__actions {
  display: flex;
  gap: var(--space-3);
}

.data-table__container {
  background: white;
  border-radius: var(--radius-lg);
  box-shadow: var(--shadow-sm);
  border: 1px solid var(--gray-200);
  overflow: hidden;
  position: relative;
}

.data-table__table {
  width: 100%;
  border-collapse: collapse;
}

.data-table__thead {
  background: var(--gray-50);
  border-bottom: 2px solid var(--gray-200);
}

.data-table__th {
  padding: var(--space-4);
  text-align: left;
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  color: var(--gray-700);
  user-select: none;

  &--sortable {
    cursor: pointer;
    transition: background var(--duration-fast);

    &:hover {
      background: var(--gray-100);
    }
  }

  &--sorted {
    color: var(--primary-600);
  }
}

.data-table__th-content {
  display: flex;
  align-items: center;
  gap: var(--space-2);
}

.data-table__sort-icon {
  color: var(--gray-400);
  flex-shrink: 0;
}

.data-table__tbody {
  background: white;
}

.data-table__row {
  border-bottom: 1px solid var(--gray-200);
  transition: background var(--duration-fast);

  &:last-child {
    border-bottom: none;
  }

  &--hovered {
    background: var(--gray-50);
  }

  &--selected {
    background: var(--primary-50);
  }
}

.data-table__td {
  padding: var(--space-4);
  font-size: var(--font-sm);
  color: var(--gray-900);
}

.data-table__checkbox {
  width: 48px;
  padding: var(--space-4);
  text-align: center;
}

.data-table__loading {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: var(--space-16);
  gap: var(--space-4);
  color: var(--gray-600);
}

.data-table__loading-spinner {
  width: 40px;
  height: 40px;
  border: 4px solid var(--gray-200);
  border-top-color: var(--primary-500);
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.data-table__empty {
  display: flex;
  justify-content: center;
  align-items: center;
  padding: var(--space-16);
}

.data-table__empty-content {
  text-align: center;
  color: var(--gray-500);

  svg {
    margin-bottom: var(--space-4);
  }
}

.data-table__pagination {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: var(--space-4);
  padding: var(--space-4);
  background: white;
  border-radius: var(--radius-lg);
  border: 1px solid var(--gray-200);
}

.data-table__pagination-info {
  font-size: var(--font-sm);
  color: var(--gray-600);
}

.data-table__pagination-controls {
  display: flex;
  gap: var(--space-2);
  align-items: center;
}

.data-table__pagination-btn {
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--gray-300);
  background: white;
  border-radius: var(--radius-md);
  font-size: var(--font-sm);
  cursor: pointer;
  transition: all var(--duration-fast);

  &:hover:not(:disabled) {
    background: var(--gray-50);
    border-color: var(--gray-400);
  }

  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }

  &:focus-visible {
    outline: 2px solid var(--primary-500);
    outline-offset: 2px;
  }
}

.data-table__pagination-pages {
  display: flex;
  gap: var(--space-1);
}

.data-table__pagination-page {
  width: 36px;
  height: 36px;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 1px solid var(--gray-300);
  background: white;
  border-radius: var(--radius-md);
  font-size: var(--font-sm);
  cursor: pointer;
  transition: all var(--duration-fast);

  &:hover {
    background: var(--gray-50);
  }

  &--active {
    background: var(--primary-500);
    color: white;
    border-color: var(--primary-500);
  }

  &:focus-visible {
    outline: 2px solid var(--primary-500);
    outline-offset: 2px;
  }
}

.data-table__pagination-size {
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--gray-300);
  background: white;
  border-radius: var(--radius-md);
  font-size: var(--font-sm);
  cursor: pointer;

  &:focus-visible {
    outline: 2px solid var(--primary-500);
    outline-offset: 2px;
  }
}

// Responsive
@media (max-width: 768px) {
  .data-table__toolbar {
    flex-direction: column;
    align-items: stretch;
  }

  .data-table__pagination {
    flex-direction: column;
    gap: var(--space-3);
  }

  .data-table__container {
    overflow-x: auto;
  }
}
</style>
