<template>
  <aside class="left-panel" role="complementary" :aria-label="panelTitle">
    <div class="panel-header">
      <h3>{{ panelTitle }}</h3>
      <el-button size="small" @click="$emit('close')" :aria-label="`关闭${panelTitle}`">
        <el-icon><Close /></el-icon>
      </el-button>
    </div>
    <div class="panel-content">
      <!-- 文档大纲 -->
      <DocumentOutline
        v-if="mode === 'outline'"
        :content="content || ''"
        :is-project-mode="isProjectMode"
        :project-files="projectFiles"
        :current-file-id="currentFileId"
        :main-file-path="mainFilePath"
        @navigate="$emit('navigate', $event)"
        @file-select="$emit('file-select', $event)"
      />
      <!-- 项目文件树 -->
      <ProjectFileTree
        v-else-if="mode === 'project-tree' && projectId"
        :project-id="typeof projectId === 'number' ? projectId : Number(projectId)"
        :project-name="projectName || ''"
        :files="projectFiles"
        :main-file-path="mainFilePath"
        @file-select="$emit('file-select', $event)"
        @file-create="$emit('file-create', $event)"
        @file-delete="handleFileDelete"
        @file-rename="$emit('file-rename', $event)"
        @file-duplicate="handleFileDuplicate"
        @file-move="$emit('file-move', $event)"
        @folder-create="$emit('folder-create', $event)"
        @folder-delete="$emit('folder-delete', $event)"
        @folder-rename="$emit('folder-rename', $event)"
        @main-file-change="$emit('main-file-change', $event)"
        @refresh="handleRefresh"
      />
    </div>
  </aside>
</template>

<script setup lang="ts">
import { Close } from '@element-plus/icons-vue'
import DocumentOutline from '@/components/latex/DocumentOutline.vue'
import ProjectFileTree from '@/components/latex/ProjectFileTree.vue'

type PanelMode = 'outline' | 'project-tree'

interface Props {
  mode: PanelMode
  panelTitle: string
  content?: string
  isProjectMode?: boolean
  projectFiles?: any[]
  currentFileId?: string
  mainFilePath?: string
  projectId?: number | string  // ProjectFileTree可能需要number
  projectName?: string
}

withDefaults(defineProps<Props>(), {
  isProjectMode: false,
  projectFiles: () => []
})

const emit = defineEmits<{
  'close': []
  'navigate': [position: { line: number; column?: number }]
  'file-select': [file: any]
  'file-create': [data: any]
  'file-delete': [fileId: string]
  'file-rename': [data: any]
  'file-duplicate': [fileId: string]
  'file-move': [data: any]
  'folder-create': [data: any]
  'folder-delete': [folderId: string]
  'folder-rename': [data: any]
  'main-file-change': [filePath: string]
  'refresh': []
}>()

// Type-safe event handlers for template usage
function handleFileDelete(event: any) {
  emit('file-delete', event)
}

function handleFileDuplicate(event: any) {
  emit('file-duplicate', event)
}

function handleRefresh() {
  emit('refresh')
}

// emit is defined but not used directly - events are emitted via handlers in template
void emit
</script>

<style scoped lang="scss">
.left-panel {
  display: flex;
  flex-direction: column;
  width: 280px;
  min-width: 280px;
  background: var(--el-bg-color);
  border-right: 1px solid var(--el-border-color);
  overflow: hidden;
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color);
  background: var(--el-fill-color-blank);

  h3 {
    margin: 0;
    font-size: 14px;
    font-weight: 500;
  }
}

.panel-content {
  flex: 1;
  overflow-y: auto;
  padding: 8px 0;
}
</style>
