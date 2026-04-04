<template>
  <el-card class="recent-activity" shadow="hover">
    <template #header>
      <div class="recent-activity__header">
        <span class="recent-activity__title">最近活动</span>
        <el-button
          type="primary"
          link
          size="small"
          @click="handleViewAll"
        >
          查看全部
        </el-button>
      </div>
    </template>

    <div v-if="loading" class="recent-activity__loading">
      <el-skeleton :rows="3" animated />
    </div>

    <div v-else-if="activities.length === 0" class="recent-activity__empty">
      <el-empty description="暂无最近活动" :image-size="60" />
    </div>

    <el-timeline v-else class="recent-activity__timeline">
      <el-timeline-item
        v-for="activity in displayedActivities"
        :key="activity.id"
        :timestamp="formatTimestamp(activity.timestamp)"
        placement="top"
        :color="getActivityColor(activity.type)"
        :icon="getActivityIcon(activity.type)"
      >
        <div class="activity-item">
          <div class="activity-item__title">{{ activity.title }}</div>
          <div class="activity-item__description">{{ activity.description }}</div>
          <router-link
            v-if="activity.data?.link"
            :to="activity.data.link"
            class="activity-item__link"
          >
            查看详情 →
          </router-link>
        </div>
      </el-timeline-item>
    </el-timeline>
  </el-card>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import {
  Document,
  Search,
  Download,
  Star,
  Connection
} from '@element-plus/icons-vue'
import { formatDistanceToNow } from 'date-fns'
import { zhCN } from 'date-fns/locale'
import type { RecentActivity } from '@/types/dashboard'

/**
 * 最近活动组件
 * 显示用户的最近操作记录
 */
interface Props {
  /** 活动列表 */
  activities?: RecentActivity[]
  /** 是否加载中 */
  loading?: boolean
  /** 显示数量 */
  displayCount?: number
}

const props = withDefaults(defineProps<Props>(), {
  activities: () => [],
  loading: false,
  displayCount: 5
})

const emit = defineEmits<{
  (e: 'view-all'): void
}>()

/**
 * 显示的活动列表
 */
const displayedActivities = computed(() => {
  return props.activities.slice(0, props.displayCount)
})

/**
 * 格式化时间戳
 */
const formatTimestamp = (timestamp: string) => {
  try {
    return formatDistanceToNow(new Date(timestamp), {
      addSuffix: true,
      locale: zhCN
    })
  } catch {
    return timestamp
  }
}

/**
 * 获取活动颜色
 */
const getActivityColor = (type: RecentActivity['type']) => {
  const colors: Record<RecentActivity['type'], string> = {
    paper_added: '#67c23a',
    search: '#409eff',
    export: '#e6a23c',
    favorite: '#f56c6c',
    crawler_created: '#909399'
  }
  return colors[type] || '#909399'
}

/**
 * 获取活动图标
 */
const getActivityIcon = (type: RecentActivity['type']) => {
  const icons: Record<RecentActivity['type'], any> = {
    paper_added: Document,
    search: Search,
    export: Download,
    favorite: Star,
    crawler_created: Connection
  }
  return icons[type]
}

/**
 * 查看全部活动
 */
const handleViewAll = () => {
  emit('view-all')
}
</script>

<style scoped lang="scss">
.recent-activity {
  border: none;
  transition: all 0.3s ease;

  &:hover {
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.12) !important;
  }

  :deep(.el-card__header) {
    padding: 16px 20px;
    border-bottom: 1px solid #f0f0f0;
  }

  :deep(.el-card__body) {
    padding: 20px;
  }

  &__header {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  &__title {
    font-size: 16px;
    font-weight: 600;
    color: #303133;
  }

  &__loading,
  &__empty {
    padding: 20px 0;
  }

  &__timeline {
    padding-left: 0;

    :deep(.el-timeline-item__wrapper) {
      padding-left: 28px;
    }

    :deep(.el-timeline-item__timestamp) {
      font-size: 12px;
      color: #909399;
      margin-bottom: 4px;
    }
  }
}

.activity-item {
  &__title {
    font-size: 14px;
    font-weight: 500;
    color: #303133;
    margin-bottom: 4px;
  }

  &__description {
    font-size: 13px;
    color: #606266;
    line-height: 1.5;
    margin-bottom: 6px;
  }

  &__link {
    font-size: 12px;
    color: #409eff;
    text-decoration: none;
    transition: color 0.3s ease;

    &:hover {
      color: #66b1ff;
      text-decoration: underline;
    }
  }
}

// 响应式设计
@media (max-width: 768px) {
  .recent-activity__timeline {
    :deep(.el-timeline-item__wrapper) {
      padding-left: 20px;
    }
  }

  .activity-item {
    &__title {
      font-size: 13px;
    }

    &__description {
      font-size: 12px;
    }
  }
}
</style>
