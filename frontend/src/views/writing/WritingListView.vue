<template>
  <div class="writing-list-view">
    <!-- 页面头部 -->
    <div class="page-header">
      <div class="page-header__content">
        <h1 class="page-header__title">协作写作</h1>
        <p class="page-header__subtitle">管理和浏览您的协作文档</p>
      </div>
      <div class="page-header__actions">
        <el-button type="primary" :icon="Plus" @click="handleCreate">
          新建文档
        </el-button>
      </div>
    </div>

    <!-- 搜索栏 -->
    <div class="toolbar">
      <el-input
        v-model="searchKeyword"
        placeholder="搜索文档..."
        :prefix-icon="Search"
        clearable
        class="search-input"
        @clear="handleSearch"
        @keyup.enter="handleSearch"
      />
    </div>

    <!-- 文档网格 -->
    <div class="documents-grid">
      <!-- 骨架屏 -->
      <div v-if="loading && documents.length === 0" class="skeleton-grid">
        <el-skeleton v-for="i in 6" :key="i" animated>
          <template #template>
            <el-skeleton-item variant="rect" style="width: 100%; height: 200px; border-radius: 8px" />
          </template>
        </el-skeleton>
      </div>

      <!-- 空状态 -->
      <div v-else-if="documents.length === 0" class="empty-state">
        <el-empty description="暂无文档">
          <el-button type="primary" :icon="Plus" @click="handleCreate">
            创建第一篇文档
          </el-button>
        </el-empty>
      </div>

      <!-- 文档卡片 -->
      <DocumentCard
        v-for="doc in documents"
        v-else
        :key="doc.id"
        :document="doc"
        @edit="handleEdit"
        @delete="handleDelete"
      />
    </div>

    <!-- 创建文档弹窗 -->
    <el-dialog v-model="createDialogVisible" title="新建文档" width="480px">
      <el-form :model="createForm" label-position="top">
        <el-form-item label="文档标题" required>
          <el-input
            v-model="createForm.title"
            placeholder="请输入文档标题"
            maxlength="200"
            show-word-limit
          />
        </el-form-item>
        <el-form-item label="文档类型">
          <el-select v-model="createForm.document_type" style="width: 100%">
            <el-option label="论文" value="paper" />
            <el-option label="报告" value="report" />
            <el-option label="笔记" value="note" />
            <el-option label="学位论文" value="thesis" />
            <el-option label="其他" value="other" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="createDialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="creating" @click="confirmCreate">
          创建
        </el-button>
      </template>
    </el-dialog>

    <!-- 删除确认弹窗 -->
    <el-dialog v-model="deleteDialogVisible" title="确认删除" width="400px">
      <span>确定要删除文档《{{ deleteTarget?.title }}》吗？此操作不可撤销。</span>
      <template #footer>
        <el-button @click="deleteDialogVisible = false">取消</el-button>
        <el-button type="danger" :loading="deleting" @click="confirmDelete">
          删除
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useWritingStore } from '@/stores/writingStore'
import { useAuthStore } from '@/stores/authStore'
import { storeToRefs } from 'pinia'
import DocumentCard from '@/components/writing/DocumentCard.vue'
import { Plus, Search } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import type { CollaborativeDocument } from '@/types/collaborative'

const router = useRouter()
const writingStore = useWritingStore()
const authStore = useAuthStore()
const { documents, loading } = storeToRefs(writingStore)

// 当前用户 ID
const currentUserId = computed(() => authStore.user?.id)

const searchKeyword = ref('')
const createDialogVisible = ref(false)
const createForm = ref({ title: '', document_type: 'paper' })
const creating = ref(false)
const deleteDialogVisible = ref(false)
const deleteTarget = ref<CollaborativeDocument | null>(null)
const deleting = ref(false)

function handleCreate() {
  createDialogVisible.value = true
  createForm.value = { title: '', document_type: 'paper' }
}

function handleEdit(doc: CollaborativeDocument) {
  router.push(`/writing/${doc.id}`)
}

function handleDelete(doc: CollaborativeDocument) {
  deleteTarget.value = doc
  deleteDialogVisible.value = true
}

function handleSearch() {
  writingStore.fetchDocuments({ title: searchKeyword.value })
}

async function confirmCreate() {
  if (!createForm.value.title.trim()) {
    ElMessage.warning('请输入文档标题')
    return
  }

  if (!currentUserId.value) {
    ElMessage.error('请先登录')
    return
  }

  creating.value = true
  try {
    const doc = await writingStore.createDocument({
      ...createForm.value,
      owner_id: currentUserId.value
    })
    ElMessage.success('文档创建成功')
    createDialogVisible.value = false
    router.push(`/writing/${doc.id}`)
  } catch (err: any) {
    console.error('创建文档失败:', err)
    ElMessage.error(err.message || '创建失败')
  } finally {
    creating.value = false
  }
}

async function confirmDelete() {
  if (!deleteTarget.value) return

  deleting.value = true
  try {
    await writingStore.deleteDocument(deleteTarget.value.id)
    ElMessage.success('删除成功')
    deleteDialogVisible.value = false
  } catch {
    ElMessage.error('删除失败')
  } finally {
    deleting.value = false
  }
}

onMounted(() => {
  // 如果用户已登录，只获取该用户的文档
  if (currentUserId.value) {
    writingStore.fetchDocuments({ owner_id: currentUserId.value })
  } else {
    // 未登录时获取所有文档（如果有权限）
    writingStore.fetchDocuments()
  }
})
</script>

<style scoped lang="scss">
.writing-list-view {
  padding: 20px;
  max-width: 1400px;
  margin: 0 auto;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 24px;
}

.page-header__content {
  flex: 1;
}

.page-header__title {
  margin: 0 0 8px 0;
  font-size: 28px;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.page-header__subtitle {
  margin: 0;
  font-size: 14px;
  color: var(--el-text-color-secondary);
}

.page-header__actions {
  flex-shrink: 0;
}

.toolbar {
  margin-bottom: 20px;

  .search-input {
    max-width: 400px;
  }
}

.documents-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 20px;
}

.skeleton-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 20px;
}

.empty-state {
  grid-column: 1 / -1;
  padding: 60px 0;
  text-align: center;
}
</style>
