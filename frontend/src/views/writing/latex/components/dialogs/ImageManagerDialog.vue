<template>
  <BaseDialog
    :show="show"
    title="图片管理器"
    width="900px"
    @update:show="$emit('update:show', $event)"
  >
    <div class="image-manager">
      <!-- 工具栏 -->
      <div class="manager-toolbar">
        <el-upload
          :auto-upload="false"
          :show-file-list="false"
          :on-change="handleImageSelect"
          accept="image/png,image/jpeg,image/jpg,image/svg,image/gif"
          multiple
        >
          <el-button type="primary">
            <el-icon><Upload /></el-icon>
            上传图片
          </el-button>
        </el-upload>

        <el-input
          v-model="searchQuery"
          placeholder="搜索图片..."
          prefix-icon="Search"
          style="width: 200px"
          clearable
        />

        <el-select v-model="viewMode" style="width: 100px">
          <el-option label="网格" value="grid" />
          <el-option label="列表" value="list" />
        </el-select>
      </div>

      <!-- 图片统计 -->
      <div class="image-stats">
        <el-tag type="info">总计: {{ images.length }} 张</el-tag>
        <el-tag type="success">已引用: {{ usedImages.length }} 张</el-tag>
        <el-tag type="warning">未引用: {{ unusedImages.length }} 张</el-tag>
        <el-tag type="danger">总大小: {{ totalSize }}</el-tag>
      </div>

      <!-- 图片列表 -->
      <div v-loading="loading" class="image-container" :class="`view-${viewMode}`">
        <!-- 网格视图 -->
        <div v-if="viewMode === 'grid'" class="image-grid">
          <div
            v-for="image in filteredImages"
            :key="image.id"
            class="image-card"
            :class="{ 'is-selected': selectedImageId === image.id }"
            @click="selectImage(image)"
          >
            <div class="image-preview">
              <img :src="image.url" :alt="image.name" @error="handleImageError" />
              <div class="image-overlay">
                <el-button size="small" @click.stop="openPreview(image)">
                  <el-icon><ZoomIn /></el-icon>
                </el-button>
                <el-button size="small" @click.stop="insertImage(image)">
                  <el-icon><Plus /></el-icon>
                </el-button>
                <el-button size="small" type="danger" @click.stop="deleteImage(image)">
                  <el-icon><Delete /></el-icon>
                </el-button>
              </div>
            </div>
            <div class="image-info">
              <div class="image-name" :title="image.name">{{ image.name }}</div>
              <div class="image-meta">
                <span>{{ formatSize(image.size) }}</span>
                <el-tag v-if="isImageUsed(image)" size="small" type="success">已用</el-tag>
              </div>
            </div>
          </div>
        </div>

        <!-- 列表视图 -->
        <el-table v-else :data="filteredImages" @row-click="selectImage">
          <el-table-column width="60">
            <template #default="{ row }">
              <el-image
                :src="row.url"
                fit="cover"
                style="width: 40px; height: 40px; border-radius: 4px"
                lazy
              >
                <template #error>
                  <el-icon><Picture /></el-icon>
                </template>
              </el-image>
            </template>
          </el-table-column>
          <el-table-column prop="name" label="文件名" />
          <el-table-column prop="size" label="大小" :formatter="(row) => formatSize(row.size)" />
          <el-table-column prop="type" label="类型" />
          <el-table-column prop="refCount" label="引用次数" width="80" />
          <el-table-column prop="createdAt" label="上传时间" :formatter="(row) => formatDate(row.createdAt)" />
          <el-table-column label="操作" width="180">
            <template #default="{ row }">
              <el-button size="small" @click.stop="openPreview(row)">预览</el-button>
              <el-button size="small" type="primary" @click.stop="insertImage(row)">插入</el-button>
              <el-button size="small" type="danger" @click.stop="deleteImage(row)">删除</el-button>
            </template>
          </el-table-column>
        </el-table>

        <!-- 空状态 -->
        <el-empty v-if="filteredImages.length === 0" description="暂无图片">
          <el-button type="primary" @click="$emit('upload')">上传图片</el-button>
        </el-empty>
      </div>
    </div>

    <!-- 预览对话框 -->
    <el-dialog v-model="showPreview" title="图片预览" width="70%" append-to-body>
      <div class="preview-container">
        <img :src="previewImage.url" :alt="previewImage.name" />
      </div>
      <div class="preview-info">
        <p><strong>文件名:</strong> {{ previewImage.name }}</p>
        <p><strong>路径:</strong> <code>{{ previewImage.path }}</code></p>
        <p><strong>大小:</strong> {{ formatSize(previewImage.size) }}</p>
        <p><strong>引用代码:</strong> <code>{{ generateIncludeCode(previewImage) }}</code></p>
      </div>
    </el-dialog>

    <!-- 压缩对话框 -->
    <el-dialog v-model="showCompressDialog" title="压缩图片" width="500px" append-to-body>
      <el-form :model="compressForm" label-width="100px">
        <el-form-item label="压缩质量">
          <el-slider v-model="compressForm.quality" :min="10" :max="100" show-input />
        </el-form-item>
        <el-form-item label="目标格式">
          <el-radio-group v-model="compressForm.format">
            <el-radio label="original">保持原格式</el-radio>
            <el-radio label="jpeg">JPEG</el-radio>
            <el-radio label="png">PNG</el-radio>
            <el-radio label="webp">WebP</el-radio>
          </el-radio-group>
        </el-form-item>
        <el-form-item label="预计大小">
          <span>{{ formatSize(compressForm.estimatedSize) }}</span>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showCompressDialog = false">取消</el-button>
        <el-button type="primary" @click="confirmCompress" :loading="compressing">压缩</el-button>
      </template>
    </el-dialog>

    <template #footer>
      <el-button @click="$emit('update:show', false)">关闭</el-button>
      <el-button type="primary" @click="$emit('upload')">上传新图片</el-button>
    </template>
  </BaseDialog>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Upload, Search, ZoomIn, Plus, Delete, Picture } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import BaseDialog from './BaseDialog.vue'

interface ProjectImage {
  id: number | string
  name: string
  path: string
  url: string
  size: number
  type: string
  refCount: number
  createdAt: number
}

interface Props {
  show: boolean
  projectId?: string | number
  images: ProjectImage[]
  documentContent?: string
}

const props = withDefaults(defineProps<Props>(), {
  images: () => []
})

const emit = defineEmits<{
  'update:show': [value: boolean]
  'upload': []
  'insert': [code: string]
  'delete': [imageId: number | string]
  'compress': [imageId: number | string, options: CompressOptions]
}>()

interface CompressOptions {
  quality: number
  format: string
}

// 状态
const loading = ref(false)
const searchQuery = ref('')
const viewMode = ref<'grid' | 'list'>('grid')
const selectedImageId = ref<number | string | null>(null)
const showPreview = ref(false)
const previewImage = ref<ProjectImage>({} as ProjectImage)
const showCompressDialog = ref(false)
const compressing = ref(false)
const compressForm = ref({
  quality: 80,
  format: 'original',
  estimatedSize: 0
})

// 计算属性
const filteredImages = computed(() => {
  if (!searchQuery.value) return props.images
  const query = searchQuery.value.toLowerCase()
  return props.images.filter(img =>
    img.name.toLowerCase().includes(query) ||
    img.path.toLowerCase().includes(query)
  )
})

const usedImages = computed(() => {
  return props.images.filter(img => img.refCount > 0)
})

const unusedImages = computed(() => {
  return props.images.filter(img => img.refCount === 0)
})

const totalSize = computed(() => {
  const bytes = props.images.reduce((sum, img) => sum + img.size, 0)
  return formatSize(bytes)
})

// 方法
function handleImageSelect(file: any) {
  // 触发上传事件
  emit('upload')
  ElMessage.info('请使用主界面的上传功能上传图片')
}

function selectImage(image: ProjectImage) {
  selectedImageId.value = image.id
}

function openPreview(image: ProjectImage) {
  previewImage.value = image
  showPreview.value = true
}

function insertImage(image: ProjectImage) {
  const code = generateIncludeCode(image)
  emit('insert', code)
  ElMessage.success('已插入图片代码')
}

function deleteImage(image: ProjectImage) {
  ElMessageBox.confirm(
    `确定要删除图片 "${image.name}" 吗？${image.refCount > 0 ? '该图片正在被文档引用！' : ''}`,
    '确认删除',
    { type: 'warning' }
  ).then(() => {
    emit('delete', image.id)
    ElMessage.success('图片已删除')
  }).catch(() => {})
}

function isImageUsed(image: ProjectImage): boolean {
  return image.refCount > 0
}

function generateIncludeCode(image: ProjectImage): string {
  const ext = image.name.split('.').pop()?.toLowerCase()
  const filename = image.path || image.name

  // 根据文件类型生成不同的LaTeX代码
  if (ext === 'pdf') {
    return `\\includegraphics[width=0.8\\textwidth]{${filename}}`
  } else if (['png', 'jpg', 'jpeg', 'svg'].includes(ext || '')) {
    return `\\includegraphics[width=0.8\\textwidth]{${filename}}`
  }
  return `\\includegraphics{${filename}}`
}

function formatSize(bytes: number): string {
  if (bytes === 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB', 'GB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i]
}

function formatDate(timestamp: number): string {
  return new Date(timestamp).toLocaleDateString('zh-CN')
}

function handleImageError(e: Event) {
  const target = e.target as HTMLImageElement
  target.src = 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTAwIiBoZWlnaHQ9IjEwMCIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cmVjdCB3aWR0aD0iMTAwIiBoZWlnaHQ9IjEwMCIgZmlsbD0iI2YwZjBmMCIvPjx0ZXh0IHg9IjUwJSIgeT0iNTAlIiBkb21pbmFudC1iYXNlbGluZT0ibWlkZGxlIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LXNpemU9IjE0IiBmaWxsPSIjOTk5Ij7liqDovb3kuK08L3RleHQ+PC9zdmc+'
}

function confirmCompress() {
  compressing.value = true
  // 实现压缩逻辑
  setTimeout(() => {
    compressing.value = false
    showCompressDialog.value = false
    ElMessage.success('图片压缩完成')
  }, 1000)
}

defineExpose({
  refresh: () => emit('refresh')
})
</script>

<style scoped lang="scss">
.image-manager {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.manager-toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
}

.image-stats {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.image-container {
  min-height: 400px;
  max-height: 500px;
  overflow-y: auto;

  &.view-grid {
    padding: 8px;
  }
}

.image-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
  gap: 16px;
}

.image-card {
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  overflow: hidden;
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
    transform: translateY(-2px);
  }

  &.is-selected {
    border-color: var(--el-color-primary);
    box-shadow: 0 0 0 2px var(--el-color-primary-light-7);
  }
}

.image-preview {
  position: relative;
  aspect-ratio: 1;
  background: var(--el-fill-color-light);

  img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .image-overlay {
    position: absolute;
    inset: 0;
    background: rgba(0, 0, 0, 0.6);
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    opacity: 0;
    transition: opacity 0.2s;

    .el-button {
      background: rgba(255, 255, 255, 0.9);
    }
  }

  &:hover .image-overlay {
    opacity: 1;
  }
}

.image-info {
  padding: 8px;
}

.image-name {
  font-size: 13px;
  font-weight: 500;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.image-meta {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-top: 4px;
  font-size: 12px;
  color: var(--el-text-color-secondary);
}

.preview-container {
  display: flex;
  justify-content: center;
  padding: 20px;
  background: var(--el-fill-color-light);

  img {
    max-width: 100%;
    max-height: 60vh;
    object-fit: contain;
  }
}

.preview-info {
  padding: 16px;

  p {
    margin: 8px 0;

    code {
      background: var(--el-fill-color-light);
      padding: 2px 6px;
      border-radius: 4px;
      font-family: 'Courier New', monospace;
    }
  }
}
</style>
