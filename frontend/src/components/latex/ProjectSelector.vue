<template>
  <div class="project-selector">
    <!-- 项目切换按钮 -->
    <el-dropdown trigger="click" @command="handleCommand" @visible-change="handleVisibleChange">
      <el-button :type="isProjectMode ? 'primary' : 'default'" size="small">
        <el-icon><FolderOpened /></el-icon>
        <span class="project-name">{{ currentProjectName }}</span>
        <el-icon class="dropdown-icon"><ArrowDown /></el-icon>
      </el-button>
      <template #dropdown>
        <el-dropdown-menu class="project-dropdown-menu">
          <!-- 当前项目信息 -->
          <div v-if="isProjectMode && currentProject" class="current-project-info">
            <div class="project-header">
              <el-icon><Folder /></el-icon>
              <span class="project-title">{{ currentProject.name }}</span>
            </div>
            <div class="project-meta">
              <span>{{ projectFiles.length }} 个文件</span>
              <span>更新于 {{ formatDate(currentProject.updatedAt) }}</span>
            </div>
          </div>

          <el-dropdown-item divided disabled v-if="isProjectMode">
            <span style="font-weight: 600">项目列表</span>
          </el-dropdown-item>

          <!-- 项目列表 -->
          <div class="projects-list" v-loading="loading">
            <el-dropdown-item
              v-for="project in projectList"
              :key="project.id"
              :command="`switch-${project.id}`"
              :class="{ 'is-active': currentProject?.id === project.id }"
            >
              <div class="project-item">
                <div class="project-item-main">
                  <el-icon><Folder /></el-icon>
                  <span class="project-item-name">{{ project.name }}</span>
                  <el-tag v-if="project.id === currentProject?.id" size="small" type="success">当前</el-tag>
                </div>
                <div class="project-item-meta">
                  <span>{{ project.files?.length || 0 }} 文件</span>
                </div>
              </div>
            </el-dropdown-item>

            <!-- 空状态 -->
            <el-dropdown-item disabled v-if="projectList.length === 0 && !loading">
              <div class="empty-state">
                <el-icon><FolderOpened /></el-icon>
                <span>暂无项目</span>
              </div>
            </el-dropdown-item>
          </div>

          <el-dropdown-item divided disabled>
            <span style="font-weight: 600">项目操作</span>
          </el-dropdown-item>

          <!-- 操作按钮 -->
          <el-dropdown-item command="create">
            <el-icon><FolderAdd /></el-icon>
            <span>新建项目</span>
          </el-dropdown-item>
          <el-dropdown-item command="refresh">
            <el-icon><Refresh /></el-icon>
            <span>刷新列表</span>
          </el-dropdown-item>
          <el-dropdown-item command="toggle-tree" v-if="isProjectMode">
            <el-icon><Menu /></el-icon>
            <span>{{ showTree ? '隐藏' : '显示' }}文件树</span>
          </el-dropdown-item>
          <el-dropdown-item command="exit-project" divided v-if="isProjectMode" class="danger-item">
            <el-icon><Close /></el-icon>
            <span>退出项目模式</span>
          </el-dropdown-item>
        </el-dropdown-menu>
      </template>
    </el-dropdown>

    <!-- 新建项目对话框 -->
    <el-dialog
      v-model="showCreateDialog"
      title="新建 LaTeX 项目"
      width="500px"
      :close-on-click-modal="false"
      :z-index="9999"
      append-to-body
      destroy-on-close
    >
      <el-form :model="newProjectForm" label-width="100px" :rules="formRules" ref="formRef">
        <el-form-item label="项目名称" prop="name">
          <el-input
            v-model="newProjectForm.name"
            placeholder="例如: 我的论文"
            maxlength="50"
            show-word-limit
          />
        </el-form-item>
        <el-form-item label="项目描述" prop="description">
          <el-input
            v-model="newProjectForm.description"
            type="textarea"
            :rows="3"
            placeholder="简要描述项目内容（可选）"
            maxlength="200"
            show-word-limit
          />
        </el-form-item>
        <el-form-item label="主文件名">
          <el-input v-model="newProjectForm.mainFile" disabled>
            <template #append>.tex</template>
          </el-input>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showCreateDialog = false">取消</el-button>
        <el-button type="primary" @click="handleCreateProject" :loading="creating">创建</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { ElMessage, ElMessageBox, type FormInstance, type FormRules } from 'element-plus'
import {
  FolderOpened,
  ArrowDown,
  Folder,
  FolderAdd,
  Refresh,
  Menu,
  Close
} from '@element-plus/icons-vue'
import { useLatexEditorStore } from '@/architecture/stores/latexEditor'

const latexStore = useLatexEditorStore()

// 状态
const loading = ref(false)
const creating = ref(false)
const showCreateDialog = ref(false)
const showTree = ref(false)

// 表单
const formRef = ref<FormInstance>()
const newProjectForm = ref({
  name: '',
  description: '',
  mainFile: 'main'
})

// 表单验证规则
const formRules: FormRules = {
  name: [
    { required: true, message: '请输入项目名称', trigger: 'blur' },
    { min: 2, max: 50, message: '项目名称长度在 2 到 50 个字符', trigger: 'blur' }
  ]
}

// 计算属性
const isProjectMode = computed(() => latexStore.isProjectMode)
const currentProject = computed(() => latexStore.currentProject)
const projectFiles = computed(() => latexStore.projectFiles)
const allProjects = computed(() => latexStore.allProjects)

const currentProjectName = computed(() => {
  if (isProjectMode.value && currentProject.value) {
    return currentProject.value.name
  }
  return '选择项目'
})

const projectList = computed(() => {
  return allProjects.value.slice(0, 10) // 限制显示数量
})

// 方法
function formatDate(timestamp: number): string {
  if (!timestamp) return ''
  const date = new Date(timestamp * 1000)
  const now = new Date()
  const diff = now.getTime() - date.getTime()
  const minutes = Math.floor(diff / 60000)
  const hours = Math.floor(diff / 3600000)
  const days = Math.floor(diff / 86400000)

  if (minutes < 1) return '刚刚'
  if (minutes < 60) return `${minutes} 分钟前`
  if (hours < 24) return `${hours} 小时前`
  if (days < 7) return `${days} 天前`
  return date.toLocaleDateString('zh-CN')
}

async function handleVisibleChange(visible: boolean) {
  if (visible && !loading.value) {
    await loadProjectsList()
  }
}

async function loadProjectsList() {
  try {
    loading.value = true
    await latexStore.loadAllProjects()
  } catch (error: any) {
    ElMessage.error('加载项目列表失败: ' + (error.message || '未知错误'))
  } finally {
    loading.value = false
  }
}

async function handleCommand(command: string) {
  const [action, payload] = command.split('-')

  switch (action) {
    case 'switch':
      await handleSwitchProject(parseInt(payload))
      break
    case 'create':
      showCreateDialog.value = true
      break
    case 'refresh':
      await loadProjectsList()
      ElMessage.success('项目列表已刷新')
      break
    case 'toggle':
      showTree.value = !showTree.value
      // 触发父组件事件
      break
    case 'exit':
      await handleExitProject()
      break
  }
}

async function handleSwitchProject(projectId: number) {
  if (projectId === currentProject.value?.id) {
    ElMessage.info('当前已是该项目')
    return
  }

  try {
    await latexStore.switchProject(projectId)
    ElMessage.success('项目切换成功')
  } catch (error: any) {
    ElMessage.error('项目切换失败: ' + (error.message || '未知错误'))
  }
}

async function handleCreateProject() {
  if (!formRef.value) return

  try {
    await formRef.value.validate()

    creating.value = true
    const projectName = newProjectForm.value.name.trim()
    const description = newProjectForm.value.description.trim()

    if (!projectName) {
      ElMessage.warning('请输入项目名称')
      return
    }

    const project = await latexStore.createNewProject(projectName, description)

    showCreateDialog.value = false
    ElMessage.success(`项目 "${projectName}" 创建成功`)

    // 重置表单
    newProjectForm.value = {
      name: '',
      description: '',
      mainFile: 'main'
    }
    formRef.value.resetFields()
  } catch (error: any) {
    if (error !== false) { // 用户取消不显示错误
      ElMessage.error('创建项目失败: ' + (error.message || '未知错误'))
    }
  } finally {
    creating.value = false
  }
}

async function handleExitProject() {
  try {
    await ElMessageBox.confirm(
      '确定要退出项目模式吗？未保存的更改可能会丢失。',
      '退出项目',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }
    )

    latexStore.setProjectMode(false)
    ElMessage.info('已退出项目模式')
  } catch {
    // 用户取消
  }
}

// 生命周期
onMounted(() => {
  // 预加载项目列表
  loadProjectsList()
})

// 暴露事件
defineEmits<{
  (e: 'toggle-tree', visible: boolean): void
}>()
</script>

<style scoped lang="scss">
.project-selector {
  display: inline-block;
}

.project-name {
  max-width: 150px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  display: inline-block;
  vertical-align: middle;
}

.dropdown-icon {
  margin-left: 4px;
  font-size: 12px;
}

.project-dropdown-menu {
  min-width: 320px;
  max-width: 400px;

  .current-project-info {
    padding: 12px 16px;
    background: var(--el-fill-color-light);
    border-bottom: 1px solid var(--el-border-color-lighter);
    margin: -4px -8px 8px -8px;
  }

  .project-header {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 8px;

    .project-title {
      font-weight: 600;
      font-size: 14px;
    }
  }

  .project-meta {
    display: flex;
    gap: 16px;
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }

  .projects-list {
    max-height: 300px;
    overflow-y: auto;

    &::-webkit-scrollbar {
      width: 6px;
    }

    &::-webkit-scrollbar-thumb {
      background: var(--el-border-color);
      border-radius: 3px;
    }
  }

  .project-item {
    width: 100%;
    padding: 4px 0;

    .project-item-main {
      display: flex;
      align-items: center;
      gap: 8px;
      margin-bottom: 4px;
    }

    .project-item-name {
      flex: 1;
      font-weight: 500;
    }

    .project-item-meta {
      font-size: 12px;
      color: var(--el-text-color-secondary);
      margin-left: 28px;
    }
  }

  .empty-state {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
    padding: 20px;
    color: var(--el-text-color-secondary);

    .el-icon {
      font-size: 32px;
      color: var(--el-border-color);
    }
  }

  .danger-item {
    :deep(.el-dropdown-menu__item) {
      color: var(--el-color-danger);

      &:hover {
        background: var(--el-color-danger-light-9);
        color: var(--el-color-danger);
      }
    }
  }
}

:deep(.el-dropdown-menu__item.is-active) {
  background: var(--el-color-primary-light-9);
  color: var(--el-color-primary);
}
</style>
