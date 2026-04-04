<template>
  <div class="paper-list-view">
    <!-- Page Header -->
    <div class="page-header">
      <div class="page-header__content">
        <h1 class="page-header__title">{{ $t('papers.title') || '论文管理' }}</h1>
        <p class="page-header__subtitle">{{ $t('papers.subtitle') || '管理和浏览您的论文收藏' }}</p>
      </div>
      <div class="page-header__actions">
        <el-button type="primary" :icon="Plus" @click="handleCreate">
          {{ $t('papers.addPaper') || '添加论文' }}
        </el-button>
      </div>
    </div>

    <!-- Toolbar -->
    <div class="toolbar">
      <div class="toolbar__left">
        <!-- Search -->
        <el-input
          v-model="searchKeyword"
          :placeholder="$t('papers.searchPlaceholder') || '搜索论文...'"
          :prefix-icon="Search"
          clearable
          class="search-input"
          @clear="handleSearchClear"
          @keyup.enter="handleSearch"
        />

        <!-- View Toggle -->
        <el-button-group class="view-toggle">
          <el-button
            :type="viewMode === 'grid' ? 'primary' : 'default'"
            :icon="Grid"
            @click="viewMode = 'grid'"
          />
          <el-button
            :type="viewMode === 'list' ? 'primary' : 'default'"
            :icon="List"
            @click="viewMode = 'list'"
          />
        </el-button-group>

        <!-- Filter Toggle -->
        <el-button
          :type="hasActiveFilters ? 'primary' : 'default'"
          :icon="Filter"
          @click="filterPanelVisible = !filterPanelVisible"
        >
          {{ $t('papers.filters') || '筛选' }}
          <el-badge v-if="activeFilterCount > 0" :value="activeFilterCount" class="filter-badge" />
        </el-button>
      </div>

      <div class="toolbar__right">
        <!-- Sort -->
        <el-select
          v-model="sortField"
          :placeholder="$t('papers.sortBy') || '排序方式'"
          @change="handleSortChange"
        >
          <el-option label="创建时间" value="created_at" />
          <el-option label="更新时间" value="updated_at" />
          <el-option label="标题" value="title" />
          <el-option label="年份" value="year" />
          <el-option label="引用数" value="citations" />
        </el-select>

        <el-button-group>
          <el-button
            :icon="sortOrder === 'asc' ? 'sort-up' : 'sort-down'"
            @click="toggleSortOrder"
          />
        </el-button-group>

        <!-- Batch Actions -->
        <template v-if="selectedPapers.length > 0">
          <el-divider direction="vertical" />
          <span class="selected-count">{{ selectedPapers.length }} selected</span>
          <el-button :icon="Download" @click="handleBatchExport">
            {{ $t('papers.export') || '导出' }}
          </el-button>
          <el-button :icon="Delete" type="danger" @click="handleBatchDelete">
            {{ $t('papers.delete') || '删除' }}
          </el-button>
        </template>
      </div>
    </div>

    <!-- Main Content -->
    <div class="content-container">
      <!-- Filter Panel -->
      <div v-show="filterPanelVisible" class="filter-panel-wrapper">
        <FilterPanel
          :checkbox-filters="filterOptions"
          @change="handleFilterChange"
          @clear="handleFilterClear"
        />
      </div>

      <!-- Papers Display -->
      <div class="papers-container">
        <!-- Grid View -->
        <div v-if="viewMode === 'grid'" class="papers-grid">
          <div v-if="loading && papers.length === 0" class="skeleton-grid">
            <el-skeleton v-for="i in 8" :key="i" animated>
              <template #template>
                <el-skeleton-item variant="rect" style="width: 100%; height: 280px; border-radius: 8px" />
              </template>
            </el-skeleton>
          </div>

          <div v-else-if="papers.length === 0" class="empty-state">
            <el-empty :description="$t('papers.noPapers') || '暂无论文数据'">
              <el-button type="primary" :icon="Plus" @click="handleCreate">
                {{ $t('papers.addFirstPaper') || '添加第一篇论文' }}
              </el-button>
            </el-empty>
          </div>

          <PaperCard
            v-for="paper in papers"
            v-else
            :key="paper.id"
            :paper="paper"
            :selected="selectedIds.includes(paper.id)"
            @select="handleSelectPaper"
            @view="handleViewPaper"
            @edit="handleEditPaper"
            @delete="handleDeletePaper"
            @toggle-bookmark="handleToggleBookmark"
            @toggle-read="handleToggleRead"
          />
        </div>

        <!-- List View -->
        <div v-else class="papers-list">
          <DataTable
            :columns="tableColumns"
            :data="papers"
            :loading="loading"
            :selectable="true"
            :pagination="true"
            :items-per-page="pageSize"
            row-key="id"
            @row-click="handleViewPaper"
            @selection-change="handleSelectionChange"
          >
            <template #cell-title="{ row }">
              <div class="title-cell">
                <el-link :underline="false" @click="handleViewPaper(row)">
                  {{ row.title }}
                </el-link>
                <div v-if="row.authors" class="title-cell-authors">{{ row.authors }}</div>
              </div>
            </template>

            <template #cell-category="{ row }">
              <el-tag v-if="row.category" size="small" type="primary">{{ row.category }}</el-tag>
            </template>

            <template #cell-year="{ row }">
              <span>{{ row.year || '-' }}</span>
            </template>

            <template #cell-source="{ row }">
              <el-tag v-if="row.source" size="small" type="info">
                {{ sourceLabels[row.source] || row.source }}
              </el-tag>
            </template>

            <template #cell-actions="{ row }">
              <el-button-group>
                <el-button :icon="View" size="small" @click="handleViewPaper(row)" />
                <el-button :icon="Edit" size="small" @click="handleEditPaper(row)" />
                <el-button :icon="Delete" size="small" type="danger" @click="handleDeletePaper(row)" />
              </el-button-group>
            </template>
          </DataTable>
        </div>

        <!-- Pagination -->
        <div v-if="totalPages > 1" class="pagination-container">
          <el-pagination
            v-model:current-page="currentPage"
            v-model:page-size="pageSize"
            :page-sizes="[10, 20, 50, 100]"
            :total="total"
            layout="total, sizes, prev, pager, next, jumper"
            @size-change="handlePageSizeChange"
            @current-change="handlePageChange"
          />
        </div>
      </div>
    </div>

    <!-- Delete Confirmation Dialog -->
    <el-dialog
      v-model="deleteDialogVisible"
      :title="$t('papers.deleteConfirmTitle') || '确认删除'"
      width="500px"
    >
      <span>{{ deleteMessage }}</span>
      <template #footer>
        <el-button @click="deleteDialogVisible = false">{{ $t('common.cancel') || '取消' }}</el-button>
        <el-button type="danger" @click="confirmDelete">
          {{ $t('common.confirm') || '确认' }}
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from 'vue'
import { useRouter } from 'vue-router'
import { usePaperStore } from '@/stores/paperStore'
import { storeToRefs } from 'pinia'
import type { Paper } from '@/api/modules/papers'
import PaperCard from '@/components/paper/PaperCard.vue'
import FilterPanel from '@/components/common/FilterPanel.vue'
import DataTable from '@/components/common/DataTable.vue'
import {
  Plus,
  Search,
  Grid,
  List,
  Filter,
  Download,
  Delete,
  View,
  Edit
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'

const router = useRouter()
const paperStore = usePaperStore()

const {
  papers,
  loading,
  total,
  page,
  pageSize,
  totalPages,
  filters,
  selectedIds,
  selectedPapers
} = storeToRefs(paperStore)

// View mode
const viewMode = ref<'grid' | 'list'>('grid')

// Search
const searchKeyword = ref('')

// Filter
const filterPanelVisible = ref(false)
const activeFilterCount = computed(() => {
  let count = 0
  if (filters.value.keyword) count++
  if (filters.value.category) count++
  if (filters.value.source) count++
  if (filters.value.year) count++
  if (filters.value.isBookmarked !== undefined) count++
  if (filters.value.isRead !== undefined) count++
  return count
})

const hasActiveFilters = computed(() => activeFilterCount.value > 0)

// Sort
const sortField = ref<'created_at' | 'updated_at' | 'title' | 'year' | 'citations'>('created_at')
const sortOrder = ref<'asc' | 'desc'>('desc')

// Filter options
const sourceLabels: Record<string, string> = {
  manual: '手动添加',
  cnki: '知网',
  ieee: 'IEEE',
  arxiv: 'ArXiv',
  pubmed: 'PubMed'
}

const filterOptions = computed(() => [
  {
    key: 'category',
    label: '分类',
    options: [
      { value: 'AI', label: '人工智能' },
      { value: 'ML', label: '机器学习' },
      { value: 'DL', label: '深度学习' },
      { value: 'NLP', label: '自然语言处理' },
      { value: 'CV', label: '计算机视觉' }
    ]
  },
  {
    key: 'source',
    label: '来源',
    options: [
      { value: 'manual', label: '手动添加' },
      { value: 'cnki', label: '知网' },
      { value: 'ieee', label: 'IEEE' },
      { value: 'arxiv', label: 'ArXiv' },
      { value: 'pubmed', label: 'PubMed' }
    ]
  },
  {
    key: 'year',
    label: '年份',
    options: [
      { value: '2024', label: '2024' },
      { value: '2023', label: '2023' },
      { value: '2022', label: '2022' },
      { value: '2021', label: '2021' },
      { value: '2020', label: '2020' }
    ]
  },
  {
    key: 'status',
    label: '状态',
    options: [
      { value: 'bookmarked', label: '已收藏' },
      { value: 'read', label: '已读' },
      { value: 'unread', label: '未读' }
    ]
  }
])

// Table columns for list view
const tableColumns = computed(() => [
  { key: 'title', label: '标题', sortable: true },
  { key: 'authors', label: '作者', width: '200px' },
  { key: 'category', label: '分类', width: '120px' },
  { key: 'year', label: '年份', width: '80px', sortable: true },
  { key: 'source', label: '来源', width: '120px' },
  { key: 'actions', label: '操作', width: '180px' }
])

// Current page
const currentPage = computed({
  get: () => page.value,
  set: (val) => paperStore.goToPage(val)
})

// Delete dialog
const deleteDialogVisible = ref(false)
const papersToDelete = ref<Paper[]>([])
const deleteMessage = computed(() => {
  const count = papersToDelete.value.length
  return count === 1
    ? `确定要删除论文《${papersToDelete.value[0].title}》吗？`
    : `确定要删除选中的 ${count} 篇论文吗？`
})

// Fetch papers
const fetchPapers = async () => {
  try {
    await paperStore.fetchPapers()
  } catch (error: any) {
    ElMessage.error(error.message || '加载论文列表失败')
  }
}

// Handle search
const handleSearch = () => {
  paperStore.setFilters({ keyword: searchKeyword.value })
  fetchPapers()
}

const handleSearchClear = () => {
  searchKeyword.value = ''
  paperStore.setFilters({ keyword: undefined })
  fetchPapers()
}

// Handle filter change
const handleFilterChange = (newFilters: Record<string, any>) => {
  const transformedFilters: any = {}

  if (newFilters.category) {
    transformedFilters.category = newFilters.category[0]
  }
  if (newFilters.source) {
    transformedFilters.source = newFilters.source[0]
  }
  if (newFilters.year) {
    transformedFilters.year = newFilters.year[0]
  }
  if (newFilters.status) {
    if (newFilters.status.includes('bookmarked')) {
      transformedFilters.isBookmarked = true
    }
    if (newFilters.status.includes('read')) {
      transformedFilters.isRead = true
    }
    if (newFilters.status.includes('unread')) {
      transformedFilters.isRead = false
    }
  }

  paperStore.setFilters(transformedFilters)
  fetchPapers()
}

const handleFilterClear = () => {
  paperStore.clearFilters()
  fetchPapers()
}

// Handle sort change
const handleSortChange = () => {
  paperStore.setSort(sortField.value, sortOrder.value)
  fetchPapers()
}

const toggleSortOrder = () => {
  sortOrder.value = sortOrder.value === 'asc' ? 'desc' : 'asc'
  handleSortChange()
}

// Handle page change
const handlePageChange = (newPage: number) => {
  currentPage.value = newPage
}

const handlePageSizeChange = (newSize: number) => {
  paperStore.setPageSize(newSize)
}

// Handle selection
const handleSelectPaper = (id: number) => {
  paperStore.toggleSelection(id)
}

const handleSelectionChange = (selectedRows: Paper[]) => {
  paperStore.deselectAll()
  selectedRows.forEach(row => paperStore.selectPaper(row.id))
}

// Handle paper actions
const handleCreate = () => {
  router.push('/papers/new')
}

const handleViewPaper = (paper: Paper) => {
  router.push(`/papers/${paper.id}`)
}

const handleEditPaper = (paper: Paper) => {
  router.push(`/papers/${paper.id}/edit`)
}

const handleDeletePaper = (paper: Paper) => {
  papersToDelete.value = [paper]
  deleteDialogVisible.value = true
}

const handleToggleBookmark = async (paper: Paper) => {
  try {
    await paperStore.toggleBookmark(paper.id)
    ElMessage.success(paper.isBookmarked ? '已取消收藏' : '已添加到收藏')
  } catch (error: any) {
    ElMessage.error(error.message || '操作失败')
  }
}

const handleToggleRead = async (paper: Paper) => {
  try {
    await paperStore.markAsRead(paper.id, !paper.isRead)
    ElMessage.success(paper.isRead ? '已标记为未读' : '已标记为已读')
  } catch (error: any) {
    ElMessage.error(error.message || '操作失败')
  }
}

// Batch operations
const handleBatchExport = async () => {
  try {
    // TODO: Implement export functionality
    ElMessage.info('导出功能开发中...')
  } catch (error: any) {
    ElMessage.error(error.message || '导出失败')
  }
}

const handleBatchDelete = () => {
  papersToDelete.value = selectedPapers.value
  deleteDialogVisible.value = true
}

const confirmDelete = async () => {
  try {
    if (papersToDelete.value.length === 1) {
      await paperStore.deletePaper(papersToDelete.value[0].id)
    } else {
      await paperStore.batchDelete()
    }
    ElMessage.success('删除成功')
    deleteDialogVisible.value = false
    papersToDelete.value = []
  } catch (error: any) {
    ElMessage.error(error.message || '删除失败')
  }
}

// Lifecycle
onMounted(() => {
  fetchPapers()
})
</script>

<style scoped lang="scss">
// ==========================================
// 现代化论文列表页面样式
// Modern Paper List View Styles
// ==========================================

.paper-list-view {
  display: flex;
  flex-direction: column;
  gap: $spacing-6;
  width: 100%;
  max-width: 1600px;
  margin: 0 auto;
}

// 页面头部
.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: $spacing-6;
  padding: $spacing-6;
  background: linear-gradient(135deg, #ffffff 0%, #f8fafc 100%);
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    box-shadow: $shadow-md;
  }

  &__content {
    flex: 1;
  }

  &__title {
    margin: 0 0 $spacing-2 0;
    font-size: $font-size-3xl;
    font-weight: $font-weight-bold;
    color: $text-primary;
    line-height: $line-height-tight;
  }

  &__subtitle {
    margin: 0;
    font-size: $font-size-sm;
    color: $text-secondary;
  }

  &__actions {
    display: flex;
    gap: $spacing-3;
  }
}

// 工具栏
.toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: $spacing-4;
  padding: $spacing-4;
  background: #ffffff;
  border-radius: $border-radius-lg;
  box-shadow: $shadow-sm;
  border: 1px solid $border-light;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  &__left,
  &__right {
    display: flex;
    align-items: center;
    gap: $spacing-3;
    flex-wrap: wrap;
  }
}

.search-input {
  width: 320px;
}

.filter-badge {
  margin-left: $spacing-1;
}

.selected-count {
  font-size: $font-size-sm;
  font-weight: $font-weight-medium;
  white-space: nowrap;
  padding: $spacing-2 $spacing-3;
  background: rgba($primary-500, 0.1);
  border-radius: $border-radius-base;
  color: $primary-600;

  .dark & {
    background: rgba($primary-400, 0.1);
    color: $primary-400;
  }
}

// 主内容区域
.content-container {
  display: flex;
  gap: $spacing-6;
  align-items: flex-start;
}

.filter-panel-wrapper {
  width: 280px;
  flex-shrink: 0;

  :deep(.filter-panel) {
    position: sticky;
    top: $spacing-4;
  }
}

.papers-container {
  flex: 1;
  min-width: 0;
}

// 论文网格
.papers-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(340px, 1fr));
  gap: $spacing-5;
  margin-bottom: $spacing-6;
}

.skeleton-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(340px, 1fr));
  gap: $spacing-5;
  margin-bottom: $spacing-6;
}

.empty-state {
  padding: $spacing-16 0;
  text-align: center;
  background: #ffffff;
  border-radius: $border-radius-lg;
  box-shadow: $shadow-sm;
  border: 1px solid $border-light;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }
}

.papers-list {
  background: #ffffff;
  border-radius: $border-radius-lg;
  box-shadow: $shadow-sm;
  border: 1px solid $border-light;
  overflow: hidden;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }
}

.title-cell {
  &-authors {
    font-size: $font-size-xs;
    color: $text-secondary;
    margin-top: $spacing-1;
  }
}

// 分页容器
.pagination-container {
  display: flex;
  justify-content: center;
  padding: $spacing-6;
  background: #ffffff;
  border-radius: $border-radius-lg;
  box-shadow: $shadow-sm;
  border: 1px solid $border-light;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }
}

// 响应式设计
@media (max-width: 1400px) {
  .paper-list-view {
    max-width: 1200px;
  }

  .search-input {
    width: 280px;
  }
}

@media (max-width: 1200px) {
  .paper-list-view {
    max-width: 100%;
  }

  .content-container {
    flex-direction: column;
  }

  .filter-panel-wrapper {
    width: 100%;
  }
}

@media (max-width: 768px) {
  .paper-list-view {
    gap: $spacing-4;
  }

  .page-header {
    flex-direction: column;
    align-items: flex-start;
    gap: $spacing-4;
    padding: $spacing-5;

    &__title {
      font-size: $font-size-2xl;
    }
  }

  .toolbar {
    flex-direction: column;
    align-items: stretch;
    gap: $spacing-4;
    padding: $spacing-4;

    &__left,
    &__right {
      width: 100%;
      justify-content: space-between;
    }
  }

  .search-input {
    width: 100%;
  }

  .papers-grid,
  .skeleton-grid {
    grid-template-columns: 1fr;
    gap: $spacing-4;
  }
}

@media (max-width: 480px) {
  .page-header {
    padding: $spacing-4;

    &__actions {
      width: 100%;

      .el-button {
        flex: 1;
      }
    }
  }

  .toolbar {
    padding: $spacing-3;
  }
}
</style>
