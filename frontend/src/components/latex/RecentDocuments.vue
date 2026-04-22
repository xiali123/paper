<template>
  <div class="recent-documents">
    <div class="recent-header">
      <h3>最近文档</h3>
      <div class="header-actions">
        <el-input
          v-model="searchQuery"
          placeholder="搜索文档..."
          prefix-icon="Search"
          size="small"
          clearable
          style="width: 200px;"
        />
        <el-dropdown @command="handleSortCommand" trigger="click">
          <el-button size="small" :icon="ArrowDown">
            排序
          </el-button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item command="recent">最近打开</el-dropdown-item>
              <el-dropdown-item command="name">名称</el-dropdown-item>
              <el-dropdown-item command="modified">修改时间</el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
    </div>

    <div v-if="loading" class="recent-loading">
      <el-skeleton :rows="3" animated />
    </div>

    <div v-else-if="sortedDocuments.length === 0" class="recent-empty">
      <el-empty
        :description="searchQuery ? '没有找到匹配的文档' : '暂无最近文档'"
        :image-size="80"
      >
        <el-button v-if="!searchQuery" type="primary" @click="$emit('create-new')">
          创建新文档
        </el-button>
      </el-empty>
    </div>

    <div v-else class="recent-list">
      <div
        v-for="doc in sortedDocuments"
        :key="doc.id"
        class="recent-item"
        @click="openDocument(doc)"
        @contextmenu.prevent="showContextMenu($event, doc)"
      >
        <div class="item-icon">
          <el-icon v-if="doc.type === 'project'"><Folder /></el-icon>
          <el-icon v-else><Document /></el-icon>
        </div>

        <div class="item-content">
          <div class="item-header">
            <h4 class="item-title">{{ doc.name }}</h4>
            <div class="item-meta">
              <span v-if="doc.type === 'project'" class="project-badge">
                项目
              </span>
              <span v-if="doc.isModified" class="modified-badge">
                未保存
              </span>
              <span class="item-time">{{ formatTime(doc.lastModified) }}</span>
            </div>
          </div>

          <div class="item-preview">
            <p>{{ doc.preview || '暂无预览' }}</p>
          </div>

          <div class="item-stats">
            <div class="stat-item" v-if="doc.wordCount">
              <el-icon><Notebook /></el-icon>
              <span>{{ formatNumber(doc.wordCount) }} 字</span>
            </div>
            <div class="stat-item" v-if="doc.formulaCount">
              <span class="stat-icon">∑</span>
              <span>{{ doc.formulaCount }} 公式</span>
            </div>
          </div>
        </div>

        <div class="item-actions">
          <el-button
            text
            size="small"
            @click.stop="favoriteDocument(doc)"
            :class="{ 'is-favorite': doc.isFavorite }"
          >
            <el-icon><StarFilled v-if="doc.isFavorite" /><Star v-else /></el-icon>
          </el-button>
          <el-dropdown @command="(cmd) => handleItemCommand(cmd, doc)" trigger="click">
            <el-button text size="small">
              <el-icon><MoreFilled /></el-icon>
            </el-button>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="open">
                  <el-icon><FolderOpened /></el-icon>
                  打开
                </el-dropdown-item>
                <el-dropdown-item command="duplicate">
                  <el-icon><DocumentCopy /></el-icon>
                  复制
                </el-dropdown-item>
                <el-dropdown-item command="rename">
                  <el-icon><Edit /></el-icon>
                  重命名
                </el-dropdown-item>
                <el-dropdown-item command="export">
                  <el-icon><Download /></el-icon>
                  导出
                </el-dropdown-item>
                <el-dropdown-item command="delete" divided>
                  <el-icon><Delete /></el-icon>
                  删除
                </el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </div>
    </div>

    <!-- 右键菜单 -->
    <transition name="fade">
      <div
        v-if="contextMenu.show"
        class="context-menu"
        :style="{ left: contextMenu.x + 'px', top: contextMenu.y + 'px' }"
        @click.stop
      >
        <div
          v-for="item in contextMenuItems"
          :key="item.command"
          class="context-menu-item"
          @click="handleItemCommand(item.command, contextMenu.doc)"
        >
          <el-icon>{{ item.icon }}</el-icon>
          <span>{{ item.label }}</span>
        </div>
      </div>
    </transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import {
  Search,
  ArrowDown,
  Folder,
  Document,
  Notebook,
  Star,
  StarFilled,
  MoreFilled,
  FolderOpened,
  DocumentCopy,
  Edit,
  Download,
  Delete
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'

interface RecentDocument {
  id: string
  name: string
  type: 'document' | 'project'
  lastModified: number
  preview?: string
  wordCount?: number
  formulaCount?: number
  isModified?: boolean
  isFavorite?: boolean
  path?: string
}

interface Props {
  documents?: RecentDocument[]
  loading?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  documents: () => [],
  loading: false
})

const emit = defineEmits<{
  'open': [document: RecentDocument]
  'create-new': []
  'favorite': [document: RecentDocument]
  'duplicate': [document: RecentDocument]
  'rename': [document: RecentDocument]
  'delete': [document: RecentDocument]
  'export': [document: RecentDocument]
}>()

const searchQuery = ref('')
const sortBy = ref<'recent' | 'name' | 'modified'>('recent')
const contextMenu = ref({
  show: false,
  x: 0,
  y: 0,
  doc: null as RecentDocument | null
})

const contextMenuItems = [
  { command: 'open', label: '打开', icon: FolderOpened },
  { command: 'duplicate', label: '复制', icon: DocumentCopy },
  { command: 'rename', label: '重命名', icon: Edit },
  { command: 'export', label: '导出', icon: Download },
  { command: 'delete', label: '删除', icon: Delete },
]

const sortedDocuments = computed(() => {
  let docs = [...props.documents]

  // 搜索过滤
  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    docs = docs.filter(doc =>
      doc.name.toLowerCase().includes(query) ||
      doc.path?.toLowerCase().includes(query)
    )
  }

  // 排序
  docs.sort((a, b) => {
    switch (sortBy.value) {
      case 'recent':
        return b.lastModified - a.lastModified
      case 'name':
        return a.name.localeCompare(b.name)
      case 'modified':
        return b.lastModified - a.lastModified
      default:
        return 0
    }
  })

  return docs
})

function openDocument(doc: RecentDocument) {
  emit('open', doc)
}

function favoriteDocument(doc: RecentDocument) {
  emit('favorite', doc)
}

function handleSortCommand(command: string) {
  sortBy.value = command as any
}

function handleItemCommand(command: string, doc: RecentDocument) {
  contextMenu.value.show = false

  switch (command) {
    case 'open':
      openDocument(doc)
      break
    case 'duplicate':
      emit('duplicate', doc)
      break
    case 'rename':
      emit('rename', doc)
      break
    case 'export':
      emit('export', doc)
      break
    case 'delete':
      ElMessageBox.confirm(
        `确定要删除"${doc.name}"吗？`,
        '确认删除',
        {
          type: 'warning',
          confirmButtonText: '删除',
          cancelButtonText: '取消'
        }
      ).then(() => {
        emit('delete', doc)
        ElMessage.success('文档已删除')
      }).catch(() => {
        // 用户取消
      })
      break
  }
}

function showContextMenu(event: MouseEvent, doc: RecentDocument) {
  contextMenu.value = {
    show: true,
    x: event.clientX,
    y: event.clientY,
    doc
  }

  // 点击外部关闭菜单
  const closeMenu = () => {
    contextMenu.value.show = false
    document.removeEventListener('click', closeMenu)
  }
  setTimeout(() => {
    document.addEventListener('click', closeMenu)
  }, 0)
}

function formatTime(timestamp: number): string {
  const now = Date.now()
  const diff = now - timestamp

  if (diff < 60000) {
    return '刚刚'
  } else if (diff < 3600000) {
    return `${Math.floor(diff / 60000)}分钟前`
  } else if (diff < 86400000) {
    return `${Math.floor(diff / 3600000)}小时前`
  } else if (diff < 604800000) {
    return `${Math.floor(diff / 86400000)}天前`
  } else {
    const date = new Date(timestamp)
    return `${date.getMonth() + 1}/${date.getDate()}`
  }
}

function formatNumber(num: number): string {
  if (num >= 10000) {
    return `${(num / 10000).toFixed(1)}w`
  } else if (num >= 1000) {
    return `${(num / 1000).toFixed(1)}k`
  }
  return num.toString()
}

// 点击外部关闭右键菜单
onMounted(() => {
  document.addEventListener('click', () => {
    contextMenu.value.show = false
  })
})
</script>

<style scoped lang="scss">
.recent-documents {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.recent-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  border-bottom: 1px solid var(--el-border-color-light);

  h3 {
    margin: 0;
    font-size: 16px;
    font-weight: 600;
    color: var(--el-text-color-primary);
  }
}

.header-actions {
  display: flex;
  gap: 8px;
  align-items: center;
}

.recent-loading {
  padding: 16px;
}

.recent-empty {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}

.recent-list {
  flex: 1;
  overflow-y: auto;
  padding: 8px;
}

.recent-item {
  display: flex;
  gap: 12px;
  padding: 12px;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    background: var(--el-fill-color-light);
  }

  &:active {
    transform: scale(0.98);
  }
}

.item-icon {
  flex-shrink: 0;
  width: 40px;
  height: 40px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 8px;
  background: var(--el-fill-color);

  .el-icon {
    font-size: 20px;
    color: var(--el-color-primary);
  }
}

.item-content {
  flex: 1;
  min-width: 0;
}

.item-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 4px;
}

.item-title {
  margin: 0;
  font-size: 14px;
  font-weight: 500;
  color: var(--el-text-color-primary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.item-meta {
  display: flex;
  gap: 6px;
  align-items: center;
}

.project-badge,
.modified-badge {
  font-size: 11px;
  padding: 2px 6px;
  border-radius: 3px;
}

.project-badge {
  background: var(--el-color-primary-light-9);
  color: var(--el-color-primary);
}

.modified-badge {
  background: var(--el-color-warning-light-9);
  color: var(--el-color-warning);
}

.item-time {
  font-size: 12px;
  color: var(--el-text-color-secondary);
}

.item-preview {
  margin-bottom: 8px;

  p {
    margin: 0;
    font-size: 12px;
    color: var(--el-text-color-secondary);
    overflow: hidden;
    text-overflow: ellipsis;
    display: -webkit-box;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
  }
}

.item-stats {
  display: flex;
  gap: 12px;
}

.stat-item {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  color: var(--el-text-color-secondary);

  .el-icon {
    font-size: 14px;
  }

  .stat-icon {
    font-weight: bold;
  }
}

.item-actions {
  flex-shrink: 0;
  display: flex;
  align-items: center;
}

.is-favorite {
  color: var(--el-color-warning) !important;
}

// 右键菜单
.context-menu {
  position: fixed;
  min-width: 150px;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color);
  border-radius: 6px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.15);
  z-index: 9999;
  padding: 4px 0;
}

.context-menu-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  cursor: pointer;
  transition: background 0.2s;
  font-size: 13px;
  color: var(--el-text-color-regular);

  &:hover {
    background: var(--el-fill-color-light);
  }

  .el-icon {
    font-size: 14px;
  }
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.2s;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>
