<template>
  <div class="version-history">
    <div class="version-header">
      <div class="version-title">
        <el-icon><Clock /></el-icon>
        <span>版本历史</span>
      </div>
      <div class="version-actions">
        <el-button :icon="Refresh" @click="refreshVersions" :loading="loading" size="small">
          刷新
        </el-button>
        <el-button :icon="DocumentAdd" @click="showSaveDialog = true" type="primary" size="small">
          保存版本
        </el-button>
      </div>
    </div>

    <div class="version-list">
      <el-table :data="versionHistory" stripe size="small" @row-click="selectVersion" highlight-current-row>
        <el-table-column prop="summary" label="摘要" min-width="150" />
        <el-table-column prop="timestamp" label="时间" width="160">
          <template #default="{ row }">
            {{ formatTime(row.timestamp) }}
          </template>
        </el-table-column>
        <el-table-column prop="author" label="作者" width="100" />
        <el-table-column label="操作" width="150" fixed="right">
          <template #default="{ row }">
            <el-button size="small" @click.stop="previewVersion(row)" link>
              预览
            </el-button>
            <el-button size="small" @click.stop="restoreToVersion(row)" link type="warning">
              恢复
            </el-button>
          </template>
        </el-table-column>
      </el-table>
      <el-empty v-if="versionHistory.length === 0" description="暂无版本历史" />
    </div>

    <el-dialog v-model="showSaveDialog" title="保存版本" width="400px">
      <el-form :model="saveForm" label-width="80px">
        <el-form-item label="摘要">
          <el-input v-model="saveForm.summary" placeholder="请输入版本摘要（可选）" maxlength="100" show-word-limit />
        </el-form-item>
        <el-form-item label="自动保存">
          <el-switch v-model="saveForm.isAutoSave" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showSaveDialog = false">取消</el-button>
        <el-button type="primary" @click="handleSaveVersion" :loading="saving">
          保存
        </el-button>
      </template>
    </el-dialog>

    <el-dialog v-model="showPreviewDialog" title="版本预览" width="800px">
      <div class="version-preview" v-if="previewVersion">
        <el-descriptions :column="2" border size="small">
          <el-descriptions-item label="版本ID">{{ previewVersion.id.slice(-8) }}</el-descriptions-item>
          <el-descriptions-item label="时间">{{ formatTime(previewVersion.timestamp) }}</el-descriptions-item>
          <el-descriptions-item label="作者">{{ previewVersion.author }}</el-descriptions-item>
          <el-descriptions-item label="行数">{{ previewVersion.totalLines }}</el-descriptions-item>
        </el-descriptions>
        <el-input :model-value="previewVersion.content" type="textarea" :rows="15" readonly />
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, watch } from 'vue'
import { Clock, Refresh, DocumentAdd } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import {
  getLatexVersionHistory,
  saveLatexVersion,
  restoreLatexVersion,
  type FrontendLatexVersionNode
} from '@/api/adapters/latexAdapter'

const props = defineProps<{
  fileId: number
  projectId: number
  userId: string
  currentContent?: string
}>()

const emit = defineEmits<{
  (e: 'restore', content: string): void
}>()

const loading = ref(false)
const saving = ref(false)
const versionHistory = ref<FrontendLatexVersionNode[]>([])
const showSaveDialog = ref(false)
const showPreviewDialog = ref(false)
const previewVersion = ref<FrontendLatexVersionNode | null>(null)

const saveForm = ref({
  summary: '',
  isAutoSave: false
})

const loadVersions = async () => {
  loading.value = true
  try {
    const history = await getLatexVersionHistory(props.fileId, props.projectId, props.userId)
    versionHistory.value = history
  } catch (error) {
    console.error('Failed to load versions:', error)
    ElMessage.error('加载版本历史失败')
  } finally {
    loading.value = false
  }
}

const refreshVersions = () => {
  loadVersions()
}

const selectVersion = (version: FrontendLatexVersionNode) => {
  console.log('Selected version:', version)
}

const formatTime = (timestamp: Date) => {
  const date = new Date(timestamp)
  return date.toLocaleString('zh-CN')
}

const handleSaveVersion = async () => {
  if (!props.currentContent) {
    ElMessage.warning('没有内容可保存')
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

    ElMessage.success('版本保存成功')
    await loadVersions()
  } catch (error) {
    console.error('Failed to save version:', error)
    ElMessage.error('保存版本失败')
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
    ElMessage.success('版本已恢复，请记得保存更改')
    await loadVersions()
  } catch (error) {
    console.error('Failed to restore version:', error)
    ElMessage.error('恢复版本失败')
  }
}

onMounted(() => {
  loadVersions()
})

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
    }

    .version-actions {
      display: flex;
      gap: 8px;
    }
  }

  .version-list {
    flex: 1;
    overflow: auto;
    padding: 16px;
  }

  .version-preview {
    .el-descriptions {
      margin-bottom: 16px;
    }
  }
}
</style>
