<template>
  <div class="review-panel">
    <div class="panel-header">
      <h2>📝 AI审稿人</h2>
      <el-select v-model="reviewType" placeholder="选择审稿类型" style="width: 200px">
        <el-option label="快速审稿" value="quick" />
        <el-option label="详细审稿" value="detailed" />
        <el-option label="同行评审" value="peer" />
      </el-select>
    </div>

    <!-- 论文选择 -->
    <div class="paper-selector">
      <el-input
        v-model="paperId"
        placeholder="输入论文ID"
        type="number"
        style="width: 200px; margin-right: 12px"
      />
      <el-button type="primary" :loading="loading" @click="generateReview">
        生成审稿报告
      </el-button>
    </div>

    <!-- 审稿结果 -->
    <div v-if="reviewResult" class="review-result">
      <!-- 评分卡片 -->
      <div class="score-card">
        <div class="score-header">
          <h3>综合评分</h3>
          <div class="score">{{ reviewResult.score }}/10</div>
        </div>
        <el-progress
          :percentage="reviewResult.score * 10"
          :color="getScoreColor(reviewResult.score)"
          :stroke-width="20"
        />
      </div>

      <!-- 录用概率 -->
      <div class="acceptance-card">
        <h3>录用概率</h3>
        <div class="probability-bar">
          <div
            class="probability-fill"
            :style="{ width: reviewResult.acceptanceProbability + '%' }"
          />
          <span class="probability-text"
            >{{ reviewResult.acceptanceProbability }}%</span
          >
        </div>
      </div>

      <!-- 总体印象 -->
      <div class="impression-card">
        <h3>总体印象</h3>
        <p>{{ reviewResult.overallImpression }}</p>
      </div>

      <!-- 优势 -->
      <div class="strengths-card">
        <h3>✅ 论文优势</h3>
        <ul>
          <li v-for="(strength, index) in reviewResult.strengths" :key="index">
            {{ strength }}
          </li>
        </ul>
      </div>

      <!-- 不足 -->
      <div class="weaknesses-card">
        <h3>❌ 需要改进</h3>
        <ul>
          <li v-for="(weakness, index) in reviewResult.weaknesses" :key="index">
            {{ weakness }}
          </li>
        </ul>
      </div>

      <!-- 改进建议 -->
      <div class="suggestions-card">
        <h3>💡 改进建议</h3>
        <ul>
          <li v-for="(suggestion, index) in reviewResult.improvementSuggestions" :key="index">
            {{ suggestion }}
          </li>
        </ul>
      </div>

      <!-- 对比论文 -->
      <div v-if="reviewResult.comparedPapers?.length" class="comparison-card">
        <h3>📚 对比论文</h3>
        <div class="comparison-list">
          <div
            v-for="paper in reviewResult.comparedPapers"
            :key="paper.id"
            class="comparison-item"
          >
            <div class="paper-title">{{ paper.title }}</div>
            <div class="paper-similarity">相似度: {{ (paper.similarity * 100).toFixed(1) }}%</div>
            <div class="paper-differences">
              <strong>差异:</strong>
              <ul>
                <li v-for="(diff, index) in paper.differences" :key="index">{{ diff }}</li>
              </ul>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 加载状态 -->
    <div v-if="loading" class="loading-state">
      <el-icon class="is-loading"><Loading /></el-icon>
      <p>AI正在审稿中...</p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { ElMessage } from 'element-plus'
import { Loading } from '@element-plus/icons-vue'
import { aiCopilotApi } from '@/api/modules/aiCopilot'
import type { AIReviewResult } from '@/types/ai'

const emit = defineEmits<{
  (e: 'review-generated', review: AIReviewResult): void
}>()

const reviewType = ref<'quick' | 'detailed' | 'peer'>('quick')
const paperId = ref<number | null>(null)
const loading = ref(false)
const reviewResult = ref<AIReviewResult | null>(null)

const generateReview = async () => {
  if (!paperId.value) {
    ElMessage.warning('请输入论文ID')
    return
  }

  loading.value = true
  try {
    const result = await aiCopilotApi.generateReview({
      paperId: paperId.value,
      reviewType: reviewType.value
    })
    reviewResult.value = result
    emit('review-generated', result)
    ElMessage.success('审稿报告生成成功')
  } catch (error) {
    ElMessage.error('生成失败，请稍后重试')
  } finally {
    loading.value = false
  }
}

const getScoreColor = (score: number) => {
  if (score >= 8) return '#67c23a'
  if (score >= 6) return '#e6a23c'
  return '#f56c6c'
}
</script>

<style scoped>
.review-panel {
  padding: 24px;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
}

.panel-header h2 {
  font-size: 24px;
  font-weight: 700;
  margin: 0;
}

.paper-selector {
  display: flex;
  align-items: center;
  margin-bottom: 32px;
  padding: 20px;
  background: #f5f7fa;
  border-radius: 8px;
}

.review-result {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.score-card,
.acceptance-card,
.impression-card,
.strengths-card,
.weaknesses-card,
.suggestions-card,
.comparison-card {
  padding: 24px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.score-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.score-header h3 {
  font-size: 18px;
  font-weight: 600;
  margin: 0;
}

.score {
  font-size: 32px;
  font-weight: 700;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.probability-bar {
  position: relative;
  height: 40px;
  background: #f0f0f0;
  border-radius: 8px;
  overflow: hidden;
  margin-top: 16px;
}

.probability-fill {
  position: absolute;
  left: 0;
  top: 0;
  height: 100%;
  background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
  transition: width 0.5s ease;
}

.probability-text {
  position: absolute;
  left: 50%;
  top: 50%;
  transform: translate(-50%, -50%);
  font-size: 16px;
  font-weight: 600;
  color: #333;
}

.impression-card p {
  line-height: 1.8;
  color: #606266;
  margin: 0;
}

.strengths-card ul,
.weaknesses-card ul,
.suggestions-card ul {
  margin: 12px 0 0 0;
  padding-left: 20px;
}

.strengths-card li,
.weaknesses-card li,
.suggestions-card li {
  line-height: 1.8;
  margin-bottom: 8px;
  color: #606266;
}

.comparison-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
  margin-top: 16px;
}

.comparison-item {
  padding: 16px;
  background: #f5f7fa;
  border-radius: 8px;
}

.paper-title {
  font-weight: 600;
  color: #303133;
  margin-bottom: 8px;
}

.paper-similarity {
  color: #67c23a;
  font-weight: 500;
  margin-bottom: 12px;
}

.paper-differences ul {
  margin: 8px 0 0 0;
  padding-left: 20px;
}

.paper-differences li {
  line-height: 1.6;
  color: #606266;
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
</style>
