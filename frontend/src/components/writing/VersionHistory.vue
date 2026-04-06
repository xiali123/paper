<template>
  <div class="version-history-panel">
    <div class="panel-header">
      <h4 class="panel-title">版本历史</h4>
      <el-button
        type="primary"
        size="small"
        :icon="FolderAdd"
        @click="handleCreateVersion"
      >
        保存版本
      </el-button>
    </div>

    <div v-if="loading && versions.length === 0" class="panel-loading">
      <el-skeleton :rows="3" animated />
    </div>

    <div v-else-if="versions.length === 0" class="panel-empty">
      <el-empty description="暂无版本记录" :image-size="60" />
    </div>

    <div v-else class="version-list">
      <el-timeline>
        <el-timeline-item
          v-for="version in versions"
          :key="version.id"
          :timestamp="formatTime(version.created_at)"
          placement="top"
        >
          <div class="version-item">
            <div class="version-header">
              <span class="version-number">版本 {{ version.version_number }}</span>
              <el-tag v-if="version.is_auto_save" size="small" effect="plain">自动保存</el-tag>
            </div>

            <div v-if="version.change_summary" class="version-summary">
              {{ version.change_summary }}
            </div>

            <div class="version-meta">
              <span>{{ version.word_count }} 字</span>
              <span>by {{ version.created_by }}</span>
            </div>
          </div>
        </el-timeline-item>
      </el-timeline>
    </div>

    <!-- 创建版本弹窗 -->
    <el-dialog v-model="dialogVisible" title="保存新版本" width="400px">
      <el-form label-position="top">
        <el-form-item label="变更说明（可选）">
          <el-input
            v-model="summary"
            type="textarea"
            :rows="3"
            placeholder="描述本次版本的变更内容..."
            maxlength="200"
            show-word-limit
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" @click="confirmCreate">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { FolderAdd } from '@element-plus/icons-vue'
import type { DocumentVersion } from '@/types/collaborative'

defineProps<{
  versions: DocumentVersion[]
  loading?: boolean
}>()

const emit = defineEmits<{
  create: [summary?: string]
}>()

const dialogVisible = ref(false)
const summary = ref('')

function handleCreateVersion() {
  summary.value = ''
  dialogVisible.value = true
}

function confirmCreate() {
  emit('create', summary.value || undefined)
  dialogVisible.value = false
}

function formatTime(time: string): string {
  const date = new Date(time)
  return date.toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit'
  })
}
</script>

<style scoped lang="scss">
.version-history-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);
}

.panel-title {
  margin: 0;
  font-size: 14px;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.panel-loading,
.panel-empty {
  padding: 20px;
}

.version-list {
  flex: 1;
  overflow-y: auto;
  padding: 12px;
}

.version-item {
  .version-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 4px;
  }

  .version-number {
    font-weight: 600;
    color: var(--el-text-color-primary);
  }

  .version-summary {
    font-size: 13px;
    color: var(--el-text-color-regular);
    margin-bottom: 4px;
  }

  .version-meta {
    display: flex;
    gap: 12px;
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }
}
</style>
