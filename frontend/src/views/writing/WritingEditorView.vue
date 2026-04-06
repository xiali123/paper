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
        <el-button :icon="ChatDotRound" @click="togglePanel('comment')">
          评论 {{ comments.length > 0 ? `(${comments.length})` : '' }}
        </el-button>
        <el-button :icon="View" @click="togglePanel('preview')">
          预览
        </el-button>
        <el-button type="success" :icon="MagicStick" :loading="suggestionsLoading" @click="handleGenerateSuggestion">
          AI 建议
        </el-button>
        <el-button :icon="Clock" @click="togglePanel('version')">
          版本历史
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

    <!-- 主体内容 -->
    <div v-else class="editor-body">
      <!-- 左侧：评论面板 -->
      <CommentPanel
        v-if="visiblePanels.comment"
        :comments="comments"
        :loading="commentsLoading"
        @add="handleAddComment"
        @resolve="handleResolveComment"
        class="side-panel side-panel--left"
      />

      <!-- 中间：LaTeX 编辑区 -->
      <div class="editor-main">
        <LatexEditor
          v-model="editContent"
          @change="handleContentChange"
          class="latex-editor-wrapper"
        />
        <div class="editor-status-bar">
          <span>字数：{{ wordCount }}</span>
          <span>{{ autoSaveStatus }}</span>
        </div>
      </div>

      <!-- 右侧：预览面板 -->
      <LatexPreview
        v-if="visiblePanels.preview"
        :content="editContent"
        class="side-panel side-panel--right"
      />

      <!-- 右侧：AI 建议面板 -->
      <AiSuggestionPanel
        v-if="visiblePanels.suggestion"
        :suggestions="suggestions"
        :loading="suggestionsLoading"
        @accept="handleAcceptSuggestion"
        @reject="handleRejectSuggestion"
        @generate="handleGenerateSuggestion"
        class="side-panel side-panel--right"
      />

      <!-- 右侧：版本历史面板 -->
      <VersionHistory
        v-if="visiblePanels.version"
        :versions="versions"
        :loading="versionsLoading"
        @create="handleCreateVersion"
        class="side-panel side-panel--right"
      />
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
import { ArrowLeft, ChatDotRound, MagicStick, Clock, View } from '@element-plus/icons-vue'
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

// 面板可见性控制
const visiblePanels = ref({
  comment: false,
  preview: true,  // 默认显示预览
  suggestion: false,
  version: false
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

.editor-body {
  flex: 1;
  display: flex;
  overflow: hidden;
}

.editor-main {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
  background: var(--el-bg-color);
}

.latex-editor-wrapper {
  flex: 1;
  min-height: 0;
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
