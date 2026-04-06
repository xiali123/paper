<template>
  <div class="export-page">
    <!-- Header Section -->
    <div class="page-header">
      <div class="header-content">
        <div class="header-title-group">
          <span class="header-icon">📤</span>
          <h1 class="page-title">导出管理</h1>
        </div>
        <p class="page-description">导出论文引用格式，支持BibTeX、CSV等格式</p>
      </div>
    </div>

    <!-- Export Options -->
    <div class="export-options">
      <div class="option-card">
        <div class="option-header">
          <span class="option-icon">📋</span>
          <h3 class="option-title">选择导出格式</h3>
        </div>
        <div class="format-buttons">
          <button
            v-for="format in exportFormats"
            :key="format.key"
            @click="exportFormat = format.key"
            :class="['format-btn', { active: exportFormat === format.key }]"
          >
            <span class="format-icon">{{ format.icon }}</span>
            <span class="format-name">{{ format.name }}</span>
            <span class="format-desc">{{ format.desc }}</span>
          </button>
        </div>
      </div>

      <div class="option-card">
        <div class="option-header">
          <span class="option-icon">⚙️</span>
          <h3 class="option-title">导出选项</h3>
        </div>
        <div class="export-settings">
          <label class="setting-label">
            <input type="checkbox" v-model="includeAbstract" class="setting-checkbox">
            包含摘要
          </label>
          <label class="setting-label">
            <input type="checkbox" v-model="includeKeywords" class="setting-checkbox">
            包含关键词
          </label>
          <label class="setting-label">
            <input type="checkbox" v-model="includeFullText" class="setting-checkbox">
            包含全文链接
          </label>
        </div>
      </div>
    </div>

    <!-- Papers Selection -->
    <div class="papers-section">
      <div class="section-header">
        <h2 class="section-title">选择论文</h2>
        <div class="section-actions">
          <button @click="selectAll" class="action-btn btn-sm btn-secondary">
            {{ allSelected ? '取消全选' : '全选' }}
          </button>
          <button
            @click="exportSelected"
            :disabled="selectedPapers.length === 0 || isExporting"
            class="action-btn btn-sm btn-primary"
          >
            {{ isExporting ? '导出中...' : `导出 (${selectedPapers.length})` }}
          </button>
        </div>
      </div>

      <!-- Papers List -->
      <div v-if="loading" class="loading-state">
        <LoadingSpinner size="large" variant="primary" text="加载论文列表..." />
      </div>

      <div v-else-if="papers.length === 0" class="empty-state">
        <EmptyState
          icon="📤"
          title="暂无论文可导出"
          description="请先添加论文到数据库"
        />
      </div>

      <div v-else class="papers-grid">
        <div
          v-for="paper in paginatedPapers"
          :key="paper.id"
          :class="['paper-card', { selected: selectedPapers.includes(paper.id) }]"
          @click="toggleSelection(paper.id)"
        >
          <div class="paper-checkbox">
            <input
              type="checkbox"
              :checked="selectedPapers.includes(paper.id)"
              @click.stop="toggleSelection(paper.id)"
            >
          </div>
          <div class="paper-content">
            <h3 class="paper-title">{{ paper.title }}</h3>
            <div class="paper-meta">
              <span class="paper-authors">{{ formatAuthors(paper.authors) }}</span>
              <span class="paper-year">{{ paper.year }}</span>
            </div>
            <div class="paper-tags">
              <span v-if="paper.journal" class="tag tag-journal">{{ paper.journal }}</span>
              <span v-if="paper.venue" class="tag tag-venue">{{ paper.venue }}</span>
            </div>
          </div>
        </div>
      </div>

      <!-- Pagination -->
      <div v-if="totalPages > 1" class="pagination">
        <button
          @click="currentPage--"
          :disabled="currentPage === 1"
          class="pagination-btn"
        >
          上一页
        </button>
        <span class="pagination-info">{{ currentPage }} / {{ totalPages }}</span>
        <button
          @click="currentPage++"
          :disabled="currentPage === totalPages"
          class="pagination-btn"
        >
          下一页
        </button>
      </div>
    </div>

    <!-- Export Preview Modal -->
    <Transition name="modal">
      <div v-if="showPreviewModal" class="modal-overlay" @click="showPreviewModal = false">
        <div class="modal-content large" @click.stop>
          <div class="modal-header">
            <h2 class="modal-title">导出预览</h2>
            <button @click="showPreviewModal = false" class="close-button">✕</button>
          </div>
          <div class="modal-body">
            <pre class="preview-content">{{ exportPreview }}</pre>
          </div>
          <div class="modal-footer">
            <button @click="showPreviewModal = false" class="btn btn-secondary">关闭</button>
            <button @click="downloadExport" class="btn btn-primary">下载文件</button>
          </div>
        </div>
      </div>
    </Transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import LoadingSpinner from '@/components/common/LoadingSpinner.vue'
import EmptyState from '@/components/common/EmptyState.vue'

// State
const papers = ref<any[]>([])
const loading = ref(false)
const selectedPapers = ref<number[]>([])
const exportFormat = ref('bibtex')
const isExporting = ref(false)
const showPreviewModal = ref(false)
const exportPreview = ref('')

// Export options
const includeAbstract = ref(true)
const includeKeywords = ref(true)
const includeFullText = ref(false)

// Pagination
const currentPage = ref(1)
const pageSize = ref(12)

// Export formats
const exportFormats = [
  { key: 'bibtex', name: 'BibTeX', icon: '📚', desc: 'LaTeX引用格式' },
  { key: 'csv', name: 'CSV', icon: '📊', desc: 'Excel表格格式' },
  { key: 'endnote', name: 'EndNote', icon: '📝', desc: 'EndNote格式' },
  { key: 'ris', name: 'RIS', icon: '📄', desc: 'Reference Manager格式' }
]

// Computed
const allSelected = computed(() => {
  return papers.value.length > 0 && selectedPapers.value.length === papers.value.length
})

const totalPages = computed(() => Math.ceil(papers.value.length / pageSize.value))

const paginatedPapers = computed(() => {
  const start = (currentPage.value - 1) * pageSize.value
  const end = start + pageSize.value
  return papers.value.slice(start, end)
})

// Methods
const fetchPapers = async () => {
  loading.value = true
  try {
    const response = await fetch('/api/papers')
    const result = await response.json()

    if (result.papers) {
      papers.value = result.papers
    } else {
      ElMessage.error('获取论文列表失败')
    }
  } catch (error: any) {
    ElMessage.error('获取论文列表失败：' + error.message)
  } finally {
    loading.value = false
  }
}

const toggleSelection = (id: number) => {
  const index = selectedPapers.value.indexOf(id)
  if (index > -1) {
    selectedPapers.value.splice(index, 1)
  } else {
    selectedPapers.value.push(id)
  }
}

const selectAll = () => {
  if (allSelected.value) {
    selectedPapers.value = []
  } else {
    selectedPapers.value = papers.value.map(p => p.id)
  }
}

const exportSelected = async () => {
  if (selectedPapers.value.length === 0) {
    alert('请先选择要导出的论文')
    return
  }

  isExporting.value = true
  try {
    if (exportFormat.value === 'bibtex') {
      // 调用批量BibTeX导出API
      const response = await fetch('http://localhost:8080/api/export/bibtex/batch', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          ids: selectedPapers.value,
          includeAbstract: includeAbstract.value,
          includeKeywords: includeKeywords.value
        })
      })
      const result = await response.json()

      if (result.success) {
        exportPreview.value = result.data.bibtex || result.data
        showPreviewModal.value = true
      } else {
        alert('导出失败：' + result.error)
      }
    } else if (exportFormat.value === 'csv') {
      // 调用CSV导出API
      const response = await fetch(`http://localhost:8080/api/export/csv?ids=${selectedPapers.value.join(',')}`)
      const result = await response.json()

      if (result.success) {
        exportPreview.value = result.data.csv
        showPreviewModal.value = true
      } else {
        alert('导出失败：' + result.error)
      }
    } else {
      // 其他格式暂未实现
      alert('该格式暂未实现，请使用BibTeX或CSV')
    }
  } catch (error: any) {
    alert('导出失败：' + error.message)
  } finally {
    isExporting.value = false
  }
}

const downloadExport = () => {
  const formatNames: Record<string, string> = {
    bibtex: 'bib',
    csv: 'csv',
    endnote: 'enw',
    ris: 'ris'
  }

  const blob = new Blob([exportPreview.value], {
    type: 'text/plain;charset=utf-8'
  })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `export_${selectedPapers.value.length}_papers.${formatNames[exportFormat.value]}`
  a.click()
  URL.revokeObjectURL(url)

  showPreviewModal.value = false
}

const formatAuthors = (authors: string) => {
  if (!authors) return 'Unknown'
  const authorList = authors.split(',')
  if (authorList.length > 2) {
    return authorList[0] + ' et al.'
  }
  return authors
}

// Lifecycle
onMounted(() => {
  fetchPapers()
})
</script>

<style scoped>
.export-page {
  width: 100%;
  max-width: 1920px;
  margin: 0 auto;
  padding: 8px;
}

/* Header */
.page-header {
  margin-bottom: 16px;
}

.header-content {
  text-align: center;
}

.header-title-group {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
  margin-bottom: 4px;
}

.header-icon {
  font-size: 36px;
}

.page-title {
  font-size: 28px;
  font-weight: 800;
  background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  margin: 0;
}

.page-description {
  font-size: 14px;
  color: #6b7280;
  margin: 0;
}

/* Export Options */
.export-options {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(360px, 1fr));
  gap: 14px;
  margin-bottom: 16px;
}

.option-card {
  background: white;
  border-radius: 12px;
  padding: 14px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  border: 1px solid #e5e7eb;
}

.option-header {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 12px;
}

.option-icon {
  font-size: 22px;
}

.option-title {
  font-size: 17px;
  font-weight: 700;
  color: #1f2937;
  margin: 0;
}

.format-buttons {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
  gap: 8px;
}

.format-btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
  padding: 10px 8px;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  background: white;
  cursor: pointer;
  transition: all 0.3s;
}

.format-btn:hover {
  border-color: #f093fb;
  box-shadow: 0 4px 12px rgba(240, 147, 251, 0.2);
}

.format-btn.active {
  border-color: #f093fb;
  background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
  color: white;
}

.format-btn.active .format-desc {
  color: rgba(255, 255, 255, 0.95);
}

.format-icon {
  font-size: 22px;
}

.format-name {
  font-weight: 600;
  font-size: 14px;
}

.format-desc {
  font-size: 11px;
  color: #6b7280;
}

.export-settings {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.setting-label {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px;
  border-radius: 6px;
  cursor: pointer;
  transition: background 0.2s;
  font-size: 14px;
}

.setting-label:hover {
  background: #f9fafb;
}

.setting-checkbox {
  width: 14px;
  height: 14px;
  cursor: pointer;
}

/* Papers Section */
.papers-section {
  background: white;
  border-radius: 12px;
  padding: 14px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 14px;
}

.section-title {
  font-size: 19px;
  font-weight: 700;
  color: #1f2937;
  margin: 0;
}

.section-actions {
  display: flex;
  gap: 8px;
}

.action-btn {
  padding: 7px 14px;
  border: none;
  border-radius: 7px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
}

.btn-sm {
  padding: 6px 12px;
  font-size: 13px;
}

.btn-primary {
  background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
  color: white;
}

.btn-primary:hover:not(:disabled) {
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(240, 147, 251, 0.4);
  opacity: 0.95;
}

.btn-primary:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.btn-secondary {
  background: #f3f4f6;
  color: #374151;
}

.btn-secondary:hover {
  background: #e5e7eb;
}

/* Papers Grid */
.papers-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
  gap: 8px;
}

.paper-card {
  display: flex;
  gap: 8px;
  padding: 10px;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  background: white;
  cursor: pointer;
  transition: all 0.3s;
}

.paper-card:hover {
  border-color: #f093fb;
  box-shadow: 0 4px 12px rgba(240, 147, 251, 0.15);
}

.paper-card.selected {
  border-color: #f093fb;
  background: linear-gradient(135deg, rgba(240, 147, 251, 0.05) 0%, rgba(245, 87, 108, 0.05) 100%);
}

.paper-checkbox input {
  width: 14px;
  height: 14px;
  cursor: pointer;
  margin-top: 1px;
}

.paper-content {
  flex: 1;
}

.paper-title {
  font-size: 14px;
  font-weight: 600;
  color: #1f2937;
  margin: 0 0 4px 0;
  line-height: 1.2;
}

.paper-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 5px;
  margin-bottom: 4px;
  font-size: 12px;
  color: #6b7280;
}

.paper-authors {
  max-width: 160px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.paper-year {
  color: #9ca3af;
}

.paper-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
}

.tag {
  padding: 2px 6px;
  border-radius: 4px;
  font-size: 11px;
  font-weight: 500;
}

.tag-journal {
  background: #dbeafe;
  color: #0369a1;
}

.tag-venue {
  background: #fce7f3;
  color: #9d174d;
}

/* Pagination */
.pagination {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 12px;
  margin-top: 14px;
}

.pagination-btn {
  padding: 5px 12px;
  border: 1px solid #e5e7eb;
  border-radius: 5px;
  background: white;
  cursor: pointer;
  transition: all 0.2s;
  font-size: 13px;
}

.pagination-btn:hover:not(:disabled) {
  background: #f9fafb;
}

.pagination-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.pagination-info {
  font-size: 13px;
  color: #6b7280;
  font-weight: 500;
}

/* Modal */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
}

.modal-content {
  background: white;
  border-radius: 12px;
  width: 90%;
  max-width: 900px;
  max-height: 85vh;
  display: flex;
  flex-direction: column;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
}

.modal-content.large {
  max-width: 1100px;
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 18px;
  border-bottom: 1px solid #e5e7eb;
}

.modal-title {
  font-size: 20px;
  font-weight: 700;
  color: #1f2937;
  margin: 0;
}

.close-button {
  width: 28px;
  height: 28px;
  border-radius: 50%;
  border: none;
  background: #f3f4f6;
  font-size: 18px;
  cursor: pointer;
  transition: all 0.3s;
}

.close-button:hover {
  background: #e5e7eb;
  transform: rotate(90deg);
}

.modal-body {
  flex: 1;
  overflow-y: auto;
  padding: 16px 18px;
}

.preview-content {
  background: #1f2937;
  color: #f9fafb;
  border-radius: 8px;
  padding: 14px;
  font-size: 12px;
  line-height: 1.4;
  max-height: 400px;
  overflow: auto;
}

.modal-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  padding: 16px 18px;
  border-top: 1px solid #e5e7eb;
}

/* Modal Transitions */
.modal-enter-active,
.modal-leave-active {
  transition: all 0.3s;
}

.modal-enter-from,
.modal-leave-to {
  opacity: 0;
}

.modal-enter-from .modal-content,
.modal-leave-to .modal-content {
  transform: scale(0.9);
}

/* Button */
.btn {
  padding: 7px 14px;
  border: none;
  border-radius: 7px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
}

/* Empty State */
.empty-state {
  padding: 32px 16px;
  text-align: center;
}

/* Loading State */
.loading-state {
  padding: 32px 16px;
  text-align: center;
}
</style>
