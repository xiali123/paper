<template>
  <div class="advanced-search-view">
    <div class="search-form">
      <!-- Search Query Builder -->
      <div class="form-section">
        <h3 class="section-title">Search Query Builder</h3>

        <!-- Boolean Operators -->
        <div class="boolean-operators">
          <div class="operator-label">Boolean Operator:</div>
          <div class="operator-buttons">
            <button
              v-for="operator in booleanOperators"
              :key="operator.value"
              @click="setBooleanOperator(operator.value)"
              :class="{ 'active': searchQuery.booleanOperator === operator.value }"
              class="operator-btn"
            >
              {{ operator.label }}
            </button>
          </div>
        </div>

        <!-- Search Fields -->
        <div class="search-fields">
          <div
            v-for="(field, index) in searchQuery.fields"
            :key="index"
            class="search-field-row"
          >
            <div class="field-controls">
              <select v-model="field.field" class="field-select">
                <option value="">Select Field</option>
                <option value="title">Title</option>
                <option value="authors">Authors</option>
                <option value="abstract">Abstract</option>
                <option value="keywords">Keywords</option>
                <option value="journal">Journal</option>
                <option value="year">Year</option>
              </select>

              <input
                v-model="field.value"
                type="text"
                :placeholder="getFieldPlaceholder(field.field)"
                class="field-input"
              />

              <button
                v-if="searchQuery.fields.length > 1"
                @click="removeField(index)"
                class="remove-field-btn"
                title="Remove field"
              >
                <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
                  <path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z" />
                </svg>
              </button>
            </div>

            <div v-if="index < searchQuery.fields.length - 1" class="field-operator">
              {{ getBooleanLabel(searchQuery.booleanOperator) }}
            </div>
          </div>

          <button @click="addField" class="add-field-btn">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
              <path d="M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z" />
            </svg>
            Add Field
          </button>
        </div>

        <!-- Quick Search Templates -->
        <div class="search-templates">
          <label class="template-label">Quick Templates:</label>
          <div class="template-buttons">
            <button
              v-for="template in searchTemplates"
              :key="template.id"
              @click="applyTemplate(template)"
              class="template-btn"
            >
              {{ template.name }}
            </button>
          </div>
        </div>
      </div>

      <!-- Filters Section -->
      <div class="form-section">
        <h3 class="section-title">Filters</h3>

        <!-- Year Range -->
        <div class="filter-group">
          <label class="filter-label">Publication Year</label>
          <div class="year-range">
            <div class="year-input-group">
              <label class="year-label">From:</label>
              <input
                v-model.number="filters.yearFrom"
                type="number"
                min="1900"
                :max="currentYear"
                placeholder="1900"
                class="year-input"
              />
            </div>
            <div class="year-input-group">
              <label class="year-label">To:</label>
              <input
                v-model.number="filters.yearTo"
                type="number"
                min="1900"
                :max="currentYear"
                :placeholder="currentYear.toString()"
                class="year-input"
              />
            </div>
          </div>
          <div class="year-presets">
            <button
              v-for="preset in yearPresets"
              :key="preset.value"
              @click="setYearPreset(preset.value)"
              class="year-preset-btn"
            >
              {{ preset.label }}
            </button>
          </div>
        </div>

        <!-- CCF Level -->
        <div class="filter-group">
          <label class="filter-label">CCF Level</label>
          <div class="ccf-levels">
            <label
              v-for="level in ccfLevels"
              :key="level.value"
              class="ccf-level-checkbox"
            >
              <input
                v-model="filters.ccfLevels"
                type="checkbox"
                :value="level.value"
                class="checkbox-input"
              />
              <span class="checkbox-label">{{ level.label }}</span>
            </label>
          </div>
        </div>

        <!-- Citation Range -->
        <div class="filter-group">
          <label class="filter-label">Citation Count</label>
          <div class="citation-range">
            <div class="range-input-group">
              <label class="range-label">Minimum:</label>
              <input
                v-model.number="filters.citationsFrom"
                type="number"
                min="0"
                placeholder="0"
                class="range-input"
              />
            </div>
            <div class="range-input-group">
              <label class="range-label">Maximum:</label>
              <input
                v-model.number="filters.citationsTo"
                type="number"
                min="0"
                placeholder="Any"
                class="range-input"
              />
            </div>
          </div>
        </div>

        <!-- Journal/Conference -->
        <div class="filter-group">
          <label class="filter-label">Journal/Conference</label>
          <input
            v-model="filters.journal"
            type="text"
            placeholder="Enter journal or conference name"
            class="journal-input"
          />
          <div class="journal-suggestions">
            <button
              v-for="journal in popularJournals"
              :key="journal"
              @click="filters.journal = journal"
              class="journal-suggestion-btn"
            >
              {{ journal }}
            </button>
          </div>
        </div>

        <!-- Paper Type -->
        <div class="filter-group">
          <label class="filter-label">Paper Type</label>
          <div class="paper-types">
            <label
              v-for="type in paperTypes"
              :key="type.value"
              class="paper-type-radio"
            >
              <input
                v-model="filters.paperType"
                type="radio"
                :value="type.value"
                name="paperType"
                class="radio-input"
              />
              <span class="radio-label">{{ type.label }}</span>
            </label>
          </div>
        </div>

        <!-- Language -->
        <div class="filter-group">
          <label class="filter-label">Language</label>
          <select v-model="filters.language" class="language-select">
            <option value="">All Languages</option>
            <option value="en">English</option>
            <option value="zh">Chinese</option>
            <option value="es">Spanish</option>
            <option value="fr">French</option>
            <option value="de">German</option>
          </select>
        </div>
      </div>

      <!-- Saved Searches -->
      <div class="form-section" v-if="savedSearches.length > 0">
        <h3 class="section-title">Saved Searches</h3>
        <div class="saved-searches">
          <div
            v-for="saved in savedSearches"
            :key="saved.id"
            class="saved-search-item"
          >
            <div class="saved-search-info">
              <h4 class="saved-search-name">{{ saved.name }}</h4>
              <p class="saved-search-details">
                {{ getSavedSearchDescription(saved) }}
              </p>
              <p class="saved-search-date">
                Last used: {{ formatDate(saved.lastUsed) }}
              </p>
            </div>
            <div class="saved-search-actions">
              <button
                @click="loadSavedSearch(saved.id)"
                class="load-search-btn"
                title="Load search"
              >
                <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
                  <path d="M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z" />
                </svg>
              </button>
              <button
                @click="deleteSavedSearch(saved.id)"
                class="delete-search-btn"
                title="Delete search"
              >
                <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
                  <path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z" />
                </svg>
              </button>
            </div>
          </div>
        </div>
      </div>

      <!-- Actions -->
      <div class="form-actions">
        <div class="save-search">
          <input
            v-model="saveSearchName"
            type="text"
            placeholder="Search name to save..."
            class="save-search-input"
          />
          <button
            @click="saveCurrentSearch"
            :disabled="!saveSearchName.trim()"
            class="save-search-btn"
          >
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
              <path d="M17 3H5c-1.11 0-2 .9-2 2v14c0 1.1.89 2 2 2h14c1.1 0 2-.9 2-2V7l-4-4zm-5 16c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3zm3-10H5V5h10v4z" />
            </svg>
            Save Search
          </button>
        </div>

        <div class="action-buttons">
          <button @click="$emit('cancel')" class="cancel-btn">
            Cancel
          </button>
          <button @click="resetForm" class="reset-btn">
            Reset
          </button>
          <button
            @click="performSearch"
            :disabled="!isFormValid"
            class="search-btn"
          >
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
              <path d="M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" />
            </svg>
            Search
          </button>
        </div>
      </div>

      <!-- Search Preview -->
      <Transition name="slide">
        <div v-if="searchQueryPreview" class="search-preview">
          <div class="preview-header">
            <h4>Search Query Preview</h4>
            <button @click="copyQuery" class="copy-query-btn" title="Copy query">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
                <path d="M16 1H4c-1.1 0-2 .9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z" />
              </svg>
            </button>
          </div>
          <div class="preview-content">
            <code>{{ searchQueryPreview }}</code>
          </div>
        </div>
      </Transition>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useSearchStore } from '@/stores/searchStore'
import { searchApi } from '@/api/modules/search'

const searchStore = useSearchStore()

// Emits
const emit = defineEmits<{
  search: [params: any]
  cancel: []
}>()

// Constants
const currentYear = new Date().getFullYear()

const booleanOperators = [
  { value: 'AND', label: 'AND' },
  { value: 'OR', label: 'OR' },
  { value: 'NOT', label: 'NOT' }
]

const searchTemplates = [
  {
    id: 'recent-ml',
    name: 'Recent ML Papers',
    fields: [
      { field: 'keywords', value: 'machine learning' },
      { field: 'year', value: currentYear.toString() }
    ],
    booleanOperator: 'AND'
  },
  {
    id: 'deep-learning-survey',
    name: 'Deep Learning Surveys',
    fields: [
      { field: 'title', value: 'survey' },
      { field: 'keywords', value: 'deep learning' }
    ],
    booleanOperator: 'AND'
  },
  {
    id: 'highly-cited',
    name: 'Highly Cited Papers',
    fields: [
      { field: 'title', value: '' }
    ],
    filters: {
      citationsFrom: 100
    }
  },
  {
    id: 'ccf-a-ai',
    name: 'CCF-A AI Conferences',
    fields: [
      { field: 'keywords', value: 'artificial intelligence' }
    ],
    filters: {
      ccfLevels: ['A']
    }
  }
]

const yearPresets = [
  { value: 1, label: 'Last Year' },
  { value: 3, label: 'Last 3 Years' },
  { value: 5, label: 'Last 5 Years' },
  { value: 10, label: 'Last 10 Years' }
]

const ccfLevels = [
  { value: 'A', label: 'CCF-A' },
  { value: 'B', label: 'CCF-B' },
  { value: 'C', label: 'CCF-C' }
]

const paperTypes = [
  { value: '', label: 'All Types' },
  { value: 'journal', label: 'Journal Article' },
  { value: 'conference', label: 'Conference Paper' },
  { value: 'preprint', label: 'Preprint' }
]

const popularJournals = [
  'Nature',
  'Science',
  'Cell',
  'NeurIPS',
  'ICML',
  'ACL',
  'CVPR',
  'ICCV',
  'ECCV',
  'AAAI'
]

// State
const searchQuery = ref<{
  booleanOperator: 'AND' | 'OR' | 'NOT'
  fields: Array<{
    field: string
    value: string
  }>
}>({
  booleanOperator: 'AND',
  fields: [
    { field: '', value: '' }
  ]
})

const filters = ref<{
  yearFrom?: number
  yearTo?: number
  ccfLevels: string[]
  citationsFrom?: number
  citationsTo?: number
  journal?: string
  paperType: string
  language: string
}>({
  ccfLevels: [],
  paperType: '',
  language: ''
})

const saveSearchName = ref('')
const savedSearches = ref<any[]>([])

// Computed
const isFormValid = computed(() => {
  const hasValidFields = searchQuery.value.fields.some(
    field => field.field && field.value.trim()
  )
  const hasFilters = Object.values(filters.value).some(
    value => Array.isArray(value) ? value.length > 0 : value !== '' && value !== undefined
  )
  return hasValidFields || hasFilters
})

const searchQueryPreview = computed(() => {
  const validFields = searchQuery.value.fields.filter(
    field => field.field && field.value.trim()
  )

  if (validFields.length === 0) return ''

  const queryParts = validFields.map(field => {
    if (field.field) {
      return `${field.field}:"${field.value}"`
    }
    return `"${field.value}"`
  })

  let query = queryParts.join(` ${searchQuery.value.booleanOperator} `)

  // Add filters to preview
  const filterParts: string[] = []
  if (filters.value.yearFrom) filterParts.push(`year>=${filters.value.yearFrom}`)
  if (filters.value.yearTo) filterParts.push(`year<=${filters.value.yearTo}`)
  if (filters.value.ccfLevels.length > 0) filterParts.push(`level:[${filters.value.ccfLevels.join(',')}]`)
  if (filters.value.citationsFrom) filterParts.push(`citations>=${filters.value.citationsFrom}`)
  if (filters.value.citationsTo) filterParts.push(`citations<=${filters.value.citationsTo}`)
  if (filters.value.journal) filterParts.push(`journal:"${filters.value.journal}"`)

  if (filterParts.length > 0) {
    query += ` AND ${filterParts.join(' AND ')}`
  }

  return query
})

// Methods
const setBooleanOperator = (operator: 'AND' | 'OR' | 'NOT') => {
  searchQuery.value.booleanOperator = operator
}

const addField = () => {
  searchQuery.value.fields.push({ field: '', value: '' })
}

const removeField = (index: number) => {
  searchQuery.value.fields.splice(index, 1)
}

const getFieldPlaceholder = (field: string) => {
  const placeholders: Record<string, string> = {
    title: 'Enter paper title...',
    authors: 'Enter author names...',
    abstract: 'Enter abstract text...',
    keywords: 'Enter keywords...',
    journal: 'Enter journal name...',
    year: 'Enter year...'
  }
  return placeholders[field] || 'Enter search term...'
}

const getBooleanLabel = (operator: string) => {
  const labels: Record<string, string> = {
    'AND': 'AND',
    'OR': 'OR',
    'NOT': 'NOT'
  }
  return labels[operator] || 'AND'
}

const applyTemplate = (template: any) => {
  searchQuery.value = {
    booleanOperator: template.booleanOperator,
    fields: [...template.fields]
  }

  if (template.filters) {
    Object.assign(filters.value, template.filters)
  }
}

const setYearPreset = (years: number) => {
  const toYear = currentYear
  const fromYear = currentYear - years + 1
  filters.value.yearFrom = fromYear
  filters.value.yearTo = toYear
}

const performSearch = () => {
  const params = buildSearchParams()
  emit('search', params)
}

const buildSearchParams = () => {
  const params: any = {}

  // Build query from fields
  const validFields = searchQuery.value.fields.filter(
    field => field.field && field.value.trim()
  )

  if (validFields.length > 0) {
    const queryParts = validFields.map(field => {
      if (field.field) {
        return `${field.field}:"${field.value}"`
      }
      return `"${field.value}"`
    })

    params.q = queryParts.join(` ${searchQuery.value.booleanOperator} `)
  }

  // Add filters
  if (filters.value.yearFrom) params.yearFrom = filters.value.yearFrom
  if (filters.value.yearTo) params.yearTo = filters.value.yearTo
  if (filters.value.ccfLevels.length > 0) params.ccfLevels = filters.value.ccfLevels
  if (filters.value.citationsFrom) params.citationsFrom = filters.value.citationsFrom
  if (filters.value.citationsTo) params.citationsTo = filters.value.citationsTo
  if (filters.value.journal) params.journal = filters.value.journal
  if (filters.value.paperType) params.paperType = filters.value.paperType
  if (filters.value.language) params.language = filters.value.language

  return params
}

const saveCurrentSearch = () => {
  if (!saveSearchName.value.trim()) return

  const params = buildSearchParams()
  searchStore.saveSearch(saveSearchName.value)

  saveSearchName.value = ''
  loadSavedSearches()
}

const loadSavedSearches = () => {
  savedSearches.value = searchStore.savedSearches
}

const loadSavedSearch = (id: string) => {
  const saved = searchStore.loadSavedSearch(id)
  if (saved) {
    // Apply the saved search parameters
    // This would need to parse the saved params and populate the form
  }
}

const deleteSavedSearch = (id: string) => {
  if (confirm('Are you sure you want to delete this saved search?')) {
    searchStore.deleteSavedSearch(id)
    loadSavedSearches()
  }
}

const getSavedSearchDescription = (saved: any) => {
  // Generate a description from the saved search parameters
  return 'Advanced search query'
}

const formatDate = (timestamp: number) => {
  const date = new Date(timestamp)
  return date.toLocaleDateString()
}

const resetForm = () => {
  searchQuery.value = {
    booleanOperator: 'AND',
    fields: [{ field: '', value: '' }]
  }

  filters.value = {
    ccfLevels: [],
    paperType: '',
    language: ''
  }

  saveSearchName.value = ''
}

const copyQuery = async () => {
  try {
    await navigator.clipboard.writeText(searchQueryPreview.value)
    // Show success notification
  } catch (error) {
    console.error('Failed to copy query:', error)
  }
}

// Lifecycle
onMounted(() => {
  loadSavedSearches()
})
</script>

<style scoped lang="scss">
.advanced-search-view {
  width: 100%;
}

.search-form {
  display: flex;
  flex-direction: column;
  gap: 2rem;
}

.form-section {
  background: #f8fafc;
  padding: 1.5rem;
  border-radius: 0.75rem;
}

.section-title {
  margin: 0 0 1.5rem 0;
  font-size: 1.25rem;
  font-weight: 600;
  color: #1e293b;
}

.boolean-operators {
  margin-bottom: 1.5rem;
}

.operator-label {
  font-size: 0.875rem;
  font-weight: 600;
  color: #475569;
  margin-bottom: 0.75rem;
}

.operator-buttons {
  display: flex;
  gap: 0.5rem;
}

.operator-btn {
  padding: 0.5rem 1rem;
  background: white;
  border: 2px solid #e2e8f0;
  border-radius: 0.5rem;
  font-size: 0.875rem;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s;
}

.operator-btn:hover {
  border-color: #667eea;
}

.operator-btn.active {
  background: #667eea;
  color: white;
  border-color: #667eea;
}

.search-fields {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.search-field-row {
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}

.field-controls {
  display: flex;
  gap: 0.5rem;
}

.field-select {
  min-width: 150px;
  padding: 0.5rem;
  border: 1px solid #e2e8f0;
  border-radius: 0.5rem;
  font-size: 0.875rem;
  background: white;
}

.field-input {
  flex: 1;
  padding: 0.5rem;
  border: 1px solid #e2e8f0;
  border-radius: 0.5rem;
  font-size: 0.875rem;
}

.remove-field-btn {
  padding: 0.5rem;
  background: #ef4444;
  color: white;
  border: none;
  border-radius: 0.5rem;
  cursor: pointer;
  transition: all 0.2s;
}

.remove-field-btn:hover {
  background: #dc2626;
}

.field-operator {
  font-size: 0.875rem;
  font-weight: 600;
  color: #667eea;
  text-align: center;
}

.add-field-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 0.5rem;
  width: 100%;
  padding: 0.75rem;
  background: white;
  border: 2px dashed #e2e8f0;
  border-radius: 0.5rem;
  font-size: 0.875rem;
  color: #64748b;
  cursor: pointer;
  transition: all 0.2s;
}

.add-field-btn:hover {
  border-color: #667eea;
  color: #667eea;
}

.search-templates {
  margin-top: 1.5rem;
}

.template-label {
  display: block;
  font-size: 0.875rem;
  font-weight: 600;
  color: #475569;
  margin-bottom: 0.75rem;
}

.template-buttons {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem;
}

.template-btn {
  padding: 0.5rem 1rem;
  background: white;
  border: 1px solid #e2e8f0;
  border-radius: 2rem;
  font-size: 0.875rem;
  cursor: pointer;
  transition: all 0.2s;
}

.template-btn:hover {
  background: #667eea;
  color: white;
  border-color: #667eea;
}

.filter-group {
  margin-bottom: 1.5rem;
}

.filter-label {
  display: block;
  font-size: 0.875rem;
  font-weight: 600;
  color: #475569;
  margin-bottom: 0.75rem;
}

.year-range {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1rem;
  margin-bottom: 1rem;
}

.year-input-group {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.year-label {
  font-size: 0.875rem;
  color: #64748b;
  white-space: nowrap;
}

.year-input {
  flex: 1;
  padding: 0.5rem;
  border: 1px solid #e2e8f0;
  border-radius: 0.5rem;
  font-size: 0.875rem;
}

.year-presets {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem;
}

.year-preset-btn {
  padding: 0.375rem 0.75rem;
  background: white;
  border: 1px solid #e2e8f0;
  border-radius: 0.375rem;
  font-size: 0.75rem;
  cursor: pointer;
  transition: all 0.2s;
}

.year-preset-btn:hover {
  background: #667eea;
  color: white;
  border-color: #667eea;
}

.ccf-levels {
  display: flex;
  flex-wrap: wrap;
  gap: 1rem;
}

.ccf-level-checkbox {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  cursor: pointer;
}

.checkbox-input {
  width: 1rem;
  height: 1rem;
  cursor: pointer;
}

.checkbox-label {
  font-size: 0.875rem;
  color: #475569;
}

.citation-range {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1rem;
}

.range-input-group {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.range-label {
  font-size: 0.875rem;
  color: #64748b;
  white-space: nowrap;
}

.range-input {
  flex: 1;
  padding: 0.5rem;
  border: 1px solid #e2e8f0;
  border-radius: 0.5rem;
  font-size: 0.875rem;
}

.journal-input {
  width: 100%;
  padding: 0.5rem;
  border: 1px solid #e2e8f0;
  border-radius: 0.5rem;
  font-size: 0.875rem;
  margin-bottom: 0.75rem;
}

.journal-suggestions {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem;
}

.journal-suggestion-btn {
  padding: 0.375rem 0.75rem;
  background: white;
  border: 1px solid #e2e8f0;
  border-radius: 2rem;
  font-size: 0.75rem;
  cursor: pointer;
  transition: all 0.2s;
}

.journal-suggestion-btn:hover {
  background: #667eea;
  color: white;
  border-color: #667eea;
}

.paper-types {
  display: flex;
  flex-wrap: wrap;
  gap: 1rem;
}

.paper-type-radio {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  cursor: pointer;
}

.radio-input {
  width: 1rem;
  height: 1rem;
  cursor: pointer;
}

.radio-label {
  font-size: 0.875rem;
  color: #475569;
}

.language-select {
  width: 100%;
  padding: 0.5rem;
  border: 1px solid #e2e8f0;
  border-radius: 0.5rem;
  font-size: 0.875rem;
  background: white;
}

.saved-searches {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.saved-search-item {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  padding: 1rem;
  background: white;
  border-radius: 0.5rem;
  border: 1px solid #e2e8f0;
}

.saved-search-info {
  flex: 1;
}

.saved-search-name {
  margin: 0 0 0.5rem 0;
  font-size: 1rem;
  font-weight: 600;
  color: #1e293b;
}

.saved-search-details {
  margin: 0 0 0.5rem 0;
  font-size: 0.875rem;
  color: #64748b;
}

.saved-search-date {
  margin: 0;
  font-size: 0.75rem;
  color: #94a3b8;
}

.saved-search-actions {
  display: flex;
  gap: 0.5rem;
}

.load-search-btn,
.delete-search-btn {
  padding: 0.5rem;
  background: white;
  border: 1px solid #e2e8f0;
  border-radius: 0.5rem;
  cursor: pointer;
  transition: all 0.2s;
}

.load-search-btn:hover {
  background: #667eea;
  color: white;
  border-color: #667eea;
}

.delete-search-btn:hover {
  background: #ef4444;
  color: white;
  border-color: #ef4444;
}

.form-actions {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.save-search {
  display: flex;
  gap: 0.5rem;
}

.save-search-input {
  flex: 1;
  padding: 0.5rem;
  border: 1px solid #e2e8f0;
  border-radius: 0.5rem;
  font-size: 0.875rem;
}

.save-search-btn {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.5rem 1rem;
  background: #667eea;
  color: white;
  border: none;
  border-radius: 0.5rem;
  font-size: 0.875rem;
  cursor: pointer;
  transition: all 0.2s;
}

.save-search-btn:hover:not(:disabled) {
  background: #5a67d8;
}

.save-search-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.action-buttons {
  display: flex;
  gap: 0.75rem;
  justify-content: flex-end;
}

.cancel-btn,
.reset-btn {
  padding: 0.75rem 1.5rem;
  background: white;
  border: 1px solid #e2e8f0;
  border-radius: 0.5rem;
  font-size: 0.875rem;
  cursor: pointer;
  transition: all 0.2s;
}

.cancel-btn:hover,
.reset-btn:hover {
  background: #f1f5f9;
}

.search-btn {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.75rem 1.5rem;
  background: #667eea;
  color: white;
  border: none;
  border-radius: 0.5rem;
  font-size: 0.875rem;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s;
}

.search-btn:hover:not(:disabled) {
  background: #5a67d8;
}

.search-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.search-preview {
  background: #1e293b;
  color: white;
  padding: 1rem;
  border-radius: 0.5rem;
  margin-top: 1rem;
}

.preview-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 0.75rem;
}

.preview-header h4 {
  margin: 0;
  font-size: 1rem;
  font-weight: 600;
}

.copy-query-btn {
  background: transparent;
  border: none;
  color: #94a3b8;
  cursor: pointer;
  transition: color 0.2s;
}

.copy-query-btn:hover {
  color: white;
}

.preview-content {
  background: #0f172a;
  padding: 1rem;
  border-radius: 0.375rem;
  overflow-x: auto;
}

.preview-content code {
  font-family: 'Courier New', monospace;
  font-size: 0.875rem;
  color: #e2e8f0;
  white-space: pre-wrap;
  word-break: break-all;
}

.slide-enter-active,
.slide-leave-active {
  transition: all 0.3s ease;
}

.slide-enter-from,
.slide-leave-to {
  opacity: 0;
  transform: translateY(-10px);
}

@media (max-width: 768px) {
  .year-range,
  .citation-range {
    grid-template-columns: 1fr;
  }

  .action-buttons {
    flex-direction: column;
  }

  .action-buttons button {
    width: 100%;
  }

  .save-search {
    flex-direction: column;
  }

  .field-controls {
    flex-direction: column;
  }

  .field-select {
    min-width: 100%;
  }
}
</style>