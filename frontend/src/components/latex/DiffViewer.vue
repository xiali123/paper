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
      <!-- 并排视图 -->
      <div v-if="viewMode === 'side-by-side'" class="side-by-side-view">
        <div class="diff-panel base-panel">
          <div class="panel-header">
            <span>{{ baseVersion?.summary || '基线版本' }}</span>
            <el-tag size="small">{{ baseVersion?.branchName }}</el-tag>
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
          </div>
        </div>

        <div class="diff-divider"></div>

        <div class="diff-panel compare-panel">
          <div class="panel-header">
            <span>{{ compareVersion?.summary || '对比版本' }}</span>
            <el-tag size="small" type="success">{{ compareVersion?.branchName }}</el-tag>
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
import { ref, computed, watch } from 'vue'
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

// 可用版本列表
const availableVersions = computed(() => {
  return props.versions.filter(v => !v.isAutoSave)
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

// 获取行的样式类
const getLineClass = (index: number, side: 'base' | 'compare') => {
  // 简单实现：可以根据diff结果添加类
  return ''
}

// 加载版本差异
const loadDiff = async () => {
  if (!baseVersionId.value || !compareVersionId.value) {
    return
  }

  if (baseVersionId.value === compareVersionId.value) {
    ElMessage.warning('请选择不同的版本进行对比')
    return
  }

  loading.value = true
  try {
    const result = await compareLatexVersions(baseVersionId.value, compareVersionId.value)

    // 解析内容为行
    baseLines.value = (baseVersion.value?.content || '').split('\n')
    compareLines.value = (compareVersion.value?.content || '').split('\n')

    // 计算差异统计
    diffStats.value = {
      additions: result.additions || 0,
      deletions: result.deletions || 0,
      modifications: result.modifications || 0
    }

    // 生成统一diff格式
    unifiedDiff.value = generateUnifiedDiff(baseLines.value, compareLines.value)
  } catch (error) {
    console.error('Failed to load diff:', error)
    ElMessage.error('加载版本对比失败')
  } finally {
    loading.value = false
  }
}

// 生成统一diff
const generateUnifiedDiff = (base: string[], compare: string[]) => {
  // 简单实现：逐行对比
  const chunks: any[] = []
  let currentChunk: any = {
    header: '@@ -1,' + base.length + ' +1,' + compare.length + ' @@',
    lines: []
  }

  const maxLines = Math.max(base.length, compare.length)

  for (let i = 0; i < maxLines; i++) {
    const baseLine = base[i]
    const compareLine = compare[i]

    if (baseLine === compareLine) {
      if (currentChunk.lines.length > 0) {
        chunks.push({ ...currentChunk })
        currentChunk = { header: '', lines: [] }
      }
      currentChunk.lines.push({
        type: 'context',
        prefix: ' ',
        content: baseLine || '',
        number: i + 1
      })
    } else if (baseLine && !compareLine) {
      currentChunk.lines.push({
        type: 'deleted',
        prefix: '-',
        content: baseLine,
        number: i + 1
      })
    } else if (!baseLine && compareLine) {
      currentChunk.lines.push({
        type: 'added',
        prefix: '+',
        content: compareLine,
        number: i + 1
      })
    } else {
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

        .panel-header {
          display: flex;
          justify-content: space-between;
          align-items: center;
          padding: 8px 12px;
          background: #f5f7fa;
          border-bottom: 1px solid #e4e7ed;
          font-size: 13px;
          font-weight: 500;
        }

        .panel-content {
          height: calc(100% - 40px);
          overflow: auto;
          font-family: 'Courier New', monospace;
          font-size: 12px;
          line-height: 1.6;
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
      }

      .panel-content {
        height: 100%;
        overflow: auto;
        font-family: 'Courier New', monospace;
        font-size: 12px;
        line-height: 1.6;
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

      &.diff-context {
        .line-content { color: #606266; }
      }

      .line-number {
        width: 40px;
        color: #909399;
        text-align: right;
        margin-right: 12px;
        user-select: none;
      }

      .line-prefix {
        width: 16px;
        text-align: center;
        margin-right: 8px;
        font-weight: bold;
      }

      .line-content {
        flex: 1;
        white-space: pre-wrap;
        word-break: break-all;
      }
    }

    .diff-chunk {
      .chunk-header {
        padding: 4px 8px;
        background: #f5f7fa;
        color: #909399;
        font-size: 12px;
        border-bottom: 1px solid #e4e7ed;
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
