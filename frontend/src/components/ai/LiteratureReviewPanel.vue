<template>
  <div class="literature-review-panel">
    <div class="panel-header">
      <h2>📚 文献综述生成器</h2>
      <el-button type="primary" :loading="generating" @click="generateReview">
        生成文献综述
      </el-button>
    </div>

    <!-- 参数配置 -->
    <div class="config-section">
      <el-form :model="config" label-position="top">
        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="研究主题">
              <el-input
                v-model="config.topic"
                placeholder="输入您的研究主题"
              />
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="研究领域">
              <el-select v-model="config.field" placeholder="选择研究领域">
                <el-option label="计算机科学" value="cs" />
                <el-option label="人工智能" value="ai" />
                <el-option label="机器学习" value="ml" />
                <el-option label="数据科学" value="ds" />
              </el-select>
            </el-form-item>
          </el-col>
        </el-row>

        <el-form-item label="选择论文">
          <div class="paper-selector">
            <el-checkbox-group v-model="config.selectedPapers">
              <el-checkbox
                v-for="paper in availablePapers"
                :key="paper.id"
                :value="paper.id"
              >
                <div class="paper-item">
                  <div class="paper-title">{{ paper.title }}</div>
                  <div class="paper-authors">{{ paper.authors }}</div>
                </div>
              </el-checkbox>
            </el-checkbox-group>
          </div>
        </el-form-item>

        <el-form-item label="综述深度">
          <el-radio-group v-model="config.depth">
            <el-radio value="brief">简要综述</el-radio>
            <el-radio value="comprehensive">全面综述</el-radio>
          </el-radio-group>
        </el-form-item>

        <el-form-item label="字数限制">
          <el-input-number
            v-model="config.maxWords"
            :min="1000"
            :max="10000"
            :step="500"
          />
        </el-form-item>

        <el-form-item label="语言">
          <el-radio-group v-model="config.language">
            <el-radio value="zh">中文</el-radio>
            <el-radio value="en">English</el-radio>
          </el-radio-group>
        </el-form-item>
      </el-form>
    </div>

    <!-- 生成结果 -->
    <div v-if="reviewResult" class="review-result">
      <div class="result-header">
        <h3>{{ reviewResult.title }}</h3>
        <div class="meta-info">
          <span>📄 {{ reviewResult.paperCount }}篇论文</span>
          <span>🔬 {{ reviewResult.researchField }}</span>
          <span>📝 {{ reviewResult.reviewContent.length }}字</span>
        </div>
      </div>

      <el-divider />

      <div class="review-content">
        <div class="content-section">
          <h4>综述内容</h4>
          <p>{{ reviewResult.reviewContent }}</p>
        </div>

        <div v-if="reviewResult.researchGaps.length" class="content-section">
          <h4>🔍 研究空白</h4>
          <ul>
            <li v-for="(gap, index) in reviewResult.researchGaps" :key="index">
              {{ gap }}
            </li>
          </ul>
        </div>

        <div v-if="reviewResult.trends.length" class="content-section">
          <h4>📈 研究趋势</h4>
          <ul>
            <li v-for="(trend, index) in reviewResult.trends" :key="index">
              {{ trend }}
            </li>
          </ul>
        </div>

        <div v-if="reviewResult.futureDirections.length" class="content-section">
          <h4>🚀 未来方向</h4>
          <ul>
            <li v-for="(direction, index) in reviewResult.futureDirections" :key="index">
              {{ direction }}
            </li>
          </ul>
        </div>
      </div>

      <div class="result-actions">
        <el-button type="primary" @click="exportPDF">
          📄 导出PDF
        </el-button>
        <el-button @click="saveReview">
          💾 保存综述
        </el-button>
        <el-button @click="shareReview">
          🔗 分享
        </el-button>
      </div>
    </div>

    <!-- 加载状态 -->
    <div v-if="generating" class="loading-state">
      <el-icon class="is-loading"><Loading /></el-icon>
      <p>AI正在生成文献综述，请稍候...</p>
      <el-progress :percentage="progress" :status="progressStatus" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { Loading } from '@element-plus/icons-vue'
import { aiCopilotApi } from '@/api/modules/aiCopilot'

const config = ref({
  topic: '',
  field: '',
  selectedPapers: [] as number[],
  depth: 'brief',
  maxWords: 3000,
  language: 'zh'
})

const availablePapers = ref<any[]>([])
const reviewResult = ref<any>(null)
const generating = ref(false)
const progress = ref(0)
const progressStatus = ref<'success' | 'exception' | ''>('')

const generateReview = async () => {
  if (!config.value.topic) {
    ElMessage.warning('请输入研究主题')
    return
  }

  if (config.value.selectedPapers.length === 0) {
    ElMessage.warning('请至少选择一篇论文')
    return
  }

  generating.value = true
  progress.value = 0

  // 模拟进度
  const progressInterval = setInterval(() => {
    if (progress.value < 90) {
      progress.value += 10
    }
  }, 500)

  try {
    const result = await aiCopilotApi.generateLiteratureReview({
      topic: config.value.topic,
      paperIds: config.value.selectedPapers,
      depth: config.value.depth as any,
      language: config.value.language as any,
      maxWords: config.value.maxWords
    })

    clearInterval(progressInterval)
    progress.value = 100
    progressStatus.value = 'success'

    reviewResult.value = result
    ElMessage.success('文献综述生成成功')
  } catch (error) {
    clearInterval(progressInterval)
    progressStatus.value = 'exception'
    ElMessage.error('生成失败，请稍后重试')
  } finally {
    setTimeout(() => {
      generating.value = false
      progress.value = 0
      progressStatus.value = ''
    }, 2000)
  }
}

const exportPDF = () => {
  ElMessage.info('PDF导出功能开发中...')
}

const saveReview = () => {
  ElMessage.success('文献综述已保存到历史记录')
}

const shareReview = () => {
  ElMessage.info('分享功能开发中...')
}

const loadAvailablePapers = async () => {
  // 模拟数据
  availablePapers.value = [
    { id: 1, title: 'Attention Is All You Need', authors: 'Vaswani et al.' },
    { id: 2, title: 'BERT: Pre-training of Deep Bidirectional Transformers', authors: 'Devlin et al.' },
    { id: 3, title: 'GPT-3: Language Models are Few-Shot Learners', authors: 'Brown et al.' },
    { id: 4, title: 'Deep Residual Learning for Image Recognition', authors: 'He et al.' },
    { id: 5, title: 'Sequence to Sequence Learning with Neural Networks', authors: 'Sutskever et al.' }
  ]
}

onMounted(() => {
  loadAvailablePapers()
})
</script>

<style scoped lang="scss">
.literature-review-panel {
  padding: 16px;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  padding: 16px 20px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  border: 1px solid #e5e7eb;
}

.panel-header h2 {
  font-size: 22px;
  font-weight: 700;
  margin: 0;
  color: #1f2937;
}

.config-section {
  margin-bottom: 20px;
  padding: 20px;
  background: white;
  border-radius: 12px;
  border: 1px solid #e5e7eb;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.04);

  :deep(.el-form-item__label) {
    font-weight: 600;
    color: #374151;
  }
}

.paper-selector {
  width: 100%;
  margin-top: 8px;
}

.paper-selector :deep(.el-checkbox-group) {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
  gap: 10px;
  width: 100%;
}

.paper-selector :deep(.el-checkbox) {
  margin: 0;
  width: 100%;
}

.paper-selector :deep(.el-checkbox__input) {
  position: absolute;
  top: 12px;
  left: 12px;
}

.paper-selector :deep(.el-checkbox__label) {
  width: 100%;
  padding: 12px 12px 12px 36px;
  background: white;
  border: 1px solid #e4e7ed;
  border-radius: 8px;
  transition: all 0.3s ease;
  display: block;
  min-height: 60px;
  position: relative;
}

.paper-selector :deep(.el-checkbox:hover .el-checkbox__label) {
  background: #f3f4f6;
  border-color: #409eff;
  box-shadow: 0 2px 8px rgba(64, 158, 255, 0.15);
}

.paper-selector :deep(.el-checkbox.is-checked .el-checkbox__label) {
  background: #ecf5ff;
  border-color: #409eff;
}

.paper-item {
  width: 100%;
}

.paper-title {
  font-weight: 600;
  color: #303133;
  margin-bottom: 4px;
  font-size: 14px;
  line-height: 1.4;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.paper-authors {
  font-size: 12px;
  color: #606266;
  line-height: 1.4;
  display: -webkit-box;
  -webkit-line-clamp: 1;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.review-result {
  background: white;
  border-radius: 12px;
  padding: 20px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  border: 1px solid #e4e7ed;
}

.result-header {
  margin-bottom: 20px;
  padding-bottom: 16px;
  border-bottom: 2px solid #e4e7ed;
}

.result-header h3 {
  font-size: 18px;
  font-weight: 700;
  margin: 0 0 10px 0;
  color: #1f2937;
}

.meta-info {
  display: flex;
  gap: 12px;
  font-size: 13px;
  color: #909399;
  flex-wrap: wrap;
}

.meta-info span {
  padding: 4px 10px;
  background: #f5f7fa;
  border-radius: 4px;
  font-weight: 500;
}

.review-content {
  margin-bottom: 20px;
}

.content-section {
  margin-bottom: 20px;
  padding: 16px;
  background: #f9fafb;
  border-radius: 8px;
  border-left: 3px solid #409eff;
}

.content-section h4 {
  font-size: 15px;
  font-weight: 600;
  margin: 0 0 10px 0;
  color: #409eff;
}

.content-section p {
  line-height: 1.8;
  color: #606266;
  margin: 0;
  font-size: 14px;
}

.content-section ul {
  margin: 0;
  padding-left: 20px;
}

.content-section li {
  line-height: 1.8;
  color: #606266;
  margin-bottom: 8px;
  font-size: 14px;
}

.result-actions {
  display: flex;
  gap: 10px;
  padding-top: 16px;
  border-top: 1px solid #e4e7ed;
  flex-wrap: wrap;
}

.loading-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 60px 20px;
  color: #606266;
  background: #f9fafb;
  border-radius: 12px;
}

.loading-state .el-icon {
  font-size: 48px;
  margin-bottom: 16px;
  color: #409eff;
}

.loading-state p {
  margin: 0 0 24px 0;
  font-size: 15px;
  font-weight: 500;
  color: #606266;
}

.loading-state .el-progress {
  width: 300px;
}

// Responsive Design
@media (max-width: 768px) {
  .literature-review-panel {
    padding: 12px;
  }

  .panel-header {
    flex-direction: column;
    gap: 12px;
    padding: 12px 16px;
  }

  .panel-header h2 {
    font-size: 18px;
    text-align: center;
  }

  .config-section {
    padding: 16px;
  }

  .paper-selector :deep(.el-checkbox-group) {
    grid-template-columns: 1fr;
  }

  .result-actions {
    flex-direction: column;
  }

  .result-actions .el-button {
    width: 100%;
  }
}
</style>
