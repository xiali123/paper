<!--
  VirtualPaperList.vue - 虚拟滚动论文列表
  优化大量数据渲染性能
-->
<template>
  <div class="virtual-paper-list" ref="containerRef">
    <!-- 滚动容器 -->
    <div
      class="scroll-content"
      :style="{ height: `${totalHeight}px` }"
      @scroll="onScroll"
    >
      <!-- 可视区域 -->
      <div
        class="visible-items"
        :style="{
          transform: `translateY(${offsetY}px)`,
          height: `${visibleHeight}px`
        }"
      >
        <PaperCard
          v-for="paper in visiblePapers"
          :key="paper.id"
          :paper="paper"
          @click="onPaperClick(paper)"
          class="paper-item"
          :style="{ height: `${itemHeight}px` }"
        />
      </div>
    </div>

    <!-- 加载状态 -->
    <div v-if="loading" class="loading-indicator">
      <v-progress-circular indeterminate />
      <span>加载中...</span>
    </div>

    <!-- 快速跳转 -->
    <div class="jump-controls" v-if="showJumpControls">
      <v-text-field
        v-model.number="jumpPage"
        label="跳转到页"
        type="number"
        :min="1"
        :max="totalPages"
        prepend-icon="mdi-page-next"
        @keyup.enter="jumpToPage"
        style="max-width: 150px"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
import { usePapersStore } from '../stores/papers'
import PaperCard from './PaperCard.vue'
import type { Paper } from '../types'

// Props
interface Props {
  itemHeight?: number // 每个item的高度（px）
  bufferSize?: number // 缓冲区大小（额外渲染的item数量）
  threshold?: number // 触发加载的阈值（距离底部多少px）
}

const props = withDefaults(defineProps<Props>(), {
  itemHeight: 120,
  bufferSize: 5,
  threshold: 200
})

// Store
const papersStore = usePapersStore()

// Refs
const containerRef = ref<HTMLElement>()
const jumpPage = ref(1)

// State
const scrollTop = ref(0)
const containerHeight = ref(600)
const loading = ref(false)
const showJumpControls = ref(false)

// Computed
const visiblePapers = computed(() => {
  const start = startIndex.value
  const end = endIndex.value
  return papersStore.searchResults.slice(start, end)
})

const totalHeight = computed(() => {
  return papersStore.searchTotal * props.itemHeight
})

const startIndex = computed(() => {
  const start = Math.floor(scrollTop.value / props.itemHeight)
  return Math.max(0, start - props.bufferSize)
})

const endIndex = computed(() => {
  const visibleCount = Math.ceil(containerHeight.value / props.itemHeight)
  const end = startIndex.value + visibleCount + props.bufferSize * 2
  return Math.min(papersStore.searchResults.length, end)
})

const offsetY = computed(() => {
  return startIndex.value * props.itemHeight
})

const visibleHeight = computed(() => {
  return (endIndex.value - startIndex.value) * props.itemHeight
})

const totalPages = computed(() => {
  return papersStore.totalPages
})

// Methods
const onScroll = (event: Event) => {
  const target = event.target as HTMLElement
  scrollTop.value = target.scrollTop

  // 检查是否需要加载更多
  const scrollBottom = target.scrollHeight - target.scrollTop - target.clientHeight
  if (scrollBottom < props.threshold && papersStore.hasNextPage && !loading.value) {
    loadMore()
  }
}

const loadMore = async () => {
  if (loading.value || !papersStore.hasNextPage) return

  loading.value = true
  try {
    await papersStore.loadNextPage()
  } catch (error) {
    console.error('Failed to load more papers:', error)
  } finally {
    loading.value = false
  }
}

const onPaperClick = (paper: Paper) => {
  papersStore.addToRecentlyViewed(paper)
  // 导航到详情页
  // router.push(`/papers/${paper.id}`)
}

const jumpToPage = async () => {
  const page = jumpPage.value
  if (page < 1 || page > totalPages.value) return

  loading.value = true
  try {
    await papersStore.searchPapers({
      q: papersStore.currentQuery.q,
      page,
      pageSize: papersStore.pageSize
    })
  } catch (error) {
    console.error('Failed to jump to page:', error)
  } finally {
    loading.value = false
    showJumpControls.value = false
  }
}

const scrollToTop = () => {
  if (containerRef.value) {
    containerRef.value.scrollTop = 0
  }
}

const scrollToItem = (index: number) => {
  if (containerRef.value) {
    const targetScroll = index * props.itemHeight
    containerRef.value.scrollTop = targetScroll
  }
}

// 快捷键支持
const handleKeydown = (event: KeyboardEvent) => {
  // Ctrl/Cmd + J: 显示跳转控件
  if ((event.ctrlKey || event.metaKey) && event.key === 'j') {
    event.preventDefault()
    showJumpControls.value = !showJumpControls.value
  }

  // Home: 回到顶部
  if (event.key === 'Home' && !event.ctrlKey && !event.metaKey) {
    event.preventDefault()
    scrollToTop()
  }

  // End: 跳到底部
  if (event.key === 'End' && !event.ctrlKey && !event.metaKey) {
    event.preventDefault()
    if (containerRef.value) {
      containerRef.value.scrollTop = containerRef.value.scrollHeight
    }
  }
}

// Lifecycle
onMounted(() => {
  if (containerRef.value) {
    containerHeight.value = containerRef.value.clientHeight
  }

  // 监听窗口大小变化
  window.addEventListener('resize', () => {
    if (containerRef.value) {
      containerHeight.value = containerRef.value.clientHeight
    }
  })

  // 监听快捷键
  document.addEventListener('keydown', handleKeydown)
})

onUnmounted(() => {
  document.removeEventListener('keydown', handleKeydown)
})

// Watch for search results changes
watch(() => papersStore.searchResults.length, () => {
  // 当搜索结果更新时，重置滚动位置
  if (papersStore.currentPage === 1) {
    scrollToTop()
  }
})

// 暴露方法供父组件调用
defineExpose({
  scrollToTop,
  scrollToItem,
  jumpToPage
})
</script>

<style scoped>
.virtual-paper-list {
  position: relative;
  height: 100%;
  overflow: hidden;
}

.scroll-content {
  overflow-y: auto;
  overflow-x: hidden;
  will-change: scroll-position;
}

.visible-items {
  position: relative;
  will-change: transform;
}

.paper-item {
  box-sizing: border-box;
  padding: 8px;
  margin-bottom: 8px;
}

.loading-indicator {
  position: absolute;
  bottom: 20px;
  left: 50%;
  transform: translateX(-50%);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  background: rgba(255, 255, 255, 0.95);
  padding: 16px 24px;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  z-index: 10;
}

.jump-controls {
  position: absolute;
  top: 20px;
  right: 20px;
  background: rgba(255, 255, 255, 0.95);
  padding: 16px;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  z-index: 10;
}

/* 自定义滚动条 */
.scroll-content::-webkit-scrollbar {
  width: 8px;
}

.scroll-content::-webkit-scrollbar-track {
  background: #f1f1f1;
  border-radius: 4px;
}

.scroll-content::-webkit-scrollbar-thumb {
  background: #888;
  border-radius: 4px;
}

.scroll-content::-webkit-scrollbar-thumb:hover {
  background: #555;
}

/* 性能优化 */
.paper-item {
  contain: layout style paint;
}
</style>
