<template>
  <div class="version-detail">
    <!-- 版本元信息 -->
    <div class="detail-section">
      <h3>版本信息</h3>
      <el-descriptions :column="2" border size="small">
        <el-descriptions-item label="版本ID">
          <el-tag size="small">{{ version.id.slice(-8) }}</el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="分支">
          <el-tag :type="version.branchName === 'main' ? 'primary' : 'success'" size="small">
            {{ version.branchName }}
          </el-tag>
        </el-descriptions-item>
        <el-descriptions-item label="创建时间">
          {{ formatTime(version.timestamp) }}
        </el-descriptions-item>
        <el-descriptions-item label="作者">
          {{ version.author }}
        </el-descriptions-item>
        <el-descriptions-item label="行数">
          {{ version.totalLines }}
        </el-descriptions-item>
        <el-descriptions-item label="变更数">
          {{ version.changeCount }}
        </el-descriptions-item>
        <el-descriptions-item label="版本位置" :span="2">
          v{{ version.position }} (深度: {{ version.depth }})
        </el-descriptions-item>
        <el-descriptions-item label="摘要" :span="2">
          {{ version.summary || '无摘要' }}
        </el-descriptions-item>
      </el-descriptions>
    </div>

    <!-- 标签 -->
    <div class="detail-section">
      <h3>标签</h3>
      <div class="tags">
        <el-tag v-if="version.isAutoSave" type="info" size="small">自动保存</el-tag>
        <el-tag v-if="version.isMerged" type="success" size="small">已合并</el-tag>
        <el-tag v-if="version.branchName === 'main'" type="primary" size="small">主分支</el-tag>
        <el-tag v-else type="success" size="small">特性分支</el-tag>
      </div>
    </div>

    <!-- 操作按钮 -->
    <div class="detail-section actions">
      <el-button-group>
        <el-button @click="handleRestore" type="warning" :icon="RefreshLeft">
          恢复此版本
        </el-button>
        <el-button @click="handleCopyContent" :icon="CopyDocument">
          复制内容
        </el-button>
        <el-button @click="handleDownload" :icon="Download">
          下载
        </el-button>
      </el-button-group>
    </div>

    <!-- 内容预览 -->
    <div class="detail-section content-section">
      <div class="section-header">
        <h3>内容预览</h3>
        <el-switch v-model="showLineNumbers" size="small" active-text="显示行号" />
      </div>

      <div class="content-preview" ref="contentRef">
        <div
          v-for="(line, index) in contentLines"
          :key="index"
          class="content-line"
          :class="{ highlighted: highlightedLine === index }"
        >
          <span v-if="showLineNumbers" class="line-number">{{ index + 1 }}</span>
          <span class="line-content">{{ line || ' ' }}</span>
        </div>
      </div>
    </div>

    <!-- 版本关系 -->
    <div v-if="version.parentId" class="detail-section">
      <h3>版本关系</h3>
      <div class="version-graph">
        <div class="graph-node parent">
          <div class="node-label">父版本</div>
          <el-tag size="small">{{ version.parentId.slice(-8) }}</el-tag>
        </div>
        <div class="graph-arrow">↓</div>
        <div class="graph-node current">
          <div class="node-label">当前版本</div>
          <el-tag size="small" type="primary">{{ version.id.slice(-8) }}</el-tag>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { RefreshLeft, CopyDocument, Download } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import type { FrontendLatexVersionNode } from '@/api/adapters/latexAdapter'

const props = defineProps<{
  version: FrontendLatexVersionNode
  currentContent?: string
}>()

const emit = defineEmits<{
  (e: 'restore', version: FrontendLatexVersionNode): void
}>()

// 状态
const showLineNumbers = ref(true)
const highlightedLine = ref<number | null>(null)
const contentRef = ref<HTMLElement>()

// 计算属性
const contentLines = computed(() => {
  return (props.version.content || '').split('\n')
})

// 格式化时间
const formatTime = (timestamp: Date) => {
  const date = new Date(timestamp)
  return date.toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit'
  })
}

// 处理恢复
const handleRestore = () => {
  emit('restore', props.version)
}

// 复制内容
const handleCopyContent = () => {
  navigator.clipboard.writeText(props.version.content || '')
    .then(() => {
      ElMessage.success('内容已复制到剪贴板')
    })
    .catch(() => {
      ElMessage.error('复制失败')
    })
}

// 下载内容
const handleDownload = () => {
  const blob = new Blob([props.version.content || ''], { type: 'text/plain' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = `version-${props.version.id.slice(-8)}.tex`
  link.click()
  URL.revokeObjectURL(url)
  ElMessage.success('下载已开始')
}
</script>

<style scoped lang="scss">
.version-detail {
  padding: 20px;

  .detail-section {
    margin-bottom: 24px;

    h3 {
      margin: 0 0 12px 0;
      font-size: 16px;
      font-weight: 600;
      color: #303133;
    }

    &.actions {
      display: flex;
      justify-content: center;
      padding: 16px 0;
      background: #f5f7fa;
      border-radius: 4px;
    }

    &.content-section {
      .section-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 12px;
      }
    }

    .tags {
      display: flex;
      gap: 8px;
      flex-wrap: wrap;
    }
  }

  .content-preview {
    max-height: 400px;
    overflow: auto;
    background: #f5f7fa;
    border: 1px solid #e4e7ed;
    border-radius: 4px;
    padding: 12px;
    font-family: 'Courier New', monospace;
    font-size: 13px;
    line-height: 1.6;

    .content-line {
      display: flex;
      padding: 2px 4px;

      &:hover {
        background: #e4e7ed;
      }

      &.highlighted {
        background: #fff7e6;
      }

      .line-number {
        width: 40px;
        color: #909399;
        text-align: right;
        margin-right: 12px;
        user-select: none;
      }

      .line-content {
        flex: 1;
        white-space: pre-wrap;
        word-break: break-all;
      }
    }
  }

  .version-graph {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 16px;
    padding: 20px;

    .graph-node {
      text-align: center;

      .node-label {
        font-size: 12px;
        color: #909399;
        margin-bottom: 4px;
      }

      &.current .el-tag {
        background: #409EFF;
        border-color: #409EFF;
        color: #fff;
      }
    }

    .graph-arrow {
      font-size: 20px;
      color: #909399;
    }
  }
}
</style>
