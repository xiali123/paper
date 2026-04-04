<template>
  <div class="paper-detail">
    <div class="page-header">
      <h1 class="page-title">论文详情</h1>
      <div class="page-actions">
        <el-button @click="goBack">返回</el-button>
        <el-button type="primary" @click="handleEdit">编辑</el-button>
      </div>
    </div>

    <el-card v-loading="loading">
      <el-descriptions :column="2" border>
        <el-descriptions-item label="标题">{{ paper.title }}</el-descriptions-item>
        <el-descriptions-item label="作者">{{ paper.authors?.join(', ') }}</el-descriptions-item>
        <el-descriptions-item label="发布日期">{{ paper.publishDate }}</el-descriptions-item>
        <el-descriptions-item label="期刊">{{ paper.journal }}</el-descriptions-item>
        <el-descriptions-item label="DOI">{{ paper.doi }}</el-descriptions-item>
        <el-descriptions-item label="状态">
          <el-tag :type="getStatusType(paper.status)">
            {{ getStatusText(paper.status) }}
          </el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="关键词" :span="2">
          <el-tag v-for="keyword in paper.keywords" :key="keyword" class="keyword-tag">
            {{ keyword }}
          </el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="摘要" :span="2">
          {{ paper.abstract }}
        </el-descriptions-item>
      </el-descriptions>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import type { Paper } from '@/types/paper'

const route = useRoute()
const router = useRouter()

const loading = ref(false)
const paper = ref<Paper>({
  id: 0,
  title: '',
  authors: [],
  abstract: '',
  keywords: [],
  publishDate: '',
  status: 'pending',
  createdAt: '',
  updatedAt: '',
})

onMounted(async () => {
  await fetchPaper()
})

async function fetchPaper() {
  loading.value = true
  try {
    const id = Number(route.params.id)
    // TODO: Replace with actual API call
    // const response = await paperApi.getPaperById(id)
    // paper.value = response.data
  } catch (error) {
    ElMessage.error('获取论文详情失败')
  } finally {
    loading.value = false
  }
}

function goBack() {
  router.back()
}

function handleEdit() {
  router.push(`/papers/${paper.value.id}/edit`)
}

function getStatusType(status: string) {
  const map: Record<string, any> = {
    pending: 'info',
    processing: 'warning',
    completed: 'success',
    failed: 'danger',
  }
  return map[status] || 'info'
}

function getStatusText(status: string) {
  const map: Record<string, string> = {
    pending: '待处理',
    processing: '处理中',
    completed: '已完成',
    failed: '失败',
  }
  return map[status] || '未知'
}
</script>

<style scoped lang="scss">
.paper-detail {
  padding: 20px;

  .keyword-tag {
    margin-right: 8px;
    margin-bottom: 8px;
  }
}
</style>
