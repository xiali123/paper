<template>
  <div class="template-edit">
    <!-- Page Header -->
    <div class="page-header">
      <h1 class="page-title">
        <el-icon><Edit /></el-icon>
        {{ isEdit ? '编辑模板' : '创建模板' }}
      </h1>
      <div class="header-actions">
        <el-button @click="handleCancel">
          <el-icon><Close /></el-icon>
          取消
        </el-button>
        <el-button type="primary" :loading="saving" @click="handleSave">
          <el-icon><Check /></el-icon>
          保存
        </el-button>
      </div>
    </div>

    <el-row :gutter="20">
      <!-- Main Form -->
      <el-col :xs="24" :lg="16">
        <el-card shadow="hover" class="form-card">
          <el-form
            ref="formRef"
            :model="form"
            :rules="rules"
            label-width="120px"
            @submit.prevent="handleSave"
          >
            <!-- Basic Information -->
            <el-divider content-position="left">
              <span>
                <el-icon><InfoFilled /></el-icon>
                基本信息
              </span>
            </el-divider>

            <el-form-item label="模板名称" prop="name">
              <el-input
                v-model="form.name"
                placeholder="输入模板名称"
                clearable
              />
            </el-form-item>

            <el-form-item label="数据源" prop="source">
              <el-select
                v-model="form.source"
                placeholder="选择数据源"
                @change="handleSourceChange"
              >
                <el-option
                  v-for="source in availableSources"
                  :key="source.source"
                  :label="source.name"
                  :value="source.source"
                >
                  <div class="source-option">
                    <span>{{ source.name }}</span>
                    <el-tag
                      v-if="!source.available"
                      type="info"
                      size="small"
                    >
                      不可用
                    </el-tag>
                  </div>
                </el-option>
              </el-select>
            </el-form-item>

            <el-form-item label="搜索关键词" prop="query">
              <el-input
                v-model="form.query"
                type="textarea"
                :rows="3"
                placeholder="输入搜索关键词或查询语句"
              />
              <div class="form-tip">
                提示：可以使用布尔运算符（AND, OR, NOT）和括号来组合查询
              </div>
            </el-form-item>

            <el-form-item label="结果数量" prop="limit">
              <el-slider
                v-model="form.limit"
                :min="1"
                :max="100"
                :step="5"
                show-stops
                :marks="{ 1: '1', 10: '10', 50: '50', 100: '100' }"
              />
              <span class="limit-display">{{ form.limit }} 篇论文</span>
            </el-form-item>

            <el-form-item label="描述">
              <el-input
                v-model="form.description"
                type="textarea"
                :rows="2"
                placeholder="模板描述（可选）"
              />
            </el-form-item>

            <!-- Advanced Options -->
            <el-divider content-position="left">
              <span>
                <el-icon><Setting /></el-icon>
                高级选项
              </span>
            </el-divider>

            <el-form-item label="包含摘要">
              <el-switch v-model="form.options.includeAbstract" />
              <span class="option-label">是否获取论文摘要</span>
            </el-form-item>

            <el-form-item label="包含全文">
              <el-switch v-model="form.options.includeFullText" />
              <span class="option-label">是否尝试获取全文（可能更慢）</span>
            </el-form-item>

            <el-form-item label="日期范围">
              <el-date-picker
                v-model="dateRange"
                type="daterange"
                range-separator="至"
                start-placeholder="开始日期"
                end-placeholder="结束日期"
                format="YYYY-MM-DD"
                value-format="YYYY-MM-DD"
                @change="handleDateRangeChange"
              />
            </el-form-item>
          </el-form>
        </el-card>
      </el-col>

      <!-- Sidebar -->
      <el-col :xs="24" :lg="8">
        <!-- Test Tool -->
        <el-card shadow="hover" class="test-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><VideoPlay /></el-icon>
                测试工具
              </span>
            </div>
          </template>

          <div class="test-content">
            <el-alert
              type="info"
              :closable="false"
              show-icon
              style="mb-4"
            >
              在保存前测试您的查询是否有效
            </el-alert>

            <el-button
              type="primary"
              :loading="testing"
              :disabled="!form.query || !form.source"
              @click="handleTest"
              style="width: 100%"
            >
              <el-icon><VideoPlay /></el-icon>
              测试查询
            </el-button>

            <div v-if="testResult" class="test-result">
              <el-divider>测试结果</el-divider>
              <el-descriptions :column="1" border>
                <el-descriptions-item label="状态">
                  <el-tag :type="testResult.success ? 'success' : 'danger'">
                    {{ testResult.success ? '成功' : '失败' }}
                  </el-tag>
                </el-descriptions-item>
                <el-descriptions-item v-if="testResult.success" label="找到结果">
                  {{ testResult.count || 0 }} 篇论文
                </el-descriptions-item>
                <el-descriptions-item v-if="testResult.time" label="响应时间">
                  {{ testResult.time }} ms
                </el-descriptions-item>
                <el-descriptions-item v-if="testResult.error" label="错误信息">
                  {{ testResult.error }}
                </el-descriptions-item>
              </el-descriptions>

              <!-- Sample Results Preview -->
              <div v-if="testResult.samples && testResult.samples.length > 0" class="samples-preview">
                <h4>示例结果</h4>
                <div
                  v-for="(sample, index) in testResult.samples.slice(0, 3)"
                  :key="index"
                  class="sample-item"
                >
                  <div class="sample-title">{{ sample.title }}</div>
                  <div class="sample-authors">{{ sample.authors }}</div>
                </div>
              </div>
            </div>
          </div>
        </el-card>

        <!-- Validation Tips -->
        <el-card shadow="hover" class="tips-card">
          <template #header>
            <div class="card-header">
              <span>
                <el-icon><Warning /></el-icon>
                验证提示
              </span>
            </div>
          </template>

          <div class="tips-content">
            <el-alert
              v-for="(tip, index) in validationTips"
              :key="index"
              :title="tip.title"
              :type="tip.type"
              :closable="false"
              show-icon
              style="margin-bottom: 10px"
            >
              {{ tip.message }}
            </el-alert>
          </div>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { useCrawlerStore, type CrawlerTemplate } from '@/stores/crawlerStore'
import { ElMessage, ElMessageBox, type FormInstance, type FormRules } from 'element-plus'
import {
  Edit,
  Close,
  Check,
  InfoFilled,
  Setting,
  VideoPlay,
  Warning
} from '@element-plus/icons-vue'
import { crawlerApi, type CrawlerSource, type CrawlerSearchRequest } from '@/api/modules/crawler'

const router = useRouter()
const route = useRoute()
const crawlerStore = useCrawlerStore()

// State
const formRef = ref<FormInstance>()
const saving = ref(false)
const testing = ref(false)
const testResult = ref<any>(null)
const dateRange = ref<[string, string] | null>(null)

// Form Data
const form = reactive<CrawlerTemplate & {
  description?: string
  options: CrawlerSearchRequest['options']
}>({
  id: '',
  name: '',
  source: 'arxiv' as CrawlerSource,
  query: '',
  limit: 10,
  description: '',
  options: {
    includeAbstract: true,
    includeFullText: false,
    dateRange: undefined
  },
  createdAt: 0,
  usageCount: 0
})

// Computed
const isEdit = computed(() => !!route.params.id)
const templateId = computed(() => route.params.id as string)
const availableSources = computed(() => crawlerStore.availableSources)

const validationTips = computed(() => {
  const tips: Array<{ title: string; message: string; type: any }> = []

  // Name validation
  if (!form.name) {
    tips.push({
      title: '模板名称',
      message: '请输入模板名称',
      type: 'warning'
    })
  }

  // Query validation
  if (!form.query) {
    tips.push({
      title: '搜索关键词',
      message: '请输入搜索关键词',
      type: 'warning'
    })
  } else if (form.query.length < 3) {
    tips.push({
      title: '搜索关键词',
      message: '关键词太短，建议至少3个字符',
      type: 'warning'
    })
  }

  // Limit validation
  if (form.limit > 50) {
    tips.push({
      title: '结果数量',
      message: '大量结果可能需要较长时间',
      type: 'info'
    })
  }

  // Source validation
  if (!form.source) {
    tips.push({
      title: '数据源',
      message: '请选择数据源',
      type: 'error'
    })
  }

  return tips
})

// Form Rules
const rules: FormRules = {
  name: [
    { required: true, message: '请输入模板名称', trigger: 'blur' },
    { min: 2, max: 50, message: '长度在 2 到 50 个字符', trigger: 'blur' }
  ],
  source: [
    { required: true, message: '请选择数据源', trigger: 'change' }
  ],
  query: [
    { required: true, message: '请输入搜索关键词', trigger: 'blur' },
    { min: 3, message: '关键词至少3个字符', trigger: 'blur' }
  ]
}

// Methods
const handleSourceChange = () => {
  // Update form when source changes
  testResult.value = null
}

const handleDateRangeChange = (value: [string, string] | null) => {
  if (value) {
    form.options.dateRange = {
      start: value[0],
      end: value[1]
    }
  } else {
    form.options.dateRange = undefined
  }
}

const handleTest = async () => {
  if (!form.query || !form.source) {
    ElMessage.warning('请先填写查询关键词和选择数据源')
    return
  }

  testing.value = true
  testResult.value = null

  try {
    const startTime = Date.now()

    // Test connection first
    const connectionResult = await crawlerApi.testConnection(form.source)

    if (!connectionResult.success) {
      testResult.value = {
        success: false,
        error: connectionResult.message,
        time: connectionResult.latency
      }
      return
    }

    // Try a small search to test the query
    const testRequest: CrawlerSearchRequest = {
      query: form.query,
      source: form.source,
      limit: 3, // Only get a few results for testing
      options: form.options
    }

    const task = await crawlerApi.search(testRequest)
    const endTime = Date.now()

    // Wait a bit for results
    await new Promise(resolve => setTimeout(resolve, 2000))
    const updatedTask = await crawlerApi.fetchTaskStatus(task.id)

    testResult.value = {
      success: updatedTask.status !== 'failed',
      count: updatedTask.completedPapers || 0,
      time: endTime - startTime,
      samples: updatedTask.papers?.slice(0, 3) || [],
      error: updatedTask.errorMessage
    }

    if (testResult.value.success) {
      ElMessage.success(`测试成功！找到 ${testResult.value.count} 篇论文`)
    } else {
      ElMessage.error('测试失败：' + testResult.value.error)
    }
  } catch (error: any) {
    testResult.value = {
      success: false,
      error: error.message
    }
    ElMessage.error('测试失败：' + error.message)
  } finally {
    testing.value = false
  }
}

const handleSave = async () => {
  if (!formRef.value) return

  try {
    await formRef.value.validate()
  } catch {
    ElMessage.warning('请修正表单中的错误')
    return
  }

  saving.value = true

  try {
    if (isEdit.value) {
      // Update existing template
      crawlerStore.updateTemplate(templateId.value, {
        name: form.name,
        source: form.source,
        query: form.query,
        limit: form.limit,
        options: form.options
      })
      ElMessage.success('模板更新成功')
    } else {
      // Create new template
      crawlerStore.createTemplate({
        name: form.name,
        source: form.source,
        query: form.query,
        limit: form.limit,
        options: form.options
      })
      ElMessage.success('模板创建成功')
    }

    router.push('/crawler/templates')
  } catch (error: any) {
    ElMessage.error('保存失败：' + error.message)
  } finally {
    saving.value = false
  }
}

const handleCancel = () => {
  ElMessageBox.confirm(
    '确定要取消吗？未保存的更改将丢失。',
    '确认',
    {
      type: 'warning',
      confirmButtonText: '确定',
      cancelButtonText: '继续编辑'
    }
  ).then(() => {
    router.back()
  }).catch(() => {})
}

// Lifecycle
onMounted(async () => {
  await crawlerStore.fetchSupportedSources()

  // Load template data if editing
  if (isEdit.value) {
    const template = crawlerStore.templates.find(t => t.id === templateId.value)
    if (template) {
      Object.assign(form, template)

      // Set date range if exists
      if (template.options?.dateRange) {
        dateRange.value = [
          template.options.dateRange.start || '',
          template.options.dateRange.end || ''
        ]
      }
    } else {
      ElMessage.error('模板不存在')
      router.push('/crawler/templates')
    }
  }
})
</script>

<style scoped lang="scss">
// ==========================================
// 现代化模板编辑页面样式
// Modern Template Edit Page Styles
// ==========================================

.template-edit {
  padding: $spacing-6;
  min-height: calc(100vh - 120px);
  background: linear-gradient(135deg, $gray-50 0%, $gray-100 100%);

  .dark & {
    background: linear-gradient(135deg, $gray-900 0%, $gray-800 100%);
  }
}

// Page Header
.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: $spacing-6;
  padding: $spacing-6 $spacing-8;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border-radius: $border-radius-xl;
  box-shadow: $shadow-md;
  border: 1px solid $border-light;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    border-color: $gray-700;
  }

  .page-title {
    display: flex;
    align-items: center;
    gap: $spacing-3;
    margin: 0;
    font-size: $font-size-2xl;
    font-weight: $font-weight-bold;
    color: $text-primary;

    .dark & {
      color: $gray-100;
    }

    .el-icon {
      color: $primary-600;

      .dark & {
        color: $primary-400;
      }
    }
  }

  .header-actions {
    display: flex;
    gap: $spacing-3;
  }
}

// Form Card
.form-card {
  margin-bottom: $spacing-6;
  border-radius: $border-radius-xl;
  border: 1px solid $border-light;
  transition: all $duration-slow;

  &:hover {
    box-shadow: $shadow-lg;
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }
}

.form-tip {
  font-size: $font-size-xs;
  color: $text-secondary;
  margin-top: $spacing-2;
  display: flex;
  align-items: center;
  gap: $spacing-1;

  &:before {
    content: '💡';
  }
}

.limit-display {
  margin-left: $spacing-3;
  color: $primary-600;
  font-weight: $font-weight-bold;
  font-size: $font-size-sm;

  .dark & {
    color: $primary-400;
  }
}

.option-label {
  margin-left: $spacing-3;
  color: $text-secondary;
  font-size: $font-size-sm;
}

.source-option {
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
}

// Card Headers
.card-header {
  font-weight: $font-weight-bold;

  span {
    display: flex;
    align-items: center;
    gap: $spacing-2;
    color: $text-primary;

    .dark & {
      color: $gray-100;
    }

    .el-icon {
      color: $primary-600;

      .dark & {
        color: $primary-400;
      }
    }
  }
}

// Test Card
.test-card {
  margin-bottom: $spacing-6;
  border-radius: $border-radius-xl;
  border: 1px solid $border-light;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }
}

.test-content {
  .test-result {
    margin-top: $spacing-6;

    .samples-preview {
      margin-top: $spacing-5;

      h4 {
        margin: 0 0 $spacing-4 0;
        font-size: $font-size-base;
        font-weight: $font-weight-semibold;
        color: $text-primary;

        .dark & {
          color: $gray-100;
        }
      }

      .sample-item {
        padding: $spacing-4;
        background: linear-gradient(135deg, $gray-50 0%, #ffffff 100%);
        border-radius: $border-radius-base;
        margin-bottom: $spacing-3;
        border: 1px solid $border-light;
        transition: all $duration-fast;

        &:hover {
          border-color: $primary-300;
          box-shadow: $shadow-sm;
        }

        .dark & {
          background: $gray-700;
          border-color: $gray-600;

          &:hover {
            border-color: $primary-500;
          }
        }

        .sample-title {
          font-weight: $font-weight-semibold;
          color: $text-primary;
          margin-bottom: $spacing-2;

          .dark & {
            color: $gray-100;
          }
        }

        .sample-authors {
          font-size: $font-size-xs;
          color: $text-secondary;
        }
      }
    }
  }
}

// Tips Card
.tips-card {
  border-radius: $border-radius-xl;
  border: 1px solid $border-light;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  .tips-content {
    .el-alert {
      :deep(.el-alert__title) {
        font-size: $font-size-sm;
      }

      margin-bottom: $spacing-3 !important;
    }
  }
}

// Responsive Design
@media (max-width: 768px) {
  .template-edit {
    padding: $spacing-4;
  }

  .page-header {
    flex-direction: column;
    gap: $spacing-4;
    padding: $spacing-4 $spacing-5;
    text-align: center;

    .page-title {
      font-size: $font-size-xl;
      justify-content: center;
    }
  }

  .header-actions {
    width: 100%;
    justify-content: center;

    .el-button {
      flex: 1;
    }
  }

  .el-col {
    margin-bottom: $spacing-4;
  }
}

/* Dark mode */
[data-theme="dark"] .template-edit {
  color: #f3f4f6;
}

[data-theme="dark"] .template-edit .page-header,
[data-theme="dark"] .template-edit .page-title,
[data-theme="dark"] .template-edit .card-header {
  color: #f3f4f6;
}

[data-theme="dark"] .template-edit .page-subtitle,
[data-theme="dark"] .template-edit .page-description {
  color: #9ca3af;
}

[data-theme="dark"] .template-edit .stat-card,
[data-theme="dark"] .template-edit .filter-card,
[data-theme="dark"] .template-edit .tasks-card,
[data-theme="dark"] .template-edit .chart-card,
[data-theme="dark"] .template-edit .table-card,
[data-theme="dark"] .template-edit .form-card,
[data-theme="dark"] .template-edit .detail-header,
[data-theme="dark"] .template-edit .detail-content,
[data-theme="dark"] .template-edit .toolbar,
[data-theme="dark"] .template-edit .export-options {
  background: rgba(40, 40, 45, 0.98) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #f3f4f6;
}

[data-theme="dark"] .template-edit .stat-value,
[data-theme="dark"] .template-edit .metric-value {
  color: #f3f4f6;
}

[data-theme="dark"] .template-edit .stat-label,
[data-theme="dark"] .template-edit .metric-label {
  color: #9ca3af;
}

[data-theme="dark"] .template-edit .empty-state,
[data-theme="dark"] .template-edit .empty-text {
  color: #9ca3af;
}
</style>
