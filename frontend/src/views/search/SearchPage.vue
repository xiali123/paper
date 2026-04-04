<template>
  <div class="search-page">
    <!-- 页面头部 -->
    <div class="page-header">
      <div class="header-content">
        <div class="header-icon">
          <el-icon :size="48"><Search /></el-icon>
        </div>
        <div class="header-text">
          <h1 class="page-title">论文搜索</h1>
          <p class="page-subtitle">搜索学术论文、期刊文章和研究报告</p>
        </div>
      </div>
    </div>

    <!-- 搜索区域 -->
    <el-card class="search-card" shadow="hover">
      <div class="search-wrapper">
        <el-input
          v-model="keyword"
          placeholder="搜索论文标题、作者、关键词..."
          size="large"
          class="search-input"
          @keyup.enter="handleSearch"
          clearable
        >
          <template #prefix>
            <el-icon class="search-icon"><Search /></el-icon>
          </template>
          <template #append>
            <el-button
              type="primary"
              @click="handleSearch"
              :loading="loading"
              class="search-button"
            >
              <el-icon><Search /></el-icon>
              搜索
            </el-button>
          </template>
        </el-input>
      </div>

      <!-- 快速搜索标签 -->
      <div class="quick-tags">
        <span class="tags-label">热门搜索：</span>
        <el-tag
          v-for="tag in quickTags"
          :key="tag"
          @click="handleQuickSearch(tag)"
          class="quick-tag"
          effect="plain"
          round
        >
          {{ tag }}
        </el-tag>
      </div>
    </el-card>

    <!-- 搜索结果 -->
    <el-card v-if="hasSearched" class="results-card" shadow="hover">
      <template #header>
        <div class="results-header">
          <div class="results-title">
            <el-icon class="title-icon"><Document /></el-icon>
            <span>搜索结果</span>
            <el-tag type="primary" effect="plain" round>{{ total }} 篇</el-tag>
          </div>
          <div class="results-meta" v-if="keyword">
            <span class="search-keyword">关键词："{{ keyword }}"</span>
          </div>
        </div>
      </template>

      <div v-loading="loading" class="results-list">
        <div v-for="item in results" :key="item.id" class="result-item">
          <div class="result-header">
            <h3 class="result-title">{{ item.title }}</h3>
            <div class="result-badges">
              <el-tag v-if="item.journal" type="info" size="small" effect="plain">
                {{ item.journal }}
              </el-tag>
              <el-tag v-if="item.publishDate" type="success" size="small" effect="plain">
                {{ item.publishDate }}
              </el-tag>
            </div>
          </div>

          <div class="result-meta">
            <span class="meta-item">
              <el-icon><User /></el-icon>
              {{ item.authors?.slice(0, 3).join(', ') }}{{ item.authors?.length > 3 ? ' 等' : '' }}
            </span>
          </div>

          <p class="result-abstract" v-if="item.abstract">{{ item.abstract }}</p>

          <div class="result-actions">
            <el-button type="primary" link @click="handleView(item)">
              <el-icon><View /></el-icon>
              查看详情
            </el-button>
            <el-button link @click="handleFavorite(item)">
              <el-icon><Star /></el-icon>
              收藏
            </el-button>
            <el-button link @click="handleCite(item)">
              <el-icon><DocumentCopy /></el-icon>
              引用
            </el-button>
          </div>
        </div>

        <el-empty
          v-if="!loading && results.length === 0"
          description="暂无搜索结果"
          :image-size="200"
        >
          <el-button type="primary" @click="handleReset">重新搜索</el-button>
        </el-empty>
      </div>

      <div class="pagination-wrapper" v-if="total > 0">
        <el-pagination
          v-model:current-page="page"
          v-model:page-size="pageSize"
          :total="total"
          :page-sizes="[10, 20, 50, 100]"
          layout="total, sizes, prev, pager, next, jumper"
          @size-change="handleSearch"
          @current-change="handleSearch"
          background
        />
      </div>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import { Search, Document, View, Star, DocumentCopy, User } from '@element-plus/icons-vue'
import type { Paper } from '@/types/paper'

const router = useRouter()

const keyword = ref('')
const loading = ref(false)
const hasSearched = ref(false)
const results = ref<Paper[]>([])
const total = ref(0)
const page = ref(1)
const pageSize = ref(20)

// 快速搜索标签
const quickTags = ref([
  '机器学习',
  '深度学习',
  '神经网络',
  '自然语言处理',
  '计算机视觉'
])

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

    // 模拟数据用于演示
    await new Promise(resolve => setTimeout(resolve, 1000))
    results.value = []
    total.value = 0

    if (results.value.length === 0) {
      ElMessage.info('未找到相关论文')
    }
  } catch (error) {
    ElMessage.error('搜索失败，请稍后重试')
  } finally {
    loading.value = false
  }
}

function handleQuickSearch(tag: string) {
  keyword.value = tag
  handleSearch()
}

function handleView(item: Paper) {
  router.push(`/papers/${item.id}`)
}

function handleFavorite(item: Paper) {
  // TODO: 实现收藏功能
  ElMessage.success('已添加到收藏')
}

function handleCite(item: Paper) {
  // TODO: 实现引用功能
  ElMessage.info('引用功能开发中')
}

function handleReset() {
  keyword.value = ''
  hasSearched.value = false
  results.value = []
  total.value = 0
  page.value = 1
}
</script>

<style scoped lang="scss">
.search-page {
  max-width: 1200px;
  margin: 0 auto;
  padding: $spacing-8;
  min-height: calc(100vh - 120px);
}

// 页面头部
.page-header {
  margin-bottom: $spacing-10;
  text-align: center;

  .header-content {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: $spacing-6;
  }

  .header-icon {
    width: 80px;
    height: 80px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
    border-radius: $border-radius-2xl;
    color: #ffffff;
    box-shadow: $shadow-xl;
  }

  .header-text {
    text-align: center;
  }

  .page-title {
    font-size: $font-size-4xl;
    font-weight: $font-weight-bold;
    margin: 0 0 $spacing-3 0;
    background: linear-gradient(135deg, $primary-600 0%, $primary-500 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
  }

  .page-subtitle {
    font-size: $font-size-lg;
    color: $text-secondary;
    margin: 0;
  }
}

// 搜索卡片
.search-card {
  margin-bottom: $spacing-8;
  border-radius: $border-radius-2xl;
  box-shadow: $shadow-lg;
  border: 2px solid $border-light;

  &:hover {
    box-shadow: $shadow-xl;
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }
}

.search-wrapper {
  margin-bottom: $spacing-6;
}

.search-input {
  .el-input__wrapper {
    padding: $spacing-4 $spacing-6;
    border-radius: $border-radius-xl;
    border: 2px solid $border-light;
    box-shadow: $shadow-sm;
    transition: all $duration-fast;

    &:hover {
      border-color: $primary-400;
    }

    &:focus-within {
      border-color: $primary-500;
      box-shadow: 0 0 0 4px rgba($primary-500, 0.1);
    }

    .dark & {
      background: $gray-700;
      border-color: $gray-600;
    }
  }

  .search-icon {
    color: $primary-500;
    font-size: $font-size-xl;
  }
}

.search-button {
  padding: $spacing-5 $spacing-8;
  font-size: $font-size-base;
  font-weight: $font-weight-bold;
  border-radius: $border-radius-lg;
}

// 快速搜索标签
.quick-tags {
  display: flex;
  align-items: center;
  gap: $spacing-3;
  flex-wrap: wrap;

  .tags-label {
    font-size: $font-size-sm;
    font-weight: $font-weight-semibold;
    color: $text-secondary;
    margin-right: $spacing-2;
  }

  .quick-tag {
    cursor: pointer;
    transition: all $duration-fast;

    &:hover {
      background: $primary-500;
      color: #ffffff;
      border-color: $primary-500;
      transform: translateY(-2px);
    }
  }
}

// 搜索结果卡片
.results-card {
  border-radius: $border-radius-2xl;
  box-shadow: $shadow-lg;
  border: 2px solid $border-light;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }
}

.results-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: $spacing-4;
  flex-wrap: wrap;
}

.results-title {
  display: flex;
  align-items: center;
  gap: $spacing-3;

  .title-icon {
    font-size: $font-size-2xl;
    color: $primary-500;
  }

  span {
    font-size: $font-size-xl;
    font-weight: $font-weight-bold;
    color: $text-primary;
  }
}

.results-meta {
  .search-keyword {
    font-size: $font-size-sm;
    color: $text-secondary;
    padding: $spacing-2 $spacing-4;
    background: $gray-100;
    border-radius: $border-radius-full;

    .dark & {
      background: $gray-700;
      color: $gray-400;
    }
  }
}

// 结果列表
.results-list {
  min-height: 400px;
  margin-bottom: $spacing-6;
}

.result-item {
  padding: $spacing-6 0;
  border-bottom: 2px solid $border-light;
  transition: all $duration-fast;

  &:last-child {
    border-bottom: none;
  }

  &:hover {
    background: linear-gradient(90deg, rgba($primary-50, 0.5) 0%, transparent 100%);
  }

  .dark &:hover {
    background: linear-gradient(90deg, rgba($gray-700, 0.5) 0%, transparent 100%);
  }
}

.result-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: $spacing-4;
  margin-bottom: $spacing-4;
}

.result-title {
  font-size: $font-size-lg;
  font-weight: $font-weight-bold;
  color: $text-primary;
  margin: 0;
  cursor: pointer;
  line-height: 1.4;
  flex: 1;

  &:hover {
    color: $primary-600;
  }

  .dark & {
    color: $gray-100;
  }
}

.result-badges {
  display: flex;
  gap: $spacing-2;
  flex-shrink: 0;
}

.result-meta {
  margin-bottom: $spacing-4;

  .meta-item {
    display: inline-flex;
    align-items: center;
    gap: $spacing-2;
    font-size: $font-size-sm;
    color: $text-secondary;

    .el-icon {
      font-size: $font-size-base;
    }
  }
}

.result-abstract {
  font-size: $font-size-base;
  color: $text-regular;
  line-height: 1.6;
  margin-bottom: $spacing-4;
  display: -webkit-box;
  -webkit-box-orient: vertical;
  -webkit-line-clamp: 3;
  overflow: hidden;
  text-overflow: ellipsis;
}

.result-actions {
  display: flex;
  gap: $spacing-4;
  flex-wrap: wrap;

  .el-button {
    display: inline-flex;
    align-items: center;
    gap: $spacing-2;
    font-size: $font-size-sm;
    font-weight: $font-weight-medium;

    .el-icon {
      font-size: $font-size-base;
    }
  }
}

// 分页
.pagination-wrapper {
  display: flex;
  justify-content: center;
  padding-top: $spacing-6;
  border-top: 2px solid $border-light;

  .dark & {
    border-top-color: $gray-700;
  }
}

// 响应式设计
@media (max-width: 768px) {
  .search-page {
    padding: $spacing-6;
  }

  .page-header {
    .page-title {
      font-size: $font-size-3xl;
    }

    .page-subtitle {
      font-size: $font-size-base;
    }
  }

  .result-header {
    flex-direction: column;
    gap: $spacing-3;
  }

  .result-badges {
    width: 100%;
    justify-content: flex-start;
  }

  .results-header {
    flex-direction: column;
    align-items: flex-start;
    gap: $spacing-3;
  }

  .quick-tags {
    flex-direction: column;
    align-items: flex-start;

    .tags-label {
      margin-bottom: $spacing-2;
    }
  }
}
</style>
