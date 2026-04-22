# LaTeX Editor UI Implementation Guide

## Quick Start

### 1. Import the Optimized Styles

In your main LaTeX editor components, import the new optimized styles:

```vue
<style scoped lang="scss">
@import '@/styles/latex-editor-design-tokens.scss';
@import '@/styles/latex-editor-optimized.scss';

// Your component-specific styles here
</style>
```

### 2. Apply Design Tokens

Replace hardcoded values with design tokens:

```scss
// BEFORE
.toolbar {
  padding: 8px 16px;
  gap: 12px;
}

// AFTER
.toolbar {
  padding: $editor-padding-sm $editor-padding-lg;
  gap: $editor-gap-md;
}
```

---

## Component-Specific Improvements

### 1. LatexEditor Component Enhancement

```vue
<template>
  <div class="latex-editor" :class="editorClasses">
    <!-- Syntax highlighting layer -->
    <pre
      class="latex-highlight"
      v-if="showHighlight && !isFocused"
      aria-hidden="true"
    >
      <code v-html="highlightedCode"></code>
    </pre>

    <!-- Main textarea with accessibility improvements -->
    <textarea
      ref="textareaRef"
      v-model="innerContent"
      class="latex-textarea"
      :class="textareaClasses"
      spellcheck="false"
      :aria-label="editorLabel"
      :aria-describedby="editorDescription"
      @focus="handleFocus"
      @blur="handleBlur"
      @input="handleInput"
      @scroll="handleScroll"
    ></textarea>

    <!-- Enhanced toolbar with proper grouping -->
    <div class="latex-toolbar" v-if="showToolbar">
      <div class="toolbar-group toolbar-group--primary">
        <el-tooltip content="行内公式 $...$" placement="top">
          <el-button
            size="small"
            @click="insert('$', '$')"
            :aria-label="插入行内公式"
          >
            $
          </el-button>
        </el-tooltip>
        <el-tooltip content="块级公式 $$...$$" placement="top">
          <el-button
            size="small"
            @click="insert('$$\n', '\n$$')"
            :aria-label="插入块级公式"
          >
            $$
          </el-button>
        </el-tooltip>
      </div>

      <div class="toolbar-divider"></div>

      <div class="toolbar-group toolbar-group--format">
        <el-tooltip content="粗体 \\textbf{}" placement="top">
          <el-button
            size="small"
            @click="insert('\\textbf{', '}')"
            :aria-label="插入粗体文本"
          >
            <span class="format-text">B</span>
          </el-button>
        </el-tooltip>
        <el-tooltip content="斜体 \\textit{}" placement="top">
          <el-button
            size="small"
            @click="insert('\\textit{', '}')"
            :aria-label="插入斜体文本"
          >
            <span class="format-text">I</span>
          </el-button>
        </el-tooltip>
      </div>

      <div class="toolbar-divider"></div>

      <div class="toolbar-group toolbar-group--environments">
        <el-dropdown trigger="click" @command="handleCommand">
          <el-button size="small" aria-label="插入LaTeX环境">
            更多
            <el-icon><ArrowDown /></el-icon>
          </el-button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item
                command="itemize"
                :aria-label="插入无序列表"
              >
                <span class="dropdown-shortcut">·</span>
                无序列表
              </el-dropdown-item>
              <el-dropdown-item
                command="enumerate"
                :aria-label="插入有序列表"
              >
                <span class="dropdown-shortcut">1.</span>
                有序列表
              </el-dropdown-item>
              <el-dropdown-item
                command="figure"
                :aria-label="插入图片环境"
              >
                <span class="dropdown-shortcut">📷</span>
                图片
              </el-dropdown-item>
              <el-dropdown-item
                command="table"
                :aria-label="插入表格环境"
              >
                <span class="dropdown-shortcut">📊</span>
                表格
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
    </div>

    <!-- Screen reader announcements -->
    <div
      id="editor-description"
      class="sr-only"
      role="status"
      aria-live="polite"
    >
      {{ editorStatus }}
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from 'vue'

const isFocused = ref(false)
const isModified = ref(false)
const wordCount = ref(0)

const editorClasses = computed(() => [
  'latex-editor--optimized',
  {
    'latex-editor--focused': isFocused.value,
    'latex-editor--modified': isModified.value
  }
])

const textareaClasses = computed(() => [
  'latex-textarea--optimized',
  {
    'latex-textarea--focused': isFocused.value
  }
])

const editorLabel = computed(() =>
  `LaTeX编辑器${isModified.value ? '（已修改）' : '（已保存）'}`
)

const editorStatus = computed(() => {
  const status = []
  if (isModified.value) status.push('文档已修改')
  status.push(`共${wordCount.value}字`)
  return status.join('，')
})

function handleFocus() {
  isFocused.value = true
}

function handleBlur() {
  isFocused.value = false
}
</script>

<style scoped lang="scss">
@import '@/styles/latex-editor-design-tokens.scss';

.latex-editor {
  @include editor-card;
  height: 100%;
  width: 100%;
  display: flex;
  flex-direction: column;
  position: relative;

  &.latex-editor--focused {
    box-shadow: $editor-focus-ring;
  }
}

.latex-textarea {
  flex: 1;
  width: 100%;
  min-height: 0;
  border: none;
  outline: none;
  resize: none;
  padding: $editor-padding-lg;
  font-family: $font-family-code;
  font-size: $editor-font-size-code;
  line-height: $editor-line-height-code;
  background: $editor-bg;
  color: $editor-text;
  white-space: pre;
  overflow-wrap: normal;
  overflow-x: auto;
  tab-size: 2;
  @include editor-transition;

  &.latex-textarea--focused {
    background: var(--el-bg-color);
  }

  &:focus {
    outline: none;
  }
}

.latex-highlight {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  margin: 0;
  padding: $editor-padding-lg;
  font-family: $font-family-code;
  font-size: $editor-font-size-code;
  line-height: $editor-line-height-code;
  pointer-events: none;
  white-space: pre-wrap;
  overflow-wrap: normal;
  overflow-x: auto;
  z-index: $editor-z-base;

  code {
    background: transparent;
    font-family: inherit;
  }

  // Optimized syntax highlighting
  :deep(.token.comment) {
    color: $syntax-comment;
  }

  :deep(.token.keyword) {
    color: $syntax-keyword;
  }

  :deep(.token.string) {
    color: $syntax-string;
  }

  :deep(.token.number) {
    color: $syntax-number;
  }

  :deep(.token.function) {
    color: $syntax-function;
  }
}

.latex-toolbar {
  display: flex;
  align-items: center;
  padding: $editor-padding-sm $editor-padding-md;
  border-top: 1px solid var(--el-border-color-lighter);
  background: $editor-bg-secondary;

  .toolbar-group {
    display: flex;
    align-items: center;
    gap: $spacing-1;
  }

  .toolbar-divider {
    width: 1px;
    height: 20px;
    margin: 0 $editor-gap-sm;
    background: var(--el-border-color-lighter);
  }

  .format-text {
    font-weight: $font-weight-bold;
    font-size: $font-size-sm;
  }

  .dropdown-shortcut {
    display: inline-block;
    min-width: 24px;
    padding: 2px 6px;
    margin-right: $spacing-2;
    font-size: $font-size-xs;
    text-align: center;
    background: var(--el-bg-color-overlay);
    border-radius: $border-radius-base;
    color: var(--el-text-color-secondary);
  }
}

.sr-only {
  @include sr-only;
}
</style>
```

### 2. Enhanced Document Outline Component

```vue
<template>
  <div class="document-outline">
    <div v-if="sections.length === 0" class="outline-empty">
      <el-empty
        description="暂无文档结构"
        :image-size="40"
        aria-label="文档大纲为空"
      />
    </div>
    <div v-else class="outline-sections">
      <div
        v-for="section in sections"
        :key="section.id"
        class="outline-item"
        :class="[
          `level-${section.level}`,
          { 'is-active': activeSection === section.id }
        ]"
        :aria-selected="activeSection === section.id"
        :aria-level="section.level"
        role="treeitem"
        tabindex="0"
        @click="navigateToSection(section)"
        @keydown.enter="navigateToSection(section)"
        @keydown.space.prevent="navigateToSection(section)"
      >
        <div class="outline-content">
          <el-icon
            v-if="section.level === 1"
            class="section-icon"
            aria-hidden="true"
          >
            <Document />
          </el-icon>
          <el-icon
            v-else-if="section.level === 2"
            class="section-icon"
            aria-hidden="true"
          >
            <Folder />
          </el-icon>
          <el-icon
            v-else
            class="section-icon"
            aria-hidden="true"
          >
            <Tickets />
          </el-icon>
          <span
            class="section-title"
            :title="section.title"
            :aria-label="`${section.title}，第${section.line}行`"
          >
            {{ section.title }}
          </span>
        </div>
        <div class="outline-info">
          <span class="section-line" aria-label="第{{ section.line }}行">
            第 {{ section.line }} 行
          </span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Document, Folder, Tickets } from '@element-plus/icons-vue'

interface Section {
  id: string
  title: string
  level: number
  line: number
  type: 'section' | 'subsection' | 'subsubsection' | 'paragraph'
}

interface Props {
  content: string
}

interface Emits {
  navigate: [{ line: number; column: number }]
}

const props = defineProps<Props>()
const emit = defineEmits<Emits>()

const activeSection = ref<string | null>(null)

// Parse document structure (same as before)
const sections = computed<Section[]>(() => {
  if (!props.content) return []

  const lines = props.content.split('\n')
  const parsedSections: Section[] = []

  lines.forEach((line, index) => {
    const lineNumber = index + 1

    const sectionMatch = line.match(/^\\section\*?\{([^}]+)\}/)
    if (sectionMatch) {
      parsedSections.push({
        id: `section-${lineNumber}`,
        title: sectionMatch[1],
        level: 1,
        line: lineNumber,
        type: 'section'
      })
      return
    }

    const subsectionMatch = line.match(/^\\subsection\*?\{([^}]+)\}/)
    if (subsectionMatch) {
      parsedSections.push({
        id: `subsection-${lineNumber}`,
        title: subsectionMatch[1],
        level: 2,
        line: lineNumber,
        type: 'subsection'
      })
      return
    }

    const subsubsectionMatch = line.match(/^\\subsubsection\*?\{([^}]+)\}/)
    if (subsubsectionMatch) {
      parsedSections.push({
        id: `subsubsection-${lineNumber}`,
        title: subsubsectionMatch[1],
        level: 3,
        line: lineNumber,
        type: 'subsubsection'
      })
      return
    }

    const paragraphMatch = line.match(/^\\paragraph\*?\{([^}]+)\}/)
    if (paragraphMatch) {
      parsedSections.push({
        id: `paragraph-${lineNumber}`,
        title: paragraphMatch[1],
        level: 4,
        line: lineNumber,
        type: 'paragraph'
      })
    }
  })

  return parsedSections
})

function navigateToSection(section: Section) {
  activeSection.value = section.id
  emit('navigate', { line: section.line, column: 1 })
}
</script>

<style scoped lang="scss">
@import '@/styles/latex-editor-design-tokens.scss';

.document-outline {
  height: 100%;
  overflow-y: auto;
  padding: $spacing-1 0;
  background: $editor-bg-secondary;
  border-right: 1px solid $editor-border;

  // Optimize scrolling
  -webkit-overflow-scrolling: touch;
}

.outline-empty {
  @include editor-flex-center;
  height: 100%;
  opacity: 0.6;
}

.outline-sections {
  .outline-item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: $editor-padding-sm $editor-padding-md;
    cursor: pointer;
    border-radius: $editor-radius-md;
    margin: $spacing-1 0;
    @include editor-focus;
    @include editor-hover($editor-hover);
    @include editor-active($editor-active);
    @include editor-transition;

    // GPU acceleration
    transform: translateZ(0);
    will-change: transform;

    &.is-active {
      background-color: var(--el-color-primary-light-9);
      border-left: 3px solid var(--el-color-primary);
      padding-left: calc($editor-padding-md - 3px);

      .section-title {
        color: var(--el-color-primary);
        font-weight: $font-weight-semibold;
      }

      &::before {
        content: '';
        position: absolute;
        left: 0;
        top: 50%;
        transform: translateY(-50%);
        width: 3px;
        height: 60%;
        background: var(--el-color-primary);
        border-radius: 0 $editor-radius-sm $editor-radius-sm 0;
      }
    }

    &.level-1 {
      font-weight: $font-weight-semibold;
      font-size: $font-size-sm;
      padding-left: $spacing-2;
    }

    &.level-2 {
      font-weight: $font-weight-medium;
      font-size: $font-size-sm;
      padding-left: 20px;
    }

    &.level-3 {
      font-weight: $font-weight-normal;
      font-size: $font-size-xs;
      padding-left: 32px;
    }

    &.level-4 {
      font-weight: $font-weight-normal;
      font-size: $font-size-xs;
      padding-left: 44px;
      color: var(--el-text-color-secondary);
    }
  }

  .outline-content {
    display: flex;
    align-items: center;
    gap: $editor-gap-sm;
    flex: 1;
    min-width: 0;
  }

  .section-icon {
    font-size: $font-size-base;
    color: var(--el-text-color-secondary);
    flex-shrink: 0;
  }

  .section-title {
    flex: 1;
    @include editor-text-truncate;
    color: var(--el-text-color-primary);
  }

  .outline-info {
    flex-shrink: 0;
    margin-left: $spacing-2;
  }

  .section-line {
    font-size: $font-size-xs;
    color: var(--el-text-color-secondary);
    background: var(--el-bg-color-overlay);
    padding: 2px 6px;
    border-radius: $border-radius-base;
  }
}
</style>
```

### 3. Enhanced Symbol Palette Component

```vue
<template>
  <div class="symbol-palette">
    <div class="palette-header">
      <h3>LaTeX 符号</h3>
      <el-input
        v-model="searchQuery"
        placeholder="搜索符号..."
        size="small"
        clearable
        prefix-icon="Search"
        aria-label="搜索LaTeX符号"
      />
    </div>

    <div class="palette-content">
      <el-tabs v-model="activeTab" class="symbol-tabs">
        <el-tab-pane
          v-for="category in categories"
          :key="category.name"
          :name="category.name"
          :label="category.label"
        >
          <div class="symbol-grid">
            <div
              v-for="symbol in getFilteredSymbols(category.symbols)"
              :key="symbol.command"
              class="symbol-item"
              :title="`${symbol.command} - ${symbol.description}`"
              :aria-label="`${symbol.description}，命令：${symbol.command}`"
              tabindex="0"
              @click="insertSymbol(symbol.command)"
              @keydown.enter="insertSymbol(symbol.command)"
              @keydown.space.prevent="insertSymbol(symbol.command)"
            >
              <span class="symbol-preview" v-html="renderSymbol(symbol)"></span>
            </div>
          </div>
        </el-tab-pane>
      </el-tabs>
    </div>

    <div class="palette-footer">
      <el-button
        size="small"
        @click="insertSymbol('\\frac{ }{ }')"
        aria-label="插入分数"
      >
        插入分数
      </el-button>
      <el-button
        size="small"
        @click="insertSymbol('\\sqrt{ }')"
        aria-label="插入根号"
      >
        插入根号
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import katex from 'katex'
import 'katex/dist/katex.min.css'

interface Symbol {
  command: string
  description: string
  category: string
  mathMode?: boolean
}

interface Emits {
  insert: [symbol: string]
}

const emit = defineEmits<Emits>()

const searchQuery = ref('')
const activeTab = ref('operators')

// Categories data (same as before)
const categories = [ /* ... */ ]

function getFilteredSymbols(symbols: Symbol[]): Symbol[] {
  if (!searchQuery.value) return symbols

  const query = searchQuery.value.toLowerCase()
  return symbols.filter(symbol =>
    symbol.command.toLowerCase().includes(query) ||
    symbol.description.toLowerCase().includes(query)
  )
}

function renderSymbol(symbol: Symbol): string {
  try {
    if (symbol.command.includes('{ }') || symbol.command.includes('{}')) {
      const simplified = symbol.command.replace(/\{ ?\}?/g, 'x')
      return katex.renderToString(simplified, {
        displayMode: false,
        throwOnError: false
      })
    }

    return katex.renderToString(symbol.command, {
      displayMode: false,
      throwOnError: false
    })
  } catch {
    return symbol.command
  }
}

function insertSymbol(symbol: string) {
  emit('insert', symbol)
}
</script>

<style scoped lang="scss">
@import '@/styles/latex-editor-design-tokens.scss';

.symbol-palette {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: $editor-bg;
}

.palette-header {
  padding: $editor-padding-lg;
  border-bottom: 1px solid var(--el-border-color-lighter);

  h3 {
    margin: 0 0 $editor-padding-md 0;
    font-size: $font-size-sm;
    font-weight: $font-weight-semibold;
    color: var(--el-text-color-primary);
  }
}

.palette-content {
  flex: 1;
  overflow-y: auto;
  padding: 0;

  // Optimize scrolling
  -webkit-overflow-scrolling: touch;
}

.symbol-tabs {
  height: 100%;

  :deep(.el-tabs__content) {
    height: calc(100% - 40px);
    overflow-y: auto;
  }

  :deep(.el-tab-pane) {
    height: 100%;
  }
}

.symbol-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax($symbol-grid-item-size, 1fr));
  gap: $symbol-grid-gap;
  padding: $editor-padding-lg;

  @include editor-mobile {
    grid-template-columns: repeat(auto-fill, minmax($symbol-grid-item-size-mobile, 1fr));
    gap: $spacing-1;
    padding: $editor-padding-md;
  }
}

.symbol-item {
  @include editor-card;
  @include editor-flex-center;
  @include editor-focus;
  @include editor-hover(var(--el-color-primary-light-9));
  @include editor-active;

  height: 50px;
  min-height: 44px; // Touch target size
  cursor: pointer;
  position: relative;

  // GPU acceleration
  transform: translateZ(0);
  will-change: transform;

  .symbol-preview {
    font-size: $font-size-sm;
    color: var(--el-text-color-primary);
    transition: transform $editor-transition-base $editor-easing;

    :deep(.katex) {
      font-size: 14px;
    }
  }

  &:hover .symbol-preview {
    transform: scale(1.1);
  }

  // Loading state
  &.is-loading {
    opacity: 0.6;
    pointer-events: none;

    &::after {
      content: '';
      position: absolute;
      width: 20px;
      height: 20px;
      border: 2px solid var(--el-color-primary);
      border-right-color: transparent;
      border-radius: 50%;
      animation: editor-spinner 0.6s linear infinite;
    }
  }
}

@keyframes editor-spinner {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

.palette-footer {
  padding: $editor-padding-lg;
  border-top: 1px solid var(--el-border-color-lighter);
  display: flex;
  gap: $editor-gap-sm;
  justify-content: center;
}
</style>
```

---

## Integration Steps

### Step 1: Update Main Styles

In `/frontend/src/styles/index.scss`, add:

```scss
// Import design system
@import './design-system.scss';
@import './variables.scss';

// Import LaTeX editor styles
@import './latex-editor-design-tokens.scss';
@import './latex-editor-optimized.scss';

// Global styles
@import './common.scss';
@import './transition.scss';
```

### Step 2: Update Components

1. Replace hardcoded values in existing components
2. Add accessibility attributes
3. Implement proper state management
4. Add responsive design improvements

### Step 3: Test

1. Test in light and dark modes
2. Test with keyboard navigation
3. Test on mobile devices
4. Test with screen readers

### Step 4: Optimize

1. Profile performance
2. Optimize animations
3. Reduce bundle size
4. Improve accessibility scores

---

## Expected Results

### Design System Consistency
- 95%+ consistent use of design tokens
- Uniform spacing, typography, and colors
- Professional, polished appearance

### Accessibility Compliance
- WCAG AA compliant color contrast (4.5:1)
- Full keyboard navigation support
- Screen reader announcements for dynamic content
- Proper ARIA labels and roles

### Responsive Design
- Optimized layouts for all screen sizes
- Touch-friendly targets (44px minimum)
- Smooth transitions between breakpoints
- Mobile-optimized interactions

### Performance
- Smooth 60fps animations
- Efficient rendering with GPU acceleration
- Optimized bundle size
- Fast load times

---

## Next Steps

1. **Review** the analysis report for detailed findings
2. **Implement** the optimized styles in your components
3. **Test** thoroughly across browsers and devices
4. **Iterate** based on user feedback and analytics

The improved design system will provide a solid foundation for future enhancements and ensure a consistent, accessible, and performant user experience.
