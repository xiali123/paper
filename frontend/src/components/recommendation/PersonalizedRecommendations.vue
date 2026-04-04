<template>
  <div class="personalized-recommendations">
    <div class="list-header">
      <h3>🎯 为您推荐</h3>
      <div class="header-actions">
        <el-button size="small" @click="refreshRecommendations">
          <el-icon><Refresh /></el-icon>
          刷新
        </el-button>
        <el-button size="small" @click="tuneRecommendations">
          <el-icon><Setting /></el-icon>
          调优
        </el-button>
      </div>
    </div>

    <!-- 推荐论文列表 -->
    <div v-if="recommendations.length" class="recommendations-list">
      <div
        v-for="rec in recommendations"
        :key="rec.paperId"
        class="recommendation-item"
      >
        <div class="item-header">
          <div class="confidence-badge" :style="{ background: getConfidenceColor(rec.confidence) }">
            {{ (rec.confidence * 100).toFixed(0) }}% 匹配
          </div>
          <div class="item-actions">
            <el-button text @click="viewPaper(rec.paperId)">
              <el-icon><View /></el-icon>
            </el-button>
            <el-button text @click="favoritePaper(rec.paperId)">
              <el-icon :class="{ 'is-favorite': rec.isFavorite }">
                <Star />
              </el-icon>
            </el-button>
            <el-button text @click="showReason(rec)">
              <el-icon><InfoFilled /></el-icon>
            </el-button>
          </div>
        </div>

        <div class="item-content">
          <h4 class="paper-title">{{ rec.paper.title }}</h4>
          <div class="paper-meta">
            <span class="authors">{{ rec.paper.authors.join(', ') }}</span>
            <span class="year">{{ rec.paper.year }}</span>
            <span class="citations">
              <el-icon><DocumentCopy /></el-icon>
              {{ rec.paper.citationCount }}
            </span>
          </div>
          <p class="paper-abstract">{{ truncateText(rec.paper.abstract, 200) }}</p>

          <div class="recommendation-reason" v-if="rec.showReason">
            <el-alert
              :title="rec.reason"
              type="info"
              :closable="false"
              show-icon
            />
          </div>
        </div>
      </div>
    </div>

    <!-- 空状态 -->
    <div v-else class="empty-state">
      <el-empty
        description="暂无推荐论文，请调整推荐设置或浏览更多论文"
      >
        <el-button type="primary" @click="goToBrowse">浏览论文</el-button>
      </el-empty>
    </div>

    <!-- 加载状态 -->
    <div v-if="loading" class="loading-state">
      <el-skeleton :rows="3" animated />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { Refresh, Setting, View, Star, InfoFilled, DocumentCopy } from '@element-plus/icons-vue'
import { useRecommendationsStore } from '@/stores/recommendations'
import type { RecommendationResult } from '@/types/recommendation'

const router = useRouter()
const recommendationsStore = useRecommendationsStore()

const recommendations = ref<RecommendationResult[]>([])
const loading = ref(false)

const loadRecommendations = async () => {
  loading.value = true
  try {
    const userId = 0 // 从Pinia获取
    const results = await recommendationsStore.loadPersonalized(userId, 20)
    recommendations.value = results.map((rec: any) => ({
      ...rec,
      isFavorite: false,
      showReason: false
    }))
  } catch (error) {
    ElMessage.error('加载推荐失败')
  } finally {
    loading.value = false
  }
}

const refreshRecommendations = () => {
  loadRecommendations()
}

const tuneRecommendations = () => {
  ElMessage.info('推荐调优功能开发中...')
}

const viewPaper = (paperId: number) => {
  router.push(`/papers/${paperId}`)
}

const favoritePaper = async (paperId: number) => {
  const rec = recommendations.value.find(r => r.paperId === paperId)
  if (rec) {
    rec.isFavorite = !rec.isFavorite
    ElMessage.success(rec.isFavorite ? '已收藏' : '已取消收藏')
  }
}

const showReason = (rec: RecommendationResult & { showReason: boolean }) => {
  rec.showReason = !rec.showReason
}

const goToBrowse = () => {
  router.push('/papers')
}

const getConfidenceColor = (confidence: number) => {
  if (confidence >= 0.8) return 'linear-gradient(135deg, #67c23a 0%, #85ce61 100%)'
  if (confidence >= 0.6) return 'linear-gradient(135deg, #e6a23c 0%, #f0c78a 100%)'
  return 'linear-gradient(135deg, #909399 0%, #b1b3b8 100%)'
}

const truncateText = (text: string, maxLength: number) => {
  if (text.length <= maxLength) return text
  return text.substring(0, maxLength) + '...'
}

onMounted(() => {
  loadRecommendations()
})
</script>

<style scoped>
.personalized-recommendations {
  padding: 20px;
}

.list-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.list-header h3 {
  font-size: 18px;
  font-weight: 600;
  margin: 0;
}

.header-actions {
  display: flex;
  gap: 8px;
}

.recommendations-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.recommendation-item {
  background: white;
  border-radius: 12px;
  padding: 20px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  transition: all 0.3s ease;
  cursor: pointer;
}

.recommendation-item:hover {
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.12);
  transform: translateY(-2px);
}

.item-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.confidence-badge {
  padding: 4px 12px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 600;
  color: white;
}

.item-actions {
  display: flex;
  gap: 4px;
}

.item-content {
  margin-bottom: 12px;
}

.paper-title {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
  margin: 0 0 8px 0;
  line-height: 1.5;
}

.paper-meta {
  display: flex;
  gap: 12px;
  font-size: 13px;
  color: #909399;
  margin-bottom: 12px;
  align-items: center;
}

.paper-meta .el-icon {
  font-size: 14px;
  margin-right: 2px;
}

.paper-abstract {
  font-size: 14px;
  color: #606266;
  line-height: 1.6;
  margin: 0 0 12px 0;
}

.recommendation-reason {
  margin-top: 12px;
}

.is-favorite {
  color: #f56c6c;
}

.empty-state {
  padding: 60px 20px;
  text-align: center;
}

.loading-state {
  padding: 20px;
}
</style>
