<template>
  <div class="papers-container">
    <!-- 页面头部 -->
    <div class="page-header">
      <div class="header-left">
        <h1 class="page-title">{{ $t('papers.title') || '我的论文' }}</h1>
        <div class="page-stats" v-if="paperStore.stats">
          <el-tag size="small" type="info">
            {{ $t('papers.total') || '总计' }}: {{ paperStore.stats.totalPapers }}
          </el-tag>
          <el-tag size="small" type="success">
            {{ $t('papers.read') || '已读' }}: {{ paperStore.stats.readPapers }}
          </el-tag>
          <el-tag size="small" type="warning">
            {{ $t('papers.unread') || '未读' }}: {{ paperStore.stats.unreadPapers }}
          </el-tag>
          <el-tag size="small" type="danger">
            {{ $t('papers.bookmarked') || '收藏' }}: {{ paperStore.stats.bookmarkedPapers }}
          </el-tag>
        </div>
      </div>
      <div class="header-right">
        <el-button
          type="primary"
          :icon="Plus"
          @click="showCreateDialog = true"
        >
          {{ $t('papers.addPaper') || '添加论文' }}
        </el-button>
      </div>
    </div>

    <!-- 搜索和筛选栏 -->
    <div class="search-filter-bar">
      <el-row :gutter="16">
        <!-- 搜索框 -->
        <el-col :span="8">
          <el-input
            v-model="searchKeyword"
            :placeholder="$t('papers.searchPlaceholder') || '搜索标题、作者、摘要...'"
            :prefix-icon="Search"
            clearable
            @clear="handleSearchClear"
            @keyup.enter="handleSearch"
          >
            <template #append>
              <el-button :icon="Search" @click="handleSearch" />
            </template>
          </el-input>
        </el-col>

        <!-- 筛选器 -->
        <el-col :span="4">
          <el-select
            v-model="paperStore.filters.category"
            :placeholder="$t('papers.category') || '分类'"
            clearable
            @change="handleFilterChange"
          >
            <el-option label="全部" value="" />
            <el-option label="人工智能" value="AI" />
            <el-option label="机器学习" value="ML" />
            <el-option label="深度学习" value="DL" />
            <el-option label="自然语言处理" value="NLP" />
            <el-option label="计算机视觉" value="CV" />
          </el-select>
        </el-col>

        <el-col :span="4">
          <el-select
            v-model="paperStore.filters.source"
            :placeholder="$t('papers.source') || '来源'"
            clearable
            @change="handleFilterChange"
          >
            <el-option label="全部" value="" />
            <el-option label="手动添加" value="manual" />
            <el-option label="知网" value="cnki" />
            <el-option label="IEEE" value="ieee" />
            <el-option label="ArXiv" value="arxiv" />
            <el-option label="PubMed" value="pubmed" />
          </el-select>
        </el-col>

        <el-col :span="4">
          <el-select
            v-model="readStatusFilter"
            :placeholder="$t('papers.status') || '状态'"
            clearable
            @change="handleStatusFilterChange"
          >
            <el-option label="全部" value="" />
            <el-option label="已读" value="read" />
            <el-option label="未读" value="unread" />
          </el-select>
        </el-col>

        <el-col :span="4">
          <el-button
            v-if="paperStore.hasFilters"
            :icon="RefreshLeft"
            @click="handleClearFilters"
          >
            {{ $t('papers.clearFilters') || '清除筛选' }}
          </el-button>
        </el-col>
      </el-row>
    </div>

    <!-- 批量操作工具栏 -->
    <div class="batch-actions" v-if="paperStore.selectedPaperIds.length > 0">
      <div class="selection-info">
        <el-checkbox
          :model-value="paperStore.allSelected"
          :indeterminate="paperStore.selectedPaperIds.length > 0 && !paperStore.allSelected"
          @change="paperStore.selectAll()"
        >
          {{ $t('papers.selected', { count: paperStore.selectedPaperIds.length }) || `已选择 ${paperStore.selectedPaperIds.length} 项` }}
        </el-checkbox>
      </div>
      <div class="action-buttons">
        <el-button
          size="small"
          :icon="Bookmark"
          @click="handleBatchBookmark(true)"
        >
          {{ $t('papers.batchBookmark') || '批量收藏' }}
        </el-button>
        <el-button
          size="small"
          @click="handleBatchMarkAsRead(true)"
        >
          {{ $t('papers.markAsRead') || '标记已读' }}
        </el-button>
        <el-button
          size="small"
          type="danger"
          :icon="Delete"
          @click="handleBatchDelete"
        >
          {{ $t('papers.batchDelete') || '批量删除' }}
        </el-button>
      </div>
    </div>

    <!-- 论文列表 -->
    <div class="papers-list" v-loading="paperStore.loading">
      <!-- 空状态 -->
      <el-empty
        v-if="!paperStore.loading && !paperStore.hasPapers"
        :description="paperStore.hasFilters ? ($t('papers.noFilterResults') || '没有符合条件的论文') : ($t('papers.noPapers') || '还没有论文，点击上方按钮添加')"
      >
        <el-button
          v-if="!paperStore.hasFilters"
          type="primary"
          :icon="Plus"
          @click="showCreateDialog = true"
        >
          {{ $t('papers.addFirstPaper') || '添加第一篇论文' }}
        </el-button>
      </el-empty>

      <!-- 论文卡片列表 -->
      <div v-else class="paper-cards">
        <el-row :gutter="16">
          <el-col
            v-for="paper in paperStore.papers"
            :key="paper.id"
            :xs="24"
            :sm="12"
            :md="8"
            :lg="6"
          >
            <paper-card
              :paper="paper"
              :selected="paperStore.selectedPaperIds.includes(paper.id)"
              @select="paperStore.selectPaper(paper.id)"
              @view="handleViewPaper"
              @edit="handleEditPaper"
              @delete="handleDeletePaper"
              @toggle-bookmark="handleToggleBookmark"
              @toggle-read="handleToggleRead"
            />
          </el-col>
        </el-row>
      </div>
    </div>

    <!-- 分页控件 -->
    <div class="pagination-container" v-if="paperStore.total > 0">
      <el-pagination
        v-model:current-page="paperStore.currentPage"
        v-model:page-size="paperStore.pageSize"
        :page-sizes="[10, 20, 50, 100]"
        :total="paperStore.total"
        layout="total, sizes, prev, pager, next, jumper"
        @size-change="handleSizeChange"
        @current-change="handlePageChange"
      />
    </div>

    <!-- 创建/编辑论文对话框 -->
    <paper-form-dialog
      v-model="showCreateDialog"
      :paper="editingPaper"
      @save="handleSavePaper"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  Plus,
  Search,
  RefreshLeft,
  Bookmark,
  Delete
} from '@element-plus/icons-vue'
import { usePaperManagementStore } from '@/stores/paperManagement'
import type { Paper } from '@/api/modules/papers'
import PaperCard from '@/components/paper/PaperCard.vue'
import PaperFormDialog from '@/components/paper/PaperFormDialog.vue'

const { t } = useI18n()
const router = useRouter()
const paperStore = usePaperManagementStore()

// 搜索关键词
const searchKeyword = ref('')

// 已读状态筛选
const readStatusFilter = ref('')

// 显示创建对话框
const showCreateDialog = ref(false)

// 编辑的论文
const editingPaper = ref<Paper | null>(null)

// 初始化
onMounted(async () => {
  await loadPapers()
  await paperStore.fetchStats()
})

// 加载论文列表
async function loadPapers() {
  const result = await paperStore.fetchPapers()
  if (!result.success && result.error) {
    ElMessage.error(result.error)
  }
}

// 搜索
async function handleSearch() {
  if (searchKeyword.value.trim()) {
    await paperStore.search(searchKeyword.value.trim())
  } else {
    await paperStore.clearFilters()
  }
}

// 清除搜索
async function handleSearchClear() {
  searchKeyword.value = ''
  await paperStore.clearFilters()
}

// 筛选变更
async function handleFilterChange() {
  await paperStore.fetchPapers(1)
}

// 状态筛选变更
async function handleStatusFilterChange() {
  if (readStatusFilter.value === 'read') {
    await paperStore.applyFilters({ isRead: true })
  } else if (readStatusFilter.value === 'unread') {
    await paperStore.applyFilters({ isRead: false })
  } else {
    await paperStore.applyFilters({ isRead: undefined })
  }
}

// 清除筛选
async function handleClearFilters() {
  searchKeyword.value = ''
  readStatusFilter.value = ''
  await paperStore.clearFilters()
}

// 分页变更
async function handlePageChange(page: number) {
  await paperStore.setPage(page)
}

// 每页数量变更
async function handleSizeChange(size: number) {
  await paperStore.setPageSize(size)
}

// 查看论文详情
function handleViewPaper(paper: Paper) {
  router.push({
    name: 'PaperDetail',
    params: { id: paper.id }
  })
}

// 编辑论文
function handleEditPaper(paper: Paper) {
  editingPaper.value = paper
  showCreateDialog.value = true
}

// 删除论文
async function handleDeletePaper(paper: Paper) {
  try {
    await ElMessageBox.confirm(
      t('papers.deleteConfirm') || `确定要删除《${paper.title}》吗？`,
      t('common.warning') || '警告',
      {
        confirmButtonText: t('common.confirm') || '确定',
        cancelButtonText: t('common.cancel') || '取消',
        type: 'warning'
      }
    )

    const result = await paperStore.deletePaper(paper.id)
    if (result.success) {
      ElMessage.success(t('papers.deleteSuccess') || '删除成功')
      await paperStore.fetchStats()
    } else if (result.error) {
      ElMessage.error(result.error)
    }
  } catch (error) {
    // 用户取消删除
  }
}

// 切换收藏
async function handleToggleBookmark(paper: Paper) {
  const result = await paperStore.toggleBookmark(paper.id)
  if (result.success) {
    ElMessage.success(
      paper.isBookmarked
        ? (t('papers.bookmarked') || '已收藏')
        : (t('papers.unbookmarked') || '已取消收藏')
    )
    await paperStore.fetchStats()
  } else if (result.error) {
    ElMessage.error(result.error)
  }
}

// 切换已读
async function handleToggleRead(paper: Paper) {
  const result = await paperStore.markAsRead(paper.id, !paper.isRead)
  if (result.success) {
    ElMessage.success(
      paper.isRead
        ? (t('papers.markedAsUnread') || '已标记为未读')
        : (t('papers.markedAsRead') || '已标记为已读')
    )
    await paperStore.fetchStats()
  } else if (result.error) {
    ElMessage.error(result.error)
  }
}

// 保存论文
async function handleSavePaper(data: any) {
  const result = editingPaper.value
    ? await paperStore.updatePaper(editingPaper.value.id, data)
    : await paperStore.createPaper(data)

  if (result.success) {
    ElMessage.success(
      editingPaper.value
        ? (t('papers.updateSuccess') || '更新成功')
        : (t('papers.createSuccess') || '添加成功')
    )
    showCreateDialog.value = false
    editingPaper.value = null
    await paperStore.fetchStats()
  } else if (result.error) {
    ElMessage.error(result.error)
  }
}

// 批量收藏
async function handleBatchBookmark(bookmarked: boolean) {
  const result = await paperStore.batchToggleBookmark(bookmarked)
  if (result.success) {
    ElMessage.success(
      bookmarked
        ? (t('papers.batchBookmarked') || '批量收藏成功')
        : (t('papers.batchUnbookmarked') || '批量取消收藏成功')
    )
  } else if (result.error) {
    ElMessage.error(result.error)
  }
}

// 批量标记已读
async function handleBatchMarkAsRead(isRead: boolean) {
  const result = await paperStore.batchMarkAsRead(isRead)
  if (result.success) {
    ElMessage.success(
      isRead
        ? (t('papers.batchMarkedAsRead') || '批量标记已读成功')
        : (t('papers.batchMarkedAsUnread') || '批量标记未读成功')
    )
    await paperStore.fetchStats()
  } else if (result.error) {
    ElMessage.error(result.error)
  }
}

// 批量删除
async function handleBatchDelete() {
  try {
    await ElMessageBox.confirm(
      t('papers.batchDeleteConfirm') || `确定要删除选中的 ${paperStore.selectedPaperIds.length} 篇论文吗？`,
      t('common.warning') || '警告',
      {
        confirmButtonText: t('common.confirm') || '确定',
        cancelButtonText: t('common.cancel') || '取消',
        type: 'warning'
      }
    )

    const result = await paperStore.batchDelete()
    if (result.success) {
      ElMessage.success(t('papers.batchDeleteSuccess') || '批量删除成功')
      await paperStore.fetchStats()
    } else if (result.error) {
      ElMessage.error(result.error)
    }
  } catch (error) {
    // 用户取消删除
  }
}
</script>

<style scoped lang="scss">
.papers-container {
  padding: 24px;
  background: #f5f7fa;
  min-height: 100vh;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
  padding: 20px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.header-left {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.page-title {
  margin: 0;
  font-size: 24px;
  font-weight: 600;
  color: #303133;
}

.page-stats {
  display: flex;
  gap: 8px;
}

.search-filter-bar {
  margin-bottom: 16px;
  padding: 16px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.batch-actions {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
  padding: 12px 16px;
  background: #e6f7ff;
  border: 1px solid #91d5ff;
  border-radius: 8px;
}

.selection-info {
  font-weight: 500;
  color: #1890ff;
}

.action-buttons {
  display: flex;
  gap: 8px;
}

.papers-list {
  min-height: 400px;
  margin-bottom: 16px;
}

.paper-cards {
  margin-top: 16px;
}

.pagination-container {
  display: flex;
  justify-content: center;
  padding: 16px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

@media (max-width: 768px) {
  .papers-container {
    padding: 16px;
  }

  .page-header {
    flex-direction: column;
    gap: 16px;
    align-items: flex-start;
  }

  .search-filter-bar {
    .el-col {
      width: 100% !important;
      margin-bottom: 8px;
    }
  }

  .batch-actions {
    flex-direction: column;
    gap: 12px;
    align-items: flex-start;
  }

  .action-buttons {
    flex-wrap: wrap;
    width: 100%;
  }
}
</style>
