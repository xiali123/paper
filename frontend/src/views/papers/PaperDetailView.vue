<template>
  <div class="paper-detail-view" role="main" aria-label="论文详情">
    <!-- Loading State -->
    <div v-if="loading" class="loading-container">
      <el-skeleton animated>
        <template #template>
          <el-skeleton-item variant="text" style="width: 60%; height: 40px; margin-bottom: 20px" />
          <el-skeleton-item variant="text" style="width: 40%; margin-bottom: 30px" />
          <el-skeleton-item variant="rect" style="width: 100%; height: 200px; margin-bottom: 20px" />
          <el-skeleton-item variant="text" style="width: 100%; margin-bottom: 10px" />
          <el-skeleton-item variant="text" style="width: 100%; margin-bottom: 10px" />
          <el-skeleton-item variant="text" style="width: 80%" />
        </template>
      </el-skeleton>
    </div>

    <!-- Error State -->
    <div v-else-if="error" class="error-container">
      <el-result icon="error" title="加载失败" :sub-title="error">
        <template #extra>
          <el-button type="primary" @click="handleRetry">重试</el-button>
          <el-button @click="handleBack">返回</el-button>
        </template>
      </el-result>
    </div>

    <!-- Paper Detail -->
    <div v-else-if="paper" class="detail-container">
      <!-- Header -->
      <div class="detail-header">
        <div class="header-left">
          <el-button :icon="ArrowLeft" @click="handleBack">返回</el-button>
        </div>
        <div class="header-right">
          <el-button-group>
            <el-tooltip content="编辑" placement="top">
              <el-button :icon="Edit" @click="handleEdit" />
            </el-tooltip>
            <el-tooltip :content="收藏" placement="top">
              <el-button
                :type="paper.isBookmarked ? 'warning' : 'default'"
                :icon="paper.isBookmarked ? StarFilled : Star"
                @click="handleToggleBookmark"
              />
            </el-tooltip>
            <el-tooltip content="导出" placement="top">
              <el-button :icon="Download" @click="handleExport" />
            </el-tooltip>
            <el-tooltip content="删除" placement="top">
              <el-button type="danger" :icon="Delete" @click="handleDelete" />
            </el-tooltip>
          </el-button-group>
        </div>
      </div>

      <!-- Main Content -->
      <div class="detail-content">
        <!-- Title Section -->
        <div class="title-section">
          <h1 class="paper-title">{{ paper.title }}</h1>
          <div class="paper-meta">
            <div class="meta-item" v-if="paper.authors">
              <el-icon><User /></el-icon>
              <span>{{ paper.authors }}</span>
            </div>
            <div class="meta-item" v-if="paper.publication">
              <el-icon><Document /></el-icon>
              <span>{{ paper.publication }}</span>
            </div>
            <div class="meta-item" v-if="paper.year">
              <el-icon><Calendar /></el-icon>
              <span>{{ paper.year }}</span>
            </div>
            <div class="meta-item" v-if="paper.volume || paper.issue || paper.pages">
              <el-icon><Document /></el-icon>
              <span>
                {{ paper.volume }}{{ paper.issue ? `(${paper.issue})` : '' }}{{ paper.pages ? `: ${paper.pages}` : '' }}
              </span>
            </div>
          </div>
        </div>

        <!-- Statistics -->
        <div class="stats-section">
          <div class="stat-item">
            <el-icon class="stat-icon" color="#409eff"><View /></el-icon>
            <span class="stat-label">引用</span>
            <span class="stat-value">{{ paper.citations || 0 }}</span>
          </div>
          <div class="stat-item">
            <el-icon class="stat-icon" color="#67c23a"><Download /></el-icon>
            <span class="stat-label">下载</span>
            <span class="stat-value">{{ paper.downloads || 0 }}</span>
          </div>
          <div class="stat-item">
            <el-icon class="stat-icon" :color="paper.isBookmarked ? '#f56c6c' : '#909399'">
              <component :is="paper.isBookmarked ? StarFilled : Star" />
            </el-icon>
            <span class="stat-label">收藏</span>
            <span class="stat-value">{{ paper.isBookmarked ? '已收藏' : '未收藏' }}</span>
          </div>
          <div class="stat-item">
            <el-icon class="stat-icon" :color="paper.isRead ? '#67c23a' : '#909399'">
              <component :is="paper.isRead ? CircleCheck : Circle" />
            </el-icon>
            <span class="stat-label">阅读</span>
            <span class="stat-value">{{ paper.isRead ? '已读' : '未读' }}</span>
          </div>
        </div>

        <!-- Reading Progress -->
        <div v-if="paper.readingProgress > 0" class="progress-section">
          <div class="progress-header">
            <span class="progress-label">阅读进度</span>
            <span class="progress-value">{{ paper.readingProgress }}%</span>
          </div>
          <el-progress :percentage="paper.readingProgress" :stroke-width="8" />
        </div>

        <!-- Tags -->
        <div v-if="paper.category || paper.tags || paper.source" class="tags-section">
          <el-tag v-if="paper.category" type="primary" size="large">{{ paper.category }}</el-tag>
          <el-tag
            v-for="(tag, index) in parseTags(paper.tags)"
            :key="index"
            size="large"
          >
            {{ tag }}
          </el-tag>
          <el-tag v-if="paper.source" type="info" size="large">
            {{ sourceLabels[paper.source] || paper.source }}
          </el-tag>
        </div>

        <!-- Abstract -->
        <div v-if="paper.abstract" class="abstract-section">
          <h3 class="section-title">摘要</h3>
          <div class="abstract-content">
            {{ paper.abstract }}
          </div>
        </div>

        <!-- DOI & URL -->
        <div v-if="paper.doi || paper.url" class="links-section">
          <h3 class="section-title">链接</h3>
          <div class="links-content">
            <div v-if="paper.doi" class="link-item">
              <span class="link-label">DOI:</span>
              <el-link :href="`https://doi.org/${paper.doi}`" target="_blank" type="primary">
                {{ paper.doi }}
              </el-link>
            </div>
            <div v-if="paper.url" class="link-item">
              <span class="link-label">URL:</span>
              <el-link :href="paper.url" target="_blank" type="primary">
                {{ paper.url }}
              </el-link>
            </div>
          </div>
        </div>

        <!-- Notes -->
        <div v-if="paper.notes" class="notes-section">
          <h3 class="section-title">笔记</h3>
          <div class="notes-content">
            {{ paper.notes }}
          </div>
        </div>

        <!-- AI Analysis (Placeholder) -->
        <div class="ai-analysis-section">
          <h3 class="section-title">
            <el-icon><MagicStick /></el-icon>
            AI 分析
          </h3>
          <el-empty description="AI 分析功能开发中..." :image-size="100" />
        </div>

        <!-- Related Papers (Placeholder) -->
        <div class="related-section">
          <h3 class="section-title">
            <el-icon><Connection /></el-icon>
            相关论文
          </h3>
          <el-empty description="相关论文推荐功能开发中..." :image-size="100" />
        </div>

        <!-- Citation Network -->
        <div class="citation-network-section">
          <h3 class="section-title">
            <el-icon><Connection /></el-icon>
            引文网络
          </h3>
          <CitationNetwork
            :center-paper="centerPaperForNetwork"
            :related-papers="mockRelatedPapers"
            :citations="mockCitations"
            @navigate-paper="handleNavigatePaper"
            @import-paper="handleImportPaper"
          />
        </div>

        <!-- Comments (Placeholder) -->
        <div class="comments-section">
          <h3 class="section-title">
            <el-icon><ChatDotRound /></el-icon>
            评论与讨论
          </h3>
          <el-empty description="评论功能开发中..." :image-size="100" />
        </div>
      </div>

      <!-- Sidebar -->
      <div class="detail-sidebar">
        <!-- Actions Card -->
        <el-card class="sidebar-card" shadow="never">
          <template #header>
            <span>快速操作</span>
          </template>
          <div class="action-buttons">
            <el-button type="primary" :icon="View" @click="handleMarkAsRead(!paper.isRead)" block>
              {{ paper.isRead ? '标记为未读' : '标记为已读' }}
            </el-button>
            <el-button :icon="Edit" @click="handleEdit" block>
              编辑论文
            </el-button>
            <el-button :icon="Download" @click="handleExport" block>
              导出引用
            </el-button>
          </div>
        </el-card>

        <!-- PDF Preview Card (Placeholder) -->
        <el-card class="sidebar-card" shadow="never">
          <template #header>
            <span>PDF 预览</span>
          </template>
          <el-empty description="PDF 预览功能开发中..." :image-size="80" />
        </el-card>

        <!-- Metadata Card -->
        <el-card class="sidebar-card" shadow="never">
          <template #header>
            <span>元数据</span>
          </template>
          <div class="metadata-list">
            <div class="metadata-item" v-if="paper.createdAt">
              <span class="metadata-label">创建时间</span>
              <span class="metadata-value">{{ formatDate(paper.createdAt) }}</span>
            </div>
            <div class="metadata-item" v-if="paper.updatedAt">
              <span class="metadata-label">更新时间</span>
              <span class="metadata-value">{{ formatDate(paper.updatedAt) }}</span>
            </div>
            <div class="metadata-item" v-if="paper.source">
              <span class="metadata-label">来源</span>
              <span class="metadata-value">{{ sourceLabels[paper.source] || paper.source }}</span>
            </div>
          </div>
        </el-card>
      </div>
    </div>

    <!-- Delete Confirmation Dialog -->
    <el-dialog
      v-model="deleteDialogVisible"
      title="确认删除"
      width="500px"
    >
      <span>确定要删除论文《{{ paper?.title }}》吗？此操作无法撤销。</span>
      <template #footer>
        <el-button @click="deleteDialogVisible = false">取消</el-button>
        <el-button type="danger" @click="confirmDelete">确认删除</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import CitationNetwork from '@/components/citation/CitationNetwork.vue'
import { useRoute, useRouter } from 'vue-router'
import { usePaperStore } from '@/stores/paperStore'
import { storeToRefs } from 'pinia'
import {
  ArrowLeft,
  Edit,
  Delete,
  Download,
  Star,
  StarFilled,
  View,
  User,
  Document,
  Calendar,
  CircleCheck,
  MagicStick,
  Connection,
  ChatDotRound
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'

const route = useRoute()
const router = useRouter()
const paperStore = usePaperStore()

const { currentPaper: paper, loading, error } = storeToRefs(paperStore)

// Dialog state
const deleteDialogVisible = ref(false)

// --- Citation Network mock data ---
const mockRelatedPapers = [
  { id: 'rp-1', title: 'Attention Is All You Need', authors: 'Vaswani A. et al.', year: 2017, citationCount: 92000 },
  { id: 'rp-2', title: 'BERT: Pre-training of Deep Bidirectional Transformers', authors: 'Devlin J. et al.', year: 2019, citationCount: 68000 },
  { id: 'rp-3', title: 'GPT-3: Language Models are Few-Shot Learners', authors: 'Brown T. et al.', year: 2020, citationCount: 45000 },
  { id: 'rp-4', title: 'Deep Residual Learning for Image Recognition', authors: 'He K. et al.', year: 2016, citationCount: 120000 },
  { id: 'rp-5', title: 'ImageNet Classification with Deep Convolutional Networks', authors: 'Krizhevsky A. et al.', year: 2012, citationCount: 85000 },
  { id: 'rp-6', title: 'A Survey on Transfer Learning', authors: 'Pan S. J., Yang Q.', year: 2010, citationCount: 15000 },
  { id: 'rp-7', title: 'Generative Adversarial Nets', authors: 'Goodfellow I. et al.', year: 2014, citationCount: 55000 },
]

const mockCitations = [
  { source: 'rp-1', target: 'rp-5', type: 'cites' as const },
  { source: 'rp-2', target: 'rp-1', type: 'cites' as const },
  { source: 'rp-3', target: 'rp-1', type: 'cites' as const },
  { source: 'rp-3', target: 'rp-2', type: 'cites' as const },
  { source: 'rp-1', target: 'rp-6', type: 'cites' as const },
  { source: 'rp-7', target: 'rp-4', type: 'cites' as const },
  { source: 'rp-4', target: 'rp-5', type: 'cites' as const },
  { source: 'rp-2', target: 'rp-6', type: 'cites' as const },
]

const centerPaperForNetwork = computed(() => {
  if (!paper.value) return { id: '', title: '', authors: '', year: 2024, citationCount: 0 }
  return {
    id: String(paper.value.id),
    title: paper.value.title || '',
    authors: paper.value.authors || '',
    year: paper.value.year || 2024,
    citationCount: paper.value.citations || 0,
  }
})

const handleNavigatePaper = (paperId: string) => {
  ElMessage.info(`导航到论文 ${paperId}（功能开发中）`)
}

const handleImportPaper = (paperId: string) => {
  ElMessage.success(`论文 ${paperId} 已成功导入收藏`)
}

// Source labels
const sourceLabels: Record<string, string> = {
  manual: '手动添加',
  cnki: '知网',
  ieee: 'IEEE',
  arxiv: 'ArXiv',
  pubmed: 'PubMed'
}

// Parse tags
const parseTags = (tags: string | undefined): string[] => {
  if (!tags) return []
  return tags.split(',').map(tag => tag.trim()).filter(tag => tag)
}

// Format date
const formatDate = (timestamp: number | string): string => {
  const date = new Date(timestamp)
  return date.toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit'
  })
}

// Fetch paper
const fetchPaper = async () => {
  const idParam = route.params.id
  const id = Number(idParam)

  // 验证ID
  if (!idParam || Number.isNaN(id) || id <= 0) {
    error.value = '论文ID格式无效，请检查URL或从论文列表重新进入'
    console.error('Invalid paper ID:', idParam)
    return
  }

  try {
    await paperStore.fetchPaper(id)
  } catch (err: any) {
    error.value = err.message || '加载论文详情失败，请稍后重试'
    console.error('Failed to fetch paper:', err)
  }
}

// Handle actions
const handleBack = () => {
  router.back()
}

const handleRetry = () => {
  fetchPaper()
}

const handleEdit = () => {
  router.push(`/papers/${paper.value?.id}/edit`)
}

const handleToggleBookmark = async () => {
  if (!paper.value) return

  try {
    await paperStore.toggleBookmark(paper.value.id)
    ElMessage.success(paper.value.isBookmarked ? '已添加到收藏' : '已取消收藏')
  } catch (error: any) {
    ElMessage.error(error.message || '操作失败')
  }
}

const handleMarkAsRead = async (isRead: boolean) => {
  if (!paper.value) return

  try {
    await paperStore.markAsRead(paper.value.id, isRead)
    ElMessage.success(isRead ? '已标记为已读' : '已标记为未读')
  } catch (error: any) {
    ElMessage.error(error.message || '操作失败')
  }
}

const handleExport = () => {
  // TODO: Implement export functionality
  ElMessage.info('导出功能开发中...')
}

const handleDelete = () => {
  deleteDialogVisible.value = true
}

const confirmDelete = async () => {
  if (!paper.value) return

  try {
    await paperStore.deletePaper(paper.value.id)
    ElMessage.success('删除成功')
    router.push('/papers')
  } catch (error: any) {
    ElMessage.error(error.message || '删除失败')
  }
}

// Lifecycle
onMounted(() => {
  fetchPaper()
})
</script>

<style scoped lang="scss">
.paper-detail-view {
  width: 100%;
  max-width: 1600px;
  margin: 0 auto;
  padding: 24px;
}

.loading-container,
.error-container {
  padding: 60px 0;
}

.detail-container {
  display: grid;
  grid-template-columns: 1fr 280px;
  gap: 24px;
  align-items: start;
}

.detail-header {
  grid-column: 1 / -1;
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 32px;
  padding: 28px 40px;
  background: white;
  border-radius: 16px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.08);
}

.header-left,
.header-right {
  display: flex;
  gap: 16px;
  align-items: center;
}

.detail-content {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.title-section {
  background: white;
  padding: 40px 48px;
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.paper-title {
  margin: 0 0 24px 0;
  font-size: 36px;
  font-weight: 700;
  line-height: 1.2;
  color: #303133;
}

.paper-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
}

.meta-item {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 14px;
  color: #606266;

  .el-icon {
    flex-shrink: 0;
  }
}

.stats-section {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 24px;
  padding: 32px 40px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.stat-item {
  display: flex;
  align-items: center;
  gap: 12px;
}

.stat-icon {
  font-size: 24px;
}

.stat-label {
  font-size: 14px;
  color: #909399;
}

.stat-value {
  font-size: 18px;
  font-weight: 600;
  color: #303133;
}

.progress-section {
  padding: 24px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.progress-header {
  display: flex;
  justify-content: space-between;
  margin-bottom: 12px;
}

.progress-label {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
}

.progress-value {
  font-size: 14px;
  color: #409eff;
  font-weight: 600;
}

.tags-section {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  padding: 24px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.section-title {
  margin: 0 0 24px 0;
  font-size: 22px;
  font-weight: 600;
  color: #303133;
  display: flex;
  align-items: center;
  gap: 8px;
}

.abstract-section,
.links-section,
.notes-section,
.ai-analysis-section,
.related-section,
.citation-network-section,
.comments-section {
  padding: 32px 40px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.abstract-content,
.notes-content {
  font-size: 15px;
  line-height: 1.8;
  color: #606266;
  white-space: pre-wrap;
}

.links-content {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.link-item {
  display: flex;
  align-items: center;
  gap: 8px;
}

.link-label {
  font-size: 14px;
  font-weight: 600;
  color: #909399;
  min-width: 50px;
}

.detail-sidebar {
  display: flex;
  flex-direction: column;
  gap: 20px;
  position: sticky;
  top: 24px;
}

.sidebar-card {
  :deep(.el-card__header) {
    padding: 20px 24px;
    font-weight: 600;
    font-size: 16px;
    border-bottom: 1px solid #e4e7ed;
  }

  :deep(.el-card__body) {
    padding: 24px;
  }
}

.action-buttons {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.metadata-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.metadata-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 14px;
}

.metadata-label {
  color: #909399;
}

.metadata-value {
  color: #303133;
  font-weight: 500;
  text-align: right;
}

@media (max-width: 1024px) {
  .detail-container {
    grid-template-columns: 1fr;
  }

  .detail-sidebar {
    position: static;
  }
}

@media (max-width: 768px) {
  .paper-detail-view {
    padding: 16px;
  }

  .detail-header {
    flex-direction: column;
    gap: 12px;
  }

  .paper-title {
    font-size: 22px;
  }

  .stats-section {
    grid-template-columns: repeat(2, 1fr);
  }

  .paper-meta {
    flex-direction: column;
    gap: 8px;
  }
}

/* Dark mode */
[data-theme="dark"] .paper-detail-view {
  color: #f3f4f6;
}

[data-theme="dark"] .paper-detail-view .page-header,
[data-theme="dark"] .paper-detail-view .page-title,
[data-theme="dark"] .paper-detail-view .card-header {
  color: #f3f4f6;
}

[data-theme="dark"] .paper-detail-view .page-subtitle,
[data-theme="dark"] .paper-detail-view .page-description {
  color: #9ca3af;
}

[data-theme="dark"] .paper-detail-view .stat-card,
[data-theme="dark"] .paper-detail-view .filter-card,
[data-theme="dark"] .paper-detail-view .tasks-card,
[data-theme="dark"] .paper-detail-view .chart-card,
[data-theme="dark"] .paper-detail-view .table-card,
[data-theme="dark"] .paper-detail-view .form-card,
[data-theme="dark"] .paper-detail-view .detail-header,
[data-theme="dark"] .paper-detail-view .detail-content,
[data-theme="dark"] .paper-detail-view .toolbar,
[data-theme="dark"] .paper-detail-view .export-options {
  background: rgba(40, 40, 45, 0.98) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #f3f4f6;
}

[data-theme="dark"] .paper-detail-view .stat-value,
[data-theme="dark"] .paper-detail-view .metric-value {
  color: #f3f4f6;
}

[data-theme="dark"] .paper-detail-view .stat-label,
[data-theme="dark"] .paper-detail-view .metric-label {
  color: #9ca3af;
}

[data-theme="dark"] .paper-detail-view .empty-state,
[data-theme="dark"] .paper-detail-view .empty-text {
  color: #9ca3af;
}
</style>
