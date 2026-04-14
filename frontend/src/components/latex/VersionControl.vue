<template>
  <div class="version-control">
    <!-- 视图切换器 -->
    <div class="view-switcher">
      <el-radio-group v-model="currentView" size="small">
        <el-radio-button value="tree">
          <el-icon><Share /></el-icon>
          分支图
        </el-radio-button>
        <el-radio-button value="cards">
          <el-icon><Grid /></el-icon>
          卡片
        </el-radio-button>
        <el-radio-button value="diff">
          <el-icon><Document /></el-icon>
          对比
        </el-radio-button>
      </el-radio-group>

      <div class="header-actions">
        <el-button :icon="Refresh" @click="refreshData" :loading="loading" size="small" circle />
        <el-button :icon="DocumentAdd" @click="showSaveDialog = true" type="primary" size="small">
          保存版本
        </el-button>
      </div>
    </div>

    <!-- 分支树形图视图 -->
    <div v-show="currentView === 'tree'" class="view-container tree-view">
      <BranchTreeView
        :versions="versionTree"
        :selected-version="selectedVersion"
        @select="handleSelectVersion"
        @compare="handleCompareVersions"
      />
    </div>

    <!-- 卡片视图 -->
    <div v-show="currentView === 'cards'" class="view-container cards-view">
      <div class="cards-grid">
        <VersionCard
          v-for="version in sortedVersions"
          :key="version.id"
          :version="version"
          :selected="selectedVersion?.id === version.id"
          :comparing="compareVersions.find(v => v.id === version.id) !== undefined"
          @select="handleSelectVersion"
          @restore="handleRestoreVersion"
          @toggle-compare="handleToggleCompare"
        />
      </div>
      <el-empty v-if="sortedVersions.length === 0" description="暂无版本历史" />
    </div>

    <!-- 差异对比视图 -->
    <div v-show="currentView === 'diff'" class="view-container diff-view">
      <DiffViewer
        :versions="compareVersions"
        :file-id="fileId"
        :project-id="projectId"
        :user-id="userId"
        @clear-compare="handleClearCompare"
      />
      <el-empty v-if="compareVersions.length === 0" description="请选择2个版本进行对比" />
    </div>

    <!-- 保存版本对话框 -->
    <el-dialog v-model="showSaveDialog" title="保存版本" width="500px">
      <el-form :model="saveForm" label-width="100px">
        <el-form-item label="版本摘要">
          <el-input
            v-model="saveForm.summary"
            type="textarea"
            :rows="3"
            placeholder="描述本次更改的内容..."
            maxlength="200"
            show-word-limit
          />
        </el-form-item>
        <el-form-item label="自动保存">
          <el-switch v-model="saveForm.isAutoSave" />
          <span class="form-tip">自动保存的版本不会计入分支</span>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showSaveDialog = false">取消</el-button>
        <el-button type="primary" @click="handleSaveVersion" :loading="saving">
          保存
        </el-button>
      </template>
    </el-dialog>

    <!-- 版本详情抽屉 -->
    <el-drawer v-model="showDetailDrawer" :title="`版本详情 - ${selectedVersion?.summary || '未命名'}`" size="50%">
      <VersionDetail
        v-if="selectedVersion"
        :version="selectedVersion"
        :current-content="currentContent"
        @restore="handleRestoreVersion"
      />
    </el-drawer>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from 'vue'
import { Share, Grid, Document, Refresh, DocumentAdd } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import BranchTreeView from './BranchTreeView.vue'
import VersionCard from './VersionCard.vue'
import DiffViewer from './DiffViewer.vue'
import VersionDetail from './VersionDetail.vue'
import {
  getLatexVersionTree,
  getLatexVersionHistory,
  saveLatexVersion,
  restoreLatexVersion,
  compareLatexVersions,
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

// 视图状态
const currentView = ref<'tree' | 'cards' | 'diff'>('tree')
const loading = ref(false)
const saving = ref(false)

// 版本数据
const versionTree = ref<FrontendLatexVersionNode[]>([])
const versionHistory = ref<FrontendLatexVersionNode[]>([])
const selectedVersion = ref<FrontendLatexVersionNode | null>(null)
const compareVersions = ref<FrontendLatexVersionNode[]>([])

// 对话框状态
const showSaveDialog = ref(false)
const showDetailDrawer = ref(false)
const saveForm = ref({
  summary: '',
  isAutoSave: false
})

// 计算属性：排序后的版本列表（用于卡片视图）
const sortedVersions = computed(() => {
  return [...versionHistory.value].sort((a, b) =>
    new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime()
  )
})

// 加载版本树
const loadVersionTree = async () => {
  loading.value = true
  try {
    const tree = await getLatexVersionTree(props.fileId, props.projectId, props.userId)
    versionTree.value = tree
  } catch (error) {
    console.error('Failed to load version tree:', error)
    ElMessage.error('加载版本树失败')
  } finally {
    loading.value = false
  }
}

// 加载版本历史
const loadVersionHistory = async () => {
  loading.value = true
  try {
    const history = await getLatexVersionHistory(props.fileId, props.projectId, props.userId)
    versionHistory.value = history
  } catch (error) {
    console.error('Failed to load version history:', error)
    ElMessage.error('加载版本历史失败')
  } finally {
    loading.value = false
  }
}

// 刷新数据
const refreshData = () => {
  loadVersionTree()
  loadVersionHistory()
}

// 选择版本
const handleSelectVersion = (version: FrontendLatexVersionNode) => {
  selectedVersion.value = version
  showDetailDrawer.value = true
}

// 恢复版本
const handleRestoreVersion = async (version: FrontendLatexVersionNode) => {
  try {
    const result = await restoreLatexVersion({ versionId: version.id })
    emit('restore', result.content)
    ElMessage.success('版本已恢复，请记得保存更改')
    showDetailDrawer.value = false
    await refreshData()
  } catch (error) {
    console.error('Failed to restore version:', error)
    ElMessage.error('恢复版本失败')
  }
}

// 切换对比版本
const handleToggleCompare = (version: FrontendLatexVersionNode) => {
  const index = compareVersions.value.findIndex(v => v.id === version.id)
  if (index >= 0) {
    compareVersions.value.splice(index, 1)
  } else {
    if (compareVersions.value.length >= 2) {
      ElMessage.warning('最多只能对比2个版本')
      return
    }
    compareVersions.value.push(version)
  }

  // 如果有2个版本了，自动切换到对比视图
  if (compareVersions.value.length === 2) {
    currentView.value = 'diff'
  }
}

// 对比版本
const handleCompareVersions = (versions: FrontendLatexVersionNode[]) => {
  compareVersions.value = versions
  currentView.value = 'diff'
}

// 清除对比
const handleClearCompare = () => {
  compareVersions.value = []
}

// 保存版本
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
      summary: saveForm.value.summary || '自动保存',
      isAutoSave: saveForm.value.isAutoSave
    })

    showSaveDialog.value = false
    saveForm.value.summary = ''
    saveForm.value.isAutoSave = false

    ElMessage.success('版本保存成功')
    await refreshData()
  } catch (error) {
    console.error('Failed to save version:', error)
    ElMessage.error('保存版本失败')
  } finally {
    saving.value = false
  }
}

onMounted(() => {
  refreshData()
})

watch(() => [props.fileId, props.projectId], () => {
  refreshData()
})
</script>

<style scoped lang="scss">
.version-control {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #f5f7fa;

  .view-switcher {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 16px;
    background: #fff;
    border-bottom: 1px solid #e4e7ed;

    .header-actions {
      display: flex;
      gap: 8px;
      align-items: center;
    }

    .form-tip {
      margin-left: 8px;
      font-size: 12px;
      color: #909399;
    }
  }

  .view-container {
    flex: 1;
    overflow: auto;
    padding: 16px;
  }

  .cards-view {
    .cards-grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
      gap: 16px;
    }
  }

  .tree-view,
  .diff-view {
    background: #fff;
    border-radius: 4px;
  }
}
</style>
