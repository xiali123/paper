<template>
  <div class="pdf-annotation-layer" :class="{ 'panel-open': panelOpen }">
    <!-- Toolbar -->
    <div class="annotation-toolbar">
      <el-button-group size="small">
        <el-button
          :type="activeTool === 'highlight' ? 'primary' : 'default'"
          @click="activeTool = activeTool === 'highlight' ? null : 'highlight'"
        >
          <svg class="tool-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5" />
          </svg>
          Highlight
        </el-button>
        <el-button
          :type="activeTool === 'note' ? 'primary' : 'default'"
          @click="activeTool = activeTool === 'note' ? null : 'note'"
        >
          <svg class="tool-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <path d="M14 2H6a2 2 0 00-2 2v16a2 2 0 002 2h12a2 2 0 002-2V8z" />
            <polyline points="14 2 14 8 20 8" />
            <line x1="16" y1="13" x2="8" y2="13" />
            <line x1="16" y1="17" x2="8" y2="17" />
          </svg>
          Note
        </el-button>
      </el-button-group>

      <el-color-picker
        v-model="activeColor"
        size="small"
        :predefine="colorPresets"
        style="margin-left: 8px"
      />

      <el-button
        size="small"
        :type="panelOpen ? 'primary' : 'default'"
        style="margin-left: 8px"
        @click="panelOpen = !panelOpen"
      >
        <svg class="tool-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <line x1="8" y1="6" x2="21" y2="6" /><line x1="8" y1="12" x2="21" y2="12" />
          <line x1="8" y1="18" x2="21" y2="18" /><line x1="3" y1="6" x2="3.01" y2="6" />
          <line x1="3" y1="12" x2="3.01" y2="12" /><line x1="3" y1="18" x2="3.01" y2="18" />
        </svg>
        {{ totalAnnotations }}
      </el-button>
    </div>

    <!-- Canvas overlay for drawing highlights -->
    <div
      class="annotation-canvas-wrapper"
      @mousedown="onMouseDown"
      @mousemove="onMouseMove"
      @mouseup="onMouseUp"
    >
      <!-- Rendered highlight rects -->
      <div
        v-for="ann in pageAnnotations"
        :key="ann.id"
        class="annotation-item"
        :class="ann.type"
        :style="annotationStyle(ann)"
        @click.stop="selectAnnotation(ann)"
      >
        <span v-if="ann.type === 'note'" class="note-icon">
          <svg viewBox="0 0 24 24" fill="currentColor" width="16" height="16">
            <path d="M14 2H6a2 2 0 00-2 2v16a2 2 0 002 2h12a2 2 0 002-2V8l-6-6zM6 20V4h7v5h5v11H6z" />
          </svg>
        </span>
      </div>

      <!-- Active drawing rect -->
      <div
        v-if="drawingRect"
        class="drawing-rect"
        :style="{
          left: drawingRect.x + 'px',
          top: drawingRect.y + 'px',
          width: drawingRect.width + 'px',
          height: drawingRect.height + 'px',
          backgroundColor: activeColor + '40',
          border: `2px solid ${activeColor}`,
        }"
      />
    </div>

    <!-- Note popup editor -->
    <div
      v-if="notePopup.visible"
      class="note-popup"
      :style="{ left: notePopup.x + 'px', top: notePopup.y + 'px' }"
    >
      <div class="note-popup-header">
        <span class="note-popup-title">Sticky Note</span>
        <button class="note-popup-close" @click="closeNotePopup">&times;</button>
      </div>
      <textarea
        ref="noteInputRef"
        v-model="notePopup.text"
        class="note-popup-input"
        placeholder="Type your note..."
        rows="3"
        @keydown.enter.ctrl="saveNotePopup"
      />
      <div class="note-popup-actions">
        <el-button size="small" type="primary" @click="saveNotePopup">Save</el-button>
        <el-button size="small" @click="closeNotePopup">Cancel</el-button>
      </div>
    </div>

    <!-- Side panel: annotation list -->
    <transition name="slide-panel">
      <div v-if="panelOpen" class="annotation-panel">
        <div class="panel-header">
          <span class="panel-title">Annotations</span>
          <button class="panel-close" @click="panelOpen = false">&times;</button>
        </div>
        <div class="panel-body">
          <div v-if="annotations.length === 0" class="panel-empty">
            No annotations yet. Use the toolbar to highlight text or add notes.
          </div>
          <div
            v-for="ann in annotations"
            :key="ann.id"
            class="panel-item"
            :class="{ selected: selectedId === ann.id }"
            @click="selectAnnotation(ann)"
          >
            <div class="panel-item-header">
              <span class="panel-item-badge" :style="{ backgroundColor: ann.color + '60' }">
                {{ ann.type === 'highlight' ? 'H' : 'N' }}
              </span>
              <span class="panel-item-page">Page {{ ann.pageNumber }}</span>
              <span class="panel-item-time">{{ formatTime(ann.createdAt) }}</span>
              <button class="panel-item-delete" @click.stop="removeAnnotation(ann.id)">
                <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="14" height="14">
                  <polyline points="3 6 5 6 21 6" /><path d="M19 6l-1 14a2 2 0 01-2 2H8a2 2 0 01-2-2L5 6" />
                  <path d="M10 11v6" /><path d="M14 11v6" />
                </svg>
              </button>
            </div>
            <div v-if="ann.type === 'note' && ann.content" class="panel-item-content">
              {{ ann.content }}
            </div>
            <div v-if="ann.type === 'highlight'" class="panel-item-content">
              <span class="color-dot" :style="{ backgroundColor: ann.color }" />
              Highlight
            </div>
          </div>
        </div>
      </div>
    </transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, nextTick } from 'vue'
import { usePdfAnnotations } from '@/composables/usePdfAnnotations'
import type { Annotation } from '@/composables/usePdfAnnotations'

interface Props {
  pdfDocument: any
  pageCount: number
  /** Unique document identifier for localStorage persistence */
  documentId?: string
  /** Currently visible page (1-based) */
  currentPage?: number
  /** Viewport scale factor so annotation coords match the rendered PDF */
  scale?: number
  /** Bounding rect of the PDF canvas container, for coordinate mapping */
  containerRect?: DOMRect
}

const props = withDefaults(defineProps<Props>(), {
  documentId: 'default',
  currentPage: 1,
  scale: 1,
})

const emit = defineEmits<{
  'annotations-change': [annotations: Annotation[]]
}>()

// ---- State ----------------------------------------------------------------

const {
  annotations,
  totalAnnotations,
  addAnnotation,
  removeAnnotation,
  updateAnnotation,
  loadAnnotations,
  getAnnotationsForPage,
} = usePdfAnnotations()

const activeTool = ref<'highlight' | 'note' | null>(null)
const activeColor = ref('#FFEB3B')
const panelOpen = ref(false)
const selectedId = ref<string | null>(null)

const colorPresets = [
  '#FFEB3B', '#FF9800', '#F44336', '#E91E63',
  '#9C27B0', '#2196F3', '#4CAF50', '#00BCD4',
]

// Drawing state
const isDrawing = ref(false)
const drawStart = ref({ x: 0, y: 0 })
const drawingRect = ref<{ x: number; y: number; width: number; height: number } | null>(null)

// Note popup state
const notePopup = ref<{ visible: boolean; x: number; y: number; text: string; editId: string | null }>({
  visible: false, x: 0, y: 0, text: '', editId: null,
})
const noteInputRef = ref<HTMLTextAreaElement>()

// ---- Computed -------------------------------------------------------------

const pageAnnotations = computed(() => getAnnotationsForPage(props.currentPage))

// Emit on changes
watch(annotations, (val) => emit('annotations-change', [...val]), { deep: true })

// Load when document changes
watch(() => props.documentId, (id) => { if (id) loadAnnotations(id) }, { immediate: true })

// ---- Drawing helpers ------------------------------------------------------

function canvasCoords(event: MouseEvent) {
  const el = (event.currentTarget as HTMLElement)
  const rect = el.getBoundingClientRect()
  return { x: event.clientX - rect.left, y: event.clientY - rect.top }
}

function onMouseDown(e: MouseEvent) {
  if (!activeTool.value) return
  const pos = canvasCoords(e)

  if (activeTool.value === 'note') {
    openNotePopup(pos.x, pos.y)
    return
  }

  isDrawing.value = true
  drawStart.value = pos
  drawingRect.value = { x: pos.x, y: pos.y, width: 0, height: 0 }
}

function onMouseMove(e: MouseEvent) {
  if (!isDrawing.value) return
  const pos = canvasCoords(e)
  const x = Math.min(drawStart.value.x, pos.x)
  const y = Math.min(drawStart.value.y, pos.y)
  const w = Math.abs(pos.x - drawStart.value.x)
  const h = Math.abs(pos.y - drawStart.value.y)
  drawingRect.value = { x, y, width: w, height: h }
}

function onMouseUp() {
  if (!isDrawing.value || !drawingRect.value) { isDrawing.value = false; return }
  const r = drawingRect.value
  isDrawing.value = false
  drawingRect.value = null

  // Ignore tiny selections (accidental clicks)
  if (r.width < 5 || r.height < 5) return

  const normX = r.x / props.scale
  const normY = r.y / props.scale
  const normW = r.width / props.scale
  const normH = r.height / props.scale

  addAnnotation('highlight', props.currentPage, {
    rect: { x: normX, y: normY, width: normW, height: normH },
    color: activeColor.value,
  })
}

// ---- Note popup -----------------------------------------------------------

function openNotePopup(x: number, y: number, existingId?: string) {
  const existing = existingId ? annotations.value.find((a) => a.id === existingId) : null
  notePopup.value = {
    visible: true,
    x,
    y,
    text: existing?.content ?? '',
    editId: existingId ?? null,
  }
  nextTick(() => noteInputRef.value?.focus())
}

function saveNotePopup() {
  if (!notePopup.value.text.trim() && !notePopup.value.editId) {
    closeNotePopup()
    return
  }
  const normX = notePopup.value.x / props.scale
  const normY = notePopup.value.y / props.scale

  if (notePopup.value.editId) {
    updateAnnotation(notePopup.value.editId, { content: notePopup.value.text })
  } else {
    addAnnotation('note', props.currentPage, {
      position: { x: normX, y: normY },
      content: notePopup.value.text,
      color: activeColor.value,
    })
  }
  closeNotePopup()
}

function closeNotePopup() {
  notePopup.value.visible = false
  notePopup.value.text = ''
  notePopup.value.editId = null
}

// ---- Selection & display --------------------------------------------------

function selectAnnotation(ann: Annotation) {
  selectedId.value = ann.id
  if (ann.type === 'note') {
    openNotePopup(ann.position!.x * props.scale, ann.position!.y * props.scale, ann.id)
  }
}

function annotationStyle(ann: Annotation): Record<string, string> {
  if (ann.type === 'highlight' && ann.rect) {
    return {
      left: ann.rect.x * props.scale + 'px',
      top: ann.rect.y * props.scale + 'px',
      width: ann.rect.width * props.scale + 'px',
      height: ann.rect.height * props.scale + 'px',
      backgroundColor: ann.color + '40',
      borderLeft: `3px solid ${ann.color}`,
    }
  }
  if (ann.type === 'note' && ann.position) {
    return {
      left: ann.position.x * props.scale + 'px',
      top: ann.position.y * props.scale + 'px',
    }
  }
  return {}
}

function formatTime(iso: string): string {
  const d = new Date(iso)
  const pad = (n: number) => String(n).padStart(2, '0')
  return `${pad(d.getMonth() + 1)}/${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}`
}
</script>

<style scoped lang="scss">
$panel-width: 280px;
$toolbar-height: 40px;

.pdf-annotation-layer {
  position: relative;
  width: 100%;
  height: 100%;
}

/* Toolbar */
.annotation-toolbar {
  display: flex;
  align-items: center;
  padding: 4px 8px;
  height: $toolbar-height;
  border-bottom: 1px solid var(--el-border-color-lighter);
  background: var(--el-bg-color);
}

.tool-icon {
  width: 14px;
  height: 14px;
  margin-right: 4px;
  vertical-align: middle;
}

/* Canvas overlay */
.annotation-canvas-wrapper {
  position: absolute;
  top: $toolbar-height;
  left: 0;
  right: 0;
  bottom: 0;
  cursor: crosshair;
}

/* Highlight items */
.annotation-item {
  position: absolute;
  transition: opacity 0.15s ease;

  &.highlight {
    pointer-events: auto;
    border-radius: 2px;
    cursor: pointer;

    &:hover {
      opacity: 0.9;
    }
  }

  &.note {
    pointer-events: auto;
    cursor: pointer;
  }
}

.note-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  border-radius: 50%;
  background: #ff9800;
  color: #fff;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.25);
  font-size: 12px;
  transition: transform 0.15s ease;

  &:hover {
    transform: scale(1.15);
  }
}

/* Drawing rect */
.drawing-rect {
  position: absolute;
  border-radius: 2px;
  pointer-events: none;
}

/* Note popup */
.note-popup {
  position: absolute;
  width: 220px;
  background: var(--el-bg-color-overlay);
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.15);
  z-index: 20;
  overflow: hidden;
}

.note-popup-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 6px 10px;
  background: var(--el-fill-color-light);
  border-bottom: 1px solid var(--el-border-color-lighter);
}

.note-popup-title {
  font-size: 12px;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.note-popup-close {
  background: none;
  border: none;
  font-size: 16px;
  color: var(--el-text-color-secondary);
  cursor: pointer;
  line-height: 1;

  &:hover {
    color: var(--el-text-color-primary);
  }
}

.note-popup-input {
  display: block;
  width: 100%;
  padding: 8px 10px;
  border: none;
  outline: none;
  resize: vertical;
  font-family: inherit;
  font-size: 13px;
  color: var(--el-text-color-regular);
  background: transparent;
}

.note-popup-actions {
  display: flex;
  justify-content: flex-end;
  gap: 6px;
  padding: 6px 10px;
  border-top: 1px solid var(--el-border-color-lighter);
}

/* Side panel */
.annotation-panel {
  position: absolute;
  top: 0;
  right: 0;
  width: $panel-width;
  height: 100%;
  background: var(--el-bg-color-overlay);
  border-left: 1px solid var(--el-border-color-lighter);
  box-shadow: -4px 0 16px rgba(0, 0, 0, 0.08);
  z-index: 30;
  display: flex;
  flex-direction: column;
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 10px 12px;
  border-bottom: 1px solid var(--el-border-color-lighter);
}

.panel-title {
  font-size: 14px;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.panel-close {
  background: none;
  border: none;
  font-size: 18px;
  color: var(--el-text-color-secondary);
  cursor: pointer;
  line-height: 1;

  &:hover {
    color: var(--el-text-color-primary);
  }
}

.panel-body {
  flex: 1;
  overflow-y: auto;
  padding: 8px;
}

.panel-empty {
  padding: 24px 12px;
  text-align: center;
  color: var(--el-text-color-secondary);
  font-size: 13px;
  line-height: 1.5;
}

.panel-item {
  padding: 8px 10px;
  margin-bottom: 6px;
  border-radius: 6px;
  border: 1px solid var(--el-border-color-lighter);
  background: var(--el-fill-color-blank);
  cursor: pointer;
  transition: background 0.15s ease, border-color 0.15s ease;

  &:hover {
    border-color: var(--el-border-color);
  }

  &.selected {
    border-color: var(--el-color-primary);
    background: var(--el-color-primary-light-9);
  }
}

.panel-item-header {
  display: flex;
  align-items: center;
  gap: 6px;
}

.panel-item-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 20px;
  height: 20px;
  border-radius: 4px;
  font-size: 11px;
  font-weight: 700;
  color: var(--el-text-color-primary);
  flex-shrink: 0;
}

.panel-item-page {
  font-size: 12px;
  color: var(--el-text-color-regular);
  font-weight: 500;
}

.panel-item-time {
  font-size: 11px;
  color: var(--el-text-color-secondary);
  margin-left: auto;
}

.panel-item-delete {
  background: none;
  border: none;
  padding: 2px;
  color: var(--el-text-color-secondary);
  cursor: pointer;
  flex-shrink: 0;
  display: inline-flex;

  &:hover {
    color: var(--el-color-danger);
  }
}

.panel-item-content {
  margin-top: 4px;
  font-size: 12px;
  color: var(--el-text-color-secondary);
  line-height: 1.4;
  word-break: break-word;
}

.color-dot {
  display: inline-block;
  width: 10px;
  height: 10px;
  border-radius: 50%;
  vertical-align: middle;
  margin-right: 4px;
}

/* Slide panel transition */
.slide-panel-enter-active,
.slide-panel-leave-active {
  transition: transform 0.25s cubic-bezier(0.16, 1, 0.3, 1);
}

.slide-panel-enter-from,
.slide-panel-leave-to {
  transform: translateX(100%);
}

/* Dark mode overrides */
[data-theme="dark"] .annotation-toolbar,
.dark-mode .annotation-toolbar {
  background: var(--el-bg-color-overlay);
}

[data-theme="dark"] .note-popup,
.dark-mode .note-popup {
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.4);
}

[data-theme="dark"] .annotation-panel,
.dark-mode .annotation-panel {
  box-shadow: -4px 0 24px rgba(0, 0, 0, 0.3);
}

[data-theme="dark"] .panel-item,
.dark-mode .panel-item {
  border-color: var(--el-border-color);

  &.selected {
    background: rgba(64, 158, 255, 0.12);
  }
}
</style>
