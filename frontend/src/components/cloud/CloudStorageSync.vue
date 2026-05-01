<template>
  <div class="cloud-sync-manager">
    <!-- 状态头部 -->
    <div class="sync-header">
      <div class="header-title">
        <el-icon><Cloudy /></el-icon>
        <span>云同步</span>
      </div>
      <div class="header-status">
        <el-tag :type="syncStatusType" size="small">
          {{ syncStatusText }}
        </el-tag>
        <el-switch
          v-model="autoSyncEnabled"
          active-text="自动同步"
          :loading="syncing"
          @change="handleAutoSyncToggle"
        />
      </div>
    </div>

    <!-- 云存储提供商 -->
    <div class="storage-providers">
      <div
        v-for="provider in providers"
        :key="provider.id"
        class="provider-card"
        :class="{ 'is-active': provider.id === activeProvider }"
        @click="selectProvider(provider.id)"
      >
        <div class="provider-icon" :style="{ backgroundColor: provider.color }">
          {{ provider.icon }}
        </div>
        <div class="provider-info">
          <div class="provider-name">{{ provider.name }}</div>
          <div class="provider-status">{{ provider.statusText }}</div>
        </div>
        <el-icon v-if="provider.id === activeProvider" class="check-icon"><Check /></el-icon>
      </div>
    </div>

    <!-- 同步统计 -->
    <div class="sync-stats">
      <div class="stat-item">
        <div class="stat-label">已同步文件</div>
        <div class="stat-value">{{ syncedFiles }}</div>
      </div>
      <div class="stat-item">
        <div class="stat-label">存储空间</div>
        <div class="stat-value">{{ storageUsed }}/{{ storageTotal }}</div>
      </div>
      <div class="stat-item">
        <div class="stat-label">最后同步</div>
        <div class="stat-value">{{ lastSyncTime }}</div>
      </div>
    </div>

    <!-- 文件列表 -->
    <div class="sync-files">
      <div class="files-header">
        <h4>同步文件</h4>
        <el-button size="small" :icon="Plus" @click="showUploadDialog = true">
          添加文件
        </el-button>
      </div>
      <div class="files-list">
        <div
          v-for="file in syncFiles"
          :key="file.id"
          class="file-item"
          :class="`status-${file.syncStatus}`"
        >
          <el-icon class="file-icon" :style="{ color: getIconColor(file.type) }">
            <Document />
          </el-icon>
          <div class="file-info">
            <div class="file-name">{{ file.name }}</div>
            <div class="file-meta">
              <span>{{ formatSize(file.size) }}</span>
              <span>{{ formatTime(file.modifiedAt) }}</span>
            </div>
          </div>
          <div class="file-status">
            <el-tag v-if="file.syncStatus === 'synced'" size="small" type="success">
              已同步
            </el-tag>
            <el-tag v-else-if="file.syncStatus === 'syncing'" size="small" type="warning">
              同步中
            </el-tag>
            <el-tag v-else size="small" type="danger">
              待同步
            </el-tag>
          </div>
          <div class="file-actions">
            <el-button size="small" text @click="syncFile(file)">
              <el-icon><Refresh /></el-icon>
            </el-button>
            <el-button size="small" text @click="removeFile(file)">
              <el-icon><Delete /></el-icon>
            </el-button>
          </div>
        </div>
        <el-empty v-if="syncFiles.length === 0" description="暂无同步文件" />
      </div>
    </div>

    <!-- 上传对话框 -->
    <el-dialog v-model="showUploadDialog" title="添加到云同步" width="500px">
      <el-form label-position="top">
        <el-form-item label="选择文件">
          <el-upload
            ref="uploadRef"
            :auto-upload="false"
            :on-change="handleFileSelect"
            :limit="10"
            multiple
          >
            <el-button type="primary">选择文件</el-button>
          </el-upload>
        </el-form-item>
        <el-form-item label="同步路径">
          <el-input v-model="uploadPath" placeholder="/LaTeX/Projects/" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showUploadDialog = false">取消</el-button>
        <el-button type="primary" @click="uploadFiles" :loading="uploading">
          上传
        </el-button>
      </template>
    </el-dialog>

    <!-- 设置对话框 -->
    <el-dialog v-model="showSettingsDialog" title="云同步设置" width="500px">
      <el-form label-position="top">
        <el-form-item label="同步间隔">
          <el-select v-model="syncInterval">
            <el-option label="实时" value="realtime" />
            <el-option label="5分钟" :value="5" />
            <el-option label="15分钟" :value="15" />
            <el-option label="30分钟" :value="30" />
          </el-select>
        </el-form-item>
        <el-form-item label="冲突策略">
          <el-radio-group v-model="conflictStrategy">
            <el-radio value="local">本地优先</el-radio>
            <el-radio value="remote">云端优先</el-radio>
            <el-radio value="timestamp">最新优先</el-radio>
          </el-radio-group>
        </el-form-item>
        <el-form-item label="网络设置">
          <el-checkbox v-model="syncOnWifi">仅WiFi下同步</el-checkbox>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showSettingsDialog = false">取消</el-button>
        <el-button type="primary" @click="saveSettings">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import {
  Cloudy,
  Check,
  Plus,
  Document,
  Refresh,
  Delete
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface StorageProvider {
  id: string
  name: string
  icon: string
  color: string
  statusText: string
  connected: boolean
}

interface SyncFile {
  id: string
  name: string
  type: string
  size: number
  modifiedAt: number
  syncStatus: 'synced' | 'syncing' | 'pending'
  cloudPath?: string
}

// 状态
const activeProvider = ref<string>('')
const syncing = ref(false)
const autoSyncEnabled = ref(false)
 const showUploadDialog = ref(false)
const showSettingsDialog = ref(false)
const uploading = ref(false)
const uploadPath = ref('/LaTeX/Projects/')
const syncInterval = ref(15)
const conflictStrategy = ref('timestamp')
const syncOnWifi = ref(false)

// 云存储提供商
const providers = ref<StorageProvider[]>([
  {
    id: 'dropbox',
    name: 'Dropbox',
    icon: '📦',
    color: '#0061FE',
    statusText: '未连接',
    connected: false
  },
  {
    id: 'googledrive',
    name: 'Google Drive',
    icon: '🔵',
    color: '#1FA463',
    statusText: '未连接',
    connected: false
  },
  {
    id: 'onedrive',
    name: 'OneDrive',
    icon: '☁️',
    color: '#0078D4',
    statusText: '未连接',
    connected: false
  },
  {
    id: 'webdav',
    name: 'WebDAV',
    icon: '🌐',
    color: '#6C757D',
    statusText: '未连接',
    connected: false
  }
])

// 同步文件
const syncFiles = ref<SyncFile[]>([])
const pendingUpload = ref<File[]>([])

// 计算属性
const syncStatusType = computed(() => {
  if (syncing.value) return 'warning'
  if (autoSyncEnabled.value) return 'success'
  return 'info'
})

const syncStatusText = computed(() => {
  if (syncing.value) return '同步中...'
  if (autoSyncEnabled.value) return '自动同步'
  return '手动同步'
})

const syncedFiles = computed(() =>
  syncFiles.value.filter(f => f.syncStatus === 'synced').length
)

const storageUsed = computed(() => {
  const total = syncFiles.value.reduce((sum, f) => sum + f.size, 0)
  return formatSize(total)
})

const storageTotal = ref('1 GB')
const lastSyncTime = ref('未同步')

// 格式化文件大小
const formatSize = (bytes: number): string => {
  if (bytes === 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB', 'GB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return Math.round(bytes / Math.pow(k, i) * 100) / 100 + ' ' + sizes[i]
}

// 格式化时间
const formatTime = (timestamp: number): string => {
  const date = new Date(timestamp)
  const now = new Date()
  const diff = now.getTime() - date.getTime()

  if (diff < 60000) return '刚刚'
  if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`
  if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`
  return date.toLocaleDateString('zh-CN')
}

// 获取图标颜色
const getIconColor = (type: string): string => {
  const colors: Record<string, string> = {
    'pdf': '#F56C6C',
    'tex': '#409EFF',
    'bib': '#67C23A',
    'png': '#E6A23C',
    'jpg': '#E6A23C'
  }
  return colors[type] || '#909399'
}

// 选择提供商
const selectProvider = async (providerId: string) => {
  const provider = providers.value.find(p => p.id === providerId)
  if (!provider) return

  if (!provider.connected) {
    // 连接云存储
    try {
      syncing.value = true
      await connectProvider(providerId)
      provider.connected = true
      provider.statusText = '已连接'
      activeProvider.value = providerId
      ElMessage.success(`已连接到 ${provider.name}`)
      loadProviderFiles()
    } catch (error) {
      ElMessage.error(`连接失败: ${error}`)
    } finally {
      syncing.value = false
    }
  } else {
    activeProvider.value = providerId
    loadProviderFiles()
  }
}

// 连接云存储提供商
const connectProvider = async (providerId: string): Promise<void> => {
  // 这里应该调用OAuth或API连接
  // 模拟连接过程
  return new Promise((resolve) => {
    setTimeout(() => {
      // 保存连接状态
      localStorage.setItem(`cloud_provider_${providerId}`, 'connected')
      resolve()
    }, 1000)
  })
}

// 加载提供商文件
const loadProviderFiles = () => {
  // 从本地存储加载文件列表
  const savedFiles = localStorage.getItem(`cloud_files_${activeProvider.value}`)
  if (savedFiles) {
    syncFiles.value = JSON.parse(savedFiles)
  }
}

// 同步文件
const syncFile = async (file: SyncFile) => {
  if (!activeProvider.value) {
    ElMessage.warning('请先选择云存储提供商')
    return
  }

  file.syncStatus = 'syncing'
  syncing.value = true

  try {
    // 这里应该调用云存储API上传文件
    await new Promise(resolve => setTimeout(resolve, 2000))

    file.syncStatus = 'synced'
    file.modifiedAt = Date.now()
    lastSyncTime.value = formatTime(Date.now())

    saveFiles()
    ElMessage.success(`已同步: ${file.name}`)
  } catch (error) {
    file.syncStatus = 'pending'
    ElMessage.error(`同步失败: ${file.name}`)
  } finally {
    syncing.value = false
  }
}

// 移除文件
const removeFile = (file: SyncFile) => {
  const index = syncFiles.value.findIndex(f => f.id === file.id)
  if (index >= 0) {
    syncFiles.value.splice(index, 1)
    saveFiles()
    ElMessage.success('已移除文件')
  }
}

// 处理文件选择
const handleFileSelect = (file: any) => {
  pendingUpload.value.push(file.raw)
}

// 上传文件
const uploadFiles = async () => {
  if (pendingUpload.value.length === 0) {
    ElMessage.warning('请选择文件')
    return
  }

  if (!activeProvider.value) {
    ElMessage.warning('请先选择云存储提供商')
    return
  }

  uploading.value = true

  try {
    for (const file of pendingUpload.value) {
      const syncFile: SyncFile = {
        id: Date.now().toString() + Math.random(),
        name: file.name,
        type: file.name.split('.').pop() || '',
        size: file.size,
        modifiedAt: Date.now(),
        syncStatus: 'pending',
        cloudPath: uploadPath.value + file.name
      }
      syncFiles.value.unshift(syncFile)
    }

    saveFiles()
    showUploadDialog.value = false
    pendingUpload.value = []

    // 自动同步
    if (autoSyncEnabled.value) {
      for (const file of syncFiles.value.filter(f => f.syncStatus === 'pending')) {
        await syncFile(file)
      }
    }

    ElMessage.success('文件已添加')
  } catch (error) {
    ElMessage.error('上传失败')
  } finally {
    uploading.value = false
  }
}

// 保存文件列表
const saveFiles = () => {
  localStorage.setItem(
    `cloud_files_${activeProvider.value}`,
    JSON.stringify(syncFiles.value)
  )
}

// 处理自动同步开关
const handleAutoSyncToggle = (enabled: boolean) => {
  if (enabled) {
    ElMessage.success('自动同步已启用')
    // 开始定期同步
    startAutoSync()
  } else {
    ElMessage.info('自动同步已关闭')
    stopAutoSync()
  }
}

// 自动同步定时器
let syncTimer: number | null = null

const startAutoSync = () => {
  if (syncTimer) clearInterval(syncTimer)

  syncTimer = window.setInterval(() => {
    if (autoSyncEnabled.value && !syncing.value) {
      syncFiles.value.filter(f => f.syncStatus === 'pending').forEach(file => {
        syncFile(file)
      })
    }
  }, syncInterval.value === 'realtime' ? 10000 : syncInterval.value * 60000)
}

const stopAutoSync = () => {
  if (syncTimer) {
    clearInterval(syncTimer)
    syncTimer = null
  }
}

// 保存设置
const saveSettings = () => {
  localStorage.setItem('cloud_sync_settings', JSON.stringify({
    syncInterval: syncInterval.value,
    conflictStrategy: conflictStrategy.value,
    syncOnWifi: syncOnWifi.value
  }))
  showSettingsDialog.value = false
  ElMessage.success('设置已保存')
}

// 初始化
onMounted(() => {
  // 加载设置
  const settings = localStorage.getItem('cloud_sync_settings')
  if (settings) {
    const parsed = JSON.parse(settings)
    syncInterval.value = parsed.syncInterval || 15
    conflictStrategy.value = parsed.conflictStrategy || 'timestamp'
    syncOnWifi.value = parsed.syncOnWifi || false
  }

  // 检查已连接的提供商
  providers.value.forEach(provider => {
    const connected = localStorage.getItem(`cloud_provider_${provider.id}`)
    if (connected === 'connected') {
      provider.connected = true
      provider.statusText = '已连接'
    }
  })
})

onUnmounted(() => {
  stopAutoSync()
})
</script>

<style scoped lang="scss">
.cloud-sync-manager {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #f5f7fa;

  .sync-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 16px;
    background: #fff;
    border-bottom: 1px solid #e4e7ed;

    .header-title {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 16px;
      font-weight: 600;
      color: #303133;
    }

    .header-status {
      display: flex;
      align-items: center;
      gap: 12px;
    }
  }

  .storage-providers {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 12px;
    padding: 16px;

    .provider-card {
      background: #fff;
      border-radius: 8px;
      padding: 12px;
      display: flex;
      align-items: center;
      gap: 12px;
      cursor: pointer;
      border: 2px solid transparent;
      transition: all 0.2s;

      &:hover {
        border-color: #c0c4cc;
      }

      &.is-active {
        border-color: #409eff;
        background: #ecf5ff;
      }

      .provider-icon {
        width: 40px;
        height: 40px;
        border-radius: 8px;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 20px;
      }

      .provider-info {
        flex: 1;

        .provider-name {
          font-weight: 500;
          color: #303133;
          margin-bottom: 2px;
        }

        .provider-status {
          font-size: 12px;
          color: #909399;
        }
      }

      .check-icon {
        color: #409eff;
        font-size: 18px;
      }
    }
  }

  .sync-stats {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 12px;
    padding: 0 16px 16px;

    .stat-item {
      background: #fff;
      border-radius: 8px;
      padding: 12px;
      text-align: center;

      .stat-label {
        font-size: 12px;
        color: #909399;
        margin-bottom: 4px;
      }

      .stat-value {
        font-size: 18px;
        font-weight: 600;
        color: #303133;
      }
    }
  }

  .sync-files {
    flex: 1;
    background: #fff;
    margin: 0 16px 16px;
    border-radius: 8px;
    padding: 16px;
    overflow: hidden;
    display: flex;
    flex-direction: column;

    .files-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 16px;

      h4 {
        margin: 0;
        font-size: 14px;
        font-weight: 600;
        color: #303133;
      }
    }

    .files-list {
      flex: 1;
      overflow-y: auto;

      .file-item {
        display: flex;
        align-items: center;
        gap: 12px;
        padding: 12px;
        border-radius: 6px;
        margin-bottom: 8px;
        border: 1px solid #e4e7ed;
        transition: all 0.2s;

        &:hover {
          background: #f5f7fa;
        }

        &.status-synced {
          border-left: 3px solid #67c23a;
        }

        &.status-syncing {
          border-left: 3px solid #e6a23c;
          background: #fff7e6;
        }

        &.status-pending {
          border-left: 3px solid #f56c6c;
        }

        .file-icon {
          font-size: 24px;
        }

        .file-info {
          flex: 1;
          min-width: 0;

          .file-name {
            font-weight: 500;
            color: #303133;
            margin-bottom: 2px;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
          }

          .file-meta {
            display: flex;
            gap: 12px;
            font-size: 12px;
            color: #909399;
          }
        }

        .file-actions {
          display: flex;
          gap: 4px;
        }
      }
    }
  }
}
</style>
