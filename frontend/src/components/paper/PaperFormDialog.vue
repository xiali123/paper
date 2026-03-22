<template>
  <el-dialog
    v-model="dialogVisible"
    :title="isEdit ? ($t('papers.editPaper') || '编辑论文') : ($t('papers.addPaper') || '添加论文')"
    width="600px"
    :close-on-click-modal="false"
    @close="handleClose"
  >
    <el-form
      ref="formRef"
      :model="form"
      :rules="rules"
      label-width="100px"
      label-position="left"
    >
      <!-- 标题 -->
      <el-form-item :label="$t('papers.title') || '标题'" prop="title" required>
        <el-input
          v-model="form.title"
          :placeholder="$t('papers.titlePlaceholder') || '请输入论文标题'"
          maxlength="500"
          show-word-limit
        />
      </el-form-item>

      <!-- 作者 -->
      <el-form-item :label="$t('papers.authors') || '作者'" prop="authors">
        <el-input
          v-model="form.authors"
          :placeholder="$t('papers.authorsPlaceholder') || '请输入作者，多个作者用逗号分隔'"
          maxlength="500"
        />
      </el-form-item>

      <!-- 摘要 -->
      <el-form-item :label="$t('papers.abstract') || '摘要'" prop="abstract">
        <el-input
          v-model="form.abstract"
          type="textarea"
          :rows="4"
          :placeholder="$t('papers.abstractPlaceholder') || '请输入论文摘要'"
          maxlength="2000"
          show-word-limit
        />
      </el-form-item>

      <!-- 发表信息 -->
      <el-row :gutter="16">
        <el-col :span="12">
          <el-form-item :label="$t('papers.publication') || '出版物'" prop="publication">
            <el-input
              v-model="form.publication"
              :placeholder="$t('papers.publicationPlaceholder') || '期刊名称'"
            />
          </el-form-item>
        </el-col>
        <el-col :span="12">
          <el-form-item :label="$t('papers.year') || '年份'" prop="year">
            <el-input
              v-model="form.year"
              :placeholder="$t('papers.yearPlaceholder') || '发表年份'"
              maxlength="4"
            />
          </el-form-item>
        </el-col>
      </el-row>

      <!-- DOI -->
      <el-form-item label="DOI" prop="doi">
        <el-input
          v-model="form.doi"
          placeholder="请输入DOI（如：10.1000/xyz123）"
        />
      </el-form-item>

      <!-- URL -->
      <el-form-item label="URL" prop="url">
        <el-input
          v-model="form.url"
          placeholder="请输入论文链接"
        />
      </el-form-item>

      <!-- 分类和标签 -->
      <el-row :gutter="16">
        <el-col :span="12">
          <el-form-item :label="$t('papers.category') || '分类'" prop="category">
            <el-select
              v-model="form.category"
              :placeholder="$t('papers.selectCategory') || '请选择分类'"
              style="width: 100%"
            >
              <el-option label="人工智能" value="AI" />
              <el-option label="机器学习" value="ML" />
              <el-option label="深度学习" value="DL" />
              <el-option label="自然语言处理" value="NLP" />
              <el-option label="计算机视觉" value="CV" />
              <el-option label="其他" value="other" />
            </el-select>
          </el-form-item>
        </el-col>
        <el-col :span="12">
          <el-form-item :label="$t('papers.source') || '来源'" prop="source">
            <el-select
              v-model="form.source"
              :placeholder="$t('papers.selectSource') || '请选择来源'"
              style="width: 100%"
            >
              <el-option label="手动添加" value="manual" />
              <el-option label="知网" value="cnki" />
              <el-option label="IEEE" value="ieee" />
              <el-option label="ArXiv" value="arxiv" />
              <el-option label="PubMed" value="pubmed" />
            </el-select>
          </el-form-item>
        </el-col>
      </el-row>

      <!-- 标签 -->
      <el-form-item :label="$t('papers.tags') || '标签'" prop="tags">
        <el-input
          v-model="form.tags"
          :placeholder="$t('papers.tagsPlaceholder') || '多个标签用逗号分隔'"
          maxlength="200"
        />
      </el-form-item>

      <!-- PDF文件 -->
      <el-form-item :label="$t('papers.pdfFile') || 'PDF文件'" prop="pdfPath">
        <el-upload
          ref="uploadRef"
          :auto-upload="false"
          :show-file-list="true"
          :limit="1"
          accept=".pdf"
          :on-change="handleFileChange"
          :on-remove="handleFileRemove"
        >
          <el-button :icon="Upload" type="primary">
            {{ $t('papers.selectPdf') || '选择PDF文件' }}
          </el-button>
          <template #tip>
            <div class="el-upload__tip">
              {{ $t('papers.pdfTip') || '支持上传PDF文件，文件大小不超过50MB' }}
            </div>
          </template>
        </el-upload>
      </el-form-item>

      <!-- 笔记 -->
      <el-form-item :label="$t('papers.notes') || '笔记'" prop="notes">
        <el-input
          v-model="form.notes"
          type="textarea"
          :rows="3"
          :placeholder="$t('papers.notesPlaceholder') || '可以添加一些笔记或备注'"
          maxlength="5000"
          show-word-limit
        />
      </el-form-item>
    </el-form>

    <template #footer>
      <div class="dialog-footer">
        <el-button @click="handleClose">
          {{ $t('common.cancel') || '取消' }}
        </el-button>
        <el-button
          type="primary"
          :loading="loading"
          @click="handleSubmit"
        >
          {{ $t('common.save') || '保存' }}
        </el-button>
      </div>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { useI18n } from 'vue-i18n'
import type { FormInstance, FormRules, UploadFile } from 'element-plus'
import { ElMessage } from 'element-plus'
import { Upload } from '@element-plus/icons-vue'
import type { Paper, CreatePaperRequest, UpdatePaperRequest } from '@/api/modules/papers'

interface Props {
  modelValue: boolean
  paper?: Paper | null
}

const props = withDefaults(defineProps<Props>(), {
  paper: null
})

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  save: [data: CreatePaperRequest | UpdatePaperRequest]
}>()

const { t } = useI18n()

const formRef = ref<FormInstance>()
const uploadRef = ref()
const loading = ref(false)

// 对话框显示状态
const dialogVisible = computed({
  get: () => props.modelValue,
  set: (value) => emit('update:modelValue', value)
})

// 是否编辑模式
const isEdit = computed(() => !!props.paper)

// 表单数据
const form = ref<CreatePaperRequest & { notes?: string }>({
  title: '',
  authors: '',
  abstract: '',
  publication: '',
  year: '',
  doi: '',
  url: '',
  pdfPath: '',
  category: '',
  source: 'manual',
  tags: '',
  notes: ''
})

// 上传的文件
const uploadedFile = ref<File | null>(null)

// 表单验证规则
const rules: FormRules = {
  title: [
    { required: true, message: '请输入论文标题', trigger: 'blur' },
    { min: 2, max: 500, message: '标题长度在2到500个字符', trigger: 'blur' }
  ],
  year: [
    {
      pattern: /^\d{4}$/,
      message: '请输入正确的年份（4位数字）',
      trigger: 'blur'
    }
  ]
}

// 监听paper变化，初始化表单
watch(() => props.paper, (newPaper) => {
  if (newPaper) {
    form.value = {
      title: newPaper.title || '',
      authors: newPaper.authors || '',
      abstract: newPaper.abstract || '',
      publication: newPaper.publication || '',
      year: newPaper.year || '',
      doi: newPaper.doi || '',
      url: newPaper.url || '',
      pdfPath: newPaper.pdfPath || '',
      category: newPaper.category || '',
      source: newPaper.source || 'manual',
      tags: newPaper.tags || '',
      notes: newPaper.notes || ''
    }
  } else {
    resetForm()
  }
}, { immediate: true })

// 重置表单
function resetForm() {
  form.value = {
    title: '',
    authors: '',
    abstract: '',
    publication: '',
    year: '',
    doi: '',
    url: '',
    pdfPath: '',
    category: '',
    source: 'manual',
    tags: '',
    notes: ''
  }
  uploadedFile.value = null
  formRef.value?.clearValidate()
}

// 关闭对话框
function handleClose() {
  resetForm()
  dialogVisible.value = false
}

// 文件选择
function handleFileChange(file: UploadFile) {
  if (file.raw) {
    // 检查文件类型
    if (file.raw.type !== 'application/pdf') {
      ElMessage.error('只能上传PDF文件')
      return
    }

    // 检查文件大小（50MB）
    const maxSize = 50 * 1024 * 1024
    if (file.raw.size > maxSize) {
      ElMessage.error('文件大小不能超过50MB')
      return
    }

    uploadedFile.value = file.raw
    form.value.pdfPath = file.name
  }
}

// 文件移除
function handleFileRemove() {
  uploadedFile.value = null
  form.value.pdfPath = ''
}

// 提交表单
async function handleSubmit() {
  if (!formRef.value) return

  try {
    // 验证表单
    await formRef.value.validate()

    loading.value = true

    // 如果有上传的文件，这里应该先上传到服务器
    // 现在简化处理，只使用文件名
    const data = { ...form.value }

    emit('save', data)

  } catch (error) {
    console.error('表单验证失败:', error)
  } finally {
    loading.value = false
  }
}
</script>

<style scoped lang="scss">
.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
}

:deep(.el-dialog__body) {
  padding: 20px;
  max-height: 600px;
  overflow-y: auto;
}

:deep(.el-form-item__label) {
  font-weight: 500;
}

:deep(.el-upload__tip) {
  font-size: 12px;
  color: #909399;
  margin-top: 8px;
}
</style>
