<template>
  <div class="ai-literature-review-page">
    <!-- Page Header -->
    <div class="page-header">
      <div class="container">
        <nav class="breadcrumb">
          <router-link to="/">Home</router-link>
          <span class="separator">/</span>
          <router-link to="/ai">AI Research Co-Pilot</router-link>
          <span class="separator">/</span>
          <span class="current">Literature Review</span>
        </nav>

        <h1 class="page-title">
          <i class="fas fa-book-open"></i>
          AI文献综述生成器
        </h1>
        <p class="page-subtitle">
          自动生成系统性文献综述，符合PRISMA指南，节省您的研究时间
        </p>
      </div>
    </div>

    <!-- Page Content -->
    <div class="page-content">
      <div class="container">
        <!-- Info Banner -->
        <div class="info-banner">
          <div class="banner-icon">
            <i class="fas fa-book-reader"></i>
          </div>
          <div class="banner-content">
            <h3>AI文献综述功能介绍</h3>
            <p>
              基于先进的AI技术，快速生成高质量的系统性文献综述：
            </p>
            <ul>
              <li><i class="fas fa-check"></i> 自动主题聚类和关键发现提取</li>
              <li><i class="fas fa-check"></i> 研究空白识别和趋势分析</li>
              <li><i class="fas fa-check"></i> 方法论总结和演进</li>
              <li><i class="fas fa-check"></i> 未来研究方向预测</li>
              <li><i class="fas fa-check"></i> PRISMA指南兼容格式</li>
            </ul>
          </div>
          <div class="banner-metrics">
            <div class="metric">
              <span class="value">~20s</span>
              <span class="label">生成时间</span>
            </div>
            <div class="metric">
              <span class="value">500</span>
              <span class="label">最大论文数</span>
            </div>
            <div class="metric">
              <span class="value">95%</span>
              <span class="label">成本降低</span>
            </div>
          </div>
        </div>

        <!-- Literature Review Display Component -->
        <LiteratureReviewDisplay />
      </div>
    </div>

    <!-- Page Footer -->
    <div class="page-footer">
      <div class="container">
        <div class="footer-links">
          <router-link to="/ai/review">
            <i class="fas fa-arrow-left"></i>
            AI审稿人
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
import LiteratureReviewDisplay from '@/components/ai/LiteratureReviewDisplay.vue'

onMounted(() => {
  document.title = 'Literature Review - PaperCrawler AI Research Co-Pilot'
})
</script>

<style scoped>
.ai-literature-review-page {
  min-height: 100vh;
  background: #f5f7fa;
}

.page-header {
  background: linear-gradient(135deg, #28a745 0%, #20c997 100%);
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
  color: #28a745;
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
  color: #28a745;
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
  background: #28a745;
  color: white;
  text-decoration: none;
  border-radius: 6px;
  transition: all 0.3s;
}

.footer-links a:hover {
  background: #20c997;
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(40, 167, 69, 0.4);
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
