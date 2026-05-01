<template>
  <article
    class="paper-card"
    :class="cardClasses"
    @click="handleClick"
    @mouseenter="isHovered = true"
    @mouseleave="isHovered = false"
  >
    <!-- Top gradient border -->
    <div class="paper-card__border" />

    <!-- Card header -->
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

        <!-- CCF level badge -->
        <span
          v-if="paper.ccf_level"
          class="paper-card__badge"
          :class="`paper-card__badge--${paper.ccf_level.toLowerCase()}`"
        >
          CCF {{ paper.ccf_level }}
        </span>
      </div>

      <!-- Action buttons -->
      <div class="paper-card__actions">
        <button
          class="paper-card__action"
          :class="{ 'paper-card__action--active': isFavorite }"
          @click.stop="toggleFavorite"
          :aria-label="isFavorite ? 'Remove from favorites' : 'Add to favorites'"
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

        <!-- Dropdown menu -->
        <div v-if="showMenu" class="paper-card__menu">
          <button @click.stop="copyCitation">Copy Citation</button>
          <button @click.stop="exportBibTeX">Export BibTeX</button>
          <button @click.stop="viewDetails">View Details</button>
        </div>
      </div>
    </header>

    <!-- Card body -->
    <section class="paper-card__body">
      <!-- Author information -->
      <div class="paper-card__authors">
        <svg class="paper-card__icon" width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
          <path d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z" />
        </svg>
        <span class="paper-card__authors-text">{{ truncatedAuthors }}</span>
      </div>

      <!-- Metadata tags -->
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
          {{ paper.citation_count }} citations
        </span>
      </div>

      <!-- Auto-generated tags -->
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

    <!-- Card footer -->
    <footer v-if="interactive" class="paper-card__footer">
      <button
        class="paper-card__view-btn"
        @click.stop="viewDetails"
      >
        View Details
        <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
          <path d="M12 4l-1.41 1.41L16.17 11H4v2h12.17l-5.58 5.59L12 20l8-8z" />
        </svg>
      </button>
    </footer>

    <!-- Selection indicator -->
    <div v-if="isSelected" class="paper-card__selection-indicator">
      <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
        <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z" />
      </svg>
    </div>
  </article>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import DOMPurify from 'dompurify'

/**
 * Paper interface representing academic paper data
 */
interface Paper {
  id: string
  title: string
  authors: string
  year: number
  venue: string
  citation_count: number
  ccf_level?: 'A' | 'B' | 'C'
  abstract?: string
}

/**
 * Component props for PaperCard
 */
interface Props {
  /** Paper data object */
  paper: Paper
  /** Whether the card is interactive/clickable */
  interactive?: boolean
  /** Keyword to highlight in title */
  highlightKeyword?: string
  /** Whether the card is selected */
  isSelected?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  interactive: true,
  highlightKeyword: '',
  isSelected: false
})

/**
 * Component events
 */
const emit = defineEmits<{
  /** Fired when card is clicked */
  click: [paper: Paper]
  /** Fired when favorite button is clicked */
  favorite: [paper: Paper, isFavorite: boolean]
  /** Fired when view details is clicked */
  view: [paper: Paper]
}>()

const isHovered = ref(false)
const isFavorite = ref(false)
const showMenu = ref(false)

/**
 * Computed classes for card styling
 */
const cardClasses = computed(() => ({
  'paper-card--hovered': isHovered.value,
  'paper-card--selected': props.isSelected,
  'paper-card--interactive': props.interactive
}))

/**
 * Truncate authors text if too long
 */
const truncatedAuthors = computed(() => {
  const maxLength = 60
  return props.paper.authors.length > maxLength
    ? props.paper.authors.substring(0, maxLength) + '...'
    : props.paper.authors
})

/**
 * Truncate venue text if too long
 */
const truncatedVenue = computed(() => {
  const maxLength = 30
  return props.paper.venue.length > maxLength
    ? props.paper.venue.substring(0, maxLength) + '...'
    : props.paper.venue
})

/**
 * Highlight keyword in title
 */
const highlightedTitle = computed(() => {
  if (!props.highlightKeyword) return props.paper.title

  const escaped = props.highlightKeyword.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
  const regex = new RegExp(`(${escaped})`, 'gi')
  const highlighted = props.paper.title.replace(regex, '<mark class="paper-card__highlight">$1</mark>')
  return DOMPurify.sanitize(highlighted, { ALLOWED_TAGS: ['mark'] })
})

/**
 * Determine citation level for coloring
 */
const citationLevel = computed(() => {
  const count = props.paper.citation_count
  if (count >= 100) return 'very-high'
  if (count >= 50) return 'high'
  if (count >= 10) return 'medium'
  return 'low'
})

/**
 * Auto-generate tags from title
 */
const tags = computed(() => {
  const title = props.paper.title.toLowerCase()
  const tags: string[] = []

  if (title.includes('testing')) tags.push('Testing')
  if (title.includes('machine learning')) tags.push('ML')
  if (title.includes('deep learning')) tags.push('Deep Learning')
  if (title.includes('software')) tags.push('Software Engineering')

  return tags.slice(0, 3)
})

/**
 * Handle card click
 */
const handleClick = () => {
  if (!props.interactive) return
  emit('click', props.paper)
}

/**
 * Toggle favorite status
 */
const toggleFavorite = () => {
  isFavorite.value = !isFavorite.value
  emit('favorite', props.paper, isFavorite.value)
}

/**
 * View paper details
 */
const viewDetails = () => {
  emit('view', props.paper)
}

/**
 * Copy citation to clipboard
 */
const copyCitation = () => {
  const citation = `${props.paper.authors} (${props.paper.year}). ${props.paper.title}. ${props.paper.venue}.`
  navigator.clipboard.writeText(citation)
  showMenu.value = false
}

/**
 * Export BibTeX to clipboard
 */
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

// Responsive
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
