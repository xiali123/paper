<template>
  <div class="stats-page">
    <div class="stats-header">
      <h1 class="page-title">数据统计</h1>
      <p class="page-subtitle">平台数据分析与学术趋势洞察</p>
    </div>

    <!-- 诊断信息 -->
    <div style="background: #f0f0f0; padding: 20px; margin: 20px; border-radius: 8px; font-family: monospace;">
      <h3>🔍 诊断信息</h3>
      <p>stats.loading: {{ stats.loading }}</p>
      <p>stats.hasData: {{ stats.hasData }}</p>
      <p>stats.error: {{ stats.error }}</p>
      <p>stats.overview 类型: {{ typeof stats.overview }}</p>
      <p>stats.overview 值: {{ stats.overview }}</p>
      <p>totalPapers: {{ stats.overview?.totalPapers }}</p>
      <p>条件判断结果:</p>
      <ul>
        <li v-if="stats.loading">✓ loading 为 true</li>
        <li v-else>✗ loading 为 false</li>
        <li v-if="stats.overview">✓ overview 为 truthy</li>
        <li v-else>✗ overview 为 falsy</li>
        <li v-if="stats.hasData">✓ hasData 为 true</li>
        <li v-else>✗ hasData 为 false</li>
      </ul>
    </div>

    <!-- Loading State -->
    <div v-if="stats.loading" style="text-align: center; padding: 40px; background: #fff3cd;">
      <p style="font-size: 18px;">⏳ 加载统计数据...</p>
    </div>

    <!-- Error State -->
    <div v-else-if="stats.error" style="text-align: center; padding: 40px; background: #f8d7da;">
      <p style="font-size: 18px;">❌ 加载失败: {{ stats.error }}</p>
      <button @click="stats.fetchOverview" style="margin-top: 20px; padding: 10px 20px;">重试</button>
    </div>

    <!-- Success State -->
    <div v-else style="padding: 20px;">
      <div style="background: #d4edda; padding: 20px; border-radius: 8px; margin-bottom: 20px;">
        <h3>✅ 数据加载成功！</h3>
        <p>应该显示下面的统计卡片</p>
      </div>

      <div style="display: grid; grid-template-columns: repeat(4, 1fr); gap: 20px; margin-bottom: 40px;">
        <div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; border-radius: 12px; text-align: center; box-shadow: 0 4px 6px rgba(0,0,0,0.1);">
          <div style="font-size: 48px; font-weight: bold; margin: 15px 0;">
            {{ stats.overview?.totalPapers?.toLocaleString() || 'N/A' }}
          </div>
          <div style="font-size: 16px;">总论文数</div>
        </div>

        <div style="background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); color: white; padding: 30px; border-radius: 12px; text-align: center; box-shadow: 0 4px 6px rgba(0,0,0,0.1);">
          <div style="font-size: 48px; font-weight: bold; margin: 15px 0;">
            {{ stats.overview?.totalJournals?.toLocaleString() || 'N/A' }}
          </div>
          <div style="font-size: 16px;">总期刊数</div>
        </div>

        <div style="background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%); color: white; padding: 30px; border-radius: 12px; text-align: center; box-shadow: 0 4px 6px rgba(0,0,0,0.1);">
          <div style="font-size: 48px; font-weight: bold; margin: 15px 0;">
            {{ stats.overview?.topTierPapers?.toLocaleString() || 'N/A' }}
          </div>
          <div style="font-size: 16px;">顶刊论文</div>
        </div>

        <div style="background: linear-gradient(135deg, #43e97b 0%, #38f9d7 100%); color: white; padding: 30px; border-radius: 12px; text-align: center; box-shadow: 0 4px 6px rgba(0,0,0,0.1);">
          <div style="font-size: 48px; font-weight: bold; margin: 15px 0;">
            {{ stats.overview?.papersLastYear?.toLocaleString() || 'N/A' }}
          </div>
          <div style="font-size: 16px;">去年论文</div>
        </div>
      </div>

      <div style="background: white; padding: 30px; border-radius: 12px; box-shadow: 0 2px 8px rgba(0,0,0,0.1);">
        <h3 style="margin-bottom: 20px;">🏆 最活跃期刊</h3>
        <p style="font-size: 18px; color: #333;">{{ stats.overview?.mostActiveJournal || 'N/A' }}</p>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted } from 'vue'
import { useStats } from '@/composables/useStats'

const stats = useStats()

onMounted(async () => {
  console.log('🎯 [Diagnostic] Component mounted')
  console.log('📊 [Diagnostic] Initial state:', {
    loading: stats.loading.value,
    hasData: stats.hasData.value,
    overview: stats.overview.value
  })

  await stats.fetchOverview()

  console.log('✅ [Diagnostic] Final state:', {
    loading: stats.loading.value,
    hasData: stats.hasData.value,
    overview: stats.overview.value
  })
})
</script>

<style scoped>
.stats-page {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
}

.page-title {
  font-size: 32px;
  text-align: center;
  margin-bottom: 10px;
}

.page-subtitle {
  text-align: center;
  color: #666;
  margin-bottom: 40px;
}
</style>
