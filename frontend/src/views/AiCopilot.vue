<template>
  <div class="ai-copilot-container">
    <!-- Page Header -->
    <div class="page-header">
      <div class="header-content">
        <div class="header-title-group">
          <span class="header-icon">🤖</span>
          <h1 class="page-title">AI研究副驾驶</h1>
        </div>
        <div class="header-description">
          智能文献分析、综述生成与研究规划助手
        </div>
      </div>
    </div>

    <!-- Tab Navigation -->
    <div class="tab-navigation">
      <el-radio-group v-model="activeTab" size="default">
        <el-radio-button value="review">
          <span class="tab-icon">📝</span>
          <span class="tab-label">AI审稿人</span>
        </el-radio-button>
        <el-radio-button value="literature">
          <span class="tab-icon">📚</span>
          <span class="tab-label">文献综述</span>
        </el-radio-button>
        <el-radio-button value="plan">
          <span class="tab-icon">🎯</span>
          <span class="tab-label">研究规划</span>
        </el-radio-button>
        <el-radio-button value="chat">
          <span class="tab-icon">💬</span>
          <span class="tab-label">AI对话</span>
        </el-radio-button>
        <el-radio-button value="history">
          <span class="tab-icon">📋</span>
          <span class="tab-label">历史记录</span>
        </el-radio-button>
      </el-radio-group>
    </div>

    <!-- Content Sections -->
    <div class="content-wrapper">
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
  if (import.meta.env.DEV) console.log('Review generated:', review)
}

const handleLiteratureReview = (review: any) => {
  if (import.meta.env.DEV) console.log('Literature review generated:', review)
}

const handleResearchPlan = (plan: any) => {
  if (import.meta.env.DEV) console.log('Research plan generated:', plan)
}
</script>

<style scoped lang="scss">
.ai-copilot-container {
  width: 100%;
  max-width: 1920px;
  margin: 0 auto;
  padding: 8px;
}

// Page Header
.page-header {
  margin-bottom: 16px;
  padding: 16px 20px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(102, 126, 234, 0.15);
}

.header-content {
  text-align: center;
}

.header-title-group {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12px;
  margin-bottom: 8px;
}

.header-icon {
  font-size: 36px;
}

.page-title {
  font-size: $font-size-3xl;
  font-weight: 700;
  color: white;
  margin: 0;
}

.header-description {
  font-size: 14px;
  color: rgba(255, 255, 255, 0.9);
  font-weight: 400;
}

// Tab Navigation
.tab-navigation {
  margin-bottom: 16px;

  :deep(.el-radio-group) {
    display: flex;
    width: 100%;
    gap: 8px;
    background: white;
    padding: 6px;
    border-radius: 10px;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
    border: 1px solid #e5e7eb;
  }

  :deep(.el-radio-button) {
    flex: 1;
    margin: 0;

    .el-radio-button__inner {
      width: 100%;
      padding: 8px 12px;
      border: none;
      background: transparent;
      color: #6b7280;
      font-size: 13px;
      font-weight: 500;
      border-radius: 6px;
      transition: all 0.3s;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;

      &:hover {
        background: #f3f4f6;
        color: #5568d3;
        font-weight: 600;
      }
    }

    &.is-active .el-radio-button__inner {
      background: linear-gradient(135deg, #5a67d8 0%, #6b46a0 100%);
      color: #ffffff;
      font-weight: 700;
      box-shadow: 0 2px 8px rgba(102, 126, 234, 0.3);
      text-shadow: 0 1px 3px rgba(0, 0, 0, 0.2);
    }
  }
}

.tab-icon {
  font-size: 16px;
  line-height: 1;
}

.tab-label {
  font-size: 13px;
}

// Content Wrapper
.content-wrapper {
  min-height: 500px;
}

.ai-section {
  animation: fadeIn 0.3s ease-out;
}

@keyframes fadeIn {
  from {
    opacity: 0;
    transform: translateY(10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

// Responsive Design
@media (max-width: 768px) {
  .page-header {
    padding: 12px 16px;
  }

  .header-icon {
    font-size: $font-size-3xl;
  }

  .page-title {
    font-size: 22px;
  }

  .header-description {
    font-size: 13px;
  }

  .tab-navigation {
    :deep(.el-radio-group) {
      flex-wrap: wrap;
    }

    :deep(.el-radio-button) {
      flex: 1 1 40%;
      min-width: 120px;
    }
  }

  .tab-label {
    font-size: 12px;
  }
}
</style>
