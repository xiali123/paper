<template>
  <div class="writing-editor-view">
    <!-- 页面头部 -->
    <div class="page-header">
      <div class="header-left">
        <el-button :icon="ArrowLeft" @click="handleBack">返回</el-button>
        <el-input
          v-if="currentDocument"
          v-model="editTitle"
          class="title-input"
          placeholder="文档标题"
          @blur="handleTitleChange"
        />
        <el-skeleton v-else animated :rows="1" style="width: 300px" />
      </div>
      <div class="header-right">
        <el-button type="success" :icon="MagicStick" :loading="suggestionsLoading" @click="handleGenerateSuggestion">
          AI 建议
        </el-button>
        <el-button type="primary" :loading="saving" @click="handleSave">
          保存
        </el-button>
      </div>
    </div>

    <!-- 加载状态 -->
    <div v-if="loading" class="loading-container">
      <el-skeleton animated :rows="15" />
    </div>

    <!-- 主体内容 - 左右分屏布局 -->
    <div v-else class="editor-body split-layout">
      <!-- 左侧：LaTeX 编辑器 (50%) -->
      <div class="editor-panel">
        <div class="panel-header">
          <span class="panel-title">LaTeX 编辑器</span>
          <div class="panel-actions">
            <el-button
              size="small"
              :icon="ChatDotRound"
              @click="visiblePanels.tools = !visiblePanels.tools"
              :type="visiblePanels.tools ? 'primary' : ''"
            >
              工具
            </el-button>
          </div>
        </div>
        <LatexEditor
          v-model="editContent"
          @change="handleContentChange"
          class="latex-editor-full"
        />
        <div class="editor-status-bar">
          <span>字数：{{ wordCount }}</span>
          <span>{{ autoSaveStatus }}</span>
        </div>
      </div>

      <!-- 分隔条 -->
      <div class="resizer" @mousedown="startResize"></div>

      <!-- 右侧：KaTeX 预览 (50%) -->
      <div class="preview-panel" :style="{ width: previewWidth + '%' }">
        <div class="panel-header">
          <span class="panel-title">预览</span>
          <div class="panel-actions">
            <el-button
              size="small"
              @click="togglePanel('comment')"
              :type="visiblePanels.comment ? 'primary' : ''"
            >
              评论 {{ comments.length > 0 ? `(${comments.length})` : '' }}
            </el-button>
          </div>
        </div>
        <LatexPreview
          :content="editContent"
          class="preview-full"
        />
      </div>

      <!-- 浮动工具面板 (抽屉) -->
      <el-drawer
        v-model="visiblePanels.tools"
        direction="ltr"
        :size="320"
        title="工具面板"
      >
        <el-tabs>
          <el-tab-pane label="AI 建议">
            <AiSuggestionPanel
              :suggestions="suggestions"
              :loading="suggestionsLoading"
              @accept="handleAcceptSuggestion"
              @reject="handleRejectSuggestion"
              @generate="handleGenerateSuggestion"
            />
          </el-tab-pane>
          <el-tab-pane label="版本历史">
            <VersionHistory
              :versions="versions"
              :loading="versionsLoading"
              @create="handleCreateVersion"
            />
          </el-tab-pane>
          <el-tab-pane label="评论">
            <CommentPanel
              :comments="comments"
              :loading="commentsLoading"
              @add="handleAddComment"
              @resolve="handleResolveComment"
            />
          </el-tab-pane>
        </el-tabs>
      </el-drawer>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useWritingStore } from '@/stores/writingStore'
import { useAuthStore } from '@/stores/authStore'
import { storeToRefs } from 'pinia'
import type { WritingSuggestion } from '@/types/collaborative'
import CommentPanel from '@/components/writing/CommentPanel.vue'
import AiSuggestionPanel from '@/components/writing/AiSuggestionPanel.vue'
import VersionHistory from '@/components/writing/VersionHistory.vue'
import LatexEditor from '@/components/latex/LatexEditor.vue'
import LatexPreview from '@/components/latex/LatexPreview.vue'
import { ArrowLeft, ChatDotRound, MagicStick } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

const route = useRoute()
const router = useRouter()
const writingStore = useWritingStore()
const authStore = useAuthStore()
const {
  currentDocument,
  loading,
  saving,
  suggestions,
  suggestionsLoading,
  versions,
  versionsLoading,
  comments,
  commentsLoading
} = storeToRefs(writingStore)

const editTitle = ref('')
const editContent = ref('')
const autoSaveStatus = ref('就绪')

// 分屏布局状态
const previewWidth = ref(50)  // 预览宽度百分比
const isResizing = ref(false)

// 面板可见性控制
const visiblePanels = ref({
  comment: false,
  preview: true,
  suggestion: false,
  version: false,
  tools: false  // 工具面板（抽屉）
})

// 面板切换逻辑
function togglePanel(panel: 'comment' | 'preview' | 'suggestion' | 'version') {
  if (panel === 'comment') {
    visiblePanels.value.comment = !visiblePanels.value.comment
  } else if (panel === 'preview') {
    visiblePanels.value.preview = !visiblePanels.value.preview
  } else {
    // 建议和版本互斥
    if (panel === 'suggestion') {
      visiblePanels.value.suggestion = !visiblePanels.value.suggestion
      if (visiblePanels.value.suggestion) visiblePanels.value.version = false
    } else {
      visiblePanels.value.version = !visiblePanels.value.version
      if (visiblePanels.value.version) visiblePanels.value.suggestion = false
    }
  }
}

// 拖拽调整大小
function startResize(e: MouseEvent) {
  isResizing.value = true
  document.addEventListener('mousemove', onResize)
  document.addEventListener('mouseup', stopResize)
}

function onResize(e: MouseEvent) {
  if (!isResizing.value) return

  const container = document.querySelector('.editor-body') as HTMLElement
  if (!container) return

  const rect = container.getBoundingClientRect()
  const percentage = ((e.clientX - rect.left) / rect.width) * 100

  // 限制在 20% - 80% 之间
  previewWidth.value = Math.max(20, Math.min(80, 100 - percentage))
}

function stopResize() {
  isResizing.value = false
  document.removeEventListener('mousemove', onResize)
  document.removeEventListener('mouseup', stopResize)
}

const wordCount = computed(() => editContent.value.length)

// 自动保存（防抖）
let saveTimer: ReturnType<typeof setTimeout> | null = null

function handleContentChange() {
  autoSaveStatus.value = '编辑中...'
  if (saveTimer) clearTimeout(saveTimer)
  saveTimer = setTimeout(() => autoSave(), 3000)
}

async function autoSave() {
  if (!currentDocument.value) return
  autoSaveStatus.value = '保存中...'
  try {
    await writingStore.saveDocument(currentDocument.value.id, {
      content: editContent.value
    })
    autoSaveStatus.value = '已自动保存'
  } catch {
    autoSaveStatus.value = '保存失败'
  }
}

async function handleSave() {
  if (!currentDocument.value) return
  try {
    await writingStore.saveDocument(currentDocument.value.id, {
      title: editTitle.value,
      content: editContent.value
    })
    ElMessage.success('保存成功')
  } catch {
    ElMessage.error('保存失败')
  }
}

async function handleTitleChange() {
  if (!currentDocument.value) return
  try {
    await writingStore.saveDocument(currentDocument.value.id, { title: editTitle.value })
  } catch {
    // 静默失败
  }
}

// AI 建议
async function handleGenerateSuggestion() {
  if (!currentDocument.value) return
  try {
    await writingStore.generateSuggestion(currentDocument.value.id, 'content', authStore.user?.id)
    visiblePanels.value.suggestion = true
    visiblePanels.value.version = false
    ElMessage.success('已生成新建议')
  } catch {
    ElMessage.error('生成建议失败')
  }
}

async function handleAcceptSuggestion(suggestionId: number) {
  try {
    const suggestion = suggestions.value.find((s: WritingSuggestion) => s.id === suggestionId)
    await writingStore.acceptSuggestion(suggestionId)
    if (suggestion) {
      const before = editContent.value.substring(0, suggestion.position_start)
      const after = editContent.value.substring(suggestion.position_end)
      editContent.value = before + suggestion.suggested_text + after
      handleContentChange()
    }
    ElMessage.success('已接受建议')
  } catch {
    ElMessage.error('操作失败')
  }
}

async function handleRejectSuggestion(suggestionId: number) {
  try {
    await writingStore.rejectSuggestion(suggestionId)
    ElMessage.info('已忽略建议')
  } catch {
    ElMessage.error('操作失败')
  }
}

// 评论
async function handleAddComment(data: { content: string }) {
  if (!currentDocument.value) return
  try {
    await writingStore.addComment(currentDocument.value.id, {
      ...data,
      user_id: authStore.user?.id
    })
    ElMessage.success('评论已添加')
  } catch {
    ElMessage.error('添加评论失败')
  }
}

async function handleResolveComment(commentId: number) {
  try {
    await writingStore.resolveComment(commentId)
    ElMessage.success('评论已解决')
  } catch {
    ElMessage.error('操作失败')
  }
}

// 版本
async function handleCreateVersion(description?: string) {
  if (!currentDocument.value) return
  try {
    await writingStore.createVersion(currentDocument.value.id, description)
    ElMessage.success('版本已保存')
  } catch {
    ElMessage.error('创建版本失败')
  }
}

// 导航
function handleBack() {
  router.push('/writing')
}

// 初始化
onMounted(async () => {
  const id = Number(route.params.id)
  if (isNaN(id)) {
    router.push('/writing')
    return
  }

  try {
    const doc = await writingStore.fetchDocument(id)
    editTitle.value = doc.title
    editContent.value = doc.content

    // 并行加载子数据
    writingStore.fetchSuggestions(id)
    writingStore.fetchVersions(id)
    writingStore.fetchComments(id)
  } catch {
    ElMessage.error('加载文档失败')
    router.push('/writing')
  }
})

onUnmounted(() => {
  writingStore.clearCurrentDocument()
  if (saveTimer) clearTimeout(saveTimer)
})
</script>

<style scoped lang="scss">
.writing-editor-view {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 20px;
  border-bottom: 1px solid var(--el-border-color-lighter);
  background: var(--el-bg-color);
}

.header-left {
  display: flex;
  align-items: center;
  gap: 12px;
}

.title-input {
  width: 400px;

  :deep(.el-input__wrapper) {
    border: none;
    background: transparent;
    box-shadow: none;

    input {
      font-size: 18px;
      font-weight: 600;
    }
  }
}

.header-right {
  display: flex;
  gap: 8px;
}

.loading-container {
  flex: 1;
  padding: 40px;
}

// 左右分屏布局
.editor-body {
  flex: 1;
  display: flex;
  overflow: hidden;

  &.split-layout {
    position: relative;
  }
}

.editor-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
  background: var(--el-bg-color);
  border-right: 1px solid var(--el-border-color-lighter);
}

.preview-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
  background: var(--el-bg-color-page);
  transition: width 0.1s ease;
}

// 拖拽分隔条
.resizer {
  width: 4px;
  background: var(--el-border-color);
  cursor: col-resize;
  flex-shrink: 0;
  transition: background 0.2s;
  position: relative;
  z-index: 10;

  &:hover {
    background: var(--el-color-primary);
  }

  &::after {
    content: '';
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    width: 20px;
    height: 40px;
    background: var(--el-color-primary);
    border-radius: 2px;
    opacity: 0;
    transition: opacity 0.2s;
  }

  &:hover::after {
    opacity: 0.2;
  }
}

// 面板头部
.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  border-bottom: 1px solid var(--el-border-color-lighter);
  background: var(--el-bg-color);
}

.panel-title {
  font-size: 13px;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.panel-actions {
  display: flex;
  gap: 4px;
}

// 全高编辑器和预览
.latex-editor-full,
.preview-full {
  flex: 1;
  min-height: 0;
}

// 移除旧的面板样式
.side-panel {
  width: 320px;
  border-left: 1px solid var(--el-border-color-lighter);
  border-right: 1px solid var(--el-border-color-lighter);
  background: var(--el-bg-color-page);

  &--left {
    border-right: none;
  }

  &--right {
    border-left: none;
  }
}

.editor-status-bar {
  display: flex;
  justify-content: space-between;
  padding: 8px 16px;
  border-top: 1px solid var(--el-border-color-lighter);
  font-size: 12px;
  color: var(--el-text-color-secondary);
}

.side-panel {
  width: 320px;
  border-left: 1px solid var(--el-border-color-lighter);
  border-right: 1px solid var(--el-border-color-lighter);
  background: var(--el-bg-color-page);

  &--left {
    border-right: none;
  }

  &--right {
    border-left: none;
  }
}
</style>
