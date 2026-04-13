<template>
  <div class="project-file-tree" role="tree" aria-label="项目文件树">
    <!-- 项目头部 -->
    <div class="tree-header" role="presentation">
      <div class="project-info">
        <el-icon class="project-icon"><Folder /></el-icon>
        <span class="project-name">{{ projectName }}</span>
      </div>
      <div class="tree-actions" role="toolbar" aria-label="文件操作">
        <el-dropdown trigger="click" @command="handleAddFile">
          <el-button size="small" text aria-label="添加文件">
            <el-icon><Plus /></el-icon>
          </el-button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item command="tex">LaTeX 文件 (.tex)</el-dropdown-item>
              <el-dropdown-item command="bib">参考文献 (.bib)</el-dropdown-item>
              <el-dropdown-item command="sty">样式文件 (.sty)</el-dropdown-item>
              <el-dropdown-item command="folder">新建文件夹</el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
        <el-button size="small" text @click="handleRefresh" aria-label="刷新">
          <el-icon><Refresh /></el-icon>
        </el-button>
      </div>
    </div>

    <!-- 文件树 -->
    <div class="tree-content">
      <el-tree
        ref="treeRef"
        :data="treeData"
        :props="treeProps"
        :highlight-current="true"
        :expand-on-click-node="false"
        :default-expand-all="true"
        node-key="id"
        @node-click="handleNodeClick"
        @node-contextmenu="handleContextMenu"
      >
        <template #default="{ node, data }">
          <div class="tree-node" :class="{ 'is-main': data.isMain, 'is-selected': selectedId === data.id }">
            <div class="node-content">
              <!-- 文件图标 -->
              <span class="node-icon">
                <el-icon v-if="data.type === 'folder'"><Folder /></el-icon>
                <el-icon v-else-if="data.fileType === 'tex'"><Document /></el-icon>
                <el-icon v-else-if="data.fileType === 'bib'"><Tickets /></el-icon>
                <el-icon v-else-if="data.fileType === 'sty'"><Collection /></el-icon>
                <el-icon v-else><DocumentCopy /></el-icon>
              </span>

              <!-- 文件名 -->
              <span class="node-label" :title="data.name">
                {{ data.name }}
                <el-tag v-if="data.isMain" size="small" type="success" effect="plain">主文件</el-tag>
              </span>
            </div>

            <!-- 节点操作 -->
            <span class="node-actions" @click.stop>
              <el-dropdown trigger="click" @command="(cmd) => handleNodeAction(cmd, data)">
                <el-icon class="more-icon"><MoreFilled /></el-icon>
                <template #dropdown>
                  <el-dropdown-menu>
                    <el-dropdown-item command="rename" v-if="data.type !== 'folder'">
                      <el-icon><Edit /></el-icon> 重命名
                    </el-dropdown-item>
                    <el-dropdown-item command="setMain" v-if="data.fileType === 'tex' && !data.isMain">
                      <el-icon><Star /></el-icon> 设为主文件
                    </el-dropdown-item>
                    <el-dropdown-item command="delete" divided>
                      <el-icon><Delete /></el-icon> 删除
                    </el-dropdown-item>
                  </el-dropdown-menu>
                </template>
              </el-dropdown>
            </span>
          </div>
        </template>
      </el-tree>
    </div>

    <!-- 新建文件对话框 -->
    <el-dialog
      v-model="showNewFileDialog"
      :title="dialogTitle"
      width="500px"
      @close="handleDialogClose"
    >
      <el-form :model="newFileForm" label-width="100px">
        <el-form-item label="文件名">
          <el-input
            v-model="newFileForm.name"
            :placeholder="filePlaceholder"
            @keyup.enter="handleConfirmNewFile"
          >
            <template #append>.{{ fileExtension }}</template>
          </el-input>
        </el-form-item>
        <el-form-item label="保存路径" v-if="showPathInput">
          <el-input
            v-model="newFileForm.path"
            placeholder="如: chapters/ (留空为根目录)"
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showNewFileDialog = false">取消</el-button>
        <el-button type="primary" @click="handleConfirmNewFile" :disabled="!newFileForm.name">
          创建
        </el-button>
      </template>
    </el-dialog>

    <!-- 重命名对话框 -->
    <el-dialog
      v-model="showRenameDialog"
      title="重命名文件"
      width="400px"
    >
      <el-form :model="renameForm" label-width="80px">
        <el-form-item label="新名称">
          <el-input
            v-model="renameForm.name"
            placeholder="输入新文件名"
            @keyup.enter="handleConfirmRename"
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showRenameDialog = false">取消</el-button>
        <el-button type="primary" @click="handleConfirmRename">确定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import {
  Folder,
  Plus,
  Refresh,
  Document,
  DocumentCopy,
  Tickets,
  Collection,
  Edit,
  Delete,
  Star,
  MoreFilled
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox, ElNotification } from 'element-plus'

// ==========================================
// 类型定义
// ==========================================

interface ProjectFile {
  id: number | string
  projectId: number
  name: string
  path: string
  content?: string
  type: 'main' | 'included' | 'bibliography' | 'image' | 'other'
  fileType?: 'tex' | 'bib' | 'sty' | 'pdf' | 'image' | 'folder'
  isMain?: boolean
  createdAt: number
  updatedAt: number
  children?: ProjectFile[]
}

interface TreeNode {
  id: number | string
  label: string
  name: string
  path: string
  type: 'file' | 'folder'
  fileType?: string
  isMain?: boolean
  children?: TreeNode[]
}

// ==========================================
// Props & Emits
// ==========================================

interface Props {
  projectId: number
  projectName: string
  files: ProjectFile[]
  mainFilePath?: string
}

const props = withDefaults(defineProps<Props>(), {
  mainFilePath: 'main.tex'
})

// 定义 emits - Vue SFC 编译器限制，使用简单类型
const emit = defineEmits<{
  (e: 'file-select', file: ProjectFile): void
  (e: 'file-create', file: { name: string; path: string; type: string }): void
  (e: 'file-delete', fileId: number | string): void
  (e: 'file-rename', fileId: number | string, newName: string): void
  (e: 'main-file-change', filePath: string): void
  (e: 'refresh'): void
}>()

// ==========================================
// 状态管理
// ==========================================

const treeRef = ref()
const selectedId = ref<number | string | null>(null)
const showNewFileDialog = ref(false)
const showRenameDialog = ref(false)
const currentFileType = ref<'tex' | 'bib' | 'sty' | 'folder'>('tex')
const currentNode = ref<ProjectFile | null>(null)

const newFileForm = ref({
  name: '',
  path: ''
})

const renameForm = ref({
  name: ''
})

// 树形数据
const treeData = computed<TreeNode[]>(() => {
  const map = new Map<string, TreeNode>()
  const roots: TreeNode[] = []

  // 首先创建所有节点
  for (const file of props.files) {
    const pathParts = file.path.split('/').filter(p => p)
    let currentPath = ''

    // 构建路径树
    for (let i = 0; i < pathParts.length; i++) {
      const part = pathParts[i]
      const isLast = i === pathParts.length - 1
      const nodePath = currentPath ? `${currentPath}/${part}` : part

      let node = map.get(nodePath)

      if (!node) {
        const isFile = isLast && part.includes('.')
        const fileType = isFile ? getFileType(part) : 'folder'

        node = {
          id: isFile ? file.id : nodePath,
          label: part,
          name: part,
          path: file.path,
          type: isFile ? 'file' : 'folder',
          fileType: isFile ? fileType : undefined,
          isMain: isFile && file.path === props.mainFilePath,
          children: []
        }

        map.set(nodePath, node)

        // 找到父节点
        if (currentPath) {
          const parent = map.get(currentPath)
          if (parent && parent.children) {
            parent.children.push(node)
          }
        } else {
          roots.push(node)
        }
      }

      currentPath = nodePath
    }
  }

  return roots
})

const treeProps = {
  children: 'children',
  label: 'name'
}

// 对话框相关
const dialogTitle = computed(() => {
  const titles = {
    tex: '新建 LaTeX 文件',
    bib: '新建参考文献文件',
    sty: '新建样式文件',
    folder: '新建文件夹'
  }
  return titles[currentFileType.value]
})

const filePlaceholder = computed(() => {
  const placeholders = {
    tex: '如: chapter1',
    bib: '如: references',
    sty: '如: mystyle',
    folder: '如: chapters'
  }
  return placeholders[currentFileType.value]
})

const fileExtension = computed(() => {
  const extensions = {
    tex: 'tex',
    bib: 'bib',
    sty: 'sty',
    folder: ''
  }
  return extensions[currentFileType.value]
})

const showPathInput = computed(() => {
  return currentFileType.value !== 'folder'
})

// ==========================================
// 辅助函数
// ==========================================

function getFileType(filename: string): string {
  const ext = filename.split('.').pop()?.toLowerCase()
  const typeMap: Record<string, string> = {
    tex: 'tex',
    bib: 'bib',
    sty: 'sty',
    pdf: 'pdf',
    png: 'image',
    jpg: 'image',
    jpeg: 'image',
    gif: 'image'
  }
  return typeMap[ext || ''] || 'other'
}

// ==========================================
// 事件处理
// ==========================================

function handleNodeClick(data: TreeNode) {
  if (data.type === 'folder') return

  selectedId.value = data.id

  // 找到对应的文件
  const file = props.files.find(f => f.id === data.id || f.path === data.path)
  if (file) {
    emit('file-select', file)
  }
}

function handleAddFile(command: string) {
  currentFileType.value = command as any
  newFileForm.value = { name: '', path: '' }
  showNewFileDialog.value = true
}

function handleConfirmNewFile() {
  const name = newFileForm.value.name.trim()
  if (!name) {
    ElMessage.warning('请输入文件名')
    return
  }

  const path = newFileForm.value.path.trim()
  const fullPath = path ? `${path}/${name}.${fileExtension.value}` : `${name}.${fileExtension.value}`

  emit('file-create', {
    name: name + (fileExtension.value ? `.${fileExtension.value}` : ''),
    path: fullPath,
    type: getTypeCommand(currentFileType.value)
  })

  showNewFileDialog.value = false
  ElMessage.success('文件创建请求已发送')
}

function getTypeCommand(type: string): string {
  const typeMap: Record<string, string> = {
    tex: 'other',
    bib: 'bibliography',
    sty: 'other',
    folder: 'other'
  }
  return typeMap[type] || 'other'
}

function handleDialogClose() {
  newFileForm.value = { name: '', path: '' }
}

function handleRefresh() {
  emit('refresh')
}

function handleContextMenu(event: MouseEvent, data: any) {
  event.preventDefault()
  // 可以在这里添加右键菜单逻辑
}

function handleNodeAction(command: string, data: any) {
  currentNode.value = data

  switch (command) {
    case 'rename':
      renameForm.value.name = data.name
      showRenameDialog.value = true
      break
    case 'setMain':
      handleSetMainFile(data)
      break
    case 'delete':
      handleDeleteFile(data)
      break
  }
}

function handleConfirmRename() {
  if (!currentNode.value) return

  const newName = renameForm.value.name.trim()
  if (!newName) {
    ElMessage.warning('请输入新名称')
    return
  }

  emit('file-rename', currentNode.value!.id, newName)
  showRenameDialog.value = false
}

async function handleSetMainFile(data: any) {
  try {
    await ElMessageBox.confirm(
      `确定要将 "${data.name}" 设为主文件吗？`,
      '设置主文件',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }
    )

    emit('main-file-change', data.path)
    ElMessage.success('主文件设置成功')
  } catch {
    // 用户取消
  }
}

async function handleDeleteFile(data: any) {
  try {
    await ElMessageBox.confirm(
      `确定要删除 "${data.name}" 吗？此操作不可恢复。`,
      '删除文件',
      {
        confirmButtonText: '删除',
        cancelButtonText: '取消',
        type: 'warning'
      }
    )

    emit('file-delete', data.id)
  } catch {
    // 用户取消
  }
}

// ==========================================
// 暴露方法
// ==========================================

defineExpose({
  setSelectedId: (id: number | string) => {
    selectedId.value = id
  },
  expandPath: (path: string) => {
    // 实现展开指定路径的逻辑
  }
})
</script>

<style scoped lang="scss">
.project-file-tree {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color-page);
  border-right: 1px solid var(--el-border-color-light);
}

.tree-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color-light);
  background: var(--el-fill-color-light);
}

.project-info {
  display: flex;
  align-items: center;
  gap: 8px;
}

.project-icon {
  font-size: 20px;
  color: var(--el-color-primary);
}

.project-name {
  font-weight: 500;
  font-size: 14px;
}

.tree-actions {
  display: flex;
  gap: 4px;
}

.tree-content {
  flex: 1;
  overflow-y: auto;
  padding: 8px 0;
}

.tree-node {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 4px 8px;
  border-radius: 4px;
  transition: background-color 0.2s;

  &:hover {
    background-color: var(--el-fill-color-light);

    .node-actions {
      opacity: 1;
    }
  }

  &.is-selected {
    background-color: var(--el-color-primary-light-9);
  }

  &.is-main {
    .node-label {
      font-weight: 500;
    }
  }
}

.node-content {
  display: flex;
  align-items: center;
  gap: 8px;
  flex: 1;
  min-width: 0;
}

.node-icon {
  flex-shrink: 0;
  font-size: 16px;
  color: var(--el-text-color-secondary);
}

.node-label {
  flex: 1;
  font-size: 13px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.node-actions {
  opacity: 0;
  transition: opacity 0.2s;

  .more-icon {
    font-size: 14px;
    color: var(--el-text-color-secondary);
    cursor: pointer;

    &:hover {
      color: var(--el-color-primary);
    }
  }
}

// 滚动条样式
.tree-content {
  &::-webkit-scrollbar {
    width: 6px;
  }

  &::-webkit-scrollbar-thumb {
    background-color: var(--el-border-color-darker);
    border-radius: 3px;

    &:hover {
      background-color: var(--el-border-color);
    }
  }
}
</style>
