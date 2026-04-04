<template>
  <div class="search-bar" :class="{ 'search-bar--focused': isFocused }">
    <div class="search-bar__input-wrapper">
      <svg class="search-bar__icon" width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" />
      </svg>

      <input
        ref="inputRef"
        v-model="searchQuery"
        type="text"
        class="search-bar__input"
        :placeholder="placeholder"
        @focus="isFocused = true"
        @blur="isFocused = false"
        @input="handleInput"
        @keydown.enter="handleSearch"
        @keydown.escape="handleClear"
        @keydown="handleKeydown"
        aria-label="Search papers"
        role="searchbox"
        :aria-expanded="showSuggestions || showHistory"
        :aria-controls="showSuggestions ? 'suggestions-list' : showHistory ? 'history-list' : undefined"
        :aria-activedescendant="activeDescendant"
      />

      <button
        v-if="searchQuery"
        class="search-bar__clear"
        @click="handleClear"
        aria-label="Clear search"
      >
        <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">
          <path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z" />
        </svg>
      </button>
    </div>

    <button
      class="search-bar__button"
      @click="handleSearch"
      :disabled="!searchQuery"
    >
      Search
    </button>

    <!-- Advanced search toggle -->
    <button
      v-if="showAdvancedToggle"
      class="search-bar__advanced-toggle"
      @click="toggleAdvanced"
      :aria-expanded="showAdvanced"
      aria-label="Toggle advanced search"
    >
      <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M3 17v2h6v-2H3zM3 5v2h10V5H3zm10 16v-2h8v-2h-8v-2h-2v6h2zM7 9v2H3v2h4v2h2V9H7zm14 4v-2H11v2h10zm-6-4h2V7h4V5h-4V3h-2v6z" />
      </svg>
    </button>

    <!-- Advanced search panel -->
    <div v-if="showAdvanced" class="search-bar__advanced-panel">
      <slot name="advanced" />
    </div>

    <!-- Autocomplete suggestions -->
    <div
      v-if="showSuggestions && suggestions.length > 0"
      id="suggestions-list"
      class="search-bar__suggestions"
      role="listbox"
    >
      <div
        v-for="(suggestion, index) in suggestions"
        :id="`suggestion-${index}`"
        :key="index"
        class="search-bar__suggestion"
        :class="{ 'search-bar__suggestion--active': index === activeSuggestion }"
        role="option"
        :aria-selected="index === activeSuggestion"
        @click="selectSuggestion(suggestion)"
        @mouseenter="activeSuggestion = index"
      >
        <svg class="search-bar__suggestion-icon" width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
          <path d="M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 3.15.75-1.23-4.5-2.67z" />
        </svg>
        <span class="search-bar__suggestion-text">{{ suggestion.text }}</span>
        <span class="search-bar__suggestion-count">{{ suggestion.count }}</span>
      </div>
    </div>

    <!-- Search history -->
    <div
      v-if="showHistory && searchHistory.length > 0"
      id="history-list"
      class="search-bar__history"
      role="listbox"
    >
      <div class="search-bar__history-header">
        <span>Recent Searches</span>
        <button @click="clearHistory" class="search-bar__history-clear">Clear</button>
      </div>
      <div
        v-for="(item, index) in searchHistory"
        :id="`history-${index}`"
        :key="index"
        class="search-bar__history-item"
        role="option"
        @click="selectHistory(item)"
      >
        <svg class="search-bar__history-icon" width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
          <path d="M13 3c-4.97 0-9 4.03-9 9H1l3.89 3.89.07.14L9 12H6c0-3.87 3.13-7 7-7s7 3.13 7 7-3.13 7-7 7c-1.93 0-3.68-.79-4.94-2.06l-1.42 1.42C8.27 19.99 10.51 21 13 21c4.97 0 9-4.03 9-9s-4.03-9-9-9zm-1 5v5l4.28 2.54.72-1.21-3.5-2.08V8H12z" />
        </svg>
        <span>{{ item }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'

/**
 * Search suggestion interface
 */
interface SearchSuggestion {
  text: string
  count: number
}

/**
 * Component props for SearchBar
 */
interface Props {
  /** Placeholder text for the search input */
  placeholder?: string
  /** Debounce time for input in milliseconds */
  debounceTime?: number
  /** Maximum number of suggestions to show */
  maxSuggestions?: number
  /** Maximum number of history items to keep */
  maxHistoryItems?: number
  /** Whether to show advanced search toggle */
  showAdvancedToggle?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  placeholder: 'Search papers by title, author, DOI...',
  debounceTime: 300,
  maxSuggestions: 8,
  maxHistoryItems: 10,
  showAdvancedToggle: false
})

/**
 * Component events
 */
const emit = defineEmits<{
  /** Fired when search is triggered */
  search: [query: string]
  /** Fired when suggestions are requested */
  suggest: [query: string]
  /** Fired when advanced search is toggled */
  toggleAdvanced: [show: boolean]
}>()

const inputRef = ref<HTMLInputElement>()
const isFocused = ref(false)
const searchQuery = ref('')
const showSuggestions = ref(false)
const showHistory = ref(false)
const showAdvanced = ref(false)
const activeSuggestion = ref(0)
const suggestions = ref<SearchSuggestion[]>([])
const searchHistory = ref<string[]>([])
let debounceTimer: number | null = null

/**
 * Get the ID of the active descendant for ARIA
 */
const activeDescendant = computed(() => {
  if (showSuggestions.value && suggestions.value.length > 0) {
    return `suggestion-${activeSuggestion.value}`
  }
  return undefined
})

/**
 * Handle input with debouncing
 */
const handleInput = () => {
  if (debounceTimer) {
    clearTimeout(debounceTimer)
  }

  debounceTimer = setTimeout(() => {
    if (searchQuery.value.trim()) {
      emit('suggest', searchQuery.value)
      showSuggestions.value = true
      showHistory.value = false
    } else {
      showSuggestions.value = false
      showHistory.value = true
    }
  }, props.debounceTime) as unknown as number
}

/**
 * Handle search action
 */
const handleSearch = () => {
  if (searchQuery.value.trim()) {
    emit('search', searchQuery.value)
    showSuggestions.value = false
    showHistory.value = false
    showAdvanced.value = false

    // Add to history
    if (!searchHistory.value.includes(searchQuery.value)) {
      searchHistory.value.unshift(searchQuery.value)
      if (searchHistory.value.length > props.maxHistoryItems) {
        searchHistory.value.pop()
      }
      // Persist to localStorage
      try {
        localStorage.setItem('searchHistory', JSON.stringify(searchHistory.value))
      } catch (e) {
        console.warn('Could not save search history:', e)
      }
    }
  }
}

/**
 * Clear search input
 */
const handleClear = () => {
  searchQuery.value = ''
  showSuggestions.value = false
  showHistory.value = false
  inputRef.value?.focus()
}

/**
 * Select a suggestion
 */
const selectSuggestion = (suggestion: SearchSuggestion) => {
  searchQuery.value = suggestion.text
  handleSearch()
}

/**
 * Select a history item
 */
const selectHistory = (item: string) => {
  searchQuery.value = item
  handleSearch()
}

/**
 * Clear search history
 */
const clearHistory = () => {
  searchHistory.value = []
  try {
    localStorage.removeItem('searchHistory')
  } catch (e) {
    console.warn('Could not clear search history:', e)
  }
}

/**
 * Toggle advanced search panel
 */
const toggleAdvanced = () => {
  showAdvanced.value = !showAdvanced.value
  emit('toggleAdvanced', showAdvanced.value)
}

/**
 * Handle keyboard navigation
 */
const handleKeydown = (e: KeyboardEvent) => {
  if (!showSuggestions.value) return

  if (e.key === 'ArrowDown') {
    e.preventDefault()
    activeSuggestion.value = Math.min(activeSuggestion.value + 1, suggestions.value.length - 1)
  } else if (e.key === 'ArrowUp') {
    e.preventDefault()
    activeSuggestion.value = Math.max(activeSuggestion.value - 1, 0)
  } else if (e.key === 'Enter' && suggestions.value.length > 0) {
    e.preventDefault()
    selectSuggestion(suggestions.value[activeSuggestion.value])
  }
}

/**
 * Load search history from localStorage on mount
 */
try {
  const savedHistory = localStorage.getItem('searchHistory')
  if (savedHistory) {
    searchHistory.value = JSON.parse(savedHistory)
  }
} catch (e) {
  console.warn('Could not load search history:', e)
}

/**
 * Expose methods for parent components
 */
defineExpose({
  focus: () => inputRef.value?.focus(),
  clear: handleClear,
  setValue: (value: string) => {
    searchQuery.value = value
  },
  getValue: () => searchQuery.value,
  setSuggestions: (newSuggestions: SearchSuggestion[]) => {
    suggestions.value = newSuggestions.slice(0, props.maxSuggestions)
  }
})
</script>

<style scoped lang="scss">
// ==========================================
// 现代化搜索栏组件样式
// Modern SearchBar Component Styles
// ==========================================

.search-bar {
  position: relative;
  display: flex;
  gap: $spacing-4;
  align-items: center;
  max-width: 700px;
  width: 100%;
  margin: 0 auto;
}

.search-bar__input-wrapper {
  position: relative;
  flex: 1;
  display: flex;
  align-items: center;
  gap: $spacing-4;
  padding: $spacing-5 $spacing-6;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border: 3px solid $border-light;
  border-radius: $border-radius-2xl;
  transition: all $duration-fast;
  box-shadow: $shadow-lg;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    border-color: $gray-600;
  }

  .search-bar--focused & {
    border-color: $primary-400;
    box-shadow: 0 0 0 6px rgba($primary-500, 0.15), $shadow-2xl;
    transform: translateY(-2px);
  }

  &:hover {
    border-color: rgba($primary-300, 0.5);
  }
}

.search-bar__icon {
  width: 24px;
  height: 24px;
  color: $primary-500;
  flex-shrink: 0;
  filter: drop-shadow(0 2px 4px rgba($primary-500, 0.3));

  .dark & {
    color: $primary-400;
    filter: drop-shadow(0 2px 4px rgba($primary-400, 0.3));
  }
}

.search-bar__input {
  flex: 1;
  border: none;
  outline: none;
  font-size: $font-size-lg;
  font-weight: $font-weight-medium;
  color: $text-primary;
  background: transparent;

  &::placeholder {
    color: $text-secondary;
    font-weight: $font-weight-normal;
  }

  .dark & {
    color: $gray-100;

    &::placeholder {
      color: $gray-500;
    }
  }
}

.search-bar__clear {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  border: none;
  background: linear-gradient(135deg, $gray-200 0%, $gray-300 100%);
  border-radius: $border-radius-full;
  color: $text-secondary;
  cursor: pointer;
  transition: all $duration-fast;
  box-shadow: $shadow-sm;

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-600 100%);
    color: $gray-300;
  }

  &:hover {
    background: linear-gradient(135deg, $danger-color 0%, rgba($danger-color, 0.8) 100%);
    color: #ffffff;
    transform: rotate(90deg) scale(1.1);
    box-shadow: $shadow-md;
  }

  &:focus-visible {
    outline: 3px solid rgba($primary-500, 0.3);
    outline-offset: 2px;
  }

  svg {
    width: 18px;
    height: 18px;
  }
}

.search-bar__button {
  padding: $spacing-5 $spacing-8;
  border: none;
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  color: #ffffff;
  font-size: $font-size-base;
  font-weight: $font-weight-bold;
  border-radius: $border-radius-2xl;
  cursor: pointer;
  transition: all $duration-fast;
  white-space: nowrap;
  box-shadow: $shadow-lg;
  letter-spacing: 0.5px;

  &:hover:not(:disabled) {
    background: linear-gradient(135deg, $primary-600 0%, $primary-700 100%);
    transform: translateY(-3px);
    box-shadow: $shadow-xl;
  }

  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
    transform: none;
  }

  &:focus-visible {
    outline: 3px solid rgba($primary-500, 0.4);
    outline-offset: 3px;
  }
}

.search-bar__advanced-toggle {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 52px;
  height: 52px;
  border: 2px solid $border-light;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border-radius: $border-radius-2xl;
  color: $text-secondary;
  cursor: pointer;
  transition: all $duration-fast;
  box-shadow: $shadow-md;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    border-color: $gray-600;
    color: $gray-400;
  }

  &:hover {
    border-color: $primary-400;
    color: $primary-600;
    background: linear-gradient(135deg, $primary-50 0%, rgba($primary-100, 0.5) 100%);
    transform: translateY(-2px) scale(1.05);
    box-shadow: $shadow-lg;
  }

  .dark &:hover {
    background: linear-gradient(135deg, rgba($primary-900, 0.4) 0%, rgba($primary-800, 0.3) 100%);
    color: $primary-400;
  }

  &:focus-visible {
    outline: 3px solid rgba($primary-500, 0.4);
    outline-offset: 3px;
  }

  svg {
    width: 24px;
    height: 24px;
  }
}

.search-bar__advanced-panel {
  position: absolute;
  top: calc(100% + $spacing-4);
  left: 0;
  right: 0;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border-radius: $border-radius-xl;
  box-shadow: $shadow-2xl;
  border: 2px solid rgba($primary-200, 0.5);
  z-index: 1000;
  padding: $spacing-6;
  animation: slideDown 0.3s ease-out;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    border-color: $gray-600;
  }
}

@keyframes slideDown {
  from {
    opacity: 0;
    transform: translateY(-10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.search-bar__suggestions,
.search-bar__history {
  position: absolute;
  top: calc(100% + $spacing-4);
  left: 0;
  right: 0;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border-radius: $border-radius-xl;
  box-shadow: $shadow-2xl;
  border: 2px solid rgba($primary-200, 0.5);
  z-index: 1000;
  max-height: 400px;
  overflow-y: auto;
  animation: slideDown 0.3s ease-out;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    border-color: $gray-600;
  }

  // 自定义滚动条
  &::-webkit-scrollbar {
    width: 8px;
  }

  &::-webkit-scrollbar-track {
    background: transparent;
  }

  &::-webkit-scrollbar-thumb {
    background: rgba($gray-400, 0.3);
    border-radius: $border-radius-full;

    &:hover {
      background: rgba($gray-400, 0.5);
    }
  }
}

.search-bar__suggestion,
.search-bar__history-item {
  display: flex;
  align-items: center;
  gap: $spacing-4;
  padding: $spacing-5 $spacing-6;
  cursor: pointer;
  transition: all $duration-fast;
  border-left: 3px solid transparent;

  &:hover,
  &--active {
    background: linear-gradient(90deg, rgba($primary-50, 0.8) 0%, transparent 100%);
    border-left-color: $primary-500;
    transform: translateX(4px);
  }

  .dark &:hover,
  .dark &--active {
    background: linear-gradient(90deg, rgba($primary-900, 0.4) 0%, transparent 100%);
    border-left-color: $primary-400;
  }
}

.search-bar__suggestion-icon,
.search-bar__history-icon {
  width: 20px;
  height: 20px;
  color: $primary-500;
  flex-shrink: 0;

  .dark & {
    color: $primary-400;
  }
}

.search-bar__suggestion-text {
  flex: 1;
  font-size: $font-size-base;
  font-weight: $font-weight-medium;
  color: $text-primary;

  .dark & {
    color: $gray-100;
  }
}

.search-bar__suggestion-count {
  font-size: $font-size-sm;
  font-weight: $font-weight-semibold;
  color: $text-secondary;
  padding: $spacing-2 $spacing-4;
  background: linear-gradient(135deg, $gray-100 0%, $gray-200 100%);
  border-radius: $border-radius-full;

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    color: $gray-400;
  }
}

.search-bar__history-item span {
  flex: 1;
  font-size: $font-size-base;
  font-weight: $font-weight-medium;
  color: $text-primary;

  .dark & {
    color: $gray-100;
  }
}

.search-bar__history-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: $spacing-4 $spacing-6;
  border-bottom: 2px solid $border-light;
  font-size: $font-size-sm;
  font-weight: $font-weight-bold;
  color: $text-primary;
  background: linear-gradient(135deg, rgba($primary-50, 0.5) 0%, rgba($primary-100, 0.3) 100%);

  .dark & {
    border-bottom-color: $gray-700;
    color: $gray-100;
    background: linear-gradient(135deg, rgba($gray-700, 0.5) 0%, rgba($gray-800, 0.3) 100%);
  }
}

.search-bar__history-clear {
  border: none;
  background: transparent;
  color: $danger-color;
  cursor: pointer;
  font-size: $font-size-sm;
  font-weight: $font-weight-semibold;
  padding: $spacing-2 $spacing-4;
  border-radius: $border-radius-base;
  transition: all $duration-fast;

  &:hover {
    background: rgba($danger-color, 0.1);
    transform: scale(1.05);
  }
}

// Responsive Design
@media (max-width: 768px) {
  .search-bar {
    max-width: 100%;
    flex-wrap: wrap;
  }

  .search-bar__input-wrapper {
    order: 1;
    min-width: 0;
  }

  .search-bar__button {
    order: 3;
    width: 100%;
    flex: 1;
  }

  .search-bar__advanced-toggle {
    order: 2;
  }

  .search-bar__advanced-panel {
    position: relative;
    top: 0;
    margin-top: $spacing-4;
  }
}
</style>
