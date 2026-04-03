<template>
  <div class="similar-papers-list">
    <div class="list-header">
      <h3>📚 相似论文</h3>
      <el-button size="small" @click="viewAll">
        查看全部
        <el-icon><ArrowRight /></el-icon>
      </el-button>
    </div>

    <div v-if="papers.length" class="papers-grid">
      <div
        v-for="paper in papers"
        :key="paper.id"
        class="paper-card"
        @click="viewPaper(paper.id)"
      >
        <div class="card-header">
          <div class="similarity-score" :style="{ borderColor: getSimilarityColor(paper.similarity) }">
            相似度: {{ (paper.similarity * 100).toFixed(1) }}%
          </div>
        </div>

        <div class="card-content">
          <h4 class="paper-title">{{ paper.title }}</h4>
          <div class="paper-authors">{{ paper.authors.join(', ') }}</div>
          <div class="paper-meta">
            <span>{{ paper.year }}</span>
            <span>{{ paper.journal }}</span>
          </div>
        </div>

        <div class="card-actions">
          <el-button type="primary" link size="small">查看详情</el-button>
          <el-button link size="small">对比</el-button>
        </div>
      </div>
    </div>

    <div v-else class="empty-state">
      <el-empty description="暂无相似论文" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { ArrowRight } from '@element-plus/icons-vue'
import { useRecommendationsStore } from '@/stores/recommendations'

const router = useRouter()
const recommendationsStore = useRecommendationsStore()

const papers = ref<any[]>([])
const loading = ref(false)

const loadSimilar = async () => {
  loading.value = true
  try {
    const paperId = 0 // 从当前页面获取
    const results = await recommendationsStore.loadSimilar(paperId, 10)
    papers.value = results.map((p: any) => ({
      ...p,
      similarity: Math.random() * 0.5 + 0.5 // 模拟相似度
    }))
  } catch (error) {
    ElMessage.error('加载相似论文失败')
  } finally {
    loading.value = false
  }
}

const viewPaper = (paperId: number) => {
  router.push(`/papers/${paperId}`)
}

const viewAll = () => {
  router.push('/recommendations?type=similar')
}

const getSimilarityColor = (similarity: number) => {
  if (similarity >= 0.8) return '#67c23a'
  if (similarity >= 0.6) return '#e6a23c'
  return '#909399'
}

onMounted(() => {
  loadSimilar()
})
</script>

<style scoped>
.similar-papers-list {
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

.papers-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 16px;
}

.paper-card {
  background: white;
  border-radius: 12px;
  padding: 20px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  cursor: pointer;
  transition: all 0.3s ease;
}

.paper-card:hover {
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.12);
  transform: translateY(-2px);
}

.card-header {
  margin-bottom: 12px;
}

.similarity-score {
  display: inline-block;
  padding: 4px 12px;
  border: 2px solid;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 600;
}

.card-content {
  margin-bottom: 12px;
}

.paper-title {
  font-size: 15px;
  font-weight: 600;
  color: #303133;
  margin: 0 0 8px 0;
  line-height: 1.4;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.paper-authors {
  font-size: 13px;
  color: #606266;
  margin-bottom: 8px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.paper-meta {
  display: flex;
  gap: 8px;
  font-size: 12px;
  color: #909399;
}

.card-actions {
  display: flex;
  justify-content: space-between;
  padding-top: 12px;
  border-top: 1px solid #e4e7ed;
}

.empty-state {
  padding: 40px 20px;
  text-align: center;
}
</style>
