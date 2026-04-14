<template>
  <div class="branch-tree-view" ref="containerRef">
    <!-- 顶部工具栏 -->
    <div class="toolbar">
      <div class="toolbar-left">
        <el-radio-group v-model="layoutMode" size="small">
          <el-radio-button value="horizontal">横向</el-radio-button>
          <el-radio-button value="vertical">纵向</el-radio-button>
        </el-radio-group>
        <el-divider direction="vertical" />
        <el-button-group size="small">
          <el-button :icon="ZoomOut" @click="handleZoom('out')" />
          <el-button @click="handleZoom('reset')">100%</el-button>
          <el-button :icon="ZoomIn" @click="handleZoom('in')" />
        </el-button-group>
        <el-button :icon="FullScreen" @click="fitToScreen" size="small">适配</el-button>
      </div>
      <div class="toolbar-right">
        <el-switch v-model="showLabels" size="small" active-text="标签" />
        <el-switch v-model="showStats" size="small" active-text="统计" />
        <el-switch v-model="showAvatars" size="small" active-text="头像" />
      </div>
    </div>

    <!-- SVG画布 -->
    <div class="canvas-container" :style="{ transform: `scale(${zoomLevel})` }">
      <svg
        ref="svgRef"
        class="tree-svg"
        :width="svgWidth"
        :height="svgHeight"
        :viewBox="`0 0 ${svgWidth} ${svgHeight}`"
      >
        <defs>
          <!-- 节点阴影 -->
          <filter id="nodeShadow" x="-50%" y="-50%" width="200%" height="200%">
            <feDropShadow dx="0" dy="2" stdDeviation="3" flood-opacity="0.15"/>
          </filter>

          <!-- 发光效果 -->
          <filter id="glow" x="-50%" y="-50%" width="200%" height="200%">
            <feGaussianBlur stdDeviation="2" result="coloredBlur"/>
            <feMerge>
              <feMergeNode in="coloredBlur"/>
              <feMergeNode in="SourceGraphic"/>
            </feMerge>
          </filter>

          <!-- 渐变定义 -->
          <linearGradient v-for="branch in branches" :key="branch.name" :id="`gradient-${branch.name}`" x1="0%" y1="0%" x2="100%" y2="100%">
            <stop offset="0%" :stop-color="branch.color" stop-opacity="0.8"/>
            <stop offset="100%" :stop-color="branch.color" stop-opacity="0.4"/>
          </linearGradient>

          <!-- 主分支特殊渐变 -->
          <linearGradient id="gradient-main" x1="0%" y1="0%" x2="100%" y2="100%">
            <stop offset="0%" stop-color="#667eea" stop-opacity="0.9"/>
            <stop offset="100%" stop-color="#764ba2" stop-opacity="0.7"/>
          </linearGradient>

          <!-- 箭头标记 -->
          <marker id="arrowhead-main" markerWidth="12" markerHeight="12" refX="10" refY="4" orient="auto">
            <path d="M0,0 L8,4 L0,8 L2,4 Z" fill="#667eea"/>
          </marker>
          <marker v-for="branch in branches.filter(b => b.name !== 'main')" :key="`arrow-${branch.name}`"
                  :id="`arrowhead-${branch.name}`" markerWidth="12" markerHeight="12" refX="10" refY="4" orient="auto">
            <path d="M0,0 L8,4 L0,8 L2,4 Z" :fill="branch.color"/>
          </marker>
        </defs>

        <!-- 背景网格 -->
        <g class="grid-lines" v-if="showGrid">
          <defs>
            <pattern id="grid" width="40" height="40" patternUnits="userSpaceOnUse">
              <path d="M 40 0 L 0 0 0 40" fill="none" stroke="#f0f0f0" stroke-width="1"/>
            </pattern>
          </defs>
          <rect width="100%" height="100%" fill="url(#grid)" />
        </g>

        <!-- 时间轴线 -->
        <g class="time-axis" v-if="layoutMode === 'vertical'">
          <line :x1="timeAxisX" :y1="padding.top" :x2="timeAxisX" :y2="svgHeight - padding.bottom"
                stroke="#e0e0e0" stroke-width="2" stroke-dasharray="5,5"/>
          <g v-for="tick in timeTicks" :key="tick.label">
            <line :x1="timeAxisX - 5" :y1="tick.y" :x2="timeAxisX + 5" :y2="tick.y" stroke="#9e9e9e" stroke-width="1"/>
            <text :x="timeAxisX - 10" :y="tick.y + 4" text-anchor="end" font-size="10" fill="#757575">{{ tick.label }}</text>
          </g>
        </g>

        <!-- 分支连接线 -->
        <g class="connections">
          <!-- 主分支线 -->
          <path
            v-for="path in mainBranchPath"
            :key="path.id"
            :d="path.d"
            stroke="url(#gradient-main)"
            stroke-width="3"
            fill="none"
            stroke-linecap="round"
            marker-end="url(#arrowhead-main)"
            class="connection-line main-branch"
          />

          <!-- 其他分支线 -->
          <path
            v-for="conn in branchConnections"
            :key="conn.id"
            :d="conn.d"
            :stroke="conn.color"
            stroke-width="2"
            fill="none"
            stroke-linecap="round"
            :marker-end="`url(#arrowhead-${conn.branchName})`"
            class="connection-line"
            :class="{ 'is-merge': conn.isMerge }"
          />
        </g>

        <!-- 分支标签 -->
        <g class="branch-labels">
          <g v-for="branch in branches" :key="`label-${branch.name}`"
             :transform="`translate(${branch.labelX}, ${branch.labelY})`"
             class="branch-label">
            <rect
              :width="branch.labelWidth"
              height="24"
              rx="12"
              :fill="branch.name === 'main' ? 'url(#gradient-main)' : `url(#gradient-${branch.name})`"
              filter="url(#nodeShadow)"
            />
            <text x="12" y="17" font-size="12" font-weight="bold" fill="#fff">
              {{ branch.displayName }}
            </text>
            <!-- 徽章 -->
            <g v-if="branch.commitCount > 0">
              <circle :cx="branch.labelWidth - 8" cy="8" r="8" fill="#ff4757"/>
              <text :x="branch.labelWidth - 8" y="11" text-anchor="middle" font-size="9" fill="#fff" font-weight="bold">
                {{ branch.commitCount > 99 ? '99+' : branch.commitCount }}
              </text>
            </g>
          </g>
        </g>

        <!-- 版本节点组 -->
        <g class="nodes">
          <g
            v-for="node in computedNodes"
            :key="node.id"
            :transform="`translate(${node.x}, ${node.y})`"
            class="node-group"
            :class="{
              'is-selected': selectedVersion?.id === node.id,
              'is-comparing': comparingVersions.has(node.id),
              'is-merged': node.isMerged,
              'is-auto-save': node.isAutoSave,
              'has-conflicts': node.hasConflicts
            }"
            @click="handleNodeClick(node)"
            @mouseenter="handleNodeHover(node, $event)"
            @mouseleave="handleNodeLeave"
          >
            <!-- 节点外圈（选中效果） -->
            <circle
              v-if="selectedVersion?.id === node.id"
              :r="nodeRadius + 6"
              :fill="getNodeColor(node)"
              opacity="0.2"
              class="selection-ring"
            >
              <animate attributeName="r" :from="nodeRadius + 6" :to="nodeRadius + 10" dur="1.5s" repeatCount="indefinite"/>
              <animate attributeName="opacity" values="0.2;0.1;0.2" dur="1.5s" repeatCount="indefinite"/>
            </circle>

            <!-- 节点主体 -->
            <circle
              :r="getNodeRadius(node)"
              :fill="getNodeFill(node)"
              :stroke="getNodeStroke(node)"
              :stroke-width="getNodeStrokeWidth(node)"
              filter="url(#nodeShadow)"
              class="node-circle"
            />

            <!-- 节点图标 -->
            <g class="node-icon">
              <!-- 合并图标 -->
              <g v-if="node.isMerged" transform="translate(-6, -6) scale(0.8)">
                <path d="M6 3L2 7l4 4M10 3l4 4-4 4M5 7h6" stroke="#fff" stroke-width="1.5" fill="none" stroke-linecap="round"/>
              </g>
              <!-- 分支图标 -->
              <g v-else-if="node.isBranchPoint" transform="translate(-6, -6) scale(0.8)">
                <circle cx="6" cy="6" r="1" fill="#fff"/>
                <path d="M6 2v8M2 6h8" stroke="#fff" stroke-width="1.5" stroke-linecap="round"/>
              </g>
              <!-- 自动保存点 -->
              <g v-else-if="node.isAutoSave" transform="translate(-5, -5)">
                <path d="M5 2v6M2 5h6" stroke="#fff" stroke-width="1.5" stroke-linecap="round"/>
              </g>
              <!-- 默认点 -->
              <circle v-else r="2" fill="#fff"/>
            </g>

            <!-- 用户头像 -->
            <g v-if="showAvatars && node.author" class="node-avatar" :transform="`translate(${nodeRadius - 2}, -${nodeRadius - 2})`">
              <clipPath :id="`avatar-${node.id}`">
                <circle r="10"/>
              </clipPath>
              <image
                :x="-10" :y="-10" width="20" height="20"
                :href="getAvatarUrl(node.author)"
                :clip-path="`url(#avatar-${node.id})`"
              />
              <circle r="10" fill="none" stroke="#fff" stroke-width="2"/>
            </g>

            <!-- 节点标签 -->
            <g v-if="showLabels" class="node-label" :transform="`translate(0, ${nodeRadius + 8})`">
              <rect
                :x="-(node.labelWidth / 2)"
                y="0"
                :width="node.labelWidth"
                height="28"
                rx="6"
                :fill="getNodeLabelFill(node)"
                filter="url(#nodeShadow)"
              />
              <text
                :x="0"
                y="12"
                text-anchor="middle"
                font-size="11"
                font-weight="500"
                fill="#fff"
              >{{ truncateText(node.summary || '未命名', 20) }}</text>
              <text
                v-if="showStats"
                :x="0"
                y="24"
                text-anchor="middle"
                font-size="9"
                fill="rgba(255,255,255,0.8)"
              >{{ formatTimeShort(node.timestamp) }}</text>
            </g>
          </g>
        </g>
      </svg>
    </div>

    <!-- 悬浮提示 -->
    <transition name="el-fade-in">
      <div
        v-if="hoveredNode"
        class="node-tooltip"
        :style="{
          left: tooltipPosition.x + 'px',
          top: tooltipPosition.y + 'px'
        }"
      >
        <div class="tooltip-header">
          <el-tag :type="hoveredNode.branchName === 'main' ? 'primary' : 'success'" size="small">
            {{ hoveredNode.branchName }}
          </el-tag>
          <span class="tooltip-time">{{ formatTime(hoveredNode.timestamp) }}</span>
        </div>
        <div class="tooltip-content">
          <div class="tooltip-summary">{{ hoveredNode.summary || '无摘要' }}</div>
          <div class="tooltip-meta">
            <span><el-icon><User /></el-icon> {{ hoveredNode.author }}</span>
            <span><el-icon><Document /></el-icon> {{ hoveredNode.totalLines }}行</span>
            <span v-if="hoveredNode.changeCount > 0">
              <el-icon><Edit /></el-icon> +{{ hoveredNode.changeCount }}
            </span>
          </div>
        </div>
      </div>
    </transition>

    <!-- 图例 -->
    <div class="legend" v-if="showLegend">
      <div class="legend-title">图例</div>
      <div class="legend-items">
        <div class="legend-item">
          <span class="legend-dot main"></span>
          <span>主分支</span>
        </div>
        <div class="legend-item">
          <span class="legend-dot feature"></span>
          <span>特性分支</span>
        </div>
        <div class="legend-item">
          <span class="legend-dot auto-save"></span>
          <span>自动保存</span>
        </div>
        <div class="legend-item">
          <span class="legend-dot merged"></span>
          <span>已合并</span>
        </div>
        <div class="legend-item">
          <span class="legend-dot selected"></span>
          <span>已选择</span>
        </div>
      </div>
    </div>

    <!-- 迷你地图 -->
    <div v-if="showMinimap" class="minimap">
      <svg :width="minimapWidth" :height="minimapHeight">
        <rect width="100%" height="100%" fill="#f5f5f5"/>
        <g :transform="`scale(${minimapScale})`">
          <circle
            v-for="node in computedNodes"
            :key="`minimap-${node.id}`"
            :cx="node.x"
            :cy="node.y"
            :r="3"
            :fill="getNodeColor(node)"
            opacity="0.6"
          />
        </g>
        <rect
          :x="viewportX"
          :y="viewportY"
          :width="viewportWidth"
          :height="viewportHeight"
          fill="none"
          stroke="#409EFF"
          stroke-width="1"
          stroke-opacity="0.5"
        />
      </svg>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
import { ZoomIn, ZoomOut, FullScreen, User, Document, Edit } from '@element-plus/icons-vue'
import type { FrontendLatexVersionNode } from '@/api/adapters/latexAdapter'

const props = defineProps<{
  versions: FrontendLatexVersionNode[]
  selectedVersion?: FrontendLatexVersionNode | null
}>()

const emit = defineEmits<{
  (e: 'select', version: FrontendLatexVersionNode): void
  (e: 'compare', versions: FrontendLatexVersionNode[]): void
}>()

// 布局参数
const nodeRadius = 14
const padding = { top: 60, right: 60, bottom: 60, left: 120 }
const nodeSpacing = { x: 100, y: 80 }
const labelWidth = 160

// 状态
const containerRef = ref<HTMLElement>()
const svgRef = ref<SVGSVGElement>()
const layoutMode = ref<'horizontal' | 'vertical'>('horizontal')
const zoomLevel = ref(1)
const showLabels = ref(true)
const showStats = ref(true)
const showAvatars = ref(false)
const showGrid = ref(true)
const showLegend = ref(true)
const showMinimap = ref(true)

const comparingVersions = ref<Set<string>>(new Set())
const hoveredNode = ref<(FrontendLatexVersionNode & { x: number; y: number }) | null>(null)
const tooltipPosition = ref({ x: 0, y: 0 })

// SVG尺寸
const svgWidth = ref(1200)
const svgHeight = ref(800)

// 计算分支
const branches = computed(() => {
  const branchMap = new Map<string, {
    name: string
    displayName: string
    color: string
    commitCount: number
    labelX: number
    labelY: number
    labelWidth: number
  }>()

  // 按分支统计提交数
  const branchCounts = new Map<string, number>()
  props.versions.forEach(v => {
    branchCounts.set(v.branchName, (branchCounts.get(v.branchName) || 0) + 1)
  })

  // 分支颜色
  const branchColors: Record<string, string> = {
    'main': '#667eea',
    'develop': '#f093fb',
    'feature': '#4facfe',
    'hotfix': '#fa709a',
    'release': '#fee140'
  }

  const getColor = (name: string, index: number) => {
    return branchColors[name] || `hsl(${(index * 60) % 360}, 70%, 55%)`
  }

  let index = 0
  props.versions.forEach((v, i) => {
    if (!branchMap.has(v.branchName)) {
      const color = getColor(v.branchName, index)
      branchMap.set(v.branchName, {
        name: v.branchName,
        displayName: v.branchName === 'main' ? 'main' : v.branchName,
        color,
        commitCount: branchCounts.get(v.branchName) || 0,
        labelX: 10,
        labelY: 30 + index * 30,
        labelWidth: Math.max(60, v.branchName.length * 8 + 30)
      })
      index++
    }
  })

  return Array.from(branchMap.values()).map((b, i) => ({
    ...b,
    labelY: 40 + i * 28
  }))
})

// 时间轴
const timeAxisX = computed(() => padding.left - 20)

const timeTicks = computed(() => {
  if (props.versions.length === 0) return []

  const timestamps = props.versions.map(v => new Date(v.timestamp).getTime())
  const minTime = Math.min(...timestamps)
  const maxTime = Math.max(...timestamps)
  const timeRange = maxTime - minTime

  // 生成5-8个时间刻度
  const tickCount = Math.min(8, Math.max(5, Math.ceil(timeRange / (24 * 60 * 60 * 1000))))
  const ticks: Array<{ y: number; label: string }> = []

  for (let i = 0; i < tickCount; i++) {
    const time = minTime + (timeRange * i) / (tickCount - 1)
    const date = new Date(time)
    const y = padding.top + ((svgHeight.value - padding.top - padding.bottom) * i) / (tickCount - 1)

    let label: string
    if (timeRange > 30 * 24 * 60 * 60 * 1000) {
      label = date.toLocaleDateString('zh-CN', { month: '2-digit', day: '2-digit' })
    } else {
      label = date.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' })
    }

    ticks.push({ y, label })
  }

  return ticks
})

// 计算节点布局
const computedNodes = computed(() => {
  if (props.versions.length === 0) return []

  const nodes: Array<FrontendLatexVersionNode & {
    x: number
    y: number
    labelWidth: number
    isBranchPoint: boolean
  }> = []

  // 按时间排序
  const sortedVersions = [...props.versions].sort((a, b) =>
    new Date(a.timestamp).getTime() - new Date(b.timestamp).getTime()
  )

  // 分配位置
  const branchPositions = new Map<string, number>()
  const columnCount = new Map<number, number>()
  let currentColumn = 0

  sortedVersions.forEach((v, index) => {
    // 分支Y位置
    if (!branchPositions.has(v.branchName)) {
      const branchIndex = branches.value.findIndex(b => b.name === v.branchName)
      branchPositions.set(v.branchName, padding.top + branchIndex * nodeSpacing.y)
    }

    // X位置（时间列）
    const branchCommits = sortedVersions.filter(sv => sv.branchName === v.branchName)
    const branchIndex = branchCommits.findIndex(sv => sv.id === v.id)
    const columnCountInBranch = columnCount.get(branchIndex) || 0
    columnCount.set(branchIndex, columnCountInBranch + 1)

    const x = padding.left + columnCountInBranch * nodeSpacing.x
    const y = branchPositions.get(v.branchName)!

    // 检查是否是分支点
    const isBranchPoint = sortedVersions.some(sv =>
      sv.parentId === v.id && sv.branchName !== v.branchName
    )

    nodes.push({
      ...v,
      x,
      y,
      labelWidth: Math.min(180, (v.summary?.length || 10) * 7 + 40),
      isBranchPoint
    })
  })

  // 更新SVG尺寸
  if (nodes.length > 0) {
    const maxX = Math.max(...nodes.map(n => n.x)) + padding.right
    const maxY = Math.max(...nodes.map(n => n.y)) + padding.bottom
    svgWidth.value = Math.max(1200, maxX)
    svgHeight.value = Math.max(800, maxY)
  }

  return nodes
})

// 主分支路径
const mainBranchPath = computed(() => {
  const mainNodes = computedNodes.value.filter(n => n.branchName === 'main')
  const paths: Array<{ id: string; d: string }> = []

  for (let i = 0; i < mainNodes.length - 1; i++) {
    const from = mainNodes[i]
    const to = mainNodes[i + 1]
    const d = createSmoothPath(from, to, 'main')
    paths.push({ id: `${from.id}-${to.id}`, d })
  }

  return paths
})

// 分支连接
const branchConnections = computed(() => {
  const nodeMap = new Map(computedNodes.value.map(n => [n.id, n]))
  const conns: Array<{
    id: string
    d: string
    color: string
    branchName: string
    isMerge: boolean
  }> = []

  computedNodes.value.forEach(node => {
    if (node.parentId && nodeMap.has(node.parentId)) {
      const parent = nodeMap.get(node.parentId)!
      const branch = branches.value.find(b => b.name === node.branchName)
      const isMerge = node.isMerged || (parent.branchName !== node.branchName)

      const d = createSmoothPath(parent, node, node.branchName, isMerge)

      conns.push({
        id: `${parent.id}-${node.id}`,
        d,
        color: branch?.color || '#909399',
        branchName: node.branchName,
        isMerge
      })
    }
  })

  return conns
})

// 创建平滑路径
const createSmoothPath = (
  from: { x: number; y: number },
  to: { x: number; y: number },
  branchName: string,
  isMerge = false
) => {
  const r = nodeRadius
  const midX = (from.x + to.x) / 2

  if (isMerge) {
    // 合并路径 - 先横向再纵向
    return `M ${from.x + r} ${from.y}
            L ${midX} ${from.y}
            Q ${midX + 20} ${(from.y + to.y) / 2}, ${to.x - r - 10} ${to.y}
            L ${to.x - r} ${to.y}`
  } else {
    // 普通路径 - S形曲线
    return `M ${from.x + r} ${from.y}
            C ${from.x + nodeSpacing.x / 2} ${from.y},
              ${to.x - nodeSpacing.x / 2} ${to.y},
              ${to.x - r} ${to.y}`
  }
}

// 节点相关方法
const getNodeRadius = (node: FrontendLatexVersionNode) => {
  if (node.isAutoSave) return nodeRadius - 4
  if (node.isMerged) return nodeRadius + 2
  return nodeRadius
}

const getNodeColor = (node: FrontendLatexVersionNode) => {
  if (node.isAutoSave) return '#E4E7ED'
  const branch = branches.value.find(b => b.name === node.branchName)
  return branch?.color || '#909399'
}

const getNodeFill = (node: FrontendLatexVersionNode) => {
  if (node.branchName === 'main') return 'url(#gradient-main)'
  return `url(#gradient-${node.branchName})`
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

const getNodeLabelFill = (node: FrontendLatexVersionNode) => {
  return getNodeColor(node)
}

// 获取用户头像URL
const getAvatarUrl = (author: string) => {
  // 基于用户名生成头像
  const seed = encodeURIComponent(author)
  return `https://api.dicebear.com/7.x/initials/svg?seed=${seed}&backgroundColor=409EFF`
}

// 格式化时间
const formatTime = (timestamp: Date) => {
  const date = new Date(timestamp)
  return date.toLocaleString('zh-CN')
}

const formatTimeShort = (timestamp: Date) => {
  const date = new Date(timestamp)
  const now = new Date()
  const diffMs = now.getTime() - date.getTime()
  const diffMins = Math.floor(diffMs / 60000)

  if (diffMins < 60) return `${diffMins}m`
  const diffHours = Math.floor(diffMs / 3600000)
  if (diffHours < 24) return `${diffHours}h`
  return date.toLocaleDateString('zh-CN', { month: '2-digit', day: '2-digit' })
}

// 截断文本
const truncateText = (text: string, maxLength: number) => {
  return text?.length > maxLength ? text.substring(0, maxLength) + '...' : text
}

// 交互处理
const handleNodeClick = (node: FrontendLatexVersionNode & { x: number; y: number }) => {
  emit('select', node)
}

const handleNodeHover = (
  node: FrontendLatexVersionNode & { x: number; y: number },
  event: MouseEvent
) => {
  hoveredNode.value = node
  tooltipPosition.value = {
    x: event.clientX + 15,
    y: event.clientY + 15
  }
}

const handleNodeLeave = () => {
  hoveredNode.value = null
}

// 缩放处理
const handleZoom = (action: 'in' | 'out' | 'reset') => {
  if (action === 'in') zoomLevel.value = Math.min(2, zoomLevel.value + 0.1)
  else if (action === 'out') zoomLevel.value = Math.max(0.5, zoomLevel.value - 0.1)
  else zoomLevel.value = 1
}

const fitToScreen = () => {
  zoomLevel.value = 1
  if (!containerRef.value) return

  const containerWidth = containerRef.value.clientWidth
  const containerHeight = containerRef.value.clientHeight
  const scaleX = containerWidth / svgWidth.value
  const scaleY = containerHeight / svgHeight.value
  zoomLevel.value = Math.min(scaleX, scaleY, 1) * 0.95
}

// 迷你地图
const minimapWidth = 150
const minimapHeight = 100

const minimapScale = computed(() => {
  return Math.min(minimapWidth / svgWidth.value, minimapHeight / svgHeight.value)
})

const viewportX = ref(0)
const viewportY = ref(0)
const viewportWidth = ref(50)
const viewportHeight = ref(50)

// 生命周期
onMounted(() => {
  fitToScreen()
})

watch(() => props.versions, () => {
  fitToScreen()
}, { deep: true })
</script>

<style scoped lang="scss">
.branch-tree-view {
  position: relative;
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #fafafa;
  overflow: hidden;

  .toolbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 8px 16px;
    background: #fff;
    border-bottom: 1px solid #e4e7ed;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);
    z-index: 10;

    .toolbar-left,
    .toolbar-right {
      display: flex;
      gap: 12px;
      align-items: center;
    }
  }

  .canvas-container {
    flex: 1;
    overflow: auto;
    position: relative;
    transition: transform 0.3s ease;
    cursor: grab;

    &:active {
      cursor: grabbing;
    }
  }

  .tree-svg {
    display: block;
    background: #fff;
    border-radius: 8px;
    box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  }

  .connection-line {
    transition: stroke-width 0.2s, opacity 0.2s;

    &:hover {
      stroke-width: 3 !important;
      opacity: 0.8;
    }

    &.is-merge {
      stroke-dasharray: 5 3;
    }
  }

  .node-group {
    cursor: pointer;
    transition: transform 0.2s;

    &:hover {
      transform: scale(1.1);
    }

    &.is-selected .node-circle {
      stroke: #409EFF;
      stroke-width: 3;
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

  .selection-ring {
    animation: pulse 1.5s ease-in-out infinite;
  }

  @keyframes pulse {
    0%, 100% { opacity: 0.3; }
    50% { opacity: 0.1; }
  }

  .node-tooltip {
    position: fixed;
    z-index: 1000;
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
      margin-bottom: 8px;
      line-height: 1.5;
    }

    .tooltip-meta {
      display: flex;
      flex-wrap: wrap;
      gap: 12px;
      font-size: 12px;
      color: #606266;

      span {
        display: flex;
        align-items: center;
        gap: 4px;

        .el-icon {
          font-size: 14px;
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
      font-size: 12px;
      font-weight: 600;
      color: #303133;
      margin-bottom: 8px;
    }

    .legend-items {
      display: flex;
      flex-direction: column;
      gap: 6px;
    }

    .legend-item {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 12px;
      color: #606266;

      .legend-dot {
        width: 12px;
        height: 12px;
        border-radius: 50%;

        &.main { background: linear-gradient(135deg, #667eea, #764ba2); }
        &.feature { background: linear-gradient(135deg, #4facfe, #00f2fe); }
        &.auto-save { background: #E4E7ED; }
        &.merged { background: #67C23A; }
        &.selected { background: #409EFF; }
      }
    }
  }

  .minimap {
    position: absolute;
    bottom: 20px;
    right: 20px;
    background: #fff;
    padding: 8px;
    border-radius: 8px;
    box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
  }
}
</style>
