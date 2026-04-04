/**
 * Export Store (Pinia)
 *
 * Manages paper export functionality including:
 * - Export history
 * - Export settings and preferences
 * - Export operations (CSV, JSON, BibTeX, etc.)
 * - Export templates
 *
 * @module stores/exportStore
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { Paper } from '@/api/modules/papers'

export type ExportFormat = 'csv' | 'json' | 'bibtex' | 'endnote' | 'xml'

export interface ExportOptions {
  format: ExportFormat
  includeAbstract: boolean
  includeNotes: boolean
  includeTags: boolean
  includePdfLinks: boolean
  sortBy?: string
  sortOrder?: 'asc' | 'desc'
}

export interface ExportRecord {
  id: string
  format: ExportFormat
  paperCount: number
  fileName: string
  fileSize: number
  createdAt: number
  options: ExportOptions
  downloadUrl?: string
}

export interface ExportTemplate {
  id: string
  name: string
  format: ExportFormat
  options: ExportOptions
  createdAt: number
  lastUsed: number
  usageCount: number
}

export const useExportStore = defineStore(
  'export',
  () => {
    // ========================================================================
    // State
    // ========================================================================

    /** Export history */
    const history = ref<ExportRecord[]>([])

    /** Export templates */
    const templates = ref<ExportTemplate[]>([])

    /** Current export options */
    const options = ref<ExportOptions>({
      format: 'csv',
      includeAbstract: true,
      includeNotes: true,
      includeTags: true,
      includePdfLinks: false,
      sortBy: 'created_at',
      sortOrder: 'desc'
    })

    /** Loading state */
    const loading = ref(false)

    /** Export progress */
    const progress = ref(0)

    /** Error message */
    const error = ref<string | null>(null)

    /** Selected paper IDs for export */
    const selectedPaperIds = ref<number[]>([])

    /** Export all papers flag */
    const exportAll = ref(false)

    /** Maximum history items */
    const maxHistoryItems = 50

    // ========================================================================
    // Computed Properties
    // ========================================================================

    /** Recent exports (last 10) */
    const recentExports = computed(() =>
      history.value.slice(0, 10)
    )

    /** Exports by format */
    const exportsByFormat = computed(() => {
      const byFormat: Record<ExportFormat, number> = {
        csv: 0,
        json: 0,
        bibtex: 0,
        endnote: 0,
        xml: 0
      }

      history.value.forEach(record => {
        byFormat[record.format]++
      })

      return byFormat
    })

    /** Total papers exported */
    const totalPapersExported = computed(() =>
      history.value.reduce((sum, record) => sum + record.paperCount, 0)
    )

    /** Most used format */
    const mostUsedFormat = computed(() => {
      const entries = Object.entries(exportsByFormat.value)
      const sorted = entries.sort((a, b) => b[1] - a[1])
      return sorted[0] ? (sorted[0][0] as ExportFormat) : 'csv'
    })

    /** Popular templates */
    const popularTemplates = computed(() =>
      [...templates.value]
        .sort((a, b) => b.usageCount - a.usageCount)
        .slice(0, 5)
    )

    /** Has selection */
    const hasSelection = computed(() =>
      selectedPaperIds.value.length > 0 || exportAll.value
    )

    /** Selection count */
    const selectionCount = computed(() =>
      exportAll.value ? -1 : selectedPaperIds.value.length // -1 means all
    )

    // ========================================================================
    // Actions
    // ========================================================================

    /**
     * Export papers
     */
    async function exportPapers(papers: Paper[], exportOptions?: Partial<ExportOptions>) {
      loading.value = true
      error.value = null
      progress.value = 0

      try {
        const opts = { ...options.value, ...exportOptions }

        // Simulate export progress
        const progressInterval = setInterval(() => {
          if (progress.value < 90) {
            progress.value += 10
          }
        }, 200)

        // TODO: Implement actual export API call
        // For now, simulate export
        await new Promise(resolve => setTimeout(resolve, 2000))

        clearInterval(progressInterval)
        progress.value = 100

        // Create export record
        const record: ExportRecord = {
          id: `export_${Date.now()}`,
          format: opts.format,
          paperCount: papers.length,
          fileName: `papers_export_${Date.now()}.${opts.format}`,
          fileSize: estimateFileSize(papers.length, opts.format),
          createdAt: Date.now(),
          options: opts
        }

        // Add to history
        addToHistory(record)

        return record
      } catch (err: any) {
        error.value = err.message || 'Export failed'
        throw err
      } finally {
        loading.value = false
        progress.value = 0
      }
    }

    /**
     * Export selected papers
     */
    async function exportSelected(papers: Paper[], exportOptions?: Partial<ExportOptions>) {
      const selectedPapers = papers.filter(p =>
        exportAll.value || selectedPaperIds.value.includes(p.id)
      )

      if (selectedPapers.length === 0) {
        throw new Error('No papers selected for export')
      }

      return await exportPapers(selectedPapers, exportOptions)
    }

    /**
     * Export to CSV
     */
    async function exportToCSV(papers: Paper[]) {
      return await exportPapers(papers, { format: 'csv' })
    }

    /**
     * Export to JSON
     */
    async function exportToJSON(papers: Paper[]) {
      return await exportPapers(papers, { format: 'json' })
    }

    /**
     * Export to BibTeX
     */
    async function exportToBibTeX(papers: Paper[]) {
      return await exportPapers(papers, { format: 'bibtex' })
    }

    /**
     * Export to EndNote
     */
    async function exportToEndNote(papers: Paper[]) {
      return await exportPapers(papers, { format: 'endnote' })
    }

    /**
     * Export to XML
     */
    async function exportToXML(papers: Paper[]) {
      return await exportPapers(papers, { format: 'xml' })
    }

    /**
     * Add to history
     */
    function addToHistory(record: ExportRecord) {
      history.value.unshift(record)

      // Limit history size
      if (history.value.length > maxHistoryItems) {
        history.value = history.value.slice(0, maxHistoryItems)
      }

      persistHistory()
    }

    /**
     * Remove from history
     */
    function removeFromHistory(id: string) {
      history.value = history.value.filter(r => r.id !== id)
      persistHistory()
    }

    /**
     * Clear history
     */
    function clearHistory() {
      history.value = []
      persistHistory()
    }

    /**
     * Create template
     */
    function createTemplate(
      name: string,
      format: ExportFormat,
      templateOptions: Partial<ExportOptions>
    ) {
      const template: ExportTemplate = {
        id: `export_template_${Date.now()}`,
        name,
        format,
        options: { ...options.value, ...templateOptions },
        createdAt: Date.now(),
        lastUsed: 0,
        usageCount: 0
      }

      templates.value.push(template)
      persistTemplates()

      return template
    }

    /**
     * Use template
     */
    function useTemplate(id: string) {
      const template = templates.value.find(t => t.id === id)
      if (template) {
        options.value = { ...template.options }
        template.lastUsed = Date.now()
        template.usageCount++
        persistTemplates()
        return template
      }
      return null
    }

    /**
     * Update template
     */
    function updateTemplate(id: string, updates: Partial<ExportTemplate>) {
      const index = templates.value.findIndex(t => t.id === id)
      if (index !== -1) {
        templates.value[index] = {
          ...templates.value[index],
          ...updates
        }
        persistTemplates()
      }
    }

    /**
     * Delete template
     */
    function deleteTemplate(id: string) {
      templates.value = templates.value.filter(t => t.id !== id)
      persistTemplates()
    }

    /**
     * Set export options
     */
    function setOptions(newOptions: Partial<ExportOptions>) {
      options.value = { ...options.value, ...newOptions }
    }

    /**
     * Reset export options to defaults
     */
    function resetOptions() {
      options.value = {
        format: 'csv',
        includeAbstract: true,
        includeNotes: true,
        includeTags: true,
        includePdfLinks: false,
        sortBy: 'created_at',
        sortOrder: 'desc'
      }
    }

    /**
     * Select papers for export
     */
    function selectPapers(ids: number[]) {
      selectedPaperIds.value = ids
      exportAll.value = false
    }

    /**
     * Select all papers
     */
    function selectAll() {
      exportAll.value = true
      selectedPaperIds.value = []
    }

    /**
     * Clear selection
     */
    function clearSelection() {
      selectedPaperIds.value = []
      exportAll.value = false
    }

    /**
     * Estimate file size
     */
    function estimateFileSize(paperCount: number, format: ExportFormat): number {
      // Average size per paper in bytes (rough estimates)
      const sizes = {
        csv: 500,    // ~500 bytes per paper
        json: 800,   // ~800 bytes per paper
        bibtex: 600, // ~600 bytes per paper
        endnote: 700, // ~700 bytes per paper
        xml: 1000    // ~1000 bytes per paper
      }

      return paperCount * sizes[format]
    }

    /**
     * Format file size
     */
    function formatFileSize(bytes: number): string {
      const units = ['B', 'KB', 'MB', 'GB']
      let size = bytes
      let unitIndex = 0

      while (size >= 1024 && unitIndex < units.length - 1) {
        size /= 1024
        unitIndex++
      }

      return `${size.toFixed(2)} ${units[unitIndex]}`
    }

    /**
     * Reset state
     */
    function reset() {
      history.value = []
      templates.value = []
      loading.value = false
      progress.value = 0
      error.value = null
      selectedPaperIds.value = []
      exportAll.value = false
      resetOptions()
    }

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * Persist history to localStorage
     */
    function persistHistory() {
      try {
        localStorage.setItem('export-history', JSON.stringify(history.value))
      } catch (err) {
        console.error('Failed to persist export history:', err)
      }
    }

    /**
     * Load history from localStorage
     */
    function loadHistory() {
      try {
        const stored = localStorage.getItem('export-history')
        if (stored) {
          history.value = JSON.parse(stored)
        }
      } catch (err) {
        console.error('Failed to load export history:', err)
      }
    }

    /**
     * Persist templates to localStorage
     */
    function persistTemplates() {
      try {
        localStorage.setItem('export-templates', JSON.stringify(templates.value))
      } catch (err) {
        console.error('Failed to persist export templates:', err)
      }
    }

    /**
     * Load templates from localStorage
     */
    function loadTemplates() {
      try {
        const stored = localStorage.getItem('export-templates')
        if (stored) {
          templates.value = JSON.parse(stored)
        }
      } catch (err) {
        console.error('Failed to load export templates:', err)
      }
    }

    // Initialize on store creation
    loadHistory()
    loadTemplates()

    // ========================================================================
    // Return
    // ========================================================================

    return {
      // State
      history,
      templates,
      options,
      loading,
      progress,
      error,
      selectedPaperIds,
      exportAll,

      // Computed
      recentExports,
      exportsByFormat,
      totalPapersExported,
      mostUsedFormat,
      popularTemplates,
      hasSelection,
      selectionCount,

      // Actions
      exportPapers,
      exportSelected,
      exportToCSV,
      exportToJSON,
      exportToBibTeX,
      exportToEndNote,
      exportToXML,
      addToHistory,
      removeFromHistory,
      clearHistory,
      createTemplate,
      useTemplate,
      updateTemplate,
      deleteTemplate,
      setOptions,
      resetOptions,
      selectPapers,
      selectAll,
      clearSelection,
      formatFileSize,
      reset
    }
  },
  {
    persist: {
      key: 'export-store',
      storage: localStorage,
      paths: ['options']
    }
  }
)
