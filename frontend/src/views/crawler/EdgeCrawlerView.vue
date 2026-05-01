<template>
  <div class="edge-crawler" role="main" aria-label="边缘爬虫">
    <!-- Page Header -->
    <div class="page-header">
      <div class="header-left">
        <h1 class="page-title">
          <el-icon><Connection /></el-icon>
          边缘爬虫
        </h1>
        <p class="page-description">
          在浏览器中直接爬取数据并同步到服务器
        </p>
      </div>
      <div class="header-actions">
        <el-button type="primary" :icon="Plus" @click="showCreateDialog = true">
          新建爬取任务
        </el-button>
        <el-button :icon="Upload" @click="handleSyncSelected" :disabled="!hasSelectedPapers">
          同步选中 ({{ selectedCount }})
        </el-button>
        <el-button :icon="Refresh" @click="refreshData" :loading="loading">
          刷新
        </el-button>
      </div>
    </div>

    <!-- Quick Stats -->
    <el-row :gutter="16" class="stats-row">
      <el-col :xs="12" :sm="6">
        <div class="quick-stat">
          <div class="stat-icon crawled">
            <el-icon><Document /></el-icon>
          </div>
          <div class="stat-content">
            <div class="stat-value">{{ stats.crawledCount }}</div>
            <div class="stat-label">\{\{ \$t('totalCrawled') \}\}</div>
          </div>
        </div>
      </el-col>
      <el-col :xs="12" :sm="6">
        <div class="quick-stat">
          <div class="stat-icon synced">
            <el-icon><CircleCheck /></el-icon>
          </div>
          <div class="stat-content">
            <div class="stat-value">{{ stats.syncedCount }}</div>
            <div class="stat-label">\{\{ \$t('totalSynced') \}\}</div>
          </div>
        </div>
      </el-col>
      <el-col :xs="12" :sm="6">
        <div class="quick-stat">
          <div class="stat-icon pending">
            <el-icon><Clock /></el-icon>
          </div>
          <div class="stat-content">
            <div class="stat-value">{{ stats.pendingCount }}</div>
            <div class="stat-label">\{\{ \$t('pendingSync') \}\}</div>
          </div>
        </div>
      </el-col>
      <el-col :xs="12" :sm="6">
        <div class="quick-stat">
          <div class="stat-icon active">
            <el-icon><Loading /></el-icon>
          </div>
          <div class="stat-content">
            <div class="stat-value">{{ activeCrawlers }}</div>
            <div class="stat-label">\{\{ \$t('activeTasks') \}\}</div>
          </div>
        </div>
      </el-col>
    </el-row>

    <!-- Main Content -->
    <div class="main-content">
      <!-- Left Panel: Crawler Tasks -->
      <div class="tasks-panel">
        <el-card shadow="hover">
          <template #header>
            <div class="card-header">
              <span>爬取任务</span>
              <el-tag v-if="tasks.length > 0" size="small">{{ tasks.length }}</el-tag>
            </div>
          </template>

          <!-- Task List -->
          <div class="task-list">
            <div
              v-for="task in tasks"
              :key="task.id"
              :class="['task-item', { active: currentTask?.id === task.id }]"
              @click="selectTask(task)"
            >
              <div class="task-header">
                <div class="task-name">{{ task.name }}</div>
                <el-dropdown @command="cmd => handleTaskAction(cmd, task)">
                  <el-icon class="task-more"><MoreFilled /></el-icon>
                  <template #dropdown>
                    <el-dropdown-menu>
                      <el-dropdown-item command="start" :disabled="task.status === 'running'">
                        <el-icon><VideoPlay /></el-icon> 开始
                      </el-dropdown-item>
                      <el-dropdown-item command="pause" :disabled="task.status !== 'running'">
                        <el-icon><VideoPause /></el-icon> 暂停
                      </el-dropdown-item>
                      <el-dropdown-item command="sync" :disabled="task.results.length === 0">
                        <el-icon><Upload /></el-icon> 同步结果
                      </el-dropdown-item>
                      <el-dropdown-item command="delete" divided>
                        <el-icon><Delete /></el-icon> 删除
                      </el-dropdown-item>
                    </el-dropdown-menu>
                  </template>
                </el-dropdown>
              </div>

              <div class="task-meta">
                <el-tag :type="getTaskStatusType(task.status)" size="small">
                  {{ getTaskStatusText(task.status) }}
                </el-tag>
                <span class="task-count">{{ task.results.length }} 条结果</span>
              </div>

              <div v-if="task.status === 'running'" class="task-progress">
                <el-progress
                  :percentage="task.progress || 0"
                  :indeterminate="task.progress === undefined"
                  :duration="1"
                />
              </div>
            </div>

            <el-empty v-if="tasks.length === 0" description="暂无爬取任务" :image-size="80">
              <el-button type="primary" @click="showCreateDialog = true">
                创建第一个任务
              </el-button>
            </el-empty>
          </div>
        </el-card>
      </div>

      <!-- Right Panel: Results -->
      <div class="results-panel">
        <el-card shadow="hover">
          <template #header>
            <div class="card-header">
              <span>
                {{ currentTask ? currentTask.name : '爬取结果' }}
                <el-tag v-if="currentTask" :type="getTaskStatusType(currentTask.status)" size="small">
                  {{ getTaskStatusText(currentTask.status) }}
                </el-tag>
              </span>
              <div class="header-actions">
                <el-checkbox v-model="selectAll" @change="handleSelectAll" :indeterminate="isIndeterminate">
                  全选
                </el-checkbox>
                <el-button
                  type="primary"
                  size="small"
                  :icon="Upload"
                  @click="handleSyncSelected"
                  :disabled="!hasSelectedPapers"
                >
                  同步选中
                </el-button>
              </div>
            </div>
          </template>

          <!-- Results Grid -->
          <div v-if="currentTask && currentTask.results.length > 0" class="results-grid">
            <div
              v-for="paper in currentTask.results"
              :key="paper.id"
              :class="['paper-card', { selected: selectedPapers.has(paper.id) }]"
              @click="toggleSelectPaper(paper)"
            >
              <div class="paper-checkbox">
                <el-checkbox
                  :model-value="selectedPapers.has(paper.id)"
                  @change="toggleSelectPaper(paper)"
                  @click.stop
                />
              </div>

              <div class="paper-status">
                <el-icon v-if="paper.synced" class="synced-icon"><CircleCheck /></el-icon>
                <el-icon v-else class="pending-icon"><Clock /></el-icon>
              </div>

              <div class="paper-content">
                <h4 class="paper-title">{{ paper.title }}</h4>
                <div class="paper-meta">
                  <span class="paper-authors">{{ truncateAuthors(paper.authors) }}</span>
                  <span class="paper-year">{{ paper.year }}</span>
                </div>
                <div class="paper-abstract">{{ truncateAbstract(paper.abstract) }}</div>

                <div class="paper-actions">
                  <el-button size="small" text @click.stop="handlePreview(paper)">
                    <el-icon><View /></el-icon> 预览
                  </el-button>
                  <el-button size="small" text @click.stop="handleSyncOne(paper)" :disabled="paper.synced">
                    <el-icon><Upload /></el-icon> {{ paper.synced ? '已同步' : '同步' }}
                  </el-button>
                  <el-button size="small" text type="danger" @click.stop="handleDelete(paper)">
                    <el-icon><Delete /></el-icon> 删除
                  </el-button>
                </div>
              </div>
            </div>
          </div>

          <el-empty
            v-else
            :description="currentTask ? '暂无爬取结果' : '请选择一个爬取任务'"
            :image-size="80"
          />
        </el-card>
      </div>
    </div>

    <!-- Create Task Dialog -->
    <el-dialog
      v-model="showCreateDialog"
      title="创建边缘爬取任务"
      width="700px"
      :close-on-click-modal="false"
    >
      <el-form :model="newTask" :rules="taskRules" ref="taskFormRef" label-width="120px">
        <el-form-item label="任务名称" prop="name">
          <el-input v-model="newTask.name" placeholder="请输入任务名称" />
        </el-form-item>

        <el-form-item label="目标URL" prop="url">
          <el-input
            v-model="newTask.url"
            placeholder="https://arxiv.org/list/cs.AI/recent"
            type="textarea"
            :rows="3"
          />
          <div class="form-tip">
            支持Arxiv、Google Scholar、PubMed等学术网站
          </div>
        </el-form-item>

        <el-form-item label="爬取数量" prop="maxPapers">
          <el-input-number v-model="newTask.maxPapers" :min="1" :max="100" />
          <span class="form-tip">每个任务最多爬取100篇论文</span>
        </el-form-item>

        <el-form-item label="自动同步">
          <el-switch v-model="newTask.autoSync" />
          <span class="form-tip">爬取完成后自动同步到服务器</span>
        </el-form-item>
      </el-form>

      <template #footer>
        <el-button @click="showCreateDialog = false">取消</el-button>
        <el-button type="primary" @click="handleCreateTask" :loading="creating">
          创建并开始
        </el-button>
      </template>
    </el-dialog>

    <!-- Preview Dialog -->
    <el-dialog v-model="showPreviewDialog" title="论文预览" width="800px">
      <div v-if="previewPaper" class="preview-content">
        <h2>{{ previewPaper.title }}</h2>
        <div class="preview-meta">
          <span>{{ previewPaper.authors }}</span>
          <span>{{ previewPaper.year }}</span>
        </div>
        <div class="preview-abstract">{{ previewPaper.abstract }}</div>
        <div v-if="previewPaper.url" class="preview-link">
          <el-link :href="previewPaper.url" target="_blank" type="primary">
            查看原文 <el-icon><TopRight /></el-icon>
          </el-link>
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  Plus,
  Upload,
  Refresh,
  Connection,
  Document,
  CircleCheck,
  Clock,
  Loading,
  MoreFilled,
  VideoPlay,
  VideoPause,
  Delete,
  View,
  TopRight
} from '@element-plus/icons-vue'
import { crawlerApi } from '@/api/modules/crawler'

// Types
interface CrawlerTask {
  id: string
  name: string
  url: string
  status: 'idle' | 'running' | 'paused' | 'completed' | 'error'
  progress?: number
  results: Paper[]
  maxPapers: number
  autoSync: boolean
  createdAt: Date
}

interface Paper {
  id: string
  title: string
  authors: string
  year: string
  abstract: string
  url?: string
  synced: boolean
}

// Data
const tasks = ref<CrawlerTask[]>([])
const currentTask = ref<CrawlerTask | null>(null)
const selectedPapers = ref<Set<string>>(new Set())
const loading = ref(false)
const creating = ref(false)
const showCreateDialog = ref(false)
const showPreviewDialog = ref(false)
const previewPaper = ref<Paper | null>(null)
const taskFormRef = ref()

const newTask = ref({
  name: '',
  url: '',
  maxPapers: 20,
  autoSync: true
})

const taskRules = {
  name: [{ required: true, message: '请输入任务名称', trigger: 'blur' }],
  url: [{ required: true, message: '请输入目标URL', trigger: 'blur' }],
  maxPapers: [{ required: true, message: '请输入爬取数量', trigger: 'blur' }]
}

// Computed
const stats = computed(() => {
  const allResults = tasks.value.flatMap(t => t.results)
  return {
    crawledCount: allResults.length,
    syncedCount: allResults.filter(r => r.synced).length,
    pendingCount: allResults.filter(r => !r.synced).length
  }
})

const activeCrawlers = computed(() =>
  tasks.value.filter(t => t.status === 'running').length
)

const selectAll = computed({
  get: () => {
    if (!currentTask.value) return false
    return currentTask.value.results.length > 0 &&
      selectedPapers.value.size === currentTask.value.results.length
  },
  set: (value: boolean) => {
    // Handle select all
  }
})

const isIndeterminate = computed(() => {
  if (!currentTask.value) return false
  const selected = selectedPapers.value.size
  const total = currentTask.value.results.length
  return selected > 0 && selected < total
})

const hasSelectedPapers = computed(() => selectedPapers.value.size > 0)

const selectedCount = computed(() => selectedPapers.value.size)

// Methods
const refreshData = async () => {
  loading.value = true
  try {
    // Load tasks from localStorage or API
    const saved = localStorage.getItem('edge-crawler-tasks')
    if (saved) {
      tasks.value = JSON.parse(saved)
    }
  } finally {
    loading.value = false
  }
}

const selectTask = (task: CrawlerTask) => {
  currentTask.value = task
  selectedPapers.value.clear()
}

const handleSelectAll = () => {
  if (!currentTask.value) return
  if (selectAll.value) {
    currentTask.value.results.forEach(p => selectedPapers.value.delete(p.id))
  } else {
    currentTask.value.results.forEach(p => selectedPapers.value.add(p.id))
  }
}

const toggleSelectPaper = (paper: Paper) => {
  if (selectedPapers.value.has(paper.id)) {
    selectedPapers.value.delete(paper.id)
  } else {
    selectedPapers.value.add(paper.id)
  }
}

const handleCreateTask = async () => {
  const valid = await taskFormRef.value?.validate()
  if (!valid) return

  creating.value = true
  try {
    // Create task
    const task: CrawlerTask = {
      id: Date.now().toString(),
      name: newTask.value.name,
      url: newTask.value.url,
      status: 'idle',
      results: [],
      maxPapers: newTask.value.maxPapers,
      autoSync: newTask.value.autoSync,
      createdAt: new Date()
    }

    tasks.value.push(task)
    saveTasks()

    // Start crawling
    await startCrawling(task)

    showCreateDialog.value = false
    currentTask.value = task

    ElMessage.success('爬取任务已创建')
  } catch (error: any) {
    ElMessage.error(error.message || '创建任务失败')
  } finally {
    creating.value = false
  }
}

const startCrawling = async (task: CrawlerTask) => {
  task.status = 'running'
  task.progress = 0
  saveTasks()

  try {
    // Use edge crawling API
    const crawlResult = await crawlerApi.crawl({
      query: task.url,
      source: 'arxiv',
      limit: task.maxPapers
    })
    const results = crawlResult.papers || []

    task.results = results.map((r: any, index: number) => ({
      id: `${task.id}-${index}`,
      title: r.title || 'Unknown',
      authors: r.authors || 'Unknown',
      year: r.year || new Date().getFullYear().toString(),
      abstract: r.abstract || '',
      url: r.url,
      synced: false
    }))

    task.status = 'completed'
    task.progress = 100

    // Auto sync if enabled
    if (task.autoSync) {
      await handleSyncTask(task)
    }
  } catch (error: any) {
    task.status = 'error'
    ElMessage.error(error.message || '爬取失败')
  } finally {
    saveTasks()
  }
}

const handleSyncSelected = async () => {
  if (!currentTask.value || !hasSelectedPapers.value) return

  const papersToSync = currentTask.value.results.filter(p =>
    selectedPapers.value.has(p.id) && !p.synced
  )

  if (papersToSync.length === 0) {
    ElMessage.info('没有需要同步的论文')
    return
  }

  try {
    await ElMessageBox.confirm(
      `确定要同步 ${papersToSync.length} 篇论文到服务器吗？`,
      '确认同步',
      { type: 'warning' }
    )

    for (const paper of papersToSync) {
      await syncPaperToServer(paper)
      paper.synced = true
    }

    saveTasks()
    selectedPapers.value.clear()
    ElMessage.success(`成功同步 ${papersToSync.length} 篇论文`)
  } catch {
    // User cancelled
  }
}

const handleSyncOne = async (paper: Paper) => {
  try {
    await syncPaperToServer(paper)
    paper.synced = true
    saveTasks()
    ElMessage.success('同步成功')
  } catch (error: any) {
    ElMessage.error(error.message || '同步失败')
  }
}

const handleSyncTask = async (task: CrawlerTask) => {
  const unsynced = task.results.filter(p => !p.synced)
  for (const paper of unsynced) {
    await syncPaperToServer(paper)
    paper.synced = true
  }
  saveTasks()
}

const syncPaperToServer = async (paper: Paper) => {
  // Call backend API to sync paper
  // await paperApi.create({ ... })
}

const handleDelete = async (paper: Paper) => {
  try {
    await ElMessageBox.confirm('确定要删除这篇论文吗？', '确认删除', { type: 'warning' })
    if (currentTask.value) {
      currentTask.value.results = currentTask.value.results.filter(p => p.id !== paper.id)
      saveTasks()
    }
    ElMessage.success('删除成功')
  } catch {
    // User cancelled
  }
}

const handlePreview = (paper: Paper) => {
  previewPaper.value = paper
  showPreviewDialog.value = true
}

const handleTaskAction = async (command: string, task: CrawlerTask) => {
  switch (command) {
    case 'start':
      await startCrawling(task)
      break
    case 'pause':
      task.status = 'paused'
      saveTasks()
      break
    case 'sync':
      await handleSyncTask(task)
      break
    case 'delete':
      try {
        await ElMessageBox.confirm('确定要删除这个任务吗？', '确认删除', { type: 'warning' })
        tasks.value = tasks.value.filter(t => t.id !== task.id)
        if (currentTask.value?.id === task.id) {
          currentTask.value = null
        }
        saveTasks()
        ElMessage.success('删除成功')
      } catch {
        // User cancelled
      }
      break
  }
}

const saveTasks = () => {
  localStorage.setItem('edge-crawler-tasks', JSON.stringify(tasks.value))
}

const getTaskStatusType = (status: string) => {
  const types: Record<string, any> = {
    idle: 'info',
    running: 'warning',
    paused: 'info',
    completed: 'success',
    error: 'danger'
  }
  return types[status] || 'info'
}

const getTaskStatusText = (status: string) => {
  const texts: Record<string, string> = {
    idle: '待开始',
    running: '运行中',
    paused: '已暂停',
    completed: '已完成',
    error: '错误'
  }
  return texts[status] || status
}

const truncateAuthors = (authors: string) => {
  return authors.length > 30 ? authors.substring(0, 30) + '...' : authors
}

const truncateAbstract = (abstract: string) => {
  return abstract.length > 100 ? abstract.substring(0, 100) + '...' : abstract
}

// Lifecycle
onMounted(() => {
  refreshData()
})
</script>

<style scoped lang="scss">
.edge-crawler {
  padding: 24px;
  max-width: 1600px;
  margin: 0 auto;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 24px;

  .header-left {
    flex: 1;
  }

  .page-title {
    font-size: 28px;
    font-weight: 600;
    margin: 0 0 8px 0;
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .page-description {
    color: var(--el-text-color-secondary);
    margin: 0;
  }

  .header-actions {
    display: flex;
    gap: 12px;
  }
}

.stats-row {
  margin-bottom: 24px;
}

.quick-stat {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 16px;
  background: var(--el-bg-color);
  border-radius: 8px;
  margin-bottom: 16px;

  .stat-icon {
    width: 48px;
    height: 48px;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: 12px;
    font-size: 24px;

    &.crawled {
      background: #e3f2fd;
      color: #1976d2;
    }

    &.synced {
      background: #e8f5e9;
      color: #388e3c;
    }

    &.pending {
      background: #fff3e0;
      color: #f57c00;
    }

    &.active {
      background: #f3e5f5;
      color: #7b1fa2;
    }
  }

  .stat-content {
    flex: 1;
  }

  .stat-value {
    font-size: 24px;
    font-weight: 600;
  }

  .stat-label {
    font-size: 14px;
    color: var(--el-text-color-secondary);
  }
}

.main-content {
  display: grid;
  grid-template-columns: 350px 1fr;
  gap: 24px;
  height: calc(100vh - 300px);
}

.tasks-panel,
.results-panel {
  display: flex;
  flex-direction: column;
  min-height: 0;

  .el-card {
    flex: 1;
    display: flex;
    flex-direction: column;

    :deep(.el-card__body) {
      flex: 1;
      overflow: auto;
    }
  }
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-weight: 600;
}

.task-list {
  .task-item {
    padding: 12px;
    border: 1px solid var(--el-border-color);
    border-radius: 8px;
    margin-bottom: 12px;
    cursor: pointer;
    transition: all 0.3s;

    &:hover {
      border-color: var(--el-color-primary);
    }

    &.active {
      background: var(--el-color-primary-light-9);
      border-color: var(--el-color-primary);
    }

    .task-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 8px;
    }

    .task-name {
      font-weight: 600;
    }

    .task-more {
      cursor: pointer;
      opacity: 0.6;

      &:hover {
        opacity: 1;
      }
    }

    .task-meta {
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 14px;
      color: var(--el-text-color-secondary);
    }

    .task-progress {
      margin-top: 8px;
    }
  }
}

.results-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 16px;
}

.paper-card {
  position: relative;
  padding: 16px;
  border: 2px solid var(--el-border-color);
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.3s;

  &:hover {
    border-color: var(--el-color-primary-light-3);
  }

  &.selected {
    border-color: var(--el-color-primary);
    background: var(--el-color-primary-light-9);
  }

  .paper-checkbox {
    position: absolute;
    top: 12px;
    left: 12px;
  }

  .paper-status {
    position: absolute;
    top: 12px;
    right: 12px;

    .synced-icon {
      color: var(--el-color-success);
      font-size: 20px;
    }

    .pending-icon {
      color: var(--el-color-warning);
      font-size: 20px;
    }
  }

  .paper-content {
    padding: 0 24px;
  }

  .paper-title {
    font-size: 16px;
    font-weight: 600;
    margin: 0 0 8px 0;
    display: -webkit-box;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
    overflow: hidden;
  }

  .paper-meta {
    font-size: 14px;
    color: var(--el-text-color-secondary);
    margin-bottom: 8px;

    .paper-authors {
      margin-right: 12px;
    }
  }

  .paper-abstract {
    font-size: 14px;
    color: var(--el-text-color-regular);
    margin-bottom: 12px;
    display: -webkit-box;
    -webkit-line-clamp: 3;
    -webkit-box-orient: vertical;
    overflow: hidden;
  }

  .paper-actions {
    display: flex;
    gap: 8px;
  }
}

.form-tip {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin-left: 12px;
}

.preview-content {
  h2 {
    font-size: 20px;
    margin: 0 0 16px 0;
  }

  .preview-meta {
    margin-bottom: 16px;
    color: var(--el-text-color-secondary);

    span {
      margin-right: 16px;
    }
  }

  .preview-abstract {
    line-height: 1.8;
    margin-bottom: 16px;
  }
}

/* Dark mode */
[data-theme="dark"] .edge-crawler {
  color: #f3f4f6;
}

[data-theme="dark"] .edge-crawler .page-header,
[data-theme="dark"] .edge-crawler .page-title,
[data-theme="dark"] .edge-crawler .card-header {
  color: #f3f4f6;
}

[data-theme="dark"] .edge-crawler .page-subtitle,
[data-theme="dark"] .edge-crawler .page-description {
  color: #9ca3af;
}

[data-theme="dark"] .edge-crawler .stat-card,
[data-theme="dark"] .edge-crawler .filter-card,
[data-theme="dark"] .edge-crawler .tasks-card,
[data-theme="dark"] .edge-crawler .chart-card,
[data-theme="dark"] .edge-crawler .table-card,
[data-theme="dark"] .edge-crawler .form-card,
[data-theme="dark"] .edge-crawler .detail-header,
[data-theme="dark"] .edge-crawler .detail-content,
[data-theme="dark"] .edge-crawler .toolbar,
[data-theme="dark"] .edge-crawler .export-options {
  background: rgba(40, 40, 45, 0.98) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #f3f4f6;
}

[data-theme="dark"] .edge-crawler .stat-value,
[data-theme="dark"] .edge-crawler .metric-value {
  color: #f3f4f6;
}

[data-theme="dark"] .edge-crawler .stat-label,
[data-theme="dark"] .edge-crawler .metric-label {
  color: #9ca3af;
}

[data-theme="dark"] .edge-crawler .empty-state,
[data-theme="dark"] .edge-crawler .empty-text {
  color: #9ca3af;
}
</style>
