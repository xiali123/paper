<template>
  <div class="latex-editor-view" role="application" aria-label="LaTeX 编辑器">
    <!-- 顶部工具栏 -->
    <header class="editor-header" role="banner">
      <nav class="document-info" aria-label="文档导航">
        <el-breadcrumb separator="/" aria-label="面包屑导航">
          <el-breadcrumb-item :to="{ path: '/writing' }">协作写作</el-breadcrumb-item>
          <el-breadcrumb-item>LaTeX编辑器</el-breadcrumb-item>
          <el-breadcrumb-item v-if="currentDocument">{{ currentDocument.name }}</el-breadcrumb-item>
        </el-breadcrumb>

        <div class="document-actions" role="toolbar" aria-label="文档操作" v-if="currentDocument">
          <el-button-group>
            <el-button
              size="small"
              :type="isModified ? 'primary' : 'default'"
              @click="saveDocument"
              :loading="saving"
              aria-label="保存文档 (Ctrl+S)"
              :aria-busy="saving"
            >
              <el-icon><DocumentChecked /></el-icon>
              保存
            </el-button>
            <el-button
              size="small"
              @click="compileDocument"
              :loading="compiling"
              aria-label="编译文档 (Ctrl+Enter)"
              :aria-busy="compiling"
            >
              <el-icon><VideoPlay /></el-icon>
              编译
            </el-button>
            <el-button
              size="small"
              @click="showPreview = !showPreview"
              :aria-label="showPreview ? '隐藏预览面板' : '显示预览面板'"
              :aria-pressed="showPreview"
            >
              <el-icon><View /></el-icon>
              {{ showPreview ? '隐藏预览' : '显示预览' }}
            </el-button>
          </el-button-group>

          <!-- 文件操作按钮 -->
          <el-button-group style="margin-left: 8px;">
            <el-dropdown size="small" @command="handleFileCommand" trigger="click" aria-label="文件操作">
              <el-button size="small" aria-label="新建或导入文档">
                <el-icon><Document /></el-icon>
                文件
              </el-button>
              <template #dropdown>
                <el-dropdown-menu aria-label="文件操作选项">
                  <el-dropdown-item command="new">
                    <span style="display: flex; align-items: center; gap: 8px;">
                      <el-icon><DocumentAdd /></el-icon>
                      <span>从模板新建...</span>
                    </span>
                  </el-dropdown-item>
                  <el-dropdown-item command="blank" divided>
                    <span style="display: flex; align-items: center; gap: 8px;">
                      <el-icon><Plus /></el-icon>
                      <span>空白文档</span>
                    </span>
                  </el-dropdown-item>
                </el-dropdown-menu>
              </template>
            </el-dropdown>
          </el-button-group>

          <!-- 布局预设按钮 -->
          <el-dropdown size="small" @command="setLayoutMode" trigger="click" aria-label="布局模式">
            <el-button size="small" aria-label="切换布局模式">
              <el-icon><Operation /></el-icon>
              布局
            </el-button>
            <template #dropdown>
              <el-dropdown-menu aria-label="布局选项">
                <el-dropdown-item command="side-by-side" :class="{ 'is-active': layoutMode === 'side-by-side' }">
                  <span class="layout-option">
                    <span class="layout-icon">⬌</span>
                    <span>左右分屏</span>
                  </span>
                </el-dropdown-item>
                <el-dropdown-item
                  command="vertical-split"
                  :class="{ 'is-active': layoutMode === 'vertical-split' }"
                  :aria-selected="layoutMode === 'vertical-split'"
                  role="menuitemradio"
                >
                  <span class="layout-option">
                    <span class="layout-icon" aria-hidden="true">⬍</span>
                    <span>上下分屏</span>
                  </span>
                </el-dropdown-item>
                <el-dropdown-item
                  command="editor-only"
                  :class="{ 'is-active': layoutMode === 'editor-only' }"
                  :aria-selected="layoutMode === 'editor-only'"
                  role="menuitemradio"
                >
                  <span class="layout-option">
                    <span class="layout-icon" aria-hidden="true">📝</span>
                    <span>仅编辑器</span>
                  </span>
                </el-dropdown-item>
                <el-dropdown-item
                  command="preview-only"
                  :class="{ 'is-active': layoutMode === 'preview-only' }"
                  :aria-selected="layoutMode === 'preview-only'"
                  role="menuitemradio"
                >
                  <span class="layout-option">
                    <span class="layout-icon" aria-hidden="true">👁</span>
                    <span>仅预览</span>
                  </span>
                </el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>

          <!-- 项目选择器 -->
          <ProjectSelector @toggle-tree="showProjectTree = $event" />
        </div>
      </nav>

      <!-- 协作用户列表 -->
      <div class="collaboration-users" role="region" aria-label="协作用户" v-if="isCollaborating">
        <el-avatar-group :max="5" aria-label="在线协作者">
          <el-avatar
            v-for="user in activeCollaborationUsers"
            :key="user.id"
            :size="32"
            :style="{ backgroundColor: user.color }"
            :aria-label="`协作者: ${user.name}`"
          >
            {{ user.name.charAt(0).toUpperCase() }}
          </el-avatar>
        </el-avatar-group>
        <span class="collaboration-status" role="status" :aria-label="`${activeCollaborationUsers.length}人正在协作`">
          <el-icon><UserFilled /></el-icon>
          {{ activeCollaborationUsers.length }} 人在线
        </span>
      </div>
    </header>

    <!-- 主编辑区域 -->
    <main class="editor-main" :class="{ 'show-preview': showPreview, 'is-mobile': isMobile }" role="main">
      <!-- 移动端标签栏 -->
      <div class="mobile-tabs" role="tablist" aria-label="编辑器视图切换" v-if="isMobile">
        <div
          class="mobile-tab"
          :class="{ active: mobileActiveTab === 'editor' }"
          @click="mobileActiveTab = 'editor'"
          role="tab"
          :aria-selected="mobileActiveTab === 'editor'"
          :aria-controls="mobileActiveTab === 'editor' ? 'editor-panel' : undefined"
          tabindex="0"
          @keydown.enter="mobileActiveTab = 'editor'"
          @keydown.space.prevent="mobileActiveTab = 'editor'"
        >
          <el-icon><Edit /></el-icon>
          <span>编辑</span>
        </div>
        <div
          class="mobile-tab"
          :class="{ active: mobileActiveTab === 'preview' }"
          @click="mobileActiveTab = 'preview'"
          role="tab"
          :aria-selected="mobileActiveTab === 'preview'"
          :aria-controls="mobileActiveTab === 'preview' ? 'preview-panel' : undefined"
          tabindex="0"
          @keydown.enter="mobileActiveTab = 'preview'"
          @keydown.space.prevent="mobileActiveTab = 'preview'"
        >
          <el-icon><View /></el-icon>
          <span>预览</span>
          <el-tag v-if="compilationStatus === 'success'" type="success" size="small" class="status-badge" aria-label="编译成功">✓</el-tag>
          <el-tag v-else-if="compilationStatus === 'compiling'" type="info" size="small" class="status-badge" aria-label="正在编译">
            <el-icon class="is-loading"><Loading /></el-icon>
          </el-tag>
        </div>
      </div>

      <!-- 左侧：编辑器面板 -->
      <section
        id="editor-panel"
        ref="editorPanelRef"
        class="editor-panel"
        role="region"
        aria-label="LaTeX 编辑器"
        :class="{
          'with-preview': showPreview && !isMobile,
          'mobile-hidden': isMobile && mobileActiveTab !== 'editor',
          'editor-only': layoutMode === 'editor-only',
          'side-by-side': layoutMode === 'side-by-side' && !isMobile,
          'vertical-split': layoutMode === 'vertical-split' && !isMobile
        }"
        :style="layoutMode === 'side-by-side' && !isMobile && showPreview ? { flex: `0 0 ${editorPanelWidth}%` } : {}"
      >
        <div class="editor-content" :class="{ 'with-outline': showOutline }">
          <!-- 文档大纲 -->
          <aside class="document-outline" role="complementary" aria-label="文档结构大纲" v-if="showOutline">
            <div class="outline-header">
              <h3 id="outline-title">文档大纲</h3>
              <el-button size="small" @click="showOutline = false" aria-label="关闭大纲">
                <el-icon><Close /></el-icon>
              </el-button>
            </div>
            <div class="outline-content" role="tree" aria-labelledby="outline-title">
              <DocumentOutline
                :content="editorContent"
                @navigate="navigateToSection"
              />
            </div>
          </aside>

          <!-- 项目文件树 -->
          <aside class="project-file-tree-panel" role="complementary" aria-label="项目文件树" v-if="showProjectTree && isProjectMode">
            <div class="tree-header">
              <h3 id="tree-title">项目文件</h3>
              <el-button size="small" @click="showProjectTree = false" aria-label="关闭文件树">
                <el-icon><Close /></el-icon>
              </el-button>
            </div>
            <div class="tree-content" role="tree" aria-labelledby="tree-title">
              <ProjectFileTree
                v-if="latexStore.currentProject"
                :project-id="latexStore.currentProject.id"
                :project-name="latexStore.currentProject.name"
                :files="latexStore.projectFiles"
                :main-file-path="latexStore.currentProject.mainFile"
                @file-select="handleFileSelect"
                @file-create="handleFileCreate"
                @file-delete="handleFileDelete"
                @file-rename="handleFileRename"
                @main-file-change="handleMainFileChange"
                @refresh="handleRefreshProject"
              />
            </div>
          </aside>

          <!-- 编辑器区域 -->
          <div class="editor-area" :class="{ 'with-outline': showOutline }">
            <!-- 编辑器工具栏 -->
            <div class="editor-toolbar">
          <el-button-group>
            <el-button size="small" @click="toggleOutline">
              <el-icon><Menu /></el-icon>
              大纲
            </el-button>
            <el-tooltip content="撤销 (Ctrl+Z)" placement="top">
              <el-button size="small" @click="undoRedo.undo()" :disabled="!undoRedo.canUndo.value">
                <el-icon><RefreshLeft /></el-icon>
              </el-button>
            </el-tooltip>
            <el-tooltip content="重做 (Ctrl+Shift+Z)" placement="top">
              <el-button size="small" @click="undoRedo.redo()" :disabled="!undoRedo.canRedo.value">
                <el-icon><RefreshRight /></el-icon>
              </el-button>
            </el-tooltip>
            <el-divider direction="vertical" />
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
            <el-tooltip content="代码片段 (Ctrl+Space)" placement="top">
              <el-button size="small" @click="showSnippets = !showSnippets" :aria-pressed="showSnippets">
                <el-icon><Collection /></el-icon>
                片段
              </el-button>
            </el-tooltip>
            <el-tooltip content="快捷键 (?) " placement="top">
              <el-button size="small" @click="showKeyboardShortcuts = true">
                <el-icon><QuestionFilled /></el-icon>
              </el-button>
            </el-tooltip>
            <el-divider direction="vertical" />
            <el-tooltip content="查找替换 (Ctrl+F)" placement="top">
              <el-button size="small" @click="showFindReplace = !showFindReplace" :aria-pressed="showFindReplace">
                <el-icon><Search /></el-icon>
                查找
              </el-button>
            </el-tooltip>
          </el-button-group>

          <!-- 查找替换面板 -->
          <transition name="el-zoom-in-top">
            <div class="find-replace-panel" v-if="showFindReplace" v-click-outside="() => showFindReplace = false">
              <div class="find-replace-row">
                <el-input
                  v-model="findQuery"
                  placeholder="查找..."
                  size="small"
                  clearable
                  @input="onFindInput"
                  @keydown.enter="findNext"
                  @keydown.shift.enter="findPrevious"
                  ref="findInputRef"
                >
                  <template #prefix>
                    <el-icon><Search /></el-icon>
                  </template>
                  <template #suffix>
                    <span class="match-count" v-if="findQuery">{{ currentMatchIndex }}/{{ totalMatches }}</span>
                  </template>
                </el-input>
                <el-button-group size="small">
                  <el-button @click="findPrevious" :disabled="!findQuery || totalMatches === 0" title="上一个 (Enter)">
                    <el-icon><ArrowUp /></el-icon>
                  </el-button>
                  <el-button @click="findNext" :disabled="!findQuery || totalMatches === 0" title="下一个 (Shift+Enter)">
                    <el-icon><ArrowDown /></el-icon>
                  </el-button>
                </el-button-group>
              </div>
              <div class="find-replace-row" v-if="showReplace">
                <el-input
                  v-model="replaceQuery"
                  placeholder="替换为..."
                  size="small"
                  clearable
                  @keydown.enter="replaceCurrent"
                >
                  <template #prefix>
                    <el-icon><RefreshRight /></el-icon>
                  </template>
                </el-input>
                <el-button-group size="small">
                  <el-button @click="replaceCurrent" :disabled="!findQuery || totalMatches === 0" title="替换当前">
                    替换
                  </el-button>
                  <el-button @click="replaceAll" :disabled="!findQuery || totalMatches === 0" title="全部替换">
                    全部替换
                  </el-button>
                </el-button-group>
              </div>
              <div class="find-replace-options">
                <el-checkbox v-model="findOptions.caseSensitive" size="small">区分大小写</el-checkbox>
                <el-checkbox v-model="findOptions.wholeWord" size="small">全字匹配</el-checkbox>
                <el-checkbox v-model="findOptions.useRegex" size="small">正则表达式</el-checkbox>
                <el-link @click="showReplace = !showReplace" type="primary" style="margin-left: auto">
                  {{ showReplace ? '隐藏替换' : '显示替换' }}
                </el-link>
              </div>
            </div>
          </transition>

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
        <div class="editor-wrapper" ref="editorScrollElement">
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
            <!-- 自动保存状态 -->
            <span v-if="autoSave.isSaving.value" class="auto-save-saving">
              <el-icon class="is-loading"><Loading /></el-icon>
              保存中...
            </span>
            <span v-else-if="autoSave.hasUnsavedChanges.value" class="auto-save-unsaved">
              未保存
            </span>
            <span v-else-if="autoSave.lastSavedAt.value" class="auto-save-saved" :title="formatAutoSaveTime(autoSave.lastSavedAt.value)">
              已保存于 {{ formatAutoSaveTime(autoSave.lastSavedAt.value) }}
            </span>
            <span v-else class="auto-save-ready">
              自动保存就绪
            </span>

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
    </section>

      <!-- 面板调整器 (仅在分屏模式显示) -->
      <div
        v-if="showPreview && !isMobile && layoutMode === 'side-by-side'"
        class="panel-resizer"
        :class="{ 'is-resizing': isResizing }"
        @mousedown="startResize"
        @dblclick="resetSplit"
        role="separator"
        aria-orientation="vertical"
        aria-label="调整编辑器和预览面板大小"
      >
        <div class="resizer-handle"></div>
      </div>

      <!-- 右侧：预览面板 -->
      <div
        ref="previewPanelRef"
        class="preview-panel"
        :class="{
          'preview-only': layoutMode === 'preview-only',
          'side-by-side': layoutMode === 'side-by-side' && !isMobile,
          'vertical-split': layoutMode === 'vertical-split' && !isMobile
        }"
        v-if="showPreview && !isMobile"
        :style="layoutMode === 'side-by-side' && !isMobile ? { flex: `0 0 ${100 - editorPanelWidth}%` } : {}"
      >
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

        <div class="preview-content" ref="previewScrollElement">
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
    </main>

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

    <!-- 快捷键面板 -->
    <el-dialog
      v-model="showKeyboardShortcuts"
      title="键盘快捷键"
      width="600px"
      :close-on-click-modal="true"
    >
      <div class="keyboard-shortcuts-content">
        <div class="shortcut-category" v-for="category in shortcutCategories" :key="category.name">
          <h4 class="category-title">{{ category.name }}</h4>
          <div class="shortcut-list">
            <div class="shortcut-item" v-for="shortcut in category.shortcuts" :key="shortcut.action">
              <div class="shortcut-action">{{ shortcut.action }}</div>
              <div class="shortcut-keys">
                <kbd v-for="key in shortcut.keys" :key="key">{{ key }}</kbd>
              </div>
            </div>
          </div>
        </div>
      </div>
      <template #footer>
        <el-button @click="showKeyboardShortcuts = false">关闭</el-button>
      </template>
    </el-dialog>

    <!-- 模板选择面板 -->
    <el-dialog
      v-model="showTemplates"
      title="选择LaTeX模板"
      width="900px"
      :close-on-click-modal="true"
    >
      <h2 id="latex-templates-title" class="sr-only">选择LaTeX模板</h2>
      <div class="templates-content">
        <div class="templates-categories">
          <div
            v-for="category in templateCategories"
            :key="category.name"
            class="category-tab"
            :class="{ active: selectedCategory === category.name }"
            @click="selectedCategory = category.name"
          >
            {{ category.name }} ({{ category.count }})
          </div>
        </div>

        <div class="templates-grid">
          <div
            v-for="template in filteredTemplates"
            :key="template.id"
            class="template-card"
            @click="applyTemplate(template)"
          >
            <div class="template-icon">{{ template.icon }}</div>
            <div class="template-info">
              <div class="template-name">{{ template.name }}</div>
              <div class="template-description">{{ template.description }}</div>
            </div>
            <el-icon class="template-arrow"><ArrowRight /></el-icon>
          </div>
        </div>
      </div>
      <template #footer>
        <el-button @click="showTemplates = false">取消</el-button>
      </template>
    </el-dialog>

    <!-- 代码片段面板 -->
    <el-dialog
      v-model="showSnippets"
      title="LaTeX代码片段"
      width="800px"
      :close-on-click-modal="true"
    >
      <LatexSnippets @insert="handleSnippetInsert" />
    </el-dialog>

    <!-- LaTeX自动补全 -->
    <LatexAutocomplete
      :is-visible="autocomplete.isVisible.value"
      :position="autocomplete.position.value"
      :query="autocomplete.query.value"
      :selected-index="autocomplete.selectedIndex.value"
      :filtered-options="autocomplete.filteredOptions.value"
      @select="() => autocomplete.selectCurrent()"
      @hover="(index) => autocomplete.selectedIndex.value = index"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch, nextTick } from 'vue'
import { useLatexEditorStore } from '@/architecture/stores/latexEditor'
import { useAutoSave } from '@/composables/useAutoSave'
import { useKeyboardShortcuts, getLatexShortcuts } from '@/composables/useKeyboardShortcuts'
import { useScrollSync } from '@/composables/useScrollSync'
import { useUndoRedo } from '@/composables/useUndoRedo'
import { useLatexAutocomplete } from '@/composables/useLatexAutocomplete'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  DocumentChecked, VideoPlay, View, UserFilled, Menu, Plus,
  Tickets, Loading, Warning, InfoFilled, Close,
  ZoomIn, ZoomOut, Edit, RefreshLeft, RefreshRight, Operation, QuestionFilled,
  Search, ArrowUp, ArrowDown, Document, DocumentAdd, Collection,
  FolderOpened, FolderAdd
} from '@element-plus/icons-vue'
import LatexPreview from '@/components/latex/LatexPreview.vue'
import LatexAutocomplete from '@/components/latex/LatexAutocomplete.vue'
import LatexEditor from '@/components/latex/LatexEditor.vue'
import DocumentOutline from '@/components/latex/DocumentOutline.vue'
import SymbolPalette from '@/components/latex/SymbolPalette.vue'
import LatexSnippets from '@/components/latex/LatexSnippets.vue'
import CollaborationPanel from '@/components/collaboration/CollaborationPanel.vue'
import ProjectFileTree from '@/components/latex/ProjectFileTree.vue'
import ProjectSelector from '@/components/latex/ProjectSelector.vue'
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
const showSnippets = ref(false)
const showCollaborationPanel = ref(false)
const showKeyboardShortcuts = ref(false) // 新增：快捷键面板
const showProjectTree = ref(false) // 新增：项目文件树
const isProjectMode = ref(false) // 新增：项目模式
const saving = ref(false)
const compiling = ref(false)
const activeErrorTab = ref('errors')
const documentId = ref<string | null>(props.documentId || null)
const mobileActiveTab = ref<'editor' | 'preview'>('editor')
const isMobile = ref(false)
const cursorPosition = ref({ line: 1, column: 1 })

// ==========================================
// Layout state - 可调整布局
// ==========================================
type LayoutMode = 'side-by-side' | 'editor-only' | 'preview-only' | 'vertical-split'
const layoutMode = ref<LayoutMode>('side-by-side')
const isResizing = ref(false)
const editorPanelWidth = ref(50) // Percentage (0-100)

// Panel refs
const editorPanelRef = ref<HTMLElement | null>(null)
const previewPanelRef = ref<HTMLElement | null>(null)

// ==========================================
// Find & Replace state
// ==========================================
const showFindReplace = ref(false)
const showReplace = ref(false)
const showTemplates = ref(false)
const findQuery = ref('')
const replaceQuery = ref('')
const currentMatchIndex = ref(0)
const totalMatches = ref(0)
const findInputRef = ref<any>(null)
const matches = ref<Array<{ start: number; end: number; text: string }>>([])

interface FindOptions {
  caseSensitive: boolean
  wholeWord: boolean
  useRegex: boolean
}

const findOptions = reactive<FindOptions>({
  caseSensitive: false,
  wholeWord: false,
  useRegex: false
})

// Store computed properties - 必须先定义这些，因为后面的 hooks 需要使用
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

// ==========================================
// Keyboard Shortcuts Data
// ==========================================

interface Shortcut {
  action: string
  keys: string[]
}

interface ShortcutCategory {
  name: string
  shortcuts: Shortcut[]
}

const shortcutCategories: ShortcutCategory[] = [
  {
    name: '文件操作',
    shortcuts: [
      { action: '保存文档', keys: ['Ctrl', 'S'] },
      { action: '编译文档', keys: ['Ctrl', 'Enter'] },
    ]
  },
  {
    name: '编辑操作',
    shortcuts: [
      { action: '撤销', keys: ['Ctrl', 'Z'] },
      { action: '重做', keys: ['Ctrl', 'Shift', 'Z'] },
      { action: '查找', keys: ['Ctrl', 'F'] },
      { action: '查找下一个', keys: ['F3'] },
      { action: '查找上一个', keys: ['Shift', 'F3'] },
      { action: '替换', keys: ['Ctrl', 'H'] },
      { action: '跳转到行', keys: ['Ctrl', 'G'] },
    ]
  },
  {
    name: '格式化',
    shortcuts: [
      { action: '粗体', keys: ['Ctrl', 'B'] },
      { action: '斜体', keys: ['Ctrl', 'I'] },
      { action: '下划线', keys: ['Ctrl', 'U'] },
    ]
  },
  {
    name: '视图控制',
    shortcuts: [
      { action: '切换预览', keys: ['Ctrl', '\\'] },
      { action: '切换大纲', keys: ['Ctrl', 'O'] },
      { action: '放大预览', keys: ['Ctrl', '+'] },
      { action: '缩小预览', keys: ['Ctrl', '-'] },
    ]
  },
  {
    name: '面板',
    shortcuts: [
      { action: '快捷键帮助', keys: ['?'] },
      { action: '符号面板', keys: ['Ctrl', 'Shift', 'S'] },
      { action: '代码片段', keys: ['Ctrl', 'Space'] },
    ]
  }
]

// ==========================================
// LaTeX 模板数据
// ==========================================

interface Template {
  id: string
  name: string
  description: string
  category: string
  content: string
  icon: string
}

const latexTemplates: Template[] = [
  {
    id: 'article',
    name: '学术论文',
    description: '标准学术论文模板',
    category: '学术论文',
    icon: '📄',
    content: `\\documentclass[12pt,a4paper]{article}

% 导言区
\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}
\\usepackage{amsmath,amsfonts,amssymb,amsthm}
\\usepackage{graphicx}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

% 标题信息
\\title{论文标题}
\\author{作者姓名}
\\date{\\today}

\\begin{document}

\\maketitle

\\begin{abstract}
  这里是摘要内容。
\\end{abstract}

\\section{引言}
这里是引言内容...

\\section{方法}
这里是方法部分...

\\section{结果}
这里是结果部分...

\\section{结论}
这里是结论部分...

\\begin{thebibliography}{9}
  \\bibitem{文献1}
  \\bibitem{文献2}
\\end{thebibliography}

\\end{document}`
  },
  {
    id: 'report',
    name: '技术报告',
    description: '技术/工程报告模板',
    category: '学术报告',
    icon: '📋',
    content: `\\documentclass[12pt,a4paper]{report}

\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}
\\usepackage{amsmath,amsfonts,amssymb}
\\usepackage{graphicx}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

\\title{技术报告标题}
\\author{作者姓名}
\\date{\\today}

\\begin{document}

\\maketitle

\\tableofcontents

\\chapter{介绍}
这里是介绍内容...

\\chapter{背景}
这里是背景内容...

\\chapter{方法}
这里是方法部分...

\\chapter{结果}
这里是结果部分...

\\chapter{结论}
这里是结论部分...

\\end{document}`
  },
  {
    id: 'beamer',
    name: '演示文稿',
    description: 'Beamer演示文稿模板',
    category: '演示文稿',
    icon: '📊',
    content: `\\documentclass{beamer}

\\usetheme{Madrid}
\\usecolortheme{default}

\\title{演示文稿标题}
\\author{作者姓名}
\\date{\\today}

\\begin{document}

\\frame{\\titlepage}

\\begin{frame}
  \\frametitle{目录}
  \\tableofcontents
\\end{frame}

\\section{第一部分}

\\begin{frame}
  \\frametitle{第一张幻灯片}
  \\begin{itemize}
    \\item 要点1
    \\item 要点2
    \\item 要点3
  \\end{itemize}
\\end{frame}

\\end{document}`
  },
  {
    id: 'book',
    name: '书籍',
    description: '书籍/教材模板',
    category: '书籍',
    icon: '📚',
    content: `\\documentclass[12pt,a4paper]{book}

\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}
\\usepackage{amsmath,amsfonts,amssymb,amsthm}
\\usepackage{graphicx}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

\\title{书籍标题}
\\author{作者姓名}
\\date{\\today}

\\begin{document}

\\frontmatter
\\maketitle

\\tableofcontents

\\mainmatter
\\chapter{第一章}
这里是第一章内容...

\\chapter{第二章}
这里是第二章内容...

\\backmatter
\\begin{thebibliography}{9}
  \\bibitem{文献1}
  \\bibitem{文献2}
\\end{thebibliography}

\\end{document}`
  },
  {
    id: 'letter',
    name: '信函',
    description: '正式信函模板',
    category: '信函',
    icon: '✉️',
    content: `\\documentclass[12pt]{letter}

\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}

\\signature{发件人姓名}
\\address{发件人地址}
\\date{\\today}

\\begin{document}

\\begin{letter}{收件人姓名}
  这里是信件正文...

  \\vspace{1cm}
  此致

  敬礼
\\end{letter}

\\end{document}`
  },
  {
    id: 'memo',
    name: '备忘录',
    description: '内部备忘录模板',
    category: '办公',
    icon: '📝',
    content: `\\documentclass[12pt,a4paper]{article}

\\usepackage[utf8]{inputenc}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

\\title{备忘录}
\\author{部门名称}
\\date{\\today}

\\begin{document}

\\section*{主题}
这里是主题内容...

\\section*{内容}
这里是详细内容...

\\section*{行动项}
\\begin{itemize}
  \\item 行动项1
  \\item 行动项2
\\end{itemize}

\\end{document}`
  },
  {
    id: 'resume',
    name: '简历',
    description: '简历/CV模板',
    category: '个人',
    icon: '👤',
    content: `\\documentclass[12pt,a4paper]{article}

\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}
\\usepackage{enumitem}

\\begin{document}

\\begin{center}
  {\\LARGE \\textbf{姓名}} \\\
  \\vspace{0.3cm}
  联系邮箱 | 电话号码 | 网站
\\end{center}

\\vspace{1cm}

\\section*{教育背景}
\\begin{itemize}
  \\item 学位 - 学校名称 (年份)
  \\item 学位 - 学校名称 (年份)
\\end{itemize}

\\section*{工作经历}
\\begin{itemize}
  \\item 职位 - 公司名称 (年份 - 至今)
  \\item 职位 - 公司名称 (年份 - 年份)
\\end{itemize}

\\section*{技能}
\\begin{itemize}
  \\item 技能1
  \\item 技能2
  \\item 技能3
\\end{itemize}

\\end{document}`
  },
  {
    id: 'notes',
    name: '课程笔记',
    description: '课程笔记模板',
    category: '教育',
    icon: '📖',
    content: `\\documentclass[12pt,a4paper]{article}

\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}
\\usepackage{amsmath,amsfonts,amssymb}
\\usepackage{graphicx}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

\\title{课程名称}
\\author{学生姓名}
\\date{学期}

\\begin{document}

\\maketitle

\\tableofcontents

\\section{第一讲：讲义标题}
这里是课程内容...

\\subsection{要点1}
详细说明...

\\subsection{要点2}
详细说明...

\\section{第二讲：讲义标题}
这里是课程内容...

\\end{document}`
  }
]

// ==========================================
// 模板系统方法和计算属性
// ==========================================

// 选中的模板分类
const selectedCategory = ref<string>('全部')

// 计算模板分类
const templateCategories = computed(() => {
  const categories = [{ name: '全部', count: latexTemplates.length }]
  const categoryMap = new Map<string, number>()

  latexTemplates.forEach(template => {
    const count = categoryMap.get(template.category) || 0
    categoryMap.set(template.category, count + 1)
  })

  categoryMap.forEach((count, name) => {
    categories.push({ name, count })
  })

  return categories
})

// 过滤模板
const filteredTemplates = computed(() => {
  if (selectedCategory.value === '全部') {
    return latexTemplates
  }
  return latexTemplates.filter(t => t.category === selectedCategory.value)
})

// 处理文件命令
function handleFileCommand(command: string) {
  switch (command) {
    case 'new':
      showTemplates.value = true
      break
    case 'blank':
      createBlankDocument()
      break
  }
}

// 创建空白文档
function createBlankDocument() {
  const blankContent = `\\documentclass[12pt,a4paper]{article}

\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}
\\usepackage{amsmath,amsfonts,amssymb}
\\usepackage{graphicx}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

\\title{文档标题}
\\author{作者姓名}
\\date{\\today}

\\begin{document}

\\section{引言}
在这里开始编写您的文档...

\\end{document}
`

  editorContent.value = blankContent
  ElMessage.success('已创建空白文档')
}

// 应用模板
function applyTemplate(template: Template) {
  ElMessageBox.confirm(
    `确定要使用"${template.name}"模板吗？当前内容将被替换。`,
    '应用模板',
    {
      confirmButtonText: '应用',
      cancelButtonText: '取消',
      type: 'warning'
    }
  ).then(() => {
    editorContent.value = template.content
    isModified.value = true
    showTemplates.value = false
    ElMessage.success(`已应用"${template.name}"模板`)
  }).catch(() => {
    // 用户取消
  })
}

// ==========================================
// Snippets handler
// ==========================================

interface Snippet {
  id: string
  name: string
  description: string
  category: string
  content: string
  preview: string
}

function handleSnippetInsert(snippet: Snippet) {
  // Insert snippet content at cursor position
  const currentContent = editorContent.value || ''
  editorContent.value = currentContent + '\n\n' + snippet.content
  isModified.value = true
  ElMessage.success(`已插入"${snippet.name}"片段`)
}

// 自动保存设置
const AUTO_SAVE_INTERVAL = 30000 // 30秒
const autoSaveEnabled = ref(true)

// 设置自动保存 - 现在 editorContent 已经定义
const autoSave = useAutoSave(
  editorContent,
  documentId,
  {
    interval: AUTO_SAVE_INTERVAL,
    debounceDelay: 2000,
    enableLocalStorage: true,
    onSave: async (_content) => {
      await latexStore.saveDocument()
    },
    onSuccess: () => {
      if (import.meta.env.DEV) {
        console.log('Auto-save successful')
      }
      // 显示保存成功提示（可选）
      // ElMessage.success('自动保存成功')
    },
    onError: (error) => {
      console.error('Auto-save failed:', error)
      ElMessage.error('自动保存失败: ' + error.message)
    }
  }
)

// 设置键盘快捷键
const shortcuts = getLatexShortcuts({
  onSave: () => {
    saveDocument()
  },
  onCompile: () => {
    compileDocument()
  },
  onBold: () => {
    insertLatexCommand('textbf')
  },
  onItalic: () => {
    insertLatexCommand('textit')
  },
  onUnderline: () => {
    insertLatexCommand('underline')
  },
  onUndo: () => {
    if (undoRedo.canUndo.value) {
      undoRedo.undo()
      ElMessage.success('已撤销')
    }
  },
  onRedo: () => {
    if (undoRedo.canRedo.value) {
      undoRedo.redo()
      ElMessage.success('已重做')
    }
  },
  onFind: () => {
    showFindReplace.value = true
    showReplace.value = false
    nextTick(() => {
      const findInput = findInputRef.value?.$el?.querySelector('input')
      if (findInput) {
        findInput.focus()
      }
    })
  },
  onReplace: () => {
    showFindReplace.value = true
    showReplace.value = true
    nextTick(() => {
      const findInput = findInputRef.value?.$el?.querySelector('input')
      if (findInput) {
        findInput.focus()
      }
    })
  },
  onGoToLine: () => {
    ElMessage.info('跳转功能即将推出')
  },
  onTogglePreview: () => {
    showPreview.value = !showPreview.value
  },
  onToggleOutline: () => {
    toggleOutline()
  },
  onToggleSnippets: () => {
    showSnippets.value = !showSnippets.value
  }
})

useKeyboardShortcuts(shortcuts, {
  enabled: true
})

// 设置滚动同步 - Use computed refs to access the actual scrollable elements
const editorScrollElement = computed(() => editorRef.value?.$el?.querySelector('textarea') as HTMLElement | undefined)
const previewScrollElement = ref<HTMLElement>()

void useScrollSync({
  editorElement: editorScrollElement,
  previewElement: previewScrollElement,
  enabled: ref(true),
  direction: 'bi-directional'
})

// 设置撤销/重做
const undoRedo = useUndoRedo(
  editorContent,
  cursorPosition,
  {
    maxSize: 100,
    enabled: ref(true)
  }
)

// 设置自动补全
const autocomplete = useLatexAutocomplete({
  editorElement: computed(() => editorRef.value?.$el?.querySelector('textarea')),
  content: editorContent,
  onSelect: (option) => {
    const textarea = editorRef.value?.$el?.querySelector('textarea') as HTMLTextAreaElement
    if (!textarea) return

    const start = textarea.selectionStart
    const text = editorContent.value

    // 找到 \ 的位置
    const backslashIndex = text.lastIndexOf('\\', start)
    if (backslashIndex !== -1) {
      const before = text.substring(0, backslashIndex)
      const after = text.substring(start)
      editorContent.value = before + option.value + after

      // 将光标移到 $1 位置
      nextTick(() => {
        const newPosition = backslashIndex + option.value.indexOf('$1')
        if (newPosition !== -1) {
          textarea.focus()
          textarea.selectionStart = textarea.selectionEnd = newPosition
        }
      })
    }
  },
  enabled: ref(true)
})

// Simple editor focus method
const editorFocus = () => {
  // Focus functionality can be added to LatexEditor component if needed
}

// Methods
function saveDocument() {
  if (import.meta.env.DEV) {
    console.log('Save document clicked')
  }
  saving.value = true
  try {
    latexStore.saveDocument()
    isModified.value = false
    if (import.meta.env.DEV) {
      console.log('Document saved successfully')
    }
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
  if (!editorRef.value) return

  const commands: Record<string, [string, string]> = {
    textbf: ['\\textbf{', '}'],
    textit: ['\\textit{', '}'],
    underline: ['\\underline{', '}'],
    emph: ['\\emph{', '}'],
    texttt: ['\\texttt{', '}'],
    textsf: ['\\textsf{', '}'],
    textsc: ['\\textsc{', '}'],
    textsl: ['\\textsl{', '}'],
    textup: ['\\textup{', '}'],
    textnormal: ['\\textnormal{', '}']
  }

  const [before, after] = commands[command] || ['\\' + command + '{', '}']

  // Get current textarea
  const textarea = editorRef.value.$el?.querySelector('textarea')
  if (!textarea) return

  const start = textarea.selectionStart
  const end = textarea.selectionEnd
  const content = editorContent.value

  const newContent = content.substring(0, start) + before + after + content.substring(end)
  editorContent.value = newContent

  // Focus and set cursor position
  nextTick(() => {
    textarea.focus()
    const newCursorPos = start + before.length
    textarea.selectionStart = newCursorPos
    textarea.selectionEnd = newCursorPos
  })
}

function insertLatexEnvironment(env: string) {
  if (!editorRef.value) return

  const environments: Record<string, [string, string]> = {
    itemize: ['\\begin{itemize}\\n  \\item ', '\\n\\end{itemize}'],
    enumerate: ['\\begin{enumerate}\\n  \\item ', '\\n\\end{enumerate}'],
    equation: ['\\begin{equation}\\n  ', '\\n\\end{equation}'],
    equationstar: ['\\begin{equation*}\\n  ', '\\n\\end{equation*}'],
    align: ['\\begin{align}\\n  ', '\\n\\end{align}'],
    alignstar: ['\\begin{align*}\\n  ', '\\n\\end{align*}'],
    figure: ['\\begin{figure}[h]\\n  \\centering\\n  \\includegraphics[width=0.8\\textwidth]{', '}\\n  \\caption{Caption}\\n  \\label{fig:label}\\n\\end{figure}'],
    table: ['\\begin{table}[h]\\n  \\centering\\n  \\begin{tabular}{', '}\\n    \\hline\\n    % Add your table content here\\n    \\hline\\n  \\end{tabular}\\n  \\caption{Caption}\\n  \\label{tab:label}\\n\\end{table}'],
    center: ['\\begin{center}\\n  ', '\\n\\end{center}'],
    flushleft: ['\\begin{flushleft}\\n  ', '\\n\\end{flushleft}'],
    flushright: ['\\begin{flushright}\\n  ', '\\n\\end{flushright}']
  }

  const [before, after] = environments[env] || ['\\begin{' + env + '}\\n  ', '\\n\\end{' + env + '}']

  // Get current textarea
  const textarea = editorRef.value.$el?.querySelector('textarea')
  if (!textarea) return

  const start = textarea.selectionStart
  const end = textarea.selectionEnd
  const content = editorContent.value

  const newContent = content.substring(0, start) + before + after + content.substring(end)
  editorContent.value = newContent

  // Focus and set cursor position
  nextTick(() => {
    textarea.focus()
    const newCursorPos = start + before.length
    textarea.selectionStart = newCursorPos
    textarea.selectionEnd = newCursorPos
  })
}

function insertSymbol(symbol: string) {
  if (!editorRef.value) return

  if (import.meta.env.DEV) {
    console.log('Insert symbol:', symbol)
  }

  // Get current textarea
  const textarea = editorRef.value.$el?.querySelector('textarea')
  if (!textarea) return

  const start = textarea.selectionStart
  const end = textarea.selectionEnd
  const content = editorContent.value

  const newContent = content.substring(0, start) + symbol + content.substring(end)
  editorContent.value = newContent

  // Focus and set cursor position
  nextTick(() => {
    textarea.focus()
    const newCursorPos = start + symbol.length
    textarea.selectionStart = newCursorPos
    textarea.selectionEnd = newCursorPos
  })

  showSymbolPalette.value = false
}

function toggleOutline() {
  if (import.meta.env.DEV) {
    console.log('Toggle outline clicked, current state:', showOutline.value)
  }
  showOutline.value = !showOutline.value
  if (import.meta.env.DEV) {
    console.log('New outline state:', showOutline.value)
  }
}

function navigateToSection(position: { line: number; column?: number }) {
  // Navigate to specific section/line in the editor
  if (import.meta.env.DEV) {
    console.log('Navigate to section:', position)
  }

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

    if (import.meta.env.DEV) {
      console.log('Navigation successful to line', line, 'column', column)
    }
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
  if (import.meta.env.DEV) {
    console.log('Inviting user:', email)
  }
}

function leaveCollaboration() {
  latexStore.stopCollaboration()
  showCollaborationPanel.value = false
}

// ==========================================
// Layout Management Methods
// ==========================================

// Set layout mode
function setLayoutMode(mode: LayoutMode) {
  layoutMode.value = mode
  // Update showPreview based on mode
  if (mode === 'editor-only') {
    showPreview.value = false
  } else if (mode === 'preview-only' || mode === 'side-by-side' || mode === 'vertical-split') {
    showPreview.value = true
  }
}

// Start panel resizing
function startResize(e: MouseEvent) {
  if (isMobile.value) return // Disable resizing on mobile

  isResizing.value = true
  document.addEventListener('mousemove', onResize)
  document.addEventListener('mouseup', stopResize)

  // Prevent text selection during resize
  e.preventDefault()
  document.body.style.cursor = 'col-resize'
  document.body.style.userSelect = 'none'
}

// Handle panel resizing
function onResize(e: MouseEvent) {
  if (!isResizing.value || !editorPanelRef.value || !previewPanelRef.value) return

  const container = editorPanelRef.value.parentElement
  if (!container) return

  const containerRect = container.getBoundingClientRect()
  const newWidth = ((e.clientX - containerRect.left) / containerRect.width) * 100

  // Clamp width between 20% and 80%
  editorPanelWidth.value = Math.max(20, Math.min(80, newWidth))
}

// Stop panel resizing
function stopResize() {
  if (!isResizing.value) return

  isResizing.value = false
  document.removeEventListener('mousemove', onResize)
  document.removeEventListener('mouseup', stopResize)

  document.body.style.cursor = ''
  document.body.style.userSelect = ''

  // Save to local storage
  try {
    localStorage.setItem('latex-editor-panel-width', editorPanelWidth.value.toString())
  } catch (e) {
    // Ignore storage errors
  }
}

// Double-click resizer to reset to 50-50 split
function resetSplit() {
  editorPanelWidth.value = 50
  try {
    localStorage.setItem('latex-editor-panel-width', '50')
  } catch (e) {
    // Ignore storage errors
  }
}

// ==========================================
// Find & Replace Methods
// ==========================================

function performFind() {
  if (!findQuery.value) {
    matches.value = []
    totalMatches.value = 0
    currentMatchIndex.value = 0
    return
  }

  const content = editorContent.value
  const foundMatches: Array<{ start: number; end: number; text: string }> = []

  let searchPattern: string | RegExp
  try {
    if (findOptions.useRegex) {
      const flags = findOptions.caseSensitive ? 'g' : 'gi'
      searchPattern = new RegExp(findQuery.value, flags)
    } else {
      searchPattern = findQuery.value
    }

    let match: RegExpExecArray | null | string
    if (searchPattern instanceof RegExp) {
      while ((match = searchPattern.exec(content)) !== null) {
        if (match.index !== undefined && match[0]) {
          foundMatches.push({
            start: match.index,
            end: match.index + match[0].length,
            text: match[0]
          })
        }
      }
    } else {
      // Simple string search
      let searchIndex = 0
      const searchContent = findOptions.caseSensitive ? content : content.toLowerCase()
      const searchQuery = findOptions.caseSensitive ? searchPattern : (searchPattern as string).toLowerCase()

      while (true) {
        const index = searchContent.indexOf(searchQuery, searchIndex)
        if (index === -1) break

        const actualText = content.substring(index, index + (searchPattern as string).length)

        // Check for whole word match
        if (findOptions.wholeWord) {
          const before = index > 0 ? content[index - 1] : ' '
          const after = index + (searchPattern as string).length < content.length
            ? content[index + (searchPattern as string).length]
            : ' '
          if (!/\W/.test(before) || !/\W/.test(after)) {
            searchIndex = index + 1
            continue
          }
        }

        foundMatches.push({
          start: index,
          end: index + (searchPattern as string).length,
          text: actualText
        })
        searchIndex = index + (searchPattern as string).length
      }
    }
  } catch (e) {
    // Invalid regex pattern
    console.error('Find error:', e)
  }

  matches.value = foundMatches
  totalMatches.value = foundMatches.length
  currentMatchIndex.value = foundMatches.length > 0 ? 1 : 0
}

function onFindInput() {
  performFind()
}

function findNext() {
  if (matches.value.length === 0) {
    performFind()
    return
  }

  if (currentMatchIndex.value < matches.value.length) {
    highlightMatch(currentMatchIndex.value)
    currentMatchIndex.value++
  } else {
    // Wrap around to first match
    currentMatchIndex.value = 1
    highlightMatch(0)
  }
}

function findPrevious() {
  if (matches.value.length === 0) {
    performFind()
    return
  }

  if (currentMatchIndex.value > 1) {
    currentMatchIndex.value--
    highlightMatch(currentMatchIndex.value - 1)
  } else {
    // Wrap around to last match
    currentMatchIndex.value = matches.value.length
    highlightMatch(matches.value.length - 1)
  }
}

function highlightMatch(matchIndex: number) {
  const match = matches.value[matchIndex]
  if (!match || !editorRef.value) return

  const textarea = editorRef.value.$el?.querySelector('textarea')
  if (!textarea) return

  textarea.focus()
  textarea.setSelectionRange(match.start, match.end)

  // Scroll to the match position
  const textBefore = textarea.value.substring(0, match.start)
  const linesBefore = textBefore.split('\n').length
  const lineHeight = 24 // Approximate line height
  textarea.scrollTop = (linesBefore - 10) * lineHeight
}

function replaceCurrent() {
  if (matches.value.length === 0 || currentMatchIndex.value === 0) return

  const match = matches.value[currentMatchIndex.value - 1]
  if (!match) return

  const content = editorContent.value
  const newContent = content.substring(0, match.start) + replaceQuery.value + content.substring(match.end)
  editorContent.value = newContent
  isModified.value = true

  // Re-find after replace
  performFind()

  // Move to next match
  if (matches.value.length > 0) {
    findNext()
  } else {
    ElMessage.success('已完成替换')
  }
}

function replaceAll() {
  if (matches.value.length === 0) return

  ElMessageBox.confirm(
    `确定要替换所有 ${totalMatches.value} 个匹配项吗？`,
    '全部替换',
    {
      confirmButtonText: '替换',
      cancelButtonText: '取消',
      type: 'warning'
    }
  ).then(() => {
    let content = editorContent.value
    let replaceCount = 0

    // Process matches in reverse order to maintain indices
    for (let i = matches.value.length - 1; i >= 0; i--) {
      const match = matches.value[i]
      content = content.substring(0, match.start) + replaceQuery.value + content.substring(match.end)
      replaceCount++
    }

    editorContent.value = content
    isModified.value = true
    matches.value = []
    totalMatches.value = 0
    currentMatchIndex.value = 0

    ElMessage.success(`已替换 ${replaceCount} 处`)
  }).catch(() => {
    // User cancelled
  })
}

// 格式化自动保存时间
function formatAutoSaveTime(date: Date): string {
  const now = new Date()
  const diff = now.getTime() - date.getTime()
  const seconds = Math.floor(diff / 1000)
  const minutes = Math.floor(seconds / 60)
  const hours = Math.floor(minutes / 60)

  if (seconds < 60) {
    return `${seconds}秒前`
  } else if (minutes < 60) {
    return `${minutes}分钟前`
  } else if (hours < 24) {
    return `${hours}小时前`
  } else {
    return date.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' })
  }
}

// 检测移动端
const checkMobile = () => {
  isMobile.value = window.innerWidth < 768
  if (isMobile.value) {
    mobileActiveTab.value = 'editor'
  }
}

// Lifecycle
onMounted(async () => {
  // 检测是否为移动端
  checkMobile()
  window.addEventListener('resize', checkMobile)

  // Load saved panel width from localStorage
  try {
    const savedWidth = localStorage.getItem('latex-editor-panel-width')
    if (savedWidth) {
      editorPanelWidth.value = parseFloat(savedWidth)
    }
  } catch (e) {
    // Ignore storage errors
  }

  // Keyboard shortcut for shortcuts panel (? key)
  const handleGlobalKeydown = (e: KeyboardEvent) => {
    // Open shortcuts panel with ? key (only when not typing in editor)
    if (e.key === '?' && !e.ctrlKey && !e.metaKey && !e.altKey) {
      const target = e.target as HTMLElement
      const isInput = target.tagName === 'INPUT' ||
                      target.tagName === 'TEXTAREA' ||
                      target.contentEditable === 'true'

      // Only trigger if not in input or if in textarea without Shift modifier
      if (!isInput || (target.tagName === 'TEXTAREA' && !e.shiftKey)) {
        showKeyboardShortcuts.value = true
        e.preventDefault()
      }
    }
  }
  window.addEventListener('keydown', handleGlobalKeydown)

  // Store handler for cleanup
  ;(window as any).__latexEditorKeydownHandler = handleGlobalKeydown

  // Initialize document
  if (props.documentId) {
    // Load existing document
    documentId.value = props.documentId
    if (import.meta.env.DEV) {
      console.log('Loading document:', props.documentId)
    }

    // 检查是否有自动保存的内容
    const restoredContent = autoSave.loadFromLocalStorage()
    if (restoredContent) {
      const shouldRestore = await ElMessageBox.confirm(
        '检测到未保存的自动保存内容，是否恢复？',
        '恢复自动保存',
        {
          confirmButtonText: '恢复',
          cancelButtonText: '放弃',
          type: 'info'
        }
      ).catch(() => false)

      if (shouldRestore) {
        editorContent.value = restoredContent
        ElMessage.success('已恢复自动保存的内容')
      } else {
        autoSave.clearLocalStorage()
      }
    }
  } else {
    // Create new document
    const newDoc = await latexStore.createNewDocument('新建文档.tex')
    documentId.value = newDoc.id
    if (import.meta.env.DEV) {
      console.log('Document created, content length:', editorContent.value?.length)
    }
  }

  // 启动自动保存
  if (autoSaveEnabled.value) {
    autoSave.startAutoSave()
    if (import.meta.env.DEV) {
      console.log('Auto-save started with interval:', AUTO_SAVE_INTERVAL)
    }
  }

  // Watch for theme changes - can be implemented with LatexEditor component
  watch(() => theme.value, (_newTheme) => {
    // Update editor theme if needed
  })

  // Watch for content changes
  watch(() => editorContent.value, (newContent, oldContent) => {
    if (import.meta.env.DEV) {
      console.log('Editor content changed in view:', { newLength: newContent?.length, oldLength: oldContent?.length })
    }
  }, { immediate: true })
})

onUnmounted(() => {
  // 停止自动保存
  autoSave.stopAutoSave()
  latexStore.cleanup()
  window.removeEventListener('resize', checkMobile)

  // Clean up keyboard event listener
  const handler = (window as any).__latexEditorKeydownHandler
  if (handler) {
    window.removeEventListener('keydown', handler)
    delete (window as any).__latexEditorKeydownHandler
  }
})


async function handleFileCreate(fileData: { name: string; path: string; type: string }) {
  try {
    await latexStore.createProjectFile(fileData)
    ElMessage.success('文件创建成功')
  } catch (error: any) {
    ElMessage.error('文件创建失败: ' + (error.message || '未知错误'))
  }
}

async function handleFileDelete(fileId: number | string) {
  try {
    await latexStore.deleteProjectFile(Number(fileId))
    ElMessage.success('文件删除成功')
  } catch (error: any) {
    ElMessage.error('文件删除失败: ' + (error.message || '未知错误'))
  }
}

async function handleFileRename(fileId: number | string, newName: string) {
  ElMessage.info('重命名功能待实现')
}

async function handleMainFileChange(filePath: string) {
  if (!latexStore.currentProject) return

  try {
    const { updateLatexProject } = await import('@/api/adapters/latexAdapter')
    await updateLatexProject(latexStore.currentProject.id, { mainFile: filePath })

    // 刷新项目
    await latexStore.loadProject(latexStore.currentProject.id)

    ElMessage.success('主文件设置成功')
  } catch (error: any) {
    ElMessage.error('设置主文件失败: ' + (error.message || '未知错误'))
  }
}

async function handleRefreshProject() {
  if (!latexStore.currentProject) return

  try {
    await latexStore.loadProject(latexStore.currentProject.id)
    ElMessage.success('项目已刷新')
  } catch (error: any) {
    ElMessage.error('刷新项目失败: ' + (error.message || '未知错误'))
  }
}

// ==========================================
// 项目模式下的编译和保存
// ==========================================

// 重写保存和编译方法以支持项目模式
const originalSaveDocument = saveDocument
const originalCompileDocument = compileDocument

saveDocument = function() {
  if (isProjectMode.value) {
    latexStore.saveCurrentProjectFile()
    isModified.value = false
  } else {
    originalSaveDocument()
  }
}

compileDocument = async function() {
  if (isProjectMode.value) {
    compiling.value = true
    try {
      const result = await latexStore.compileProject()
      if (result.success) {
        if (previewRef.value) {
          previewRef.value.updatePreview(result.output || '')
        }
      }
    } finally {
      compiling.value = false
    }
  } else {
    await originalCompileDocument()
  }
}

// Expose methods to template
defineExpose({
  saveDocument,
  compileDocument,
  focus: editorFocus,
  getContent: () => editorContent.value
})
</script>

<style scoped lang="scss">
// ==========================================
// LaTeX Editor Optimized Styles
// 全套优化：视觉 + 布局 + 交互 + 无障碍
// ==========================================

// 优化变量
$transition-fast: 150ms;
$transition-base: 200ms;
$transition-slow: 300ms;
$easing-out: cubic-bezier(0.4, 0, 0.2, 1);
$shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.05);
$shadow-md: 0 4px 8px rgba(0, 0, 0, 0.1);
$shadow-lg: 0 8px 16px rgba(0, 0, 0, 0.15);

// ==========================================
// 全局样式优化
// ==========================================

.latex-editor-view {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);
  position: relative;
  isolation: isolate; // 创建新的层叠上下文，防止子元素重叠
  overflow: hidden; // 防止内容溢出导致重影

  // 确保所有子元素正确渲染
  > * {
    position: relative;
    z-index: 1;
  }
}

// ==========================================
// 1. 按钮状态优化
// ==========================================

:deep(.el-button) {
  transition: all $transition-base $easing-out;
  position: relative;
  overflow: hidden;

  &:hover:not(:disabled) {
    transform: translateY(-1px);
    box-shadow: $shadow-md;
  }

  &:active:not(:disabled) {
    transform: translateY(0) scale(0.98);
    box-shadow: $shadow-sm;
  }

  &:focus-visible {
    outline: 2px solid var(--el-color-primary);
    outline-offset: 2px;
    border-radius: var(--el-border-radius-base);
  }

  &.is-loading {
    position: relative;
    color: transparent !important;
    pointer-events: none;

    &::after {
      content: '';
      position: absolute;
      inset: 0;
      width: 16px;
      height: 16px;
      top: 50%;
      left: 50%;
      margin-left: -8px;
      margin-top: -8px;
      border: 2px solid currentColor;
      border-right-color: transparent;
      border-radius: 50%;
      animation: spin 0.6s linear infinite;
    }
  }
}

@keyframes spin {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

// ==========================================
// 2. 头部区域优化
// ==========================================

.editor-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);
  background: linear-gradient(
    to bottom,
    var(--el-bg-color-page) 0%,
    rgba(0, 0, 0, 0.02) 100%
  );

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

    :deep(.el-avatar-group .el-avatar) {
      border: 2px solid var(--el-bg-color-page);
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
      transition: all $transition-base $easing-out;

      &:hover {
        transform: translateY(-2px);
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
      }
    }

    .collaboration-status {
      display: flex;
      align-items: center;
      gap: 6px;
      padding: 4px 10px;
      background: var(--el-color-success-light-9);
      color: var(--el-color-success);
      border-radius: 12px;
      font-size: 12px;
      font-weight: 500;
    }
  }
}

// ==========================================
// 3. 面板布局
// ==========================================

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
      padding: 14px 16px;
      border-bottom: 1px solid var(--el-border-color-lighter);

      h3 {
        margin: 0;
        font-size: 14px;
        font-weight: 600;
        color: var(--el-text-color-primary);
      }
    }

    .outline-content {
      padding: 8px 12px;
      height: calc(100% - 50px);
      overflow-y: auto;
    }
  }

  .project-file-tree-panel {
    width: 280px;
    border-right: 1px solid var(--el-border-color-lighter);
    background: var(--el-bg-color-page);
    flex-shrink: 0;
    display: flex;
    flex-direction: column;

    .tree-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 14px 16px;
      border-bottom: 1px solid var(--el-border-color-lighter);

      h3 {
        margin: 0;
        font-size: 14px;
        font-weight: 600;
        color: var(--el-text-color-primary);
      }
    }

    .tree-content {
      flex: 1;
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
    padding: 10px 16px;
    border-bottom: 1px solid var(--el-border-color-lighter);
    background: linear-gradient(
      to bottom,
      var(--el-bg-color-page) 0%,
      rgba(0, 0, 0, 0.02) 100%
    );

    :deep(.el-button-group) {
      position: relative;
      padding: 0 8px;

      &:not(:last-child)::after {
        content: '';
        position: absolute;
        right: 0;
        top: 50%;
        transform: translateY(-50%);
        width: 1px;
        height: 20px;
        background: var(--el-border-color-light);
      }

      &:not(:last-child) {
        margin-right: 4px;
      }
    }

    .compilation-status {
      :deep(.el-tag) {
        display: inline-flex;
        align-items: center;
        gap: 4px;
        padding: 4px 10px;
        border-radius: 12px;
        font-weight: 500;
        font-size: 12px;
        transition: all $transition-base $easing-out;

        &.el-tag--success {
          background: linear-gradient(135deg, #d1fae5 0%, #a7f3d0 100%);
          border-color: #34d399;
          color: #065f46;
        }

        &.el-tag--danger {
          background: linear-gradient(135deg, #fee2e2 0%, #fecaca 100%);
          border-color: #f87171;
          color: #991b1b;
        }

        &.el-tag--warning {
          background: linear-gradient(135deg, #fef3c7 0%, #fde68a 100%);
          border-color: #fbbf24;
          color: #92400e;
        }

        &.el-tag--info {
          background: linear-gradient(135deg, #dbeafe 0%, #bfdbfe 100%);
          border-color: #60a5fa;
          color: #1e40af;
        }

        &.el-tag--compiling {
          animation: pulse 1.5s ease-in-out infinite;
        }
      }
    }
  }

  @keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.7; }
  }

  .editor-wrapper {
    flex: 1;
    display: flex;
    flex-direction: column;
    position: relative;
    overflow: hidden;
    isolation: isolate; // 创建新的层叠上下文

    // GPU 加速
    will-change: transform;
    transform: translateZ(0);
    backface-visibility: hidden;

    // 确保子元素正确渲染
    :deep(*) {
      box-sizing: border-box;
    }
  }

  .editor-status-bar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 6px 16px;
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
      align-items: center;

      .text-warning {
        color: var(--el-color-warning);
      }

      // 自动保存状态样式增强
      .auto-save-saving,
      .auto-save-unsaved,
      .auto-save-saved,
      .auto-save-ready {
        display: flex;
        align-items: center;
        gap: 4px;
        padding: 2px 8px;
        border-radius: 12px;
        font-weight: 500;
        transition: all $transition-base $easing-out;
      }

      .auto-save-saving {
        color: var(--el-color-primary);
        background-color: var(--el-color-primary-light-9);

        .el-icon {
          animation: rotating 2s linear infinite;
        }
      }

      .auto-save-unsaved {
        color: var(--el-color-warning);
        background-color: var(--el-color-warning-light-9);
      }

      .auto-save-saved {
        color: var(--el-color-success);
        background-color: var(--el-color-success-light-9);
      }

      .auto-save-ready {
        color: var(--el-text-color-secondary);
      }
    }
  }

  @keyframes rotating {
    from {
      transform: rotate(0deg);
    }
    to {
      transform: rotate(360deg);
    }
  }
}

// ==========================================
// 面板调整器 (Panel Resizer)
// ==========================================

.panel-resizer {
  position: relative;
  width: 6px;
  background: var(--el-border-color-lighter);
  cursor: col-resize;
  transition: all $transition-base $easing-out;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;

  &:hover {
    background: var(--el-color-primary);
    width: 8px;
  }

  &.is-resizing {
    background: var(--el-color-primary);
    width: 8px;

    .resizer-handle {
      opacity: 1;
    }
  }

  .resizer-handle {
    position: absolute;
    width: 4px;
    height: 40px;
    background: var(--el-color-primary);
    border-radius: 2px;
    opacity: 0;
    transition: opacity $transition-base $easing-out;
  }

  &:hover .resizer-handle {
    opacity: 0.8;
  }
}

// ==========================================
// 布局模式样式
// ==========================================

// 左右分屏模式
.editor-main {
  &:not(.is-mobile) {
    // Side-by-side layout
    .editor-panel.side-by-side {
      border-right: 1px solid var(--el-border-color-lighter);
    }

    .preview-panel.side-by-side {
      border-left: 1px solid var(--el-border-color-lighter);
    }

    // 上下分屏模式
    .editor-panel.vertical-split,
    .preview-panel.vertical-split {
      flex: 1;
      border-right: none;
      border-left: none;
    }

    .editor-panel.vertical-split {
      border-bottom: 1px solid var(--el-border-color-lighter);
    }

    // 仅编辑器模式
    .editor-panel.editor-only {
      flex: 1;
      border-right: none;
    }

    // 仅预览模式
    .preview-panel.preview-only {
      flex: 1;
    }
  }
}

// ==========================================
// 4. 预览面板优化
// ==========================================

.preview-panel {
  flex: 0.4;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);

  .preview-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 14px 18px;
    border-bottom: 1px solid var(--el-border-color-lighter);
    background: linear-gradient(
      to bottom,
      var(--el-bg-color-page) 0%,
      rgba(0, 0, 0, 0.02) 100%
    );

    h3 {
      margin: 0;
      font-size: 15px;
      font-weight: 600;
      color: var(--el-text-color-primary);
      display: flex;
      align-items: center;
      gap: 6px;

      &::before {
        content: '';
        width: 4px;
        height: 16px;
        background: var(--el-color-primary);
        border-radius: 2px;
      }
    }

    .preview-controls {
      display: flex;
      gap: 6px;

      :deep(.el-button) {
        min-width: 32px;
        height: 32px;
        padding: 0 8px;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 14px;
        font-weight: 500;
      }
    }
  }

  .preview-content {
    flex: 1;
    overflow: hidden;

    // GPU 加速
    will-change: transform;
    transform: translateZ(0);
    backface-visibility: hidden;

    // 内容可见性优化
    > * {
      contain: content style layout;
      content-visibility: auto;

      // 淡入动画
      animation: fadeIn 0.3s ease-out;
    }
  }

  @keyframes fadeIn {
    from {
      opacity: 0;
      transform: translateY(8px);
    }
    to {
      opacity: 1;
      transform: translateY(0);
    }
  }

  .error-panel {
    border-top: 1px solid var(--el-border-color-lighter);
    background: var(--el-bg-color-page);

    .error-header {
      padding: 10px 16px 0;
    }

    .error-content {
      max-height: 200px;
      overflow-y: auto;
      padding: 8px 16px;

      .error-item {
        display: flex;
        align-items: flex-start;
        gap: 10px;
        padding: 10px 12px;
        border-bottom: 1px solid var(--el-border-color-lighter);
        cursor: pointer;
        transition: all $transition-base $easing-out;
        border-left: 3px solid transparent;
        border-radius: 4px;

        &:last-child {
          border-bottom: none;
        }

        &:hover {
          background: var(--el-bg-color-overlay);
          transform: translateX(4px);
        }

        &.error {
          border-left-color: var(--el-color-danger);
          background: linear-gradient(90deg, rgba(239, 68, 68, 0.05) 0%, transparent 100%);
        }

        &.warning {
          border-left-color: var(--el-color-warning);
          background: linear-gradient(90deg, rgba(245, 158, 11, 0.05) 0%, transparent 100%);
        }

        .error-line {
          font-weight: 600;
          color: var(--el-text-color-primary);
          min-width: 60px;
          font-size: 12px;
        }

        .error-message {
          flex: 1;
          color: var(--el-text-color-regular);
          font-size: 13px;
          line-height: 1.4;
        }

        .el-icon {
          margin-top: 2px;
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
      grid-template-columns: repeat(auto-fill, minmax(56px, 1fr));
      gap: 10px;
      padding: 16px;

      .symbol-item {
        display: flex;
        align-items: center;
        justify-content: center;
        height: 50px;
        border: 1px solid var(--el-border-color-lighter);
        border-radius: 8px;
        cursor: pointer;
        transition: all $transition-base $easing-out;
        background: var(--el-bg-color);
        position: relative;
        overflow: hidden;

        // 涟纹效果
        &::before {
          content: '';
          position: absolute;
          inset: 0;
          background: radial-gradient(circle, rgba(99, 102, 241, 0.1) 0%, transparent 70%);
          opacity: 0;
          transform: scale(0);
          transition: all $transition-base $easing-out;
        }

        &:hover {
          border-color: var(--el-color-primary);
          transform: translateY(-2px);
          box-shadow: $shadow-md;

          &::before {
            opacity: 1;
            transform: scale(1);
          }
        }

        &:active {
          transform: translateY(0);
          box-shadow: $shadow-sm;
        }

        // 缩放动画
        animation: scaleIn 0.2s ease-out;
      }

      @keyframes scaleIn {
        from {
          opacity: 0;
          transform: scale(0.95);
        }
        to {
          opacity: 1;
          transform: scale(1);
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

        :deep(.el-input) {
          flex: 1;
        }
      }
    }
  }
}

// ==========================================
// 5. 移动端标签栏优化
// ==========================================

.mobile-tabs {
  display: none;
  position: sticky;
  top: 0;
  z-index: 100;
  background: var(--el-bg-color-page);
  border-bottom: 1px solid var(--el-border-color);
  padding: 10px 12px;
  box-shadow: $shadow-sm;

  .mobile-tab {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    padding: 12px;
    border-radius: 10px;
    cursor: pointer;
    transition: all $transition-base $easing-out;
    color: var(--el-text-color-secondary);
    font-weight: 500;
    font-size: 14px;
    min-height: 44px; // 触摸目标最小尺寸
    position: relative;
    overflow: hidden;

    // 涟纹效果
    &::before {
      content: '';
      position: absolute;
      inset: 0;
      background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
      transform: translateX(-100%);
      transition: transform $transition-slow $easing-out;
    }

    &:active::before {
      transform: translateX(100%);
    }

    &.active {
      background: var(--el-color-primary);
      color: white;
      box-shadow: $shadow-md;

      &::after {
        content: '';
        position: absolute;
        bottom: -2px;
        left: 50%;
        transform: translateX(-50%);
        width: 20px;
        height: 3px;
        background: white;
        border-radius: 2px;
      }
    }

    .status-badge {
      margin-left: 4px;
    }
  }
}

.editor-main.is-mobile {
  .mobile-tabs {
    display: flex;
    gap: 8px;
  }
}

// ==========================================
// 6. 响应式设计
// ==========================================

@media (max-width: 768px) {
  .latex-editor-view {
    .editor-header {
      flex-direction: column;
      gap: 8px;
      align-items: stretch;

      .document-info {
        flex-direction: column;
        align-items: stretch;
        gap: 8px;

        .document-actions {
          width: 100%;

          :deep(.el-button-group) {
            display: flex;
            width: 100%;

            .el-button {
              flex: 1;
              min-height: 44px; // 触摸目标最小尺寸
            }
          }
        }
      }
    }

    .editor-toolbar {
      padding: 10px 12px;
      gap: 8px;
      flex-wrap: wrap;

      :deep(.el-button-group) {
        margin: 0 4px;
      }

      // 隐藏工具栏文本以节省空间
      .toolbar-text {
        display: none;
      }
    }

    .editor-status-bar {
      padding: 10px 14px;
      font-size: 11px;

      .status-left,
      .status-right {
        gap: 12px;
      }
    }
  }

  .editor-main {
    flex-direction: column;

    &.is-mobile {
      flex-direction: column;

      .editor-panel {
        flex: 1;
        min-height: 50vh;

        &.mobile-hidden {
          display: none;
        }
      }

      .preview-panel {
        flex: 1;
        min-height: 50vh;

        &.mobile-panel {
          position: relative;
          z-index: 1;
        }
      }
    }

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

  .symbol-palette {
    .symbol-grid {
      grid-template-columns: repeat(auto-fill, minmax(48px, 1fr));
      gap: 8px;
      padding: 12px;

      .symbol-item {
        height: 48px; // 触摸友好的尺寸
        min-height: 48px;
      }
    }
  }
}

// ==========================================
// 7. 深色模式增强
// ==========================================

[data-theme="dark"] {
  .latex-editor-view {
    background: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);

    .editor-header {
      background: rgba(15, 23, 42, 0.8);
      backdrop-filter: blur(20px) saturate(180%);
      border-bottom: 1px solid rgba(148, 163, 184, 0.1);
    }

    .editor-toolbar {
      background: rgba(30, 41, 59, 0.5);

      :deep(.el-button-group) {
        &:not(:last-child)::after {
          background: rgba(148, 163, 184, 0.2);
        }
      }
    }

    .editor-status-bar {
      background: rgba(15, 23, 42, 0.6);
      border-top: 1px solid rgba(148, 163, 184, 0.1);
    }

    .preview-panel {
      background: rgba(15, 23, 42, 0.5);

      .preview-content {
        background: rgba(30, 41, 59, 0.3);
      }
    }

    .document-outline {
      background: rgba(30, 41, 59, 0.5);
      border-right: 1px solid rgba(148, 163, 184, 0.1);
    }

    // 暗色模式下的发光效果
    :deep(.el-button.el-button--primary) {
      box-shadow: 0 8px 32px rgba(99, 102, 241, 0.4);

      &:hover {
        box-shadow: 0 12px 40px rgba(99, 102, 241, 0.5);
      }
    }
  }
}

// ==========================================
// 8. 滚动条美化
// ==========================================

// 自定义滚动条样式
:deep(.editor-textarea),
:deep(.preview-content),
:deep(.document-outline),
:deep(.outline-content),
.error-content {
  &::-webkit-scrollbar {
    width: 8px;
    height: 8px;
  }

  &::-webkit-scrollbar-track {
    background: transparent;
  }

  &::-webkit-scrollbar-thumb {
    background: var(--el-border-color);
    border-radius: 4px;

    &:hover {
      background: var(--el-border-color-dark);
    }
  }

  & {
    scrollbar-width: thin;
    scrollbar-color: var(--el-border-color) transparent;
  }
}

// ==========================================
// 9. 无障碍增强
// ==========================================

// 屏幕阅读器只读内容
.sr-only {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border-width: 0;
}

// 仅在焦点时显示的轮廓
.focus-visible-only {
  &:focus:not(:focus-visible) {
    outline: none;
  }

  &:focus-visible {
    outline: 2px solid var(--el-color-primary);
    outline-offset: 2px;
    border-radius: 4px;
  }
}

// 高对比度模式支持
@media (prefers-contrast: high) {
  :deep(.el-button),
  :deep(.symbol-item),
  :deep(.outline-item) {
    border-width: 2px;
  }

  :deep(.latex-textarea) {
    border-width: 2px;
  }
}

// 减少动画模式支持
@media (prefers-reduced-motion: reduce) {
  *,
  *::before,
  *::after {
    animation-duration: 0.01ms !important;
    animation-iteration-count: 1 !important;
    transition-duration: 0.01ms !important;
  }
}

// ==========================================
// 10. 打印样式
// ==========================================

@media print {
  .latex-editor-view {
    .editor-header,
    .editor-toolbar,
    .editor-status-bar,
    .mobile-tabs {
      display: none !important;
    }

    .editor-panel,
    .preview-panel {
      flex: 1;
      page-break-inside: avoid;
    }
  }
}

// ==========================================
// 11. 成功/错误反馈动画
// ==========================================

// 成功状态动画
@keyframes successPulse {
  0% {
    box-shadow: 0 0 0 0 rgba(82, 196, 26, 0.7);
  }
  50% {
    box-shadow: 0 0 0 8px rgba(82, 196, 26, 0.3);
  }
  100% {
    box-shadow: 0 0 0 0 rgba(82, 196, 26, 0.7);
  }
}

.save-success {
  animation: successPulse 0.6s ease-in-out;
}

// 错误抖动动画
@keyframes shake {
  0%, 100% {
    transform: translateX(0);
  }
  10%, 30%, 50%, 70%, 90% {
    transform: translateX(-4px);
  }
  20%, 40%, 60%, 80% {
    transform: translateX(4px);
  }
}

.save-error {
  animation: shake 0.4s ease-in-out;
}

// ==========================================
// 12. 下拉菜单优化
// ==========================================

:deep(.el-dropdown-menu) {
  border: 1px solid var(--el-border-color-lighter);
  box-shadow: $shadow-lg;
  border-radius: 8px;
  padding: 4px;

  .el-dropdown-menu__item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    padding: 8px 12px;
    border-radius: 4px;
    transition: all $transition-fast $easing-out;

    &:hover {
      background: var(--el-bg-color-overlay);
    }

    &:active {
      background: var(--el-color-primary-light-9);
    }

    // 布局选项样式
    .layout-option {
      display: flex;
      align-items: center;
      gap: 8px;
      width: 100%;

      .layout-icon {
        font-size: 16px;
        width: 20px;
        text-align: center;
      }
    }

    // Active state
    &.is-active {
      background: var(--el-color-primary-light-9);
      color: var(--el-color-primary);
      font-weight: 500;

      .layout-icon {
        transform: scale(1.1);
      }
    }
  }
}

// ==========================================
// 13. 面包屑导航增强
// ==========================================

:deep(.el-breadcrumb) {
  .el-breadcrumb__item {
    font-size: 13px;
    font-weight: 500;

    &:last-child {
      color: var(--el-text-color-primary);
      font-weight: 600;
    }
  }

  .el-breadcrumb__separator {
    color: var(--el-text-color-secondary);
    margin: 0 6px;
  }
}

// ==========================================
// 14. 抽屉优化
// ==========================================

:deep(.el-drawer) {
  .el-drawer__header {
    margin-bottom: 0;
    padding: 16px 20px;
    border-bottom: 1px solid var(--el-border-color-lighter);

    .el-drawer__title {
      font-size: 16px;
      font-weight: 600;
      color: var(--el-text-color-primary);
    }
  }
}

// ==========================================
// 15. 大纲项样式
// ==========================================

:deep(.outline-item) {
  padding: 8px 12px;
  cursor: pointer;
  border-radius: 6px;
  transition: all $transition-base $easing-out;
  border-left: 2px solid transparent;

  &:hover {
    background: var(--el-bg-color-overlay);
    border-left-color: var(--el-border-color);
    transform: translateX(2px);
  }

  &.active {
    background: var(--el-color-primary-light-9);
    border-left-color: var(--el-color-primary);
    color: var(--el-color-primary);
    font-weight: 500;
  }

  // 滑入动画
  animation: slideIn 0.2s ease-out;
}

@keyframes slideIn {
  from {
    opacity: 0;
    transform: translateX(-10px);
  }
  to {
    opacity: 1;
    transform: translateX(0);
  }
}

// ==========================================
// 16. 快捷键面板样式
// ==========================================

.keyboard-shortcuts-content {
  padding: 8px 0;

  .shortcut-category {
    margin-bottom: 24px;

    &:last-child {
      margin-bottom: 0;
    }

    .category-title {
      margin: 0 0 12px 0;
      font-size: 14px;
      font-weight: 600;
      color: var(--el-text-color-primary);
      padding-bottom: 8px;
      border-bottom: 1px solid var(--el-border-color-lighter);
    }

    .shortcut-list {
      display: flex;
      flex-direction: column;
      gap: 8px;

      .shortcut-item {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 8px 12px;
        border-radius: 6px;
        transition: all $transition-fast $easing-out;

        &:hover {
          background: var(--el-bg-color-overlay);
        }

        .shortcut-action {
          font-size: 14px;
          color: var(--el-text-color-regular);
        }

        .shortcut-keys {
          display: flex;
          gap: 4px;

          kbd {
            display: inline-block;
            padding: 4px 8px;
            min-width: 24px;
            text-align: center;
            font-size: 12px;
            font-family: 'Monaco', 'Menlo', monospace;
            font-weight: 500;
            color: var(--el-text-color-primary);
            background: var(--el-bg-color-page);
            border: 1px solid var(--el-border-color);
            border-radius: 4px;
            box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
          }
        }
      }
    }
  }
}

// ==========================================
// 17. 查找替换面板样式
// ==========================================

.find-replace-panel {
  position: absolute;
  top: 100%;
  right: 0;
  z-index: 1000;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 8px;
  box-shadow: $shadow-lg;
  padding: 12px;
  min-width: 400px;
  margin-top: 4px;

  .find-replace-row {
    display: flex;
    gap: 8px;
    align-items: center;
    margin-bottom: 8px;

    &:last-child {
      margin-bottom: 0;
    }

    :deep(.el-input) {
      flex: 1;
    }

    .match-count {
      font-size: 12px;
      color: var(--el-text-color-secondary);
      font-weight: 500;
    }
  }

  .find-replace-options {
    display: flex;
    align-items: center;
    gap: 12px;
    padding-top: 8px;
    border-top: 1px solid var(--el-border-color-lighter);
    margin-top: 8px;
    font-size: 13px;
  }
}

// 编辑器工具栏需要相对定位来支持查找面板的绝对定位
.editor-toolbar {
  position: relative;
}

// ==========================================
// 18. 模板面板样式
// ==========================================

.templates-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
  min-height: 400px;

  .templates-categories {
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
    padding: 8px 0;
    border-bottom: 1px solid var(--el-border-color-lighter);

    .category-tab {
      padding: 8px 16px;
      border-radius: 20px;
      cursor: pointer;
      transition: all $transition-base $easing-out;
      background: var(--el-bg-color-page);
      border: 1px solid var(--el-border-color);
      color: var(--el-text-color-secondary);
      font-size: 13px;
      font-weight: 500;

      &:hover {
        background: var(--el-bg-color-overlay);
        border-color: var(--el-color-primary);
        color: var(--el-color-primary);
      }

      &.active {
        background: var(--el-color-primary);
        border-color: var(--el-color-primary);
        color: white;
      }
    }
  }

  .templates-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(240px, 1fr));
    gap: 16px;
    padding: 8px 0;

    .template-card {
      display: flex;
      align-items: center;
      gap: 12px;
      padding: 16px;
      border: 1px solid var(--el-border-color-lighter);
      border-radius: 8px;
      cursor: pointer;
      transition: all $transition-base $easing-out;
      background: var(--el-bg-color);
      position: relative;
      overflow: hidden;

      &:hover {
        border-color: var(--el-color-primary);
        transform: translateY(-2px);
        box-shadow: $shadow-md;

        .template-arrow {
          transform: translateX(4px);
        }

        &::before {
          content: '';
          position: absolute;
          inset: 0;
          background: linear-gradient(90deg, transparent, rgba(99, 102, 241, 0.05), transparent);
          opacity: 1;
        }
      }

      .template-icon {
        font-size: 32px;
        width: 48px;
        height: 48px;
        display: flex;
        align-items: center;
        justify-content: center;
        background: var(--el-bg-color-page);
        border-radius: 8px;
        flex-shrink: 0;
      }

      .template-info {
        flex: 1;
        min-width: 0;

        .template-name {
          font-size: 15px;
          font-weight: 600;
          color: var(--el-text-color-primary);
          margin-bottom: 4px;
        }

        .template-description {
          font-size: 12px;
          color: var(--el-text-color-secondary);
          line-height: 1.4;
        }
      }

      .template-arrow {
        color: var(--el-text-color-secondary);
        transition: transform $transition-base $easing-out;
      }
    }
  }
}
</style>