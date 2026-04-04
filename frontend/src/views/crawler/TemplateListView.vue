<template>
  <div class="template-list">
    <!-- Page Header -->
    <div class="page-header">
      <h1 class="page-title">
        <el-icon><Grid /></el-icon>
        模板管理
      </h1>
      <div class="header-actions">
        <el-button type="primary" :icon="Plus" @click="handleCreate">
          创建模板
        </el-button>
      </div>
    </div>

    <!-- Search and Filter -->
    <el-card shadow="hover" class="filter-card">
      <el-form :inline="true" :model="filterForm">
        <el-form-item label="搜索">
          <el-input
            v-model="filterForm.search"
            placeholder="搜索模板名称或描述"
            clearable
            @clear="handleFilter"
            @keyup.enter="handleFilter"
          >
            <template #prefix>
              <el-icon><Search /></el-icon>
            </template>
          </el-input>
        </el-form-item>
        <el-form-item label="数据源">
          <el-select
            v-model="filterForm.source"
            placeholder="全部数据源"
            clearable
            @change="handleFilter"
          >
            <el-option label="arXiv" value="arxiv" />
            <el-option label="PubMed" value="pubmed" />
            <el-option label="Google Scholar" value="scholar" />
            <el-option label="IEEE Xplore" value="ieeexplore" />
            <el-option label="ACM" value="acm" />
          </el-select>
        </el-form-item>
        <el-form-item>
          <el-button type="primary" @click="handleFilter">
            <el-icon><Search /></el-icon>
            搜索
          </el-button>
          <el-button @click="handleReset">
            <el-icon><RefreshLeft /></el-icon>
            重置
          </el-button>
        </el-form-item>
      </el-form>
    </el-card>

    <!-- View Toggle -->
    <div class="view-toggle">
      <el-radio-group v-model="viewMode">
        <el-radio-button label="table">
          <el-icon><List /></el-icon>
          表格视图
        </el-radio-button>
        <el-radio-button label="grid">
          <el-icon><Grid /></el-icon>
          网格视图
        </el-radio-button>
      </el-radio-group>
    </div>

    <!-- Templates Table View -->
    <el-card v-if="viewMode === 'table'" shadow="hover" class="table-card">
      <el-table
        :data="filteredTemplates"
        v-loading="loading"
        border
        stripe
        @selection-change="handleSelectionChange"
      >
        <el-table-column type="selection" width="55" />
        <el-table-column prop="name" label="模板名称" min-width="200" />
        <el-table-column prop="source" label="数据源" width="130">
          <template #default="{ row }">
            <el-tag :type="getSourceTagType(row.source)">
              {{ getSourceName(row.source) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="query" label="搜索关键词" min-width="200" show-overflow-tooltip />
        <el-table-column prop="limit" label="结果数量" width="100" align="center" />
        <el-table-column prop="usageCount" label="使用次数" width="100" align="center" />
        <el-table-column prop="lastUsed" label="最后使用" width="160">
          <template #default="{ row }">
            {{ row.lastUsed ? formatDate(row.lastUsed) : '未使用' }}
          </template>
        </el-table-column>
        <el-table-column label="操作" width="280" fixed="right">
          <template #default="{ row }">
            <el-button
              link
              type="primary"
              :icon="VideoPlay"
              @click="handleRun(row)"
            >
              运行
            </el-button>
            <el-button
              link
              type="primary"
              :icon="Edit"
              @click="handleEdit(row)"
            >
              编辑
            </el-button>
            <el-button
              link
              type="primary"
              :icon="Download"
              @click="handleExport(row)"
            >
              导出
            </el-button>
            <el-button
              link
              type="danger"
              :icon="Delete"
              @click="handleDelete(row)"
            >
              删除
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>

    <!-- Templates Grid View -->
    <div v-else class="grid-view">
      <el-empty v-if="filteredTemplates.length === 0 && !loading" description="暂无模板" />

      <el-row :gutter="20" v-else>
        <el-col
          v-for="template in filteredTemplates"
          :key="template.id"
          :xs="24"
          :sm="12"
          :md="8"
          :lg="6"
        >
          <el-card shadow="hover" class="template-card">
            <div class="template-header">
              <div class="template-icon" :class="template.source">
                <el-icon>
                  <component :is="getSourceIcon(template.source)" />
                </el-icon>
              </div>
              <el-dropdown @command="(cmd) => handleCommand(cmd, template)">
                <el-icon class="more-icon"><MoreFilled /></el-icon>
                <template #dropdown>
                  <el-dropdown-menu>
                    <el-dropdown-item command="run" :icon="VideoPlay">
                      运行
                    </el-dropdown-item>
                    <el-dropdown-item command="edit" :icon="Edit">
                      编辑
                    </el-dropdown-item>
                    <el-dropdown-item command="export" :icon="Download">
                      导出
                    </el-dropdown-item>
                    <el-dropdown-item command="delete" :icon="Delete" divided>
                      删除
                    </el-dropdown-item>
                  </el-dropdown-menu>
                </template>
              </el-dropdown>
            </div>

            <div class="template-body">
              <h3 class="template-name">{{ template.name }}</h3>
              <el-tag :type="getSourceTagType(template.source)" size="small">
                {{ getSourceName(template.source) }}
              </el-tag>
              <p class="template-query">{{ template.query }}</p>
              <div class="template-meta">
                <span>
                  <el-icon><Document /></el-icon>
                  限制: {{ template.limit || 10 }}
                </span>
                <span>
                  <el-icon><View /></el-icon>
                  使用: {{ template.usageCount }} 次
                </span>
              </div>
            </div>

            <div class="template-footer">
              <el-button type="primary" size="small" @click="handleRun(template)">
                <el-icon><VideoPlay /></el-icon>
                运行
              </el-button>
              <el-button size="small" @click="handleEdit(template)">
                <el-icon><Edit /></el-icon>
                编辑
              </el-button>
            </div>
          </el-card>
        </el-col>
      </el-row>
    </div>

    <!-- Batch Actions -->
    <div v-if="selectedTemplates.length > 0" class="batch-actions">
      <el-card shadow="hover">
        <div class="batch-content">
          <span>已选择 {{ selectedTemplates.length }} 个模板</span>
          <div class="batch-buttons">
            <el-button type="danger" :icon="Delete" @click="handleBatchDelete">
              批量删除
            </el-button>
            <el-button :icon="Download" @click="handleBatchExport">
              批量导出
            </el-button>
            <el-button @click="selectedTemplates = []">
              取消选择
            </el-button>
          </div>
        </div>
      </el-card>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useCrawlerStore, type CrawlerTemplate } from '@/stores/crawlerStore'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  Grid,
  Plus,
  Search,
  RefreshLeft,
  List,
  VideoPlay,
  Edit,
  Delete,
  Download,
  Document,
  View,
  MoreFilled
} from '@element-plus/icons-vue'
import { crawlerApi, type CrawlerSource, type CrawlerSearchRequest } from '@/api/modules/crawler'

const router = useRouter()
const crawlerStore = useCrawlerStore()

// State
const loading = ref(false)
const viewMode = ref<'table' | 'grid'>('grid')
const selectedTemplates = ref<CrawlerTemplate[]>([])

// Filter Form
const filterForm = ref({
  search: '',
  source: '' as CrawlerSource | ''
})

// Computed
const templates = computed(() => crawlerStore.templates)

const filteredTemplates = computed(() => {
  let filtered = templates.value

  // Search filter
  if (filterForm.value.search) {
    const search = filterForm.value.search.toLowerCase()
    filtered = filtered.filter(t =>
      t.name.toLowerCase().includes(search) ||
      t.query.toLowerCase().includes(search)
    )
  }

  // Source filter
  if (filterForm.value.source) {
    filtered = filtered.filter(t => t.source === filterForm.value.source)
  }

  return filtered
})

// Methods
const handleCreate = () => {
  router.push('/crawler/templates/new')
}

const handleEdit = (template: CrawlerTemplate) => {
  router.push(`/crawler/templates/${template.id}/edit`)
}

const handleRun = async (template: CrawlerTemplate) => {
  try {
    ElMessage.info('正在启动爬虫任务...')

    // Increment usage count
    crawlerStore.useTemplate(template.id)

    // Start crawler task
    const request: CrawlerSearchRequest = {
      query: template.query,
      source: template.source,
      limit: template.limit,
      options: template.options
    }

    const task = await crawlerStore.startTask(request)

    ElMessage.success(`任务已启动，ID: ${task.id}`)
    router.push('/crawler/tasks')
  } catch (error: any) {
    ElMessage.error('启动任务失败: ' + error.message)
  }
}

const handleExport = (template: CrawlerTemplate) => {
  const data = JSON.stringify(template, null, 2)
  const blob = new Blob([data], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `template-${template.name}-${Date.now()}.json`
  a.click()
  URL.revokeObjectURL(url)
  ElMessage.success('导出成功')
}

const handleDelete = (template: CrawlerTemplate) => {
  ElMessageBox.confirm(
    `确定要删除模板 "${template.name}" 吗？`,
    '确认删除',
    {
      type: 'warning',
      confirmButtonText: '删除',
      cancelButtonText: '取消'
    }
  ).then(() => {
    crawlerStore.deleteTemplate(template.id)
    ElMessage.success('删除成功')
  }).catch(() => {})
}

const handleFilter = () => {
  // Filter is computed, no action needed
}

const handleReset = () => {
  filterForm.value = {
    search: '',
    source: ''
  }
}

const handleSelectionChange = (selection: CrawlerTemplate[]) => {
  selectedTemplates.value = selection
}

const handleCommand = (command: string, template: CrawlerTemplate) => {
  switch (command) {
    case 'run':
      handleRun(template)
      break
    case 'edit':
      handleEdit(template)
      break
    case 'export':
      handleExport(template)
      break
    case 'delete':
      handleDelete(template)
      break
  }
}

const handleBatchDelete = () => {
  ElMessageBox.confirm(
    `确定要删除选中的 ${selectedTemplates.value.length} 个模板吗？`,
    '确认删除',
    {
      type: 'warning',
      confirmButtonText: '删除',
      cancelButtonText: '取消'
    }
  ).then(() => {
    selectedTemplates.value.forEach(t => {
      crawlerStore.deleteTemplate(t.id)
    })
    selectedTemplates.value = []
    ElMessage.success('批量删除成功')
  }).catch(() => {})
}

const handleBatchExport = () => {
  const data = JSON.stringify(selectedTemplates.value, null, 2)
  const blob = new Blob([data], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `templates-${Date.now()}.json`
  a.click()
  URL.revokeObjectURL(url)
  ElMessage.success('批量导出成功')
}

const getSourceName = (source: CrawlerSource) => {
  const names: Record<CrawlerSource, string> = {
    arxiv: 'arXiv',
    pubmed: 'PubMed',
    scholar: 'Google Scholar',
    ieeexplore: 'IEEE Xplore',
    acm: 'ACM'
  }
  return names[source] || source
}

const getSourceTagType = (source: CrawlerSource) => {
  const types: Record<string, any> = {
    arxiv: 'success',
    pubmed: 'warning',
    scholar: 'danger',
    ieeexplore: 'primary',
    acm: 'info'
  }
  return types[source] || 'info'
}

const getSourceIcon = (source: CrawlerSource) => {
  // Return a default icon - in real app, you'd have specific icons
  return Document
}

const formatDate = (timestamp: number) => {
  const date = new Date(timestamp)
  return date.toLocaleString()
}

// Lifecycle
onMounted(() => {
  // Templates are loaded from localStorage in the store
  if (templates.value.length === 0) {
    // Add some sample templates if empty
    crawlerStore.createTemplate({
      name: '深度学习最新论文',
      source: 'arxiv',
      query: 'deep learning',
      limit: 20
    })

    crawlerStore.createTemplate({
      name: '医学人工智能研究',
      source: 'pubmed',
      query: 'artificial intelligence in medicine',
      limit: 15
    })

    crawlerStore.createTemplate({
      name: '机器学习综述',
      source: 'scholar',
      query: 'machine learning survey',
      limit: 10
    })
  }
})
</script>

<style scoped lang="scss">
// ==========================================
// 模板列表页面现代化样式
// Modern Template List Styles
// ==========================================

.template-list {
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
  padding: $spacing-8;
  background: linear-gradient(135deg, #ffffff 0%, #f8fafc 100%);
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  margin-bottom: $spacing-6;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
  }

  .page-title {
    display: flex;
    align-items: center;
    gap: $spacing-3;
    margin: 0;
    font-size: $font-size-3xl;
    font-weight: $font-weight-bold;
    color: $text-primary;

    .el-icon {
      color: $primary-500;
    }
  }

  .header-actions {
    display: flex;
    gap: $spacing-3;
  }
}

// 筛选卡片
.filter-card {
  margin-bottom: $spacing-6;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  :deep(.el-card__body) {
    padding: $spacing-5;
  }

  :deep(.el-form--inline .el-form-item) {
    margin-right: $spacing-4;
    margin-bottom: $spacing-2;
  }
}

// 视图切换
.view-toggle {
  margin-bottom: $spacing-6;
  display: flex;
  justify-content: flex-end;

  :deep(.el-radio-group) {
    background: #ffffff;
    padding: $spacing-1;
    border-radius: $border-radius-lg;
    border: 1px solid $border-light;
    box-shadow: $shadow-sm;

    .dark & {
      background: $gray-800;
      border-color: $gray-700;
    }

    .el-radio-button {
      margin: 0;
      padding: $spacing-3 $spacing-4;
      border-radius: $border-radius-base;

      .el-radio-button__inner {
        font-weight: $font-weight-medium;
      }
    }
  }
}

// 表格卡片
.table-card {
  margin-bottom: $spacing-6;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  :deep(.el-card__body) {
    padding: $spacing-5;
  }

  :deep(.el-table) {
    border-radius: $border-radius-lg;

    th {
      background: $gray-50 !important;
      color: $text-primary;
      font-weight: $font-weight-semibold;
    }

    tr {
      &:hover {
        background: $gray-100 !important;
      }

      &.el-table__row--striped {
        background: rgba($gray-100, 0.5);
      }
    }

    .dark & {
      th {
        background: $gray-700 !important;
        color: $gray-200;
      }

      tr:hover {
        background: $gray-700 !important;
      }

      tr.el-table__row--striped {
        background: rgba($gray-700, 0.5);
      }
    }
  }
}

// 网格视图
.grid-view {
  margin-bottom: $spacing-6;

  :deep(.el-col) {
    margin-bottom: $spacing-5;
  }
}

// 模板卡片
.template-card {
  height: 100%;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  transition: all $duration-slow;
  overflow: hidden;
  display: flex;
  flex-direction: column;

  &:hover {
    box-shadow: $shadow-lg;
    transform: translateY(-4px);
    border-color: $primary-200;
  }

  .dark &:hover {
    border-color: $primary-400;
  }

  :deep(.el-card__body) {
    padding: 0;
    height: 100%;
    display: flex;
    flex-direction: column;
  }

  .template-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: $spacing-5;
    background: linear-gradient(135deg, $gray-50 0%, #ffffff 100%);
    border-bottom: 1px solid $border-light;

    .dark & {
      background: linear-gradient(135deg, $gray-800 0%, $gray-700 100%);
      border-bottom-color: $gray-700;
    }

    .template-icon {
      width: 56px;
      height: 56px;
      border-radius: $border-radius-lg;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 24px;
      color: white;
      box-shadow: $shadow-sm;
      flex-shrink: 0;

      &.arxiv {
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      }

      &.pubmed {
        background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
      }

      &.scholar {
        background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
      }

      &.ieeexplore {
        background: linear-gradient(135deg, #43e97b 0%, #38f9d7 100%);
      }

      &.acm {
        background: linear-gradient(135deg, #fa709a 0%, #fee140 100%);
      }
    }

    .more-icon {
      cursor: pointer;
      font-size: 20px;
      color: $text-secondary;
      padding: $spacing-2;
      border-radius: $border-radius-base;
      transition: all $duration-fast;

      &:hover {
        color: $primary-600;
        background: rgba($primary-500, 0.1);
      }

      .dark &:hover {
        color: $primary-400;
        background: rgba($primary-400, 0.1);
      }
    }
  }

  .template-body {
    flex: 1;
    padding: $spacing-5;
    display: flex;
    flex-direction: column;

    .template-name {
      font-size: $font-size-lg;
      font-weight: $font-weight-semibold;
      color: $text-primary;
      margin: 0 0 $spacing-3 0;
      line-height: $line-height-tight;
    }

    .template-query {
      color: $text-regular;
      font-size: $font-size-sm;
      margin: $spacing-3 0;
      line-height: $line-height-relaxed;
      min-height: 48px;
      padding: $spacing-3;
      background: $gray-50;
      border-radius: $border-radius-base;
      border: 1px solid $border-light;

      .dark & {
        background: $gray-800;
        border-color: $gray-700;
      }
    }

    .template-meta {
      display: flex;
      justify-content: space-between;
      font-size: $font-size-sm;
      color: $text-secondary;
      margin-top: auto;
      padding-top: $spacing-4;

      span {
        display: flex;
        align-items: center;
        gap: $spacing-2;
        font-weight: $font-weight-medium;
      }

      .el-icon {
        font-size: 16px;
      }
    }
  }

  .template-footer {
    display: flex;
    gap: $spacing-3;
    padding: $spacing-4 $spacing-5;
    border-top: 1px solid $border-light;

    .dark & {
      border-top-color: $gray-700;
    }

    .el-button {
      flex: 1;
      font-weight: $font-weight-medium;
    }
  }
}

// 批量操作栏
.batch-actions {
  position: fixed;
  bottom: $spacing-6;
  left: 50%;
  transform: translateX(-50%);
  z-index: 1030;
  min-width: 500px;
  animation: slideUp $duration-slow $easing-ease-out;

  :deep(.el-card) {
    background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
    border: none;
    box-shadow: $shadow-lg;

    .el-card__body {
      padding: $spacing-4 $spacing-5;
    }
  }

  .batch-content {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: $spacing-6;

    span {
      color: white;
      font-size: $font-size-base;
      font-weight: $font-weight-semibold;
    }

    .batch-buttons {
      display: flex;
      gap: $spacing-3;
    }
  }
}

@keyframes slideUp {
  from {
    transform: translateX(-50%) translateY(100%);
    opacity: 0;
  }
  to {
    transform: translateX(-50%) translateY(0);
    opacity: 1;
  }
}

// 响应式设计
@media (max-width: 1200px) {
  .template-list {
    max-width: 100%;
  }
}

@media (max-width: 768px) {
  .page-header {
    flex-direction: column;
    align-items: flex-start;
    gap: $spacing-4;
    padding: $spacing-5;

    .page-title {
      font-size: $font-size-2xl;
    }

    .header-actions {
      width: 100%;

      .el-button {
        flex: 1;
      }
    }
  }

  .filter-card {
    :deep(.el-form--inline .el-form-item) {
      display: block;
      margin-right: 0;
      margin-bottom: $spacing-4;
    }

    :deep(.el-form-item__content) {
      width: 100%;

      .el-input,
      .el-select {
        width: 100%;
      }
    }
  }

  .view-toggle {
    justify-content: center;
  }

  .template-card {
    .template-body {
      .template-meta {
        flex-direction: column;
        gap: $spacing-2;
      }
    }
  }

  .batch-actions {
    min-width: 90%;
    left: 5%;
    transform: none;

    .batch-content {
      flex-direction: column;
      gap: $spacing-3;

      .batch-buttons {
        width: 100%;
        display: flex;
        flex-direction: column;

        .el-button {
          width: 100%;
        }
      }
    }
  }
}

@media (max-width: 480px) {
  .page-header {
    padding: $spacing-4;
  }

  .filter-card,
  .table-card {
    :deep(.el-card__body) {
      padding: $spacing-4;
    }
  }

  .template-card .template-header {
    padding: $spacing-4;

    .template-icon {
      width: 48px;
      height: 48px;
      font-size: 20px;
    }
  }

  .template-card .template-body {
    padding: $spacing-4;
  }

  .template-card .template-footer {
    padding: $spacing-3 $spacing-4;
    flex-direction: column;

    .el-button {
      width: 100%;
    }
  }
}
</style>
