<template>
  <div class="paper-edit-view">
    <!-- Page Header -->
    <div class="page-header">
      <div class="header-left">
        <el-button :icon="ArrowLeft" @click="handleBack">返回</el-button>
        <h1 class="page-title">{{ isEditMode ? '编辑论文' : '添加论文' }}</h1>
      </div>
      <div class="header-right">
        <el-button @click="handleBack">取消</el-button>
        <el-button type="primary" :loading="submitting" @click="handleSubmit">
          {{ isEditMode ? '保存' : '创建' }}
        </el-button>
      </div>
    </div>

    <!-- Loading State -->
    <div v-if="loading" class="loading-container">
      <el-skeleton animated>
        <template #template>
          <el-skeleton-item variant="text" style="width: 40%; height: 36px; margin-bottom: 24px" />
          <el-skeleton-item variant="rect" style="width: 100%; height: 60px; margin-bottom: 16px" />
          <el-skeleton-item variant="rect" style="width: 100%; height: 120px; margin-bottom: 16px" />
          <el-skeleton-item variant="rect" style="width: 100%; height: 60px; margin-bottom: 16px" />
          <el-skeleton-item variant="rect" style="width: 100%; height: 60px" />
        </template>
      </el-skeleton>
    </div>

    <!-- Form -->
    <el-form
      v-else
      ref="formRef"
      :model="formData"
      :rules="formRules"
      label-position="top"
      class="paper-form"
    >
      <el-card class="form-section" shadow="never">
        <template #header>
          <span class="section-title">基本信息</span>
        </template>

        <el-row :gutter="20">
          <el-col :span="24">
            <el-form-item label="论文标题 *" prop="title">
              <el-input
                v-model="formData.title"
                placeholder="请输入论文标题"
                maxlength="500"
                show-word-limit
              />
            </el-form-item>
          </el-col>

          <el-col :xs="24" :sm="24" :md="18" :lg="20">
            <el-form-item label="作者 *" prop="authors">
              <el-input
                v-model="formData.authors"
                placeholder="请输入作者，多个作者用逗号分隔"
                maxlength="500"
                show-word-limit
              />
            </el-form-item>
          </el-col>

          <el-col :xs="24" :sm="12" :md="6" :lg="4">
            <el-form-item label="发表年份 *" prop="year">
              <el-input-number
                v-model="formData.year"
                :min="1900"
                :max="2100"
                :step="1"
                placeholder="年份"
                controls-position="right"
                class="full-width"
              />
            </el-form-item>
          </el-col>
        </el-row>

        <el-form-item label="摘要" prop="abstract">
          <el-input
            v-model="formData.abstract"
            type="textarea"
            :rows="6"
            placeholder="请输入论文摘要"
            maxlength="5000"
            show-word-limit
          />
        </el-form-item>
      </el-card>

      <el-card class="form-section" shadow="never">
        <template #header>
          <span class="section-title">发表信息</span>
        </template>

        <el-row :gutter="20">
          <el-col :xs="24" :sm="12">
            <el-form-item label="期刊/会议" prop="publication">
              <el-autocomplete
                v-model="formData.publication"
                :fetch-suggestions="searchPublications"
                placeholder="请输入期刊或会议名称"
                style="width: 100%"
              />
            </el-form-item>
          </el-col>

          <el-col :xs="12" :sm="6">
            <el-form-item label="卷" prop="volume">
              <el-input v-model="formData.volume" placeholder="卷" />
            </el-form-item>
          </el-col>

          <el-col :xs="12" :sm="6">
            <el-form-item label="期" prop="issue">
              <el-input v-model="formData.issue" placeholder="期" />
            </el-form-item>
          </el-col>
        </el-row>

        <el-row :gutter="20">
          <el-col :xs="24" :sm="12">
            <el-form-item label="页码" prop="pages">
              <el-input v-model="formData.pages" placeholder="例如: 123-145" />
            </el-form-item>
          </el-col>

          <el-col :xs="24" :sm="12">
            <el-form-item label="DOI" prop="doi">
              <el-input v-model="formData.doi" placeholder="例如: 10.1000/xyz123" />
            </el-form-item>
          </el-col>
        </el-row>

        <el-form-item label="URL" prop="url">
          <el-input v-model="formData.url" placeholder="论文链接 URL" />
        </el-form-item>
      </el-card>

      <el-card class="form-section" shadow="never">
        <template #header>
          <span class="section-title">分类与标签</span>
        </template>

        <el-row :gutter="20">
          <el-col :xs="24" :sm="12">
            <el-form-item label="分类" prop="category">
              <el-select
                v-model="formData.category"
                placeholder="请选择分类"
                allow-create
                filterable
                style="width: 100%"
              >
                <el-option label="人工智能" value="AI" />
                <el-option label="机器学习" value="ML" />
                <el-option label="深度学习" value="DL" />
                <el-option label="自然语言处理" value="NLP" />
                <el-option label="计算机视觉" value="CV" />
                <el-option label="强化学习" value="RL" />
                <el-option label="知识图谱" value="KG" />
                <el-option label="数据挖掘" value="DM" />
              </el-select>
            </el-form-item>
          </el-col>

          <el-col :xs="24" :sm="12">
            <el-form-item label="来源" prop="source">
              <el-select v-model="formData.source" placeholder="请选择来源" style="width: 100%">
                <el-option label="手动添加" value="manual" />
                <el-option label="知网" value="cnki" />
                <el-option label="IEEE" value="ieee" />
                <el-option label="ArXiv" value="arxiv" />
                <el-option label="PubMed" value="pubmed" />
                <el-option label="Google Scholar" value="scholar" />
              </el-select>
            </el-form-item>
          </el-col>
        </el-row>

        <el-form-item label="标签" prop="tags">
          <el-select
            v-model="tagValues"
            multiple
            filterable
            allow-create
            placeholder="请输入标签，按回车添加"
            style="width: 100%"
          >
            <el-option
              v-for="tag in commonTags"
              :key="tag"
              :label="tag"
              :value="tag"
            />
          </el-select>
        </el-form-item>
      </el-card>

      <el-card class="form-section" shadow="never">
        <template #header>
          <span class="section-title">其他信息</span>
        </template>

        <el-row :gutter="20">
          <el-col :xs="24" :sm="12">
            <el-form-item label="引用数" prop="citations">
              <el-input-number
                v-model="formData.citations"
                :min="0"
                :step="1"
                controls-position="right"
                class="full-width"
              />
            </el-form-item>
          </el-col>

          <el-col :xs="24" :sm="12">
            <el-form-item label="下载次数" prop="downloads">
              <el-input-number
                v-model="formData.downloads"
                :min="0"
                :step="1"
                controls-position="right"
                class="full-width"
              />
            </el-form-item>
          </el-col>
        </el-row>

        <el-form-item label="阅读进度" prop="readingProgress">
          <el-slider
            v-model="formData.readingProgress"
            :min="0"
            :max="100"
            :step="5"
            show-stops
            :marks="{ 0: '未读', 50: '50%', 100: '读完' }"
          />
        </el-form-item>

        <el-form-item label="笔记" prop="notes">
          <el-input
            v-model="formData.notes"
            type="textarea"
            :rows="4"
            placeholder="添加笔记或备注..."
            maxlength="2000"
            show-word-limit
          />
        </el-form-item>

        <el-form-item label="附件" prop="attachments">
          <el-upload
            v-model:file-list="fileList"
            action="#"
            :auto-upload="false"
            :on-change="handleFileChange"
            :on-remove="handleFileRemove"
            multiple
            drag
          >
            <el-icon class="el-icon--upload"><UploadFilled /></el-icon>
            <div class="el-upload__text">
              将文件拖到此处，或<em>点击上传</em>
            </div>
            <template #tip>
              <div class="el-upload__tip">
                支持 PDF、DOC、DOCX 等格式，单个文件不超过 50MB
              </div>
            </template>
          </el-upload>
        </el-form-item>
      </el-card>

      <!-- Batch Import Section (Only for new paper) -->
      <el-card v-if="!isEditMode" class="form-section" shadow="never">
        <template #header>
          <span class="section-title">批量导入</span>
        </template>

        <el-alert
          title="支持从 BibTeX、EndNote、RIS 等格式批量导入论文"
          type="info"
          :closable="false"
          show-icon
          style="margin-bottom: 16px"
        />

        <el-button :icon="Upload" @click="handleBatchImport">
          批量导入论文
        </el-button>
      </el-card>
    </el-form>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { usePaperStore } from '@/stores/paperStore'
import { storeToRefs } from 'pinia'
import type { FormInstance, FormRules, UploadUserFile, UploadFile } from 'element-plus'
import {
  ArrowLeft,
  Upload,
  UploadFilled
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

const route = useRoute()
const router = useRouter()
const paperStore = usePaperStore()

const { currentPaper: paper, loading } = storeToRefs(paperStore)

// Form ref
const formRef = ref<FormInstance>()

// Submitting state
const submitting = ref(false)

// Is edit mode
const isEditMode = computed(() => !!route.params.id && route.params.id !== 'new')

// Form data
const formData = ref({
  title: '',
  authors: '',
  abstract: '',
  publication: '',
  year: new Date().getFullYear(),
  volume: '',
  issue: '',
  pages: '',
  doi: '',
  url: '',
  category: '',
  source: 'manual',
  tags: '',
  citations: 0,
  downloads: 0,
  readingProgress: 0,
  notes: '',
  attachments: [] as string[]
})

// Tag values for select
const tagValues = ref<string[]>([])

// File list
const fileList = ref<UploadUserFile[]>([])

// Common tags
const commonTags = ref([
  '深度学习',
  '神经网络',
  '自然语言处理',
  '计算机视觉',
  '强化学习',
  'Transformer',
  'BERT',
  'GNN',
  'Attention',
  'CNN',
  'RNN',
  '预训练模型',
  '知识图谱',
  '推荐系统',
  '聚类',
  '分类',
  '回归'
])

// Common publications
const commonPublications = ref([
  'Nature',
  'Science',
  'Cell',
  'NEURIPS',
  'ICML',
  'ICLR',
  'ACL',
  'EMNLP',
  'CVPR',
  'ICCV',
  'ECCV',
  'AAAI',
  'IJCAI',
  'KDD',
  'WWW',
  'SIGIR',
  'ACM Turing'
])

// Form validation rules
const formRules: FormRules = {
  title: [
    { required: true, message: '请输入论文标题', trigger: 'blur' },
    { min: 2, max: 500, message: '标题长度应在 2 到 500 个字符之间', trigger: 'blur' }
  ],
  authors: [
    { required: true, message: '请输入作者', trigger: 'blur' },
    { min: 2, max: 500, message: '作者信息长度应在 2 到 500 个字符之间', trigger: 'blur' }
  ],
  year: [
    { required: true, message: '请输入发表年份', trigger: 'blur' },
    { type: 'number', min: 1900, max: 2100, message: '请输入有效的年份', trigger: 'blur' }
  ],
  abstract: [
    { max: 5000, message: '摘要长度不能超过 5000 个字符', trigger: 'blur' }
  ],
  doi: [
    {
      pattern: /^10\.\d{4,9}\/[-._;()/:A-Z0-9]+$/i,
      message: '请输入有效的 DOI 格式',
      trigger: 'blur'
    }
  ],
  url: [
    { type: 'url', message: '请输入有效的 URL', trigger: 'blur' }
  ]
}

// Search publications
const searchPublications = (queryString: string, cb: any) => {
  const results = queryString
    ? commonPublications.value
        .filter(pub => pub.toLowerCase().includes(queryString.toLowerCase()))
        .map(pub => ({ value: pub }))
    : commonPublications.value.map(pub => ({ value: pub }))
  cb(results)
}

// Handle file change
const handleFileChange = (file: UploadFile) => {
  console.log('File changed:', file)
}

// Handle file remove
const handleFileRemove = (file: UploadFile) => {
  console.log('File removed:', file)
}

// Handle batch import
const handleBatchImport = () => {
  ElMessage.info('批量导入功能开发中...')
}

// Load paper data for edit mode
const loadPaperData = async () => {
  if (!isEditMode.value) return

  const id = Number(route.params.id)
  if (isNaN(id)) {
    ElMessage.error('无效的论文 ID')
    router.push('/papers')
    return
  }

  try {
    await paperStore.fetchPaper(id)

    if (paper.value) {
      formData.value = {
        title: paper.value.title || '',
        authors: paper.value.authors || '',
        abstract: paper.value.abstract || '',
        publication: paper.value.publication || '',
        year: paper.value.year ? parseInt(paper.value.year) : new Date().getFullYear(),
        volume: paper.value.volume || '',
        issue: paper.value.issue || '',
        pages: paper.value.pages || '',
        doi: paper.value.doi || '',
        url: paper.value.url || '',
        category: paper.value.category || '',
        source: paper.value.source || 'manual',
        tags: paper.value.tags || '',
        citations: paper.value.citations || 0,
        downloads: paper.value.downloads || 0,
        readingProgress: paper.value.readingProgress || 0,
        notes: paper.value.notes || '',
        attachments: paper.value.attachments || []
      }

      // Parse tags
      if (paper.value.tags) {
        tagValues.value = paper.value.tags.split(',').map(tag => tag.trim()).filter(tag => tag)
      }
    }
  } catch (error: any) {
    ElMessage.error(error.message || '加载论文数据失败')
    router.push('/papers')
  }
}

// Handle form submit
const handleSubmit = async () => {
  if (!formRef.value) return

  try {
    await formRef.value.validate()

    submitting.value = true

    // Convert tags array to string
    const submitData = {
      ...formData.value,
      tags: tagValues.value.join(','),
      year: formData.value.year.toString()
    }

    if (isEditMode.value) {
      await paperStore.updatePaper(Number(route.params.id), submitData)
      ElMessage.success('保存成功')
    } else {
      await paperStore.createPaper(submitData)
      ElMessage.success('创建成功')
    }

    router.push('/papers')
  } catch (error: any) {
    if (error.errors) {
      // Validation error
      return
    }
    ElMessage.error(error.message || '保存失败')
  } finally {
    submitting.value = false
  }
}

// Handle back
const handleBack = () => {
  router.back()
}

// Lifecycle
onMounted(() => {
  loadPaperData()
})
</script>

<style scoped lang="scss">
.paper-edit-view {
  max-width: 1200px;
  margin: 0 auto;
  padding: 24px;
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

.header-left,
.header-right {
  display: flex;
  align-items: center;
  gap: 16px;
}

.page-title {
  margin: 0;
  font-size: 24px;
  font-weight: 700;
  color: #303133;
}

.loading-container {
  padding: 60px 20px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.paper-form {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.form-section {
  :deep(.el-card__header) {
    padding: 16px 20px;
    background: #f5f7fa;
    border-bottom: 1px solid #e4e7ed;
  }

  :deep(.el-card__body) {
    padding: 20px;
  }
}

.section-title {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.full-width {
  width: 100%;
}

:deep(.el-form-item__label) {
  font-weight: 500;
  color: #606266;
}

:deep(.el-upload-dragger) {
  width: 100%;
  min-height: 180px;
}

@media (max-width: 768px) {
  .paper-edit-view {
    padding: 16px;
  }

  .page-header {
    flex-direction: column;
    gap: 12px;
    align-items: stretch;
  }

  .header-left,
  .header-right {
    justify-content: space-between;
  }

  .page-title {
    font-size: 20px;
  }

  .form-section {
    :deep(.el-card__body) {
      padding: 16px;
    }
  }
}
</style>
