<template>
  <div class="diff-viewer">
    <!-- 对比头部 -->
    <div class="diff-header">
      <div class="version-selectors">
        <div class="selector-main">
          <div class="selector-item">
            <label class="selector-label">
              <el-icon class="label-icon"><Files /></el-icon>
              选择对比版本 (可多选)
            </label>
            <el-select
              v-model="selectedVersionIds"
              multiple
              collapse-tags
              collapse-tags-tooltip
              placeholder="选择要对比的版本..."
              @change="handleVersionsChange"
              class="version-select-multiple"
              max-collapse-tags="3"
            >
              <el-option
                v-for="v in availableVersions"
                :key="v.id"
                :label="formatVersionLabel(v)"
                :value="v.id"
              >
                <div class="version-option">
                  <div class="option-header">
                    <span class="option-summary">{{ v.summary || '未命名版本' }}</span>
                    <el-tag v-if="v.branchName !== 'main'" size="small" type="success">{{ v.branchName }}</el-tag>
                    <el-tag v-if="v.isAutoSave" size="small" type="info">自动</el-tag>
                  </div>
                  <div class="option-meta">
                    <span class="option-time">{{ formatTimeRelative(v.timestamp) }}</span>
                    <span class="option-author">{{ v.author }}</span>
                    <span class="option-lines">{{ v.totalLines }}行</span>
                  </div>
                </div>
              </el-option>
            </el-select>
          </div>
        </div>

        <!-- 快捷选择按钮 -->
        <div class="quick-select">
          <el-button size="small" @click="selectLatestTwo" :disabled="availableVersions.length < 2">
            最新2个
          </el-button>
          <el-button size="small" @click="selectLatestThree" :disabled="availableVersions.length < 3">
            最新3个
          </el-button>
          <el-button size="small" @click="selectAllManual" :disabled="availableVersions.length === 0">
            全部手动
          </el-button>
          <el-button size="small" @click="clearSelection" :disabled="selectedVersionIds.length === 0">
            <el-icon><Close /></el-icon>
          </el-button>
        </div>
      </div>

      <div class="diff-actions">
        <div class="selection-info">
          <el-tag size="small" type="info">已选择 {{ selectedVersionIds.length }} 个版本</el-tag>
          <span v-if="selectedVersionIds.length >= 2" class="compare-hint">
            下方显示多版本对比内容
          </span>
        </div>
        <el-button-group size="small">
          <el-tooltip content="下载对比" placement="top">
            <el-button :icon="Download" @click="downloadDiff">下载</el-button>
          </el-tooltip>
          <el-tooltip content="清除对比" placement="top">
            <el-button :icon="Close" @click="$emit('clearCompare')">关闭</el-button>
          </el-tooltip>
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
      <div class="stat-item versions">
        <el-icon><Files /></el-icon>
        <span>{{ selectedVersionIds.length }} 个版本</span>
      </div>
    </div>

    <!-- 多版本对比视图 -->
    <div class="diff-content" ref="diffContainer">
      <!-- 加载状态 -->
      <div v-if="loading" class="loading-state">
        <el-skeleton :rows="10" animated />
      </div>

      <!-- 空状态 -->
      <div v-else-if="selectedVersions.length === 0" class="empty-state">
        <el-empty description="请选择要对比的版本">
          <el-button type="primary" @click="selectLatestTwo">快速选择最新2个版本</el-button>
        </el-empty>
      </div>

      <!-- 多版本并排视图 -->
      <div v-else class="multi-version-view">
        <div class="versions-header">
          <div class="version-cell header-cell row-number">#</div>
          <div
            v-for="(version, idx) in selectedVersions"
            :key="version.id"
            class="version-cell header-cell version-name"
          >
            <div class="version-info">
              <el-tag :type="getVersionTagType(idx)" size="small">
                V{{ idx + 1 }}
              </el-tag>
              <span class="version-summary">{{ version.summary?.substring(0, 20) || '未命名' }}</span>
            </div>
            <div class="version-meta">
              <span>{{ formatTimeRelative(version.timestamp) }}</span>
            </div>
          </div>
        </div>

        <div class="versions-content">
          <div
            v-for="line in maxLines"
            :key="line"
            class="version-row"
            :class="{ 'has-diff': hasDifference(line) }"
          >
            <div class="version-cell row-number">{{ line }}</div>
            <div
              v-for="(version, idx) in selectedVersions"
              :key="version.id"
              class="version-cell line-content"
              :class="getLineClassForVersion(line, idx)"
              :title="getVersionLine(version, line)"
            >
              <span class="content-text">{{ getVersionLine(version, line) || ' ' }}</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'
import { Plus, Minus, Download, Close, Files } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import type { FrontendLatexVersionNode } from '@/api/adapters/latexAdapter'

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
  if (import.meta.env.DEV) console.log('[DiffViewer] Mounted with props:', {
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
const loading = ref(false)
const selectedVersionIds = ref<string[]>([])

// 选中的版本
const selectedVersions = computed(() => {
  return props.versions.filter(v => selectedVersionIds.value.includes(v.id))
})

// 可用版本列表
const availableVersions = computed(() => {
  return props.versions
})

// 版本内容行缓存
const versionLinesCache = ref<Map<string, string[]>>(new Map())

// 最大行数
const maxLines = computed(() => {
  if (selectedVersions.value.length === 0) return 0
  return Math.max(...selectedVersions.value.map(v => getVersionLines(v).length))
})

// 差异统计
const diffStats = computed(() => {
  if (selectedVersions.value.length < 2) return null

  const linesArray = selectedVersions.value.map(v => getVersionLines(v))
  const allLines = new Set<string>()
  linesArray.forEach(lines => lines.forEach(line => allLines.add(line)))

  const firstLines = new Set(linesArray[0])
  let additions = 0
  let deletions = 0

  for (let i = 1; i < linesArray.length; i++) {
    const currentSet = new Set(linesArray[i])
    linesArray[i].forEach(line => {
      if (!firstLines.has(line)) additions++
    })
    firstLines.forEach(line => {
      if (!currentSet.has(line)) deletions++
    })
  }

  return { additions, deletions, modifications: 0 }
})

// 获取版本标签类型
const getVersionTagType = (idx: number) => {
  const total = selectedVersions.value.length
  if (idx === 0) return 'danger' // 最早版本 - 红色
  if (idx === total - 1) return 'success' // 最新版本 - 绿色
  return 'primary' // 中间版本 - 蓝色
}

// 获取版本内容行
const getVersionLines = (version: FrontendLatexVersionNode): string[] => {
  if (!versionLinesCache.value.has(version.id)) {
    const content = version.content || ''
    versionLinesCache.value.set(version.id, content === '' ? [] : content.split('\n'))
  }
  return versionLinesCache.value.get(version.id)!
}

// 获取指定行的内容
const getVersionLine = (version: FrontendLatexVersionNode, line: number): string => {
  const lines = getVersionLines(version)
  return lines[line - 1] || ''
}

// 检查某行是否有差异
const hasDifference = (line: number): boolean => {
  if (selectedVersions.value.length < 2) return false
  const firstLine = getVersionLine(selectedVersions.value[0], line)
  return selectedVersions.value.some(v => getVersionLine(v, line) !== firstLine)
}

// 获取行的样式类
const getLineClassForVersion = (line: number, versionIdx: number): string => {
  if (selectedVersions.value.length < 2) return ''

  const currentLine = getVersionLine(selectedVersions.value[versionIdx], line)
  const firstLine = getVersionLine(selectedVersions.value[0], line)

  if (currentLine !== firstLine) {
    // 检查是否在其他版本中出现过
    const appearsInOther = selectedVersions.value.some((v, idx) =>
      idx !== versionIdx && getVersionLine(v, line) === currentLine
    )
    return appearsInOther ? 'diff-modified' : 'diff-added'
  }

  // 检查是否仅在第一个版本中存在
  const onlyInFirst = selectedVersions.value.slice(1).every(v =>
    getVersionLine(v, line) !== currentLine
  )
  if (versionIdx === 0 && onlyInFirst && currentLine) {
    return 'diff-deleted'
  }

  return ''
}

// 格式化版本标签
const formatVersionLabel = (version: FrontendLatexVersionNode) => {
  const date = new Date(version.timestamp).toLocaleDateString('zh-CN')
  const summary = version.summary || '未命名'
  return `${date} - ${summary.substring(0, 20)}`
}

// 格式化相对时间
const formatTimeRelative = (timestamp: Date) => {
  const now = new Date()
  const diff = now.getTime() - new Date(timestamp).getTime()
  const minutes = Math.floor(diff / 60000)
  const hours = Math.floor(diff / 3600000)
  const days = Math.floor(diff / 86400000)

  if (minutes < 1) return '刚刚'
  if (minutes < 60) return `${minutes}分钟前`
  if (hours < 24) return `${hours}小时前`
  if (days < 7) return `${days}天前`
  return new Date(timestamp).toLocaleDateString('zh-CN')
}

// 快捷选择：最新2个
const selectLatestTwo = () => {
  const nonAutoSave = availableVersions.value.filter(v => !v.isAutoSave)
  if (nonAutoSave.length >= 2) {
    selectedVersionIds.value = [nonAutoSave[1].id, nonAutoSave[0].id]
  } else if (availableVersions.value.length >= 2) {
    selectedVersionIds.value = [availableVersions.value[1].id, availableVersions.value[0].id]
  }
}

// 快捷选择：最新3个
const selectLatestThree = () => {
  const nonAutoSave = availableVersions.value.filter(v => !v.isAutoSave)
  if (nonAutoSave.length >= 3) {
    selectedVersionIds.value = [nonAutoSave[2].id, nonAutoSave[1].id, nonAutoSave[0].id]
  } else if (availableVersions.value.length >= 3) {
    selectedVersionIds.value = [
      availableVersions.value[2].id,
      availableVersions.value[1].id,
      availableVersions.value[0].id
    ]
  }
}

// 快捷选择：全部手动保存版本
const selectAllManual = () => {
  const manualVersions = availableVersions.value.filter(v => !v.isAutoSave)
  if (manualVersions.length > 5) {
    ElMessage.warning(`手动版本过多(${manualVersions.length}个)，仅选择最新5个`)
    selectedVersionIds.value = manualVersions.slice(0, 5).map(v => v.id).reverse()
  } else if (manualVersions.length > 0) {
    selectedVersionIds.value = manualVersions.map(v => v.id).reverse()
  }
}

// 清空选择
const clearSelection = () => {
  selectedVersionIds.value = []
}

// 处理版本变化
const handleVersionsChange = () => {
  if (import.meta.env.DEV) console.log('[DiffViewer] Selected versions:', selectedVersionIds.value)
  // 清空缓存以重新计算
  versionLinesCache.value.clear()
}

// 下载差异
const downloadDiff = () => {
  if (selectedVersions.value.length === 0) {
    ElMessage.warning('请先选择版本')
    return
  }

  let content = `多版本对比 (${selectedVersions.value.length}个版本)\n`
  content += `${'='.repeat(50)}\n\n`

  selectedVersions.value.forEach((v, idx) => {
    content += `版本${idx + 1}: ${v.summary || '未命名'}\n`
    content += `时间: ${formatTimeRelative(v.timestamp)}\n`
    content += `行数: ${v.totalLines}\n`
    content += `${'-'.repeat(30)}\n`
  })

  const blob = new Blob([content], { type: 'text/plain;charset=utf-8' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = `multi-version-diff-${Date.now()}.txt`
  link.click()
  URL.revokeObjectURL(url)
  ElMessage.success('下载已开始')
}

// 监听props.versions变化
watch(() => props.versions, (newVersions) => {
  if (newVersions.length >= 2 && selectedVersionIds.value.length === 0) {
    // 自动选择最新2个版本
    selectLatestTwo()
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
    align-items: flex-start;
    gap: 16px;
    padding: 16px;
    background: #fff;
    border-bottom: 1px solid #e4e7ed;

    .version-selectors {
      display: flex;
      flex-direction: column;
      gap: 12px;
      flex: 1;

      .selector-main {
        .selector-item {
          display: flex;
          flex-direction: column;
          gap: 8px;

          .selector-label {
            display: flex;
            align-items: center;
            gap: 6px;
            font-size: 14px;
            font-weight: 500;
            color: #303133;

            .label-icon {
              font-size: 16px;
              color: #409EFF;
            }
          }

          .version-select-multiple {
            width: 100%;
            max-width: 600px;
          }
        }
      }

      .quick-select {
        display: flex;
        gap: 8px;
        flex-wrap: wrap;
      }
    }

    .diff-actions {
      display: flex;
      align-items: center;
      gap: 16px;
      flex-shrink: 0;

      .selection-info {
        display: flex;
        align-items: center;
        gap: 8px;

        .compare-hint {
          font-size: 12px;
          color: #909399;
        }
      }
    }
  }

  // 版本选项样式
  :deep(.version-option) {
    display: flex;
    flex-direction: column;
    gap: 4px;
    padding: 4px 0;

    .option-header {
      display: flex;
      align-items: center;
      gap: 8px;

      .option-summary {
        flex: 1;
        font-size: 13px;
        color: #303133;
        font-weight: 500;
      }
    }

    .option-meta {
      display: flex;
      gap: 12px;
      font-size: 11px;
      color: #909399;

      .option-time,
      .option-author,
      .option-lines {
        &:not(:last-child)::after {
          content: '·';
          margin-left: 12px;
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
      &.versions { color: #409EFF; }
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

    .multi-version-view {
      background: #fff;
      border: 1px solid #e4e7ed;
      border-radius: 4px;
      overflow: hidden;

      .versions-header {
        display: flex;
        position: sticky;
        top: 0;
        z-index: 10;
        background: #f5f7fa;
        border-bottom: 2px solid #e4e7ed;

        .version-cell {
          padding: 12px 8px;
          font-size: 12px;
          font-weight: 600;
          color: #606266;
          text-align: center;
          border-right: 1px solid #e4e7ed;

          &.row-number {
            width: 60px;
            min-width: 60px;
            background: #909399;
            color: #fff;
          }

          &.version-name {
            flex: 1;
            min-width: 150px;
            padding: 8px;

            .version-info {
              display: flex;
              align-items: center;
              gap: 6px;
              margin-bottom: 4px;

              .version-summary {
                font-size: 13px;
                font-weight: 500;
              }
            }

            .version-meta {
              font-size: 11px;
              color: #909399;
              font-weight: normal;
            }
          }

          &:last-child {
            border-right: none;
          }
        }
      }

      .versions-content {
        .version-row {
          display: flex;
          border-bottom: 1px solid #f0f0f0;

          &:hover {
            background: #f5f7fa;
          }

          &.has-diff {
            background: #fff9e6;
          }

          .version-cell {
            padding: 4px 8px;
            font-family: 'Courier New', monospace;
            font-size: 12px;
            line-height: 1.6;
            border-right: 1px solid #f0f0f0;
            white-space: pre;
            overflow: hidden;
            text-overflow: ellipsis;

            &.row-number {
              width: 60px;
              min-width: 60px;
              text-align: center;
              color: #909399;
              background: #fafafa;
              font-weight: 500;
              user-select: none;
            }

            &.line-content {
              flex: 1;
              min-width: 150px;

              .content-text {
                word-break: break-all;
              }

              &.diff-added {
                background: #f0f9ff;
                color: #67C23A;
              }

              &.diff-deleted {
                background: #fef0f0;
                color: #F56C6C;
              }

              &.diff-modified {
                background: #fff7e6;
                color: #E6A23C;
              }
            }

            &:last-child {
              border-right: none;
            }
          }
        }
      }
    }
  }
}
</style>
