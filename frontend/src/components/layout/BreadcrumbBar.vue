<template>
  <nav class="breadcrumb-bar" aria-label="Breadcrumb navigation">
    <el-breadcrumb separator="/" class="breadcrumb">
      <el-breadcrumb-item
        v-for="(item, index) in breadcrumbItems"
        :key="index"
        :to="item.to"
        :class="{ 'is-current': index === breadcrumbItems.length - 1 }"
      >
        <el-icon v-if="item.icon">
          <component :is="item.icon" />
        </el-icon>
        <span>{{ item.title }}</span>
      </el-breadcrumb-item>
    </el-breadcrumb>

    <!-- Page Actions (Optional Slot) -->
    <div v-if="$slots.actions" class="breadcrumb-actions">
      <slot name="actions" />
    </div>
  </nav>
</template>

<script setup lang="ts">
import { computed, h } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import {
  Odometer,
  Document,
  Collection,
  Star,
  Folder,
  PriceTag,
  Connection,
  Search,
  DataAnalysis,
  DataLine,
  PieChart,
  Clock,
  Download,
  Setting,
  User,
  Tools,
  Operation
} from '@element-plus/icons-vue'
import { useI18n } from 'vue-i18n'

const route = useRoute()
const router = useRouter()
const { t } = useI18n()

// Icon mapping
const iconMap: Record<string, any> = {
  dashboard: Odometer,
  papers: Document,
  'papers-all': Collection,
  'papers-favorites': Star,
  'papers-categories': Folder,
  'papers-tags': PriceTag,
  crawler: Connection,
  search: Search,
  statistics: DataAnalysis,
  'statistics-overview': DataLine,
  'statistics-charts': PieChart,
  'statistics-timeline': Clock,
  export: Download,
  settings: Setting,
  'settings-profile': User,
  'settings-preferences': Tools,
  'settings-system': Operation
}

// Build breadcrumb items from route
const breadcrumbItems = computed(() => {
  const items: Array<{
    title: string
    to?: string
    icon?: any
  }> = []

  // Add home
  items.push({
    title: t('nav.home'),
    to: '/',
    icon: Odometer
  })

  // Get matched routes with meta information
  const matchedRoutes = route.matched.filter(item => item.meta?.title || item.name)

  matchedRoutes.forEach((matchedRoute, index) => {
    const routeName = String(matchedRoute.name || '')
    const iconKey = routeName.replace(/-/g, '').toLowerCase()
    const icon = iconMap[iconKey]

    items.push({
      title: matchedRoute.meta?.title
        ? t(matchedRoute.meta.title as string)
        : String(matchedRoute.name),
      to: index === matchedRoutes.length - 1 ? undefined : matchedRoute.path,
      icon
    })
  })

  // Add dynamic params if available
  if (route.params.id) {
    items.push({
      title: String(route.params.id),
      to: undefined
    })
  }

  return items
})
</script>

<style scoped lang="scss">
.breadcrumb-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--space-4);
  padding: var(--space-4) 0;
  min-height: 48px;
}

.breadcrumb {
  flex: 1;

  :deep(.el-breadcrumb__item) {
    .el-breadcrumb__inner {
      display: flex;
      align-items: center;
      gap: var(--space-2);
      color: var(--gray-600);
      font-size: var(--font-sm);
      transition: color var(--duration-fast);

      &:hover {
        color: var(--primary-600);
      }

      .dark & {
        color: var(--gray-400);

        &:hover {
          color: var(--primary-400);
        }
      }

      .el-icon {
        font-size: var(--font-base);
      }
    }

    &:last-child {
      .el-breadcrumb__inner {
        color: var(--gray-900);
        font-weight: var(--font-medium);
        cursor: default;

        &:hover {
          color: var(--gray-900);
        }

        .dark & {
          color: var(--gray-100);
        }
      }
    }

    .el-breadcrumb__separator {
      color: var(--gray-400);
      margin: 0 var(--space-2);

      .dark & {
        color: var(--gray-600);
      }
    }
  }
}

.breadcrumb-actions {
  flex-shrink: 0;
  display: flex;
  align-items: center;
  gap: var(--space-3);
}

// Responsive Design
@media (max-width: 768px) {
  .breadcrumb-bar {
    flex-direction: column;
    align-items: flex-start;
    gap: var(--space-3);

    .breadcrumb {
      width: 100%;
      overflow-x: auto;
      white-space: nowrap;

      // Hide scrollbar
      &::-webkit-scrollbar {
        display: none;
      }
      -ms-overflow-style: none;
      scrollbar-width: none;
    }
  }

  .breadcrumb-actions {
    width: 100%;
    justify-content: flex-end;
  }
}

@media (prefers-reduced-motion: reduce) {
  .breadcrumb {
    :deep(.el-breadcrumb__item .el-breadcrumb__inner) {
      transition: none;
    }
  }
}
</style>
