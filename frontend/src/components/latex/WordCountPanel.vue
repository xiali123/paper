<template>
  <div class="word-count-panel">
    <div class="panel-header">
      <h4>Word Count</h4>
      <button class="settings-toggle" @click="showSettings = !showSettings" title="Toggle target settings">
        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <circle cx="12" cy="12" r="3" />
          <path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42" />
        </svg>
      </button>
    </div>

    <!-- Live stats overview -->
    <div class="stats-overview">
      <div class="stat-row">
        <span class="stat-label">Words</span>
        <span class="stat-value">{{ totalWords.toLocaleString() }}</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Characters</span>
        <span class="stat-value">{{ totalChars.toLocaleString() }}</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Chars (no spaces)</span>
        <span class="stat-value">{{ charsNoSpaces.toLocaleString() }}</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Reading time</span>
        <span class="stat-value">{{ readingTime }}</span>
      </div>
    </div>

    <!-- Target word count with progress -->
    <div v-if="showSettings || wordTarget > 0" class="target-section">
      <div class="target-header">
        <span class="stat-label">Target</span>
        <input v-if="showSettings" type="number" class="target-input"
          :value="wordTarget" @input="onTargetInput($event)" placeholder="e.g. 5000" min="0" />
        <span v-else class="stat-value">{{ wordTarget.toLocaleString() }}</span>
      </div>
      <div v-if="wordTarget > 0" class="progress-wrapper">
        <div class="progress-bar">
          <div class="progress-fill" :class="progressClass" :style="{ width: progressPercent + '%' }" />
        </div>
        <span class="progress-text">{{ progressPercent }}%</span>
      </div>
    </div>

    <!-- Section breakdown -->
    <div class="section-breakdown">
      <div class="breakdown-header"><span class="stat-label">Section Breakdown</span></div>
      <div class="section-list">
        <div v-for="sec in sections" :key="sec.name" class="section-item">
          <div class="section-info">
            <span class="section-name">{{ sec.name }}</span>
            <span class="section-words">{{ sec.words.toLocaleString() }}</span>
          </div>
          <div class="section-bar-track">
            <div class="section-bar-fill" :style="{ width: sectionBarWidth(sec.words) + '%' }" />
          </div>
        </div>
        <div v-if="sections.length === 0" class="empty-state">No sections detected</div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'

interface SectionCount { name: string; words: number }

const props = defineProps<{ content: string }>()

const showSettings = ref(false)
const wordTarget = ref(0)

/** Strip LaTeX noise from text for word counting. */
function stripLatex(text: string): string {
  return text
    .replace(/\$\$[^$]+\$\$/g, ' ')
    .replace(/\$[^$]+\$/g, ' ')
    .replace(/\\\[.*?\\\]/gs, ' ')
    .replace(/\\\(.*?\\\)/gs, ' ')
    .replace(/\\[a-zA-Z]+(\{[^}]*\})*/g, ' ')
    .replace(/%[^\n]*/g, '')
    .replace(/[{}]/g, ' ')
    .replace(/\s+/g, ' ')
    .trim()
}

function countWords(text: string): number {
  const cleaned = stripLatex(text)
  return cleaned ? cleaned.split(/\s+/).length : 0
}

const sections = computed<SectionCount[]>(() => {
  const src = props.content || ''
  if (!src.trim()) return []

  const regex = /\\(section|subsection)\*?\s*\{([^}]*)\}/g
  const result: SectionCount[] = []
  const boundaries: { name: string; idx: number }[] = []
  let m: RegExpExecArray | null

  while ((m = regex.exec(src)) !== null) {
    const prefix = m[1] === 'subsection' ? '  ' : ''
    boundaries.push({ name: prefix + (m[2] || m[1]), idx: m.index + m[0].length })
  }

  if (boundaries.length === 0) {
    const w = countWords(src)
    return w > 0 ? [{ name: 'Document', words: w }] : []
  }

  // Preamble before first section
  const preamble = src.substring(0, boundaries[0].idx - boundaries[0].name.length)
  const preambleW = countWords(preamble)
  if (preambleW > 0) result.push({ name: 'Preamble', words: preambleW })

  for (let i = 0; i < boundaries.length; i++) {
    const start = boundaries[i].idx
    const end = i + 1 < boundaries.length
      ? boundaries[i + 1].idx - boundaries[i + 1].name.length
      : src.length
    result.push({ name: boundaries[i].name, words: countWords(src.substring(start, Math.max(start, end))) })
  }
  return result
})

const totalWords = computed(() => countWords(props.content || ''))
const totalChars = computed(() => (props.content || '').length)
const charsNoSpaces = computed(() => (props.content || '').replace(/\s/g, '').length)
const readingTime = computed(() => {
  const mins = Math.max(1, Math.round(totalWords.value / 200))
  if (mins < 60) return `~${mins} min`
  const h = Math.floor(mins / 60), m = mins % 60
  return m > 0 ? `~${h}h ${m}m` : `~${h}h`
})
const progressPercent = computed(() =>
  wordTarget.value <= 0 ? 0 : Math.min(Math.round((totalWords.value / wordTarget.value) * 100), 100))
const progressClass = computed(() => {
  const p = progressPercent.value
  if (p >= 100) return 'status-complete'
  if (p >= 75) return 'status-on-track'
  if (p >= 40) return 'status-behind'
  return 'status-low'
})

function sectionBarWidth(words: number): number {
  return Math.round((words / Math.max(...sections.value.map(s => s.words), 1)) * 100)
}
function onTargetInput(e: Event) {
  const v = parseInt((e.target as HTMLInputElement).value, 10)
  wordTarget.value = isNaN(v) || v < 0 ? 0 : v
}
</script>

<style scoped lang="scss">
@use '@/styles/variables' as *;

$br: var(--el-border-color-light, #{$border-light});
$br2: var(--el-border-color-lighter, #{$border-lighter});
$text: var(--el-text-color-primary, #{$gray-900});
$text2: var(--el-text-color-secondary, #{$gray-500});

.word-count-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
  font-family: $font-family-base;
  font-size: $font-size-sm;
  color: $text;
  background: var(--el-bg-color, #{$bg-color});
  border-left: 1px solid $br;

  .panel-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: $spacing-3 $spacing-4;
    border-bottom: 1px solid $br;
    h4 { margin: 0; font-size: $font-size-sm; font-weight: $font-weight-semibold; }
    .settings-toggle {
      display: flex; align-items: center; justify-content: center;
      width: 24px; height: 24px; border: none; background: transparent;
      color: $text2; cursor: pointer; border-radius: $border-radius-base;
      transition: all $duration-fast ease;
      &:hover { background: var(--el-fill-color, #{$gray-100}); color: var(--el-color-primary); }
    }
  }

  .stat-label { color: $text2; font-size: $font-size-xs; }
  .stat-value {
    font-weight: $font-weight-semibold; font-size: $font-size-sm;
    color: $text; font-variant-numeric: tabular-nums;
  }

  .stats-overview {
    padding: $spacing-3 $spacing-4;
    border-bottom: 1px solid $br2;
    .stat-row { display: flex; justify-content: space-between; align-items: center; padding: $spacing-1 0; }
  }

  .target-section {
    padding: $spacing-3 $spacing-4;
    border-bottom: 1px solid $br2;
    .target-header { display: flex; justify-content: space-between; align-items: center; }
    .target-input {
      width: 100px; padding: $spacing-1 $spacing-2; font-size: $font-size-xs;
      font-weight: $font-weight-semibold; text-align: right;
      border: 1px solid var(--el-border-color, #{$border-base}); border-radius: $border-radius-base;
      background: var(--el-bg-color, #{$bg-color}); color: $text;
      font-variant-numeric: tabular-nums; outline: none;
      transition: border-color $duration-fast ease;
      -moz-appearance: textfield;
      &::-webkit-outer-spin-button, &::-webkit-inner-spin-button { -webkit-appearance: none; margin: 0; }
      &:focus { border-color: var(--el-color-primary); }
    }
    .progress-wrapper {
      display: flex; align-items: center; gap: $spacing-2; margin-top: $spacing-2;
      .progress-bar {
        flex: 1; height: 6px; background: var(--el-fill-color, #{$gray-100});
        border-radius: $border-radius-full; overflow: hidden;
      }
      .progress-fill {
        height: 100%; border-radius: $border-radius-full;
        transition: width $duration-slow ease, background $duration-base ease;
        &.status-low { background: var(--el-color-danger, #{$danger-color}); }
        &.status-behind { background: var(--el-color-warning, #{$warning-color}); }
        &.status-on-track { background: var(--el-color-primary, #{$primary-500}); }
        &.status-complete { background: var(--el-color-success, #{$success-color}); }
      }
      .progress-text {
        font-size: 11px; font-weight: $font-weight-medium; color: $text2;
        min-width: 32px; text-align: right; font-variant-numeric: tabular-nums;
      }
    }
  }

  .section-breakdown {
    flex: 1; overflow-y: auto; padding: $spacing-3 $spacing-4;
    .breakdown-header { margin-bottom: $spacing-2; }
    .section-list { display: flex; flex-direction: column; gap: $spacing-2; }
    .section-item {
      .section-info { display: flex; justify-content: space-between; align-items: center; margin-bottom: 2px; }
      .section-name {
        font-size: $font-size-xs; color: var(--el-text-color-regular, #{$gray-700});
        overflow: hidden; text-overflow: ellipsis; white-space: nowrap; max-width: 70%;
      }
      .section-words {
        font-size: 11px; font-weight: $font-weight-semibold; color: $text; font-variant-numeric: tabular-nums;
      }
      .section-bar-track {
        height: 3px; background: var(--el-fill-color-lighter, #{$gray-100});
        border-radius: $border-radius-full; overflow: hidden;
      }
      .section-bar-fill {
        height: 100%; background: var(--el-color-primary-light-3, #{$primary-300});
        border-radius: $border-radius-full; transition: width $duration-slow ease; min-width: 2px;
      }
    }
    .empty-state {
      padding: $spacing-6 0; text-align: center; font-size: $font-size-xs;
      color: var(--el-text-color-placeholder, #{$gray-400}); font-style: italic;
    }
  }
}

[data-theme="dark"] .word-count-panel {
  .section-bar-track { background: rgba(255, 255, 255, 0.06); }
  .target-input { background: rgba(255, 255, 255, 0.05); border-color: rgba(255, 255, 255, 0.15); color: #e5e7eb; }
  .progress-bar { background: rgba(255, 255, 255, 0.08); }
}
</style>
