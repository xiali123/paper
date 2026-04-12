<template>
  <div class="document-outline">
    <div v-if="sections.length === 0" class="outline-empty">
      <el-empty description="暂无文档结构" :image-size="40" />
    </div>
    <div v-else class="outline-sections">
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
</template>

<script setup lang="ts">
import { ref, computed, watch, onUnmounted } from 'vue'
import { Document, Folder, Tickets } from '@element-plus/icons-vue'

interface Section {
  id: string
  title: string
  level: number
  line: number
  type: 'section' | 'subsection' | 'subsubsection' | 'paragraph'
}

interface Props {
  content: string
}

interface Emits {
  navigate: [{ line: number; column: number }]
}

const props = defineProps<Props>()
const emit = defineEmits<Emits>()

const activeSection = ref<string | null>(null)

// Debug logging
if (import.meta.env.DEV) {
  console.log('DocumentOutline component mounted, content length:', props.content?.length || 0)
}

// 解析文档结构
const sections = computed<Section[]>(() => {
  if (!props.content) return []

  const lines = props.content.split('\n')
  const parsedSections: Section[] = []

  lines.forEach((line, index) => {
    const lineNumber = index + 1

    // 匹配章节命令
    const sectionMatch = line.match(/^\\section\*?\{([^}]+)\}/)
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

    const subsectionMatch = line.match(/^\\subsection\*?\{([^}]+)\}/)
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

    const subsubsectionMatch = line.match(/^\\subsubsection\*?\{([^}]+)\}/)
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

    const paragraphMatch = line.match(/^\\paragraph\*?\{([^}]+)\}/)
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
})

function navigateToSection(section: Section) {
  if (import.meta.env.DEV) {
    console.log('DocumentOutline: navigating to section', section)
  }
  activeSection.value = section.id
  emit('navigate', { line: section.line, column: 1 })
}

// 监听内容变化，更新高亮 - optimized to prevent memory leaks
let outlineTimeout: number | null = null

watch(() => props.content, (newContent) => {
  if (import.meta.env.DEV) {
    console.log('DocumentOutline content changed, new length:', newContent?.length || 0)
  }

  // Debounced processing to prevent excessive recomputation
  if (outlineTimeout) {
    clearTimeout(outlineTimeout)
  }

  outlineTimeout = setTimeout(() => {
    // 可以在这里添加逻辑来根据当前光标位置更新activeSection
    outlineTimeout = null
  }, 300)
})

// 清理定时器防止内存泄漏
onUnmounted(() => {
  if (outlineTimeout) {
    clearTimeout(outlineTimeout)
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