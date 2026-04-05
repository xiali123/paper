<template>
  <div class="collaborative-container">
    <div class="collaborative-header">
      <h1>✍️ 实时协作写作</h1>
      <el-button type="primary" @click="createDocumentDialog = true">
        创建新文档
      </el-button>
    </div>

    <!-- 文档列表 -->
    <div class="documents-list">
      <div
        v-for="doc in documents"
        :key="doc.id"
        class="document-card"
        @click="openDocument(doc.id)"
      >
        <div class="doc-header">
          <h3>{{ doc.title }}</h3>
          <el-tag :type="getStatusType(doc.status)">{{ doc.status }}</el-tag>
        </div>
        <div class="doc-meta">
          <span>{{ doc.wordCount }} 字</span>
          <span>{{ formatDate(doc.updatedAt) }}</span>
        </div>
      </div>
    </div>

    <!-- 创建文档对话框 -->
    <el-dialog v-model="createDocumentDialog" title="创建协作文档" width="500px">
      <el-form :model="newDoc" label-position="top">
        <el-form-item label="文档标题">
          <el-input v-model="newDoc.title" placeholder="输入文档标题" />
        </el-form-item>
        <el-form-item label="文档类型">
          <el-select v-model="newDoc.documentType" placeholder="选择类型">
            <el-option label="学术论文" value="academic_paper" />
            <el-option label="研究报告" value="research_report" />
            <el-option label="综述文章" value="review_article" />
            <el-option label="会议论文" value="conference_paper" />
          </el-select>
        </el-form-item>
        <el-form-item label="模板">
          <el-select v-model="newDoc.templateId" placeholder="选择模板（可选）">
            <el-option label="IEEE论文模板" :value="1" />
            <el-option label="ACM论文模板" :value="2" />
            <el-option label="自定义模板" :value="3" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="createDocumentDialog = false">取消</el-button>
        <el-button type="primary" @click="createDocument" :loading="creating">
          创建
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { collaborativeApi } from '@/api/modules/collaborative'
import type { CollaborativeDocument } from '@/types/collaborative'

const router = useRouter()
const createDocumentDialog = ref(false)
const creating = ref(false)
const documents = ref<CollaborativeDocument[]>([])

const newDoc = ref({
  title: '',
  documentType: '',
  templateId: undefined
})

const openDocument = (docId: number) => {
  router.push(`/collaborative/${docId}`)
}

const createDocument = async () => {
  if (!newDoc.value.title || !newDoc.value.documentType) {
    ElMessage.warning('请填写完整信息')
    return
  }

  creating.value = true
  try {
    const doc = await collaborativeApi.createDocument({
      title: newDoc.value.title,
      documentType: newDoc.value.documentType,
      templateId: newDoc.value.templateId
    })
    documents.value.unshift(doc)
    createDocumentDialog.value = false
    newDoc.value = { title: '', documentType: '', templateId: undefined }
    ElMessage.success('文档创建成功')
    openDocument(doc.id)
  } catch (error) {
    ElMessage.error('创建失败')
  } finally {
    creating.value = false
  }
}

const getStatusType = (status: string) => {
  if (status === 'active') return 'success'
  if (status === 'archived') return 'info'
  return 'danger'
}

const formatDate = (dateStr: string) => {
  const date = new Date(dateStr)
  const now = new Date()
  const diff = now.getTime() - date.getTime()

  if (diff < 86400000) return '今天'
  if (diff < 172800000) return '昨天'
  return date.toLocaleDateString()
}
</script>

<style scoped>
.collaborative-container {
  max-width: 1400px;
  margin: 0 auto;
  padding: $spacing-6;
}

.collaborative-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 32px;
}

.collaborative-header h1 {
  font-size: 32px;
  font-weight: 700;
  margin: 0;
}

.documents-list {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 20px;
}

.document-card {
  background: white;
  border-radius: 12px;
  padding: $spacing-5;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  cursor: pointer;
  transition: all 0.3s ease;
}

.document-card:hover {
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.12);
  transform: translateY(-2px);
}

.doc-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 12px;
}

.doc-header h3 {
  font-size: 18px;
  font-weight: 600;
  color: #303133;
  margin: 0;
  flex: 1;
  margin-right: 12px;
}

.doc-meta {
  display: flex;
  gap: 12px;
  font-size: 14px;
  color: #909399;
}
</style>
