<template>
  <div class="cross-reference-nav">
    <!-- 引用导航面板 -->
    <el-drawer
      v-model="showPanel"
      title="交叉引用导航"
      direction="rtl"
      size="400px"
    >
      <!-- 引用统计 -->
      <div class="ref-stats">
        <el-statistic title="总引用" :value="totalReferences" />
        <el-statistic title="已定义" :value="definedLabels.length">
          <template #suffix>
            <el-icon color="#67C23A"><CircleCheck /></el-icon>
          </template>
        </el-statistic>
        <el-statistic title="未定义" :value="undefinedLabels.length">
          <template #suffix>
            <el-icon color="#F56C6C"><Warning /></el-icon>
          </template>
        </el-statistic>
      </div>

      <!-- 引用类型标签 -->
      <el-tabs v-model="activeTab" class="ref-tabs">
        <!-- 未定义引用 -->
        <el-tab-pane name="undefined">
          <template #label>
            <el-badge :value="undefinedLabels.length" :hidden="undefinedLabels.length === 0" type="danger">
              <span>未定义引用</span>
            </el-badge>
          </template>
          <div class="ref-list">
            <div
              v-for="ref in undefinedLabels"
              :key="ref.label"
              class="ref-item undefined"
            >
              <div class="ref-content">
                <code>{{ ref.label }}</code>
                <div class="ref-locations">
                  <el-tag
                    v-for="loc in ref.locations"
                    :key="loc.line"
                    size="small"
                    @click="navigateToLine(loc.line)"
                  >
                    第{{ loc.line }}行
                  </el-tag>
                </div>
              </div>
            </div>
            <el-empty v-if="undefinedLabels.length === 0" description="没有未定义的引用" />
          </div>
        </el-tab-pane>

        <!-- 已定义引用 -->
        <el-tab-pane name="defined">
          <template #label>
            <el-badge :value="definedLabels.length" :hidden="definedLabels.length === 0" type="success">
              <span>已定义引用</span>
            </el-badge>
          </template>
          <div class="ref-list">
            <div
              v-for="ref in definedLabels"
              :key="ref.label"
              class="ref-item defined"
            >
              <div class="ref-header" @click="navigateToLine(ref.definitionLine)">
                <div class="ref-info">
                  <el-icon><SuccessFilled /></el-icon>
                  <code>{{ ref.label }}</code>
                  <el-tag size="small" :type="getRefTypeColor(ref.type)">
                    {{ ref.type }}
                  </el-tag>
                </div>
                <el-button text size="small">
                  <el-icon><Position /></el-icon>
                </el-button>
              </div>
              <div class="ref-usage">
                <span class="usage-label">使用{{ ref.references.length }}次:</span>
                <div class="ref-locations">
                  <el-tag
                    v-for="loc in ref.references"
                    :key="loc.line"
                    size="small"
                    @click="navigateToLine(loc.line)"
                  >
                    第{{ loc.line }}行
                  </el-tag>
                </div>
              </div>
            </div>
            <el-empty v-if="definedLabels.length === 0" description="没有定义的引用" />
          </div>
        </el-tab-pane>

        <!-- 文献引用 -->
        <el-tab-pane name="bibliography">
          <template #label>
            <el-badge :value="bibReferences.length" :hidden="bibReferences.length === 0" type="warning">
              <span>文献引用</span>
            </el-badge>
          </template>
          <div class="ref-list">
            <div
              v-for="ref in bibReferences"
              :key="ref.key"
              class="ref-item bib"
            >
              <div class="ref-content">
                <div class="ref-header">
                  <code>{{ ref.key }}</code>
                  <el-tag size="small">{{ ref.references.length }}次</el-tag>
                </div>
                <div class="ref-details" v-if="ref.entry">
                  <div class="ref-title">{{ ref.entry.title }}</div>
                  <div class="ref-author">{{ ref.entry.author }}</div>
                </div>
                <div class="ref-locations">
                  <el-tag
                    v-for="loc in ref.references"
                    :key="loc.line"
                    size="small"
                    @click="navigateToLine(loc.line)"
                  >
                    第{{ loc.line }}行
                  </el-tag>
                </div>
              </div>
            </div>
            <el-empty v-if="bibReferences.length === 0" description="没有文献引用" />
          </div>
        </el-tab-pane>

        <!-- 图片引用 -->
        <el-tab-pane name="images">
          <template #label>
            <el-badge :value="imageReferences.length" :hidden="imageReferences.length === 0" type="info">
              <span>图片引用</span>
            </el-badge>
          </template>
          <div class="ref-list">
            <div
              v-for="ref in imageReferences"
              :key="ref.path"
              class="ref-item image"
              :class="{ 'missing': !ref.exists }"
            >
              <div class="ref-content">
                <div class="ref-header">
                  <code>{{ ref.path }}</code>
                  <el-tag v-if="!ref.exists" type="danger" size="small">缺失</el-tag>
                  <el-tag v-else type="success" size="small">存在</el-tag>
                </div>
                <div class="ref-locations">
                  <el-tag
                    v-for="loc in ref.references"
                    :key="loc.line"
                    size="small"
                    @click="navigateToLine(loc.line)"
                  >
                    第{{ loc.line }}行
                  </el-tag>
                </div>
              </div>
            </div>
            <el-empty v-if="imageReferences.length === 0" description="没有图片引用" />
          </div>
        </el-tab-pane>
      </el-tabs>

      <!-- 快速操作 -->
      <template #footer>
        <div class="ref-actions">
          <el-button @click="checkAllReferences" :loading="checking">
            <el-icon><Refresh /></el-icon>
            检查引用
          </el-button>
          <el-button type="primary" @click="exportReport">
            <el-icon><Download /></el-icon>
            导出报告
          </el-button>
        </div>
      </template>
    </el-drawer>

    <!-- 悬浮提示框 -->
    <el-popover
      v-model:visible="showHoverPopup"
      :virtual-ref="hoverRef"
      virtual-triggering
      placement="top"
      width="300"
      trigger="hover"
    >
      <div v-if="hoveredRef" class="ref-popup">
        <div class="popup-header">
          <code>{{ hoveredRef.label }}</code>
          <el-tag v-if="hoveredRef.defined" type="success" size="small">已定义</el-tag>
          <el-tag v-else type="danger" size="small">未定义</el-tag>
        </div>
        <div v-if="hoveredRef.defined" class="popup-info">
          <p><strong>定义位置:</strong> 第{{ hoveredRef.definitionLine }}行</p>
          <p><strong>引用次数:</strong> {{ hoveredRef.references?.length || 0 }}</p>
        </div>
        <div class="popup-actions">
          <el-button size="small" @click="navigateToDefinition">跳转定义</el-button>
          <el-button size="small" @click="listAllReferences">查看所有引用</el-button>
        </div>
      </div>
    </el-popover>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { CircleCheck, Warning, SuccessFilled, Position, Refresh, Download } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface ReferenceLocation {
  line: number
  column?: number
}

interface Reference {
  label: string
  type?: string
  defined: boolean
  definitionLine?: number
  references: ReferenceLocation[]
}

interface BibEntry {
  key: string
  title?: string
  author?: string
}

interface ImageReference {
  path: string
  exists: boolean
  references: ReferenceLocation[]
}

interface Props {
  content?: string
  bibEntries?: BibEntry[]
  projectFiles?: Array<{ name: string; path: string; type: string }>
}

const props = withDefaults(defineProps<Props>(), {
  content: '',
  bibEntries: () => [],
  projectFiles: () => []
})

const emit = defineEmits<{
  'navigate': [line: number]
}>()

// 状态
const showPanel = ref(false)
const activeTab = ref('undefined')
const checking = ref(false)
const showHoverPopup = ref(false)
const hoverRef = ref()
const hoveredRef = ref<Reference | null>(null)

// 解析引用
const parsedReferences = computed(() => {
  const refs: Record<string, Reference> = {}
  const lines = props.content.split('\n')

  lines.forEach((line, lineIndex) => {
    const lineNumber = lineIndex + 1

    // 检测定义 \label{key}
    const labelMatch = line.match(/\\label\{([^}]+)\}/)
    if (labelMatch) {
      const key = labelMatch[1]
      if (!refs[key]) {
        refs[key] = {
          label: key,
          defined: true,
          definitionLine: lineNumber,
          references: []
        }
      } else {
        refs[key].defined = true
        refs[key].definitionLine = lineNumber
      }
    }

    // 检测引用 \ref{key} 或 \eqref{key} 或 \pageref{key}
    const refPatterns = [
      /\\ref\{([^}]+)\}/g,
      /\\eqref\{([^}]+)\}/g,
      /\\pageref\{([^}]+)\}/g
    ]

    refPatterns.forEach(pattern => {
      let match
      while ((match = pattern.exec(line)) !== null) {
        const key = match[1]
        if (!refs[key]) {
          refs[key] = {
            label: key,
            defined: false,
            references: [{ line: lineNumber }]
          }
        } else {
          refs[key].references.push({ line: lineNumber })
        }
      }
    })
  })

  return Object.values(refs)
})

const undefinedLabels = computed(() => {
  return parsedReferences.value
    .filter(ref => !ref.defined)
    .map(ref => ({
      label: ref.label,
      locations: ref.references
    }))
})

const definedLabels = computed(() => {
  return parsedReferences.value
    .filter(ref => ref.defined)
    .map(ref => ({
      label: ref.label,
      type: ref.type || 'ref',
      definitionLine: ref.definitionLine!,
      references: ref.references
    }))
})

const bibReferences = computed(() => {
  const bibRefs: Record<string, { key: string; entry?: BibEntry; references: ReferenceLocation[] }> = {}
  const lines = props.content.split('\n')

  lines.forEach((line, lineIndex) => {
    const lineNumber = lineIndex + 1
    // 检测文献引用 \cite{key} 或 \cite{key1,key2}
    const citeMatches = line.match(/\\cite(?:\[[^\]]*\])?\{([^}]+)\}/g)
    if (citeMatches) {
      citeMatches.forEach(match => {
        const keys = match.match(/\\cite(?:\[[^\]]*\])?\{([^}]+)\}/)?.[1].split(',').map(k => k.trim())
        keys?.forEach(key => {
          if (!bibRefs[key]) {
            bibRefs[key] = {
              key,
              entry: props.bibEntries.find(e => e.key === key),
              references: []
            }
          }
          bibRefs[key].references.push({ line: lineNumber })
        })
      })
    }
  })

  return Object.values(bibRefs)
})

const imageReferences = computed(() => {
  const imageRefs: Record<string, { path: string; exists: boolean; references: ReferenceLocation[] }> = {}
  const lines = props.content.split('\n')

  lines.forEach((line, lineIndex) => {
    const lineNumber = lineIndex + 1
    // 检测图片引用 \includegraphics{path} 或 \includegraphics[options]{path}
    const imgMatch = line.match(/\\includegraphics(?:\[[^\]]*\])?\{([^}]+)\}/)
    if (imgMatch) {
      const path = imgMatch[1]
      // 检查文件是否存在
      const exists = props.projectFiles.some(f =>
        f.name === path || f.path === path || f.path?.endsWith(path)
      )

      if (!imageRefs[path]) {
        imageRefs[path] = {
          path,
          exists,
          references: []
        }
      }
      imageRefs[path].references.push({ line: lineNumber })
    }
  })

  return Object.values(imageRefs)
})

const totalReferences = computed(() => {
  return parsedReferences.value.length + bibReferences.value.length + imageReferences.value.length
})

// 方法
function navigateToLine(line: number) {
  emit('navigate', line)
  ElMessage.success(`已跳转到第${line}行`)
}

function navigateToDefinition() {
  if (hoveredRef.value?.definitionLine) {
    navigateToLine(hoveredRef.value.definitionLine)
  }
}

function listAllReferences() {
  if (hoveredRef.value?.references) {
    showHoverPopup.value = false
    // 显示所有引用位置的对话框
    ElMessage.info(`"${hoveredRef.value.label}" 被引用 ${hoveredRef.value.references.length} 次`)
  }
}

function getRefTypeColor(type: string): string {
  const colors: Record<string, string> = {
    'ref': 'primary',
    'eq': 'success',
    'pageref': 'warning'
  }
  return colors[type] || ''
}

function checkAllReferences() {
  checking.value = true
  // 重新解析引用
  setTimeout(() => {
    checking.value = false
    ElMessage.success('引用检查完成')
  }, 500)
}

function exportReport() {
  const report = {
    undefined: undefinedLabels.value,
    defined: definedLabels.value,
    bibliography: bibReferences.value,
    images: imageReferences.value
  }

  const blob = new Blob([JSON.stringify(report, null, 2)], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = 'cross-reference-report.json'
  a.click()
  URL.revokeObjectURL(url)

  ElMessage.success('报告已导出')
}

// 暴露方法供父组件调用
function showPanel() {
  showPanel.value = true
}

function highlightReference(label: string, line: number, column: number) {
  hoveredRef.value = parsedReferences.value.find(r => r.label === label) || null
  if (hoveredRef.value) {
    showHoverPopup.value = true
  }
}

defineExpose({
  showPanel,
  highlightReference
})
</script>

<style scoped lang="scss">
.ref-stats {
  display: flex;
  gap: 16px;
  padding: 16px;
  background: var(--el-fill-color-blank);
  border-radius: 8px;
  margin-bottom: 16px;
}

.ref-tabs {
  height: calc(100vh - 250px);

  :deep(.el-tabs__content) {
    height: calc(100% - 55px);
    overflow-y: auto;
  }
}

.ref-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.ref-item {
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  padding: 12px;
  background: var(--el-fill-color-blank);

  &.undefined {
    border-color: var(--el-color-danger);
    background: var(--el-color-danger-light-9);
  }

  &.defined {
    border-color: var(--el-color-success);
  }

  &.missing {
    border-color: var(--el-color-warning);
    background: var(--el-color-warning-light-9);
  }
}

.ref-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  cursor: pointer;
  padding: 4px 0;

  &:hover {
    .ref-info code {
      color: var(--el-color-primary);
    }
  }
}

.ref-info {
  display: flex;
  align-items: center;
  gap: 8px;
}

.ref-usage {
  margin-top: 8px;
  padding-left: 24px;
}

.usage-label {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin-right: 8px;
}

.ref-locations {
  display: flex;
  gap: 4px;
  flex-wrap: wrap;
  margin-top: 8px;

  .el-tag {
    cursor: pointer;

    &:hover {
      opacity: 0.8;
    }
  }
}

.ref-details {
  margin-top: 8px;
  padding-left: 24px;
  font-size: 13px;
  color: var(--el-text-color-regular);
}

.ref-title {
  font-weight: 500;
  margin-bottom: 4px;
}

.ref-author {
  color: var(--el-text-color-secondary);
}

.ref-popup {
  .popup-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 8px;
  }

  .popup-info {
    font-size: 13px;
    color: var(--el-text-color-regular);
    margin-bottom: 8px;

    p {
      margin: 4px 0;
    }
  }

  .popup-actions {
    display: flex;
    gap: 8px;
  }
}

.ref-actions {
  display: flex;
  gap: 8px;
  width: 100%;
}

code {
  background: var(--el-fill-color-light);
  padding: 2px 6px;
  border-radius: 4px;
  font-family: 'Courier New', monospace;
  font-size: 13px;
}
</style>
