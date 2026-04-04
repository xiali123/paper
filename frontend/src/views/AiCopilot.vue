<template>
  <div class="ai-copilot-container">
    <!-- 顶部导航 -->
    <div class="ai-header">
      <h1>🤖 AI研究副驾驶</h1>
      <div class="ai-tabs">
        <button
          v-for="tab in tabs"
          :key="tab.key"
          :class="['tab-btn', { active: activeTab === tab.key }]"
          @click="activeTab = tab.key"
        >
          <span class="tab-icon">{{ tab.icon }}</span>
          <span class="tab-label">{{ tab.label }}</span>
        </button>
      </div>
    </div>

    <!-- AI审稿人 -->
    <div v-if="activeTab === 'review'" class="ai-section">
      <ReviewPanel :paper-id="selectedPaperId" @review-generated="handleReviewGenerated" />
    </div>

    <!-- 文献综述 -->
    <div v-else-if="activeTab === 'literature'" class="ai-section">
      <LiteratureReviewPanel @review-generated="handleLiteratureReview" />
    </div>

    <!-- 研究规划 -->
    <div v-else-if="activeTab === 'plan'" class="ai-section">
      <ResearchPlanPanel @plan-generated="handleResearchPlan" />
    </div>

    <!-- AI对话 -->
    <div v-else-if="activeTab === 'chat'" class="ai-section">
      <AIChatPanel />
    </div>

    <!-- 历史记录 -->
    <div v-else-if="activeTab === 'history'" class="ai-section">
      <AIHistoryPanel />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import ReviewPanel from '@/components/ai/ReviewPanel.vue'
import LiteratureReviewPanel from '@/components/ai/LiteratureReviewPanel.vue'
import ResearchPlanPanel from '@/components/ai/ResearchPlanPanel.vue'
import AIChatPanel from '@/components/ai/AIChatPanel.vue'
import AIHistoryPanel from '@/components/ai/AIHistoryPanel.vue'

const activeTab = ref('review')
const selectedPaperId = ref<number | null>(null)

const tabs = [
  { key: 'review', label: 'AI审稿人', icon: '📝' },
  { key: 'literature', label: '文献综述', icon: '📚' },
  { key: 'plan', label: '研究规划', icon: '🎯' },
  { key: 'chat', label: 'AI对话', icon: '💬' },
  { key: 'history', label: '历史记录', icon: '📋' }
]

const handleReviewGenerated = (review: any) => {
  console.log('Review generated:', review)
}

const handleLiteratureReview = (review: any) => {
  console.log('Literature review generated:', review)
}

const handleResearchPlan = (plan: any) => {
  console.log('Research plan generated:', plan)
}
</script>

<style scoped>
.ai-copilot-container {
  max-width: 1400px;
  margin: 0 auto;
  padding: 24px;
}

.ai-header {
  margin-bottom: 32px;
}

.ai-header h1 {
  font-size: 32px;
  font-weight: 700;
  margin-bottom: 24px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.ai-tabs {
  display: flex;
  gap: 8px;
  border-bottom: 2px solid #e5e7eb;
  padding-bottom: 2px;
}

.tab-btn {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 20px;
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

.tab-icon {
  font-size: 18px;
}

.ai-section {
  min-height: 600px;
}
</style>
