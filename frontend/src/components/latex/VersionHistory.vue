<template>
  <div class="version-history">
    <!-- 头部操作栏 -->
    <div class="version-header">
      <div class="version-title">
        <el-icon><Clock /></el-icon>
        <span>版本历史</span>
      </div>
      <div class="version-actions">
        <el-button
          :icon="Refresh"
          @click="refreshVersions"
          :loading="loading"
          size="small"
        >
          刷新
        </el-button>
        <el-button
          :icon="DocumentAdd"
          @click="showSaveDialog = true"
          type="primary"
          size="small"
        >
          保存版本
        </el-button>
      </div>
    </div>

    <!-- 分支可视化视图 -->
    <div class="version-tree-view" v-if="viewMode === 'tree'">
      <div class="tree-canvas" ref="treeCanvas">
        <svg v-if="versionTree.length > 0" class="tree-svg">
          <!-- 绘制连接线 -->
          <g class="tree-lines">
            <path
              v-for="(line, index) in treeLines"
              :key="'line-' + index"
              :d="line.path"
              :stroke="line.color"
              stroke-width="2"
              fill="none"
            />
          </g>
          <!-- 绘制节点 -->
          <g class="tree-nodes">
            <g
              v-for="node in versionTree"
              :key="node.id"
              :transform="`translate(${node.x}, ${node.y})`"
              @click="selectVersion(node)"
              :class="['tree-node', { active: selectedVersion?.id === node.id, 'auto-save': node.isAutoSave }]"
            >
              <circle :r="node.depth === 0 ? 8 : 6" />
              <text x="12" y="4" class="node-label">{{ formatSummary(node.summary) }}</text>
              <text x="12" y="18" class="node-time">{{ formatTime(node.timestamp) }}</text>
            </g>
          </g>
        </svg>
        <el-empty v-else description="暂无版本历史" :image-size="80" />
      </div>
    </div>

    <!-- 列表视图 -->
    <div class="version-list-view" v-else>
      <el-table
        :data="versionHistory"
        stripe
        size="small"
        @row-click="selectVersion"
        highlight-current-row
      >
        <el-table-column width="40">
          <template #default="{ row }">
            <el-icon
              :color="row.depth === 0 ? '#409eff' : '#67c23a'"
              :size="16"
            >
              <component :is="row.depth === 0 ? 'Position' : 'Share'" />
            </el-icon>
          </template>
        </el-table-column>
        <el-table-column prop="summary" label="摘要" min-width="150">
          <template #default="{ row }">
            <span class="version-summary">{{ row.summary }}</span>
          </template>
        </el-table-column>
        <el-table-column prop="branchName" label="分支" width="100">
          <template #default="{ row }">
            <el-tag :type="row.depth === 0 ? 'primary' : 'success'" size="small">
              {{ row.branchName }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="timestamp" label="时间" width="140">
          <template #default="{ row }">
            {{ formatTime(row.timestamp) }}
          </template>
        </el-table-column>
        <el-table-column prop="author" label="作者" width="100" />
        <el-table-column prop="changeCount" label="变更" width="80" align="right">
          <template #default="{ row }">
            {{ row.changeCount > 0 ? row.changeCount + ' 字符' : '-' }}
          </template>
        </el-table-column>
        <el-table-column label="操作" width="180" fixed="right">
          <template #default="{ row }">
            <el-button
              size="small"
              @click.stop="previewVersion(row)"
              :icon="View"
              link
            >
              预览
            </el-button>
            <el-button
              size="small"
              @click.stop="restoreToVersion(row)"
              :icon="RefreshLeft"
              link
              type="warning"
            >
              恢复
            </el-button>
            <el-button
              size="small"
              @click.stop="compareVersion(row)"
              :icon="DocumentCopy"
              link
              type="info"
            >
              对比
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </div>

    <!-- 视图切换 -->
    <div class="view-toggle">
      <el-radio-group v-model="viewMode" size="small">
        <el-radio-button value="list">
          <el-icon><List /></el-icon>
          列表
        </el-radio-button>
        <el-radio-button value="tree">
          <el-icon><Share /></el-icon>
          分支图
        </el-radio-button>
      </el-radio-group>
    </div>

    <!-- 保存版本对话框 -->
    <el-dialog
      v-model="showSaveDialog"
      title="保存版本"
      width="400px"
    >
      <el-form :model="saveForm" label-width="80px">
        <el-form-item label="摘要">
          <el-input
            v-model="saveForm.summary"
            placeholder="请输入版本摘要（可选）"
            maxlength="100"
            show-word-limit
          />
        </el-form-item>
        <el-form-item label="自动保存">
          <el-switch v-model="saveForm.isAutoSave" />
          <span class="form-tip">自动保存版本不计入手动版本数</span>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showSaveDialog = false">取消</el-button>
        <el-button type="primary" @click="handleSaveVersion" :loading="saving">
          保存
        </el-button>
      </template>
    </el-dialog>

    <!-- 版本预览对话框 -->
    <el-dialog
      v-model="showPreviewDialog"
      :title="`版本预览 - ${previewVersion?.summary || '无摘要'}`"
      width="900px"
    >
      <div class="version-preview" v-if="previewVersion">
        <div class="preview-meta">
          <el-descriptions :column="3" border size="small">
            <el-descriptions-item label="版本ID">{{ previewVersion.id.slice(-8) }}</el-descriptions-item>
            <el-descriptions-item label="分支">{{ previewVersion.branchName }}</el-descriptions-item>
            <el-descriptions-item label="时间">{{ formatTime(previewVersion.timestamp) }}</el-descriptions-item>
            <el-descriptions-item label="作者">{{ previewVersion.author }}</el-descriptions-item>
            <el-descriptions-item label="行数">{{ previewVersion.totalLines }}</el-descriptions-item>
            <el-descriptions-item label="变更字符">{{ previewVersion.changeCount }}</el-descriptions-item>
          </el-descriptions>
        </div>
        <div class="preview-content">
          <el-input
            :model-value="previewVersion.content"
            type="textarea"
            :rows="15"
            readonly
            font-family="monospace"
          />
        </div>
      </div>
    </el-dialog>

    <!-- 版本对比对话框 -->
    <el-dialog
      v-model="showCompareDialog"
      title="版本对比"
      width="1000px"
    >
      <div class="version-compare" v-if="compareResult">
        <div class="compare-summary">
          <el-statistic title="新增行" :value="compareResult.additions">
            <template #prefix><el-icon color="#67c23a"><Plus /></el-icon></template>
          </el-statistic>
          <el-statistic title="删除行" :value="compareResult.deletions">
            <template #prefix><el-icon color="#f56c6c"><Minus /></el-icon></template>
          </el-statistic>
          <el-statistic title="修改行" :value="compareResult.modifications">
            <template #prefix><el-icon color="#e6a23c"><RefreshRight /></el-icon></template>
          </el-statistic>
          <el-statistic title="总变更" :value="compareResult.totalChanges" />
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'
import {
  Clock, Refresh, DocumentAdd, List, Share, View, RefreshLeft,
  DocumentCopy, Position, Plus, Minus, RefreshRight
} from '@element-plus/icons-vue'
import {
  getLatexVersionHistory,
  getLatexVersionTree,
  saveLatexVersion,
  restoreLatexVersion,
  compareLatexVersions,
  type FrontendLatexVersionNode
} from '@/api/adapters/latexAdapter'

// Props
const props = defineProps<{
  fileId: number
  projectId: number
  userId: string
  currentContent?: string
}>()

// Emits
const emit = defineEmits<{
  (e: 'restore', content: string): void
}>()

// State
const loading = ref(false)
const saving = ref(false)
const viewMode = ref<'list' | 'tree'>('list')
const versionHistory = ref<FrontendLatexVersionNode[]>([])
const versionTree = ref<FrontendLatexVersionNode[]>([])
const selectedVersion = ref<FrontendLatexVersionNode | null>(null)
const compareBaseVersion = ref<FrontendLatexVersionNode | null>(null)

// Dialogs
const showSaveDialog = ref(false)
const showPreviewDialog = ref(false)
const showCompareDialog = ref(false)

// Form
const saveForm = ref({
  summary: '',
  isAutoSave: false
})

// Preview & Compare
const previewVersion = ref<FrontendLatexVersionNode | null>(null)
const compareResult = ref<any>(null)

// Tree visualization
const treeCanvas = ref<HTMLElement>()
interface TreeNode {
  id: string
  x: number
  y: number
  depth: number
  summary: string
  timestamp: Date
  isAutoSave: boolean
}

interface TreeLine {
  path: string
  color: string
}

const treeLines = computed<TreeLine[]>(() => {
  const nodes = versionTree.value.map((v, i) => ({
    ...v,
    x: v.depth * 150 + 50,
    y: i * 60 + 40
  }))

  const lines: TreeLine[] = []
  for (const node of nodes) {
    if (node.parentId) {
      const parent = nodes.find(n => n.id === node.parentId)
      if (parent) {
        lines.push({
          path: `M ${parent.x} ${parent.y} L ${node.x} ${node.y}`,
          color: node.depth === 0 ? '#409eff' : '#67c23a'
        })
      }
    }
  }
  return lines
})

// Methods
const loadVersions = async () => {
  loading.value = true
  try {
    const [history, tree] = await Promise.all([
      getLatexVersionHistory(props.fileId, props.projectId, props.userId),
      getLatexVersionTree(props.fileId, props.projectId, props.userId)
    ])
    versionHistory.value = history
    versionTree.value = tree
  } catch (error) {
    console.error('Failed to load versions:', error)
  } finally {
    loading.value = false
  }
}

const refreshVersions = () => {
  loadVersions()
}

const selectVersion = (version: FrontendLatexVersionNode) => {
  selectedVersion.value = version
}

const formatSummary = (summary: string) => {
  return summary.length > 20 ? summary.slice(0, 20) + '...' : summary
}

const formatTime = (timestamp: Date) => {
  const date = new Date(timestamp)
  const now = new Date()
  const diff = now.getTime() - date.getTime()

  if (diff < 60000) return '刚刚'
  if (diff < 3600000) return Math.floor(diff / 60000) + '分钟前'
  if (diff < 86400000) return Math.floor(diff / 3600000) + '小时前'
  if (diff < 604800000) return Math.floor(diff / 86400000) + '天前'

  return date.toLocaleString('zh-CN', {
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit'
  })
}

const handleSaveVersion = async () => {
  if (!props.currentContent) {
    return
  }

  saving.value = true
  try {
    await saveLatexVersion({
      fileId: props.fileId,
      projectId: props.projectId,
      userId: props.userId,
      content: props.currentContent,
      summary: saveForm.value.summary,
      isAutoSave: saveForm.value.isAutoSave
    })

    showSaveDialog.value = false
    saveForm.value.summary = ''
    saveForm.value.isAutoSave = false

    await loadVersions()
  } catch (error) {
    console.error('Failed to save version:', error)
  } finally {
    saving.value = false
  }
}

const previewVersion = (version: FrontendLatexVersionNode) => {
  previewVersion.value = version
  showPreviewDialog.value = true
}

const restoreToVersion = async (version: FrontendLatexVersionNode) => {
  try {
    const result = await restoreLatexVersion({ versionId: version.id })
    emit('restore', result.content)
    await loadVersions()
  } catch (error) {
    console.error('Failed to restore version:', error)
  }
}

const compareVersion = async (version: FrontendLatexVersionNode) => {
  if (!compareBaseVersion.value) {
    compareBaseVersion.value = version
    return
  }

  if (compareBaseVersion.value.id === version.id) {
    return
  }

  try {
    const result = await compareLatexVersions(compareBaseVersion.value.id, version.id)
    compareResult.value = result
    showCompareDialog.value = true
    compareBaseVersion.value = null
  } catch (error) {
    console.error('Failed to compare versions:', error)
  }
}

// Lifecycle
onMounted(() => {
  loadVersions()
})

// Watch for file changes
watch(() => [props.fileId, props.projectId], () => {
  loadVersions()
})
</script>

<style scoped lang="scss">
.version-history {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #fff;
  border-radius: 8px;

  .version-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 16px;
    border-bottom: 1px solid #ebeef5;

    .version-title {
      display: flex;
      align-items: center;
      gap: 8px;
      font-weight: 500;
      color: #303133;
    }

    .version-actions {
      display: flex;
      gap: 8px;
    }
  }

  .version-tree-view {
    flex: 1;
    overflow: auto;
    padding: 16px;

    .tree-canvas {
      min-height: 300px;

      .tree-svg {
        width: 100%;
        height: 100%;

        .tree-nodes {
          .tree-node {
            cursor: pointer;
            transition: all 0.2s;

            &:hover {
              circle {
                stroke: #409eff;
                stroke-width: 2;
              }
            }

            &.active circle {
              stroke: #409eff;
              stroke-width: 3;
            }

            &.auto-save circle {
              stroke-dasharray: 2;
              stroke: #909399;
            }

            text {
              font-size: 12px;
              fill: #606266;

              &.node-label {
                font-weight: 500;
              }

              &.node-time {
                fill: #909399;
                font-size: 10px;
              }
            }
          }
        }
      }
    }
  }

  .version-list-view {
    flex: 1;
    overflow: auto;

    :deep(.el-table) {
      .el-table__row {
        cursor: pointer;

        &:hover {
          background-color: #f5f7fa;
        }
      }
    }
  }

  .view-toggle {
    padding: 8px 16px;
    border-top: 1px solid #ebeef5;
    display: flex;
    justify-content: center;
  }

  .form-tip {
    margin-left: 8px;
    font-size: 12px;
    color: #909399;
  }

  .version-preview {
    .preview-meta {
      margin-bottom: 16px;
    }

    .preview-content {
      :deep(.el-textarea__inner) {
        font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
        font-size: 13px;
        line-height: 1.6;
      }
    }
  }

  .version-compare {
    .compare-summary {
      display: flex;
      justify-content: space-around;
      padding: 20px 0;

      :deep(.el-statistic) {
        text-align: center;
      }
    }
  }
}
</style>
