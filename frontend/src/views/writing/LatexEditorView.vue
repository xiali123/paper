<template>
  <div class="latex-editor-view">
    <!-- 顶部工具栏 -->
    <div class="editor-header">
      <div class="document-info">
        <el-breadcrumb separator="/">
          <el-breadcrumb-item :to="{ path: '/writing' }">协作写作</el-breadcrumb-item>
          <el-breadcrumb-item>LaTeX编辑器</el-breadcrumb-item>
          <el-breadcrumb-item v-if="currentDocument">{{ currentDocument.name }}</el-breadcrumb-item>
        </el-breadcrumb>

        <div class="document-actions" v-if="currentDocument">
          <el-button-group>
            <el-button
              size="small"
              :type="isModified ? 'primary' : 'default'"
              @click="saveDocument"
              :loading="saving"
            >
              <el-icon><DocumentChecked /></el-icon>
              保存
            </el-button>
            <el-button size="small" @click="compileDocument" :loading="compiling">
              <el-icon><VideoPlay /></el-icon>
              编译
            </el-button>
            <el-button size="small" @click="showPreview = !showPreview">
              <el-icon><View /></el-icon>
              {{ showPreview ? '隐藏预览' : '显示预览' }}
            </el-button>
          </el-button-group>
        </div>
      </div>

      <!-- 协作用户列表 -->
      <div class="collaboration-users" v-if="isCollaborating">
        <el-avatar-group :max="5">
          <el-avatar
            v-for="user in activeCollaborationUsers"
            :key="user.id"
            :size="32"
            :style="{ backgroundColor: user.color }"
          >
            {{ user.name.charAt(0).toUpperCase() }}
          </el-avatar>
        </el-avatar-group>
        <span class="collaboration-status">
          <el-icon><UserFilled /></el-icon>
          {{ activeCollaborationUsers.length }} 人在线
        </span>
      </div>
    </div>

    <!-- 主编辑区域 -->
    <div class="editor-main" :class="{ 'show-preview': showPreview }">
      <!-- 左侧：编辑器面板 -->
      <div class="editor-panel" :class="{ 'with-preview': showPreview }">
        <div class="editor-content" :class="{ 'with-outline': showOutline }">
          <!-- 文档大纲 -->
          <div class="document-outline" v-if="showOutline">
            <div class="outline-header">
              <h3>文档大纲</h3>
              <el-button size="small" @click="showOutline = false">
                <el-icon><Close /></el-icon>
              </el-button>
            </div>
            <div class="outline-content">
              <DocumentOutline
                :content="editorContent"
                @navigate="navigateToSection"
              />
            </div>
          </div>

          <!-- 编辑器区域 -->
          <div class="editor-area" :class="{ 'with-outline': showOutline }">
            <!-- 编辑器工具栏 -->
            <div class="editor-toolbar">
          <el-button-group>
            <el-button size="small" @click="toggleOutline">
              <el-icon><Menu /></el-icon>
              大纲
            </el-button>
            <el-button size="small" @click="insertLatexCommand('textbf')">
              <b>B</b>
            </el-button>
            <el-button size="small" @click="insertLatexCommand('textit')">
              <i>I</i>
            </el-button>
            <el-dropdown size="small" @command="insertLatexEnvironment">
              <el-button size="small">
                <el-icon><Plus /></el-icon>
                环境
              </el-button>
              <template #dropdown>
                <el-dropdown-menu>
                  <el-dropdown-item command="itemize">无序列表</el-dropdown-item>
                  <el-dropdown-item command="enumerate">有序列表</el-dropdown-item>
                  <el-dropdown-item command="equation">数学公式</el-dropdown-item>
                  <el-dropdown-item command="figure">图片环境</el-dropdown-item>
                  <el-dropdown-item command="table">表格环境</el-dropdown-item>
                </el-dropdown-menu>
              </template>
            </el-dropdown>
            <el-button size="small" @click="showSymbolPalette = !showSymbolPalette">
              <el-icon><Tickets /></el-icon>
              符号
            </el-button>
          </el-button-group>

          <!-- 编译状态指示器 -->
          <div class="compilation-status">
            <el-tag
              v-if="compilationStatus !== 'idle'"
              :type="compilationStatusType"
              size="small"
            >
              <el-icon v-if="compilationStatus === 'compiling'"><Loading /></el-icon>
              {{ compilationStatusText }}
            </el-tag>
          </div>
        </div>

        <!-- Use the working LatexEditor component -->
        <div class="editor-wrapper">
          <LatexEditor
            ref="editorRef"
            v-model="editorContent"
            @change="() => isModified = true"
            @cursor-change="(position) => (latexStore as any).updateCursorPosition(position)"
          />
        </div>

        <!-- 状态栏 -->
        <div class="editor-status-bar">
          <div class="status-left">
            <span>行: {{ cursorPosition.line }}, 列: {{ cursorPosition.column }}</span>
            <span>字符数: {{ documentStats.characters }}</span>
            <span>字数: {{ documentStats.words }}</span>
          </div>
          <div class="status-right">
            <span :class="{ 'text-warning': isModified }">
              {{ isModified ? '已修改' : '已保存' }}
            </span>
            <span>LaTeX</span>
            <span :class="theme === 'dark' ? 'dark' : 'light'">
              {{ theme === 'dark' ? '深色' : '浅色' }}主题
            </span>
          </div>
        </div>
      </div>
    </div>
  </div>

      <!-- 右侧：预览面板 -->
      <div class="preview-panel" v-if="showPreview">
        <div class="preview-header">
          <h3>实时预览</h3>
          <div class="preview-controls">
            <el-button-group size="small">
              <el-button @click="zoomOut">
                <el-icon><ZoomOut /></el-icon>
              </el-button>
              <el-button @click="zoomIn">
                <el-icon><ZoomIn /></el-icon>
              </el-button>
              <el-button @click="resetZoom">
                {{ Math.round(previewScale * 100) }}%
              </el-button>
            </el-button-group>
          </div>
        </div>

        <div class="preview-content">
          <LatexPreview
            :content="editorContent"
            :scale="previewScale"
            :theme="theme"
            ref="previewRef"
          />
        </div>

        <!-- 错误面板 -->
        <div class="error-panel" v-if="hasErrors || hasWarnings">
          <div class="error-header">
            <el-tabs v-model="activeErrorTab">
              <el-tab-pane name="errors" v-if="hasErrors">
                <template #label>
                  <el-badge :value="errors.length" type="danger">
                    <el-icon><Warning /></el-icon>
                    错误
                  </el-badge>
                </template>
              </el-tab-pane>
              <el-tab-pane name="warnings" v-if="hasWarnings">
                <template #label>
                  <el-badge :value="warnings.length" type="warning">
                    <el-icon><InfoFilled /></el-icon>
                    警告
                  </el-badge>
                </template>
              </el-tab-pane>
            </el-tabs>
          </div>

          <div class="error-content">
            <div
              v-for="error in activeErrors"
              :key="error.line"
              class="error-item"
              @click="navigateToError(error)"
            >
              <el-icon :class="error.type"><Warning /></el-icon>
              <span class="error-line">第 {{ error.line }} 行:</span>
              <span class="error-message">{{ error.message }}</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 符号面板 -->
    <el-drawer
      v-model="showSymbolPalette"
      title="LaTeX符号"
      direction="rtl"
      size="300px"
    >
      <SymbolPalette @insert="insertSymbol" />
    </el-drawer>

    <!-- 协作面板 -->
    <el-drawer
      v-model="showCollaborationPanel"
      title="协作功能"
      direction="rtl"
      size="350px"
    >
      <CollaborationPanel
        :session="collaborationSession"
        :users="collaborationUsers"
        @invite="inviteUser"
        @leave="leaveCollaboration"
      />
    </el-drawer>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useLatexEditorStore } from '@/architecture/stores/latexEditor'
import { ElMessage } from 'element-plus'
import {
  DocumentChecked, VideoPlay, View, UserFilled, Menu, Plus,
  Tickets, Loading, Warning, InfoFilled, Close,
  ZoomIn, ZoomOut
} from '@element-plus/icons-vue'
import LatexPreview from '@/components/latex/LatexPreview.vue'
import LatexEditor from '@/components/latex/LatexEditor.vue'
import DocumentOutline from '@/components/latex/DocumentOutline.vue'
import SymbolPalette from '@/components/latex/SymbolPalette.vue'
import CollaborationPanel from '@/components/collaboration/CollaborationPanel.vue'
// Monaco editor integration removed - using simple LatexEditor component

// Props and emits
interface Props {
  documentId?: string
}

const props = defineProps<Props>()

// Router and store
const latexStore = useLatexEditorStore()

// Refs
const previewRef = ref<InstanceType<typeof LatexPreview> | null>(null)
const editorRef = ref<any>(null)

// Reactive state
const showPreview = ref(true)
const showOutline = ref(false)
const showSymbolPalette = ref(false)
const showCollaborationPanel = ref(false)
const saving = ref(false)
const compiling = ref(false)
const editorLoaded = ref(false)
const activeErrorTab = ref('errors')

// Store computed properties
const currentDocument = computed(() => latexStore.currentDocument)
const editorContent = computed({
  get: () => latexStore.editorContent,
  set: (value) => latexStore.updateDocumentContent(value)
})
const compilationStatus = computed(() => latexStore.compilationStatus)
const isCollaborating = computed(() => latexStore.isCollaborating)
const collaborationSession = computed(() => latexStore.collaborationSession)
const collaborationUsers = computed(() => Array.from(latexStore.collaborationUsers.values()))
const activeCollaborationUsers = computed(() => latexStore.activeCollaborationUsers)
const documentStats = computed(() => latexStore.documentStats)
const cursorPosition = ref({ line: 1, column: 1 })

// Sync cursor position with store
watch(() => latexStore.editorCursor, (newPosition) => {
  cursorPosition.value = newPosition
}, { immediate: true })
const theme = computed(() => latexStore.currentTheme)
const previewScale = computed({
  get: () => latexStore.previewScale,
  set: (value) => latexStore.setPreviewScale(value)
})

const errors = computed(() => latexStore.errors)
const warnings = computed(() => latexStore.warnings)
const hasErrors = computed(() => latexStore.hasErrors)
const hasWarnings = computed(() => latexStore.hasWarnings)

// Computed properties
const isModified = ref(false)
const compilationStatusType = computed(() => {
  switch (compilationStatus.value) {
    case 'success': return 'success'
    case 'error': return 'danger'
    case 'warning': return 'warning'
    case 'compiling': return 'info'
    default: return 'info'
  }
})

const compilationStatusText = computed(() => {
  switch (compilationStatus.value) {
    case 'success': return '编译成功'
    case 'error': return '编译错误'
    case 'warning': return '编译警告'
    case 'compiling': return '编译中...'
    default: return '就绪'
  }
})

const activeErrors = computed(() => {
  return activeErrorTab.value === 'errors' ? errors.value : warnings.value
})

// Simple editor focus method
const editorFocus = () => {
  // Focus functionality can be added to LatexEditor component if needed
}

// Methods
function saveDocument() {
  console.log('Save document clicked')
  saving.value = true
  try {
    latexStore.saveDocument()
    isModified.value = false
    console.log('Document saved successfully')
    // Show success message
    ElMessage.success('文档保存成功')
  } catch (error) {
    console.error('Save failed:', error)
    // Show error message
    ElMessage.error('文档保存失败: ' + (error instanceof Error ? error.message : '未知错误'))
  } finally {
    saving.value = false
  }
}

async function compileDocument() {
  compiling.value = true
  try {
    const result = await latexStore.compileDocument()
    if (result.success) {
      // Update preview
      if (previewRef.value) {
        previewRef.value.refresh()
      }
    }
  } catch (error) {
    console.error('Compilation failed:', error)
  } finally {
    compiling.value = false
  }
}

function insertLatexCommand(command: string) {
  // This function can be implemented with the LatexEditor component's toolbar
  console.log('Insert LaTeX command:', command)
}

function insertLatexEnvironment(env: string) {
  // This function can be implemented with the LatexEditor component's toolbar
  console.log('Insert LaTeX environment:', env)
}

function insertSymbol(symbol: string) {
  // This function can be implemented with the LatexEditor component's toolbar
  console.log('Insert symbol:', symbol)
  showSymbolPalette.value = false
}

function toggleOutline() {
  console.log('Toggle outline clicked, current state:', showOutline.value)
  showOutline.value = !showOutline.value
  console.log('New outline state:', showOutline.value)
}

function navigateToSection(position: { line: number; column?: number }) {
  // Navigate to specific section/line in the editor
  console.log('Navigate to section:', position)

  try {
    // Update cursor position in the store
    ;(latexStore as any).updateCursorPosition(position)

    // Update local cursor position for immediate feedback
    const { line, column = 1 } = position
    cursorPosition.value = {
      line,
      column
    }

    // Navigate to the position in the editor
    if (editorRef.value) {
      editorRef.value.navigateTo({ line, column })
    }

    console.log('Navigation successful to line', line, 'column', column)
  } catch (error) {
    console.error('Navigation failed:', error)
  }
}

function navigateToError(error: any) {
  // Navigate to error location
  navigateToSection({ line: error.line, column: error.column || 1 })
}

function zoomIn() {
  previewScale.value = Math.min(2.0, previewScale.value + 0.1)
}

function zoomOut() {
  previewScale.value = Math.max(0.5, previewScale.value - 0.1)
}

function resetZoom() {
  previewScale.value = 1.0
}

function inviteUser(email: string) {
  // Handle user invitation
  console.log('Inviting user:', email)
}

function leaveCollaboration() {
  latexStore.stopCollaboration()
  showCollaborationPanel.value = false
}

// Lifecycle
onMounted(async () => {
  // Initialize document
  if (props.documentId) {
    // Load existing document
    console.log('Loading document:', props.documentId)
  } else {
    // Create new document
    console.log('Creating new document...')
    await latexStore.createNewDocument('新建文档.tex')
    console.log('Document created, content length:', editorContent.value?.length)
  }

  // Setup auto-save (placeholder - will be implemented in phase 2)
  // latexStore.startAutoSave()

  // Watch for theme changes - can be implemented with LatexEditor component
  watch(() => theme.value, (_newTheme) => {
    // Update editor theme if needed
  })

  // Watch for content changes
  watch(() => editorContent.value, (newContent, oldContent) => {
    console.log('Editor content changed in view:', { newLength: newContent?.length, oldLength: oldContent?.length })
  }, { immediate: true })
})

onUnmounted(() => {
  // latexStore.stopAutoSave() // placeholder for phase 2
  latexStore.cleanup()
})

// Expose methods to template
defineExpose({
  saveDocument,
  compileDocument,
  focus: editorFocus,
  getContent: () => editorContent.value
})
</script>

<style scoped lang="scss">
.latex-editor-view {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);
}

.editor-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);
  background: var(--el-bg-color-page);

  .document-info {
    display: flex;
    align-items: center;
    gap: 16px;

    .document-actions {
      display: flex;
      gap: 8px;
    }
  }

  .collaboration-users {
    display: flex;
    align-items: center;
    gap: 12px;

    .collaboration-status {
      font-size: 12px;
      color: var(--el-text-color-secondary);
      display: flex;
      align-items: center;
      gap: 4px;
    }
  }
}

.editor-main {
  flex: 1;
  display: flex;
  overflow: hidden;

  &.show-preview {
    .editor-panel {
      border-right: 1px solid var(--el-border-color-lighter);
    }
  }
}

.editor-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);

  &.with-preview {
    flex: 0.6;
  }

  &.with-outline {
    display: flex;
    flex-direction: row;
  }

  .editor-content {
    flex: 1;
    display: flex;
    flex-direction: row;
    overflow: hidden;
  }

  .document-outline {
    width: 250px;
    border-right: 1px solid var(--el-border-color-lighter);
    background: var(--el-bg-color-page);
    flex-shrink: 0;

    .outline-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 12px 16px;
      border-bottom: 1px solid var(--el-border-color-lighter);

      h3 {
        margin: 0;
        font-size: 14px;
        font-weight: 600;
      }
    }

    .outline-content {
      padding: 8px;
      height: calc(100% - 50px);
      overflow-y: auto;
    }
  }

  .editor-area {
    flex: 1;
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }


  .editor-toolbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 8px 16px;
    border-bottom: 1px solid var(--el-border-color-lighter);
    background: var(--el-bg-color-page);

    .compilation-status {
      display: flex;
      align-items: center;
      gap: 8px;
    }
  }

  .editor-wrapper {
    flex: 1;
    display: flex;
    flex-direction: column;
  }

  .editor-status-bar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 4px 16px;
    border-top: 1px solid var(--el-border-color-lighter);
    background: var(--el-bg-color-overlay);
    font-size: 12px;
    color: var(--el-text-color-secondary);

    .status-left {
      display: flex;
      gap: 16px;
    }

    .status-right {
      display: flex;
      gap: 16px;

      .text-warning {
        color: var(--el-color-warning);
      }
    }
  }
}

.preview-panel {
  flex: 0.4;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);

  .preview-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 16px;
    border-bottom: 1px solid var(--el-border-color-lighter);

    h3 {
      margin: 0;
      font-size: 14px;
      font-weight: 600;
    }

    .preview-controls {
      display: flex;
      gap: 4px;
    }
  }

  .preview-content {
    flex: 1;
    overflow: hidden;
  }

  .error-panel {
    border-top: 1px solid var(--el-border-color-lighter);
    background: var(--el-bg-color-page);

    .error-header {
      padding: 8px 16px 0;
    }

    .error-content {
      max-height: 200px;
      overflow-y: auto;
      padding: 8px 16px;

      .error-item {
        display: flex;
        align-items: flex-start;
        gap: 8px;
        padding: 8px 0;
        border-bottom: 1px solid var(--el-border-color-lighter);
        cursor: pointer;
        transition: background-color 0.2s;

        &:hover {
          background: var(--el-bg-color-overlay);
        }

        .error-line {
          font-weight: 600;
          color: var(--el-text-color-primary);
          min-width: 60px;
        }

        .error-message {
          flex: 1;
          color: var(--el-text-color-regular);
        }

        .error {
          color: var(--el-color-danger);
        }

        .warning {
          color: var(--el-color-warning);
        }
      }
    }
  }

  .symbol-palette {
    .symbol-grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(40px, 1fr));
      gap: 8px;
      padding: 16px;

      .symbol-item {
        display: flex;
        align-items: center;
        justify-content: center;
        height: 40px;
        border: 1px solid var(--el-border-color-lighter);
        border-radius: 4px;
        cursor: pointer;
        transition: all 0.2s;

        &:hover {
          background: var(--el-color-primary-light-9);
          border-color: var(--el-color-primary);
        }
      }
    }
  }

  .collaboration-panel {
    .user-list {
      padding: 16px;

      .user-item {
        display: flex;
        align-items: center;
        gap: 8px;
        padding: 8px 0;

        .user-avatar {
          width: 32px;
          height: 32px;
          border-radius: 50%;
          display: flex;
          align-items: center;
          justify-content: center;
          color: white;
          font-weight: 600;
        }

        .user-info {
          flex: 1;

          .user-name {
            font-weight: 600;
            color: var(--el-text-color-primary);
          }

          .user-status {
            font-size: 12px;
            color: var(--el-text-color-secondary);
          }
        }
      }
    }

    .invite-section {
      padding: 16px;
      border-top: 1px solid var(--el-border-color-lighter);

      .invite-form {
        display: flex;
        gap: 8px;
        margin-top: 8px;

        .el-input {
          flex: 1;
        }
      }
    }
  }
}

// 响应式设计
@media (max-width: 768px) {
  .latex-editor-view {
    .editor-header {
      flex-direction: column;
      gap: 8px;
      align-items: stretch;
    }

    .editor-main {
      flex-direction: column;

      .editor-panel {
        &.with-preview {
          flex: 1;
        }
      }

      .preview-panel {
        flex: 1;
        min-height: 300px;
      }
    }
  }
}

// 深色模式适配
.dark {
  .latex-editor-view {
    .monaco-editor {
      background: #1e1e1e !important;
    }

    .preview-panel {
      .preview-content {
        background: #1e1e1e;
      }
    }
  }
}</style>