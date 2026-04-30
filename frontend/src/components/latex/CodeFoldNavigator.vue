/**
 * LaTeX代码折叠和大纲导航
 * 支持section折叠、大纲快速跳转、面包屑导航
 */

<template>
  <div class="code-fold-navigator">
    <!-- 大纲面板 -->
    <div class="outline-panel">
      <div class="panel-header">
        <h4>文档大纲</h4>
        <div class="header-actions">
          <el-button size="small" text @click="expandAll">
            <el-icon><Plus /></el-icon>
          </el-button>
          <el-button size="small" text @click="collapseAll">
            <el-icon><Minus /></el-icon>
          </el-button>
        </div>
      </div>

      <!-- 面包屑导航 -->
      <div v-if="breadcrumb.length > 0" class="breadcrumb-nav">
        <el-breadcrumb separator="/">
          <el-breadcrumb-item
            v-for="(item, index) in breadcrumb"
            :key="index"
            @click="jumpToOutline(item)"
          >
            {{ item.label }}
          </el-breadcrumb-item>
        </el-breadcrumb>
      </div>

      <!-- 大纲列表 -->
      <div class="outline-list">
        <div
          v-for="item in outline"
          :key="item.id"
          class="outline-item"
          :class="[
            `level-${item.level}`,
            {
              'is-active': activeOutline === item.id,
              'is-collapsed': collapsedSections.has(item.id)
            }
          ]"
          :style="{ paddingLeft: `${(item.level - 1) * 16 + 8}px` }"
          @click="toggleSection(item)"
        >
          <div class="outline-content">
            <el-icon
              v-if="item.hasChildren"
              class="collapse-icon"
              :size="14"
            >
              <ArrowRight v-if="collapsedSections.has(item.id)" />
              <ArrowDown v-else />
            </el-icon>
            <span class="outline-label">{{ item.label }}</span>
            <el-tag v-if="item.line" size="small" type="info">
              {{ item.line }}
            </el-tag>
          </div>
        </div>

        <el-empty v-if="outline.length === 0" description="暂无大纲" :image-size="60" />
      </div>
    </div>

    <!-- 折叠状态指示器 -->
    <div v-if="foldedRegions.length > 0" class="fold-indicator">
      <el-tag size="small" type="warning">
        已折叠 {{ foldedRegions.length }} 个区域
      </el-tag>
      <el-button size="small" text @click="expandAll">
        展开全部
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'
import { Plus, Minus, ArrowRight, ArrowDown } from '@element-plus/icons-vue'
import type * as Monaco from 'monaco-editor'

interface OutlineItem {
  id: string
  level: number
  label: string
  line: number
  type: 'part' | 'chapter' | 'section' | 'subsection' | 'subsubsection'
  hasChildren: boolean
  children?: OutlineItem[]
}

interface Props {
  editor?: any
  content?: string
}

const props = defineProps<Props>()

const emit = defineEmits<{
  (e: 'jump-to-line', line: number): void
}>()

// 状态
const outline = ref<OutlineItem[]>([])
const activeOutline = ref<string | null>(null)
const collapsedSections = ref<Set<string>>(new Set())
const breadcrumb = ref<Array<{ id: string; label: string; line: number }>>([])

// 正则表达式模式
const SECTION_PATTERNS = [
  { type: 'part', regex: /^\\part\*?\s*\{([^}]+)\}/i, level: 0 },
  { type: 'chapter', regex: /^\\chapter\*?\s*\{([^}]+)\}/i, level: 1 },
  { type: 'section', regex: /^\\section\*?\s*\{([^}]+)\}/i, level: 2 },
  { type: 'subsection', regex: /^\\subsection\*?\s*\{([^}]+)\}/i, level: 3 },
  { type: 'subsubsection', regex: /^\\subsubsection\*?\s*\{([^}]+)\}/i, level: 4 }
]

// 解析LaTeX大纲
const parseOutline = (content: string) => {
  const items: OutlineItem[] = []
  const lines = content.split('\n')

  lines.forEach((line, index) => {
    for (const pattern of SECTION_PATTERNS) {
      const match = line.match(pattern.regex)
      if (match) {
        const label = match[1].trim()
        const item: OutlineItem = {
          id: `sec-${index}`,
          level: pattern.level,
          label,
          line: index + 1,
          type: pattern.type as any,
          hasChildren: false
        }
        items.push(item)
        break
      }
    }
  })

  // 构建层次结构
  buildHierarchy(items)
  outline.value = items
}

// 构建层次结构
const buildHierarchy = (items: OutlineItem[]) => {
  const stack: OutlineItem[] = []

  items.forEach(item => {
    // 弹出比当前级别高的项
    while (stack.length > 0 && stack[stack.length - 1].level >= item.level) {
      stack.pop()
    }

    // 设置父子关系
    if (stack.length > 0) {
      stack[stack.length - 1].hasChildren = true
      if (!stack[stack.length - 1].children) {
        stack[stack.length - 1].children = []
      }
      stack[stack.length - 1].children!.push(item)
    }

    stack.push(item)
  })
}

// 切换section折叠
const toggleSection = (item: OutlineItem) => {
  if (!item.hasChildren) {
    // 没有子项，直接跳转
    jumpToLine(item.line)
    return
  }

  if (collapsedSections.value.has(item.id)) {
    collapsedSections.value.delete(item.id)
  } else {
    collapsedSections.value.add(item.id)
  }

  updateBreadcrumb()
}

// 跳转到大纲项
const jumpToOutline = (item: { id: string; label: string; line: number }) => {
  jumpToLine(item.line)
}

// 跳转到指定行
const jumpToLine = (line: number) => {
  emit('jump-to-line', line)

  const editor = props.editor || (window as any).monacoEditor
  if (editor) {
    editor.revealLineInCenter(line)
    editor.setPosition({ lineNumber: line, column: 1 })
    editor.focus()
  }

  // 更新当前激活项
  updateActiveOutline(line)
}

// 更新当前激活的大纲项
const updateActiveOutline = (line: number) => {
  // 找到最近的section
  let activeId: string | null = null
  let minDistance = Infinity

  const findActive = (items: OutlineItem[]) => {
    for (const item of items) {
      if (item.line <= line && line - item.line < minDistance) {
        minDistance = line - item.line
        activeId = item.id
      }
      if (item.children) {
        findActive(item.children)
      }
    }
  }

  findActive(outline.value)
  activeOutline.value = activeId
  updateBreadcrumb()
}

// 更新面包屑
const updateBreadcrumb = () => {
  if (!activeOutline.value) {
    breadcrumb.value = []
    return
  }

  const path: Array<{ id: string; label: string; line: number }> = []

  const findPath = (items: OutlineItem[], targetId: string, currentPath: OutlineItem[] = []): boolean => {
    for (const item of items) {
      const newPath = [...currentPath, item]

      if (item.id === targetId) {
        path.push(...newPath.filter(i => !collapsedSections.value.has(i.id)))
        return true
      }

      if (item.children && !collapsedSections.value.has(item.id)) {
        if (findPath(item.children, targetId, newPath)) {
          return true
        }
      }
    }
    return false
  }

  findPath(outline.value, activeOutline.value)

  breadcrumb.value = path.map(item => ({
    id: item.id,
    label: item.label,
    line: item.line
  }))
}

// 展开全部
const expandAll = () => {
  collapsedSections.value.clear()
  updateBreadcrumb()
}

// 折叠全部
const collapseAll = () => {
  outline.value.forEach(item => {
    if (item.hasChildren) {
      collapsedSections.value.add(item.id)
    }
  })
  updateBreadcrumb()
}

// 已折叠区域
const foldedRegions = computed(() => {
  return Array.from(collapsedSections.value).map(id => {
    const findItem = (items: OutlineItem[]): OutlineItem | null => {
      for (const item of items) {
        if (item.id === id) return item
        if (item.children) {
          const found = findItem(item.children)
          if (found) return found
        }
      }
      return null
    }
    return findItem(outline.value)
  }).filter(Boolean)
})

// 监听内容变化
watch(() => props.content, (newContent) => {
  if (newContent) {
    parseOutline(newContent)
  }
}, { immediate: true })

// 监听光标位置变化（如果编辑器可用）
onMounted(() => {
  const editor = props.editor || (window as any).monacoEditor
  if (editor) {
    editor.onDidChangeCursorPosition((e: any) => {
      updateActiveOutline(e.position.lineNumber)
    })
  }
})

// 暴露方法
defineExpose({
  refresh: () => {
    if (props.content) {
      parseOutline(props.content)
    }
  },
  jumpToLine,
  expandAll,
  collapseAll
})
</script>

<style scoped lang="scss">
.code-fold-navigator {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: var(--el-bg-color-page);
  color: var(--el-text-color-primary);

  .outline-panel {
    flex: 1;
    display: flex;
    flex-direction: column;
    overflow: hidden;

    .panel-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 12px 16px;
      border-bottom: 1px solid var(--el-border-color-light);

      h4 {
        margin: 0;
        font-size: 14px;
        font-weight: 600;
      }

      .header-actions {
        display: flex;
        gap: 4px;
      }
    }

    .breadcrumb-nav {
      padding: 8px 16px;
      border-bottom: 1px solid var(--el-border-color-lighter);

      :deep(.el-breadcrumb__item) {
        cursor: pointer;

        &:hover {
          color: var(--el-color-primary);
        }
      }

      :deep(.el-breadcrumb__separator) {
        margin: 0 6px;
      }
    }

    .outline-list {
      flex: 1;
      overflow-y: auto;
      padding: 8px 0;

      .outline-item {
        cursor: pointer;
        user-select: none;

        &.is-active {
          .outline-content {
            background: var(--el-fill-color-light);
            color: var(--el-color-primary);
          }
        }

        &.is-collapsed {
          .collapse-icon {
            transform: rotate(-90deg);
          }
        }

        .outline-content {
          display: flex;
          align-items: center;
          gap: 6px;
          padding: 6px 8px;
          margin: 0 8px;
          border-radius: 4px;
          transition: all 0.2s;

          &:hover {
            background: var(--el-fill-color);
          }

          .collapse-icon {
            flex-shrink: 0;
            transition: transform 0.2s;
          }

          .outline-label {
            flex: 1;
            font-size: 13px;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
          }
        }
      }
    }
  }

  .fold-indicator {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 16px;
    border-top: 1px solid var(--el-border-color-light);
    background: var(--el-bg-color);
  }
}

// 不同层级的缩进
.level-0 {
  font-weight: 600;
  font-size: 14px;
}

.level-1 {
  font-weight: 500;
  font-size: 13px;
}

.level-2 {
  font-size: 13px;
}

.level-3 {
  font-size: 12px;
  opacity: 0.9;
}

.level-4 {
  font-size: 12px;
  opacity: 0.8;
}
</style>
