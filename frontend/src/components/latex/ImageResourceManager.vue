/**
 * 图片资源管理器 (深度优化版)
 * 管理LaTeX文档中的图片，支持拖拽上传、自动引用生成
 * 全新UI设计，现代化交互体验
 */

<template>
  <div class="image-resource-manager">
    <!-- 头部工具栏 -->
    <div class="manager-header">
      <div class="header-left">
        <div class="header-title">
          <el-icon :size="20" color="var(--el-color-primary)"><Picture /></el-icon>
          <span>图片资源</span>
          <el-tag v-if="totalImages > 0" type="info" effect="plain" size="small" class="count-tag">
            {{ totalImages }} 张
          </el-tag>
        </div>
      </div>
      <div class="header-actions">
        <!-- 搜索框 -->
        <el-input
          v-model="searchQuery"
          placeholder="搜索图片..."
          :prefix-icon="Search"
          clearable
          class="main-search"
        />

        <!-- 筛选和排序 -->
        <el-dropdown trigger="click">
          <el-button>
            <el-icon><Filter /></el-icon>
            筛选
            <el-icon class="el-icon--right"><ArrowDown /></el-icon>
          </el-button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item v-for="type in [{label: '全部类型', value: ''}, {label: 'PNG', value: 'png'}, {label: 'JPG', value: 'jpg'}, {label: 'SVG', value: 'svg'}]" :key="type.value" @click="filterType = type.value">
                {{ type.label }}
              </el-dropdown-item>
              <el-dropdown-item divided v-for="sort in [{label: '按名称', value: 'name'}, {label: '按大小', value: 'size'}, {label: '按日期', value: 'date'}]" :key="sort.value" @click="sortBy = sort.value">
                {{ sort.label }}
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>

        <!-- 操作按钮 -->
        <el-button-group>
          <el-tooltip content="上传图片" placement="bottom">
            <el-button type="primary" :icon="Upload" @click="triggerUpload">
              上传
            </el-button>
          </el-tooltip>
          <el-dropdown trigger="click">
            <el-button>
              更多
              <el-icon class="el-icon--right"><ArrowDown /></el-icon>
            </el-button>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item @click="createFolder">
                  <el-icon><FolderAddIcon /></el-icon>
                  新建文件夹
                </el-dropdown-item>
                <el-dropdown-item @click="refreshFolders">
                  <el-icon><Refresh /></el-icon>
                  刷新
                </el-dropdown-item>
                <el-dropdown-item divided v-if="selectedCount > 0" @click="handleBatchAction('edit')">
                  <el-icon><Edit /></el-icon>
                  批量编辑 ({{ selectedCount }})
                </el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </el-button-group>
      </div>
    </div>

    <!-- 主内容区 -->
    <div class="manager-content">
      <!-- 侧边栏 -->
      <div class="manager-sidebar" :class="{ 'is-collapsed': sidebarCollapsed }">
        <div class="sidebar-header">
          <span v-if="!sidebarCollapsed" class="sidebar-title">资源文件夹</span>
          <el-button
            :icon="sidebarCollapsed ? ArrowRight : ArrowLeft"
            text
            size="small"
            @click="sidebarCollapsed = !sidebarCollapsed"
          />
        </div>
        <div class="sidebar-tree">
          <el-tree
            ref="folderTreeRef"
            :data="folderTree"
            :props="treeProps"
            node-key="id"
            :default-expand-all="!sidebarCollapsed"
            highlight-current
            @node-click="handleFolderClick"
            :class="{ 'is-collapsed': sidebarCollapsed }"
          >
            <template #default="{ node, data }">
              <div class="tree-node" :class="{ 'is-collapsed': sidebarCollapsed }">
                <el-icon class="node-icon">
                  <Folder v-if="data.type === 'folder'" />
                  <Picture v-else />
                </el-icon>
                <span v-if="!sidebarCollapsed" class="node-label">{{ node.label }}</span>
                <el-tag v-if="!sidebarCollapsed && data.count" size="small" type="info">
                  {{ data.count }}
                </el-tag>
              </div>
            </template>
          </el-tree>
        </div>

        <!-- 存储信息 -->
        <div v-if="!sidebarCollapsed" class="sidebar-footer">
          <div class="storage-info">
            <div class="storage-bar">
              <div class="storage-used" :style="{ width: storagePercent + '%' }"></div>
            </div>
            <div class="storage-text">
              {{ formatSize(totalSize) }} / {{ formatSize(maxStorage) }}
            </div>
          </div>
        </div>
      </div>

      <!-- 图片网格区 -->
      <div class="manager-main">
        <!-- 面包屑导航 -->
        <div class="breadcrumb-bar">
          <el-breadcrumb separator="/">
            <el-breadcrumb-item
              v-for="item in breadcrumb"
              :key="item.id"
              @click="navigateToFolder(item)"
            >
              <el-icon><Folder /></el-icon>
              {{ item.name }}
            </el-breadcrumb-item>
          </el-breadcrumb>
          <div class="breadcrumb-actions">
            <el-button-group size="small">
              <el-button
                :icon="Grid"
                :type="viewMode === 'grid' ? 'primary' : ''"
                @click="viewMode = 'grid'"
              />
              <el-button
                :icon="List"
                :type="viewMode === 'list' ? 'primary' : ''"
                @click="viewMode = 'list'"
              />
            </el-button-group>
          </div>
        </div>

        <!-- 图片网格视图 -->
        <div
          class="image-container"
          @dragover.prevent="handleDragOver"
          @drop.prevent="handleDrop"
          @dragleave.prevent="handleDragLeave"
          :class="{ 'is-dragging': isDragging, 'is-list-view': viewMode === 'list' }"
        >
          <!-- 拖拽上传区 -->
          <div
            v-if="displayedImages.length === 0 || isDragging"
            class="upload-zone"
            :class="{ 'is-active': isDragging }"
          >
            <div class="upload-illustration">
              <el-icon :size="60" color="var(--el-color-primary)">
                <UploadFilled />
              </el-icon>
            </div>
            <div class="upload-text">拖拽图片到此处</div>
            <div class="upload-hint">支持 PNG, JPG, JPEG, PDF, SVG 格式</div>
            <el-button type="primary" size="large" @click="triggerUpload">
              <el-icon><Upload /></el-icon>
              选择图片
            </el-button>
          </div>

          <!-- 图片卡片 -->
          <div
            v-for="image in displayedImages"
            :key="image.id"
            class="image-card"
            :class="{ 'is-selected': selectedImages.has(image.id), 'is-favorite': image.isFavorite }"
            @click="selectImage(image)"
            @dblclick="openPreview(image)"
          >
            <!-- 收藏按钮 -->
            <div class="card-favorite" @click.stop="toggleFavorite(image, $event)">
              <el-icon :class="{ 'is-active': image.isFavorite }">
                <Star />
              </el-icon>
            </div>

            <div class="card-checkbox">
              <el-checkbox
                :model-value="selectedImages.has(image.id)"
                @change="toggleSelectImage(image, $event)"
                @click.stop
              />
            </div>
            <div class="card-preview">
              <img
                :src="image.url"
                :alt="image.name"
                loading="lazy"
                @error="handleImageError"
              />
              <div class="preview-overlay">
                <div class="overlay-actions">
                  <el-tooltip content="插入引用" placement="top">
                    <el-button
                      size="small"
                      circle
                      type="primary"
                      @click.stop="insertReference(image)"
                    >
                      <el-icon><Plus /></el-icon>
                    </el-button>
                  </el-tooltip>
                  <el-tooltip content="编辑" placement="top">
                    <el-button
                      size="small"
                      circle
                      @click.stop="editImage(image)"
                    >
                      <el-icon><Edit /></el-icon>
                    </el-button>
                  </el-tooltip>
                  <el-tooltip content="预览" placement="top">
                    <el-button
                      size="small"
                      circle
                      @click.stop="openPreview(image)"
                    >
                      <el-icon><ZoomIn /></el-icon>
                    </el-button>
                  </el-tooltip>
                  <el-tooltip content="删除" placement="top">
                    <el-button
                      size="small"
                      circle
                      type="danger"
                      @click.stop="deleteImage(image)"
                    >
                      <el-icon><Delete /></el-icon>
                    </el-button>
                  </el-tooltip>
                </div>
              </div>
              <!-- 使用次数标记 -->
              <div v-if="image.usedCount && image.usedCount > 0" class="usage-badge">
                <el-icon><DataAnalysis /></el-icon>
                {{ image.usedCount }}
              </div>
              <div class="type-badge">
                {{ image.type.toUpperCase() }}
              </div>
            </div>
            <div class="card-info">
              <div class="image-name" :title="image.name">
                {{ image.name }}
              </div>
              <div v-if="image.description" class="image-description">
                {{ image.description }}
              </div>
              <div class="image-meta">
                <span class="meta-item">
                  <el-icon><Picture /></el-icon>
                  {{ image.width }}×{{ image.height }}
                </span>
                <span class="meta-item">
                  <el-icon><Document /></el-icon>
                  {{ formatSize(image.size) }}
                </span>
                <span v-if="image.usedCount" class="meta-item usage">
                  <el-icon><DataAnalysis /></el-icon>
                  {{ image.usedCount }}次
                </span>
              </div>
              <div v-if="image.tags && image.tags.length > 0" class="image-tags">
                <el-tag
                  v-for="tag in image.tags.slice(0, 3)"
                  :key="tag"
                  size="small"
                  type="info"
                  effect="plain"
                  class="image-tag"
                >
                  {{ tag }}
                </el-tag>
                <span v-if="image.tags.length > 3" class="more-tags">+{{ image.tags.length - 3 }}</span>
              </div>
            </div>
          </div>

          <!-- 空状态 -->
          <div v-if="displayedImages.length === 0 && !isDragging" class="empty-state">
            <div class="empty-illustration">
              <el-icon :size="64" color="var(--el-text-color-placeholder)">
                <Picture />
              </el-icon>
            </div>
            <div class="empty-title">此文件夹为空</div>
            <div class="empty-desc">上传图片开始管理您的资源</div>
            <el-button type="primary" @click="triggerUpload">
              <el-icon><Upload /></el-icon>
              上传第一张图片
            </el-button>
          </div>
        </div>
      </div>
    </div>

    <!-- 预览对话框 -->
    <el-dialog
      v-model="showPreviewDialog"
      title=""
      width="90%"
      top="3vh"
      class="preview-dialog"
      append-to-body
    >
      <div class="preview-content">
        <div class="preview-image">
          <div class="preview-controls">
            <el-button-group>
              <el-tooltip content="放大" placement="left">
                <el-button :icon="ZoomIn" @click="previewZoom = Math.min(previewZoom + 0.25, 3)" size="small" />
              </el-tooltip>
              <el-tooltip content="缩小" placement="top">
                <el-button :icon="ZoomOut" @click="previewZoom = Math.max(previewZoom - 0.25, 0.25)" size="small" />
              </el-tooltip>
              <el-tooltip content="重置" placement="top">
                <el-button @click="resetPreviewTransform" size="small">
                  {{ Math.round(previewZoom * 100) }}%
                </el-button>
              </el-tooltip>
            </el-button-group>
            <el-button-group>
              <el-tooltip content="左旋转" placement="top">
                <el-button :icon="RefreshLeft" @click="previewRotation -= 90" size="small" />
              </el-tooltip>
              <el-tooltip content="右旋转" placement="top">
                <el-button :icon="Refresh" @click="previewRotation += 90" size="small" />
              </el-tooltip>
              <el-tooltip content="重置" placement="top">
                <el-button @click="resetPreviewTransform" size="small">
                  {{ previewRotation }}°
                </el-button>
              </el-tooltip>
            </el-button-group>
            <el-tag v-if="previewImage?.tags && previewImage.tags.length > 0" size="small">
              <el-icon><TagIcon /></el-icon>
              {{ previewImage.tags.join(', ') }}
            </el-tag>
          </div>
          <div class="preview-image-container">
            <img
              :src="previewImage?.url"
              :alt="previewImage?.name"
              :style="{
                transform: `scale(${previewZoom}) rotate(${previewRotation}deg)`
              }"
            />
          </div>
        </div>
        <div class="preview-sidebar">
          <div class="preview-header">
            <div class="preview-title">{{ previewImage?.name }}</div>
            <el-button :icon="Download" @click="downloadImage">下载</el-button>
          </div>

          <el-descriptions :column="1" size="small" border class="preview-details">
            <el-descriptions-item label="尺寸">
              {{ previewImage?.width }}×{{ previewImage?.height }} px
            </el-descriptions-item>
            <el-descriptions-item label="文件大小">
              {{ previewImage ? formatSize(previewImage.size) : '-' }}
            </el-descriptions-item>
            <el-descriptions-item label="格式">
              {{ previewImage?.type?.toUpperCase() }}
            </el-descriptions-item>
            <el-descriptions-item label="路径">
              <code>{{ previewImage?.path }}</code>
            </el-descriptions-item>
          </el-descriptions>

          <el-divider />

          <div class="reference-section">
            <div class="section-title">
              <el-icon><CodeIcon /></el-icon>
              LaTeX 引用
            </div>
            <el-tabs v-model="referenceType" class="reference-tabs">
              <el-tab-pane label="基础" name="basic">
                <el-input
                  :model-value="generateImageCommand(previewImage, 'basic')"
                  type="textarea"
                  :rows="3"
                  readonly
                  class="code-input"
                >
                  <template #append>
                    <el-button @click="copyReference('basic')">复制</el-button>
                  </template>
                </el-input>
              </el-tab-pane>
              <el-tab-pane label="指定宽度" name="width">
                <el-input
                  :model-value="generateImageCommand(previewImage, 'width')"
                  type="textarea"
                  :rows="3"
                  readonly
                  class="code-input"
                >
                  <template #append>
                    <el-button @click="copyReference('width')">复制</el-button>
                  </template>
                </el-input>
              </el-tab-pane>
              <el-tab-pane label="图片环境" name="figure">
                <el-input
                  :model-value="generateImageCommand(previewImage, 'figure')"
                  type="textarea"
                  :rows="5"
                  readonly
                  class="code-input"
                >
                  <template #append>
                    <el-button @click="copyReference('figure')">复制</el-button>
                  </template>
                </el-input>
              </el-tab-pane>
            </el-tabs>
          </div>

          <div class="preview-actions">
            <el-button-group style="width: 100%">
              <el-button @click="insertReference(previewImage!)">
                <el-icon><Plus /></el-icon>
                插入
              </el-button>
              <el-button @click="editImage(previewImage!)">
                <el-icon><Edit /></el-icon>
                编辑
              </el-button>
              <el-button type="danger" @click="deleteImage(previewImage!)">
                <el-icon><Delete /></el-icon>
                删除
              </el-button>
            </el-button-group>
          </div>
        </div>
      </div>
    </el-dialog>

    <!-- 编辑对话框 -->
    <el-dialog
      v-model="showEditDialog"
      title="编辑图片"
      width="600px"
      class="edit-dialog"
    >
      <el-form ref="imageFormRef" :model="currentEditImage" label-width="100px">
        <el-form-item label="文件名">
          <el-input v-model="currentEditImage.name" />
        </el-form-item>

        <el-form-item label="移动到">
          <el-tree-select
            v-model="currentEditImage.folder"
            :data="folderTreeForSelect"
            :props="treeProps"
            check-strictly
            placeholder="选择文件夹"
          />
        </el-form-item>

        <el-form-item label="图片操作">
          <div class="edit-operations">
            <el-button @click="optimizeImage" :loading="optimizing">
              <el-icon><MagicStick /></el-icon>
              压缩图片
            </el-button>
            <el-button @click="convertImage">
              <el-icon><Switch /></el-icon>
              转换格式
            </el-button>
          </div>
        </el-form-item>
      </el-form>

      <template #footer>
        <div class="dialog-footer">
          <el-button @click="showEditDialog = false">取消</el-button>
          <el-button type="primary" @click="saveImageEdit">保存</el-button>
        </div>
      </template>
    </el-dialog>

    <!-- 批量编辑对话框 -->
    <el-dialog
      v-model="showBatchEditDialog"
      :title="`批量编辑 (${selectedCount} 张图片)`"
      width="600px"
      class="batch-edit-dialog"
    >
      <el-form label-width="100px">
        <el-form-item label="添加标签">
          <el-select
            v-model="batchEditData.addTags"
            multiple
            filterable
            allow-create
            placeholder="选择或输入新标签"
            style="width: 100%"
          >
            <el-option
              v-for="tag in allTags"
              :key="tag"
              :label="tag"
              :value="tag"
            />
          </el-select>
        </el-form-item>

        <el-form-item label="移除标签">
          <el-select
            v-model="batchEditData.removeTags"
            multiple
            placeholder="选择要移除的标签"
            style="width: 100%"
          >
            <el-option
              v-for="tag in allTags"
              :key="tag"
              :label="tag"
              :value="tag"
            />
          </el-select>
        </el-form-item>

        <el-form-item label="设置分类">
          <el-select
            v-model="batchEditData.category"
            placeholder="选择分类"
            clearable
            style="width: 100%"
          >
            <el-option label="图表" value="chart" />
            <el-option label="截图" value="screenshot" />
            <el-option label="照片" value="photo" />
            <el-option label="图标" value="icon" />
            <el-option label="插图" value="illustration" />
          </el-select>
        </el-form-item>

        <el-alert
          type="info"
          :closable="false"
          show-icon
        >
          批量操作将影响所有选中的 {{ selectedCount }} 张图片
        </el-alert>
      </el-form>

      <template #footer>
        <div class="dialog-footer">
          <el-button @click="showBatchEditDialog = false">取消</el-button>
          <el-button type="primary" @click="saveBatchEdit">
            应用更改
          </el-button>
        </div>
      </template>
    </el-dialog>

    <!-- 隐藏的文件输入 -->
    <input
      ref="fileInputRef"
      type="file"
      accept="image/*,.pdf"
      multiple
      style="display: none"
      @change="handleFileSelect"
    >
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import {
  Search,
  Upload,
  UploadFilled,
  FolderOpened,
  Folder,
  Picture,
  Refresh,
  Plus,
  ArrowDown,
  Edit,
  Delete,
  MagicStick,
  ArrowLeft,
  ArrowRight,
  Grid,
  List,
  Document,
  Download,
  Switch,
  FolderAdd as FolderAddIcon,
  Tickets as CodeIcon,
  ZoomIn,
  ZoomOut,
  RefreshLeft,
  Sort,
  PriceTag as Tag,
  Star,
  DataAnalysis,
  Filter
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'

interface ImageFile {
  id: string
  name: string
  type: string
  size: number
  width: number
  height: number
  url: string
  path: string
  folder: string
  tags?: string[]
  uploadDate?: number
  usedCount?: number
  commandType?: 'includegraphics' | 'includegraphics-width' | 'figure'
  isFavorite?: boolean
  description?: string
  category?: string
  lastModified?: number
  exif?: {
    camera?: string
    date?: string
    location?: string
  }
}

interface FolderNode {
  id: string
  name: string
  type: 'folder' | 'root'
  path: string
  count: number
  children?: FolderNode[]
}

interface Props {
  documentId?: string
}

const props = defineProps<Props>()

const emit = defineEmits<{
  (e: 'insert-reference', command: string): void
}>()

// 状态
const searchQuery = ref('')
const filterType = ref('')
const sortBy = ref('date')
const tagFilter = ref('')
const selectedImages = ref<Set<string>>(new Set())
const currentFolder = ref<string>('images')
const isDragging = ref(false)
const showPreviewDialog = ref(false)
const showEditDialog = ref(false)
const showBatchEditDialog = ref(false)
const showTagDialog = ref(false)
const optimizing = ref(false)
const sidebarCollapsed = ref(false)
const viewMode = ref<'grid' | 'list'>('grid')
const referenceType = ref('basic')
const previewZoom = ref(1)
const previewRotation = ref(0)

// 批量编辑数据
const batchEditData = ref({
  tags: [] as string[],
  category: '',
  addTags: [] as string[],
  removeTags: [] as string[]
})

// 所有图片标签
const allTags = computed(() => {
  const tags = new Set<string>()
  images.value.forEach(img => {
    img.tags?.forEach(tag => tags.add(tag))
  })
  return Array.from(tags).sort()
})

const previewImage = ref<ImageFile | null>(null)
const currentEditImage = ref<ImageFile>({
  id: '',
  name: '',
  type: '',
  size: 0,
  width: 0,
  height: 0,
  url: '',
  path: '',
  folder: '',
  commandType: 'includegraphics'
})

const fileInputRef = ref<HTMLInputElement>()
const imageFormRef = ref()

// 存储配置
const maxStorage = 100 * 1024 * 1024 // 100MB

// 模拟数据
const images = ref<ImageFile[]>([])
const folderTree = ref<FolderNode[]>([])

const treeProps = {
  children: 'children',
  label: 'name'
}

// 计算属性
const totalImages = computed(() => images.value.length)
const totalSize = computed(() => images.value.reduce((sum, img) => sum + img.size, 0))
const storagePercent = computed(() => Math.min((totalSize.value / maxStorage) * 100, 100))
const selectedCount = computed(() => selectedImages.value.size)

const folderTreeForSelect = computed(() => {
  return [
    { id: 'images', name: '图片', type: 'root' as const, path: 'images', count: 0 },
    ...(folderTree.value[0]?.children || [])
  ]
})

// 面包屑
const breadcrumb = computed(() => {
  const parts = currentFolder.value.split('/')
  const result = []
  let path = ''

  result.push({ id: 'images', name: '图片' })

  for (let i = 0; i < parts.length; i++) {
    if (parts[i]) {
      path += (i > 0 ? '/' : '') + parts[i]
      result.push({ id: path, name: parts[i] })
    }
  }

  return result
})

// 显示的图片
const displayedImages = computed(() => {
  let filtered = images.value.filter(img => img.folder === currentFolder.value)

  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    filtered = filtered.filter(img =>
      img.name.toLowerCase().includes(query)
    )
  }

  if (filterType.value) {
    filtered = filtered.filter(img => img.type === filterType.value)
  }

  if (tagFilter.value) {
    filtered = filtered.filter(img => img.tags?.includes(tagFilter.value))
  }

  // 排序
  filtered.sort((a, b) => {
    switch (sortBy.value) {
      case 'name':
        return a.name.localeCompare(b.name)
      case 'size':
        return a.size - b.size
      case 'date':
      default:
        return (b.uploadDate || 0) - (a.uploadDate || 0)
    }
  })

  return filtered
})

// 格式化文件大小
const formatSize = (bytes: number): string => {
  if (bytes === 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB', 'GB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return Math.round(bytes / Math.pow(k, i) * 100) / 100 + ' ' + sizes[i]
}

// 生成LaTeX引用命令
const generateImageCommand = (image: ImageFile | null, type: string = referenceType.value): string => {
  if (!image) return ''

  const path = image.path

  switch (type) {
    case 'basic':
      return `\\includegraphics{${path}}`
    case 'width':
      return `\\includegraphics[width=0.8\\textwidth]{${path}}`
    case 'figure':
      return `\\begin{figure}[htbp]\n  \\centering\n  \\includegraphics[width=0.8\\textwidth]{${path}}\n  \\caption{${image.name.split('.')[0]}}\n  \\label{fig:${image.name.split('.')[0]}}\n\\end{figure}`
    default:
      return `\\includegraphics{${path}}`
  }
}

// 触发上传
const triggerUpload = () => {
  fileInputRef.value?.click()
}

// 处理文件选择
const handleFileSelect = async (event: Event) => {
  const target = event.target as HTMLInputElement
  const files = target.files

  if (!files || files.length === 0) return

  await uploadFiles(Array.from(files))

  target.value = ''
}

// 上传文件
const uploadFiles = async (files: File[]) => {
  for (const file of files) {
    const info = await readImageInfo(file)

    const newImage: ImageFile = {
      id: `img-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`,
      name: file.name,
      type: file.type.split('/')[1] || 'png',
      size: file.size,
      width: info.width,
      height: info.height,
      url: info.url,
      path: `${currentFolder.value}/${file.name}`,
      folder: currentFolder.value,
      commandType: 'includegraphics'
    }

    images.value.push(newImage)
  }

  updateFolderTree()
  ElMessage.success(`成功上传 ${files.length} 张图片`)
}

// 读取图片信息
const readImageInfo = (file: File): Promise<{
  width: number
  height: number
  url: string
}> => {
  return new Promise((resolve) => {
    const img = new Image()
    const url = URL.createObjectURL(file)

    img.onload = () => {
      resolve({
        width: img.naturalWidth,
        height: img.naturalHeight,
        url
      })
    }

    img.src = url
  })
}

// 处理拖拽
const handleDragOver = () => {
  isDragging.value = true
}

const handleDragLeave = () => {
  isDragging.value = false
}

const handleDrop = async (event: DragEvent) => {
  isDragging.value = false

  const files = event.dataTransfer?.files
  if (!files || files.length === 0) return

  const imageFiles = Array.from(files).filter(f => f.type.startsWith('image/') || f.type === 'application/pdf')

  if (imageFiles.length > 0) {
    await uploadFiles(imageFiles)
  }
}

// 选择图片
const selectImage = (image: ImageFile) => {
  // 单击由复选框处理
}

const toggleSelectImage = (image: ImageFile, checked: boolean) => {
  if (checked) {
    selectedImages.value.add(image.id)
  } else {
    selectedImages.value.delete(image.id)
  }
}

// 预览图片
const openPreview = (image: ImageFile) => {
  previewImage.value = image
  resetPreviewTransform()
  showPreviewDialog.value = true
}

// 重置预览变换
const resetPreviewTransform = () => {
  previewZoom.value = 1
  previewRotation.value = 0
}

// 编辑图片
const editImage = (image: ImageFile) => {
  currentEditImage.value = { ...image }
  showEditDialog.value = true
}

// 保存图片编辑
const saveImageEdit = () => {
  const index = images.value.findIndex(img => img.id === currentEditImage.value.id)
  if (index >= 0) {
    images.value[index] = { ...currentEditImage.value }
  }
  showEditDialog.value = false
  ElMessage.success('保存成功')
}

// 删除图片
const deleteImage = async (image: ImageFile) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除图片 "${image.name}" 吗？`,
      '确认删除',
      {
        type: 'warning',
        confirmButtonText: '删除',
        cancelButtonText: '取消'
      }
    )

    images.value = images.value.filter(img => img.id !== image.id)
    selectedImages.value.delete(image.id)
    updateFolderTree()
    ElMessage.success('删除成功')
  } catch {
    // 用户取消
  }
}

// 插入引用
const insertReference = (image: ImageFile) => {
  const command = generateImageCommand(image)
  emit('insert-reference', command)
  ElMessage.success('已插入引用命令')
}

// 复制引用
const copyReference = (type?: string) => {
  const refType = type || referenceType.value
  const command = generateImageCommand(previewImage.value, refType)
  navigator.clipboard.writeText(command)
  ElMessage.success('已复制到剪贴板')
}

// 下载图片
const downloadImage = () => {
  if (!previewImage.value) return

  const link = document.createElement('a')
  link.href = previewImage.value.url
  link.download = previewImage.value.name
  link.click()
}

// 图片优化
const optimizeImage = async () => {
  optimizing.value = true

  // 模拟图片压缩
  await new Promise(resolve => setTimeout(resolve, 2000))

  optimizing.value = false
  ElMessage.success('图片已优化')
}

// 格式转换
const convertImage = () => {
  ElMessage.info('格式转换功能开发中...')
}

// 批量操作
const handleBatchAction = async (action: string) => {
  if (selectedImages.value.size === 0) {
    ElMessage.warning('请先选择图片')
    return
  }

  switch (action) {
    case 'insert':
      ElMessage.success(`已插入 ${selectedImages.value.size} 张图片引用`)
      selectedImages.value.clear()
      break
    case 'edit':
      openBatchEdit()
      break
    case 'tag':
      openBatchEdit()
      break
    case 'optimize':
      await optimizeBatch()
      break
    case 'move':
      ElMessage.info('批量移动功能开发中...')
      break
    case 'delete':
      await deleteBatch()
      break
  }
}

const optimizeBatch = async () => {
  try {
    await ElMessageBox.confirm(
      `确定压缩选中的 ${selectedImages.value.size} 张图片吗？`,
      '确认压缩',
      { type: 'warning' }
    )

    optimizing.value = true
    await new Promise(resolve => setTimeout(resolve, 2000))
    optimizing.value = false

    ElMessage.success('压缩完成')
  } catch {
    // 用户取消
  }
}

const deleteBatch = async () => {
  try {
    await ElMessageBox.confirm(
      `确定删除选中的 ${selectedImages.value.size} 张图片吗？`,
      '确认删除',
      { type: 'warning' }
    )

    images.value = images.value.filter(img => !selectedImages.value.has(img.id))
    selectedImages.value.clear()

    updateFolderTree()
    ElMessage.success('删除成功')
  } catch {
    // 用户取消
  }
}

// 批量编辑
const openBatchEdit = () => {
  batchEditData.value = {
    tags: [],
    category: '',
    addTags: [],
    removeTags: []
  }
  showBatchEditDialog.value = true
}

const saveBatchEdit = () => {
  const selectedImagesList = images.value.filter(img => selectedImages.value.has(img.id))

  selectedImagesList.forEach(img => {
    // 添加标签
    if (batchEditData.value.addTags.length > 0) {
      img.tags = [...(img.tags || []), ...batchEditData.value.addTags]
      // 去重
      img.tags = [...new Set(img.tags)]
    }

    // 移除标签
    if (batchEditData.value.removeTags.length > 0) {
      img.tags = (img.tags || []).filter(t => !batchEditData.value.removeTags.includes(t))
    }

    // 设置分类
    if (batchEditData.value.category) {
      img.category = batchEditData.value.category
    }
  })

  ElMessage.success(`已更新 ${selectedImagesList.length} 张图片`)
  showBatchEditDialog.value = false
  selectedImages.value.clear()
}

// 切换收藏
const toggleFavorite = (image: ImageFile, event: Event) => {
  event.stopPropagation()
  image.isFavorite = !image.isFavorite
  ElMessage.success(image.isFavorite ? '已收藏' : '已取消收藏')
}

// 创建文件夹
const createFolder = () => {
  ElMessage.prompt('请输入文件夹名称', '新建文件夹', {
    confirmButtonText: '创建',
    cancelButtonText: '取消'
  }).then(({ value }) => {
    if (value) {
      ElMessage.success(`文件夹 "${value}" 已创建`)
      // TODO: 实际创建文件夹逻辑
    }
  }).catch(() => {})
}

// 刷新文件夹
const refreshFolders = () => {
  updateFolderTree()
  ElMessage.success('已刷新')
}

// 更新文件夹树
const updateFolderTree = () => {
  const folders = new Set<string>()
  images.value.forEach(img => {
    folders.add(img.folder)
  })

  folderTree.value = [
    {
      id: 'images',
      name: '图片',
      type: 'root',
      path: 'images',
      count: images.value.filter(i => i.folder === 'images').length,
      children: Array.from(folders).filter(f => f !== 'images').map(folder => ({
        id: folder,
        name: folder.split('/').pop() || folder,
        type: 'folder',
        path: folder,
        count: images.value.filter(i => i.folder === folder).length
      }))
    }
  ]
}

// 导航到文件夹
const navigateToFolder = (item: { id: string; name: string }) => {
  currentFolder.value = item.id
}

// 文件夹点击
const handleFolderClick = (data: FolderNode) => {
  currentFolder.value = data.path
}

// 图片加载错误处理
const handleImageError = (event: Event) => {
  const img = event.target as HTMLImageElement
  img.src = 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cGF0aCBkPSJNMCAwaDE2djE2SDBWMHptMiAyaDEydjEySDJWMnptMiAyaDh2OEg0VjR6IiBmaWxsPSIjRTVFOUY5Ii8+PC9zdmc+'
}

// 初始化
onMounted(() => {
  updateFolderTree()

  // 添加一些示例图片
  const sampleImages: ImageFile[] = [
    {
      id: 'img-1',
      name: 'figure1.png',
      type: 'png',
      size: 128000,
      width: 800,
      height: 600,
      url: 'https://via.placeholder.com/800x600/409EFF/ffffff?text=Figure+1',
      path: 'images/figure1.png',
      folder: 'images',
      commandType: 'includegraphics'
    },
    {
      id: 'img-2',
      name: 'diagram.svg',
      type: 'svg',
      size: 45000,
      width: 600,
      height: 400,
      url: 'https://via.placeholder.com/600x400/67C23A/ffffff?text=Diagram',
      path: 'images/diagram.svg',
      folder: 'images',
      commandType: 'includegraphics'
    },
    {
      id: 'img-3',
      name: 'chart.jpg',
      type: 'jpg',
      size: 256000,
      width: 1000,
      height: 500,
      url: 'https://via.placeholder.com/1000x500/E6A23C/ffffff?text=Chart',
      path: 'images/charts/chart.jpg',
      folder: 'images/charts',
      commandType: 'includegraphics'
    }
  ]

  images.value = sampleImages
  updateFolderTree()
})

defineExpose({
  uploadImages: uploadFiles
})
</script>

<style scoped lang="scss">
.image-resource-manager {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: var(--el-bg-color-page);
}

// 头部
.manager-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 24px 8px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color-light);

  .header-left {
    .header-title {
      display: flex;
      align-items: center;
      gap: 10px;
      font-size: 16px;
      font-weight: 600;
      color: var(--el-text-color-primary);
    }
  }

  .header-actions {
    display: flex;
    align-items: center;
    gap: 14px;

    .main-search {
      width: 320px;
    }
  }
}

// 主内容
.manager-content {
  flex: 1;
  display: flex;
  overflow: hidden;
}

// 侧边栏
.manager-sidebar {
  width: 240px;
  background: var(--el-bg-color);
  border-right: 1px solid var(--el-border-color-light);
  display: flex;
  flex-direction: column;
  transition: width 0.3s;

  &.is-collapsed {
    width: 50px;
  }

  .sidebar-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 14px;
    border-bottom: 1px solid var(--el-border-color-lighter);

    .sidebar-title {
      font-size: 13px;
      font-weight: 500;
    }
  }

  .sidebar-tree {
    flex: 1;
    overflow-y: auto;
    padding: 10px;

    :deep(.el-tree) {
      background: transparent;
    }

    :deep(.el-tree-node__content) {
      border-radius: 9px;
      padding: 7px 10px;
      transition: all 0.2s;

      &:hover {
        background: var(--el-fill-color-light);
        transform: translateX(2px);
      }
    }

    :deep(.is-current > .el-tree-node__content) {
      background: linear-gradient(90deg, var(--el-color-primary-light-9) 0%, transparent 100%);
      color: var(--el-color-primary);
      font-weight: 500;
    }

    .tree-node {
      display: flex;
      align-items: center;
      gap: 10px;
      width: 100%;

      &.is-collapsed {
        justify-content: center;
      }

      .node-icon {
        flex-shrink: 0;
        color: var(--el-color-primary);
      }

      .node-label {
        flex: 1;
        font-size: 13px;
        font-weight: 400;
      }
    }
  }

  .sidebar-footer {
    padding: 14px;
    border-top: 1px solid var(--el-border-color-light);
    background: linear-gradient(180deg, transparent 0%, var(--el-fill-color-light) 100%);

    .storage-info {
      .storage-bar {
        height: 6px;
        background: var(--el-fill-color);
        border-radius: 3px;
        overflow: hidden;
        margin-bottom: 8px;

        .storage-used {
          height: 100%;
          background: linear-gradient(90deg, var(--el-color-primary), var(--el-color-success));
          transition: width 0.5s cubic-bezier(0.4, 0, 0.2, 1);
          box-shadow: 0 0 10px rgba(103, 194, 58, 0.3);
        }
      }

      .storage-text {
        font-size: 12px;
        color: var(--el-text-color-secondary);
        text-align: center;
        font-weight: 500;
      }
    }
  }
}

// 主区域
.manager-main {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.breadcrumb-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 24px 10px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color-light);

  :deep(.el-breadcrumb) {
    font-size: 13px;
    font-weight: 500;
  }

  :deep(.el-breadcrumb__item) {
    cursor: pointer;
    transition: all 0.2s;

    &:hover {
      color: var(--el-color-primary);
    }
  }
}

.image-container {
  flex: 1;
  padding: 20px;
  overflow-y: auto;
  background: linear-gradient(180deg, var(--el-fill-color-extra-light) 0%, var(--el-fill-color-lighter) 100%);

  &.is-dragging {
    background: linear-gradient(135deg, var(--el-color-primary-light-9) 0%, var(--el-color-primary-light-8) 100%);
  }

  &.is-list-view {
    display: block;

    .image-card {
      display: grid;
      grid-template-columns: 60px 1fr auto;
      gap: 12px;
      padding: 12px 20px;
      align-items: center;
      height: auto;
      margin-bottom: 8px;

      .card-preview {
        width: 60px;
        height: 60px;
      }

      .card-checkbox {
        position: static;
      }
    }
  }
}

// 上传区
.upload-zone {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 80px 40px;
  border: 3px dashed var(--el-border-color);
  border-radius: 20px;
  background: var(--el-bg-color);
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  position: relative;
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    inset: 0;
    background: linear-gradient(135deg, var(--el-color-primary-light-9) 0%, var(--el-color-primary-light-8) 100%);
    opacity: 0;
    transition: opacity 0.3s;
  }

  &.is-active {
    border-color: var(--el-color-primary);
    transform: scale(1.02);

    &::before {
      opacity: 1;
    }
  }

  > * {
    position: relative;
    z-index: 1;
  }

  .upload-illustration {
    margin-bottom: 24px;
    animation: pulse 2s ease-in-out infinite;
  }

  @keyframes pulse {
    0%, 100% {
      opacity: 0.6;
      transform: scale(1);
    }
    50% {
      opacity: 1;
      transform: scale(1.05);
    }
  }

  .upload-text {
    font-size: 18px;
    font-weight: 600;
    color: var(--el-text-color-primary);
    margin-bottom: 8px;
  }

  .upload-hint {
    font-size: 14px;
    color: var(--el-text-color-secondary);
    margin-bottom: 28px;
  }
}

// 图片网格
.image-container:not(.is-list-view) {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(230px, 1fr));
  gap: 20px;
  padding: 20px;
}

.image-card {
  position: relative;
  background: var(--el-bg-color);
  border-radius: 14px;
  border: 1px solid var(--el-border-color-lighter);
  overflow: hidden;
  cursor: pointer;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.04);
  transition: all 0.28s cubic-bezier(0.4, 0, 0.2, 1);

  &:hover {
    border-color: var(--el-color-primary-light-6);
    box-shadow:
      0 6px 20px rgba(0, 0, 0, 0.08),
      0 3px 10px rgba(64, 158, 255, 0.1);
    transform: translateY(-3px);
  }

  &.is-selected {
    border-color: var(--el-color-primary);
    background: linear-gradient(135deg, var(--el-color-primary-light-9) 0%, var(--el-bg-color) 100%);
    box-shadow:
      0 0 0 3px rgba(64, 158, 255, 0.1),
      0 8px 20px rgba(64, 158, 255, 0.2);
  }

  &.is-favorite {
    border-color: var(--el-color-warning);
    background: linear-gradient(135deg, rgba(230, 162, 60, 0.03) 0%, var(--el-bg-color) 100%);
  }

  // 收藏按钮
  .card-favorite {
    position: absolute;
    top: 10px;
    right: 10px;
    z-index: 3;
    width: 32px;
    height: 32px;
    background: rgba(255, 255, 255, 0.98);
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
    box-shadow: 0 2px 6px rgba(0, 0, 0, 0.10);
    backdrop-filter: blur(10px);

    &:hover {
      background: white;
      transform: scale(1.12);
      box-shadow: 0 4px 10px rgba(230, 162, 60, 0.25);
    }

    &:active {
      transform: scale(1.05);
    }

    .el-icon {
      font-size: 18px;
      color: var(--el-text-color-secondary);
      transition: all 0.2s;

      &.is-active {
        color: var(--el-color-warning);
        transform: scale(1.1);
      }
    }
  }

  .card-checkbox {
    position: absolute;
    top: 10px;
    left: 10px;
    z-index: 2;
    background: rgba(255, 255, 255, 0.98);
    border-radius: 50%;
    padding: 4px;
    box-shadow: 0 2px 6px rgba(0, 0, 0, 0.10);
    backdrop-filter: blur(10px);
    transition: all 0.2s;

    &:hover {
      background: white;
      transform: scale(1.1);
    }
  }

  .card-preview {
    position: relative;
    width: 100%;
    aspect-ratio: 4 / 3;
    background: linear-gradient(135deg, var(--el-fill-color-light) 0%, var(--el-fill-color) 100%);
    overflow: hidden;

    img {
      width: 100%;
      height: 100%;
      object-fit: cover;
      transition: transform 0.5s cubic-bezier(0.4, 0, 0.2, 1);
      filter: brightness(0.98);
    }

    &:hover img {
      transform: scale(1.1);
      filter: brightness(1.05);
    }

    .preview-overlay {
      position: absolute;
      inset: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 10px;
      background: rgba(0, 0, 0, 0.75);
      opacity: 0;
      transition: opacity 0.28s cubic-bezier(0.4, 0, 0.2, 1);
      backdrop-filter: blur(10px);

      .overlay-actions {
        display: flex;
        gap: 10px;
        flex-wrap: wrap;
        justify-content: center;
      }

      .el-button {
        background: rgba(255, 255, 255, 0.98);
        border: none;
        box-shadow: 0 4px 14px rgba(0, 0, 0, 0.25);
        transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
        backdrop-filter: blur(10px);

        &:hover {
          background: white;
          transform: scale(1.12) translateY(-2px);
          box-shadow: 0 6px 16px rgba(0, 0, 0, 0.30);
        }

        &:active {
          transform: scale(1.08) translateY(-1px);
        }
      }
    }

    &:hover .preview-overlay {
      opacity: 1;
    }

    // 使用次数标记
    .usage-badge {
      position: absolute;
      top: 10px;
      right: 50px;
      display: inline-flex;
      align-items: center;
      gap: 4px;
      padding: 4px 10px;
      background: linear-gradient(135deg, var(--el-color-success) 0%, var(--el-color-success-light-3) 100%);
      color: white;
      font-size: 11px;
      font-weight: 700;
      border-radius: 14px;
      z-index: 2;
      box-shadow: 0 3px 8px rgba(103, 194, 58, 0.30);
      letter-spacing: 0.3px;
    }

    .type-badge {
      position: absolute;
      bottom: 10px;
      right: 10px;
      padding: 4px 10px;
      background: rgba(0, 0, 0, 0.85);
      color: white;
      font-size: 10px;
      font-weight: 700;
      border-radius: 8px;
      letter-spacing: 0.8px;
      backdrop-filter: blur(6px);
      box-shadow: 0 2px 6px rgba(0, 0, 0, 0.18);
    }
  }

  .card-info {
    padding: 14px 16px;
    background: linear-gradient(180deg, transparent 0%, var(--el-fill-color-extra-light) 100%);

    .image-name {
      font-size: 14px;
      font-weight: 700;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
      margin-bottom: 8px;
      color: var(--el-text-color-primary);
      letter-spacing: 0.3px;
      line-height: 1.4;
    }

    .image-description {
      font-size: 12px;
      color: var(--el-text-color-secondary);
      margin-bottom: 10px;
      line-height: 1.5;
      display: -webkit-box;
      -webkit-line-clamp: 2;
      -webkit-box-orient: vertical;
      overflow: hidden;
      min-height: 36px;
    }

    .image-meta {
      display: flex;
      gap: 12px;
      flex-wrap: wrap;
      font-size: 11px;
      color: var(--el-text-color-secondary);
      margin-bottom: 10px;
      padding: 7px 10px;
      background: var(--el-fill-color);
      border-radius: 9px;

      .meta-item {
        display: inline-flex;
        align-items: center;
        gap: 5px;
        font-weight: 500;

        .el-icon {
          font-size: 12px;
        }

        &.usage {
          color: var(--el-color-success);
          font-weight: 700;
          padding: 2px 8px;
          background: var(--el-color-success-light-9);
          border-radius: 12px;
        }
      }
    }

    .image-tags {
      display: flex;
      flex-wrap: wrap;
      gap: 6px;
      margin-top: 8px;

      .image-tag {
        font-size: 11px;
        height: 20px;
        line-height: 20px;
        padding: 0 10px;
        border-radius: 10px;
        font-weight: 500;
        letter-spacing: 0.3px;
      }

      .more-tags {
        font-size: 11px;
        color: var(--el-text-color-placeholder);
        padding: 0 6px;
        font-weight: 500;
      }
    }
  }
}

// 空状态
.empty-state {
  grid-column: 1 / -1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 120px 40px;
  min-height: 500px;

  .empty-illustration {
    margin-bottom: 32px;
    opacity: 0.5;
    animation: float 3s ease-in-out infinite;
  }

  @keyframes float {
    0%, 100% {
      transform: translateY(0px);
    }
    50% {
      transform: translateY(-12px);
    }
  }

  .empty-title {
    font-size: 20px;
    font-weight: 700;
    color: var(--el-text-color-primary);
    margin-bottom: 10px;
    letter-spacing: 0.3px;
  }

  .empty-desc {
    font-size: 15px;
    color: var(--el-text-color-secondary);
    margin-bottom: 24px;
    line-height: 1.6;
  }
}

// 预览对话框
.preview-dialog {
  :deep(.el-dialog__header) {
    display: none;
  }

  :deep(.el-dialog__body) {
    padding: 0;
    height: 75vh;
  }
}

.preview-content {
  display: flex;
  height: 100%;
}

.preview-image {
  flex: 1;
  display: flex;
  flex-direction: column;
  background: var(--el-fill-color-darker);

  .preview-controls {
    display: flex;
    justify-content: center;
    align-items: center;
    gap: 12px;
    padding: 16px;
    background: rgba(0, 0, 0, 0.3);
    border-bottom: 1px solid var(--el-border-color-lighter);
  }

  .preview-image-container {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 20px;
    overflow: hidden;

    img {
      max-width: 100%;
      max-height: 100%;
      object-fit: contain;
      border-radius: 8px;
      transition: transform 0.3s ease;
    }
  }
}

.preview-sidebar {
  width: 320px;
  background: var(--el-bg-color);
  border-left: 1px solid var(--el-border-color-lighter);
  display: flex;
  flex-direction: column;
  overflow: hidden;

  .preview-header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    padding: 16px;
    border-bottom: 1px solid var(--el-border-color-lighter);

    .preview-title {
      font-size: 14px;
      font-weight: 500;
      font-family: monospace;
      word-break: break-all;
      line-height: 1.4;
    }
  }

  .preview-details {
    flex: 0 0 auto;

    code {
      font-family: monospace;
      font-size: 12px;
    }
  }

  .reference-section {
    padding: 16px;
    flex: 1;
    overflow-y: auto;

    .section-title {
      display: flex;
      align-items: center;
      gap: 6px;
      font-size: 13px;
      font-weight: 500;
      margin-bottom: 12px;
    }

    .code-input {
      :deep(.el-textarea__inner) {
        font-family: 'Consolas', 'Monaco', monospace;
        font-size: 12px;
        line-height: 1.5;
      }
    }
  }

  .preview-actions {
    padding: 12px 16px;
    border-top: 1px solid var(--el-border-color-lighter);
  }
}

// 编辑对话框
.edit-dialog {
  .edit-operations {
    display: flex;
    gap: 10px;
  }
}

// 批量编辑对话框
.batch-edit-dialog {
  :deep(.el-form-item__label) {
    font-weight: 500;
  }

  .el-select {
    :deep(.el-tag) {
      max-width: 150px;
    }
  }
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}

// 响应式
@media (max-width: 768px) {
  .manager-header {
    flex-direction: column;
    align-items: stretch;
    gap: 12px;
  }

  .header-actions {
    justify-content: space-between;

    .search-filter {
      flex: 1;

      .search-input {
        width: 100%;
      }
    }
  }

  .manager-sidebar {
    position: absolute;
    left: 0;
    top: 0;
    bottom: 0;
    z-index: 100;
    box-shadow: 2px 0 8px rgba(0, 0, 0, 0.1);

    &.is-collapsed {
      width: 0;
      border: none;
    }
  }

  .preview-content {
    flex-direction: column;
  }

  .preview-sidebar {
    width: 100%;
    border-left: none;
    border-top: 1px solid var(--el-border-color-lighter);
    max-height: 40vh;
  }
}
</style>
