<template>
  <div class="diff-viewer">
    <!-- 对比头部 -->
    <div class="diff-header">
      <div class="version-selectors">
        <div class="selector-item">
          <label>基线版本:</label>
          <el-select
            v-model="baseVersionId"
            placeholder="选择基线版本"
            @change="handleVersionChange"
          >
            <el-option
              v-for="v in availableVersions"
              :key="v.id"
              :label="formatVersionLabel(v)"
              :value="v.id"
            />
          </el-select>
        </div>
        <div class="selector-item">
          <label>对比版本:</label>
          <el-select
            v-model="compareVersionId"
            placeholder="选择对比版本"
            @change="handleVersionChange"
          >
            <el-option
              v-for="v in availableVersions"
              :key="v.id"
              :label="formatVersionLabel(v)"
              :value="v.id"
            />
          </el-select>
        </div>
      </div>

      <div class="diff-actions">
        <el-button-group size="small">
          <el-button :icon="RefreshLeft" @click="swapVersions">交换</el-button>
          <el-button :icon="Download" @click="downloadDiff">下载</el-button>
          <el-button :icon="Close" @click="$emit('clearCompare')">清除</el-button>
        </el-button-group>
      </div>
    </div>

    <!-- 对比统计 -->
    <div v-if="diffStats" class="diff-stats">
      <div class="stat-item added">
        <el-icon><Plus /></el-icon>
        <span>新增 {{ diffStats.additions }} 行</span>
      </div>
      <div class="stat-item removed">
        <el-icon><Minus /></el-icon>
        <span>删除 {{ diffStats.deletions }} 行</span>
      </div>
      <div class="stat-item modified">
        <el-icon><Edit /></el-icon>
        <span>修改 {{ diffStats.modifications }} 处</span>
      </div>
    </div>

    <!-- 对比视图 -->
    <div class="diff-content" ref="diffContainer">
      <!-- 加载状态 -->
      <div v-if="loading" class="loading-state">
        <el-skeleton :rows="10" animated />
      </div>

      <!-- 空状态 -->
      <div v-else-if="baseLines.length === 0 && compareLines.length === 0" class="empty-state">
        <el-empty description="版本内容为空或未选择版本" />
      </div>

      <!-- 并排视图 -->
      <div v-else-if="viewMode === 'side-by-side'" class="side-by-side-view">
        <div class="diff-panel base-panel">
          <div class="panel-header">
            <span>{{ baseVersion?.summary || '基线版本' }}</span>
            <el-tag size="small">{{ baseVersion?.branchName || 'main' }}</el-tag>
            <span class="line-count">{{ baseLines.length }} 行</span>
          </div>
          <div class="panel-content">
            <div
              v-for="(line, index) in baseLines"
              :key="`base-${index}`"
              class="diff-line"
              :class="getLineClass(index, 'base')"
            >
              <span class="line-number">{{ index + 1 }}</span>
              <span class="line-content">{{ line || ' ' }}</span>
            </div>
            <div v-if="baseLines.length === 0" class="no-content">无内容</div>
          </div>
        </div>

        <div class="diff-divider"></div>

        <div class="diff-panel compare-panel">
          <div class="panel-header">
            <span>{{ compareVersion?.summary || '对比版本' }}</span>
            <el-tag size="small" type="success">{{ compareVersion?.branchName || 'main' }}</el-tag>
            <span class="line-count">{{ compareLines.length }} 行</span>
          </div>
          <div class="panel-content">
            <div
              v-for="(line, index) in compareLines"
              :key="`compare-${index}`"
              class="diff-line"
              :class="getLineClass(index, 'compare')"
            >
              <span class="line-number">{{ index + 1 }}</span>
              <span class="line-content">{{ line || ' ' }}</span>
            </div>
            <div v-if="compareLines.length === 0" class="no-content">无内容</div>
          </div>
        </div>
      </div>

      <!-- 统一视图 -->
      <div v-else class="unified-view">
        <div class="diff-panel">
          <div class="panel-content">
            <div
              v-for="(chunk, chunkIndex) in unifiedDiff"
              :key="`chunk-${chunkIndex}`"
              class="diff-chunk"
            >
              <div class="chunk-header">{{ chunk.header }}</div>
              <div
                v-for="(line, lineIndex) in chunk.lines"
                :key="`line-${chunkIndex}-${lineIndex}`"
                class="diff-line"
                :class="`diff-${line.type}`"
              >
                <span class="line-number">{{ line.number }}</span>
                <span class="line-prefix">{{ line.prefix }}</span>
                <span class="line-content">{{ line.content }}</span>
              </div>
            </div>
            <div v-if="unifiedDiff.length === 0" class="no-content">无差异内容</div>
          </div>
        </div>
      </div>
    </div>

    <!-- 视图切换 -->
    <div class="view-mode-toggle">
      <el-radio-group v-model="viewMode" size="small">
        <el-radio-button value="side-by-side">
          并排视图
        </el-radio-button>
        <el-radio-button value="unified">
          统一视图
        </el-radio-button>
      </el-radio-group>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'
import { Plus, Minus, Edit, RefreshLeft, Download, Close } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import type { FrontendLatexVersionNode } from '@/api/adapters/latexAdapter'
import { compareLatexVersions } from '@/api/adapters/latexAdapter'

const props = defineProps<{
  versions: FrontendLatexVersionNode[]
  fileId: number
  projectId: number
  userId: string
}>()

const emit = defineEmits<{
  (e: 'clearCompare'): void
}>()

// 组件挂载时打印调试信息
onMounted(() => {
  console.log('[DiffViewer] Mounted with props:', {
    versionsCount: props.versions.length,
    versions: props.versions.map(v => ({
      id: v.id,
      summary: v.summary,
      hasContent: !!v.content,
      contentLength: v.content?.length || 0
    }))
  })
})

// 状态
const viewMode = ref<'side-by-side' | 'unified'>('side-by-side')
const baseVersionId = ref<string>('')
const compareVersionId = ref<string>('')
const loading = ref(false)

// 差异数据
const baseLines = ref<string[]>([])
const compareLines = ref<string[]>([])
const unifiedDiff = ref<any[]>([])
const diffStats = ref<{
  additions: number
  deletions: number
  modifications: number
} | null>(null)

// 可用版本列表（包含自动保存版本，以便有足够数据进行对比）
const availableVersions = computed(() => {
  return props.versions
})

// 选中的版本
const baseVersion = computed(() => {
  return availableVersions.value.find(v => v.id === baseVersionId.value)
})

const compareVersion = computed(() => {
  return availableVersions.value.find(v => v.id === compareVersionId.value)
})

// 格式化版本标签
const formatVersionLabel = (version: FrontendLatexVersionNode) => {
  const date = new Date(version.timestamp).toLocaleDateString('zh-CN')
  const summary = version.summary || '未命名'
  return `${date} - ${summary.substring(0, 20)}`
}

// 差异映射 - 用于并排视图高亮
const baseLineDiffMap = ref<Map<number, 'added' | 'deleted' | 'modified' | 'context'>>(new Map())
const compareLineDiffMap = ref<Map<number, 'added' | 'deleted' | 'modified' | 'context'>>(new Map())

// 获取行的样式类
const getLineClass = (index: number, side: 'base' | 'compare') => {
  const map = side === 'base' ? baseLineDiffMap.value : compareLineDiffMap.value
  const type = map.get(index)
  if (!type) return ''

  if (type === 'added') return 'diff-added'
  if (type === 'deleted') return 'diff-deleted'
  if (type === 'modified') return 'diff-modified'
  return ''
}

// 计算行差异映射（用于并排视图）
const computeLineDiffMaps = () => {
  const baseMap = new Map<number, 'added' | 'deleted' | 'modified' | 'context'>()
  const compareMap = new Map<number, 'added' | 'deleted' | 'modified' | 'context'>()

  const baseSet = new Set(baseLines.value)
  const compareSet = new Set(compareLines.value)

  // 标记base中的行
  baseLines.value.forEach((line, i) => {
    if (!compareSet.has(line)) {
      baseMap.set(i, 'deleted')
    } else {
      baseMap.set(i, 'context')
    }
  })

  // 标记compare中的行
  compareLines.value.forEach((line, i) => {
    if (!baseSet.has(line)) {
      compareMap.set(i, 'added')
    } else {
      compareMap.set(i, 'context')
    }
  })

  baseLineDiffMap.value = baseMap
  compareLineDiffMap.value = compareMap
}

// 加载版本差异
const loadDiff = async () => {
  if (!baseVersionId.value || !compareVersionId.value) {
    console.log('[DiffViewer] 版本ID未设置:', { baseVersionId: baseVersionId.value, compareVersionId: compareVersionId.value })
    return
  }

  if (baseVersionId.value === compareVersionId.value) {
    ElMessage.warning('请选择不同的版本进行对比')
    return
  }

  loading.value = true
  try {
    console.log('[DiffViewer] 加载差异:', {
      baseVersionId: baseVersionId.value,
      compareVersionId: compareVersionId.value,
      baseVersion: baseVersion.value,
      compareVersion: compareVersion.value
    })

    // 直接使用版本内容
    const baseContent = baseVersion.value?.content || ''
    const compareContent = compareVersion.value?.content || ''

    console.log('[DiffViewer] 版本内容:', {
      baseContentLength: baseContent.length,
      compareContentLength: compareContent.length,
      baseContentPreview: baseContent.substring(0, 100),
      compareContentPreview: compareContent.substring(0, 100)
    })

    if (!baseContent.trim() && !compareContent.trim()) {
      ElMessage.warning('版本内容为空，无法对比')
      baseLines.value = []
      compareLines.value = []
      loading.value = false
      return
    }

    // 解析内容为行
    baseLines.value = baseContent === '' ? [] : baseContent.split('\n')
    compareLines.value = compareContent === '' ? [] : compareContent.split('\n')

    console.log('[DiffViewer] 解析后的行数:', {
      baseLines: baseLines.value.length,
      compareLines: compareLines.value.length
    })

    // 计算差异统计（简单实现）
    const baseSet = new Set(baseLines.value)
    const compareSet = new Set(compareLines.value)

    let additions = 0
    let deletions = 0

    compareLines.value.forEach(line => {
      if (!baseSet.has(line)) additions++
    })

    baseLines.value.forEach(line => {
      if (!compareSet.has(line)) deletions++
    })

    const modifications = Math.min(additions, deletions)
    diffStats.value = {
      additions: additions - modifications,
      deletions: deletions - modifications,
      modifications
    }

    console.log('[DiffViewer] 差异统计:', diffStats.value)

    // 生成统一diff格式
    unifiedDiff.value = generateUnifiedDiff(baseLines.value, compareLines.value)
    console.log('[DiffViewer] 生成的diff块数:', unifiedDiff.value.length)

    // 计算并排视图的差异映射
    computeLineDiffMaps()
  } catch (error) {
    console.error('Failed to load diff:', error)
    ElMessage.error('加载版本对比失败')
  } finally {
    loading.value = false
  }
}

// 生成统一diff
const generateUnifiedDiff = (base: string[], compare: string[]) => {
  const chunks: any[] = []

  // 如果两个内容完全相同且为空
  if (base.length === 0 && compare.length === 0) {
    return [{
      header: '@@ -0,0 +0,0 @@',
      lines: [{
        type: 'context',
        prefix: ' ',
        content: '(无内容)',
        number: 0
      }]
    }]
  }

  // 如果两个内容完全相同
  if (base.length === compare.length && base.every((line, i) => line === compare[i])) {
    return [{
      header: `@@ -1,${base.length} +1,${compare.length} @@`,
      lines: base.map((line, i) => ({
        type: 'context',
        prefix: ' ',
        content: line || '',
        number: i + 1
      }))
    }]
  }

  // 简单实现：逐行对比
  let currentChunk: any = {
    header: '@@ -1,' + base.length + ' +1,' + compare.length + ' @@',
    lines: []
  }

  const maxLines = Math.max(base.length, compare.length)

  for (let i = 0; i < maxLines; i++) {
    const baseLine = base[i]
    const compareLine = compare[i]

    if (baseLine === compareLine) {
      // 相同行
      currentChunk.lines.push({
        type: 'context',
        prefix: ' ',
        content: baseLine || '',
        number: i + 1
      })
    } else if (baseLine && !compareLine) {
      // 删除行
      currentChunk.lines.push({
        type: 'deleted',
        prefix: '-',
        content: baseLine,
        number: i + 1
      })
    } else if (!baseLine && compareLine) {
      // 新增行
      currentChunk.lines.push({
        type: 'added',
        prefix: '+',
        content: compareLine,
        number: i + 1
      })
    } else {
      // 修改行
      currentChunk.lines.push({
        type: 'deleted',
        prefix: '-',
        content: baseLine,
        number: i + 1
      })
      currentChunk.lines.push({
        type: 'added',
        prefix: '+',
        content: compareLine,
        number: i + 1
      })
    }
  }

  if (currentChunk.lines.length > 0) {
    chunks.push(currentChunk)
  }

  return chunks
}

// 交换版本
const swapVersions = () => {
  const temp = baseVersionId.value
  baseVersionId.value = compareVersionId.value
  compareVersionId.value = temp
  loadDiff()
}

// 下载差异
const downloadDiff = () => {
  if (!unifiedDiff.value.length) {
    ElMessage.warning('没有差异内容')
    return
  }

  let content = ''
  unifiedDiff.value.forEach(chunk => {
    content += chunk.header + '\n'
    chunk.lines.forEach(line => {
      content += line.prefix + line.content + '\n'
    })
  })

  const blob = new Blob([content], { type: 'text/plain' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = `diff-${baseVersionId.value?.slice(0, 8)}-${compareVersionId.value?.slice(0, 8)}.txt`
  link.click()
  URL.revokeObjectURL(url)
}

// 处理版本变化
const handleVersionChange = () => {
  loadDiff()
}

// 监听props.versions变化
watch(() => props.versions, (newVersions) => {
  if (newVersions.length >= 2) {
    // 自动设置基线和对比版本
    baseVersionId.value = newVersions[newVersions.length - 2].id
    compareVersionId.value = newVersions[newVersions.length - 1].id
    loadDiff()
  }
}, { immediate: true })
</script>

<style scoped lang="scss">
.diff-viewer {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #f5f7fa;

  .diff-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 16px;
    background: #fff;
    border-bottom: 1px solid #e4e7ed;

    .version-selectors {
      display: flex;
      gap: 16px;
      flex: 1;

      .selector-item {
        display: flex;
        align-items: center;
        gap: 8px;

        label {
          font-size: 13px;
          color: #606266;
          white-space: nowrap;
        }

        .el-select {
          width: 200px;
        }
      }
    }
  }

  .diff-stats {
    display: flex;
    gap: 16px;
    padding: 12px 16px;
    background: #fff;
    border-bottom: 1px solid #e4e7ed;

    .stat-item {
      display: flex;
      align-items: center;
      gap: 4px;
      font-size: 13px;

      &.added { color: #67C23A; }
      &.removed { color: #F56C6C; }
      &.modified { color: #E6A23C; }
    }
  }

  .diff-content {
    flex: 1;
    overflow: auto;
    padding: 16px;

    .loading-state,
    .empty-state {
      display: flex;
      align-items: center;
      justify-content: center;
      height: 100%;
      min-height: 300px;
    }

    .side-by-side-view {
      display: flex;
      height: 100%;
      gap: 16px;

      .diff-panel {
        flex: 1;
        background: #fff;
        border: 1px solid #e4e7ed;
        border-radius: 4px;
        overflow: hidden;
        display: flex;
        flex-direction: column;

        .panel-header {
          display: flex;
          justify-content: space-between;
          align-items: center;
          gap: 8px;
          padding: 8px 12px;
          background: #f5f7fa;
          border-bottom: 1px solid #e4e7ed;
          font-size: 13px;
          font-weight: 500;

          .line-count {
            font-size: 11px;
            color: #909399;
          }
        }

        .panel-content {
          flex: 1;
          overflow: auto;
          font-family: 'Courier New', monospace;
          font-size: 12px;
          line-height: 1.6;
        }

        .no-content {
          padding: 40px;
          text-align: center;
          color: #909399;
          font-style: italic;
        }
      }

      .diff-divider {
        width: 1px;
        background: #e4e7ed;
      }
    }

    .unified-view {
      height: 100%;

      .diff-panel {
        height: 100%;
        background: #fff;
        border: 1px solid #e4e7ed;
        border-radius: 4px;
        overflow: hidden;
        display: flex;
        flex-direction: column;
      }

      .panel-content {
        flex: 1;
        overflow: auto;
        font-family: 'Courier New', monospace;
        font-size: 12px;
        line-height: 1.6;
      }

      .no-content {
        padding: 40px;
        text-align: center;
        color: #909399;
        font-style: italic;
      }
    }

    .diff-line {
      display: flex;
      padding: 2px 8px;

      &:hover {
        background: #f5f7fa;
      }

      &.diff-added {
        background: #f0f9ff;
        .line-content { color: #67C23A; }
      }

      &.diff-deleted {
        background: #fef0f0;
        .line-content { color: #F56C6C; }
      }

      &.diff-modified {
        background: #fff7e6;
        .line-content { color: #E6A23C; }
      }

      &.diff-context {
        .line-content { color: #606266; }
      }

      .line-number {
        width: 40px;
        color: #909399;
        text-align: right;
        margin-right: 12px;
        user-select: none;
        flex-shrink: 0;
      }

      .line-prefix {
        width: 16px;
        text-align: center;
        margin-right: 8px;
        font-weight: bold;
        flex-shrink: 0;
      }

      .line-content {
        flex: 1;
        white-space: pre-wrap;
        word-break: break-all;
        min-width: 0;
      }
    }

    .diff-chunk {
      .chunk-header {
        padding: 4px 8px;
        background: #f5f7fa;
        color: #909399;
        font-size: 12px;
        border-bottom: 1px solid #e4e7ed;
        font-weight: 500;
      }
    }
  }

  .view-mode-toggle {
    display: flex;
    justify-content: center;
    padding: 8px;
    background: #fff;
    border-top: 1px solid #e4e7ed;
  }
}
</style>
