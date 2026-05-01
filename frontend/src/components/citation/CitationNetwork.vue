<!-- CitationNetwork.vue - Interactive citation network visualization
  Force-directed graph using SVG + manual simulation (no d3 dependency). -->
<template>
  <div class="citation-network" :class="{ 'is-loading': loading }">
    <div class="network-toolbar">
      <div class="filter-group">
        <label class="filter-label">Year Range</label>
        <el-slider v-model="yearRange" range :min="minYear" :max="maxYear" :step="1"
          size="small" class="year-slider" @change="applyFilters" />
      </div>
      <div class="filter-group">
        <label class="filter-label">Min Citations</label>
        <el-input-number v-model="minCitations" :min="0" :step="10" size="small"
          controls-position="right" class="citation-input" @change="applyFilters" />
      </div>
      <el-button size="small" @click="resetSimulation" :icon="RefreshIcon">Reset Layout</el-button>
    </div>

    <div v-if="loading" class="network-loading">
      <el-icon class="is-loading" :size="32"><Loading /></el-icon>
      <span>Loading citation network...</span>
    </div>
    <div v-if="!loading && filteredNodes.length === 0" class="network-empty">
      <p>No citation data available for this paper.</p>
    </div>

    <svg v-show="!loading && filteredNodes.length > 0" ref="svgRef" class="network-svg"
      @mousemove="onMouseMove" @mouseleave="hideTooltip">
      <defs>
        <marker id="arrow-cites" viewBox="0 0 10 6" refX="10" refY="3"
          markerWidth="8" markerHeight="6" orient="auto-start-reverse">
          <path d="M0,0 L10,3 L0,6 Z" fill="#667eea" />
        </marker>
        <marker id="arrow-cited-by" viewBox="0 0 10 6" refX="10" refY="3"
          markerWidth="8" markerHeight="6" orient="auto-start-reverse">
          <path d="M0,0 L10,3 L0,6 Z" fill="#e6a23c" />
        </marker>
      </defs>
      <g class="zoom-group" ref="zoomGroupRef">
        <line v-for="edge in visibleEdges" :key="`${edge.source}-${edge.target}`"
          :x1="nodePosition(edge.source)?.x ?? 0" :y1="nodePosition(edge.source)?.y ?? 0"
          :x2="nodePosition(edge.target)?.x ?? 0" :y2="nodePosition(edge.target)?.y ?? 0"
          class="network-edge" :class="`edge-${edge.type}`"
          :marker-end="`url(#arrow-${edge.type})`" />
        <g v-for="node in filteredNodes" :key="node.id" class="network-node"
          :class="{ 'is-center': node.id === centerPaper.id, 'is-hovered': hoveredNode === node.id }"
          :transform="`translate(${positions[node.id]?.x ?? 0}, ${positions[node.id]?.y ?? 0})`"
          @mouseenter="onNodeEnter(node)" @mouseleave="onNodeLeave" @click="onNodeClick(node)">
          <circle :r="nodeRadius(node)" :fill="nodeColor(node)"
            :stroke="node.id === centerPaper.id ? '#fff' : 'transparent'"
            :stroke-width="node.id === centerPaper.id ? 3 : 0" class="node-circle" />
          <text class="node-label" dy="-0.5em" text-anchor="middle">
            {{ truncateTitle(node.title) }}
          </text>
          <g v-if="hoveredNode === node.id && node.id !== centerPaper.id"
            class="node-import" @click.stop="onImportClick(node)">
            <rect x="12" y="-28" width="22" height="22" rx="4" fill="#67c23a" opacity="0.9" />
            <text x="23" y="-13" text-anchor="middle" fill="#fff" font-size="14" font-weight="bold">+</text>
          </g>
        </g>
      </g>
    </svg>

    <div v-if="tooltip.visible" class="network-tooltip" :style="tooltipStyle">
      <h4 class="tooltip-title">{{ tooltip.paper?.title }}</h4>
      <p class="tooltip-authors">{{ tooltip.paper?.authors }}</p>
      <div class="tooltip-meta">
        <span class="tooltip-year">{{ tooltip.paper?.year }}</span>
        <span class="tooltip-citations">{{ tooltip.paper?.citationCount }} citations</span>
      </div>
    </div>

    <div class="network-legend">
      <div class="legend-section">
        <span class="legend-heading">Year</span>
        <div class="legend-gradient" />
        <div class="legend-labels"><span>{{ minYear }}</span><span>{{ maxYear }}</span></div>
      </div>
      <div class="legend-section">
        <span class="legend-heading">Edges</span>
        <div class="legend-item"><span class="legend-line cites-line" /><span>Cites</span></div>
        <div class="legend-item"><span class="legend-line cited-by-line" /><span>Cited by</span></div>
      </div>
      <div class="legend-section"><span class="legend-heading">Node size = citation count</span></div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed, onMounted, onUnmounted, watch, nextTick } from 'vue'
import { Loading, Refresh as RefreshIcon } from '@element-plus/icons-vue'

interface PaperNode {
  id: string; title: string; authors: string
  year: number; citationCount: number; abstract?: string
}
interface CitationEdge { source: string; target: string; type: 'cites' | 'cited-by' }
interface SimNode { x: number; y: number; vx: number; vy: number }

const props = withDefaults(defineProps<{
  centerPaper: PaperNode; relatedPapers: PaperNode[]
  citations: CitationEdge[]; loading?: boolean
}>(), { loading: false })

const emit = defineEmits<{
  'navigate-paper': [paperId: string]
  'import-paper': [paperId: string]
}>()

const svgRef = ref<SVGSVGElement | null>(null)
const zoomGroupRef = ref<SVGGElement | null>(null)
const positions = reactive<Record<string, SimNode>>({})
const hoveredNode = ref<string | null>(null)
const yearRange = ref<[number, number]>([1900, new Date().getFullYear()])
const minCitations = ref(0)
let animFrameId = 0
let simAlpha = 1.0

const tooltip = reactive({ visible: false, paper: null as PaperNode | null, x: 0, y: 0 })
const pan = reactive({ x: 0, y: 0, scale: 1 })
let isPanning = false
let panStart = { x: 0, y: 0 }

// --- Computed ---
const allNodes = computed(() => [props.centerPaper, ...props.relatedPapers])
const minYear = computed(() => Math.min(...allNodes.value.map(n => n.year)))
const maxYear = computed(() => Math.max(...allNodes.value.map(n => n.year)))

const filteredNodes = computed(() => {
  const [yMin, yMax] = yearRange.value
  return allNodes.value.filter(n => n.year >= yMin && n.year <= yMax && n.citationCount >= minCitations.value)
})
const filteredNodeIds = computed(() => new Set(filteredNodes.value.map(n => n.id)))
const visibleEdges = computed(() =>
  props.citations.filter(e => filteredNodeIds.value.has(e.source) && filteredNodeIds.value.has(e.target))
)
const tooltipStyle = computed(() => ({ left: `${tooltip.x + 12}px`, top: `${tooltip.y - 10}px` }))

// --- Helpers ---
const nodeRadius = (node: PaperNode) => {
  if (node.id === props.centerPaper.id) return 22
  const maxC = Math.max(...allNodes.value.map(n => n.citationCount), 1)
  return 8 + (node.citationCount / maxC) * 14
}
const nodeColor = (node: PaperNode) => {
  if (node.id === props.centerPaper.id) return '#e6a23c'
  const span = maxYear.value - minYear.value || 1
  const t = (node.year - minYear.value) / span
  return `rgb(${Math.round(74 + t * 171)},${Math.round(124 + t * 34)},${Math.round(247 - t * 181)})`
}
const truncateTitle = (title: string) => title.length > 28 ? title.slice(0, 25) + '...' : title
const nodePosition = (id: string) => positions[id] ?? null

// --- Force Simulation (manual, no d3) ---
function initPositions() {
  const cx = 400, cy = 300
  positions[props.centerPaper.id] = { x: cx, y: cy, vx: 0, vy: 0 }
  props.relatedPapers.forEach((n, i) => {
    const angle = (2 * Math.PI * i) / props.relatedPapers.length
    const dist = 120 + Math.random() * 80
    positions[n.id] = { x: cx + Math.cos(angle) * dist, y: cy + Math.sin(angle) * dist, vx: 0, vy: 0 }
  })
  simAlpha = 1.0
}

function tickSimulation() {
  if (simAlpha < 0.001) return
  const cx = 400, cy = 300
  const nodes = filteredNodes.value, edges = visibleEdges.value
  const repulsion = 2000, attraction = 0.005, centerPull = 0.01

  for (const n of nodes) { const p = positions[n.id]; if (p) { p.vx = 0; p.vy = 0 } }

  // Repulsion
  for (let i = 0; i < nodes.length; i++) {
    for (let j = i + 1; j < nodes.length; j++) {
      const a = positions[nodes[i].id], b = positions[nodes[j].id]
      if (!a || !b) continue
      const dx = a.x - b.x, dy = a.y - b.y
      const dist = Math.sqrt(dx * dx + dy * dy) || 1
      const f = repulsion / (dist * dist), fx = (dx / dist) * f, fy = (dy / dist) * f
      a.vx += fx; a.vy += fy; b.vx -= fx; b.vy -= fy
    }
  }
  // Attraction
  for (const e of edges) {
    const a = positions[e.source], b = positions[e.target]
    if (!a || !b) continue
    const dx = b.x - a.x, dy = b.y - a.y
    const dist = Math.sqrt(dx * dx + dy * dy) || 1
    const f = dist * attraction
    a.vx += (dx / dist) * f; a.vy += (dy / dist) * f
    b.vx -= (dx / dist) * f; b.vy -= (dy / dist) * f
  }
  // Center gravity + velocity damping
  for (const n of nodes) {
    const p = positions[n.id]; if (!p) continue
    if (n.id === props.centerPaper.id) continue // pin center
    p.vx = (p.vx + (cx - p.x) * centerPull) * 0.6
    p.vy = (p.vy + (cy - p.y) * centerPull) * 0.6
    p.x = Math.max(20, Math.min(780, p.x + p.vx * simAlpha))
    p.y = Math.max(20, Math.min(580, p.y + p.vy * simAlpha))
  }
  simAlpha *= 0.995
}

function animationLoop() { tickSimulation(); if (simAlpha >= 0.001) animFrameId = requestAnimationFrame(animationLoop) }
function resetSimulation() { initPositions(); simAlpha = 1.0 }
function applyFilters() { simAlpha = Math.max(simAlpha, 0.3) }

// --- Interactions ---
function onNodeEnter(node: PaperNode) { hoveredNode.value = node.id; tooltip.paper = node; tooltip.visible = true }
function onNodeLeave() { hoveredNode.value = null; tooltip.visible = false }
function onMouseMove(e: MouseEvent) { tooltip.x = e.offsetX; tooltip.y = e.offsetY }
function hideTooltip() { tooltip.visible = false; hoveredNode.value = null }
function onNodeClick(node: PaperNode) { emit('navigate-paper', node.id) }
function onImportClick(node: PaperNode) { emit('import-paper', node.id) }

// --- Pan & Zoom ---
function applyTransform() {
  zoomGroupRef.value?.setAttribute('transform', `translate(${pan.x},${pan.y}) scale(${pan.scale})`)
}
function onWheel(e: WheelEvent) {
  e.preventDefault()
  const newScale = Math.min(3, Math.max(0.3, pan.scale * (e.deltaY > 0 ? 0.92 : 1.08)))
  const rect = svgRef.value!.getBoundingClientRect()
  const mx = e.clientX - rect.left, my = e.clientY - rect.top
  pan.x = mx - (mx - pan.x) * (newScale / pan.scale)
  pan.y = my - (my - pan.y) * (newScale / pan.scale)
  pan.scale = newScale; applyTransform()
}
function onPanStart(e: MouseEvent) {
  if ((e.target as Element).closest('.network-node')) return
  isPanning = true; panStart = { x: e.clientX - pan.x, y: e.clientY - pan.y }
}
function onPanMove(e: MouseEvent) { if (!isPanning) return; pan.x = e.clientX - panStart.x; pan.y = e.clientY - panStart.y; applyTransform() }
function onPanEnd() { isPanning = false }

function attachZoom() {
  const svg = svgRef.value; if (!svg) return
  svg.addEventListener('wheel', onWheel, { passive: false })
  svg.addEventListener('mousedown', onPanStart); svg.addEventListener('mousemove', onPanMove)
  svg.addEventListener('mouseup', onPanEnd); svg.addEventListener('mouseleave', onPanEnd)
}
function detachZoom() {
  const svg = svgRef.value; if (!svg) return
  svg.removeEventListener('wheel', onWheel)
  svg.removeEventListener('mousedown', onPanStart); svg.removeEventListener('mousemove', onPanMove)
  svg.removeEventListener('mouseup', onPanEnd); svg.removeEventListener('mouseleave', onPanEnd)
}

// --- Lifecycle ---
onMounted(() => { initPositions(); nextTick(() => { attachZoom(); applyTransform() }); animFrameId = requestAnimationFrame(animationLoop) })
onUnmounted(() => { cancelAnimationFrame(animFrameId); detachZoom() })
watch(() => [props.centerPaper, props.relatedPapers, props.citations], () => { resetSimulation() }, { deep: true })
</script>

<style scoped lang="scss">
.citation-network {
  position: relative; width: 100%;
  background: var(--el-bg-color, #fff); border-radius: 12px;
  border: 1px solid var(--el-border-color-lighter, #e4e7ed); overflow: hidden;
}
.network-toolbar {
  display: flex; align-items: center; gap: 16px; padding: 12px 20px;
  border-bottom: 1px solid var(--el-border-color-lighter, #e4e7ed); flex-wrap: wrap;
}
.filter-group { display: flex; align-items: center; gap: 8px; }
.filter-label { font-size: 13px; color: var(--el-text-color-secondary, #909399); white-space: nowrap; }
.year-slider { width: 160px; }
.citation-input { width: 120px; }

.network-loading, .network-empty {
  display: flex; flex-direction: column; align-items: center; justify-content: center;
  height: 400px; color: var(--el-text-color-secondary, #909399); gap: 12px;
}
.network-svg { width: 100%; height: 500px; cursor: grab; user-select: none; &:active { cursor: grabbing; } }
.network-edge {
  stroke-width: 1.5; opacity: 0.5; transition: opacity 0.2s;
  &.edge-cites { stroke: #667eea; } &.edge-cited-by { stroke: #e6a23c; }
}
.network-node {
  cursor: pointer; transition: filter 0.2s;
  &:hover .node-circle, &.is-hovered .node-circle { filter: brightness(1.2) drop-shadow(0 2px 6px rgba(0,0,0,0.25)); }
  &.is-center .node-circle { filter: drop-shadow(0 0 8px rgba(230,162,60,0.5)); }
}
.node-circle { transition: r 0.3s, filter 0.2s; }
.node-label { font-size: 9px; fill: var(--el-text-color-regular, #606266); pointer-events: none; font-weight: 500; }
.node-import { cursor: pointer; opacity: 0; animation: fade-in 0.15s ease forwards; }

.network-tooltip {
  position: absolute; background: var(--el-bg-color-overlay, #fff);
  border: 1px solid var(--el-border-color-lighter, #e4e7ed); border-radius: 8px;
  padding: 10px 14px; box-shadow: 0 4px 16px rgba(0,0,0,0.12);
  pointer-events: none; max-width: 300px; z-index: 10;
}
.tooltip-title { font-size: 13px; font-weight: 600; margin: 0 0 4px; color: var(--el-text-color-primary, #303133); }
.tooltip-authors { font-size: 12px; color: var(--el-text-color-secondary, #909399); margin: 0 0 6px; }
.tooltip-meta { display: flex; gap: 12px; font-size: 12px; }
.tooltip-year { color: #667eea; font-weight: 600; }
.tooltip-citations { color: #e6a23c; font-weight: 600; }

.network-legend {
  display: flex; gap: 24px; padding: 10px 20px;
  border-top: 1px solid var(--el-border-color-lighter, #e4e7ed);
  font-size: 11px; color: var(--el-text-color-secondary, #909399); flex-wrap: wrap;
}
.legend-section { display: flex; flex-direction: column; gap: 4px; }
.legend-heading { font-weight: 600; font-size: 11px; }
.legend-gradient { width: 100px; height: 10px; border-radius: 3px; background: linear-gradient(to right, #4a7cf7, #f59e42); }
.legend-labels { display: flex; justify-content: space-between; width: 100px; }
.legend-item { display: flex; align-items: center; gap: 6px; }
.legend-line { display: inline-block; width: 20px; height: 2px; border-radius: 1px; &.cites-line { background: #667eea; } &.cited-by-line { background: #e6a23c; } }

@keyframes fade-in { from { opacity: 0; transform: scale(0.8); } to { opacity: 1; transform: scale(1); } }

[data-theme='dark'] {
  .citation-network { background: var(--el-bg-color, #1a1a2e); border-color: rgba(255,255,255,0.08); }
  .network-toolbar { border-color: rgba(255,255,255,0.08); }
  .node-label { fill: rgba(255,255,255,0.7); }
  .network-tooltip { background: #252540; border-color: rgba(255,255,255,0.1); box-shadow: 0 4px 20px rgba(0,0,0,0.4); }
  .network-legend { border-color: rgba(255,255,255,0.08); }
}
</style>
