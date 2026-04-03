<template>
  <div class="recommendation-card" @click="handleClick">
    <div class="card-header">
      <div class="confidence-badge" :style="{ background: confidenceColor }">
        {{ (recommendation.confidence * 100).toFixed(0) }}% 匹配
      </div>
      <el-button text @click.stop="toggleFavorite">
        <el-icon :class="{ 'is-favorite': isFavorite }">
          <Star />
        </el-icon>
      </el-button>
    </div>

    <div class="card-body">
      <h3 class="paper-title">{{ recommendation.paper.title }}</h3>

      <div class="paper-authors">
        {{ recommendation.paper.authors.join(', ') }}
      </div>

      <div class="paper-meta">
        <span class="year">{{ recommendation.paper.year }}</span>
        <span class="citations"
          >📊 {{ recommendation.paper.citationCount }} 引用</span
        >
      </div>

      <div class="paper-abstract">
        {{ truncateText(recommendation.paper.abstract, 150) }}
      </div>

      <div class="recommendation-reason">
        <el-icon><InfoFilled /></el-icon>
        <span>{{ recommendation.reason }}</span>
      </div>
    </div>

    <div class="card-footer">
      <el-button type="primary" link @click.stop="viewPaper">查看详情</el-button>
      <el-button link @click.stop="provideFeedback">反馈</el-button>
    </div>

    <!-- 反馈对话框 -->
    <el-dialog v-model="feedbackVisible" title="推荐反馈" width="400px">
      <el-form label-position="top">
        <el-form-item label="这篇推荐对您有帮助吗？">
          <el-radio-group v-model="feedback.liked">
            <el-radio :label="true">👍 有帮助</el-radio>
            <el-radio :label="false">👎 没帮助</el-radio>
          </el-radio-group>
        </el-form-item>
        <el-form-item label="评分">
          <el-rate v-model="feedback.rating" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="feedbackVisible = false">取消</el-button>
        <el-button type="primary" @click="submitFeedback">提交</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { Star, InfoFilled } from '@element-plus/icons-vue'
import { recommendationsApi } from '@/api/modules/recommendations'
import type { RecommendationResult } from '@/types/recommendation'

interface Props {
  recommendation: RecommendationResult
}

const props = defineProps<Props>()
const router = useRouter()

const isFavorite = ref(false)
const feedbackVisible = ref(false)
const feedback = ref({
  liked: false,
  rating: 0
})

const confidenceColor = computed(() => {
  const confidence = props.recommendation.confidence
  if (confidence >= 0.8) return 'linear-gradient(135deg, #67c23a 0%, #85ce61 100%)'
  if (confidence >= 0.6) return 'linear-gradient(135deg, #e6a23c 0%, #f0c78a 100%)'
  return 'linear-gradient(135deg, #909399 0%, #b1b3b8 100%)'
})

const handleClick = () => {
  viewPaper()
}

const viewPaper = () => {
  router.push(`/papers/${props.recommendation.paperId}`)
}

const toggleFavorite = () => {
  isFavorite.value = !isFavorite.value
  ElMessage.success(isFavorite.value ? '已收藏' : '已取消收藏')
}

const provideFeedback = () => {
  feedbackVisible.value = true
}

const submitFeedback = async () => {
  try {
    await recommendationsApi.submitFeedback({
      userId: 0, // 从Pinia获取
      paperId: props.recommendation.paperId,
      liked: feedback.value.liked,
      rating: feedback.value.rating
    })
    ElMessage.success('感谢您的反馈')
    feedbackVisible.value = false
  } catch (error) {
    ElMessage.error('提交失败，请稍后重试')
  }
}

const truncateText = (text: string, maxLength: number) => {
  if (text.length <= maxLength) return text
  return text.substring(0, maxLength) + '...'
}
</script>

<style scoped>
.recommendation-card {
  background: white;
  border-radius: 12px;
  padding: 20px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  transition: all 0.3s ease;
  cursor: pointer;
}

.recommendation-card:hover {
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.12);
  transform: translateY(-2px);
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.confidence-badge {
  padding: 4px 12px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 600;
  color: white;
}

.card-body {
  margin-bottom: 16px;
}

.paper-title {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
  margin: 0 0 8px 0;
  line-height: 1.5;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.paper-authors {
  font-size: 14px;
  color: #606266;
  margin-bottom: 8px;
  display: -webkit-box;
  -webkit-line-clamp: 1;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.paper-meta {
  display: flex;
  gap: 12px;
  font-size: 13px;
  color: #909399;
  margin-bottom: 12px;
}

.paper-abstract {
  font-size: 14px;
  color: #606266;
  line-height: 1.6;
  margin-bottom: 12px;
}

.recommendation-reason {
  display: flex;
  align-items: flex-start;
  gap: 6px;
  padding: 12px;
  background: #f0f9ff;
  border-left: 3px solid #409eff;
  border-radius: 4px;
}

.recommendation-reason .el-icon {
  flex-shrink: 0;
  margin-top: 2px;
}

.recommendation-reason span {
  font-size: 13px;
  color: #409eff;
  line-height: 1.5;
}

.card-footer {
  display: flex;
  justify-content: space-between;
  padding-top: 12px;
  border-top: 1px solid #e4e7ed;
}

.is-favorite {
  color: #f56c6c;
}
</style>
