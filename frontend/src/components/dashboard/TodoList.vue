<template>
  <el-card class="todo-list" shadow="hover">
    <template #header>
      <div class="todo-list__header">
        <div class="todo-list__title">
          <el-icon><List /></el-icon>
          <span>待办事项</span>
          <el-badge
            :value="pendingCount"
            :hidden="pendingCount === 0"
            class="todo-list__badge"
          />
        </div>
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

    <div v-if="loading" class="todo-list__loading">
      <el-skeleton :rows="3" animated />
    </div>

    <div v-else-if="todoItems.length === 0" class="todo-list__empty">
      <el-empty description="暂无待办事项" :image-size="60" />
    </div>

    <div v-else class="todo-list__items">
      <div
        v-for="todo in displayedTodos"
        :key="todo.id"
        class="todo-item"
        :class="[
          `todo-item--${todo.priority}`,
          { 'todo-item--completed': todo.status === 'completed' }
        ]"
      >
        <div class="todo-item__checkbox">
          <el-checkbox
            :model-value="todo.status === 'completed'"
            @change="handleToggleStatus(todo)"
          />
        </div>
        <div class="todo-item__content">
          <div class="todo-item__title">{{ todo.title }}</div>
          <div class="todo-item__description">{{ todo.description }}</div>
          <div class="todo-item__meta">
            <el-tag
              v-if="todo.dueDate"
              size="small"
              :type="getDueDateTagType(todo.dueDate, todo.status)"
            >
              {{ formatDueDate(todo.dueDate) }}
            </el-tag>
            <el-tag
              size="small"
              :type="getPriorityTagType(todo.priority)"
            >
              {{ getPriorityLabel(todo.priority) }}
            </el-tag>
          </div>
        </div>
        <div class="todo-item__actions">
          <el-button
            type="primary"
            link
            size="small"
            @click="handleAction(todo)"
          >
            处理
          </el-button>
        </div>
      </div>
    </div>

    <div v-if="todoItems.length > displayCount" class="todo-list__more">
      <el-button text type="primary" @click="handleViewMore">
        查看更多 ({{ todoItems.length - displayCount }})
      </el-button>
    </div>
  </el-card>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { List } from '@element-plus/icons-vue'
import { formatDistanceToNow, isPast, isToday, isTomorrow } from 'date-fns'
import { zhCN } from 'date-fns/locale'
import type { TodoItem } from '@/types/dashboard'

/**
 * 待办事项组件
 * 显示和管理待办事项列表
 */
interface Props {
  /** 待办事项列表 */
  todoItems?: TodoItem[]
  /** 是否加载中 */
  loading?: boolean
  /** 显示数量 */
  displayCount?: number
}

const props = withDefaults(defineProps<Props>(), {
  todoItems: () => [],
  loading: false,
  displayCount: 5
})

const emit = defineEmits<{
  (e: 'toggle-status', todo: TodoItem): void
  (e: 'action', todo: TodoItem): void
  (e: 'view-all'): void
  (e: 'view-more'): void
}>()

/**
 * 显示的待办事项
 */
const displayedTodos = computed(() => {
  return props.todoItems.slice(0, props.displayCount)
})

/**
 * 待处理数量
 */
const pendingCount = computed(() => {
  return props.todoItems.filter(
    (todo) => todo.status !== 'completed'
  ).length
})

/**
 * 获取优先级标签类型
 */
const getPriorityTagType = (priority: TodoItem['priority']) => {
  const types = {
    high: 'danger',
    medium: 'warning',
    low: 'info'
  }
  return types[priority]
}

/**
 * 获取优先级标签文本
 */
const getPriorityLabel = (priority: TodoItem['priority']) => {
  const labels = {
    high: '高优先级',
    medium: '中优先级',
    low: '低优先级'
  }
  return labels[priority]
}

/**
 * 获取截止日期标签类型
 */
const getDueDateTagType = (dueDate: string, status: TodoItem['status']) => {
  if (status === 'completed') return 'info'

  const date = new Date(dueDate)
  if (isPast(date) && !isToday(date)) return 'danger'
  if (isToday(date) || isTomorrow(date)) return 'warning'
  return 'success'
}

/**
 * 格式化截止日期
 */
const formatDueDate = (dueDate: string) => {
  try {
    const date = new Date(dueDate)
    if (isToday(date)) return '今天截止'
    if (isTomorrow(date)) return '明天截止'
    if (isPast(date)) return '已逾期'

    return formatDistanceToNow(date, { addSuffix: true, locale: zhCN })
  } catch {
    return dueDate
  }
}

/**
 * 切换待办状态
 */
const handleToggleStatus = (todo: TodoItem) => {
  emit('toggle-status', todo)
}

/**
 * 处理待办
 */
const handleAction = (todo: TodoItem) => {
  emit('action', todo)
}

/**
 * 查看全部
 */
const handleViewAll = () => {
  emit('view-all')
}

/**
 * 查看更多
 */
const handleViewMore = () => {
  emit('view-more')
}
</script>

<style scoped lang="scss">
.todo-list {
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
  }

  &__title {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 16px;
    font-weight: 600;
    color: #303133;

    .el-icon {
      color: #409eff;
    }
  }

  &__badge {
    margin-left: 8px;

    :deep(.el-badge__content) {
      background-color: #f56c6c;
    }
  }

  &__loading,
  &__empty {
    padding: 20px 0;
  }

  &__items {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  &__more {
    margin-top: 16px;
    text-align: center;
  }
}

.todo-item {
  display: flex;
  align-items: flex-start;
  gap: 12px;
  padding: 12px;
  border: 1px solid #e4e7ed;
  border-radius: 8px;
  background: #fff;
  transition: all 0.3s ease;

  &:hover {
    border-color: #409eff;
    box-shadow: 0 2px 8px rgba(64, 158, 255, 0.1);
  }

  &--completed {
    opacity: 0.6;

    .todo-item__title {
      text-decoration: line-through;
    }
  }

  &--high {
    border-left: 3px solid #f56c6c;
  }

  &--medium {
    border-left: 3px solid #e6a23c;
  }

  &--low {
    border-left: 3px solid #909399;
  }

  &__checkbox {
    padding-top: 2px;
  }

  &__content {
    flex: 1;
    min-width: 0;
  }

  &__title {
    font-size: 14px;
    font-weight: 500;
    color: #303133;
    margin-bottom: 4px;
    line-height: 1.4;
  }

  &__description {
    font-size: 12px;
    color: #606266;
    margin-bottom: 8px;
    line-height: 1.4;
    display: -webkit-box;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
    overflow: hidden;
  }

  &__meta {
    display: flex;
    gap: 6px;
    flex-wrap: wrap;
  }

  &__actions {
    flex-shrink: 0;
    padding-top: 2px;
  }
}

// 响应式设计
@media (max-width: 768px) {
  .todo-item {
    padding: 10px;

    &__title {
      font-size: 13px;
    }

    &__description {
      font-size: 11px;
    }

    &__actions {
      .el-button {
        font-size: 12px;
        padding: 4px 8px;
      }
    }
  }
}
</style>
