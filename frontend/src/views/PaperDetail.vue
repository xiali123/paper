<template>
  <div class="paper-detail-page">
    <!-- 加载状态 -->
    <div v-if="paper.loading" class="loading-state">
      <div class="spinner"></div>
      <p>加载论文详情...</p>
    </div>

    <!-- 论文详情 -->
    <div v-else-if="paper.hasData && paper.paper" class="paper-content">
      <button @click="goBack" class="back-btn">
        ← 返回
      </button>

      <div class="paper-header">
        <div class="paper-title-section">
          <h1 class="paper-title">{{ paper.paper.title }}</h1>
          <span :class="`level-badge level-${paper.paper.level.toLowerCase()}`">
            {{ formatLevel(paper.paper.level) }}
          </span>
        </div>

        <div class="paper-meta">
          <div class="meta-item">
            <span class="meta-icon">📄</span>
            <span>{{ paper.paper.journal.full || paper.paper.journal.short }}</span>
          </div>
          <div class="meta-item">
            <span class="meta-icon">📅</span>
            <span>{{ paper.paper.year }}</span>
          </div>
          <div class="meta-item">
            <span class="meta-icon">✍️</span>
            <span>{{ paper.paper.authors }}</span>
          </div>
        </div>

        <!-- 论文链接 -->
        <div v-if="paper.paper.urls" class="paper-links">
          <a
            v-if="paper.paper.urls.doi"
            :href="paper.paper.urls.doi"
            target="_blank"
            rel="noopener"
            class="link-btn"
          >
            <span class="link-icon">🔗</span>
            查看 DOI
          </a>
          <a
            v-if="paper.paper.urls.journal"
            :href="paper.paper.urls.journal"
            target="_blank"
            rel="noopener"
            class="link-btn"
          >
            <span class="link-icon">📚</span>
            期刊链接
          </a>
        </div>
      </div>

      <!-- 论文摘要 -->
      <div v-if="paper.paper.abstract" class="paper-section">
        <h2>摘要</h2>
        <p class="abstract">{{ paper.paper.abstract }}</p>
      </div>

      <!-- 关键词 -->
      <div v-if="paper.paper.keywords && paper.paper.keywords.length > 0" class="paper-section">
        <h2>关键词</h2>
        <div class="keywords">
          <span v-for="keyword in paper.paper.keywords" :key="keyword" class="keyword-tag">
            {{ keyword }}
          </span>
        </div>
      </div>

      <!-- 统计信息 -->
      <div v-if="hasStats" class="paper-section">
        <h2>统计信息</h2>
        <div class="stats-grid">
          <div v-if="paper.paper.citations" class="stat-item">
            <span class="stat-value">{{ paper.paper.citations }}</span>
            <span class="stat-label">引用数</span>
          </div>
          <div v-if="paper.paper.references" class="stat-item">
            <span class="stat-value">{{ paper.paper.references }}</span>
            <span class="stat-label">参考文献</span>
          </div>
          <div v-if="paper.paper.downloadCount" class="stat-item">
            <span class="stat-value">{{ paper.paper.downloadCount }}</span>
            <span class="stat-label">下载次数</span>
          </div>
        </div>
      </div>

      <!-- 相关论文 -->
      <div v-if="paper.paper.relatedPapers && paper.paper.relatedPapers.length > 0" class="paper-section">
        <h2>相关论文</h2>
        <div class="related-papers">
          <div
            v-for="related in paper.paper.relatedPapers"
            :key="related.id"
            class="related-paper-item"
            @click="viewPaper(related.id)"
          >
            <h3>{{ related.title }}</h3>
            <div class="related-meta">
              <span>{{ related.journal.short }}</span>
              <span>{{ related.year }}</span>
              <span :class="'level-tag level-' + related.level.toLowerCase()">
                {{ formatLevel(related.level) }}
              </span>
            </div>
          </div>
        </div>
      </div>

      <!-- 操作按钮 -->
      <div class="paper-actions">
        <button @click="copyBibTex" class="action-btn secondary">
          <span class="btn-icon">📋</span>
          复制 BibTeX
        </button>
        <button @click="exportPaper" class="action-btn secondary">
          <span class="btn-icon">📥</span>
          导出
        </button>
        <button @click="sharePaper" class="action-btn primary">
          <span class="btn-icon">🔗</span>
          分享
        </button>
      </div>
    </div>

    <!-- 错误状态 -->
    <div v-else class="error-state">
      <div class="error-icon">⚠️</div>
      <h3>无法加载论文详情</h3>
      <p>{{ paper.error || '论文可能不存在或已被删除' }}</p>
      <button @click="goBack" class="back-btn">返回</button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted, computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { usePaper } from '@/composables/usePaper'
import { formatLevel } from '@/utils/format'
import { copyToClipboard } from '@/utils/format'

const route = useRoute()
const router = useRouter()
const paper = usePaper()

// 获取论文ID
const paperId = computed(() => parseInt(route.params.id as string))

// 是否有统计信息
const hasStats = computed(() => {
  return paper.paper?.citations || paper.paper?.references || paper.paper?.downloadCount
})

// 返回上一页
const goBack = () => {
  router.back()
}

// 查看论文
const viewPaper = (id: number) => {
  router.push({ name: 'paper-detail', params: { id } })
}

// 复制 BibTeX
const copyBibTex = () => {
  if (!paper.paper) return

  const bibTex = `@article{${paper.paper.id},
  title={${paper.paper.title}},
  author={${paper.paper.authors}},
  journal={${paper.paper.journal.full || paper.paper.journal.short}},
  year={${paper.paper.year}}
}`

  copyToClipboard(bibTex).then(success => {
    if (success) {
      alert('BibTeX 已复制到剪贴板')
    } else {
      alert('复制失败，请手动复制')
    }
  })
}

// 导出论文
const exportPaper = () => {
  if (!paper.paper) return
  // 实现导出功能
  alert('导出功能开发中...')
}

// 分享论文
const sharePaper = async () => {
  if (!paper.paper) return

  const url = window.location.href
  const title = paper.paper.title

  if (navigator.share) {
    try {
      await navigator.share({
        title,
        url
      })
    } catch (err) {
      console.error('Share failed:', err)
    }
  } else {
    copyToClipboard(url).then(success => {
      if (success) {
        alert('链接已复制到剪贴板')
      } else {
        alert('复制失败')
      }
    })
  }
}

// 加载论文详情
onMounted(() => {
  if (paperId.value) {
    paper.fetchDetail(paperId.value)
  }
})
</script>

<style scoped>
.paper-detail-page {
  max-width: 900px;
  margin: 0 auto;
  padding: 40px 20px;
}

.loading-state {
  text-align: center;
  padding: 60px 20px;
  color: white;
}

.spinner {
  width: 40px;
  height: 40px;
  border: 4px solid rgba(255, 255, 255, 0.3);
  border-top-color: white;
  border-radius: 50%;
  margin: 0 auto 20px;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

.paper-content {
  background: rgba(255, 255, 255, 0.95);
  border-radius: 15px;
  padding: 40px;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
}

.back-btn {
  padding: 10px 20px;
  background: transparent;
  color: #667eea;
  border: 1px solid #667eea;
  border-radius: 8px;
  font-size: 14px;
  cursor: pointer;
  transition: all 0.3s;
  margin-bottom: 30px;
}

.back-btn:hover {
  background: #667eea;
  color: white;
}

.paper-header {
  margin-bottom: 40px;
}

.paper-title-section {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: 20px;
  margin-bottom: 25px;
}

.paper-title {
  font-size: 28px;
  color: #1f2937;
  margin: 0;
  line-height: 1.4;
  flex: 1;
}

.level-badge {
  padding: 8px 16px;
  border-radius: 20px;
  font-size: 14px;
  font-weight: 600;
  white-space: nowrap;
}

.level-a {
  background: #fecaca;
  color: #991b1b;
}

.level-b {
  background: #fed7aa;
  color: #9a3412;
}

.level-c {
  background: #d1d5db;
  color: #374151;
}

.paper-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 25px;
  margin-bottom: 25px;
}

.meta-item {
  display: flex;
  align-items: center;
  gap: 8px;
  color: #6b7280;
  font-size: 15px;
}

.meta-icon {
  font-size: 18px;
}

.paper-links {
  display: flex;
  gap: 15px;
  flex-wrap: wrap;
}

.link-btn {
  padding: 10px 20px;
  background: #f3f4f6;
  color: #374151;
  border: 1px solid #d1d5db;
  border-radius: 8px;
  text-decoration: none;
  font-size: 14px;
  font-weight: 500;
  display: flex;
  align-items: center;
  gap: 8px;
  transition: all 0.3s;
}

.link-btn:hover {
  background: #e5e7eb;
  border-color: #9ca3af;
}

.paper-section {
  margin-bottom: 35px;
  padding-bottom: 35px;
  border-bottom: 1px solid #e5e7eb;
}

.paper-section:last-of-type {
  border-bottom: none;
}

.paper-section h2 {
  font-size: 20px;
  color: #1f2937;
  margin-bottom: 15px;
}

.abstract {
  color: #4b5563;
  line-height: 1.8;
  font-size: 16px;
  margin: 0;
}

.keywords {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.keyword-tag {
  padding: 6px 14px;
  background: #eef2ff;
  color: #4f46e5;
  border-radius: 20px;
  font-size: 14px;
  font-weight: 500;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
  gap: 20px;
}

.stat-item {
  text-align: center;
  padding: 20px;
  background: #f9fafb;
  border-radius: 10px;
}

.stat-value {
  display: block;
  font-size: 28px;
  font-weight: 700;
  color: #667eea;
  margin-bottom: 5px;
}

.stat-label {
  font-size: 14px;
  color: #6b7280;
}

.related-papers {
  display: flex;
  flex-direction: column;
  gap: 15px;
}

.related-paper-item {
  padding: 15px;
  background: #f9fafb;
  border-radius: 10px;
  border-left: 3px solid #667eea;
  cursor: pointer;
  transition: all 0.3s;
}

.related-paper-item:hover {
  background: #f3f4f6;
  transform: translateX(5px);
}

.related-paper-item h3 {
  font-size: 16px;
  color: #1f2937;
  margin: 0 0 10px 0;
}

.related-meta {
  display: flex;
  gap: 15px;
  font-size: 14px;
  color: #6b7280;
}

.level-tag {
  padding: 3px 10px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 600;
}

.level-a {
  background: #fecaca;
  color: #991b1b;
}

.level-b {
  background: #fed7aa;
  color: #9a3412;
}

.level-c {
  background: #d1d5db;
  color: #374151;
}

.paper-actions {
  display: flex;
  gap: 15px;
  flex-wrap: wrap;
}

.action-btn {
  padding: 12px 24px;
  border: none;
  border-radius: 10px;
  font-size: 15px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
  display: flex;
  align-items: center;
  gap: 8px;
}

.action-btn.primary {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.action-btn.primary:hover {
  transform: translateY(-2px);
  box-shadow: 0 5px 15px rgba(102, 126, 234, 0.3);
}

.action-btn.secondary {
  background: white;
  color: #667eea;
  border: 2px solid #667eea;
}

.action-btn.secondary:hover {
  background: #667eea;
  color: white;
}

.btn-icon {
  font-size: 16px;
}

.error-state {
  text-align: center;
  padding: 60px 20px;
  color: white;
}

.error-icon {
  font-size: 64px;
  margin-bottom: 20px;
}

.error-state h3 {
  font-size: 24px;
  margin-bottom: 10px;
  color: white;
}

.error-state p {
  color: rgba(255, 255, 255, 0.9);
  margin-bottom: 20px;
}
</style>
