<template>
  <el-card class="recommended-content" shadow="hover">
    <template #header>
      <div class="recommended-content__header">
        <div class="recommended-content__title">
          <el-icon><Star /></el-icon>
          <span>推荐内容</span>
        </div>
        <el-radio-group v-model="activeTab" size="small">
          <el-radio-button value="papers">推荐论文</el-radio-button>
          <el-radio-button value="trending">热门搜索</el-radio-button>
        </el-radio-group>
      </div>
    </template>

    <div v-if="loading" class="recommended-content__loading">
      <el-skeleton :rows="3" animated />
    </div>

    <div v-else-if="activeTab === 'papers'" class="recommended-content__papers">
      <div v-if="papers.length === 0" class="recommended-content__empty">
        <el-empty description="暂无推荐论文" :image-size="60" />
      </div>
      <div v-else class="paper-list">
        <div
          v-for="item in displayedPapers"
          :key="item.paper.id"
          class="paper-item"
        >
          <router-link
            :to="`/papers/${item.paper.id}`"
            class="paper-item__link"
          >
            <div class="paper-item__title">{{ item.paper.title }}</div>
            <div class="paper-item__meta">
              <span class="paper-item__journal">
                {{ item.paper.journal }}
              </span>
              <span class="paper-item__year">{{ item.paper.year }}</span>
            </div>
            <div class="paper-item__reason">
              <el-tag size="small" type="info">
                {{ item.reason }}
              </el-tag>
              <el-rate
                v-model="item.score"
                disabled
                show-score
                text-color="#ff9900"
                score-template="{value}"
              />
            </div>
          </router-link>
        </div>
      </div>
      <div v-if="papers.length > displayCount" class="recommended-content__more">
        <el-button text type="primary" @click="handleViewMorePapers">
          查看更多 ({{ papers.length - displayCount }})
        </el-button>
      </div>
    </div>

    <div v-else-if="activeTab === 'trending'" class="recommended-content__trending">
      <div v-if="trendingSearches.length === 0" class="recommended-content__empty">
        <el-empty description="暂无热门搜索" :image-size="60" />
      </div>
      <div v-else class="trending-list">
        <div
          v-for="(item, index) in displayedTrending"
          :key="item.keyword"
          class="trending-item"
          @click="handleSearch(item.keyword)"
        >
          <div class="trending-item__rank" :class="`rank-${index + 1}`">
            {{ index + 1 }}
          </div>
          <div class="trending-item__content">
            <div class="trending-item__keyword">{{ item.keyword }}</div>
            <div class="trending-item__count">{{ item.count }} 次搜索</div>
          </div>
          <div class="trending-item__trend">
            <el-icon
              :size="16"
              :color="getTrendColor(item.trend)"
            >
              <component :is="getTrendIcon(item.trend)" />
            </el-icon>
          </div>
        </div>
      </div>
      <div v-if="trendingSearches.length > displayCount" class="recommended-content__more">
        <el-button text type="primary" @click="handleViewMoreTrending">
          查看更多 ({{ trendingSearches.length - displayCount }})
        </el-button>
      </div>
    </div>
  </el-card>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Star, Top, Bottom, Minus } from '@element-plus/icons-vue'
import type { RecommendedPaper, TrendingSearch } from '@/types/dashboard'

/**
 * 推荐内容组件
 * 显示推荐论文和热门搜索
 */
interface Props {
  /** 推荐论文列表 */
  papers?: RecommendedPaper[]
  /** 热门搜索列表 */
  trendingSearches?: TrendingSearch[]
  /** 是否加载中 */
  loading?: boolean
  /** 显示数量 */
  displayCount?: number
}

const props = withDefaults(defineProps<Props>(), {
  papers: () => [],
  trendingSearches: () => [],
  loading: false,
  displayCount: 5
})

const emit = defineEmits<{
  (e: 'search', keyword: string): void
  (e: 'view-more-papers'): void
  (e: 'view-more-trending'): void
}>()

const activeTab = ref<'papers' | 'trending'>('papers')

/**
 * 显示的论文列表
 */
const displayedPapers = computed(() => {
  return props.papers.slice(0, props.displayCount)
})

/**
 * 显示的热门搜索
 */
const displayedTrending = computed(() => {
  return props.trendingSearches.slice(0, props.displayCount)
})

/**
 * 获取趋势图标
 */
const getTrendIcon = (trend: TrendingSearch['trend']) => {
  const icons = {
    up: Top,
    down: Bottom,
    stable: Minus
  }
  return icons[trend]
}

/**
 * 获取趋势颜色
 */
const getTrendColor = (trend: TrendingSearch['trend']) => {
  const colors = {
    up: '#67c23a',
    down: '#f56c6c',
    stable: '#909399'
  }
  return colors[trend]
}

/**
 * 搜索关键词
 */
const handleSearch = (keyword: string) => {
  emit('search', keyword)
}

/**
 * 查看更多论文
 */
const handleViewMorePapers = () => {
  emit('view-more-papers')
}

/**
 * 查看更多热门搜索
 */
const handleViewMoreTrending = () => {
  emit('view-more-trending')
}
</script>

<style scoped lang="scss">
.recommended-content {
  border: none;
  transition: all 0.3s ease;
  height: 100%;

  &:hover {
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.12) !important;
  }

  :deep(.el-card__header) {
    padding: 16px 20px;
    border-bottom: 1px solid #f0f0f0;
  }

  :deep(.el-card__body) {
    padding: 20px;
    height: calc(100% - 65px);
    overflow-y: auto;
  }

  &__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
  }

  &__title {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 16px;
    font-weight: 600;
    color: #303133;

    .el-icon {
      color: #f56c6c;
    }
  }

  &__loading,
  &__empty {
    padding: 20px 0;
  }

  &__more {
    margin-top: 16px;
    text-align: center;
  }
}

.paper-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.paper-item {
  border: 1px solid #e4e7ed;
  border-radius: 8px;
  padding: 12px;
  transition: all 0.3s ease;
  background: #fff;

  &:hover {
    border-color: #409eff;
    box-shadow: 0 2px 8px rgba(64, 158, 255, 0.1);
  }

  &__link {
    text-decoration: none;
    display: block;
  }

  &__title {
    font-size: 14px;
    font-weight: 500;
    color: #303133;
    margin-bottom: 6px;
    line-height: 1.4;
    display: -webkit-box;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
    overflow: hidden;
  }

  &__meta {
    display: flex;
    gap: 8px;
    margin-bottom: 8px;
    font-size: 12px;
  }

  &__journal {
    color: #606266;
  }

  &__year {
    color: #909399;
  }

  &__reason {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;

    :deep(.el-rate) {
      .el-rate__text {
        font-size: 12px;
      }
    }
  }
}

.trending-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.trending-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 10px 12px;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.3s ease;

  &:hover {
    background: #f5f7fa;
  }

  &__rank {
    width: 24px;
    height: 24px;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 12px;
    font-weight: 700;
    border-radius: 4px;
    background: #f0f2f5;
    color: #606266;

    &.rank-1 {
      background: linear-gradient(135deg, #ffd700 0%, #ffed4e 100%);
      color: #fff;
    }

    &.rank-2 {
      background: linear-gradient(135deg, #c0c0c0 0%, #e8e8e8 100%);
      color: #fff;
    }

    &.rank-3 {
      background: linear-gradient(135deg, #cd7f32 0%, #e59866 100%);
      color: #fff;
    }
  }

  &__content {
    flex: 1;
    min-width: 0;
  }

  &__keyword {
    font-size: 14px;
    font-weight: 500;
    color: #303133;
    margin-bottom: 2px;
  }

  &__count {
    font-size: 12px;
    color: #909399;
  }

  &__trend {
    flex-shrink: 0;
  }
}

// 响应式设计
@media (max-width: 768px) {
  .recommended-content__header {
    flex-direction: column;
    align-items: flex-start;

    .el-radio-group {
      width: 100%;

      :deep(.el-radio-button) {
        flex: 1;
      }
    }
  }

  .paper-item {
    padding: 10px;

    &__title {
      font-size: 13px;
    }

    &__reason {
      flex-direction: column;
      align-items: flex-start;
    }
  }

  .trending-item {
    padding: 8px 10px;
  }
}
</style>
