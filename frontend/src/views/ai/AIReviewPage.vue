<template>
  <div class="ai-review-page">
    <!-- Page Header -->
    <div class="page-header">
      <div class="container">
        <nav class="breadcrumb">
          <router-link to="/">Home</router-link>
          <span class="separator">/</span>
          <router-link to="/ai">AI Research Co-Pilot</router-link>
          <span class="separator">/</span>
          <span class="current">AI Reviewer</span>
        </nav>

        <h1 class="page-title">
          <i class="fas fa-robot"></i>
          AI审稿人系统
        </h1>
        <p class="page-subtitle">
          模拟顶级期刊审稿流程，提供专业的同行评审意见和改进建议
        </p>
      </div>
    </div>

    <!-- Page Content -->
    <div class="page-content">
      <div class="container">
        <!-- Info Banner -->
        <div class="info-banner">
          <div class="banner-icon">
            <i class="fas fa-info-circle"></i>
          </div>
          <div class="banner-content">
            <h3>AI审稿人功能介绍</h3>
            <p>
              使用先进的AI技术模拟真实期刊审稿流程，为您提供：
            </p>
            <ul>
              <li><i class="fas fa-check"></i> 专业评分（方法论、创新性、展示质量）</li>
              <li><i class="fas fa-check"></i> 录用概率预测</li>
              <li><i class="fas fa-check"></i> 详细的优缺点分析</li>
              <li><i class="fas fa-check"></i> 与相关论文的对比分析</li>
              <li><i class="fas fa-check"></i> 具体的改进建议</li>
            </ul>
          </div>
          <div class="banner-metrics">
            <div class="metric">
              <span class="value">~15s</span>
              <span class="label">响应时间</span>
            </div>
            <div class="metric">
              <span class="value">95%</span>
              <span class="label">成本降低</span>
            </div>
            <div class="metric">
              <span class="value">&gt;95%</span>
              <span class="label">成功率</span>
            </div>
          </div>
        </div>

        <!-- AI Review Interface Component -->
        <AIReviewInterface />
      </div>
    </div>

    <!-- Page Footer -->
    <div class="page-footer">
      <div class="container">
        <div class="footer-links">
          <router-link to="/ai/literature-review">
            <i class="fas fa-arrow-right"></i>
            文献综述生成器
          </router-link>
          <router-link to="/ai/research-plan">
            <i class="fas fa-arrow-right"></i>
            研究计划助手
          </router-link>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted } from 'vue'
import AIReviewInterface from '@/components/ai/AIReviewInterface.vue'

onMounted(() => {
  document.title = 'AI Reviewer - PaperCrawler AI Research Co-Pilot'
})
</script>

<style scoped>
.ai-review-page {
  min-height: 100vh;
  background: #f5f7fa;
}

.page-header {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  padding: 40px 0;
  margin-bottom: 30px;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 20px;
}

.breadcrumb {
  display: flex;
  align-items: center;
  margin-bottom: 20px;
  font-size: 14px;
}

.breadcrumb a {
  color: rgba(255, 255, 255, 0.8);
  text-decoration: none;
  transition: color 0.3s;
}

.breadcrumb a:hover {
  color: white;
}

.breadcrumb .separator {
  margin: 0 8px;
  color: rgba(255, 255, 255, 0.6);
}

.breadcrumb .current {
  color: white;
  font-weight: 500;
}

.page-title {
  font-size: 2.5rem;
  margin: 0 0 12px 0;
  display: flex;
  align-items: center;
  gap: 12px;
}

.page-subtitle {
  font-size: 1.1rem;
  opacity: 0.9;
  margin: 0;
}

.page-content {
  padding-bottom: 40px;
}

.info-banner {
  background: white;
  border-radius: 12px;
  padding: 24px;
  margin-bottom: 30px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  display: flex;
  gap: 24px;
}

.banner-icon {
  font-size: 48px;
  color: #667eea;
  flex-shrink: 0;
}

.banner-content {
  flex: 1;
}

.banner-content h3 {
  margin: 0 0 12px 0;
  color: #2c3e50;
}

.banner-content p {
  color: #6c757d;
  margin-bottom: 12px;
}

.banner-content ul {
  list-style: none;
  padding: 0;
  margin: 0;
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 8px;
}

.banner-content li {
  color: #495057;
  display: flex;
  align-items: center;
  gap: 8px;
}

.banner-content li i {
  color: #28a745;
}

.banner-metrics {
  display: flex;
  flex-direction: column;
  gap: 16px;
  min-width: 120px;
}

.metric {
  text-align: center;
  padding: 12px;
  background: #f8f9fa;
  border-radius: 8px;
}

.metric .value {
  display: block;
  font-size: 1.5rem;
  font-weight: bold;
  color: #667eea;
}

.metric .label {
  display: block;
  font-size: 12px;
  color: #6c757d;
  margin-top: 4px;
}

.page-footer {
  background: white;
  border-top: 1px solid #e9ecef;
  padding: 20px 0;
  margin-top: 40px;
}

.footer-links {
  display: flex;
  gap: 20px;
  justify-content: center;
}

.footer-links a {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 20px;
  background: #667eea;
  color: white;
  text-decoration: none;
  border-radius: 6px;
  transition: all 0.3s;
}

.footer-links a:hover {
  background: #764ba2;
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
}

@media (max-width: 768px) {
  .page-title {
    font-size: 1.8rem;
  }

  .info-banner {
    flex-direction: column;
  }

  .banner-metrics {
    flex-direction: row;
  }

  .footer-links {
    flex-direction: column;
  }
}
</style>
