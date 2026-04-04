<template>
  <div class="quick-actions">
    <h3 class="quick-actions__title">快速操作</h3>
    <div class="quick-actions__grid">
      <router-link
        v-for="action in actions"
        :key="action.id"
        :to="action.route"
        class="quick-action-card"
        :class="`quick-action-card--${action.type}`"
      >
        <div class="quick-action-card__icon">
          <el-icon :size="28">
            <component :is="action.icon" />
          </el-icon>
        </div>
        <div class="quick-action-card__content">
          <div class="quick-action-card__label">{{ action.label }}</div>
          <div class="quick-action-card__description">{{ action.description }}</div>
        </div>
        <el-icon class="quick-action-card__arrow" :size="16">
          <ArrowRight />
        </el-icon>
      </router-link>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, markRaw } from 'vue'
import {
  DocumentAdd,
  Connection,
  Download,
  DataAnalysis,
  ArrowRight
} from '@element-plus/icons-vue'
import type { QuickAction } from '@/types/dashboard'

/**
 * 快速操作组件
 * 提供常用功能的快捷入口
 */
const actions = ref<QuickAction[]>([
  {
    id: 'add-paper',
    icon: markRaw(DocumentAdd),
    label: '添加论文',
    description: '手动添加或导入论文',
    route: '/papers/add',
    type: 'primary'
  },
  {
    id: 'create-crawler',
    icon: markRaw(Connection),
    label: '创建爬虫',
    description: '配置新的爬虫任务',
    route: '/crawler',
    type: 'success'
  },
  {
    id: 'export-data',
    icon: markRaw(Download),
    label: '导出数据',
    description: '导出论文数据',
    route: '/export',
    type: 'warning'
  },
  {
    id: 'view-stats',
    icon: markRaw(DataAnalysis),
    label: '查看统计',
    description: '查看详细统计数据',
    route: '/stats',
    type: 'info'
  }
])
</script>

<style scoped lang="scss">
.quick-actions {
  &__title {
    font-size: 16px;
    font-weight: 600;
    color: #303133;
    margin-bottom: 16px;
  }

  &__grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 12px;
  }
}

.quick-action-card {
  position: relative;
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 16px;
  background: #fff;
  border: 1px solid #e4e7ed;
  border-radius: 8px;
  text-decoration: none;
  transition: all 0.3s ease;
  cursor: pointer;

  &:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
    border-color: transparent;

    .quick-action-card__arrow {
      transform: translateX(4px);
    }
  }

  &:active {
    transform: translateY(0);
  }

  &__icon {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 48px;
    height: 48px;
    border-radius: 8px;
    flex-shrink: 0;
    color: #fff;
  }

  &__content {
    flex: 1;
    min-width: 0;
  }

  &__label {
    font-size: 14px;
    font-weight: 600;
    color: #303133;
    margin-bottom: 2px;
  }

  &__description {
    font-size: 12px;
    color: #909399;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  &__arrow {
    color: #c0c4cc;
    transition: transform 0.3s ease;
    flex-shrink: 0;
  }

  // 类型样式
  &--primary .quick-action-card__icon {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  }

  &--success .quick-action-card__icon {
    background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
  }

  &--warning .quick-action-card__icon {
    background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
  }

  &--info .quick-action-card__icon {
    background: linear-gradient(135deg, #30cfd0 0%, #330867 100%);
  }

  &--danger .quick-action-card__icon {
    background: linear-gradient(135deg, #fa709a 0%, #fee140 100%);
  }
}

// 响应式设计
@media (max-width: 768px) {
  .quick-actions__grid {
    grid-template-columns: 1fr;
  }

  .quick-action-card {
    padding: 12px;

    &__icon {
      width: 40px;
      height: 40px;

      :deep(.el-icon) {
        font-size: 20px !important;
      }
    }

    &__label {
      font-size: 13px;
    }

    &__description {
      font-size: 11px;
    }
  }
}
</style>
