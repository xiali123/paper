<template>
  <div class="paper-list">
    <div class="page-header">
      <h1 class="page-title">论文管理</h1>
      <el-button type="primary" @click="handleAdd">
        <el-icon><Plus /></el-icon>
        添加论文
      </el-button>
    </div>

    <el-card class="filter-card">
      <el-form :inline="true" :model="query">
        <el-form-item label="关键词">
          <el-input v-model="query.keyword" placeholder="请输入关键词" clearable />
        </el-form-item>
        <el-form-item label="作者">
          <el-input v-model="query.author" placeholder="请输入作者" clearable />
        </el-form-item>
        <el-form-item label="状态">
          <el-select v-model="query.status" placeholder="请选择状态" clearable>
            <el-option label="全部" value="" />
            <el-option label="待处理" value="pending" />
            <el-option label="处理中" value="processing" />
            <el-option label="已完成" value="completed" />
            <el-option label="失败" value="failed" />
          </el-select>
        </el-form-item>
        <el-form-item>
          <el-button type="primary" @click="handleSearch">搜索</el-button>
          <el-button @click="handleReset">重置</el-button>
        </el-form-item>
      </el-form>
    </el-card>

    <el-card class="table-card">
      <el-table :data="tableData" v-loading="loading" border stripe>
        <el-table-column prop="id" label="ID" width="80" />
        <el-table-column prop="title" label="标题" min-width="200" show-overflow-tooltip />
        <el-table-column prop="authors" label="作者" width="150" show-overflow-tooltip />
        <el-table-column prop="publishDate" label="发布日期" width="120" />
        <el-table-column prop="journal" label="期刊" width="150" show-overflow-tooltip />
        <el-table-column prop="status" label="状态" width="100">
          <template #default="{ row }">
            <el-tag :type="getStatusType(row.status)">
              {{ getStatusText(row.status) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="180" fixed="right">
          <template #default="{ row }">
            <el-button link type="primary" @click="handleView(row)">查看</el-button>
            <el-button link type="primary" @click="handleEdit(row)">编辑</el-button>
            <el-button link type="danger" @click="handleDelete(row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>

      <el-pagination
        v-model:current-page="query.page"
        v-model:page-size="query.pageSize"
        :total="total"
        :page-sizes="[10, 20, 50, 100]"
        layout="total, sizes, prev, pager, next, jumper"
        @size-change="fetchData"
        @current-change="fetchData"
      />
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import type { Paper, PaperQuery } from '@/types/paper'

const router = useRouter()

const loading = ref(false)
const tableData = ref<Paper[]>([])
const total = ref(0)

const query = reactive<PaperQuery>({
  page: 1,
  pageSize: 20,
  keyword: '',
  author: '',
  status: '',
})

onMounted(() => {
  fetchData()
})

async function fetchData() {
  loading.value = true
  try {
    // TODO: Replace with actual API call
    // const response = await paperApi.getPapers(query)
    // tableData.value = response.data.items
    // total.value = response.data.total
    
    // Mock data for now
    tableData.value = []
    total.value = 0
  } catch (error) {
    ElMessage.error('获取论文列表失败')
  } finally {
    loading.value = false
  }
}

function handleAdd() {
  router.push('/papers/create')
}

function handleView(row: Paper) {
  router.push(`/papers/${row.id}`)
}

function handleEdit(row: Paper) {
  router.push(`/papers/${row.id}/edit`)
}

function handleDelete(row: Paper) {
  ElMessageBox.confirm('确定要删除这篇论文吗？', '提示', {
    confirmButtonText: '确定',
    cancelButtonText: '取消',
    type: 'warning',
  })
    .then(async () => {
      try {
        // TODO: Replace with actual API call
        // await paperApi.deletePaper(row.id)
        ElMessage.success('删除成功')
        fetchData()
      } catch (error) {
        ElMessage.error('删除失败')
      }
    })
    .catch(() => {})
}

function handleSearch() {
  query.page = 1
  fetchData()
}

function handleReset() {
  query.keyword = ''
  query.author = ''
  query.status = ''
  query.page = 1
  fetchData()
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
.paper-list {
  padding: 20px;

  .filter-card {
    margin-bottom: 20px;
  }

  .table-card {
    .el-pagination {
      margin-top: 20px;
      justify-content: flex-end;
    }
  }
}
</style>
