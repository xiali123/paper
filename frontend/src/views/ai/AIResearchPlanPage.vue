<template>
  <div class="ai-research-plan-page">
    <!-- Page Header -->
    <div class="page-header">
      <div class="container">
        <nav class="breadcrumb">
          <router-link to="/">Home</router-link>
          <span class="separator">/</span>
          <router-link to="/ai">AI Research Co-Pilot</router-link>
          <span class="separator">/</span>
          <span class="current">Research Plan</span>
        </nav>

        <h1 class="page-title">
          <i class="fas fa-clipboard-list"></i>
          AI研究计划助手
        </h1>
        <p class="page-subtitle">
          生成完整的研究项目计划，包含SMART目标、方法论设计和时间安排
        </p>
      </div>
    </div>

    <!-- Page Content -->
    <div class="page-content">
      <div class="container">
        <!-- Info Banner -->
        <div class="info-banner">
          <div class="banner-icon">
            <i class="fas fa-lightbulb"></i>
          </div>
          <div class="banner-content">
            <h3>AI研究计划功能介绍</h3>
            <p>
              使用AI技术帮助您快速创建专业的研究项目计划书：
            </p>
            <ul>
              <li><i class="fas fa-check"></i> SMART目标自动设定</li>
              <li><i class="fas fa-check"></i> 详细的方法论设计</li>
              <li><i class="fas fa-check"></i> 可视化时间线和阶段规划</li>
              <li><i class="fas fa-check"></i> 资源需求和预算评估</li>
              <li><i class="fas fa-check"></i> 风险评估和缓解措施</li>
            </ul>
          </div>
          <div class="banner-metrics">
            <div class="metric">
              <span class="value">~18s</span>
              <span class="label">生成时间</span>
            </div>
            <div class="metric">
              <span class="value">60</span>
              <span class="label">最大周期(月)</span>
            </div>
            <div class="metric">
              <span class="value">95%</span>
              <span class="label">成本降低</span>
            </div>
          </div>
        </div>

        <!-- Research Plan Visualization Component -->
        <ResearchPlanVisualization />
      </div>
    </div>

    <!-- Page Footer -->
    <div class="page-footer">
      <div class="container">
        <div class="footer-links">
          <router-link to="/ai/literature-review">
            <i class="fas fa-arrow-left"></i>
            文献综述生成器
          </router-link>
          <router-link to="/ai/review">
            <i class="fas fa-arrow-left"></i>
            AI审稿人
          </router-link>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted } from 'vue'
import ResearchPlanVisualization from '@/components/ai/ResearchPlanVisualization.vue'

onMounted(() => {
  document.title = 'Research Plan - PaperCrawler AI Research Co-Pilot'
})
</script>

<style scoped>
.ai-research-plan-page {
  min-height: 100vh;
  background: #f5f7fa;
}

.page-header {
  background: linear-gradient(135deg, #007bff 0%, #0056b3 100%);
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
  color: #007bff;
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
  color: #007bff;
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
  color: #007bff;
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
  background: #007bff;
  color: white;
  text-decoration: none;
  border-radius: 6px;
  transition: all 0.3s;
}

.footer-links a:hover {
  background: #0056b3;
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0, 123, 255, 0.4);
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
