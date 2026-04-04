<template>
  <div class="search-page">
    <div class="page-header">
      <h1 class="page-title">搜索</h1>
    </div>

    <el-card class="search-card">
      <el-input
        v-model="keyword"
        placeholder="请输入搜索关键词"
        size="large"
        @keyup.enter="handleSearch"
      >
        <template #append>
          <el-button @click="handleSearch">
            <el-icon><Search /></el-icon>
            搜索
          </el-button>
        </template>
      </el-input>
    </el-card>

    <el-card v-if="hasSearched" class="results-card">
      <template #header>
        <div class="results-header">
          <span>搜索结果 ({{ total }})</span>
        </div>
      </template>

      <div v-loading="loading" class="results-list">
        <div v-for="item in results" :key="item.id" class="result-item">
          <h3 class="result-title">{{ item.title }}</h3>
          <div class="result-meta">
            <span>{{ item.authors?.join(', ') }}</span>
            <span>{{ item.publishDate }}</span>
            <span>{{ item.journal }}</span>
          </div>
          <p class="result-abstract">{{ item.abstract }}</p>
          <div class="result-actions">
            <el-button link type="primary" @click="handleView(item)">查看详情</el-button>
          </div>
        </div>

        <el-empty v-if="!loading && results.length === 0" description="暂无搜索结果" />
      </div>

      <el-pagination
        v-if="total > 0"
        v-model:current-page="page"
        v-model:page-size="pageSize"
        :total="total"
        :page-sizes="[10, 20, 50, 100]"
        layout="total, sizes, prev, pager, next, jumper"
        @size-change="handleSearch"
        @current-change="handleSearch"
      />
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import type { Paper } from '@/types/paper'

const router = useRouter()

const keyword = ref('')
const loading = ref(false)
const hasSearched = ref(false)
const results = ref<Paper[]>([])
const total = ref(0)
const page = ref(1)
const pageSize = ref(20)

async function handleSearch() {
  if (!keyword.value.trim()) {
    ElMessage.warning('请输入搜索关键词')
    return
  }

  loading.value = true
  hasSearched.value = true
  try {
    // TODO: Replace with actual API call
    // const response = await paperApi.searchPapers(keyword.value, {
    //   page: page.value,
    //   pageSize: pageSize.value,
    // })
    // results.value = response.data.items
    // total.value = response.data.total
    results.value = []
    total.value = 0
  } catch (error) {
    ElMessage.error('搜索失败')
  } finally {
    loading.value = false
  }
}

function handleView(item: Paper) {
  router.push(`/papers/${item.id}`)
}
</script>

<style scoped lang="scss">
.search-page {
  padding: 20px;

  .search-card {
    margin-bottom: 20px;
  }

  .results-card {
    .results-list {
      min-height: 400px;

      .result-item {
        padding: 20px 0;
        border-bottom: 1px solid #ebeef5;

        &:last-child {
          border-bottom: none;
        }

        .result-title {
          font-size: 18px;
          font-weight: 500;
          color: #303133;
          margin-bottom: 10px;
          cursor: pointer;

          &:hover {
            color: #409eff;
          }
        }

        .result-meta {
          font-size: 14px;
          color: #909399;
          margin-bottom: 10px;

          span {
            margin-right: 15px;
          }
        }

        .result-abstract {
          font-size: 14px;
          color: #606266;
          line-height: 1.6;
          margin-bottom: 10px;
          display: -webkit-box;
          -webkit-box-orient: vertical;
          -webkit-line-clamp: 3;
          overflow: hidden;
          text-overflow: ellipsis;
        }

        .result-actions {
          display: flex;
          gap: 10px;
        }
      }
    }

    .el-pagination {
      margin-top: 20px;
      justify-content: flex-end;
    }
  }
}
</style>
