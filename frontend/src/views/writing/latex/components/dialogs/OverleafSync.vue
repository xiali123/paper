<template>
  <el-drawer
    :model-value="show"
    @update:model-value="$emit('update:show', $event)"
    title="Overleaf 项目同步"
    direction="rtl"
    size="500px"
  >
    <div class="overleaf-sync">
      <!-- 连接配置 -->
      <div class="config-section">
        <h4>连接配置</h4>
        <el-form :model="overleafConfig" label-width="100px" size="small">
          <el-form-item label="API URL">
            <el-input
              v-model="overleafConfig.apiUrl"
              placeholder="https://api.overleaf.com"
            />
          </el-form-item>
          <el-form-item label="API Token">
            <el-input
              v-model="overleafConfig.apiToken"
              type="password"
              show-password
              placeholder="输入Overleaf API Token"
            >
              <template #append>
                <el-button @click="openOverleafDocs" :icon="Link">
                  <el-icon><DocumentCopy /></el-icon>
                获取
                </el-button>
              </template>
            </el-input>
            <div class="form-tip">
              在 Overleaf Account Settings → API Tokens 中生成
            </div>
          </el-form-item>
          <el-form-item>
            <el-button type="primary" @click="testConnection" :loading="testing">
              <el-icon><Connection /></el-icon>
              测试连接
            </el-button>
          </el-form-item>
        </el-form>
      </div>

      <!-- 项目选择 -->
      <div v-if="isConnected" class="project-section">
        <h4>Overleaf 项目</h4>
        <div class="project-list">
          <div
            v-for="project in overleafProjects"
            :key="project.id"
            class="project-item"
          >
            <div class="project-info">
              <h5>{{ project.name }}</h5>
              <p class="project-meta">
                <span>{{ project.lastUpdated }}</span>
                <span>• {{ project.owner }}</span>
              </p>
            </div>
            <div class="project-actions">
              <el-dropdown trigger="click" @command="(cmd) => handleProjectAction(cmd, project)">
                <el-button text>
                  <el-icon><MoreFilled /></el-icon>
                </el-button>
                <template #dropdown>
                  <el-dropdown-menu>
                    <el-dropdown-item command="sync">同步到本地</el-dropdown-item>
                    <el-dropdown-item command="syncFrom">从本地同步</el-dropdown-item>
                    <el-dropdown-item command="autoSync" divided>
                      <el-checkbox v-model="project.autoSync">
                        自动同步
                      </el-checkbox>
                    </el-dropdown-item>
                  </el-dropdown-menu>
                </template>
              </el-dropdown>
            </div>
          </div>
        </div>

        <el-empty
          v-if="overleafProjects.length === 0"
          description="暂无项目"
        >
          <el-button @click="fetchOverleafProjects">
            <el-icon><Refresh /></el-icon>
            刷新项目列表
          </el-button>
        </el-empty>
      </div>

      <!-- 同步状态 -->
      <div v-if="syncStatus" class="sync-status">
        <h4>同步状态</h4>
        <div class="status-item">
          <el-icon :class="getStatusIcon(syncStatus.status)">
            <Loading v-if="syncStatus.status === 'syncing'" />
            <CircleCheck v-else-if="syncStatus.status === 'success'" />
            <CircleClose v-else />
          </el-icon>
          <div class="status-content">
            <div class="status-title">{{ syncStatus.title }}</div>
            <div class="status-message">{{ syncStatus.message }}</div>
            <el-progress
              v-if="syncStatus.status === 'syncing'"
              :percentage="syncStatus.progress"
              :status="syncStatus.status === 'syncing' ? undefined : 'success'"
            />
          </div>
        </div>
      </div>

      <!-- 同步历史 -->
      <div v-if="syncHistory.length > 0" class="history-section">
        <div class="section-header">
          <h4>同步历史</h4>
          <el-button text size="small" @click="clearHistory">
            <el-icon><Delete /></el-icon>
            清除
          </el-button>
        </div>
        <el-timeline>
          <el-timeline-item
            v-for="item in syncHistory"
            :key="item.id"
            :timestamp="item.timestamp"
            placement="top"
          >
            <el-tag :type="getSyncTypeColor(item.type)" size="small">
              {{ getSyncTypeLabel(item.type) }}
            </el-tag>
            <p>{{ item.message }}</p>
            <div v-if="item.details" class="history-details">
              <code>{{ item.details }}</code>
            </div>
          </el-timeline-item>
        </el-timeline>
      </div>
    </div>
  </el-drawer>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import {
  Connection, Link, DocumentCopy, Refresh, MoreFilled, Loading,
  CircleCheck, CircleClose, Delete
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface OverleafProject {
  id: string
  name: string
  owner: string
  lastUpdated: string
  autoSync?: boolean
}

interface OverleafConfig {
  apiUrl: string
  apiToken: string
}

interface Props {
  show: boolean
  projectId?: string
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'update:show': [value: boolean]
  'sync': [direction: 'toLocal' | 'fromLocal', projectId: string]
  'auto-sync-change': [projectId: string, enabled: boolean]
}>()

// 状态
const testing = ref(false)
const isConnected = ref(false)
const overleafConfig = ref<OverleafConfig>({
  apiUrl: 'https://api.overleaf.com',
  apiToken: ''
})
const overleafProjects = ref<OverleafProject[]>([])
const syncStatus = ref<{
  status: 'idle' | 'syncing' | 'success' | 'error'
  title: string
  message: string
  progress?: number
}>({ status: 'idle', title: '', message: '' })

const syncHistory = ref<Array<{
  id: string
  type: 'upload' | 'download' | 'auto'
  timestamp: number
  message: string
  details?: string
}>>([])

// 方法
function openOverleafDocs() {
  window.open('https://www.overleaf.com/user/settings/api_tokens', '_blank')
}

async function testConnection() {
  if (!overleafConfig.value.apiToken) {
    ElMessage.warning('请输入API Token')
    return
  }

  testing.value = true
  try {
    // 模拟API调用
    await new Promise(resolve => setTimeout(resolve, 1500))
    isConnected.value = true
    ElMessage.success('连接成功')
    fetchOverleafProjects()
  } catch (error) {
    isConnected.value = false
    ElMessage.error('连接失败：' + (error as Error).message)
  } finally {
    testing.value = false
  }
}

async function fetchOverleafProjects() {
  try {
    // 模拟获取项目列表
    await new Promise(resolve => setTimeout(resolve, 1000))

    overleafProjects.value = [
      {
        id: 'proj1',
        name: 'Research Paper 2024',
        owner: 'currentUser',
        lastUpdated: '2小时前',
        autoSync: false
      },
      {
        id: 'proj2',
        name: 'Thesis Document',
        owner: 'currentUser',
        lastUpdated: '1天前',
        autoSync: true
      }
    ]
  } catch (error) {
    ElMessage.error('获取项目失败')
  }
}

async function handleProjectAction(command: string, project: OverleafProject) {
  switch (command) {
    case 'sync':
      await syncToLocal(project)
      break
    case 'syncFrom':
      await syncFromLocal(project)
      break
    case 'autoSync':
      toggleAutoSync(project)
      break
  }
}

async function syncToLocal(project: OverleafProject) {
  syncStatus.value = {
    status: 'syncing',
    title: '正在同步...',
    message: `从 "${project.name}" 同步到本地`,
    progress: 0
  }

  try {
    // 模拟同步过程
    for (let i = 0; i <= 100; i += 10) {
      syncStatus.value.progress = i
      await new Promise(resolve => setTimeout(resolve, 200))
    }

    syncStatus.value = {
      status: 'success',
      title: '同步完成',
      message: `成功从 "${project.name}" 同步 ${Math.floor(Math.random() * 20)} 个更改`
    }

    addSyncHistory({
      type: 'download',
      message: `从 "${project.name}" 同步到本地`,
      details: `${Math.floor(Math.random() * 20)} 个文件已更新`
    })

    ElMessage.success('同步完成')
  } catch (error) {
    syncStatus.value = {
      status: 'error',
      title: '同步失败',
      message: (error as Error).message
    }
  }
}

async function syncFromLocal(project: OverleafProject) {
  syncStatus.value = {
    status: 'syncing',
    title: '正在上传...',
    message: `从本地同步到 "${project.name}"`,
    progress: 0
  }

  try {
    // 模拟上传过程
    for (let i = 0; i <= 100; i += 20) {
      syncStatus.value.progress = i
      await new Promise(resolve => setTimeout(resolve, 300))
    }

    syncStatus.value = {
      status: 'success',
      title: '上传完成',
      message: `成功将 ${Math.floor(Math.random() * 10)} 个文件上传到 Overleaf`
    }

    addSyncHistory({
      type: 'upload',
      message: `从本地同步到 "${project.name}"`,
      details: `${Math.floor(Math.random() * 10)} 个文件已上传`
    })

    ElMessage.success('上传完成')
  } catch (error) {
    syncStatus.value = {
      status: 'error',
      title: '上传失败',
      message: (error as Error).message
    }
  }
}

function toggleAutoSync(project: OverleafProject) {
  project.autoSync = !project.autoSync
  emit('auto-sync-change', project.id, project.autoSync)

  addSyncHistory({
    type: 'auto',
    message: `"${project.name}" 自动同步已${project.autoSync ? '启用' : '禁'}`
  })

  ElMessage.success(`自动同步已${project.autoSync ? '启用' : '禁用'}`)
}

function addSyncHistory(item: Omit<typeof syncHistory.value[number], 'id'>) {
  syncHistory.value.unshift({
    id: Date.now().toString(),
    timestamp: Date.now(),
    ...item
  })

  // 限制历史记录
  if (syncHistory.value.length > 20) {
    syncHistory.value = syncHistory.value.slice(0, 20)
  }
}

function getStatusIcon(status: string): string {
  return status === 'syncing' ? 'is-loading' : ''
}

function getSyncTypeColor(type: string): string {
  const colors = {
    upload: 'warning',
    download: 'success',
    auto: 'info'
  }
  return colors[type as keyof typeof colors] || 'info'
}

function getSyncTypeLabel(type: string): string {
  const labels = {
    upload: '上传',
    download: '下载',
    auto: '自动同步'
  }
  return labels[type as keyof typeof labels] || type
}

function clearHistory() {
  syncHistory.value = []
  ElMessage.success('历史记录已清除')
}

// 监听对话框打开
watch(() => props.show, (show) => {
  if (show && isConnected.value) {
    fetchOverleafProjects()
  }
})
</script>

<style scoped lang="scss">
.overleaf-sync {
  display: flex;
  flex-direction: column;
  gap: 20px;
  padding: 16px;
}

.config-section,
.project-section,
.sync-status,
.history-section {
  h4 {
    margin: 0 0 12px 0;
    font-size: 14px;
    font-weight: 500;
  }
}

.form-tip {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin-top: 4px;
}

.project-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.project-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px;
  background: var(--el-fill-color-blank);
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  transition: all 0.2s;

  &:hover {
    border-color: var(--el-color-primary);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  }
}

.project-info {
  flex: 1;
  min-width: 0;

  h5 {
    margin: 0 0 4px 0;
    font-size: 14px;
  }
}

.project-meta {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  display: flex;
  gap: 8px;
}

.status-item {
  display: flex;
  gap: 12px;
  padding: 12px;
  background: var(--el-fill-color-blank);
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
}

.status-content {
  flex: 1;
}

.status-title {
  font-weight: 500;
  margin-bottom: 4px;
}

.status-message {
  font-size: 13px;
  color: var(--el-text-color-secondary);
}

.history-section {
  flex: 1;
  overflow-y: auto;
  max-height: 300px;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.history-details {
  margin-top: 4px;

  code {
    display: block;
    font-size: 11px;
    color: var(--el-text-color-secondary);
    background: var(--el-fill-color-light);
    padding: 2px 4px;
    border-radius: 3px;
    word-break: break-all;
  }
}

.is-loading {
  animation: rotating 2s linear infinite;
}

@keyframes rotating {
  from {
    transform: rotate(0deg);
  }
  to {
    transform: rotate(360deg);
  }
}
</style>
