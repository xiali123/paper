<!--
  ModernPaperCard.vue - 现代化论文卡片组件
  使用新的设计系统
-->
<template>
  <div
    class="modern-paper-card"
    :class="{
      'is-selected': isSelected,
      'is-favorite': isFavorite,
      'is-interactive': interactive
    }"
    @click="handleClick"
    @mouseenter="onMouseEnter"
    @mouseleave="onMouseLeave"
  >
    <!-- 顶部：标题和操作栏 -->
    <div class="card-header">
      <h3 class="paper-title" v-html="highlightedTitle"></h3>

      <div class="card-actions">
        <v-btn
          icon
          size="small"
          variant="text"
          @click.stop="toggleFavorite"
          :color="isFavorite ? 'error' : 'default'"
        >
          <v-icon>{{ isFavorite ? 'mdi-heart' : 'mdi-heart-outline' }}</v-icon>
        </v-btn>

        <v-menu>
          <template v-slot:activator="{ props }">
            <v-btn icon="mdi-dots-vertical" variant="text" size="small" v-bind="props" />
          </template>

          <v-list>
            <v-list-item @click="copyCitation">
              <v-list-item-title>复制引用</v-list-item-title>
            </v-list-item>
            <v-list-item @click="exportBibTex">
              <v-list-item-title>导出 BibTeX</v-list-item-title>
            </v-list-item>
            <v-list-item @click="openDetails">
              <v-list-item-title>查看详情</v-list-item-title>
            </v-list-item>
            <v-divider />
            <v-list-item @click="toggleSelect">
              <v-list-item-title>{{ isSelected ? '取消选择' : '选择' }}</v-list-item-title>
            </v-list-item>
          </v-list>
        </v-menu>
      </div>
    </div>

    <!-- 中部：作者和元数据 -->
    <div class="card-body">
      <div class="authors">
        <v-icon size="small" color="grey">mdi-account-multiple</v-icon>
        <span class="authors-text">{{ truncatedAuthors }}</span>
      </div>

      <div class="metadata">
        <v-chip size="x-small" color="primary" variant="outlined">
          <v-icon start size="x-small">mdi-calendar</v-icon>
          {{ paper.year }}
        </v-chip>

        <v-chip size="x-small" color="info" variant="outlined">
          <v-icon start size="x-small">mdi-book-open-variant</v-icon>
          {{ truncatedVenue }}
        </v-chip>

        <v-chip
          size="x-small"
          :color="citationColor"
          variant="outlined"
        >
          <v-icon start size="x-small">mdi-quote-open</v-icon>
          {{ paper.citation_count }} 引用
        </v-chip>
      </div>
    </div>

    <!-- 底部：标签和操作 -->
    <div class="card-footer">
      <div class="tags">
        <v-chip
          v-for="tag in tags"
          :key="tag"
          size="x-small"
          variant="tonal"
          color="grey"
        >
          {{ tag }}
        </v-chip>
      </div>

      <v-btn
        v-if="interactive"
        size="small"
        color="primary"
        variant="text"
        @click.stop="openDetails"
      >
        查看详情
        <v-icon end size="small">mdi-arrow-right</v-icon>
      </v-btn>
    </div>

    <!-- 选中指示器 -->
    <div v-if="isSelected" class="selection-indicator">
      <v-icon color="primary">mdi-check-circle</v-icon>
    </div>

    <!-- 悬浮效果指示器 -->
    <div v-if="isHovered" class="hover-indicator"></div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import DOMPurify from 'dompurify'
import { usePapersStore } from '../stores/papers'
import type { Paper } from '../types'

// Props
interface Props {
  paper: Paper
  interactive?: boolean
  highlightKeyword?: string
}

const props = withDefaults(defineProps<Props>(), {
  interactive: true,
  highlightKeyword: ''
})

// Store
const papersStore = usePapersStore()

// State
const isHovered = ref(false)

// Computed
const isSelected = computed(() => {
  return papersStore.isSelected(props.paper.id)
})

const isFavorite = computed(() => {
  return papersStore.isFavorite(props.paper.id)
})

const truncatedAuthors = computed(() => {
  const authors = props.paper.authors
  return authors.length > 60 ? authors.substring(0, 60) + '...' : authors
})

const truncatedVenue = computed(() => {
  const venue = props.paper.venue
  return venue.length > 30 ? venue.substring(0, 30) + '...' : venue
})

const highlightedTitle = computed(() => {
  if (!props.highlightKeyword) return props.paper.title

  const keyword = props.highlightKeyword
  const title = props.paper.title
  const escaped = keyword.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
  const regex = new RegExp(`(${escaped})`, 'gi')
  const highlighted = title.replace(regex, '<mark>$1</mark>')
  return DOMPurify.sanitize(highlighted, { ALLOWED_TAGS: ['mark'] })
})

const citationColor = computed(() => {
  const count = props.paper.citation_count
  if (count >= 100) return 'success'
  if (count >= 50) return 'info'
  if (count >= 10) return 'warning'
  return 'grey'
})

const tags = computed(() => {
  // 从标题或摘要中提取标签（示例）
  const tags: string[] = []
  const title = props.paper.title.toLowerCase()

  if (title.includes('testing')) tags.push('Testing')
  if (title.includes('machine learning')) tags.push('ML')
  if (title.includes('deep learning')) tags.push('Deep Learning')
  if (title.includes('software')) tags.push('Software Engineering')

  return tags.slice(0, 3)
})

// Methods
const handleClick = () => {
  if (!props.interactive) return
  papersStore.addToRecentlyViewed(props.paper)
  openDetails()
}

const toggleFavorite = () => {
  papersStore.toggleFavorite(props.paper.id)
}

const toggleSelect = () => {
  papersStore.toggleSelectPaper(props.paper.id)
}

const openDetails = () => {
  // 导航到详情页
  console.log('Opening details for paper:', props.paper.id)
}

const copyCitation = () => {
  const citation = `${props.paper.authors} (${props.paper.year}). ${props.paper.title}. ${props.paper.venue}.`
  navigator.clipboard.writeText(citation)
  console.log('Citation copied:', citation)
}

const exportBibTex = () => {
  const bibTeX = `@article{${props.paper.id},
  title={${props.paper.title}},
  author={${props.paper.authors}},
  year={${props.paper.year}},
  venue={${props.paper.venue}}
}`
  navigator.clipboard.writeText(bibTeX)
  console.log('BibTeX exported:', bibTeX)
}

const onMouseEnter = () => {
  isHovered.value = true
}

const onMouseLeave = () => {
  isHovered.value = false
}
</script>

<style scoped lang="scss">
@import '../styles/design-system.scss';

.modern-paper-card {
  position: relative;
  background: white;
  border-radius: map-get($border-radius, 'lg');
  padding: map-get($spacing, '6');
  box-shadow: map-get($box-shadow, 'sm');
  transition: all map-get($transition-duration, 'base') map-get($transition-timing, 'ease-in-out');
  cursor: pointer;
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 3px;
    background: linear-gradient(90deg, map-get($colors, 'primary-500'), map-get($colors, 'secondary-500'));
    opacity: 0;
    transition: opacity map-get($transition-duration, 'base');
  }

  &:hover {
    box-shadow: map-get($box-shadow, 'lg');
    transform: translateY(-2px);

    &::before {
      opacity: 1;
    }
  }

  &.is-selected {
    border: 2px solid map-get($colors, 'primary-500');
    background: map-get($colors, 'primary-50');
  }

  &.is-interactive {
    cursor: pointer;
  }
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: map-get($spacing, '4');
  margin-bottom: map-get($spacing, '4');
}

.paper-title {
  margin: 0;
  font-size: map-get($font-size, 'lg');
  font-weight: map-get($font-weight, 'semibold');
  line-height: map-get($line-height, 'tight');
  color: map-get($colors, 'gray-900');
  flex: 1;

  :deep(mark) {
    background: yellow;
    padding: 0 2px;
    border-radius: 2px;
  }
}

.card-actions {
  display: flex;
  gap: map-get($spacing, '1');
  flex-shrink: 0;
}

.card-body {
  display: flex;
  flex-direction: column;
  gap: map-get($spacing, '3');
  margin-bottom: map-get($spacing, '4');
}

.authors {
  display: flex;
  align-items: center;
  gap: map-get($spacing, '2');
  color: map-get($colors, 'gray-600');
  font-size: map-get($font-size, 'sm');
}

.authors-text {
  @include text-clamp(1);
}

.metadata {
  display: flex;
  flex-wrap: wrap;
  gap: map-get($spacing, '2');
}

.card-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: map-get($spacing, '4');
  padding-top: map-get($spacing, '4');
  border-top: 1px solid map-get($colors, 'gray-200');
}

.tags {
  display: flex;
  flex-wrap: wrap;
  gap: map-get($spacing, '2');
  flex: 1;
}

.selection-indicator {
  position: absolute;
  top: map-get($spacing, '4');
  right: map-get($spacing, '4');
  width: 24px;
  height: 24px;
  background: map-get($colors, 'primary-500');
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  animation: scale-in 0.2s ease-out;
}

.hover-indicator {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  pointer-events: none;
  border-radius: map-get($border-radius, 'lg');
  box-shadow: inset 0 0 0 2px map-get($colors, 'primary-500');
  opacity: 0.1;
}

@keyframes scale-in {
  from {
    transform: scale(0);
    opacity: 0;
  }
  to {
    transform: scale(1);
    opacity: 1;
  }
}

// 响应式设计
@include respond-to('sm') {
  .modern-paper-card {
    padding: map-get($spacing, '4');
  }

  .paper-title {
    font-size: map-get($font-size, 'base');
  }
}
</style>
