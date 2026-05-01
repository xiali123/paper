/**
 * PDF Annotation state management composable
 * Manages highlight and sticky-note annotations with localStorage persistence.
 */

import { ref, computed, watch } from 'vue'

export interface AnnotationRect {
  x: number
  y: number
  width: number
  height: number
}

export interface AnnotationPosition {
  x: number
  y: number
}

export interface Annotation {
  id: string
  type: 'highlight' | 'note'
  pageNumber: number
  rect?: AnnotationRect
  position?: AnnotationPosition
  content: string
  color: string
  createdAt: string
}

const STORAGE_PREFIX = 'pdf-annotations-'

function generateId(): string {
  return `ann_${Date.now()}_${Math.random().toString(36).slice(2, 9)}`
}

export function usePdfAnnotations() {
  const annotations = ref<Annotation[]>([])
  const currentDocId = ref<string>('')

  // --- Persistence ----------------------------------------------------------

  function loadAnnotations(docId: string) {
    currentDocId.value = docId
    try {
      const raw = localStorage.getItem(STORAGE_PREFIX + docId)
      annotations.value = raw ? JSON.parse(raw) : []
    } catch {
      annotations.value = []
    }
  }

  function saveAnnotations() {
    if (!currentDocId.value) return
    try {
      localStorage.setItem(
        STORAGE_PREFIX + currentDocId.value,
        JSON.stringify(annotations.value)
      )
    } catch (e) {
      console.warn('[usePdfAnnotations] Failed to persist annotations:', e)
    }
  }

  // Auto-save whenever annotations change
  watch(annotations, saveAnnotations, { deep: true })

  // --- CRUD -----------------------------------------------------------------

  function addAnnotation(
    type: Annotation['type'],
    pageNumber: number,
    data: Partial<Pick<Annotation, 'rect' | 'position' | 'content' | 'color'>>
  ): Annotation {
    const annotation: Annotation = {
      id: generateId(),
      type,
      pageNumber,
      content: data.content ?? '',
      color: data.color ?? (type === 'highlight' ? '#FFEB3B' : '#FF9800'),
      rect: data.rect,
      position: data.position,
      createdAt: new Date().toISOString(),
    }
    annotations.value.push(annotation)
    return annotation
  }

  function removeAnnotation(id: string) {
    const idx = annotations.value.findIndex((a) => a.id === id)
    if (idx !== -1) annotations.value.splice(idx, 1)
  }

  function updateAnnotation(id: string, patch: Partial<Annotation>) {
    const ann = annotations.value.find((a) => a.id === id)
    if (ann) {
      Object.assign(ann, patch)
    }
  }

  // --- Queries --------------------------------------------------------------

  function getAnnotationsForPage(pageNumber: number): Annotation[] {
    return annotations.value.filter((a) => a.pageNumber === pageNumber)
  }

  const totalAnnotations = computed(() => annotations.value.length)

  return {
    annotations,
    currentDocId,
    totalAnnotations,

    addAnnotation,
    removeAnnotation,
    updateAnnotation,
    loadAnnotations,
    saveAnnotations,
    getAnnotationsForPage,
  }
}
