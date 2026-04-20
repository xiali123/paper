<template>
  <div class="document-outline">
    <div v-if="projectFiles.length === 0 && sections.length === 0" class="outline-empty">
      <el-empty description="暂无文档结构" :image-size="40" />
    </div>

    <!-- 项目模式：显示多文件大纲 -->
    <div v-else-if="isProjectMode" class="project-outline">
      <div class="project-files">
        <div
          v-for="file in projectFiles"
          :key="file.id"
          class="file-section"
          :class="{ 'is-current': file.id === currentFileId }"
        >
          <!-- 文件标题 -->
          <div class="file-header" @click="handleFileClick(file)">
            <el-icon class="file-icon">
              <Document v-if="file.path === mainFilePath" />
              <Folder v-else />
            </el-icon>
            <span class="file-name" :title="file.name">
              {{ file.name }}
              <el-tag v-if="file.path === mainFilePath" size="small" type="primary">主文件</el-tag>
            </span>
            <el-icon :class="['file-expand-icon', { 'is-expanded': expandedFiles.has(file.id) }]">
              <ArrowDown />
            </el-icon>
          </div>

          <!-- 文件的章节结构 -->
          <div v-if="expandedFiles.has(file.id)" class="file-sections">
            <div v-if="getFileSections(file).length === 0" class="file-empty">
              <span class="empty-text">无章节结构</span>
            </div>
            <div v-else>
              <div
                v-for="section in getFileSections(file)"
                :key="`${file.id}-${section.id}`"
                class="outline-item"
                :class="[
                  `level-${section.level}`,
                  { 'is-active': activeSection === `${file.id}-${section.id}` }
                ]"
                @click.stop="navigateToFileSection(file, section)"
              >
                <div class="outline-content">
                  <el-icon v-if="section.level === 1" class="section-icon">
                    <Tickets />
                  </el-icon>
                  <el-icon v-else class="section-icon">
                    <DocumentCopy />
                  </el-icon>
                  <span class="section-title" :title="section.title">
                    {{ section.title }}
                  </span>
                </div>
                <div class="outline-info">
                  <span class="section-line">{{ section.line }}</span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 单文件模式：显示当前文件大纲 -->
    <div v-else class="single-file-outline">
      <div class="outline-sections">
        <div
          v-for="section in sections"
          :key="section.id"
          class="outline-item"
          :class="[`level-${section.level}`, { 'is-active': activeSection === section.id }]"
          @click="navigateToSection(section)"
        >
          <div class="outline-content">
            <el-icon v-if="section.level === 1" class="section-icon">
              <Document />
            </el-icon>
            <el-icon v-else-if="section.level === 2" class="section-icon">
              <Folder />
            </el-icon>
            <el-icon v-else class="section-icon">
              <Tickets />
            </el-icon>
            <span class="section-title" :title="section.title">
              {{ section.title }}
            </span>
          </div>
          <div class="outline-info">
            <span class="section-line">第 {{ section.line }} 行</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onUnmounted } from 'vue'
import {
  Document,
  Folder,
  Tickets,
  DocumentCopy,
  ArrowDown
} from '@element-plus/icons-vue'

interface Section {
  id: string
  title: string
  level: number
  line: number
  type: 'section' | 'subsection' | 'subsubsection' | 'paragraph'
}

interface ProjectFile {
  id: number | string
  name: string
  path: string
  content?: string
}

interface Props {
  content: string
  isProjectMode?: boolean
  projectFiles?: ProjectFile[]
  currentFileId?: number | string
  mainFilePath?: string
}

interface Emits {
  navigate: [{ line: number; column: number }]
  fileSelect: [file: ProjectFile]
}

const props = withDefaults(defineProps<Props>(), {
  isProjectMode: false,
  projectFiles: () => [],
  mainFilePath: ''
})

const emit = defineEmits<Emits>()

const activeSection = ref<string | null>(null)
const expandedFiles = ref<Set<number | string>>(new Set())

// 文件章节缓存
const fileSectionsCache = ref<Map<number | string, Section[]>>(new Map())

// Debug logging
if (import.meta.env.DEV) {
  console.log('DocumentOutline component mounted', {
    isProjectMode: props.isProjectMode,
    contentLength: props.content?.length || 0,
    projectFilesCount: props.projectFiles?.length || 0
  })
}

// 解析文档结构
function parseSections(content: string): Section[] {
  if (!content) return []

  const lines = content.split('\n')
  const parsedSections: Section[] = []

  lines.forEach((line, index) => {
    const lineNumber = index + 1

    // 匹配章节命令
    const sectionMatch = line.match(/\\section\*?\{([^}]+)\}/)
    if (sectionMatch) {
      parsedSections.push({
        id: `section-${lineNumber}`,
        title: sectionMatch[1],
        level: 1,
        line: lineNumber,
        type: 'section'
      })
      return
    }

    const subsectionMatch = line.match(/\\subsection\*?\{([^}]+)\}/)
    if (subsectionMatch) {
      parsedSections.push({
        id: `subsection-${lineNumber}`,
        title: subsectionMatch[1],
        level: 2,
        line: lineNumber,
        type: 'subsection'
      })
      return
    }

    const subsubsectionMatch = line.match(/\\subsubsection\*?\{([^}]+)\}/)
    if (subsubsectionMatch) {
      parsedSections.push({
        id: `subsubsection-${lineNumber}`,
        title: subsubsectionMatch[1],
        level: 3,
        line: lineNumber,
        type: 'subsubsection'
      })
      return
    }

    const paragraphMatch = line.match(/\\paragraph\*?\{([^}]+)\}/)
    if (paragraphMatch) {
      parsedSections.push({
        id: `paragraph-${lineNumber}`,
        title: paragraphMatch[1],
        level: 4,
        line: lineNumber,
        type: 'paragraph'
      })
    }
  })

  return parsedSections
}

// 获取文件的章节结构
function getFileSections(file: ProjectFile): Section[] {
  if (!fileSectionsCache.value.has(file.id)) {
    const sections = parseSections(file.content || '')
    fileSectionsCache.value.set(file.id, sections)
  }
  return fileSectionsCache.value.get(file.id)!
}

// 单文件模式的章节
const sections = computed<Section[]>(() => {
  return parseSections(props.content)
})

// 处理文件点击
function handleFileClick(file: ProjectFile) {
  // 切换展开/折叠
  if (expandedFiles.value.has(file.id)) {
    expandedFiles.value.delete(file.id)
  } else {
    expandedFiles.value.add(file.id)
  }

  // 如果不是当前文件，切换到该文件
  if (file.id !== props.currentFileId) {
    emit('fileSelect', file)
  }
}

// 导航到文件的章节
function navigateToFileSection(file: ProjectFile, section: Section) {
  if (import.meta.env.DEV) {
    console.log('DocumentOutline: navigating to file section', { file, section })
  }

  activeSection.value = `${file.id}-${section.id}`

  // 如果不是当前文件，先切换文件
  if (file.id !== props.currentFileId) {
    emit('fileSelect', file)
    // 等待文件切换后再导航（实际滚动在父组件处理）
  } else {
    emit('navigate', { line: section.line, column: 1 })
  }
}

// 单文件模式导航
function navigateToSection(section: Section) {
  if (import.meta.env.DEV) {
    console.log('DocumentOutline: navigating to section', section)
  }
  activeSection.value = section.id
  emit('navigate', { line: section.line, column: 1 })
}

// 监听项目文件变化，更新缓存
watch(() => props.projectFiles, (newFiles) => {
  if (import.meta.env.DEV) {
    console.log('DocumentOutline: projectFiles changed', newFiles?.length || 0)
  }

  // 清理已删除文件的缓存
  const newFileIds = new Set(newFiles?.map(f => f.id) || [])
  for (const fileId of fileSectionsCache.value.keys()) {
    if (!newFileIds.has(fileId)) {
      fileSectionsCache.value.delete(fileId)
    }
  }

  // 更新当前文件的缓存
  if (props.currentFileId) {
    const currentFile = newFiles?.find(f => f.id === props.currentFileId)
    if (currentFile && currentFile.content) {
      fileSectionsCache.value.set(props.currentFileId, parseSections(currentFile.content))
    }
  }
}, { deep: true })

// 监听当前文件变化
watch(() => props.currentFileId, (newFileId) => {
  if (import.meta.env.DEV) {
    console.log('DocumentOutline: currentFileId changed', newFileId)
  }

  // 自动展开当前文件
  if (newFileId && !expandedFiles.value.has(newFileId)) {
    expandedFiles.value.add(newFileId)
  }
}, { immediate: true })

// 监听内容变化（单文件模式）
let outlineTimeout: number | null = null

watch(() => props.content, (newContent) => {
  if (!props.isProjectMode && import.meta.env.DEV) {
    console.log('DocumentOutline content changed, new length:', newContent?.length || 0)
  }

  // Debounced processing to prevent excessive recomputation
  if (outlineTimeout) {
    clearTimeout(outlineTimeout)
  }

  outlineTimeout = setTimeout(() => {
    outlineTimeout = null
  }, 300)
})

// 清理定时器防止内存泄漏
onUnmounted(() => {
  if (outlineTimeout) {
    clearTimeout(outlineTimeout)
  }
})

// 暴露方法供父组件调用
defineExpose({
  refreshCache: () => {
    fileSectionsCache.value.clear()
  }
})
</script>

<style scoped lang="scss">
.document-outline {
  height: 100%;
  overflow-y: auto;
  padding: 8px 0;
}

.outline-empty {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 100%;
  opacity: 0.6;
}

// 项目模式样式
.project-outline {
  .project-files {
    .file-section {
      margin-bottom: 8px;

      .file-header {
        display: flex;
        align-items: center;
        gap: 8px;
        padding: 10px 12px;
        cursor: pointer;
        border-radius: 6px;
        transition: all 0.2s ease;
        user-select: none;

        &:hover {
          background-color: var(--el-bg-color-overlay);
        }

        &.is-current {
          background-color: var(--el-color-primary-light-9);
          border-left: 3px solid var(--el-color-primary);
        }

        .file-icon {
          font-size: 18px;
          color: var(--el-color-primary);
          flex-shrink: 0;
        }

        .file-name {
          flex: 1;
          font-size: 13px;
          font-weight: 500;
          color: var(--el-text-color-primary);
          overflow: hidden;
          text-overflow: ellipsis;
          white-space: nowrap;
          display: flex;
          align-items: center;
          gap: 6px;

          .el-tag {
            flex-shrink: 0;
          }
        }

        .file-expand-icon {
          font-size: 14px;
          color: var(--el-text-color-secondary);
          flex-shrink: 0;
          transition: transform 0.2s ease;

          &.is-expanded {
            transform: rotate(180deg);
          }
        }
      }

      .file-sections {
        padding-left: 16px;

        .file-empty {
          padding: 8px 12px;
          color: var(--el-text-color-placeholder);
          font-size: 12px;

          .empty-text {
            font-style: italic;
          }
        }
      }
    }
  }
}

// 单文件模式样式
.single-file-outline {
  .outline-sections {
    .outline-item {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 8px 12px;
      cursor: pointer;
      border-radius: 4px;
      margin: 2px 0;
      transition: all 0.2s ease;

      &:hover {
        background-color: var(--el-bg-color-overlay);
      }

      &.is-active {
        background-color: var(--el-color-primary-light-9);
        border-left: 3px solid var(--el-color-primary);
      }

      &.level-1 {
        font-weight: 600;
        font-size: 14px;
        padding-left: 8px;
      }

      &.level-2 {
        font-weight: 500;
        font-size: 13px;
        padding-left: 20px;
      }

      &.level-3 {
        font-weight: 400;
        font-size: 12px;
        padding-left: 32px;
      }

      &.level-4 {
        font-weight: 400;
        font-size: 11px;
        padding-left: 44px;
        color: var(--el-text-color-secondary);
      }
    }
  }
}

// 通用样式
.outline-sections {
  .outline-content {
    display: flex;
    align-items: center;
    gap: 8px;
    flex: 1;
    min-width: 0;
  }

  .section-icon {
    font-size: 16px;
    color: var(--el-text-color-secondary);
    flex-shrink: 0;
  }

  .section-title {
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    color: var(--el-text-color-primary);
  }

  .outline-info {
    flex-shrink: 0;
    margin-left: 8px;
  }

  .section-line {
    font-size: 11px;
    color: var(--el-text-color-secondary);
    background: var(--el-bg-color-overlay);
    padding: 2px 6px;
    border-radius: 3px;
  }
}
</style>
