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
                <template v-if="editingNode && editingNode.id === data.id">
                  <input
                    ref="editInputRef"
                    v-model="editingValue"
                    class="inline-edit-input"
                    @blur="handleInlineRenameConfirm"
                    @keydown.esc="handleInlineRenameCancel"
                    @keydown.enter="handleInlineRenameConfirm"
                  />
                  <el-tag v-if="data.isMain" size="small" type="success" effect="plain">主文件</el-tag>
                </template>
                <template v-else>
                  {{ data.name }}
                  <el-tag v-if="data.isMain" size="small" type="success" effect="plain">主文件</el-tag>
                </template>
              </span>
            </div>

            <!-- 节点操作 -->
            <span class="node-actions" @click.stop>
              <el-dropdown trigger="click" @command="(cmd) => handleNodeAction(cmd, data)">
                <el-button text size="small" class="more-btn">
                  <el-icon><MoreFilled /></el-icon>
                </el-button>
                <template #dropdown>
                  <el-dropdown-menu>
                    <!-- 文件夹操作 -->
                    <template v-if="data.type === 'folder'">
                      <el-dropdown-item command="createFolder">
                        <el-icon><FolderAdd /></el-icon> 新建子目录
                      </el-dropdown-item>
                      <el-dropdown-item command="createFile">
                        <el-icon><DocumentAdd /></el-icon> 新建文件
                      </el-dropdown-item>
                      <el-dropdown-item command="rename" divided>
                        <el-icon><Edit /></el-icon> 重命名
                      </el-dropdown-item>
                      <el-dropdown-item command="deleteFolder">
                        <el-icon><Delete /></el-icon> 删除目录
                      </el-dropdown-item>
                    </template>
                    <!-- 文件操作 -->
                    <template v-else>
                      <el-dropdown-item command="rename">
                        <el-icon><Edit /></el-icon> 重命名
                      </el-dropdown-item>
                      <el-dropdown-item command="setMain" v-if="data.fileType === 'tex' && !data.isMain">
                        <el-icon><Star /></el-icon> 设为主文件
                      </el-dropdown-item>
                      <el-dropdown-item command="duplicate" divided>
                        <el-icon><DocumentCopy /></el-icon> 复制
                      </el-dropdown-item>
                      <el-dropdown-item command="move">
                        <el-icon><Sort /></el-icon> 移动
                      </el-dropdown-item>
                      <el-dropdown-item command="delete">
                        <el-icon><Delete /></el-icon> 删除
                      </el-dropdown-item>
                    </template>
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
  DocumentAdd,
  FolderAdd,
  Tickets,
  Collection,
  Edit,
  Delete,
  Star,
  MoreFilled,
  Sort
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
  (e: 'file-create', file: { name: string; path: string; type: string; parentPath?: string }): void
  (e: 'file-delete', fileId: number | string): void
  (e: 'file-rename', fileId: number | string, newName: string): void
  (e: 'file-duplicate', fileId: number | string): void
  (e: 'file-move', fileId: number | string, targetPath: string): void
  (e: 'folder-create', folderPath: string): void
  (e: 'folder-delete', folderPath: string): void
  (e: 'folder-rename', oldPath: string, newName: string): void
  (e: 'main-file-change', filePath: string): void
  (e: 'refresh'): void
}>()

// ==========================================
// 状态管理
// ==========================================

const treeRef = ref()
const editInputRef = ref<HTMLInputElement>()
const selectedId = ref<number | string | null>(null)
const showNewFileDialog = ref(false)
const showRenameDialog = ref(false)
const currentFileType = ref<'tex' | 'bib' | 'sty' | 'folder'>('tex')
const currentNode = ref<ProjectFile | null>(null)
const editingNode = ref<TreeNode | null>(null) // 当前正在编辑的节点
const editingValue = ref('') // 编辑中的值

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
          path: isFile ? file.path : nodePath, // 文件夹使用节点路径，文件使用文件路径
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

function handleNodeAction(command: string, data: any) {
  currentNode.value = data

  switch (command) {
    // 文件夹操作
    case 'createFolder':
      handleCreateFolder(data)
      break
    case 'createFile':
      handleCreateFileInFolder(data)
      break
    case 'rename':
      // 原地重命名
      startInlineRename(data)
      break
    case 'deleteFolder':
      handleDeleteFolder(data)
      break
    // 文件操作
    case 'setMain':
      handleSetMainFile(data)
      break
    case 'duplicate':
      handleDuplicateFile(data)
      break
    case 'move':
      handleMoveFile(data)
      break
    case 'delete':
      handleDeleteFile(data)
      break
  }
}

// ==========================================
// 原地重命名功能
// ==========================================

function startInlineRename(node: TreeNode) {
  editingNode.value = node
  editingValue.value = node.name

  nextTick(() => {
    if (editInputRef.value) {
      editInputRef.value.focus()
      editInputRef.value.select()
    }
  })
}

function handleInlineRenameConfirm() {
  if (!editingNode.value || !currentNode.value) return

  const newName = editingValue.value.trim()
  if (!newName) {
    ElMessage.warning('文件名不能为空')
    return
  }

  // 检查是否真的改变了
  if (newName === editingNode.value.name) {
    editingNode.value = null
    editingValue.value = ''
    return
  }

  // 执行重命名
  emit('file-rename', editingNode.value.id, newName)

  editingNode.value = null
  editingValue.value = ''
}

function handleInlineRenameCancel() {
  editingNode.value = null
  editingValue.value = ''
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
// 文件夹操作
// ==========================================

async function handleCreateFolder(parentFolder: any) {
  try {
    const { value } = await ElMessageBox.prompt(
      `请输入新目录名称（将在 "${parentFolder.name}" 下创建）`,
      '新建目录',
      {
        confirmButtonText: '创建',
        cancelButtonText: '取消',
        inputPattern: /^[^/\\:*?"<>|]+$/,
        inputErrorMessage: '目录名称不能包含特殊字符'
      }
    )

    if (value && value.trim()) {
      const folderPath = `${parentFolder.path}/${value.trim()}`
      emit('folder-create', folderPath)
    }
  } catch {
    // 用户取消
  }
}

function handleCreateFileInFolder(folder: any) {
  currentFileType.value = 'tex'
  newFileForm.value = {
    name: '',
    path: folder.path // 设置父目录路径
  }
  showNewFileDialog.value = true
}

async function handleDeleteFolder(folder: any) {
  try {
    await ElMessageBox.confirm(
      `确定要删除目录 "${folder.name}" 及其所有内容吗？此操作不可恢复。`,
      '删除目录',
      {
        confirmButtonText: '删除',
        cancelButtonText: '取消',
        type: 'warning'
      }
    )

    emit('folder-delete', folder.path)
  } catch {
    // 用户取消
  }
}

// ==========================================
// 文件操作
// ==========================================

async function handleDuplicateFile(file: any) {
  try {
    const { value } = await ElMessageBox.prompt(
      `输入复制后的文件名（原文件: ${file.name}）`,
      '复制文件',
      {
        confirmButtonText: '复制',
        cancelButtonText: '取消',
        inputValue: file.name.replace(/(\.[^.]+)$/, '_copy$1'),
        inputPattern: /^[^/\\:*?"<>|]+\.[a-z]+$/i,
        inputErrorMessage: '请输入有效的文件名（包含扩展名）'
      }
    )

    if (value && value.trim()) {
      const newPath = file.path.substring(0, file.path.lastIndexOf('/') + 1) + value.trim()
      emit('file-create', {
        name: value.trim(),
        path: newPath,
        type: 'other',
        parentPath: file.path.substring(0, file.path.lastIndexOf('/'))
      })
    }
  } catch {
    // 用户取消
  }
}

async function handleMoveFile(file: any) {
  ElMessage.info('移动文件功能开发中，请使用删除后重新创建的方式')
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
  padding: 4px 0;

  :deep(.el-tree) {
    background: transparent;

    .el-tree-node {
      position: relative;
      white-space: nowrap;

      .el-tree-node__content {
        width: 100%;
        display: flex;
        align-items: center;
        height: auto;
        min-height: 36px;
        padding: 0;
        margin-bottom: 2px;

        .el-tree-node__expand-icon {
          padding: 6px 4px;
          font-size: 14px;
        }

        .el-tree-node__label {
          flex: 1;
          overflow: hidden;
          text-overflow: ellipsis;
        }
      }

      .el-tree-node__children {
        overflow: visible;
      }
    }
  }
}

.tree-node {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 6px 12px;
  border-radius: 6px;
  transition: all 0.2s;
  min-height: 36px;
  width: 100%;
  box-sizing: border-box;

  &:hover {
    background-color: var(--el-fill-color-light);
  }

  &.is-selected {
    background-color: var(--el-color-primary-light-9);
    border: 1px solid var(--el-color-primary-light-7);
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
  display: flex;
  align-items: center;
  gap: 4px;

  .inline-edit-input {
    flex: 1;
    min-width: 0;
    padding: 2px 8px;
    font-size: 13px;
    font-family: 'Fira Code', 'Consolas', 'Monaco', 'Courier New', monospace;
    border: 1px solid var(--el-color-primary);
    border-radius: 4px;
    outline: none;
    background: var(--el-bg-color);
    color: var(--el-text-color-primary);

    &:focus {
      border-color: var(--el-color-primary);
      box-shadow: 0 0 0 2px var(--el-color-primary-light-7);
    }
  }
}

.node-actions {
  flex-shrink: 0;
  margin-left: auto;
  padding-right: 4px;

  .more-btn {
    padding: 2px;
    min-height: auto;
    width: 24px;
    height: 24px;
    border-radius: 4px;
    background: transparent;
    border: none;
    transition: all 0.2s;
    opacity: 0.4;

    .el-icon {
      font-size: 14px;
      color: var(--el-text-color-secondary);
      transform: rotate(90deg);
    }

    &:hover {
      background: var(--el-fill-color-light);
      opacity: 1;

      .el-icon {
        color: var(--el-text-color-regular);
      }
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

// Dropdown菜单项样式
:deep(.el-dropdown-menu__item) {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;

  .el-icon {
    font-size: 16px;
    flex-shrink: 0;
  }
}
</style>
