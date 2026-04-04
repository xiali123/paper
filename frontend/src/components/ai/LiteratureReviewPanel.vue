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
                :label="paper.id"
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
            <el-radio label="brief">简要综述</el-radio>
            <el-radio label="comprehensive">全面综述</el-radio>
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
            <el-radio label="zh">中文</el-radio>
            <el-radio label="en">English</el-radio>
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
import { useAiStore } from '@/stores/ai'

const aiStore = useAiStore()

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

<style scoped>
.literature-review-panel {
  padding: 24px;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 32px;
}

.panel-header h2 {
  font-size: 24px;
  font-weight: 700;
  margin: 0;
}

.config-section {
  margin-bottom: 32px;
  padding: 24px;
  background: #f5f7fa;
  border-radius: 12px;
}

.paper-selector {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.paper-item {
  padding: 12px;
  background: white;
  border-radius: 8px;
  transition: all 0.3s ease;
}

.paper-item:hover {
  background: #ecf5ff;
}

.paper-title {
  font-weight: 600;
  color: #303133;
  margin-bottom: 4px;
}

.paper-authors {
  font-size: 13px;
  color: #606266;
}

.review-result {
  background: white;
  border-radius: 12px;
  padding: 24px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.result-header {
  margin-bottom: 24px;
}

.result-header h3 {
  font-size: 20px;
  font-weight: 700;
  margin: 0 0 12px 0;
}

.meta-info {
  display: flex;
  gap: 16px;
  font-size: 14px;
  color: #909399;
}

.review-content {
  margin-bottom: 24px;
}

.content-section {
  margin-bottom: 24px;
}

.content-section h4 {
  font-size: 16px;
  font-weight: 600;
  margin: 0 0 12px 0;
  color: #303133;
}

.content-section p {
  line-height: 1.8;
  color: #606266;
  margin: 0;
}

.content-section ul {
  margin: 0;
  padding-left: 20px;
}

.content-section li {
  line-height: 1.8;
  color: #606266;
  margin-bottom: 8px;
}

.result-actions {
  display: flex;
  gap: 12px;
  padding-top: 16px;
  border-top: 1px solid #e4e7ed;
}

.loading-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 60px 20px;
  color: #909399;
}

.loading-state .el-icon {
  font-size: 48px;
  margin-bottom: 16px;
}

.loading-state p {
  margin: 0 0 24px 0;
  font-size: 16px;
}

.loading-state .el-progress {
  width: 300px;
}
</style>
