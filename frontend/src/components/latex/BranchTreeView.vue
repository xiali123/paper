<template>
  <div class="branch-tree-view" ref="containerRef">
    <!-- 工具栏 -->
    <div class="toolbar">
      <div class="toolbar-left">
        <el-radio-group v-model="viewMode" size="small">
          <el-radio-button value="tree">树形图</el-radio-button>
          <el-radio-button value="graph">有向图</el-radio-button>
          <el-radio-button value="timeline">时间轴</el-radio-button>
        </el-radio-group>
        <el-divider direction="vertical" />
        <el-button-group size="small">
          <el-button :icon="ZoomOut" @click="zoomOut" />
          <el-button @click="zoomReset">100%</el-button>
          <el-button :icon="ZoomIn" @click="zoomIn" />
        </el-button-group>
        <el-button :icon="FullScreen" @click="fitView" size="small">适配</el-button>
      </div>
      <div class="toolbar-right">
        <el-switch v-model="showLabels" size="small" active-text="标签" />
        <el-switch v-model="showMeta" size="small" active-text="详情" />
      </div>
    </div>

    <!-- SVG容器 -->
    <div class="svg-container" ref="svgContainerRef" :style="{ transform: `scale(${zoomLevel}) translate(${panX}px, ${panY}px)` }">
      <svg
        ref="svgRef"
        class="tree-svg"
        :width="canvasSize.width"
        :height="canvasSize.height"
        @mousedown="startPan"
        @mousemove="onPan"
        @mouseup="endPan"
        @mouseleave="endPan"
        @wheel.prevent="onWheel"
      >
        <defs>
          <!-- 节点阴影 -->
          <filter id="shadow" x="-50%" y="-50%" width="200%" height="200%">
            <feDropShadow dx="0" dy="2" stdDeviation="2" flood-opacity="0.2"/>
          </filter>

          <!-- 发光效果 -->
          <filter id="glow">
            <feGaussianBlur stdDeviation="2.5" result="coloredBlur"/>
            <feMerge>
              <feMergeNode in="coloredBlur"/>
              <feMergeNode in="SourceGraphic"/>
            </feMerge>
          </filter>

          <!-- 渐变 -->
          <linearGradient id="grad-main" x1="0%" y1="0%" x2="100%" y2="100%">
            <stop offset="0%" stop-color="#667eea"/>
            <stop offset="100%" stop-color="#764ba2"/>
          </linearGradient>

          <!-- 分支颜色渐变 -->
          <linearGradient v-for="color in branchColors" :key="color.name" :id="`grad-${color.name}`">
            <stop offset="0%" :stop-color="color.color"/>
            <stop offset="100%" :stop-color="lightenColor(color.color, 20)"/>
          </linearGradient>

          <!-- 箭头 -->
          <marker id="arrow-main" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto">
            <path d="M0,0 L0,6 L9,3 z" fill="#667eea"/>
          </marker>
          <marker v-for="color in branchColors" :key="`arrow-${color.name}`"
                  :id="`arrow-${color.name}`" markerWidth="10" markerHeight="10" refX="8" refY="3" orient="auto">
            <path d="M0,0 L0,6 L9,3 z" :fill="color.color"/>
          </marker>
        </defs>

        <!-- 背景网格 -->
        <g class="grid" v-if="showGrid">
          <pattern id="smallGrid" width="20" height="20" patternUnits="userSpaceOnUse">
            <circle cx="1" cy="1" r="1" fill="#e0e0e0"/>
          </pattern>
          <rect width="100%" height="100%" fill="url(#smallGrid)" opacity="0.5"/>
        </g>

        <!-- 分支泳道背景 -->
        <g class="swimlanes" v-if="viewMode === 'tree'">
          <rect
            v-for="lane in swimlanes"
            :key="lane.name"
            :x="lane.x"
            :y="0"
            :width="lane.width"
            :height="canvasSize.height"
            :fill="lane.color"
            opacity="0.03"
          />
          <text
            v-for="lane in swimlanes"
            :key="`label-${lane.name}`"
            :x="lane.x + 10"
            :y="20"
            :fill="lane.color"
            font-size="12"
            font-weight="bold"
            opacity="0.5"
          >{{ lane.displayName }}</text>
        </g>

        <!-- 时间轴模式的水平时间线 -->
        <g class="timeline-axis" v-if="viewMode === 'timeline'">
          <!-- 为每个分支绘制水平泳道线 -->
          <g v-for="(branch, index) in branchColors" :key="`timeline-${branch.name}`">
            <line
              :x1="100"
              :y1="60 + index * TIMELINE_LANE_HEIGHT + TIMELINE_LANE_HEIGHT / 2"
              :x2="canvasSize.width - 50"
              :y2="60 + index * TIMELINE_LANE_HEIGHT + TIMELINE_LANE_HEIGHT / 2"
              :stroke="branch.color"
              stroke-width="2"
              opacity="0.3"
            />
            <text
              :x="canvasSize.width - 45"
              :y="60 + index * TIMELINE_LANE_HEIGHT + TIMELINE_LANE_HEIGHT / 2 + 4"
              :fill="branch.color"
              font-size="11"
              font-weight="600"
              text-anchor="end"
            >{{ branch.displayName }}</text>
          </g>

          <!-- 时间刻度 -->
          <g v-for="tick in timelineTicks" :key="`tick-${tick.label}`">
            <line
              :x1="tick.x"
              :y1="40"
              :x2="tick.x"
              :y2="canvasSize.height - 40"
              stroke="#e0e0e0"
              stroke-width="1"
              stroke-dasharray="4,4"
            />
            <text
              :x="tick.x"
              :y="35"
              text-anchor="middle"
              font-size="10"
              fill="#909399"
            >{{ tick.label }}</text>
          </g>
        </g>

        <!-- 有向图模式的层级线 -->
        <g class="graph-levels" v-if="viewMode === 'graph'">
          <g v-for="level in graphLevels" :key="`level-${level}`">
            <line
              :x1="50"
              :y1="80 + level * GRAPH_ROW_HEIGHT"
              :x2="canvasSize.width - 50"
              :y2="80 + level * GRAPH_ROW_HEIGHT"
              stroke="#e0e0e0"
              stroke-width="1"
              stroke-dasharray="5,5"
            />
            <text
              :x="45"
              :y="80 + level * GRAPH_ROW_HEIGHT + 4"
              text-anchor="end"
              font-size="10"
              fill="#909399"
            >L{{ level }}</text>
          </g>
        </g>

        <!-- 连接线组 -->
        <g class="connections">
          <!-- 父子连接 -->
          <path
            v-for="conn in parentChildConnections"
            :key="`conn-${conn.from.id}-${conn.to.id}`"
            :d="conn.path"
            :stroke="conn.color"
            stroke-width="2"
            fill="none"
            :marker-end="conn.markerEnd"
            class="connection parent-child"
            :class="{ 'is-merge': conn.isMerge }"
          />

          <!-- 合并连接（特殊样式） -->
          <path
            v-for="conn in mergeConnections"
            :key="`merge-${conn.from.id}-${conn.to.id}`"
            :d="conn.path"
            stroke="#67C23A"
            stroke-width="2.5"
            fill="none"
            stroke-dasharray="5,3"
            class="connection merge"
          />
        </g>

        <!-- 节点组 -->
        <g class="nodes">
          <g
            v-for="node in laidOutNodes"
            :key="node.id"
            :transform="`translate(${node.x}, ${node.y})`"
            class="node"
            :class="{
              'is-selected': selectedVersion?.id === node.id,
              'is-comparing': comparingVersions.has(node.id),
              'is-merged': node.isMerged,
              'is-auto-save': node.isAutoSave,
              'is-branch-point': node.isBranchPoint,
              'is-merge-point': node.isMergePoint
            }"
            @click="handleNodeClick(node)"
            @mouseenter="showTooltip(node, $event)"
            @mouseleave="hideTooltip"
          >
            <!-- 选中光环 -->
            <circle
              v-if="selectedVersion?.id === node.id"
              :r="nodeRadius + 8"
              :fill="getNodeColor(node)"
              opacity="0.15"
              class="selection-halo"
            >
              <animate
                attributeName="r"
                :values="`${nodeRadius + 8};${nodeRadius + 12};${nodeRadius + 8}`"
                dur="2s"
                repeatCount="indefinite"
              />
              <animate
                attributeName="opacity"
                values="0.15;0.05;0.15"
                dur="2s"
                repeatCount="indefinite"
              />
            </circle>

            <!-- 节点主体 -->
            <circle
              :r="getNodeRadius(node)"
              :fill="getNodeFill(node)"
              :stroke="getNodeStroke(node)"
              :stroke-width="getNodeStrokeWidth(node)"
              class="node-circle"
            />

            <!-- 节点图标/文字 -->
            <g class="node-content" pointer-events="none">
              <!-- 合并点 -->
              <g v-if="node.isMergePoint">
                <path d="M-5,-3 L0,3 L5,-3" stroke="#fff" stroke-width="2" fill="none" stroke-linecap="round" stroke-linejoin="round"/>
                <circle cx="0" cy="3" r="1.5" fill="#fff"/>
              </g>
              <!-- 分支点 -->
              <g v-else-if="node.isBranchPoint">
                <circle cx="0" cy="0" r="2" fill="#fff"/>
                <path d="M-6,0 L6,0 M0,-6 L0,6" stroke="#fff" stroke-width="1.5" stroke-linecap="round"/>
              </g>
              <!-- 普通点 -->
              <circle v-else-if="!node.isAutoSave" r="2.5" fill="#fff"/>
            </g>

            <!-- 标签 -->
            <g v-if="showLabels" class="node-label" transform="translate(0, 24)">
              <foreignObject :x="-75" :y="0" width="150" height="50">
                <div class="label-content" :class="`label-${node.branchName}`">
                  <div class="label-summary">{{ node.summary || '未命名版本' }}</div>
                  <div v-if="showMeta" class="label-meta">
                    <span class="label-author">{{ node.author }}</span>
                    <span class="label-time">{{ formatTimeRelative(node.timestamp) }}</span>
                  </div>
                </div>
              </foreignObject>
            </g>

            <!-- 统计徽章 -->
            <g v-if="showMeta && node.changeCount > 0" transform="translate(12, -12)">
              <rect :x="-10" :y="-8" :width="20" height="16" rx="4" fill="#67C23A"/>
              <text x="0" y="4" text-anchor="middle" font-size="10" fill="#fff" font-weight="bold">
                +{{ node.changeCount }}
              </text>
            </g>
          </g>
        </g>
      </svg>
    </div>

    <!-- 工具提示 -->
    <transition name="el-fade-in">
      <div
        v-if="tooltipNode"
        class="tooltip"
        :style="{
          left: tooltipPos.x + 'px',
          top: tooltipPos.y + 'px'
        }"
      >
        <div class="tooltip-header">
          <el-tag :type="tooltipNode.branchName === 'main' ? '' : 'success'" size="small">
            {{ tooltipNode.branchName }}
          </el-tag>
          <span class="tooltip-time">{{ formatTime(tooltipNode.timestamp) }}</span>
        </div>
        <div class="tooltip-body">
          <div class="tooltip-summary">{{ tooltipNode.summary || '无摘要' }}</div>
          <div class="tooltip-info">
            <div class="info-item">
              <span class="info-label">作者:</span>
              <span class="info-value">{{ tooltipNode.author }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">内容:</span>
              <span class="info-value">{{ tooltipNode.totalLines }} 行</span>
            </div>
            <div v-if="tooltipNode.changeCount > 0" class="info-item">
              <span class="info-label">变更:</span>
              <span class="info-value change-add">+{{ tooltipNode.changeCount }}</span>
            </div>
            <div class="info-item">
              <span class="info-label">位置:</span>
              <span class="info-value">v{{ tooltipNode.position }}</span>
            </div>
          </div>
        </div>
      </div>
    </transition>

    <!-- 图例 -->
    <div class="legend" v-if="showLegend">
      <div class="legend-title">图例</div>
      <div class="legend-items">
        <div class="legend-item" @click="toggleFilter('main')">
          <span class="legend-dot main"></span>
          <span>主分支</span>
        </div>
        <div class="legend-item" @click="toggleFilter('branch')">
          <span class="legend-dot branch"></span>
          <span>特性分支</span>
        </div>
        <div class="legend-item" @click="toggleFilter('auto-save')">
          <span class="legend-dot auto-save"></span>
          <span>自动保存</span>
        </div>
        <div class="legend-item" @click="toggleFilter('merged')">
          <span class="legend-dot merged"></span>
          <span>已合并</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from 'vue'
import { ZoomIn, ZoomOut, FullScreen } from '@element-plus/icons-vue'
import type { FrontendLatexVersionNode } from '@/api/adapters/latexAdapter'

const props = defineProps<{
  versions: FrontendLatexVersionNode[]
  selectedVersion?: FrontendLatexVersionNode | null
}>()

const emit = defineEmits<{
  (e: 'select', version: FrontendLatexVersionNode): void
  (e: 'compare', versions: FrontendLatexVersionNode[]): void
}>()

// 布局常量
const NODE_RADIUS = 12
const LANE_WIDTH = 180
const LANE_SPACING = 20
const TIME_COLUMN_WIDTH = 120
const ROW_SPACING = 70
const TIMELINE_LANE_HEIGHT = 80
const TIMELINE_SPACING_X = 150
const GRAPH_ROW_HEIGHT = 100
const GRAPH_COLUMN_WIDTH = 200

// 状态
const containerRef = ref<HTMLElement>()
const svgContainerRef = ref<HTMLElement>()
const svgRef = ref<SVGSVGElement>()
const viewMode = ref<'tree' | 'graph' | 'timeline'>('tree')
const zoomLevel = ref(1)
const panX = ref(0)
const panY = ref(0)
const isPanning = ref(false)
const panStart = ref({ x: 0, y: 0 })
const showLabels = ref(true)
const showMeta = ref(true)
const showGrid = ref(true)
const showLegend = ref(true)
const comparingVersions = ref<Set<string>>(new Set())

// 工具提示
const tooltipNode = ref<(FrontendLatexVersionNode & { x: number; y: number }) | null>(null)
const tooltipPos = ref({ x: 0, y: 0 })

// 分支颜色
const branchColors = computed(() => {
  const colorMap = new Map<string, string>()
  const colors = [
    { name: 'main', color: '#667eea' },
    { name: 'develop', color: '#f093fb' },
    { name: 'feature', color: '#4facfe' },
    { name: 'hotfix', color: '#fa709a' },
    { name: 'release', color: '#fee140' }
  ]

  const branches = [...new Set(props.versions.map(v => v.branchName))]
  return branches.map((name, i) => ({
    name,
    color: colorMap.get(name) || colors[i % colors.length].color,
    displayName: name === 'main' ? '主分支' : name
  }))
})

// 泳道（分支垂直区域）- 仅在树形图模式下使用
const swimlanes = computed(() => {
  if (viewMode.value !== 'tree') return []
  return branchColors.value.map((branch, i) => ({
    ...branch,
    x: 80 + i * LANE_WIDTH,
    width: LANE_WIDTH - LANE_SPACING
  }))
})

// 核心布局算法 - 根据视图模式组织节点
const laidOutNodes = computed(() => {
  if (props.versions.length === 0) return []

  if (viewMode.value === 'timeline') {
    return layoutTimelineNodes()
  } else if (viewMode.value === 'graph') {
    return layoutGraphNodes()
  } else {
    return layoutTreeNodes()
  }
})

// 树形图布局 - 分支泳道
const layoutTreeNodes = () => {
  const nodes: Array<FrontendLatexVersionNode & {
    x: number
    y: number
    branchIndex: number
    timeIndex: number
    isBranchPoint: boolean
    isMergePoint: boolean
  }> = []

  // 按分支分组
  const branchGroups = new Map<string, FrontendLatexVersionNode[]>()
  props.versions.forEach(v => {
    if (!branchGroups.has(v.branchName)) {
      branchGroups.set(v.branchName, [])
    }
    branchGroups.get(v.branchName)!.push(v)
  })

  // 对每个分支按时间排序
  branchGroups.forEach((versions, branchName) => {
    versions.sort((a, b) =>
      new Date(a.timestamp).getTime() - new Date(b.timestamp).getTime()
    )
  })

  const branchIndexMap = new Map<string, number>()
  branchColors.value.forEach((b, i) => branchIndexMap.set(b.name, i))

  // 时间列索引
  const timeColumnMap = new Map<string, number>()
  let currentTimeColumn = 0

  branchGroups.forEach((versions, branchName) => {
    const branchIndex = branchIndexMap.get(branchName)!
    const laneX = 80 + branchIndex * LANE_WIDTH + LANE_WIDTH / 2

    versions.forEach((v, idx) => {
      // 计算时间列（简化：使用索引）
      if (!timeColumnMap.has(v.timestamp as unknown as string)) {
        timeColumnMap.set(v.timestamp as unknown as string, currentTimeColumn++)
      }
      const timeIndex = timeColumnMap.get(v.timestamp as unknown as string)!

      const x = laneX
      const y = 80 + timeIndex * ROW_SPACING

      // 检查特殊点
      const children = props.versions.filter(child => child.parentId === v.id)
      const isBranchPoint = children.some(c => c.branchName !== v.branchName)
      const isMergePoint = v.parentId && props.versions.find(p => p.id === v.parentId)?.branchName !== v.branchName

      nodes.push({
        ...v,
        x,
        y,
        branchIndex,
        timeIndex,
        isBranchPoint,
        isMergePoint
      })
    })
  })

  return nodes
}

// 有向图布局 - 力导向风格
const layoutGraphNodes = () => {
  const nodes: Array<FrontendLatexVersionNode & {
    x: number
    y: number
    level: number
    position: number
    isBranchPoint: boolean
    isMergePoint: boolean
  }> = []

  // 按时间排序
  const sortedVersions = [...props.versions].sort((a, b) =>
    new Date(a.timestamp).getTime() - new Date(b.timestamp).getTime()
  )

  // 分层计算（根据父子关系）
  const levels = new Map<string, number>()
  const calculateLevel = (version: FrontendLatexVersionNode, depth = 0): number => {
    if (levels.has(version.id)) return levels.get(version.id)!

    if (!version.parentId) {
      levels.set(version.id, 0)
      return 0
    }

    const parent = props.versions.find(v => v.id === version.parentId)
    if (parent) {
      const parentLevel = calculateLevel(parent, depth + 1)
      levels.set(version.id, parentLevel + 1)
      return parentLevel + 1
    }

    levels.set(version.id, 0)
    return 0
  }

  sortedVersions.forEach(v => calculateLevel(v))

  // 按层分组
  const levelGroups = new Map<number, FrontendLatexVersionNode[]>()
  sortedVersions.forEach(v => {
    const level = levels.get(v.id) || 0
    if (!levelGroups.has(level)) {
      levelGroups.set(level, [])
    }
    levelGroups.get(level)!.push(v)
  })

  levelGroups.forEach((versionsInLevel, level) => {
    const layerWidth = versionsInLevel.length * GRAPH_COLUMN_WIDTH
    const startX = (canvasSize.value.width - layerWidth) / 2

    versionsInLevel.forEach((v, idx) => {
      const x = startX + idx * GRAPH_COLUMN_WIDTH + GRAPH_COLUMN_WIDTH / 2
      const y = 80 + level * GRAPH_ROW_HEIGHT

      // 检查特殊点
      const children = props.versions.filter(child => child.parentId === v.id)
      const isBranchPoint = children.some(c => c.branchName !== v.branchName)
      const isMergePoint = v.parentId && props.versions.find(p => p.id === v.parentId)?.branchName !== v.branchName

      nodes.push({
        ...v,
        x,
        y,
        level,
        position: idx,
        isBranchPoint,
        isMergePoint
      })
    })
  })

  return nodes
}

// 时间轴布局 - 水平时间线
const layoutTimelineNodes = () => {
  const nodes: Array<FrontendLatexVersionNode & {
    x: number
    y: number
    lane: number
    isBranchPoint: boolean
    isMergePoint: boolean
  }> = []

  // 按时间排序
  const sortedVersions = [...props.versions].sort((a, b) =>
    new Date(a.timestamp).getTime() - new Date(b.timestamp).getTime()
  )

  // 为每个分支分配泳道（水平方向）
  const branchLaneMap = new Map<string, number>()
  branchColors.value.forEach((b, i) => branchLaneMap.set(b.name, i))

  const TIMELINE_START_X = 120
  const TIMELINE_SPACING_X = 150
  const TIMELINE_LANE_HEIGHT = 80

  sortedVersions.forEach((v, idx) => {
    const lane = branchLaneMap.get(v.branchName) || 0
    const x = TIMELINE_START_X + idx * TIMELINE_SPACING_X
    const y = 60 + lane * TIMELINE_LANE_HEIGHT

    // 检查特殊点
    const children = props.versions.filter(child => child.parentId === v.id)
    const isBranchPoint = children.some(c => c.branchName !== v.branchName)
    const isMergePoint = v.parentId && props.versions.find(p => p.id === v.parentId)?.branchName !== v.branchName

    nodes.push({
      ...v,
      x,
      y,
      lane,
      isBranchPoint,
      isMergePoint
    })
  })

  return nodes
})

// 画布尺寸
const canvasSize = computed(() => {
  if (laidOutNodes.value.length === 0) {
    return { width: 800, height: 600 }
  }

  let width = 800
  let height = 600

  if (viewMode.value === 'timeline') {
    // 时间轴模式：水平延伸
    const nodeCount = laidOutNodes.value.length
    width = Math.max(1200, 120 + nodeCount * TIMELINE_SPACING_X + 100)
    height = Math.max(400, 60 + branchColors.value.length * TIMELINE_LANE_HEIGHT + 60)
  } else if (viewMode.value === 'graph') {
    // 有向图模式：根据层级和每层节点数
    const maxX = Math.max(...laidOutNodes.value.map(n => n.x)) + 150
    const maxY = Math.max(...laidOutNodes.value.map(n => n.y)) + 100
    width = Math.max(800, maxX)
    height = Math.max(600, maxY)
  } else {
    // 树形图模式：分支泳道
    const maxX = Math.max(...laidOutNodes.value.map(n => n.x)) + 100
    const maxY = Math.max(...laidOutNodes.value.map(n => n.y)) + 100
    width = Math.max(800, maxX)
    height = Math.max(600, maxY)
  }

  return { width, height }
})

// 父子连接
const parentChildConnections = computed(() => {
  const nodeMap = new Map(laidOutNodes.value.map(n => [n.id, n]))
  const connections: Array<{
    from: typeof laidOutNodes.value[0]
    to: typeof laidOutNodes.value[0]
    path: string
    color: string
    markerEnd: string
    isMerge: boolean
  }> = []

  laidOutNodes.value.forEach(node => {
    if (node.parentId && nodeMap.has(node.parentId)) {
      const parent = nodeMap.get(node.parentId)!
      const branch = branchColors.value.find(b => b.name === node.branchName)
      const isMerge = parent.branchName !== node.branchName

      let path: string

      if (viewMode.value === 'timeline') {
        // 时间轴模式：水平连接
        if (isMerge) {
          // 跨泳道连接 - 贝塞尔曲线
          const midX = (parent.x + node.x) / 2
          path = `M ${parent.x + NODE_RADIUS} ${parent.y}
                  C ${midX} ${parent.y}, ${midX} ${node.y}, ${node.x - NODE_RADIUS} ${node.y}`
        } else {
          // 同泳道连接 - 水平直线
          path = `M ${parent.x + NODE_RADIUS} ${parent.y}
                  L ${node.x - NODE_RADIUS} ${node.y}`
        }
      } else if (viewMode.value === 'graph') {
        // 有向图模式：垂直层级连接
        if (isMerge) {
          const midX = (parent.x + node.x) / 2
          path = `M ${parent.x} ${parent.y + NODE_RADIUS}
                  C ${parent.x} ${parent.y + 40}, ${node.x} ${parent.y + 20}, ${node.x} ${node.y - NODE_RADIUS}`
        } else {
          path = `M ${parent.x} ${parent.y + NODE_RADIUS}
                  L ${node.x} ${node.y - NODE_RADIUS}`
        }
      } else {
        // 树形图模式：分支泳道连接
        if (isMerge) {
          // 合并连接 - 从父节点到子节点，带拐角
          const midY = (parent.y + node.y) / 2
          path = `M ${parent.x} ${parent.y + NODE_RADIUS}
                  L ${parent.x} ${midY}
                  L ${node.x} ${midY}
                  L ${node.x} ${node.y - NODE_RADIUS}`
        } else {
          // 同分支连接 - 直线
          path = `M ${parent.x} ${parent.y + NODE_RADIUS}
                  L ${node.x} ${node.y - NODE_RADIUS}`
        }
      }

      connections.push({
        from: parent,
        to: node,
        path,
        color: branch?.color || '#909399',
        markerEnd: `url(#arrow-${node.branchName})`,
        isMerge
      })
    }
  })

  return connections
})

// 合并连接（特殊显示）
const mergeConnections = computed(() => {
  return parentChildConnections.value.filter(c => c.isMerge)
})

// 时间轴刻度
const timelineTicks = computed(() => {
  if (props.versions.length === 0 || viewMode.value !== 'timeline') return []

  const timestamps = props.versions.map(v => new Date(v.timestamp).getTime())
  const minTime = Math.min(...timestamps)
  const maxTime = Math.max(...timestamps)
  const timeRange = maxTime - minTime

  // 生成5-8个时间刻度
  const tickCount = Math.min(8, Math.max(5, Math.ceil(timeRange / (24 * 60 * 60 * 1000))))
  const ticks: Array<{ x: number; label: string }> = []

  const startX = 120
  const availableWidth = canvasSize.value.width - 170

  for (let i = 0; i < tickCount; i++) {
    const time = minTime + (timeRange * i) / Math.max(1, tickCount - 1)
    const date = new Date(time)
    const x = startX + (availableWidth * i) / Math.max(1, tickCount - 1)

    let label: string
    if (timeRange > 30 * 24 * 60 * 60 * 1000) {
      label = date.toLocaleDateString('zh-CN', { month: '2-digit', day: '2-digit' })
    } else {
      label = date.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' })
    }

    ticks.push({ x, label })
  }

  return ticks
})

// 有向图层级
const graphLevels = computed(() => {
  if (viewMode.value !== 'graph' || laidOutNodes.value.length === 0) return []

  const levels = new Set<number>()
  laidOutNodes.value.forEach(n => {
    if ('level' in n) levels.add((n as any).level)
  })

  return Array.from(levels).sort((a, b) => a - b)
})

// 获取节点属性
const getNodeRadius = (node: FrontendLatexVersionNode) => {
  if (node.isAutoSave) return NODE_RADIUS - 4
  if (node.isMerged) return NODE_RADIUS + 2
  return NODE_RADIUS
}

const getNodeFill = (node: FrontendLatexVersionNode) => {
  if (node.isAutoSave) return '#E4E7ED'
  return `url(#grad-${node.branchName})`
}

const getNodeColor = (node: FrontendLatexVersionNode) => {
  const branch = branchColors.value.find(b => b.name === node.branchName)
  return branch?.color || '#909399'
}

const getNodeStroke = (node: FrontendLatexVersionNode) => {
  if (props.selectedVersion?.id === node.id) return '#409EFF'
  if (comparingVersions.value.has(node.id)) return '#E6A23C'
  if (node.isMerged) return '#67C23A'
  return '#fff'
}

const getNodeStrokeWidth = (node: FrontendLatexVersionNode) => {
  if (props.selectedVersion?.id === node.id) return 3
  if (comparingVersions.value.has(node.id)) return 3
  return 2
}

// 颜色辅助函数
const lightenColor = (color: string, percent: number) => {
  const num = parseInt(color.replace('#', ''), 16)
  const amt = Math.round(2.55 * percent)
  const R = (num >> 16) + amt
  const G = (num >> 8 & 0x00FF) + amt
  const B = (num & 0x0000FF) + amt
  return '#' + (0x1000000 +
    (R < 255 ? R < 1 ? 0 : R : 255) * 0x10000 +
    (G < 255 ? G < 1 ? 0 : G : 255) * 0x100 +
    (B < 255 ? B < 1 ? 0 : B : 255)
  ).toString(16).slice(1)
}

// 格式化时间
const formatTime = (timestamp: Date) => {
  return new Date(timestamp).toLocaleString('zh-CN')
}

const formatTimeRelative = (timestamp: Date) => {
  const now = new Date()
  const diff = now.getTime() - new Date(timestamp).getTime()
  const minutes = Math.floor(diff / 60000)
  const hours = Math.floor(diff / 3600000)
  const days = Math.floor(diff / 86400000)

  if (minutes < 60) return `${minutes}分钟前`
  if (hours < 24) return `${hours}小时前`
  if (days < 7) return `${days}天前`
  return new Date(timestamp).toLocaleDateString('zh-CN')
}

// 交互处理
const handleNodeClick = (node: FrontendLatexVersionNode & { x: number; y: number }) => {
  emit('select', node)
}

const showTooltip = (node: FrontendLatexVersionNode & { x: number; y: number }, event: MouseEvent) => {
  tooltipNode.value = node
  tooltipPos.value = {
    x: event.clientX + 15,
    y: event.clientY + 15
  }
}

const hideTooltip = () => {
  tooltipNode.value = null
}

// 缩放和拖拽
const zoomIn = () => { zoomLevel.value = Math.min(2, zoomLevel.value * 1.2) }
const zoomOut = () => { zoomLevel.value = Math.max(0.3, zoomLevel.value / 1.2) }
const zoomReset = () => { zoomLevel.value = 1; panX.value = 0; panY.value = 0 }
const fitView = () => {
  zoomReset()
  if (!svgContainerRef.value || !containerRef.value) return

  const container = containerRef.value
  const scale = Math.min(
    (container.clientWidth - 40) / canvasSize.value.width,
    (container.clientHeight - 100) / canvasSize.value.height
  )
  zoomLevel.value = Math.min(scale, 1)
}

const startPan = (e: MouseEvent) => {
  isPanning.value = true
  panStart.value = { x: e.clientX - panX.value, y: e.clientY - panY.value }
}

const onPan = (e: MouseEvent) => {
  if (!isPanning.value) return
  panX.value = e.clientX - panStart.value.x
  panY.value = e.clientY - panStart.value.y
}

const endPan = () => {
  isPanning.value = false
}

const onWheel = (e: WheelEvent) => {
  const delta = e.deltaY > 0 ? 0.9 : 1.1
  zoomLevel.value = Math.min(2, Math.max(0.3, zoomLevel.value * delta))
}

// 过滤器（预留）
const toggleFilter = (type: string) => {
  // TODO: 实现分支过滤
}

// 生命周期
onMounted(() => {
  fitView()
})

watch(() => props.versions, () => {
  fitView()
}, { deep: true })
</script>

<style scoped lang="scss">
.branch-tree-view {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #f8f9fa;
  overflow: hidden;

  .toolbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 8px 16px;
    background: #fff;
    border-bottom: 1px solid #e4e7ed;
    z-index: 10;
  }

  .svg-container {
    flex: 1;
    overflow: hidden;
    position: relative;
    cursor: grab;
    transition: transform 0.1s ease-out;

    &:active {
      cursor: grabbing;
    }
  }

  .tree-svg {
    display: block;
  }

  .connection {
    transition: stroke-width 0.2s;

    &:hover {
      stroke-width: 3 !important;
    }

    &.is-merge {
      opacity: 0.6;
    }
  }

  .node {
    cursor: pointer;
    transition: transform 0.2s;

    &:hover {
      transform: scale(1.1);
    }

    &.is-selected .node-circle {
      filter: url(#glow);
    }

    &.is-comparing .node-circle {
      stroke: #E6A23C;
      stroke-width: 3;
    }

    &.is-merged .node-circle {
      stroke-dasharray: 3 2;
    }

    &.is-auto-save {
      opacity: 0.7;

      .node-circle {
        stroke-dasharray: 2 2;
      }
    }
  }

  .node-label {
    pointer-events: none;

    .label-content {
      background: rgba(255, 255, 255, 0.95);
      border-radius: 6px;
      padding: 6px 10px;
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
      text-align: center;

      .label-summary {
        font-size: 12px;
        color: #303133;
        margin-bottom: 4px;
        word-break: break-word;
      }

      .label-meta {
        display: flex;
        justify-content: center;
        gap: 8px;
        font-size: 10px;
        color: #909399;
      }
    }
  }

  .tooltip {
    position: fixed;
    z-index: 9999;
    background: #fff;
    border: 1px solid #e4e7ed;
    border-radius: 8px;
    padding: 12px;
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.15);
    min-width: 200px;
    pointer-events: none;

    .tooltip-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 8px;
      padding-bottom: 8px;
      border-bottom: 1px solid #f5f7fa;

      .tooltip-time {
        font-size: 12px;
        color: #909399;
      }
    }

    .tooltip-summary {
      font-size: 13px;
      color: #303133;
      margin-bottom: 12px;
    }

    .tooltip-info {
      .info-item {
        display: flex;
        justify-content: space-between;
        margin-bottom: 6px;
        font-size: 12px;

        .info-label {
          color: #909399;
        }

        .info-value {
          color: #606266;

          &.change-add {
            color: #67C23A;
            font-weight: 500;
          }
        }
      }
    }
  }

  .legend {
    position: absolute;
    bottom: 20px;
    left: 20px;
    background: #fff;
    padding: 12px 16px;
    border-radius: 8px;
    box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);

    .legend-title {
      font-size: 13px;
      font-weight: 600;
      color: #303133;
      margin-bottom: 10px;
    }

    .legend-items {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }

    .legend-item {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 12px;
      color: #606266;
      cursor: pointer;
      padding: 4px;
      border-radius: 4px;
      transition: background 0.2s;

      &:hover {
        background: #f5f7fa;
      }

      .legend-dot {
        width: 12px;
        height: 12px;
        border-radius: 50%;

        &.main { background: linear-gradient(135deg, #667eea, #764ba2); }
        &.branch { background: linear-gradient(135deg, #4facfe, #00f2fe); }
        &.auto-save { background: #E4E7ED; }
        &.merged { background: #67C23A; }
      }
    }
  }
}
</style>
