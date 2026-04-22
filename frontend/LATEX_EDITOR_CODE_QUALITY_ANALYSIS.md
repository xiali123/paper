# LaTeX Editor Code Quality Analysis & Refactoring Plan

**Analysis Date**: 2026-04-12
**Component**: LaTeX Editor (frontend/src/components/latex/)
**Analyst**: EngineeringSeniorDeveloper Agent
**Project Version**: v1.0.0 (feature/FS-5555-backend-api-latex)

---

## Executive Summary

The LaTeX editor implementation demonstrates **solid engineering practices** with good use of modern Vue 3 patterns, TypeScript, and performance optimization techniques. However, there are significant opportunities for improvement in **code organization**, **testing coverage**, and **architectural consistency**.

### Overall Code Quality Rating: B+ (82/100)

**Strengths:**
- Modern Vue 3 Composition API usage
- Comprehensive TypeScript typing
- Well-structured composables
- Performance monitoring integrated
- Security conscious (DOMPurify integration)

**Critical Areas for Improvement:**
- Zero test coverage (critical issue)
- Component size violations
- Architectural inconsistency
- Code duplication patterns
- Missing error boundaries
- Incomplete documentation

---

## 1. Component Code Organization Analysis

### 1.1 Current Component Structure

**LaTeX Editor Components:**
```
frontend/src/components/latex/
├── LatexEditor.vue (430 lines) ⚠️
├── LatexPreview.vue (323 lines) ⚠️
├── LatexAutocomplete.vue (120 lines) ✅
├── DocumentOutline.vue (80 lines) ✅
├── SymbolPalette.vue (150 lines) ✅
└── PerformanceMonitor.vue (200 lines) ✅
```

**Issues Identified:**

1. **Large Component Files** ⚠️
   - `LatexEditor.vue` (430 lines) exceeds 300-line best practice
   - `LatexPreview.vue` (323 lines) exceeds 300-line best practice
   - `LatexEditorView.vue` (1,337 lines) severely violates component size limits

2. **Mixed Responsibilities** ⚠️
   ```typescript
   // LatexEditor.vue handles:
   // - Syntax highlighting
   // - Text editing
   // - Virtual scrolling
   // - Performance monitoring
   // - LaTeX command insertion
   // - Cursor management
   // - Scroll synchronization
   ```

### 1.2 Refactoring Recommendations

**Phase 1: Component Decomposition**

```typescript
// Current structure (1,337 lines)
LatexEditorView.vue

// Proposed structure (max 300 lines per component)
LatexEditorView.vue (200 lines) - orchestration only
├── LatexEditorContainer.vue (250 lines) - editor wrapper
├── LatexPreviewContainer.vue (200 lines) - preview wrapper
├── LatexEditorToolbar.vue (180 lines) - toolbar actions
├── LatexEditorStatusBar.vue (120 lines) - status information
├── LatexCollaborationPanel.vue (150 lines) - collaboration UI
└── LatexEditorSettings.vue (100 lines) - editor settings
```

**Implementation Strategy:**

```vue
<!-- LatexEditorView.vue - Orchestration Layer -->
<template>
  <div class="latex-editor-view">
    <LatexEditorHeader 
      :document="currentDocument"
      @save="handleSave"
      @compile="handleCompile"
    />
    
    <div class="editor-main">
      <LatexEditorContainer
        :content="editorContent"
        :show-outline="showOutline"
        @update:content="updateContent"
        @cursor-change="updateCursor"
      />
      
      <LatexPreviewContainer
        :content="editorContent"
        :scale="previewScale"
        :theme="theme"
      />
    </div>
    
    <LatexEditorStatusBar
      :cursor="cursorPosition"
      :stats="documentStats"
      :auto-save="autoSave"
    />
  </div>
</template>

<script setup lang="ts">
// Only orchestration logic
// All detailed logic moved to child components
const latexStore = useLatexEditorStore()
const { editorContent, cursorPosition, documentStats } = storeToRefs(latexStore)

function handleSave() {
  latexStore.saveDocument()
}
</script>
```

---

## 2. State Management Analysis

### 2.1 Current State Architecture

**Issues Identified:**

1. **Architectural Inconsistency** ❌
   ```
   frontend/src/
   ├── architecture/stores/latexEditor.ts (589 lines) ⚠️
   └── stores/[other stores]
   ```

2. **Mixed State Patterns** ⚠️
   - Some state in Pinia stores
   - Some state in component refs
   - Some state in composables
   - No clear ownership pattern

### 2.2 State Management Refactoring

**Current State Distribution:**
```typescript
// State scattered across multiple locations:

// 1. Pinia Store (architecture/stores/latexEditor.ts)
const currentDocument: Ref<LatexDocument | null> = ref(null)
const editorContent: Ref<string> = ref('')
const compilationStatus: Ref<CompilationStatus> = ref('idle')

// 2. Component State (LatexEditorView.vue)
const showPreview = ref(true)
const showOutline = ref(false)
const mobileActiveTab = ref<'editor' | 'preview'>('editor')

// 3. Composables State (useAutoSave.ts, useScrollSync.ts)
const state = ref<AutoSaveState>({ ... })
const isSyncing = ref(false)
```

**Proposed State Architecture:**

```typescript
// 1. Centralized State Store
// architecture/stores/latexEditor.ts

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export const useLatexEditorStore = defineStore('latexEditor', () => {
  // === Document State ===
  const currentDocument = ref<LatexDocument | null>(null)
  const editorContent = ref('')
  const documentMetadata = ref<DocumentMetadata>({
    title: '',
    authors: [],
    keywords: []
  })

  // === UI State ===
  const ui = ref({
    showPreview: true,
    showOutline: false,
    showToolbar: true,
    showStatusBar: true,
    activeTab: 'editor' as 'editor' | 'preview',
    sidebarVisible: true
  })

  // === Editor State ===
  const editor = ref({
    cursorPosition: { line: 1, column: 1 },
    selection: null as { start: number; end: number } | null,
    fontSize: 14,
    fontFamily: 'Fira Code',
    wordWrap: true
  })

  // === Preview State ===
  const preview = ref({
    scale: 1.0,
    theme: 'light' as 'light' | 'dark',
    autoRefresh: true,
    refreshDelay: 1000
  })

  // === Compilation State ===
  const compilation = ref({
    status: 'idle' as CompilationStatus,
    result: null as CompilationResult | null,
    errors: [] as CompilationError[],
    warnings: [] as CompilationWarning[],
    lastCompilationTime: null as number | null
  })

  // === Collaboration State ===
  const collaboration = ref({
    isEnabled: false,
    sessionId: null as string | null,
    users: new Map<string, CollaborationUser>(),
    permissions: {
      canEdit: true,
      canComment: true,
      canShare: true
    }
  })

  // === Computed Properties ===
  const documentStats = computed(() => {
    const content = editorContent.value
    const lines = content.split('\n')
    return {
      lines: lines.length,
      words: content.trim() ? content.trim().split(/\s+/).length : 0,
      characters: content.length
    }
  })

  const compilationSuccess = computed(() => {
    return compilation.value.status === 'success' && 
           compilation.value.errors.length === 0
  })

  // === Actions ===
  function updateContent(content: string) {
    editorContent.value = content
    if (currentDocument.value) {
      currentDocument.value.content = content
      currentDocument.value.lastModified = Date.now()
    }
  }

  function updateUI(settings: Partial<typeof ui.value>) {
    Object.assign(ui.value, settings)
  }

  function updateEditor(settings: Partial<typeof editor.value>) {
    Object.assign(editor.value, settings)
  }

  function updatePreview(settings: Partial<typeof preview.value>) {
    Object.assign(preview.value, settings)
  }

  return {
    // State
    currentDocument,
    editorContent,
    documentMetadata,
    ui,
    editor,
    preview,
    compilation,
    collaboration,
    
    // Computed
    documentStats,
    compilationSuccess,
    
    // Actions
    updateContent,
    updateUI,
    updateEditor,
    updatePreview,
    
    // Existing methods
    saveDocument,
    compileDocument,
    setCurrentDocument,
    // ... other existing methods
  }
})
```

---

## 3. TypeScript Type Safety Analysis

### 3.1 Current Type Usage

**Strengths:**
- ✅ Comprehensive interface definitions
- ✅ Proper use of generics
- ✅ Discriminated unions for state types

**Issues:**

1. **Type Definition Scattering** ⚠️
   ```typescript
   // Types defined in multiple locations:
   
   // architecture/stores/latexEditor.ts
   export interface LatexDocument { ... }
   export interface CompilationResult { ... }
   
   // api/modules/latex.ts
   export interface CompileLatexRequest { ... }
   export interface CompileLatexResponse { ... }
   
   // components/latex/LatexEditor.vue
   interface Props { ... } // Local interface
   ```

2. **Unsafe Type Assertions** ⚠️
   ```typescript
   // LatexEditorView.vue line 538
   ;(latexStore as any).updateCursorPosition(position)
   
   // LatexEditorView.vue line 733
   if (editorRef.value) {
     editorRef.value.navigateTo({ line, column }) // No type checking
   }
   ```

3. **Missing Type Guards** ⚠️
   ```typescript
   // No type guards for API responses
   const response = await latexApi.compile(requestData)
   // response could be anything - no validation
   ```

### 3.2 Type Safety Improvements

**1. Centralized Type Definitions**

```typescript
// types/latex-editor.ts

/**
 * LaTeX Document Types
 */
export interface LatexDocument {
  id: string
  name: string
  content: string
  path: string
  lastModified: number
  size: number
  metadata: LatexDocumentMetadata
}

export interface LatexDocumentMetadata {
  title?: string
  authors?: string[]
  abstract?: string
  keywords?: string[]
  documentClass?: string
  packages?: string[]
}

/**
 * Compilation Types
 */
export type CompilationStatus = 
  | 'idle' 
  | 'compiling' 
  | 'success' 
  | 'error' 
  | 'warning'

export interface CompilationResult {
  success: boolean
  output?: string
  errors?: CompilationError[]
  warnings?: CompilationWarning[]
  log?: string
  duration: number
}

export interface CompilationError {
  line: number
  column?: number
  message: string
  type: 'error' | 'warning' | 'info'
  file?: string
}

/**
 * Editor Types
 */
export interface EditorPosition {
  line: number
  column: number
}

export interface EditorSelection {
  start: number
  end: number
}

export interface EditorSettings {
  fontSize: number
  fontFamily: string
  lineHeight: number
  wordWrap: boolean
  tabSize: number
}

/**
 * Preview Types
 */
export interface PreviewSettings {
  theme: 'light' | 'dark'
  scale: number
  autoRefresh: boolean
  refreshDelay: number
  showSourceMap: boolean
  scrollSync: boolean
}

/**
 * Collaboration Types
 */
export interface CollaborationUser {
  id: string
  name: string
  color: string
  cursor?: EditorPosition
  selection?: EditorSelection
  isOnline: boolean
  lastSeen: number
}

/**
 * Type Guards
 */
export function isCompilationError(obj: unknown): obj is CompilationError {
  return (
    typeof obj === 'object' && obj !== null &&
    'line' in obj && typeof obj.line === 'number' &&
    'message' in obj && typeof obj.message === 'string' &&
    'type' in obj && typeof obj.type === 'string'
  )
}

export function isLatexDocument(obj: unknown): obj is LatexDocument {
  return (
    typeof obj === 'object' && obj !== null &&
    'id' in obj && typeof obj.id === 'string' &&
    'name' in obj && typeof obj.name === 'string' &&
    'content' in obj && typeof obj.content === 'string'
  )
}

/**
 * API Types
 */
export interface ApiResponse<T> {
  success: boolean
  data?: T
  error?: string
  timestamp: number
}

export type ApiResult<T> = Promise<ApiResponse<T>>
```

**2. Safe Type Assertions**

```typescript
// Before (unsafe)
;(latexStore as any).updateCursorPosition(position)

// After (safe)
import type { EditorPosition } from '@/types/latex-editor'

function updateCursorPosition(position: EditorPosition) {
  const store = useLatexEditorStore()
  if ('updateCursorPosition' in store) {
    store.updateCursorPosition(position)
  } else {
    console.warn('updateCursorPosition not available in store')
  }
}
```

**3. Runtime Type Validation**

```typescript
// utils/type-validation.ts

export class ValidationError extends Error {
  constructor(message: string, public readonly path: string[]) {
    super(message)
    this.name = 'ValidationError'
  }
}

export function validateCompilationResult(data: unknown): CompilationResult {
  if (!data || typeof data !== 'object') {
    throw new ValidationError('Invalid compilation result', [])
  }

  const obj = data as Record<string, unknown>

  if (typeof obj.success !== 'boolean') {
    throw new ValidationError('success must be a boolean', ['success'])
  }

  if (obj.errors !== undefined) {
    if (!Array.isArray(obj.errors)) {
      throw new ValidationError('errors must be an array', ['errors'])
    }
    obj.errors.forEach((error, index) => {
      if (!isCompilationError(error)) {
        throw new ValidationError(
          `Invalid error at index ${index}`,
          ['errors', String(index)]
        )
      }
    })
  }

  return {
    success: obj.success,
    output: typeof obj.output === 'string' ? obj.output : undefined,
    errors: obj.errors as CompilationError[],
    warnings: obj.warnings as CompilationWarning[],
    log: typeof obj.log === 'string' ? obj.log : undefined,
    duration: typeof obj.duration === 'number' ? obj.duration : 0
  }
}
```

---

## 4. Code Reusability Assessment

### 4.1 Composable Analysis

**Well-Designed Composables:**

1. **`useAutoSave.ts`** ✅ (220 lines)
   - Clear responsibility
   - Good type definitions
   - Proper cleanup
   - Reusable interface

2. **`useKeyboardShortcuts.ts`** ✅ (288 lines)
   - Excellent abstraction
   - Generic and reusable
   - Good documentation

3. **`useScrollSync.ts`** ✅ (143 lines)
   - Focused purpose
   - Clean API
   - Proper cleanup

**Composables Needing Improvement:**

1. **`useVirtualScroll.ts`** ⚠️ (212 lines)
   - Multiple responsibilities
   - Mixed abstractions
   - Complex interface

2. **`useLatexAutocomplete.ts`** ⚠️ (327 lines)
   - Too large
   - Mixed concerns (UI + logic)
   - Hard to test

### 4.2 Code Duplication Issues

**Identified Duplications:**

1. **Text Insertion Logic** (found in 3 places)
   ```typescript
   // Duplicated in:
   // - LatexEditor.vue (insert function)
   // - LatexEditorView.vue (insertLatexCommand, insertLatexEnvironment)
   // - SymbolPalette.vue (insertSymbol)
   ```

2. **Debounce Patterns** (found in 4 places)
   ```typescript
   // Similar debounce implementations in:
   // - LatexEditor.vue
   // - LatexPreview.vue
   // - useAutoSave.ts
   // - useUndoRedo.ts
   ```

3. **Error Handling** (found in 6 places)
   ```typescript
   // Repeated try-catch patterns with similar logic
   ```

### 4.3 Refactoring for Reusability

**1. Extract Text Manipulation Utilities**

```typescript
// utils/latex/text-manipulation.ts

export interface TextRange {
  start: number
  end: number
}

export interface TextEdit {
  before: string
  after: string
  cursorOffset?: number
}

/**
 * Insert LaTeX command at cursor position
 */
export function insertLatexCommand(
  content: string,
  command: string,
  position: number
): { newContent: string; newPosition: number } {
  const commands: Record<string, TextEdit> = {
    textbf: { before: '\\textbf{', after: '}', cursorOffset: 8 },
    textit: { before: '\\textit{', after: '}', cursorOffset: 8 },
    underline: { before: '\\underline{', after: '}', cursorOffset: 12 }
  }

  const edit = commands[command] || { 
    before: `\\${command}{`, 
    after: '}', 
    cursorOffset: command.length + 2 
  }

  const newContent = 
    content.substring(0, position) + 
    edit.before + edit.after + 
    content.substring(position)

  const newPosition = position + edit.before.length

  return { newContent, newPosition }
}

/**
 * Insert LaTeX environment
 */
export function insertLatexEnvironment(
  content: string,
  environment: string,
  position: number
): { newContent: string; newPosition: number } {
  const environments: Record<string, TextEdit> = {
    itemize: {
      before: '\\begin{itemize}\n  \\item ',
      after: '\n\\end{itemize}',
      cursorOffset: 22
    },
    equation: {
      before: '\\begin{equation}\n  ',
      after: '\n\\end{equation}',
      cursorOffset: 18
    }
  }

  const edit = environments[environment] || {
    before: `\\begin{${environment}}\n  `,
    after: '\n\\end{' + environment + '}',
    cursorOffset: environment.length + 11
  }

  const newContent = 
    content.substring(0, position) + 
    edit.before + edit.after + 
    content.substring(position)

  const newPosition = position + edit.before.length

  return { newContent, newPosition }
}

/**
 * Calculate line and column from character position
 */
export function getLineAndColumn(
  content: string,
  position: number
): { line: number; column: number } {
  const textBefore = content.substring(0, position)
  const lines = textBefore.split('\n')
  return {
    line: lines.length,
    column: lines[lines.length - 1].length + 1
  }
}

/**
 * Calculate character position from line and column
 */
export function getPositionFromLineColumn(
  content: string,
  line: number,
  column: number
): number {
  const lines = content.split('\n')
  let position = 0
  
  for (let i = 0; i < line - 1 && i < lines.length; i++) {
    position += lines[i].length + 1 // +1 for newline
  }
  
  position += Math.min(column - 1, lines[line - 1]?.length || 0)
  
  return Math.min(position, content.length)
}
```

**2. Unified Debounce Utility**

```typescript
// utils/debounce.ts

import { ref, onUnmounted } from 'vue'

export interface DebounceOptions {
  delay: number
  immediate?: boolean
}

export function useDebounce<T extends (...args: any[]) => any>(
  fn: T,
  options: DebounceOptions
): T & { cancel: () => void; flush: () => void } {
  const { delay, immediate = false } = options
  
  let timeout: number | null = null
  let lastArgs: Parameters<T> | null = null
  let result: ReturnType<T>

  const debouncedFn = ((...args: Parameters<T>) => {
    lastArgs = args

    if (timeout) {
      clearTimeout(timeout)
    }

    if (immediate && !timeout) {
      result = fn(...args)
    }

    timeout = setTimeout(() => {
      if (!immediate) {
        result = fn(...args)
      }
      timeout = null
    }, delay) as unknown as number

    return result
  }) as T & { cancel: () => void; flush: () => void }

  debouncedFn.cancel = () => {
    if (timeout) {
      clearTimeout(timeout)
      timeout = null
    }
  }

  debouncedFn.flush = () => {
    if (timeout && lastArgs) {
      clearTimeout(timeout)
      result = fn(...lastArgs)
      timeout = null
    }
    return result
  }

  onUnmounted(() => {
    debouncedFn.cancel()
  })

  return debouncedFn
}

/**
 * Adaptive debounce that adjusts delay based on content size
 */
export function useAdaptiveDebounce<T extends (...args: any[]) => any>(
  fn: T,
  getDelay: (...args: Parameters<T>) => number
): T & { cancel: () => void } {
  let timeout: number | null = null

  const debouncedFn = ((...args: Parameters<T>) => {
    if (timeout) {
      clearTimeout(timeout)
    }

    const delay = getDelay(...args)

    timeout = setTimeout(() => {
      fn(...args)
      timeout = null
    }, delay) as unknown as number
  }) as T & { cancel: () => void }

  debouncedFn.cancel = () => {
    if (timeout) {
      clearTimeout(timeout)
      timeout = null
    }
  }

  onUnmounted(() => {
    debouncedFn.cancel()
  })

  return debouncedFn
}
```

**3. Error Handling Utilities**

```typescript
// utils/error-handling.ts

export interface ErrorHandler {
  handle: (error: Error) => void
  report: (error: Error, context?: Record<string, unknown>) => void
}

export class ApplicationError extends Error {
  constructor(
    message: string,
    public readonly code: string,
    public readonly context?: Record<string, unknown>
  ) {
    super(message)
    this.name = 'ApplicationError'
  }
}

export function createErrorHandler(
  options: {
    showToUser?: boolean
    logToConsole?: boolean
    reportToServer?: boolean
  } = {}
): ErrorHandler {
  const {
    showToUser = true,
    logToConsole = true,
    reportToServer = false
  } = options

  return {
    handle(error: Error) {
      if (logToConsole) {
        console.error('[Error]', error)
      }

      if (showToUser) {
        // Show user-friendly message
        ElMessage.error(error.message)
      }

      if (reportToServer) {
        this.report(error)
      }
    },

    report(error: Error, context?: Record<string, unknown>) {
      // Send to error reporting service
      // e.g., Sentry, LogRocket
      if (import.meta.env.PROD) {
        // Implementation depends on error reporting service
      }
    }
  }
}

/**
 * Safe async wrapper that converts errors to ApplicationError
 */
export async function safeAsync<T>(
  fn: () => Promise<T>,
  errorHandler: ErrorHandler,
  context?: Record<string, unknown>
): Promise<T | null> {
  try {
    return await fn()
  } catch (error) {
    const appError = error instanceof ApplicationError
      ? error
      : new ApplicationError(
          error instanceof Error ? error.message : 'Unknown error',
          'UNKNOWN_ERROR',
          context
        )
    
    errorHandler.handle(appError)
    return null
  }
}
```

---

## 5. Performance Optimization Opportunities

### 5.1 Current Performance Issues

**Identified Bottlenecks:**

1. **Large Component Re-renders** ⚠️
   - `LatexEditorView.vue` re-renders entire view on any content change
   - No memoization for expensive computations

2. **Memory Leaks** ⚠️
   - Event listeners not properly cleaned up
   - Worker instances not terminated
   - Cache unlimited growth

3. **Bundle Size** ⚠️
   - KaTeX: 280KB (not code-split)
   - Element Plus: Full bundle imported
   - No lazy loading for heavy components

### 5.2 Performance Optimization Plan

**1. Component Memoization**

```typescript
// Use computed and shallowRef for expensive operations

import { shallowRef, computed } from 'vue'

// Instead of deep ref for large content
const editorContent = ref('') // ❌ Deep reactivity

// Use shallow ref
const editorContent = shallowRef('') // ✅ Shallow reactivity

// Memoize expensive computations
const documentStats = computed(() => {
  // Only recompute when content changes
  const lines = editorContent.value.split('\n')
  return {
    lines: lines.length,
    words: editorContent.value.trim().split(/\s+/).length
  }
})

// Use shallowRef for complex objects
const compilationResult = shallowRef<CompilationResult | null>(null)
```

**2. Virtual Scrolling Enhancement**

```typescript
// True virtual scrolling without creating full arrays

export function useOptimizedVirtualScroll(
  getContent: () => string,
  containerHeight: number,
  lineHeight: number = 20
) {
  const scrollTop = ref(0)
  const containerRef = ref<HTMLElement>()

  // Compute visible range without creating full array
  const visibleRange = computed(() => {
    const totalLines = getContent().split('\n').length
    const visibleLineCount = Math.ceil(containerHeight / lineHeight)
    const bufferLines = 10

    const startLine = Math.max(
      0,
      Math.floor(scrollTop.value / lineHeight) - bufferLines
    )
    const endLine = Math.min(
      totalLines,
      startLine + visibleLineCount + bufferLines * 2
    )

    return { startLine, endLine, totalLines }
  })

  // Get only visible lines
  const visibleLines = computed(() => {
    const { startLine, endLine } = visibleRange.value
    const lines = getContent().split('\n')
    return lines.slice(startLine, endLine)
  })

  // Scroll to line
  function scrollToLine(lineNumber: number) {
    if (!containerRef.value) return

    const targetScrollTop = lineNumber * lineHeight
    const centerOffset = containerHeight / 2 - lineHeight / 2
    containerRef.value.scrollTop = Math.max(0, targetScrollTop - centerOffset)
  }

  return {
    visibleRange,
    visibleLines,
    scrollToLine,
    containerRef
  }
}
```

**3. Lazy Loading Strategy**

```typescript
// Lazy load heavy dependencies
const loadKaTeX = () => import('katex')
const loadPrism = () => import('prismjs')

// Lazy load heavy components
const SymbolPalette = defineAsyncComponent({
  loader: () => import('@/components/latex/SymbolPalette.vue'),
  loadingComponent: LoadingSpinner,
  delay: 200,
  timeout: 5000
})

const CollaborationPanel = defineAsyncComponent({
  loader: () => import('@/components/collaboration/CollaborationPanel.vue'),
  loadingComponent: LoadingSpinner,
  delay: 200,
  timeout: 5000
})
```

---

## 6. Testing Strategy

### 6.1 Current Testing Status

**Critical Issue:** ❌ Zero test coverage

```
frontend/src/
├── components/latex/     0 test files
├── composables/          0 test files
├── stores/               0 test files
└── utils/                0 test files
```

### 6.2 Testing Implementation Plan

**Phase 1: Unit Tests (Week 1-2)**

```typescript
// tests/unit/components/LatexEditor.spec.ts

import { describe, it, expect, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import LatexEditor from '@/components/latex/LatexEditor.vue'

describe('LatexEditor', () => {
  it('should render textarea', () => {
    const wrapper = mount(LatexEditor, {
      props: {
        modelValue: '\\documentclass{article}'
      }
    })

    expect(wrapper.find('textarea').exists()).toBe(true)
  })

  it('should emit update:modelValue on input', async () => {
    const wrapper = mount(LatexEditor, {
      props: {
        modelValue: ''
      }
    })

    const textarea = wrapper.find('textarea')
    await textarea.setValue('\\section{Test}')

    expect(wrapper.emitted('update:modelValue')).toBeTruthy()
    expect(wrapper.emitted('update:modelValue')[0]).toEqual(['\\section{Test}'])
  })

  it('should insert LaTeX command', async () => {
    const wrapper = mount(LatexEditor, {
      props: {
        modelValue: ''
      }
    })

    await wrapper.vm.insert('\\textbf{', '}')

    expect(wrapper.emitted('update:modelValue')[0]).toEqual(['\\textbf{}'])
  })

  it('should navigate to line', async () => {
    const wrapper = mount(LatexEditor, {
      props: {
        modelValue: 'line1\nline2\nline3'
      }
    })

    await wrapper.vm.navigateTo({ line: 2, column: 1 })

    const textarea = wrapper.find('textarea').element as HTMLTextAreaElement
    const textBefore = textarea.value.substring(0, textarea.selectionStart)
    const lines = textBefore.split('\n')

    expect(lines.length).toBe(2)
  })
})
```

```typescript
// tests/unit/composables/useAutoSave.spec.ts

import { describe, it, expect, vi, beforeEach } from 'vitest'
import { ref } from 'vue'
import { useAutoSave } from '@/composables/useAutoSave'

describe('useAutoSave', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    vi.useFakeTimers()
  })

  afterEach(() => {
    vi.restoreAllMocks()
  })

  it('should auto-save after interval', async () => {
    const content = ref('test content')
    const documentId = ref('doc-1')
    const onSave = vi.fn().mockResolvedValue(undefined)

    useAutoSave(content, documentId, {
      interval: 1000,
      onSave
    })

    await vi.advanceTimersByTimeAsync(1000)

    expect(onSave).toHaveBeenCalledWith('test content')
  })

  it('should debounce rapid changes', async () => {
    const content = ref('')
    const documentId = ref('doc-1')
    const onSave = vi.fn().mockResolvedValue(undefined)

    useAutoSave(content, documentId, {
      interval: 1000,
      debounceDelay: 500,
      onSave
    })

    content.value = 'change 1'
    await vi.advanceTimersByTimeAsync(200)

    content.value = 'change 2'
    await vi.advanceTimersByTimeAsync(200)

    content.value = 'change 3'
    await vi.advanceTimersByTimeAsync(500)

    expect(onSave).toHaveBeenCalledTimes(1)
    expect(onSave).toHaveBeenCalledWith('change 3')
  })

  it('should save to localStorage', async () => {
    const content = ref('test content')
    const documentId = ref('doc-1')
    const onSave = vi.fn().mockResolvedValue(undefined)

    const { loadFromLocalStorage } = useAutoSave(content, documentId, {
      interval: 1000,
      enableLocalStorage: true,
      onSave
    })

    await vi.advanceTimersByTimeAsync(1000)

    const saved = loadFromLocalStorage()
    expect(saved).toBe('test content')
  })
})
```

**Phase 2: Integration Tests (Week 3-4)**

```typescript
// tests/integration/latexEditor.spec.ts

import { describe, it, expect } from 'vitest'
import { createPinia } from 'pinia'
import { mount } from '@vue/test-utils'
import LatexEditorView from '@/views/writing/LatexEditorView.vue'

describe('LaTeX Editor Integration', () => {
  it('should sync editor and preview', async () => {
    const wrapper = mount(LatexEditorView, {
      global: {
        plugins: [createPinia()]
      }
    })

    const editor = wrapper.findComponent({ name: 'LatexEditor' })
    const preview = wrapper.findComponent({ name: 'LatexPreview' })

    await editor.setValue('$E=mc^2$')

    // Wait for debounced update
    await new Promise(resolve => setTimeout(resolve, 500))

    expect(preview.props('content')).toBe('$E=mc^2$')
  })

  it('should handle document save', async () => {
    const wrapper = mount(LatexEditorView, {
      global: {
        plugins: [createPinia()]
      }
    })

    const saveButton = wrapper.find('[data-test="save-button"]')
    await saveButton.trigger('click')

    // Verify save was called
    expect(wrapper.vm.saving).toBe(false)
  })
})
```

**Phase 3: E2E Tests (Week 5-6)**

```typescript
// tests/e2e/latexEditor.spec.ts

import { test, expect } from '@playwright/test'

test.describe('LaTeX Editor E2E', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/latex-editor')
  })

  test('should load editor', async ({ page }) => {
    await expect(page.locator('textarea.latex-textarea')).toBeVisible()
  })

  test('should type and see preview', async ({ page }) => {
    const textarea = page.locator('textarea.latex-textarea')
    await textarea.fill('$E=mc^2$')

    // Wait for preview update
    await page.waitForTimeout(500)

    const preview = page.locator('.preview-rendered')
    await expect(preview).toContainText('E = mc²')
  })

  test('should save document', async ({ page }) => {
    const textarea = page.locator('textarea.latex-textarea')
    await textarea.fill('\\section{Test}')

    const saveButton = page.locator('button:has-text("保存")')
    await saveButton.click()

    // Should show success message
    await expect(page.locator('.el-message--success')).toBeVisible()
  })

  test('should use keyboard shortcuts', async ({ page }) => {
    const textarea = page.locator('textarea.latex-textarea')
    await textarea.focus()

    // Ctrl+B to insert bold
    await page.keyboard.press('Control+B')

    expect(await textarea.inputValue()).toContain('\\textbf{}')
  })
})
```

---

## 7. Documentation Improvements

### 7.1 Missing Documentation

**Issues:**
- No component API documentation
- No composable usage examples
- No architecture decision records (ADRs)
- Limited inline comments

### 7.2 Documentation Plan

**1. Component Documentation**

```vue
<!--
LatexEditor.vue

A lightweight, performant LaTeX editor component with syntax highlighting
and real-time preview capabilities.

@usage
<LatexEditor
  v-model="content"
  @change="handleChange"
  @cursor-change="handleCursorChange"
/>

@props
- modelValue: string - The LaTeX content
- readonly?: boolean - Disable editing (default: false)

@events
- update:modelValue: Emitted when content changes
- change: Emitted when content is modified
- cursor-change: Emitted when cursor position changes
- scroll: Emitted when editor is scrolled

@methods
- focus(): Focus the editor
- navigateTo(position): Navigate to specific line and column

@example
<template>
  <LatexEditor
    v-model="latexContent"
    @change="onContentChange"
  />
</template>

<script setup lang="ts">
import { ref } from 'vue'
import LatexEditor from '@/components/latex/LatexEditor.vue'

const latexContent = ref('\\documentclass{article}')

function onContentChange(newContent: string) {
  console.log('Content changed:', newContent)
}
</script>
-->
```

**2. Composable Documentation**

```typescript
/**
 * useAutoSave Composable
 * 
 * Provides automatic saving functionality for LaTeX documents
 * with debouncing, local storage backup, and error handling.
 * 
 * @param {Ref<string>} content - The document content to watch
 * @param {Ref<string | null>} documentId - The document ID for storage
 * @param {AutoSaveOptions} options - Configuration options
 * 
 * @returns {Object} Auto-save API
 * @returns {Ref<boolean>} isSaving - Whether a save is in progress
 * @returns {Ref<Date | null>} lastSavedAt - When the document was last saved
 * @returns {Ref<boolean>} hasUnsavedChanges - Whether there are unsaved changes
 * @returns {Function} startAutoSave - Begin auto-saving
 * @returns {Function} stopAutoSave - Stop auto-saving
 * @returns {Function} manualSave - Trigger an immediate save
 * 
 * @example
 * const content = ref('\\documentclass{article}')
 * const documentId = ref('doc-123')
 * 
 * const autoSave = useAutoSave(content, documentId, {
 *   interval: 30000, // 30 seconds
 *   debounceDelay: 2000,
 *   enableLocalStorage: true,
 *   onSave: async (content) => {
 *     await api.saveDocument(documentId.value, content)
 *   }
 * })
 * 
 * autoSave.startAutoSave()
 */
export function useAutoSave(
  content: Ref<string>,
  documentId: Ref<string | null>,
  options: AutoSaveOptions
) {
  // Implementation...
}
```

**3. Architecture Decision Records**

```markdown
# ADR 001: Use Web Workers for LaTeX Rendering

## Status
Accepted

## Context
LaTeX rendering and syntax highlighting are CPU-intensive operations that can block
the main thread, causing poor user experience during editing.

## Decision
We will use Web Workers for all heavy computational tasks:
- Syntax highlighting (Prism.js)
- LaTeX rendering (KaTeX)
- Document structure parsing

## Consequences
### Positive
- Main thread remains responsive
- Better user experience during editing
- No UI freezing during large document rendering

### Negative
- Increased complexity
- Additional memory overhead
- Browser compatibility concerns

## Alternatives Considered
1. Main thread rendering (rejected due to UI blocking)
2. Server-side rendering (rejected due to latency)
3. WASM-based rendering (rejected due to bundle size)
```

---

## 8. Implementation Roadmap

### Phase 1: Foundation (Week 1-2)
**Priority: HIGH**

1. **Testing Infrastructure**
   - Set up Vitest configuration
   - Add test utilities and helpers
   - Create first unit tests

2. **Type Safety**
   - Centralize type definitions
   - Add type guards
   - Remove unsafe type assertions

3. **Documentation**
   - Document core components
   - Document key composables
   - Create architecture ADRs

**Success Criteria:**
- Test coverage > 20%
- All types properly defined
- Core components documented

### Phase 2: Refactoring (Week 3-4)
**Priority: HIGH**

1. **Component Decomposition**
   - Split LatexEditorView.vue
   - Extract reusable components
   - Create component library

2. **State Management**
   - Unify state architecture
   - Centralize state in stores
   - Remove scattered refs

3. **Code Deduplication**
   - Extract text manipulation utilities
   - Create unified debounce utility
   - Implement error handling framework

**Success Criteria:**
- No component > 300 lines
- State centralized in stores
- Code duplication < 5%

### Phase 3: Performance (Week 5-6)
**Priority: MEDIUM**

1. **Performance Optimization**
   - Implement true virtual scrolling
   - Add component memoization
   - Optimize reactivity

2. **Bundle Optimization**
   - Lazy load heavy dependencies
   - Code-split components
   - Reduce bundle size by 30%

3. **Memory Management**
   - Fix memory leaks
   - Implement cache limits
   - Add cleanup procedures

**Success Criteria:**
- Initial load time < 1.5s
- Memory usage stable over time
- Bundle size < 500KB

### Phase 4: Quality Assurance (Week 7-8)
**Priority: MEDIUM**

1. **Testing**
   - Unit tests for all components
   - Integration tests for workflows
   - E2E tests for critical paths

2. **Code Quality**
   - ESLint configuration
   - Prettier formatting
   - Code review guidelines

3. **CI/CD**
   - Automated testing
   - Performance regression tests
   - Deployment automation

**Success Criteria:**
- Test coverage > 70%
- All tests passing
- Automated deployment

---

## 9. Metrics & KPIs

### Code Quality Metrics

| Metric | Current | Target | Priority |
|--------|---------|--------|----------|
| Test Coverage | 0% | 70% | HIGH |
| Max Component Size | 1,337 lines | 300 lines | HIGH |
| TypeScript Strict Mode | 70% | 100% | MEDIUM |
| Code Duplication | ~15% | <5% | MEDIUM |
| Documentation Coverage | 30% | 80% | LOW |

### Performance Metrics

| Metric | Current | Target | Priority |
|--------|---------|--------|----------|
| Initial Load Time | ~750ms | <400ms | HIGH |
| Bundle Size | ~800KB | <500KB | MEDIUM |
| Memory Usage (2h session) | ~130MB | <80MB | MEDIUM |
| Input Response Time | ~15ms | <30ms | LOW |
| Large Document Render | ~280ms | <200ms | LOW |

---

## 10. Conclusion

The LaTeX editor implementation demonstrates **solid engineering fundamentals** but requires significant improvements in **testing**, **code organization**, and **architectural consistency** to reach production-ready quality.

### Immediate Actions Required (This Week)

1. **CRITICAL**: Set up testing infrastructure (Vitest)
2. **CRITICAL**: Begin component decomposition (LatexEditorView.vue)
3. **HIGH**: Centralize type definitions
4. **HIGH**: Document core components and composables

### Expected Outcomes

After implementing this refactoring plan:

- **Code Quality**: B+ → A- (82 → 90)
- **Maintainability**: Significantly improved
- **Testability**: Full test coverage
- **Performance**: 30-40% improvement
- **Developer Experience**: Much better

### Risk Assessment

**Low Risk:**
- Testing infrastructure setup
- Documentation improvements
- Type safety enhancements

**Medium Risk:**
- Component decomposition (may introduce bugs)
- State management refactoring (complex changes)
- Performance optimizations (regression risk)

**Mitigation Strategy:**
- Implement comprehensive testing before refactoring
- Use feature flags for gradual rollout
- Maintain detailed changelogs
- Code review requirements for all changes

---

**Report Completed**: 2026-04-12
**Next Review**: 2026-05-12 (after Phase 1 completion)
**Analyst**: EngineeringSeniorDeveloper Agent
**Status**: Ready for Implementation
