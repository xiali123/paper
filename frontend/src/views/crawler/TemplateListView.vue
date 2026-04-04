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
.template-list {
  padding: 20px;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;

  .page-title {
    display: flex;
    align-items: center;
    gap: 10px;
    margin: 0;
    font-size: 24px;
  }
}

.filter-card {
  margin-bottom: 20px;
}

.view-toggle {
  margin-bottom: 20px;
  display: flex;
  justify-content: flex-end;
}

.table-card {
  margin-bottom: 20px;
}

.grid-view {
  margin-bottom: 20px;
}

.template-card {
  margin-bottom: 20px;
  height: 100%;
  display: flex;
  flex-direction: column;
  transition: all 0.3s;

  &:hover {
    transform: translateY(-4px);
    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
  }

  .template-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 15px;

    .template-icon {
      width: 50px;
      height: 50px;
      border-radius: 10px;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 24px;
      color: white;

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
      color: #909399;

      &:hover {
        color: #409eff;
      }
    }
  }

  .template-body {
    flex: 1;

    .template-name {
      font-size: 18px;
      margin: 0 0 10px 0;
      color: #303133;
    }

    .template-query {
      color: #606266;
      font-size: 14px;
      margin: 10px 0;
      min-height: 40px;
    }

    .template-meta {
      display: flex;
      justify-content: space-between;
      font-size: 12px;
      color: #909399;
      margin: 15px 0;

      span {
        display: flex;
        align-items: center;
        gap: 4px;
      }
    }
  }

  .template-footer {
    display: flex;
    gap: 10px;
    padding-top: 15px;
    border-top: 1px solid #ebeef5;

    .el-button {
      flex: 1;
    }
  }
}

.batch-actions {
  position: fixed;
  bottom: 20px;
  left: 50%;
  transform: translateX(-50%);
  z-index: 1000;
  min-width: 400px;

  .batch-content {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 20px;

    .batch-buttons {
      display: flex;
      gap: 10px;
    }
  }
}

// Responsive
@media (max-width: 768px) {
  .page-header {
    flex-direction: column;
    gap: 10px;
    text-align: center;
  }

  .batch-actions {
    min-width: 90%;
    left: 5%;
    transform: none;

    .batch-content {
      flex-direction: column;
      gap: 10px;

      .batch-buttons {
        width: 100%;
        justify-content: center;

        .el-button {
          flex: 1;
        }
      }
    }
  }
}
</style>
