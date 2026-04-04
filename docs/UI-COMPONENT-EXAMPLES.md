# PaperCrawler UI Component Examples
## 组件实现示例与最佳实践

**版本**: 2.0.0
**更新日期**: 2026-04-04
**配套文档**: [UI-DESIGN-SYSTEM.md](./UI-DESIGN-SYSTEM.md)

---

## 目录

1. [论文卡片组件](#论文卡片组件-paper-card)
2. [搜索栏组件](#搜索栏组件-search-bar)
3. [数据表格组件](#数据表格组件-data-table)
4. [过滤器组件](#过滤器组件-filter-panel)
5. [爬虫任务监控组件](#爬虫任务监控组件-crawler-monitor)
6. [统计图表组件](#统计图表组件-chart-components)
7. [布局组件](#布局组件-layout-components)
8. [最佳实践](#最佳实践)

---

## 论文卡片组件 (Paper Card)

### 标准论文卡片

```vue
<!-- PaperCard.vue -->
<template>
  <article
    class="paper-card"
    :class="cardClasses"
    @click="handleClick"
    @mouseenter="isHovered = true"
    @mouseleave="isHovered = false"
  >
    <!-- 顶部渐变边框 -->
    <div class="paper-card__border" />

    <!-- 卡片头部 -->
    <header class="paper-card__header">
      <div class="paper-card__title-section">
        <h3 class="paper-card__title">
          <mark
            v-if="highlightKeyword"
            class="paper-card__highlight"
            v-html="highlightedTitle"
          />
          <span v-else>{{ paper.title }}</span>
        </h3>

        <!-- CCF 等级徽章 -->
        <span
          v-if="paper.ccf_level"
          class="paper-card__badge"
          :class="`paper-card__badge--${paper.ccf_level.toLowerCase()}`"
        >
          CCF {{ paper.ccf_level }}
        </span>
      </div>

      <!-- 操作按钮 -->
      <div class="paper-card__actions">
        <button
          class="paper-card__action"
          :class="{ 'paper-card__action--active': isFavorite }"
          @click.stop="toggleFavorite"
          :aria-label="isFavorite ? '从收藏移除' : '添加到收藏'"
        >
          <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
            <path
              :d="isFavorite
                ? 'M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z'
                : 'M16.5 3c-1.74 0-3.41.81-4.5 2.09C10.91 3.81 9.24 3 7.5 3 4.42 3 2 5.42 2 8.5c0 3.78 3.4 6.86 8.55 11.54L12 21.35l1.45-1.32C18.6 15.36 22 12.28 22 8.5 22 5.42 19.58 3 16.5 3zm-4.4 15.55l-.1.1-.1-.1C7.14 14.24 4 11.39 4 8.5 4 6.5 5.5 5 7.5 5c1.54 0 3.04.99 3.57 2.36h1.87C13.46 5.99 14.96 5 16.5 5c2 0 3.5 1.5 3.5 3.5 0 2.89-3.14 5.74-7.9 10.05z'"
            />
          </svg>
        </button>

        <button
          class="paper-card__action"
          @click.stop="showMenu = !showMenu"
          :aria-expanded="showMenu"
          aria-haspopup="true"
        >
          <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
            <path d="M12 8c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm0 2c-1.1 0-2 .9-2 2s.9 2 2 2 2-.9 2-2-.9-2-2-2zm0 6c-1.1 0-2 .9-2 2s.9 2 2 2 2-.9 2-2-.9-2-2-2z" />
          </svg>
        </button>

        <!-- 下拉菜单 -->
        <div v-if="showMenu" class="paper-card__menu">
          <button @click.stop="copyCitation">复制引用</button>
          <button @click.stop="exportBibTeX">导出 BibTeX</button>
          <button @click.stop="viewDetails">查看详情</button>
        </div>
      </div>
    </header>

    <!-- 卡片主体 -->
    <section class="paper-card__body">
      <!-- 作者信息 -->
      <div class="paper-card__authors">
        <svg class="paper-card__icon" width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
          <path d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" />
        </svg>
        <span class="paper-card__authors-text">{{ truncatedAuthors }}</span>
      </div>

      <!-- 元数据标签 -->
      <div class="paper-card__metadata">
        <span class="paper-card__tag paper-card__tag--year">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor">
            <path d="M19 3h-1V1h-2v2H8V1H6v2H5c-1.11 0-1.99.9-1.99 2L3 19c0 1.1.89 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm0 16H5V8h14v11z" />
          </svg>
          {{ paper.year }}
        </span>

        <span class="paper-card__tag paper-card__tag--venue">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor">
            <path d="M18 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM6 4h5v8l-2.5-1.5L6 12V4z" />
          </svg>
          {{ truncatedVenue }}
        </span>

        <span
          class="paper-card__tag"
          :class="`paper-card__tag--citation-${citationLevel}`"
        >
          <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor">
            <path d="M18 17H6v-2h12v2zm0-4H6v-2h12v2zm0-4H6V7h12v2zM3 22l1.5-1.5L6 22l1.5-1.5L9 22l1.5-1.5L12 22l1.5-1.5L15 22l1.5-1.5L18 22l1.5-1.5L21 22V2l-1.5 1.5L18 2l-1.5 1.5L15 2l-1.5 1.5L12 2l-1.5 1.5L9 2 7.5 3.5 6 2 4.5 3.5 3 2v20z" />
          </svg>
          {{ paper.citation_count }} 引用
        </span>
      </div>

      <!-- 自动生成的标签 -->
      <div v-if="tags.length > 0" class="paper-card__tags">
        <span
          v-for="tag in tags"
          :key="tag"
          class="paper-card__tag paper-card__tag--auto"
        >
          #{{ tag }}
        </span>
      </div>
    </section>

    <!-- 卡片底部 -->
    <footer class="paper-card__footer">
      <button
        v-if="interactive"
        class="paper-card__view-btn"
        @click.stop="viewDetails"
      >
        查看详情
        <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
          <path d="M12 4l-1.41 1.41L16.17 11H4v2h12.17l-5.58 5.59L12 20l8-8z" />
        </svg>
      </button>
    </footer>

    <!-- 选中状态指示器 -->
    <div v-if="isSelected" class="paper-card__selection-indicator">
      <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
        <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z" />
      </svg>
    </div>
  </article>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'

interface Paper {
  id: string
  title: string
  authors: string
  year: number
  venue: string
  citation_count: number
  ccf_level?: 'A' | 'B' | 'C'
}

interface Props {
  paper: Paper
  interactive?: boolean
  highlightKeyword?: string
  isSelected?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  interactive: true,
  highlightKeyword: '',
  isSelected: false
})

const emit = defineEmits<{
  click: [paper: Paper]
  favorite: [paper: Paper]
  view: [paper: Paper]
}>()

const isHovered = ref(false)
const isFavorite = ref(false)
const showMenu = ref(false)

const cardClasses = computed(() => ({
  'paper-card--hovered': isHovered.value,
  'paper-card--selected': props.isSelected,
  'paper-card--interactive': props.interactive
}))

const truncatedAuthors = computed(() => {
  const maxLength = 60
  return props.paper.authors.length > maxLength
    ? props.paper.authors.substring(0, maxLength) + '...'
    : props.paper.authors
})

const truncatedVenue = computed(() => {
  const maxLength = 30
  return props.paper.venue.length > maxLength
    ? props.paper.venue.substring(0, maxLength) + '...'
    : props.paper.venue
})

const highlightedTitle = computed(() => {
  if (!props.highlightKeyword) return props.paper.title

  const regex = new RegExp(`(${props.highlightKeyword})`, 'gi')
  return props.paper.title.replace(regex, '<mark class="paper-card__highlight">$1</mark>')
})

const citationLevel = computed(() => {
  const count = props.paper.citation_count
  if (count >= 100) return 'very-high'
  if (count >= 50) return 'high'
  if (count >= 10) return 'medium'
  return 'low'
})

const tags = computed(() => {
  const title = props.paper.title.toLowerCase()
  const tags: string[] = []

  if (title.includes('testing')) tags.push('Testing')
  if (title.includes('machine learning')) tags.push('ML')
  if (title.includes('deep learning')) tags.push('Deep Learning')
  if (title.includes('software')) tags.push('Software Engineering')

  return tags.slice(0, 3)
})

const handleClick = () => {
  if (!props.interactive) return
  emit('click', props.paper)
}

const toggleFavorite = () => {
  isFavorite.value = !isFavorite.value
  emit('favorite', props.paper)
}

const viewDetails = () => {
  emit('view', props.paper)
}

const copyCitation = () => {
  const citation = `${props.paper.authors} (${props.paper.year}). ${props.paper.title}. ${props.paper.venue}.`
  navigator.clipboard.writeText(citation)
  showMenu.value = false
}

const exportBibTeX = () => {
  const bibTeX = `@article{${props.paper.id},
  title={${props.paper.title}},
  author={${props.paper.authors}},
  year={${props.paper.year}},
  venue={${props.paper.venue}}
}`
  navigator.clipboard.writeText(bibTeX)
  showMenu.value = false
}
</script>

<style scoped lang="scss">
.paper-card {
  position: relative;
  background: white;
  border-radius: var(--radius-lg);
  padding: var(--space-5);
  box-shadow: var(--shadow-sm);
  transition: all var(--duration-base) var(--ease-out);
  cursor: default;
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 3px;
    background: linear-gradient(90deg, var(--primary-500), var(--secondary-500));
    opacity: 0;
    transition: opacity var(--duration-base);
  }

  &--interactive {
    cursor: pointer;
  }

  &--hovered {
    box-shadow: var(--shadow-lg);
    transform: translateY(-2px);

    &::before {
      opacity: 1;
    }
  }

  &--selected {
    border: 2px solid var(--primary-500);
    background: var(--primary-50);
  }
}

.paper-card__header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: var(--space-4);
  margin-bottom: var(--space-4);
}

.paper-card__title-section {
  flex: 1;
  min-width: 0;
}

.paper-card__title {
  margin: 0 0 var(--space-2) 0;
  font-size: var(--font-lg);
  font-weight: var(--font-semibold);
  line-height: var(--leading-tight);
  color: var(--gray-900);
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.paper-card__highlight {
  background: yellow;
  padding: 0 2px;
  border-radius: 2px;
}

.paper-card__badge {
  display: inline-block;
  padding: var(--space-1) var(--space-2);
  font-size: var(--font-xs);
  font-weight: var(--font-semibold);
  border-radius: var(--radius-sm);

  &--a {
    background: var(--ccf-a-bg);
    color: var(--ccf-a-text);
    border: 1px solid var(--ccf-a-border);
  }

  &--b {
    background: var(--ccf-b-bg);
    color: var(--ccf-b-text);
    border: 1px solid var(--ccf-b-border);
  }

  &--c {
    background: var(--ccf-c-bg);
    color: var(--ccf-c-text);
    border: 1px solid var(--ccf-c-border);
  }
}

.paper-card__actions {
  display: flex;
  gap: var(--space-1);
  flex-shrink: 0;
}

.paper-card__action {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 36px;
  height: 36px;
  border: none;
  background: transparent;
  border-radius: var(--radius-md);
  color: var(--gray-600);
  cursor: pointer;
  transition: all var(--duration-fast);

  &:hover {
    background: var(--gray-100);
    color: var(--gray-900);
  }

  &:focus-visible {
    outline: 2px solid var(--primary-500);
    outline-offset: 2px;
  }

  &--active {
    color: var(--error-500);
  }
}

.paper-card__menu {
  position: absolute;
  top: 48px;
  right: var(--space-4);
  background: white;
  border-radius: var(--radius-md);
  box-shadow: var(--shadow-lg);
  border: 1px solid var(--gray-200);
  z-index: var(--z-dropdown);
  min-width: 150px;

  button {
    display: block;
    width: 100%;
    padding: var(--space-3) var(--space-4);
    border: none;
    background: transparent;
    text-align: left;
    cursor: pointer;
    font-size: var(--font-sm);

    &:hover {
      background: var(--gray-50);
    }

    &:first-child {
      border-radius: var(--radius-md) var(--radius-md) 0 0;
    }

    &:last-child {
      border-radius: 0 0 var(--radius-md) var(--radius-md);
    }
  }
}

.paper-card__body {
  display: flex;
  flex-direction: column;
  gap: var(--space-3);
}

.paper-card__authors {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  color: var(--gray-600);
  font-size: var(--font-sm);
}

.paper-card__icon {
  flex-shrink: 0;
}

.paper-card__authors-text {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.paper-card__metadata {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-2);
}

.paper-card__tag {
  display: inline-flex;
  align-items: center;
  gap: var(--space-1);
  padding: var(--space-1) var(--space-2);
  font-size: var(--font-xs);
  border-radius: var(--radius-full);
  border: 1px solid var(--gray-300);
  background: var(--gray-50);
  color: var(--gray-700);

  &--year {
    color: var(--primary-700);
    background: var(--primary-50);
    border-color: var(--primary-200);
  }

  &--venue {
    color: var(--info-700);
    background: var(--info-50);
    border-color: var(--info-200);
  }

  &--citation-low {
    color: var(--citation-low);
    background: var(--gray-100);
    border-color: var(--gray-300);
  }

  &--citation-medium {
    color: var(--citation-medium);
    background: var(--warning-50);
    border-color: var(--warning-200);
  }

  &--citation-high {
    color: var(--citation-high);
    background: var(--info-50);
    border-color: var(--info-200);
  }

  &--citation-very-high {
    color: var(--citation-very-high);
    background: var(--success-50);
    border-color: var(--success-200);
  }

  &--auto {
    color: var(--secondary-700);
    background: var(--secondary-50);
    border-color: var(--secondary-200);
  }
}

.paper-card__tags {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-2);
}

.paper-card__footer {
  display: flex;
  justify-content: flex-end;
  padding-top: var(--space-4);
  border-top: 1px solid var(--gray-200);
  margin-top: var(--space-2);
}

.paper-card__view-btn {
  display: inline-flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-3);
  border: none;
  background: transparent;
  color: var(--primary-600);
  font-size: var(--font-sm);
  font-weight: var(--font-medium);
  border-radius: var(--radius-md);
  cursor: pointer;
  transition: all var(--duration-fast);

  &:hover {
    background: var(--primary-50);
  }

  &:focus-visible {
    outline: 2px solid var(--primary-500);
    outline-offset: 2px;
  }
}

.paper-card__selection-indicator {
  position: absolute;
  top: var(--space-4);
  right: var(--space-4);
  width: 24px;
  height: 24px;
  background: var(--primary-500);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  color: white;
  animation: scale-in 0.2s ease-out;
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

// 响应式
@media (max-width: 640px) {
  .paper-card {
    padding: var(--space-4);
  }

  .paper-card__title {
    font-size: var(--font-base);
  }

  .paper-card__metadata {
    flex-direction: column;
  }

  .paper-card__footer {
    justify-content: center;
  }

  .paper-card__view-btn {
    width: 100%;
    justify-content: center;
  }
}
</style>
```

---

## 搜索栏组件 (Search Bar)

### 主搜索栏

```vue
<!-- SearchBar.vue -->
<template>
  <div class="search-bar" :class="{ 'search-bar--focused': isFocused }">
    <div class="search-bar__input-wrapper">
      <svg class="search-bar__icon" width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
        <path d="M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" />
      </svg>

      <input
        ref="inputRef"
        v-model="searchQuery"
        type="text"
        class="search-bar__input"
        :placeholder="placeholder"
        @focus="isFocused = true"
        @blur="isFocused = false"
        @input="handleInput"
        @keydown.enter="handleSearch"
        @keydown.escape="handleClear"
        aria-label="搜索论文"
      />

      <button
        v-if="searchQuery"
        class="search-bar__clear"
        @click="handleClear"
        aria-label="清除搜索"
      >
        <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">
          <path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z" />
        </svg>
      </button>
    </div>

    <button
      class="search-bar__button"
      @click="handleSearch"
      :disabled="!searchQuery"
    >
      搜索
    </button>

    <!-- 自动完成建议 -->
    <div
      v-if="showSuggestions && suggestions.length > 0"
      class="search-bar__suggestions"
    >
      <div
        v-for="(suggestion, index) in suggestions"
        :key="index"
        class="search-bar__suggestion"
        :class="{ 'search-bar__suggestion--active': index === activeSuggestion }"
        @click="selectSuggestion(suggestion)"
        @mouseenter="activeSuggestion = index"
      >
        <svg class="search-bar__suggestion-icon" width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
          <path d="M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 3.15.75-1.23-4.5-2.67z" />
        </svg>
        <span class="search-bar__suggestion-text">{{ suggestion.text }}</span>
        <span class="search-bar__suggestion-count">{{ suggestion.count }}</span>
      </div>
    </div>

    <!-- 搜索历史 -->
    <div
      v-if="showHistory && searchHistory.length > 0"
      class="search-bar__history"
    >
      <div class="search-bar__history-header">
        <span>搜索历史</span>
        <button @click="clearHistory" class="search-bar__history-clear">清除</button>
      </div>
      <div
        v-for="(item, index) in searchHistory"
        :key="index"
        class="search-bar__history-item"
        @click="selectHistory(item)"
      >
        <svg class="search-bar__history-icon" width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
          <path d="M13 3c-4.97 0-9 4.03-9 9H1l3.89 3.89.07.14L9 12H6c0-3.87 3.13-7 7-7s7 3.13 7 7-3.13 7-7 7c-1.93 0-3.68-.79-4.94-2.06l-1.42 1.42C8.27 19.99 10.51 21 13 21c4.97 0 9-4.03 9-9s-4.03-9-9-9zm-1 5v5l4.28 2.54.72-1.21-3.5-2.08V8H12z" />
        </svg>
        <span>{{ item }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'

interface Props {
  placeholder?: string
  debounceTime?: number
}

const props = withDefaults(defineProps<Props>(), {
  placeholder: '搜索论文标题、作者、DOI...',
  debounceTime: 300
})

const emit = defineEmits<{
  search: [query: string]
  suggest: [query: string]
}>()

const inputRef = ref<HTMLInputElement>()
const isFocused = ref(false)
const searchQuery = ref('')
const showSuggestions = ref(false)
const showHistory = ref(false)
const activeSuggestion = ref(0)
const suggestions = ref<Array<{ text: string; count: number }>>([])
const searchHistory = ref<string[]>([])
let debounceTimer: number | null = null

const handleInput = () => {
  if (debounceTimer) {
    clearTimeout(debounceTimer)
  }

  debounceTimer = setTimeout(() => {
    if (searchQuery.value.trim()) {
      emit('suggest', searchQuery.value)
      showSuggestions.value = true
      showHistory.value = false
    } else {
      showSuggestions.value = false
      showHistory.value = true
    }
  }, props.debounceTime) as unknown as number
}

const handleSearch = () => {
  if (searchQuery.value.trim()) {
    emit('search', searchQuery.value)
    showSuggestions.value = false
    showHistory.value = false

    // 添加到历史
    if (!searchHistory.value.includes(searchQuery.value)) {
      searchHistory.value.unshift(searchQuery.value)
      if (searchHistory.value.length > 10) {
        searchHistory.value.pop()
      }
    }
  }
}

const handleClear = () => {
  searchQuery.value = ''
  showSuggestions.value = false
  showHistory.value = false
  inputRef.value?.focus()
}

const selectSuggestion = (suggestion: { text: string; count: number }) => {
  searchQuery.value = suggestion.text
  handleSearch()
}

const selectHistory = (item: string) => {
  searchQuery.value = item
  handleSearch()
}

const clearHistory = () => {
  searchHistory.value = []
}

// 键盘导航
const handleKeydown = (e: KeyboardEvent) => {
  if (!showSuggestions.value) return

  if (e.key === 'ArrowDown') {
    e.preventDefault()
    activeSuggestion.value = Math.min(activeSuggestion.value + 1, suggestions.value.length - 1)
  } else if (e.key === 'ArrowUp') {
    e.preventDefault()
    activeSuggestion.value = Math.max(activeSuggestion.value - 1, 0)
  } else if (e.key === 'Enter' && suggestions.value.length > 0) {
    e.preventDefault()
    selectSuggestion(suggestions.value[activeSuggestion.value])
  }
}
</script>

<style scoped lang="scss">
.search-bar {
  position: relative;
  display: flex;
  gap: var(--space-3);
  align-items: center;
  max-width: 600px;
  width: 100%;
}

.search-bar__input-wrapper {
  position: relative;
  flex: 1;
  display: flex;
  align-items: center;
  gap: var(--space-3);
  padding: var(--space-3) var(--space-4);
  background: white;
  border: 2px solid var(--gray-300);
  border-radius: var(--radius-full);
  transition: all var(--duration-base);

  .search-bar--focused & {
    border-color: var(--primary-500);
    box-shadow: 0 0 0 3px rgba(33, 150, 243, 0.1);
  }
}

.search-bar__icon {
  color: var(--gray-500);
  flex-shrink: 0;
}

.search-bar__input {
  flex: 1;
  border: none;
  outline: none;
  font-size: var(--font-base);
  color: var(--gray-900);
  background: transparent;

  &::placeholder {
    color: var(--gray-400);
  }
}

.search-bar__clear {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  border: none;
  background: var(--gray-200);
  border-radius: var(--radius-full);
  color: var(--gray-600);
  cursor: pointer;
  transition: all var(--duration-fast);

  &:hover {
    background: var(--gray-300);
  }
}

.search-bar__button {
  padding: var(--space-3) var(--space-6);
  border: none;
  background: var(--primary-500);
  color: white;
  font-size: var(--font-sm);
  font-weight: var(--font-medium);
  border-radius: var(--radius-full);
  cursor: pointer;
  transition: all var(--duration-fast);
  white-space: nowrap;

  &:hover:not(:disabled) {
    background: var(--primary-600);
  }

  &:disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }
}

.search-bar__suggestions,
.search-bar__history {
  position: absolute;
  top: calc(100% + var(--space-2));
  left: 0;
  right: 0;
  background: white;
  border-radius: var(--radius-lg);
  box-shadow: var(--shadow-lg);
  border: 1px solid var(--gray-200);
  z-index: var(--z-dropdown);
  max-height: 300px;
  overflow-y: auto;
}

.search-bar__suggestion,
.search-bar__history-item {
  display: flex;
  align-items: center;
  gap: var(--space-3);
  padding: var(--space-3) var(--space-4);
  cursor: pointer;
  transition: background var(--duration-fast);

  &:hover,
  &--active {
    background: var(--gray-50);
  }
}

.search-bar__suggestion-text {
  flex: 1;
  font-size: var(--font-sm);
  color: var(--gray-900);
}

.search-bar__suggestion-count {
  font-size: var(--font-xs);
  color: var(--gray-500);
}

.search-bar__history-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: var(--space-2) var(--space-4);
  border-bottom: 1px solid var(--gray-200);
  font-size: var(--font-xs);
  color: var(--gray-600);
}

.search-bar__history-clear {
  border: none;
  background: transparent;
  color: var(--primary-600);
  cursor: pointer;
  font-size: var(--font-xs);

  &:hover {
    text-decoration: underline;
  }
}

// 响应式
@media (max-width: 640px) {
  .search-bar {
    max-width: 100%;
  }

  .search-bar__button {
    display: none;
  }
}
</style>
```

---

## 数据表格组件 (Data Table)

```vue
<!-- DataTable.vue -->
<template>
  <div class="data-table">
    <!-- 表格工具栏 -->
    <div class="data-table__toolbar">
      <div class="data-table__filters">
        <slot name="filters" />
      </div>

      <div class="data-table__actions">
        <slot name="actions" />
      </div>
    </div>

    <!-- 表格容器 -->
    <div class="data-table__container">
      <table class="data-table__table">
        <thead class="data-table__thead">
          <tr>
            <th v-if="selectable" class="data-table__checkbox">
              <input
                type="checkbox"
                :checked="allSelected"
                @change="toggleAll"
                aria-label="选择全部"
              />
            </th>
            <th
              v-for="column in columns"
              :key="column.key"
              class="data-table__th"
              :class="{
                'data-table__th--sortable': column.sortable,
                'data-table__th--sorted': sortKey === column.key
              }"
              :style="{ width: column.width }"
              @click="column.sortable && sort(column.key)"
            >
              <div class="data-table__th-content">
                <span>{{ column.label }}</span>
                <svg
                  v-if="column.sortable"
                  class="data-table__sort-icon"
                  width="16"
                  height="16"
                  viewBox="0 0 24 24"
                  fill="currentColor"
                >
                  <path
                    :d="sortKey === column.key
                      ? (sortOrder === 'asc'
                        ? 'M3 18h6v-2H3v2zM3 6v2h18V6H3zm0 7h12v-2H3v2z'
                        : 'M3 18h6v-2H3v2zM3 6v2h18V6H3zm0 7h12v-2H3v2z')
                      : 'M3 18h6v-2H3v2zM3 6v2h18V6H3zm0 7h12v-2H3v2z'"
                  />
                </svg>
              </div>
            </th>
          </tr>
        </thead>

        <tbody class="data-table__tbody">
          <tr
            v-for="(row, index) in sortedData"
            :key="getRowKey(row, index)"
            class="data-table__row"
            :class="{
              'data-table__row--selected': isSelected(row),
              'data-table__row--hovered': hoveredRow === index
            }"
            @click="handleRowClick(row)"
            @mouseenter="hoveredRow = index"
            @mouseleave="hoveredRow = -1"
          >
            <td v-if="selectable" class="data-table__checkbox">
              <input
                type="checkbox"
                :checked="isSelected(row)"
                @change="toggleRow(row)"
                @click.stop
              />
            </td>
            <td
              v-for="column in columns"
              :key="column.key"
              class="data-table__td"
            >
              <slot
                v-if="$slots[`cell-${column.key}`]"
                :name="`cell-${column.key}`"
                :row="row"
                :value="row[column.key]"
              />
              <span v-else>{{ row[column.key] }}</span>
            </td>
          </tr>
        </tbody>
      </table>

      <!-- 空状态 -->
      <div v-if="sortedData.length === 0" class="data-table__empty">
        <slot name="empty">
          <div class="data-table__empty-content">
            <svg width="64" height="64" viewBox="0 0 24 24" fill="currentColor">
              <path d="M19 5v14H5V5h14m0-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm-4.86 8.86l-3 3.87L9 13.14 6 17h12l-3.86-5.14z" />
            </svg>
            <p>暂无数据</p>
          </div>
        </slot>
      </div>
    </div>

    <!-- 分页 -->
    <div v-if="pagination" class="data-table__pagination">
      <div class="data-table__pagination-info">
        显示 {{ paginationStart }}-{{ paginationEnd }} / 共 {{ totalItems }} 项
      </div>

      <div class="data-table__pagination-controls">
        <button
          class="data-table__pagination-btn"
          :disabled="currentPage === 1"
          @click="changePage(currentPage - 1)"
        >
          上一页
        </button>

        <div class="data-table__pagination-pages">
          <button
            v-for="page in visiblePages"
            :key="page"
            class="data-table__pagination-page"
            :class="{ 'data-table__pagination-page--active': page === currentPage }"
            @click="changePage(page)"
          >
            {{ page }}
          </button>
        </div>

        <button
          class="data-table__pagination-btn"
          :disabled="currentPage === totalPages"
          @click="changePage(currentPage + 1)"
        >
          下一页
        </button>
      </div>

      <select
        v-model="pageSize"
        class="data-table__pagination-size"
        @change="handlePageSizeChange"
      >
        <option :value="10">10 条/页</option>
        <option :value="20">20 条/页</option>
        <option :value="50">50 条/页</option>
        <option :value="100">100 条/页</option>
      </select>
    </div>
  </div>
</template>

<script setup lang="ts" generic="T extends Record<string, any">
import { ref, computed } from 'vue'

interface Column {
  key: string
  label: string
  width?: string
  sortable?: boolean
}

interface Props {
  columns: Column[]
  data: T[]
  rowKey?: keyof T
  selectable?: boolean
  pagination?: boolean
  itemsPerPage?: number
}

const props = withDefaults(defineProps<Props>(), {
  rowKey: 'id',
  selectable: false,
  pagination: false,
  itemsPerPage: 20
})

const emit = defineEmits<{
  rowClick: [row: T]
  sort: [key: string, order: 'asc' | 'desc']
}>()

const selectedRows = ref<Set<T[keyof T]>>(new Set())
const hoveredRow = ref(-1)
const sortKey = ref<string>('')
const sortOrder = ref<'asc' | 'desc'>('asc')
const currentPage = ref(1)
const pageSize = ref(props.itemsPerPage)

const sortedData = computed(() => {
  if (!sortKey.value) return props.data

  return [...props.data].sort((a, b) => {
    const aVal = a[sortKey.value]
    const bVal = b[sortKey.value]

    if (aVal < bVal) return sortOrder.value === 'asc' ? -1 : 1
    if (aVal > bVal) return sortOrder.value === 'asc' ? 1 : -1
    return 0
  })
})

const paginatedData = computed(() => {
  if (!props.pagination) return sortedData.value

  const start = (currentPage.value - 1) * pageSize.value
  const end = start + pageSize.value
  return sortedData.value.slice(start, end)
})

const totalPages = computed(() => {
  return Math.ceil(props.data.length / pageSize.value)
})

const totalItems = computed(() => props.data.length)

const paginationStart = computed(() => {
  return (currentPage.value - 1) * pageSize.value + 1
})

const paginationEnd = computed(() => {
  return Math.min(currentPage.value * pageSize.value, totalItems.value)
})

const visiblePages = computed(() => {
  const pages: number[] = []
  const maxVisible = 5

  if (totalPages.value <= maxVisible) {
    for (let i = 1; i <= totalPages.value; i++) {
      pages.push(i)
    }
  } else {
    let start = Math.max(1, currentPage.value - 2)
    let end = Math.min(totalPages.value, start + maxVisible - 1)

    if (end - start < maxVisible - 1) {
      start = Math.max(1, end - maxVisible + 1)
    }

    for (let i = start; i <= end; i++) {
      pages.push(i)
    }
  }

  return pages
})

const allSelected = computed(() => {
  return paginatedData.value.length > 0 &&
    paginatedData.value.every(row => isSelected(row))
})

const getRowKey = (row: T, index: number) => {
  return row[props.rowKey] ?? index
}

const isSelected = (row: T) => {
  return selectedRows.value.has(row[props.rowKey])
}

const toggleRow = (row: T) => {
  const key = row[props.rowKey]
  if (selectedRows.value.has(key)) {
    selectedRows.value.delete(key)
  } else {
    selectedRows.value.add(key)
  }
}

const toggleAll = () => {
  if (allSelected.value) {
    paginatedData.value.forEach(row => {
      selectedRows.value.delete(row[props.rowKey])
    })
  } else {
    paginatedData.value.forEach(row => {
      selectedRows.value.add(row[props.rowKey])
    })
  }
}

const sort = (key: string) => {
  if (sortKey.value === key) {
    sortOrder.value = sortOrder.value === 'asc' ? 'desc' : 'asc'
  } else {
    sortKey.value = key
    sortOrder.value = 'asc'
  }
  emit('sort', key, sortOrder.value)
}

const handleRowClick = (row: T) => {
  emit('rowClick', row)
}

const changePage = (page: number) => {
  if (page >= 1 && page <= totalPages.value) {
    currentPage.value = page
  }
}

const handlePageSizeChange = () => {
  currentPage.value = 1
}
</script>

<style scoped lang="scss">
.data-table {
  display: flex;
  flex-direction: column;
  gap: var(--space-4);
}

.data-table__toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: var(--space-4);
}

.data-table__filters {
  display: flex;
  gap: var(--space-3);
  flex: 1;
}

.data-table__actions {
  display: flex;
  gap: var(--space-3);
}

.data-table__container {
  background: white;
  border-radius: var(--radius-lg);
  box-shadow: var(--shadow-sm);
  border: 1px solid var(--gray-200);
  overflow: hidden;
}

.data-table__table {
  width: 100%;
  border-collapse: collapse;
}

.data-table__thead {
  background: var(--gray-50);
  border-bottom: 2px solid var(--gray-200);
}

.data-table__th {
  padding: var(--space-4);
  text-align: left;
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  color: var(--gray-700);
  user-select: none;

  &--sortable {
    cursor: pointer;
    transition: background var(--duration-fast);

    &:hover {
      background: var(--gray-100);
    }
  }

  &--sorted {
    color: var(--primary-600);
  }
}

.data-table__th-content {
  display: flex;
  align-items: center;
  gap: var(--space-2);
}

.data-table__sort-icon {
  color: var(--gray-400);
  flex-shrink: 0;
}

.data-table__tbody {
  background: white;
}

.data-table__row {
  border-bottom: 1px solid var(--gray-200);
  transition: background var(--duration-fast);

  &:last-child {
    border-bottom: none;
  }

  &--hovered {
    background: var(--gray-50);
  }

  &--selected {
    background: var(--primary-50);
  }
}

.data-table__td {
  padding: var(--space-4);
  font-size: var(--font-sm);
  color: var(--gray-900);
}

.data-table__checkbox {
  width: 48px;
  padding: var(--space-4);
  text-align: center;
}

.data-table__empty {
  display: flex;
  justify-content: center;
  align-items: center;
  padding: var(--space-16);
}

.data-table__empty-content {
  text-align: center;
  color: var(--gray-500);

  svg {
    margin-bottom: var(--space-4);
  }
}

.data-table__pagination {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: var(--space-4);
  padding: var(--space-4);
  background: white;
  border-radius: var(--radius-lg);
  border: 1px solid var(--gray-200);
}

.data-table__pagination-info {
  font-size: var(--font-sm);
  color: var(--gray-600);
}

.data-table__pagination-controls {
  display: flex;
  gap: var(--space-2);
  align-items: center;
}

.data-table__pagination-btn {
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--gray-300);
  background: white;
  border-radius: var(--radius-md);
  font-size: var(--font-sm);
  cursor: pointer;
  transition: all var(--duration-fast);

  &:hover:not(:disabled) {
    background: var(--gray-50);
    border-color: var(--gray-400);
  }

  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }
}

.data-table__pagination-pages {
  display: flex;
  gap: var(--space-1);
}

.data-table__pagination-page {
  width: 36px;
  height: 36px;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 1px solid var(--gray-300);
  background: white;
  border-radius: var(--radius-md);
  font-size: var(--font-sm);
  cursor: pointer;
  transition: all var(--duration-fast);

  &:hover {
    background: var(--gray-50);
  }

  &--active {
    background: var(--primary-500);
    color: white;
    border-color: var(--primary-500);
  }
}

.data-table__pagination-size {
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--gray-300);
  background: white;
  border-radius: var(--radius-md);
  font-size: var(--font-sm);
  cursor: pointer;
}

// 响应式
@media (max-width: 768px) {
  .data-table__toolbar {
    flex-direction: column;
    align-items: stretch;
  }

  .data-table__pagination {
    flex-direction: column;
    gap: var(--space-3);
  }

  .data-table__container {
    overflow-x: auto;
  }
}
</style>
```

---

## 最佳实践

### 1. 组件命名规范

**文件命名**:
- 使用 PascalCase: `PaperCard.vue`
- 多个单词组合: `SearchBar.vue`, `DataTable.vue`
- 功能性后缀: `Button.vue`, `IconButton.vue`, `ButtonGroup.vue`

**组件内部命名**:
- 使用 BEM 命名法: `.paper-card`, `.paper-card__header`, `.paper-card--selected`
- CSS 变量使用 kebab-case: `--primary-500`, `--font-size-base`
- JavaScript 使用 camelCase: `searchQuery`, `handleClick`

### 2. Props 设计原则

**必需 vs 可选**:
```typescript
// ✅ 好的设计
interface Props {
  paper: Paper           // 必需
  interactive?: boolean  // 可选，有默认值
  size?: 'sm' | 'md' | 'lg'  // 有限的选项
}

// ❌ 不好的设计
interface Props {
  paper?: Paper          // 必需数据不应是可选的
  width?: number         // 过于自由，应使用预设尺寸
}
```

**默认值设置**:
```typescript
const props = withDefaults(defineProps<Props>(), {
  interactive: true,
  size: 'md',
  disabled: false
})
```

### 3. 事件设计

**事件命名**:
- 使用动词: `click`, `submit`, `search`
- 描述性命名: `favorite-toggled`, `data-loaded`
- 遵循 Vue 3 风格: `update:modelValue`

**事件载荷**:
```typescript
// ✅ 明确的载荷类型
const emit = defineEmits<{
  click: [event: MouseEvent]
  search: [query: string, filters: SearchFilters]
  rowClick: [row: Paper]
}>()
```

### 4. 插槽设计

**命名插槽**:
```vue
<template>
  <div class="component">
    <slot name="header" />
    <slot />  <!-- 默认插槽 -->
    <slot name="footer" />
  </div>
</template>
```

**作用域插槽**:
```vue
<slot name="cell" :row="row" :value="row[column.key]" />
```

### 5. 性能优化

**计算属性 vs 方法**:
```typescript
// ✅ 使用计算属性（有缓存）
const truncatedTitle = computed(() => {
  return paper.title.length > 100
    ? paper.title.substring(0, 100) + '...'
    : paper.title
})

// ❌ 使用方法（每次调用都重新计算）
const truncateTitle = (title: string) => {
  return title.length > 100
    ? title.substring(0, 100) + '...'
    : title
}
```

**列表渲染优化**:
```vue
<!-- ✅ 使用 key -->
<div v-for="item in items" :key="item.id">

<!-- ❌ 不使用 key -->
<div v-for="item in items">
```

**虚拟滚动**:
```vue
<!-- 对于超长列表，使用虚拟滚动 -->
<RecycleScroller
  :items="items"
  :item-size="100"
  key-field="id"
>
  <template #default="{ item }">
    <PaperCard :paper="item" />
  </template>
</RecycleScroller>
```

### 6. 可访问性实践

**语义化 HTML**:
```vue
<!-- ✅ 语义化 -->
<header>...</header>
<main>...</main>
<nav aria-label="主导航">...</nav>
<article>...</article>

<!-- ❌ 全是 div -->
<div class="header">...</div>
<div class="main">...</div>
```

**ARIA 属性**:
```vue
<button
  aria-label="关闭对话框"
  @click="close"
>
  <span aria-hidden="true">&times;</span>
</button>

<div
  role="status"
  aria-live="polite"
  aria-atomic="true"
>
  {{ statusMessage }}
</div>
```

**键盘导航**:
```typescript
const handleKeydown = (e: KeyboardEvent) => {
  switch (e.key) {
    case 'Enter':
      handleSelect()
      break
    case 'Escape':
      handleCancel()
      break
    case 'ArrowDown':
      handleNext()
      break
    case 'ArrowUp':
      handlePrevious()
      break
  }
}
```

### 7. 样式组织

**CSS 变量优先**:
```css
/* ✅ 使用设计 token */
.paper-card {
  padding: var(--space-5);
  border-radius: var(--radius-lg);
  color: var(--gray-900);
}

/* ❌ 硬编码值 */
.paper-card {
  padding: 20px;
  border-radius: 8px;
  color: #212121;
}
```

**作用域样式**:
```vue
<style scoped lang="scss">
/* 组件内部样式，不会污染全局 */
</style>
```

### 8. 测试策略

**单元测试**:
```typescript
import { mount } from '@vue/test-utils'
import { describe, it, expect } from 'vitest'
import PaperCard from './PaperCard.vue'

describe('PaperCard', () => {
  it('显示论文标题', () => {
    const wrapper = mount(PaperCard, {
      props: {
        paper: {
          id: '1',
          title: 'Test Paper',
          authors: 'Test Author',
          year: 2024,
          venue: 'CVPR',
          citation_count: 100
        }
      }
    })

    expect(wrapper.text()).toContain('Test Paper')
  })

  it('点击时触发 click 事件', async () => {
    const wrapper = mount(PaperCard, {
      props: {
        paper: { /* ... */ },
        interactive: true
      }
    })

    await wrapper.trigger('click')
    expect(wrapper.emitted('click')).toBeTruthy()
  })
})
```

### 9. 文档编写

**组件文档模板**:
```markdown
# PaperCard

论文卡片组件，用于展示单篇论文的详细信息。

## 基础用法

\`\`\`vue
<PaperCard
  :paper="paper"
  :interactive="true"
  @click="handleClick"
/>
\`\`\`

## Props

| 参数 | 说明 | 类型 | 默认值 |
|------|------|------|--------|
| paper | 论文数据对象 | `Paper` | - |
| interactive | 是否可点击 | `boolean` | `true` |
| highlightKeyword | 高亮关键词 | `string` | `''` |

## Events

| 事件名 | 说明 | 参数 |
|--------|------|------|
| click | 卡片被点击 | `(paper: Paper) => void` |
| favorite | 收藏按钮被点击 | `(paper: Paper) => void` |

## 示例

### 基础示例
### 带高亮的示例
### 选中状态示例
```

### 10. 错误处理

**Prop 验证**:
```typescript
const props = defineProps<{
  paper: Paper
  size?: 'sm' | 'md' | 'lg'
}>()

// 运行时验证
watch(() => props.paper, (newPaper) => {
  if (!newPaper.id) {
    console.error('Paper must have an id')
  }
}, { immediate: true })
```

**错误边界**:
```vue
<template>
  <div v-if="error" class="error-boundary">
    <p>组件加载失败</p>
    <button @click="retry">重试</button>
  </div>
  <div v-else>
    <!-- 正常内容 -->
  </div>
</template>

<script setup lang="ts">
import { ref, onErrorCaptured } from 'vue'

const error = ref(null)

onErrorCaptured((err) => {
  error.value = err
  return false  // 阻止错误继续向上传播
})
</script>
```

---

**文档结束**

本文档提供了 PaperCrawler 项目中核心组件的详细实现示例和最佳实践指南。所有代码示例均遵循 [UI-DESIGN-SYSTEM.md](./UI-DESIGN-SYSTEM.md) 中定义的设计规范。
