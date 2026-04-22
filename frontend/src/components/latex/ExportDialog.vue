<template>
  <el-dialog
    v-model="visible"
    title="导出文档"
    width="500px"
    :close-on-click-modal="false"
  >
    <div class="export-dialog">
      <!-- 导出格式选择 -->
      <div class="export-section">
        <h4>选择格式</h4>
        <div class="format-grid">
          <div
            v-for="format in exportFormats"
            :key="format.id"
            class="format-card"
            :class="{ selected: selectedFormat === format.id }"
            @click="selectedFormat = format.id"
          >
            <div class="format-icon">{{ format.icon }}</div>
            <div class="format-info">
              <div class="format-name">{{ format.name }}</div>
              <div class="format-desc">{{ format.description }}</div>
            </div>
            <el-icon v-if="selectedFormat === format.id" class="format-check">
              <CircleCheckFilled />
            </el-icon>
          </div>
        </div>
      </div>

      <!-- 导出选项 -->
      <div class="export-section" v-if="selectedFormat">
        <h4>导出选项</h4>
        <div class="options-list">
          <div class="option-item">
            <span>包含目录</span>
            <el-switch v-model="options.includeToc" />
          </div>
          <div class="option-item" v-if="selectedFormat === 'pdf'">
            <span>编译后导出</span>
            <el-switch v-model="options.compileFirst" />
          </div>
          <div class="option-item" v-if="selectedFormat === 'word'">
            <span>保留格式</span>
            <el-switch v-model="options.preserveFormatting" />
          </div>
          <div class="option-item">
            <span>包含批注</span>
            <el-switch v-model="options.includeComments" />
          </div>
          <div class="option-item">
            <span>添加元数据</span>
            <el-switch v-model="options.includeMetadata" />
          </div>
        </div>
      </div>

      <!-- 文件名设置 -->
      <div class="export-section">
        <h4>文件名</h4>
        <el-input
          v-model="filename"
          placeholder="输入文件名"
          :prefix-icon="Document"
        >
          <template #suffix>
            .{{ selectedFormat || 'pdf' }}
          </template>
        </el-input>
      </div>
    </div>

    <template #footer>
      <el-button @click="visible = false">取消</el-button>
      <el-button
        type="primary"
        @click="handleExport"
        :loading="exporting"
        :disabled="!selectedFormat"
      >
        {{ exporting ? '导出中...' : '导出' }}
      </el-button>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { CircleCheckFilled, Document } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface ExportFormat {
  id: string
  name: string
  description: string
  icon: string
  extension: string
}

interface ExportOptions {
  includeToc: boolean
  compileFirst: boolean
  preserveFormatting: boolean
  includeComments: boolean
  includeMetadata: boolean
}

const emit = defineEmits<{
  export: [format: string, options: ExportOptions, filename: string]
}>()

const visible = ref(false)
const exporting = ref(false)
const selectedFormat = ref<string>('pdf')
const filename = ref<string>('document')
const options = ref<ExportOptions>({
  includeToc: true,
  compileFirst: true,
  preserveFormatting: true,
  includeComments: false,
  includeMetadata: true
})

const exportFormats: ExportFormat[] = [
  {
    id: 'pdf',
    name: 'PDF',
    description: '标准 PDF 文档',
    icon: '📄',
    extension: 'pdf'
  },
  {
    id: 'latex',
    name: 'LaTeX',
    description: 'LaTeX 源文件',
    icon: '𝑳',
    extension: 'tex'
  },
  {
    id: 'markdown',
    name: 'Markdown',
    description: 'Markdown 格式',
    icon: '𝕄',
    extension: 'md'
  },
  {
    id: 'word',
    name: 'Word',
    description: 'Microsoft Word',
    icon: '📝',
    extension: 'docx'
  },
  {
    id: 'html',
    name: 'HTML',
    description: '网页格式',
    icon: '🌐',
    extension: 'html'
  },
  {
    id: 'images',
    name: '图片',
    description: '导出为图片',
    icon: '🖼️',
    extension: 'zip'
  }
]

const handleExport = async () => {
  if (!selectedFormat.value) {
    ElMessage.warning('请选择导出格式')
    return
  }

  if (!filename.value.trim()) {
    ElMessage.warning('请输入文件名')
    return
  }

  exporting.value = true

  try {
    // 触发导出事件
    emit('export', selectedFormat.value, options.value, filename.value)

    // 模拟导出过程
    await new Promise(resolve => setTimeout(resolve, 1500))

    ElMessage.success(`文档已导出为 ${selectedFormat.value.toUpperCase()}`)
    visible.value = false
  } catch (error) {
    ElMessage.error('导出失败，请重试')
  } finally {
    exporting.value = false
  }
}

const open = (defaultFilename?: string) => {
  visible.value = true
  if (defaultFilename) {
    filename.value = defaultFilename
  }
  // 重置状态
  selectedFormat.value = 'pdf'
  options.value = {
    includeToc: true,
    compileFirst: true,
    preserveFormatting: true,
    includeComments: false,
    includeMetadata: true
  }
}

defineExpose({ open })
</script>

<style scoped lang="scss">
.export-dialog {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.export-section {
  h4 {
    font-size: 14px;
    font-weight: 600;
    color: var(--el-text-color-primary);
    margin: 0 0 12px 0;
  }
}

.format-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
}

.format-card {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px;
  border: 2px solid var(--el-border-color);
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s;
  position: relative;

  &:hover {
    border-color: var(--el-color-primary);
    background: var(--el-fill-color-light);
  }

  &.selected {
    border-color: var(--el-color-primary);
    background: var(--el-color-primary-light-9);
  }
}

.format-icon {
  font-size: 32px;
  width: 48px;
  height: 48px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--el-fill-color);
  border-radius: 6px;
}

.format-info {
  flex: 1;
}

.format-name {
  font-size: 14px;
  font-weight: 500;
  color: var(--el-text-color-primary);
  margin-bottom: 2px;
}

.format-desc {
  font-size: 12px;
  color: var(--el-text-color-secondary);
}

.format-check {
  position: absolute;
  top: 8px;
  right: 8px;
  font-size: 18px;
  color: var(--el-color-primary);
}

.options-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.option-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px 12px;
  background: var(--el-fill-color-light);
  border-radius: 6px;

  span {
    font-size: 13px;
    color: var(--el-text-color-regular);
  }
}
</style>
