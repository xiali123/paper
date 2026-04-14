<template>
  <div class="branch-tree-view" ref="containerRef">
    <svg ref="svgRef" class="tree-svg" :width="svgWidth" :height="svgHeight">
      <defs>
        <!-- 分支颜色渐变 -->
        <linearGradient v-for="color in branchColors" :key="color" :id="`gradient-${color}`">
          <stop offset="0%" :stop-color="color" stop-opacity="0.3"/>
          <stop offset="100%" :stop-color="color" stop-opacity="0.1"/>
        </linearGradient>
        <!-- 箭头标记 -->
        <marker id="arrowhead" markerWidth="10" markerHeight="10" refX="9" refY="3" orient="auto">
          <polygon points="0 0, 10 3, 0 6" fill="#909399"/>
        </marker>
      </defs>

      <!-- 连接线 -->
      <g class="connections">
        <path
          v-for="conn in connections"
          :key="`${conn.from}-${conn.to}`"
          :d="conn.path"
          :stroke="conn.color"
          stroke-width="2"
          fill="none"
          marker-end="url(#arrowhead)"
        />
      </g>

      <!-- 分支标签 -->
      <g class="branch-labels">
        <text
          v-for="branch in branches"
          :key="branch.name"
          :x="10"
          :y="branch.y"
          :fill="branch.color"
          font-size="12"
          font-weight="bold"
        >
          {{ branch.name }}
        </text>
      </g>

      <!-- 版本节点 -->
      <g class="nodes">
        <g
          v-for="node in layoutNodes"
          :key="node.id"
          :transform="`translate(${node.x}, ${node.y})`"
          class="node"
          :class="{
            selected: selectedVersion?.id === node.id,
            comparing: comparingVersions.has(node.id),
            'is-merge': node.isMerged,
            'is-auto-save': node.isAutoSave
          }"
          @click="handleNodeClick(node)"
        >
          <!-- 节点圆圈 -->
          <circle
            :r="nodeRadius"
            :fill="getNodeColor(node)"
            :stroke="getNodeStroke(node)"
            stroke-width="2"
          />

          <!-- 节点图标（合并/分支） -->
          <text
            v-if="node.isMerged"
            x="0"
            y="4"
            text-anchor="middle"
            font-size="10"
            fill="#fff"
          >⤝</text>

          <!-- 节点信息 -->
          <g v-if="showLabels" class="node-label">
            <rect
              :x="nodeRadius + 5"
              :y="-12"
              :width="labelWidth"
              height="24"
              rx="4"
              :fill="getNodeColor(node)"
              fill-opacity="0.9"
            />
            <text
              :x="nodeRadius + 10"
              y="4"
              font-size="11"
              fill="#fff"
            >
              {{ truncateText(node.summary, 20) }}
            </text>
          </g>
        </g>
      </g>
    </svg>

    <!-- 工具栏 -->
    <div class="toolbar">
      <el-button-group size="small">
        <el-button :icon="ZoomIn" @click="zoomIn" />
        <el-button :icon="ZoomOut" @click="zoomOut" />
        <el-button :icon="FullScreen" @click="fitToScreen" />
      </el-button-group>
      <el-switch v-model="showLabels" size="small" active-text="显示标签" />
    </div>

    <!-- 图例 -->
    <div class="legend">
      <div class="legend-item">
        <span class="legend-dot main-branch"></span>
        <span>主分支</span>
      </div>
      <div class="legend-item">
        <span class="legend-dot feature-branch"></span>
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
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
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

// 布局参数
const nodeRadius = 12
const nodeSpacing = { x: 80, y: 60 }
const labelWidth = 140
const svgWidth = ref(800)
const svgHeight = ref(600)

// 状态
const containerRef = ref<HTMLElement>()
const svgRef = ref<SVGSVGElement>()
const showLabels = ref(true)
const comparingVersions = ref<Set<string>>(new Set())

// 分支颜色
const branchColors = [
  '#409EFF', // blue - main
  '#67C23A', // green
  '#E6A23C', // orange
  '#F56C6C', // red
  '#909399', // gray
  '#9C27B0', // purple
  '#00BCD4', // cyan
  '#FF9800'  // amber
]

// 计算分支
const branches = computed(() => {
  const branchMap = new Map<string, { name: string; color: string; y: number }>()

  props.versions.forEach((v, index) => {
    if (!branchMap.has(v.branchName)) {
      branchMap.set(v.branchName, {
        name: v.branchName,
        color: branchColors[branchMap.size % branchColors.length],
        y: 30 + branchMap.size * 25
      })
    }
  })

  return Array.from(branchMap.values())
})

// 计算节点布局
const layoutNodes = computed(() => {
  const nodes: Array<FrontendLatexVersionNode & { x: number; y: number }> = []

  // 按时间排序
  const sortedVersions = [...props.versions].sort((a, b) =>
    new Date(a.timestamp).getTime() - new Date(b.timestamp).getTime()
  )

  // 为每个节点分配位置
  const branchY = new Map<string, number>()
  let currentX = 100

  sortedVersions.forEach((v, index) => {
    if (!branchY.has(v.branchName)) {
      const branchIndex = branches.value.findIndex(b => b.name === v.branchName)
      branchY.set(v.branchName, 80 + branchIndex * 80)
    }

    nodes.push({
      ...v,
      x: currentX,
      y: branchY.get(v.branchName)!
    })

    currentX += nodeSpacing.x
  })

  return nodes
})

// 计算连接线
const connections = computed(() => {
  const nodeMap = new Map(layoutNodes.value.map(n => [n.id, n]))
  const conns: Array<{ from: string; to: string; path: string; color: string }> = []

  layoutNodes.value.forEach(node => {
    if (node.parentId && nodeMap.has(node.parentId)) {
      const parent = nodeMap.get(node.parentId)!
      const branch = branches.value.find(b => b.name === node.branchName)

      // 绘制贝塞尔曲线连接
      const path = `M ${parent.x + nodeRadius} ${parent.y}
                    C ${parent.x + nodeSpacing.x / 2} ${parent.y},
                      ${node.x - nodeSpacing.x / 2} ${node.y},
                      ${node.x - nodeRadius} ${node.y}`

      conns.push({
        from: parent.id,
        to: node.id,
        path,
        color: branch?.color || '#909399'
      })
    }
  })

  return conns
})

// 获取节点颜色
const getNodeColor = (node: FrontendLatexVersionNode & { x: number; y: number }) => {
  if (node.isAutoSave) return '#E4E7ED'
  const branch = branches.value.find(b => b.name === node.branchName)
  return branch?.color || '#909399'
}

// 获取节点描边
const getNodeStroke = (node: FrontendLatexVersionNode & { x: number; y: number }) => {
  if (props.selectedVersion?.id === node.id) return '#409EFF'
  if (comparingVersions.value.has(node.id)) return '#E6A23C'
  if (node.isMerged) return '#67C23A'
  return '#fff'
}

// 截断文本
const truncateText = (text: string, maxLength: number) => {
  if (!text) return '未命名版本'
  return text.length > maxLength ? text.substring(0, maxLength) + '...' : text
}

// 处理节点点击
const handleNodeClick = (node: FrontendLatexVersionNode & { x: number; y: number }) => {
  emit('select', node)
}

// 缩放
const zoomIn = () => {
  svgWidth.value *= 1.2
  svgHeight.value *= 1.2
}

const zoomOut = () => {
  svgWidth.value /= 1.2
  svgHeight.value /= 1.2
}

const fitToScreen = () => {
  if (!containerRef.value) return

  const containerWidth = containerRef.value.clientWidth - 40
  const containerHeight = containerRef.value.clientHeight - 40

  const nodes = layoutNodes.value
  if (nodes.length === 0) return

  const maxX = Math.max(...nodes.map(n => n.x)) + 200
  const maxY = Math.max(...nodes.map(n => n.y)) + 100

  svgWidth.value = Math.max(containerWidth, maxX)
  svgHeight.value = Math.max(containerHeight, maxY)
}

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
  overflow: auto;
  background: #fafafa;

  .tree-svg {
    display: block;
    background: #fff;
    border-radius: 4px;
  }

  .node {
    cursor: pointer;
    transition: transform 0.2s;

    &:hover {
      transform: scale(1.1);
    }

    &.selected circle {
      stroke: #409EFF;
      stroke-width: 3;
    }

    &.comparing circle {
      stroke: #E6A23C;
      stroke-width: 3;
    }

    &.is-merge circle {
      stroke-dasharray: 3 2;
    }

    &.is-auto-save circle {
      opacity: 0.6;
    }

    .node-label {
      pointer-events: none;
      transition: opacity 0.2s;
    }
  }

  .toolbar {
    position: absolute;
    top: 10px;
    right: 10px;
    display: flex;
    gap: 8px;
    background: #fff;
    padding: 8px;
    border-radius: 4px;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  }

  .legend {
    position: absolute;
    bottom: 10px;
    left: 10px;
    display: flex;
    flex-direction: column;
    gap: 4px;
    background: #fff;
    padding: 8px 12px;
    border-radius: 4px;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);

    .legend-item {
      display: flex;
      align-items: center;
      gap: 6px;
      font-size: 12px;
    }

    .legend-dot {
      width: 12px;
      height: 12px;
      border-radius: 50%;

      &.main-branch { background: #409EFF; }
      &.feature-branch { background: #67C23A; }
      &.auto-save { background: #E4E7ED; }
      &.merged { background: #F56C6C; }
    }
  }
}
</style>
