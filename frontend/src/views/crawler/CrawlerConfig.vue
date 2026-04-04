<template>
  <div class="crawler-config">
    <div class="page-header">
      <h1 class="page-title">爬虫配置</h1>
      <el-button type="primary" @click="handleAdd">
        <el-icon><Plus /></el-icon>
        添加配置
      </el-button>
    </div>

    <el-card>
      <el-table :data="tableData" v-loading="loading" border stripe>
        <el-table-column prop="id" label="ID" width="80" />
        <el-table-column prop="name" label="名称" width="200" />
        <el-table-column prop="source" label="来源" width="120" />
        <el-table-column prop="enabled" label="状态" width="80">
          <template #default="{ row }">
            <el-tag :type="row.enabled ? 'success' : 'info'">
              {{ row.enabled ? '启用' : '禁用' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="keywords" label="关键词" min-width="200" show-overflow-tooltip />
        <el-table-column label="操作" width="250" fixed="right">
          <template #default="{ row }">
            <el-button link type="primary" @click="handleStart(row)" :disabled="!row.enabled">启动</el-button>
            <el-button link type="primary" @click="handleEdit(row)">编辑</el-button>
            <el-button link type="danger" @click="handleDelete(row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'

const loading = ref(false)
const tableData = ref([])

onMounted(() => {
  fetchData()
})

async function fetchData() {
  loading.value = true
  try {
    // TODO: Replace with actual API call
    tableData.value = []
  } catch (error) {
    ElMessage.error('获取配置列表失败')
  } finally {
    loading.value = false
  }
}

function handleAdd() {
  ElMessage.info('添加配置功能开发中')
}

function handleEdit(row: any) {
  ElMessage.info('编辑配置功能开发中')
}

function handleStart(row: any) {
  ElMessage.info('启动爬虫功能开发中')
}

function handleDelete(row: any) {
  ElMessage.info('删除配置功能开发中')
}
</script>

<style scoped lang="scss">
.crawler-config {
  padding: 20px;
}
</style>
