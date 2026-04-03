<template>
  <div class="trending-papers-list">
    <div class="list-header">
      <h3>🔥 热门论文</h3>
      <el-select v-model="timeWindow" size="small" @change="loadTrending">
        <el-option label="今日" value="day" />
        <el-option label="本周" value="week" />
        <el-option label="本月" value="month" />
      </el-select>
    </div>

    <!-- 热门论文列表 -->
    <div v-if="papers.length" class="trending-list">
      <div
        v-for="(paper, index) in papers"
        :key="paper.id"
        class="trending-item"
      >
        <div class="rank-badge" :class="`rank-${index + 1}`">
          #{{ index + 1 }}
        </div>

        <div class="item-content" @click="viewPaper(paper.id)">
          <h4 class="paper-title">{{ paper.title }}</h4>
          <div class="paper-authors">{{ paper.authors.join(', ') }}</div>

          <div class="paper-stats">
            <div class="stat-item">
              <el-icon><View /></el-icon>
              <span>{{ formatNumber(paper.views) }}</span>
            </div>
            <div class="stat-item">
              <el-icon><DocumentCopy /></el-icon>
              <span>{{ formatNumber(paper.citationCount) }}</span>
            </div>
            <div class="stat-item">
              <el-icon><Download /></el-icon>
              <span>{{ formatNumber(paper.downloads) }}</span>
            </div>
          </div>

          <div class="trend-indicator" :class="paper.trend">
            <el-icon v-if="paper.trend === 'up'"><Top /></el-icon>
            <el-icon v-else-if="paper.trend === 'down'"><Bottom /></el-icon>
            <span>{{ getTrendText(paper.trend) }}</span>
          </div>
        </div>

        <div class="item-actions">
          <el-button size="small" @click.stop="quickView(paper)">
            快速预览
          </el-button>
          <el-button size="small" @click.stop="addToLibrary(paper.id)">
            加入文献库
          </el-button>
        </div>
      </div>
    </div>

    <!-- 空状态 -->
    <div v-else class="empty-state">
      <el-empty description="暂无热门论文数据" />
    </div>

    <!-- 快速预览对话框 -->
    <el-dialog v-model="previewVisible" title="快速预览" width="60%">
      <div v-if="previewPaper" class="preview-content">
        <h3>{{ previewPaper.title }}</h3>
        <div class="preview-meta">
          <span>{{ previewPaper.authors.join(', ') }}</span>
          <span>{{ previewPaper.year }}</span>
          <span>{{ previewPaper.journal }}</span>
        </div>
        <el-divider />
        <div class="preview-abstract">
          <h4>摘要</h4>
          <p>{{ previewPaper.abstract }}</p>
        </div>
        <div class="preview-keywords">
          <h4>关键词</h4>
          <el-tag
            v-for="keyword in previewPaper.keywords"
            :key="keyword"
            size="small"
          >
            {{ keyword }}
          </el-tag>
        </div>
      </div>
      <template #footer>
        <el-button @click="previewVisible = false">关闭</el-button>
        <el-button type="primary" @click="viewPaper(previewPaper.id)">
          查看详情
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { Top, Bottom, View, DocumentCopy, Download } from '@element-plus/icons-vue'
import { useRecommendationsStore } from '@/stores/recommendations'

const router = useRouter()
const recommendationsStore = useRecommendationsStore()

const papers = ref<any[]>([])
const loading = ref(false)
const timeWindow = ref<'day' | 'week' | 'month'>('week')
const previewVisible = ref(false)
const previewPaper = ref<any>(null)

const loadTrending = async () => {
  loading.value = true
  try {
    const results = await recommendationsStore.loadTrending(20)
    papers.value = results.map((p: any) => ({
      ...p,
      views: Math.floor(Math.random() * 10000) + 1000,
      downloads: Math.floor(Math.random() * 5000) + 500,
      trend: Math.random() > 0.5 ? 'up' : 'down'
    }))
  } catch (error) {
    ElMessage.error('加载热门论文失败')
  } finally {
    loading.value = false
  }
}

const viewPaper = (paperId: number) => {
  router.push(`/papers/${paperId}`)
}

const quickView = (paper: any) => {
  previewPaper.value = paper
  previewVisible.value = true
}

const addToLibrary = async (paperId: number) => {
  ElMessage.success('已加入文献库')
}

const formatNumber = (num: number) => {
  if (num >= 1000000) return (num / 1000000).toFixed(1) + 'M'
  if (num >= 1000) return (num / 1000).toFixed(1) + 'K'
  return num.toString()
}

const getTrendText = (trend: string) => {
  if (trend === 'up') return '上升'
  if (trend === 'down') return '下降'
  return '稳定'
}

onMounted(() => {
  loadTrending()
})
</script>

<style scoped>
.trending-papers-list {
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

.trending-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.trending-item {
  display: flex;
  gap: 12px;
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  transition: all 0.3s ease;
}

.trending-item:hover {
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.12);
}

.rank-badge {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  border-radius: 50%;
  font-size: 18px;
  font-weight: 700;
  color: white;
  flex-shrink: 0;
}

.rank-badge.rank-1 {
  background: linear-gradient(135deg, #ffd700 0%, #ffed4e 100%);
}

.rank-badge.rank-2 {
  background: linear-gradient(135deg, #c0c0c0 0%, #e0e0e0 100%);
}

.rank-badge.rank-3 {
  background: linear-gradient(135deg, #cd7f32 0%, #e6a23c 100%);
}

.item-content {
  flex: 1;
  cursor: pointer;
}

.paper-title {
  font-size: 15px;
  font-weight: 600;
  color: #303133;
  margin: 0 0 6px 0;
  line-height: 1.4;
}

.paper-authors {
  font-size: 13px;
  color: #606266;
  margin-bottom: 8px;
}

.paper-stats {
  display: flex;
  gap: 16px;
  margin-bottom: 8px;
}

.stat-item {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  color: #909399;
}

.stat-item .el-icon {
  font-size: 14px;
}

.trend-indicator {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  font-weight: 600;
}

.trend-indicator.up {
  color: #67c23a;
}

.trend-indicator.down {
  color: #f56c6c;
}

.item-actions {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.empty-state {
  padding: 40px 20px;
  text-align: center;
}

.preview-content h3 {
  font-size: 18px;
  font-weight: 600;
  margin: 0 0 12px 0;
}

.preview-meta {
  display: flex;
  gap: 12px;
  font-size: 13px;
  color: #909399;
  margin-bottom: 16px;
}

.preview-abstract h4 {
  font-size: 14px;
  font-weight: 600;
  margin: 0 0 8px 0;
}

.preview-abstract p {
  font-size: 14px;
  line-height: 1.6;
  color: #606266;
  margin: 0;
}

.preview-keywords {
  margin-top: 16px;
}

.preview-keywords h4 {
  font-size: 14px;
  font-weight: 600;
  margin: 0 0 8px 0;
}

.preview-keywords .el-tag {
  margin-right: 8px;
  margin-bottom: 8px;
}
</style>
