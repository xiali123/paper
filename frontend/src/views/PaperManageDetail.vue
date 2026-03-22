<template>
  <div class="paper-manage-detail-container" v-loading="paperStore.loading">
    <!-- 错误状态 -->
    <el-empty
      v-if="!paperStore.loading && !paperStore.currentPaper"
      :description="$t('papers.notFound') || '论文不存在'"
    >
      <el-button type="primary" @click="$router.back()">
        {{ $t('common.back') || '返回' }}
      </el-button>
    </el-empty>

    <template v-else-if="paperStore.currentPaper">
      <!-- 页面头部 -->
      <div class="page-header">
        <div class="header-left">
          <el-button :icon="ArrowLeft" @click="$router.back()">
            {{ $t('common.back') || '返回' }}
          </el-button>
        </div>
        <div class="header-right">
          <el-button
            :type="paper.isBookmarked ? 'warning' : 'default'"
            :icon="paper.isBookmarked ? StarFilled : Star"
            @click="handleToggleBookmark"
          >
            {{ paper.isBookmarked ? ($t('papers.bookmarked') || '已收藏') : ($t('papers.bookmark') || '收藏') }}
          </el-button>
          <el-button
            type="primary"
            :icon="Edit"
            @click="showEditDialog = true"
          >
            {{ $t('papers.edit') || '编辑' }}
          </el-button>
          <el-button
            type="danger"
            :icon="Delete"
            @click="handleDelete"
          >
            {{ $t('papers.delete') || '删除' }}
          </el-button>
        </div>
      </div>

      <!-- 论文信息 -->
      <div class="paper-info-section">
        <el-card>
          <template #header>
            <div class="card-header">
              <span class="card-title">{{ $t('papers.paperInfo') || '论文信息' }}</span>
              <div class="paper-meta">
                <el-tag v-if="paper.category" type="primary" size="small">
                  {{ paper.category }}
                </el-tag>
                <el-tag type="info" size="small">
                  {{ sourceLabels[paper.source] || paper.source }}
                </el-tag>
                <el-tag v-if="paper.createdAt" size="small">
                  {{ $t('papers.addedAt') || '添加于' }} {{ formatDate(paper.createdAt) }}
                </el-tag>
              </div>
            </div>
          </template>

          <!-- 标题 -->
          <h1 class="paper-title">{{ paper.title }}</h1>

          <!-- 作者 -->
          <div class="info-row" v-if="paper.authors">
            <span class="info-label">
              <el-icon><User /></el-icon>
              {{ $t('papers.authors') || '作者' }}:
            </span>
            <span class="info-value">{{ paper.authors }}</span>
          </div>

          <!-- 发表信息 -->
          <div class="info-row" v-if="paper.publication || paper.year">
            <span class="info-label">
              <el-icon><Document /></el-icon>
              {{ $t('papers.publication') || '出版物' }}:
            </span>
            <span class="info-value">
              {{ paper.publication }}{{ paper.year ? ` (${paper.year})` : '' }}
            </span>
          </div>

          <!-- DOI -->
          <div class="info-row" v-if="paper.doi">
            <span class="info-label">DOI:</span>
            <span class="info-value">
              <el-link :href="`https://doi.org/${paper.doi}`" target="_blank" type="primary">
                {{ paper.doi }}
              </el-link>
            </span>
          </div>

          <!-- URL -->
          <div class="info-row" v-if="paper.url">
            <span class="info-label">URL:</span>
            <span class="info-value">
              <el-link :href="paper.url" target="_blank" type="primary">
                {{ paper.url }}
              </el-link>
            </span>
          </div>

          <!-- 摘要 -->
          <div class="info-section" v-if="paper.abstract">
            <h3>{{ $t('papers.abstract') || '摘要' }}</h3>
            <p class="abstract-text">{{ paper.abstract }}</p>
          </div>

          <!-- 关键词 -->
          <div class="info-section" v-if="paper.keywords">
            <h3>{{ $t('papers.keywords') || '关键词' }}</h3>
            <div class="keywords-list">
              <el-tag
                v-for="(keyword, index) in paper.keywords.split(',')"
                :key="index"
                class="keyword-tag"
              >
                {{ keyword.trim() }}
              </el-tag>
            </div>
          </div>

          <!-- 标签 -->
          <div class="info-section" v-if="paper.tags">
            <h3>{{ $t('papers.tags') || '标签' }}</h3>
            <div class="tags-list">
              <el-tag
                v-for="(tag, index) in paper.tags.split(',')"
                :key="index"
                type="info"
                class="tag-item"
              >
                {{ tag.trim() }}
              </el-tag>
            </div>
          </div>

          <!-- 阅读进度 -->
          <div class="info-section" v-if="paper.readingProgress > 0">
            <h3>{{ $t('papers.readingProgress') || '阅读进度' }}</h3>
            <el-progress
              :percentage="paper.readingProgress"
              :stroke-width="20"
            />
          </div>
        </el-card>
      </div>

      <!-- 笔记区域 -->
      <div class="notes-section">
        <el-card class="notes-card">
          <template #header>
            <div class="card-header">
              <span class="card-title">
                <el-icon><EditPen /></el-icon>
                {{ $t('papers.notes') || '笔记' }}
              </span>
              <el-button
                size="small"
                type="primary"
                @click="isEditingNotes = true"
                v-if="!isEditingNotes"
              >
                {{ $t('papers.editNotes') || '编辑笔记' }}
              </el-button>
            </div>
          </template>

          <div v-if="isEditingNotes" class="notes-editor">
            <el-input
              v-model="notesText"
              type="textarea"
              :rows="15"
              :placeholder="$t('papers.notesPlaceholder') || '在这里添加你的笔记...'"
              maxlength="5000"
              show-word-limit
            />
            <div class="notes-actions">
              <el-button @click="cancelEditNotes">
                {{ $t('common.cancel') || '取消' }}
              </el-button>
              <el-button
                type="primary"
                :loading="savingNotes"
                @click="saveNotes"
              >
                {{ $t('common.save') || '保存' }}
              </el-button>
            </div>
          </div>

          <div v-else class="notes-display">
            <p v-if="paper.notes">{{ paper.notes }}</p>
            <el-empty
              v-else
              :description="$t('papers.noNotes') || '还没有笔记'"
              :image-size="80"
            />
          </div>
        </el-card>
      </div>
    </template>

    <!-- 编辑对话框 -->
    <paper-form-dialog
      v-model="showEditDialog"
      :paper="paper"
      @save="handleUpdatePaper"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  ArrowLeft,
  Star,
  StarFilled,
  Edit,
  Delete,
  User,
  Document,
  EditPen
} from '@element-plus/icons-vue'
import { usePaperManagementStore } from '@/stores/paperManagement'
import PaperFormDialog from '@/components/paper/PaperFormDialog.vue'

const { t } = useI18n()
const route = useRoute()
const router = useRouter()
const paperStore = usePaperManagementStore()

// 显示编辑对话框
const showEditDialog = ref(false)

// 是否正在编辑笔记
const isEditingNotes = ref(false)

// 笔记文本
const notesText = ref('')

// 保存笔记中
const savingNotes = ref(false)

// 当前论文
const paper = computed(() => paperStore.currentPaper!)

// 来源标签映射
const sourceLabels: Record<string, string> = {
  manual: '手动添加',
  cnki: '知网',
  ieee: 'IEEE',
  arxiv: 'ArXiv',
  pubmed: 'PubMed'
}

// 初始化
onMounted(async () => {
  const paperId = Number(route.params.id)
  if (paperId) {
    const result = await paperStore.fetchPaper(paperId)
    if (result.success && paper.value) {
      notesText.value = paper.value.notes || ''
    } else if (result.error) {
      ElMessage.error(result.error)
    }
  }
})

// 格式化日期
function formatDate(dateStr: string): string {
  if (!dateStr) return ''
  const date = new Date(dateStr)
  return date.toLocaleDateString()
}

// 切换收藏
async function handleToggleBookmark() {
  if (!paper.value) return

  const result = await paperStore.toggleBookmark(paper.value.id)
  if (result.success) {
    ElMessage.success(
      paper.value.isBookmarked
        ? (t('papers.bookmarked') || '已收藏')
        : (t('papers.unbookmarked') || '已取消收藏')
    )
  } else if (result.error) {
    ElMessage.error(result.error)
  }
}

// 删除论文
async function handleDelete() {
  if (!paper.value) return

  try {
    await ElMessageBox.confirm(
      t('papers.deleteConfirm') || `确定要删除《${paper.value.title}》吗？`,
      t('common.warning') || '警告',
      {
        confirmButtonText: t('common.confirm') || '确定',
        cancelButtonText: t('common.cancel') || '取消',
        type: 'warning'
      }
    )

    const result = await paperStore.deletePaper(paper.value.id)
    if (result.success) {
      ElMessage.success(t('papers.deleteSuccess') || '删除成功')
      router.back()
    } else if (result.error) {
      ElMessage.error(result.error)
    }
  } catch (error) {
    // 用户取消删除
  }
}

// 更新论文
async function handleUpdatePaper(data: any) {
  if (!paper.value) return

  const result = await paperStore.updatePaper(paper.value.id, data)
  if (result.success) {
    ElMessage.success(t('papers.updateSuccess') || '更新成功')
    showEditDialog.value = false
  } else if (result.error) {
    ElMessage.error(result.error)
  }
}

// 保存笔记
async function saveNotes() {
  if (!paper.value) return

  savingNotes.value = true

  const result = await paperStore.updatePaper(paper.value.id, {
    notes: notesText.value
  })

  savingNotes.value = false

  if (result.success) {
    ElMessage.success(t('papers.notesSaved') || '笔记保存成功')
    isEditingNotes.value = false
  } else if (result.error) {
    ElMessage.error(result.error)
  }
}

// 取消编辑笔记
function cancelEditNotes() {
  notesText.value = paper.value?.notes || ''
  isEditingNotes.value = false
}
</script>

<style scoped lang="scss">
.paper-manage-detail-container {
  padding: 24px;
  background: #f5f7fa;
  min-height: 100vh;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
  padding: 16px 20px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.paper-info-section,
.notes-section {
  margin-bottom: 16px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.card-title {
  font-size: 18px;
  font-weight: 600;
  color: #303133;
  display: flex;
  align-items: center;
  gap: 8px;
}

.paper-meta {
  display: flex;
  gap: 8px;
}

.paper-title {
  margin: 0 0 24px 0;
  font-size: 28px;
  font-weight: 700;
  color: #303133;
  line-height: 1.4;
}

.info-row {
  display: flex;
  align-items: flex-start;
  margin-bottom: 16px;
  font-size: 14px;

  .info-label {
    min-width: 80px;
    font-weight: 500;
    color: #606266;
    display: flex;
    align-items: center;
    gap: 6px;
  }

  .info-value {
    flex: 1;
    color: #303133;
    word-break: break-word;
  }
}

.info-section {
  margin-top: 24px;
  padding-top: 24px;
  border-top: 1px solid #e4e7ed;

  h3 {
    margin: 0 0 12px 0;
    font-size: 16px;
    font-weight: 600;
    color: #303133;
  }
}

.abstract-text {
  margin: 0;
  font-size: 14px;
  line-height: 1.8;
  color: #606266;
  text-align: justify;
}

.keywords-list,
.tags-list {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.notes-card {
  min-height: 400px;

  :deep(.el-card__body) {
    min-height: 340px;
  }
}

.notes-editor {
  .notes-actions {
    display: flex;
    justify-content: flex-end;
    gap: 12px;
    margin-top: 16px;
  }
}

.notes-display {
  p {
    font-size: 14px;
    line-height: 1.8;
    color: #606266;
    white-space: pre-wrap;
    word-break: break-word;
  }
}

@media (max-width: 768px) {
  .paper-manage-detail-container {
    padding: 16px;
  }

  .page-header {
    flex-direction: column;
    gap: 12px;
    align-items: stretch;
  }

  .paper-title {
    font-size: 22px;
  }

  .info-row {
    flex-direction: column;
    gap: 4px;

    .info-label {
      min-width: auto;
    }
  }
}
</style>
