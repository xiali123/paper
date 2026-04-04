<template>
  <div class="filter-panel" :class="{ 'filter-panel--collapsed': isCollapsed }">
    <!-- Header -->
    <div class="filter-panel__header">
      <h3 class="filter-panel__title">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
          <path d="M10 18h4v-2h-4v2zM3 6v2h18V6H3zm3 7h12v-2H6v2z" />
        </svg>
        Filters
        <span v-if="activeFilterCount > 0" class="filter-panel__count">
          {{ activeFilterCount }}
        </span>
      </h3>
      <div class="filter-panel__actions">
        <button
          v-if="hasActiveFilters"
          class="filter-panel__clear"
          @click="clearAllFilters"
          aria-label="Clear all filters"
        >
          Clear All
        </button>
        <button
          class="filter-panel__toggle"
          @click="toggleCollapse"
          :aria-expanded="!isCollapsed"
          aria-label="Toggle filter panel"
        >
          <svg
            width="20"
            height="20"
            viewBox="0 0 24 24"
            fill="currentColor"
            :class="{ 'filter-panel__toggle-icon--rotated': !isCollapsed }"
          >
            <path d="M7.41 8.59L12 13.17l4.59-4.58L18 10l-6 6-6-6 1.41-1.41z" />
          </svg>
        </button>
      </div>
    </div>

    <!-- Filters -->
    <div v-show="!isCollapsed" class="filter-panel__body">
      <!-- Checkbox filters -->
      <div
        v-for="filter in checkboxFilters"
        :key="filter.key"
        class="filter-panel__section"
      >
        <h4 class="filter-panel__section-title">{{ filter.label }}</h4>
        <div class="filter-panel__checkbox-group">
          <label
            v-for="option in filter.options"
            :key="option.value"
            class="filter-panel__checkbox-item"
          >
            <input
              type="checkbox"
              :checked="isFilterActive(filter.key, option.value)"
              @change="toggleCheckboxFilter(filter.key, option.value)"
              :aria-label="option.label"
            />
            <span class="filter-panel__checkbox-label">{{ option.label }}</span>
            <span v-if="option.count" class="filter-panel__checkbox-count">
              {{ option.count }}
            </span>
          </label>
        </div>
      </div>

      <!-- Date range filters -->
      <div
        v-for="filter in dateRangeFilters"
        :key="filter.key"
        class="filter-panel__section"
      >
        <h4 class="filter-panel__section-title">{{ filter.label }}</h4>
        <div class="filter-panel__date-range">
          <input
            type="date"
            :value="getDateRangeValue(filter.key, 'start')"
            @input="setDateRange(filter.key, 'start', $event)"
            :placeholder="filter.startPlaceholder || 'From'"
            class="filter-panel__date-input"
            :aria-label="`${filter.label} start date`"
          />
          <span class="filter-panel__date-separator">to</span>
          <input
            type="date"
            :value="getDateRangeValue(filter.key, 'end')"
            @input="setDateRange(filter.key, 'end', $event)"
            :placeholder="filter.endPlaceholder || 'To'"
            class="filter-panel__date-input"
            :aria-label="`${filter.label} end date`"
          />
        </div>
      </div>

      <!-- Slider range filters -->
      <div
        v-for="filter in sliderFilters"
        :key="filter.key"
        class="filter-panel__section"
      >
        <h4 class="filter-panel__section-title">
          {{ filter.label }}
          <span class="filter-panel__slider-value">
            {{ getSliderValue(filter.key) }}
          </span>
        </h4>
        <input
          type="range"
          :min="filter.min"
          :max="filter.max"
          :step="filter.step || 1"
          :value="getSliderValue(filter.key)"
          @input="setSliderValue(filter.key, $event)"
          class="filter-panel__slider"
          :aria-label="filter.label"
        />
        <div class="filter-panel__slider-labels">
          <span>{{ filter.min }}</span>
          <span>{{ filter.max }}</span>
        </div>
      </div>

      <!-- Custom filter slots -->
      <slot name="custom-filters" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'

/**
 * Checkbox option interface
 */
interface CheckboxOption {
  value: string
  label: string
  count?: number
}

/**
 * Checkbox filter interface
 */
interface CheckboxFilter {
  key: string
  label: string
  options: CheckboxOption[]
}

/**
 * Date range filter interface
 */
interface DateRangeFilter {
  key: string
  label: string
  startPlaceholder?: string
  endPlaceholder?: string
}

/**
 * Slider filter interface
 */
interface SliderFilter {
  key: string
  label: string
  min: number
  max: number
  step?: number
}

/**
 * Component props for FilterPanel
 */
interface Props {
  /** Array of checkbox filters */
  checkboxFilters?: CheckboxFilter[]
  /** Array of date range filters */
  dateRangeFilters?: DateRangeFilter[]
  /** Array of slider filters */
  sliderFilters?: SliderFilter[]
  /** Initial collapsed state */
  initiallyCollapsed?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  checkboxFilters: () => [],
  dateRangeFilters: () => [],
  sliderFilters: () => [],
  initiallyCollapsed: false
})

/**
 * Component events
 */
const emit = defineEmits<{
  /** Fired when filters change */
  change: [filters: Record<string, any>]
  /** Fired when filters are cleared */
  clear: []
}>()

const isCollapsed = ref(props.initiallyCollapsed)
const activeFilters = ref<Record<string, any>>({})

/**
 * Count of active filters
 */
const activeFilterCount = computed(() => {
  let count = 0

  // Count checkbox filters
  props.checkboxFilters.forEach(filter => {
    const values = activeFilters.value[filter.key]
    if (values && values.length > 0) {
      count += values.length
    }
  })

  // Count date range filters
  props.dateRangeFilters.forEach(filter => {
    const range = activeFilters.value[filter.key]
    if (range && (range.start || range.end)) {
      count++
    }
  })

  // Count slider filters
  props.sliderFilters.forEach(filter => {
    const value = activeFilters.value[filter.key]
    if (value !== undefined && value !== filter.min) {
      count++
    }
  })

  return count
})

/**
 * Check if any filters are active
 */
const hasActiveFilters = computed(() => {
  return activeFilterCount.value > 0
})

/**
 * Check if a specific filter option is active
 */
const isFilterActive = (filterKey: string, value: string) => {
  const values = activeFilters.value[filterKey]
  return values && values.includes(value)
}

/**
 * Toggle checkbox filter
 */
const toggleCheckboxFilter = (filterKey: string, value: string) => {
  if (!activeFilters.value[filterKey]) {
    activeFilters.value[filterKey] = []
  }

  const index = activeFilters.value[filterKey].indexOf(value)
  if (index > -1) {
    activeFilters.value[filterKey].splice(index, 1)
    if (activeFilters.value[filterKey].length === 0) {
      delete activeFilters.value[filterKey]
    }
  } else {
    activeFilters.value[filterKey].push(value)
  }

  emitChange()
}

/**
 * Get date range value
 */
const getDateRangeValue = (filterKey: string, type: 'start' | 'end') => {
  const range = activeFilters.value[filterKey]
  return range ? range[type] || '' : ''
}

/**
 * Set date range value
 */
const setDateRange = (filterKey: string, type: 'start' | 'end', event: Event) => {
  const target = event.target as HTMLInputElement
  const value = target.value

  if (!activeFilters.value[filterKey]) {
    activeFilters.value[filterKey] = { start: '', end: '' }
  }

  activeFilters.value[filterKey][type] = value

  // Remove filter if both dates are empty
  if (!activeFilters.value[filterKey].start && !activeFilters.value[filterKey].end) {
    delete activeFilters.value[filterKey]
  }

  emitChange()
}

/**
 * Get slider value
 */
const getSliderValue = (filterKey: string) => {
  const filter = props.sliderFilters.find(f => f.key === filterKey)
  return activeFilters.value[filterKey] || (filter ? filter.min : 0)
}

/**
 * Set slider value
 */
const setSliderValue = (filterKey: string, event: Event) => {
  const target = event.target as HTMLInputElement
  const value = parseInt(target.value, 10)

  const filter = props.sliderFilters.find(f => f.key === filterKey)
  if (value === (filter ? filter.min : 0)) {
    delete activeFilters.value[filterKey]
  } else {
    activeFilters.value[filterKey] = value
  }

  emitChange()
}

/**
 * Clear all filters
 */
const clearAllFilters = () => {
  activeFilters.value = {}
  emit('clear')
  emitChange()
}

/**
 * Toggle collapse state
 */
const toggleCollapse = () => {
  isCollapsed.value = !isCollapsed.value
}

/**
 * Emit change event with current filters
 */
const emitChange = () => {
  emit('change', { ...activeFilters.value })
}

/**
 * Expose methods for parent components
 */
defineExpose({
  getFilters: () => ({ ...activeFilters.value }),
  setFilters: (filters: Record<string, any>) => {
    activeFilters.value = { ...filters }
    emitChange()
  },
  clearFilters: clearAllFilters,
  isActive: () => hasActiveFilters.value
})
</script>

<style scoped lang="scss">
.filter-panel {
  background: white;
  border-radius: var(--radius-lg);
  box-shadow: var(--shadow-sm);
  border: 1px solid var(--gray-200);
  overflow: hidden;
}

.filter-panel__header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: var(--space-4);
  border-bottom: 1px solid var(--gray-200);
}

.filter-panel__title {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  margin: 0;
  font-size: var(--font-base);
  font-weight: var(--font-semibold);
  color: var(--gray-900);
}

.filter-panel__count {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 20px;
  height: 20px;
  padding: 0 var(--space-1);
  font-size: var(--font-xs);
  font-weight: var(--font-semibold);
  background: var(--primary-500);
  color: white;
  border-radius: var(--radius-full);
}

.filter-panel__actions {
  display: flex;
  gap: var(--space-2);
  align-items: center;
}

.filter-panel__clear {
  padding: var(--space-2) var(--space-3);
  border: none;
  background: transparent;
  color: var(--primary-600);
  font-size: var(--font-sm);
  font-weight: var(--font-medium);
  cursor: pointer;
  border-radius: var(--radius-md);
  transition: all var(--duration-fast);

  &:hover {
    background: var(--primary-50);
  }

  &:focus-visible {
    outline: 2px solid var(--primary-500);
    outline-offset: 2px;
  }
}

.filter-panel__toggle {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  border: none;
  background: transparent;
  color: var(--gray-600);
  cursor: pointer;
  border-radius: var(--radius-md);
  transition: all var(--duration-fast);

  &:hover {
    background: var(--gray-100);
  }

  &:focus-visible {
    outline: 2px solid var(--primary-500);
    outline-offset: 2px;
  }
}

.filter-panel__toggle-icon--rotated {
  transform: rotate(180deg);
}

.filter-panel__body {
  padding: var(--space-4);
  max-height: 500px;
  overflow-y: auto;
}

.filter-panel__section {
  margin-bottom: var(--space-6);

  &:last-child {
    margin-bottom: 0;
  }
}

.filter-panel__section-title {
  margin: 0 0 var(--space-3) 0;
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  color: var(--gray-700);
  display: flex;
  align-items: center;
  gap: var(--space-2);
}

.filter-panel__slider-value {
  margin-left: auto;
  font-weight: var(--font-normal);
  color: var(--primary-600);
}

.filter-panel__checkbox-group {
  display: flex;
  flex-direction: column;
  gap: var(--space-2);
}

.filter-panel__checkbox-item {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2);
  border-radius: var(--radius-md);
  cursor: pointer;
  transition: background var(--duration-fast);

  &:hover {
    background: var(--gray-50);
  }

  input[type="checkbox"] {
    width: 18px;
    height: 18px;
    cursor: pointer;

    &:focus-visible {
      outline: 2px solid var(--primary-500);
      outline-offset: 2px;
    }
  }
}

.filter-panel__checkbox-label {
  flex: 1;
  font-size: var(--font-sm);
  color: var(--gray-700);
  cursor: pointer;
}

.filter-panel__checkbox-count {
  font-size: var(--font-xs);
  color: var(--gray-500);
}

.filter-panel__date-range {
  display: flex;
  align-items: center;
  gap: var(--space-2);
}

.filter-panel__date-input {
  flex: 1;
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--gray-300);
  border-radius: var(--radius-md);
  font-size: var(--font-sm);
  color: var(--gray-900);
  background: white;

  &:focus {
    outline: none;
    border-color: var(--primary-500);
    box-shadow: 0 0 0 3px rgba(33, 150, 243, 0.1);
  }
}

.filter-panel__date-separator {
  font-size: var(--font-sm);
  color: var(--gray-500);
}

.filter-panel__slider {
  width: 100%;
  margin-bottom: var(--space-2);
  cursor: pointer;

  &:focus {
    outline: none;
  }

  &::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 18px;
    height: 18px;
    border-radius: 50%;
    background: var(--primary-500);
    cursor: pointer;
    transition: background var(--duration-fast);

    &:hover {
      background: var(--primary-600);
    }
  }

  &::-moz-range-thumb {
    width: 18px;
    height: 18px;
    border-radius: 50%;
    background: var(--primary-500);
    cursor: pointer;
    border: none;
    transition: background var(--duration-fast);

    &:hover {
      background: var(--primary-600);
    }
  }

  &::-webkit-slider-runnable-track {
    height: 6px;
    background: var(--gray-200);
    border-radius: var(--radius-full);
  }

  &::-moz-range-track {
    height: 6px;
    background: var(--gray-200);
    border-radius: var(--radius-full);
  }
}

.filter-panel__slider-labels {
  display: flex;
  justify-content: space-between;
  font-size: var(--font-xs);
  color: var(--gray-500);
}

// Responsive
@media (max-width: 640px) {
  .filter-panel__header {
    flex-direction: column;
    align-items: flex-start;
    gap: var(--space-3);
  }

  .filter-panel__actions {
    width: 100%;
    justify-content: space-between;
  }

  .filter-panel__date-range {
    flex-direction: column;
    align-items: stretch;
  }

  .filter-panel__date-separator {
    text-align: center;
  }
}
</style>
