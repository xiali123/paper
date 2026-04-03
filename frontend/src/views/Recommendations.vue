<template>
  <div class="recommendations-container">
    <div class="recommendations-header">
      <h1>🎯 个性化推荐</h1>
      <p class="subtitle">基于您的研究兴趣智能推荐论文</p>
    </div>

    <!-- 推荐类型切换 -->
    <div class="recommendation-tabs">
      <button
        v-for="type in recommendationTypes"
        :key="type.key"
        :class="['tab-btn', { active: activeType === type.key }]"
        @click="activeType = type.key"
      >
        {{ type.label }}
      </button>
    </div>

    <!-- 个性化推荐 -->
    <div v-if="activeType === 'personalized'" class="recommendations-content">
      <PersonalizedRecommendations />
    </div>

    <!-- 相似论文 -->
    <div v-else-if="activeType === 'similar'" class="recommendations-content">
      <SimilarPapers />
    </div>

    <!-- 热门论文 -->
    <div v-else-if="activeType === 'trending'" class="recommendations-content">
      <TrendingPapers />
    </div>

    <!-- 用户画像 -->
    <div v-else-if="activeType === 'profile'" class="recommendations-content">
      <UserProfile />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import PersonalizedRecommendations from '@/components/recommendation/PersonalizedRecommendations.vue'
import SimilarPapers from '@/components/recommendation/SimilarPapers.vue'
import TrendingPapers from '@/components/recommendation/TrendingPapers.vue'
import UserProfile from '@/components/recommendation/UserProfile.vue'

const activeType = ref('personalized')

const recommendationTypes = [
  { key: 'personalized', label: '个性化推荐' },
  { key: 'similar', label: '相似论文' },
  { key: 'trending', label: '热门论文' },
  { key: 'profile', label: '用户画像' }
]
</script>

<style scoped>
.recommendations-container {
  max-width: 1400px;
  margin: 0 auto;
  padding: 24px;
}

.recommendations-header {
  margin-bottom: 32px;
}

.recommendations-header h1 {
  font-size: 32px;
  font-weight: 700;
  margin-bottom: 8px;
}

.subtitle {
  color: #6b7280;
  font-size: 16px;
}

.recommendation-tabs {
  display: flex;
  gap: 8px;
  margin-bottom: 32px;
  border-bottom: 2px solid #e5e7eb;
  padding-bottom: 2px;
}

.tab-btn {
  padding: 12px 24px;
  border: none;
  background: transparent;
  cursor: pointer;
  border-radius: 8px 8px 0 0;
  transition: all 0.3s ease;
  font-size: 15px;
  font-weight: 500;
  color: #6b7280;
}

.tab-btn:hover {
  background: #f3f4f6;
  color: #374151;
}

.tab-btn.active {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.recommendations-content {
  min-height: 600px;
}
</style>
