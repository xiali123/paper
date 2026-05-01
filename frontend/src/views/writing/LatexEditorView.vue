<template>
  <div class="latex-editor-view" role="application" aria-label="LaTeX 编辑器">
    <!-- 顶部导航栏 - 简化版 -->
    <header class="editor-header">
      <!-- 左侧：文档名 -->
      <div class="header-left">
        <div class="doc-title" v-if="currentDocument || currentProject">
          <el-icon><Document /></el-icon>
          <span class="title-text">{{ (currentDocument || currentProject)?.name }}</span>
          <el-tag v-if="unsavedChanges" type="warning" size="small" effect="plain">未保存</el-tag>
        </div>
        <div class="doc-title" v-else>
          <el-icon><Edit /></el-icon>
          <span class="title-text">新建文档</span>
        </div>
      </div>

      <!-- 右侧：核心操作 -->
      <div class="header-right">
        <div class="header-actions">
          <!-- 编译 -->
          <el-tooltip content="编译 F5">
            <el-button size="small" :loading="compiling" @click="compileDocument">
              <el-icon><VideoPlay /></el-icon>
            </el-button>
          </el-tooltip>

          <!-- 保存 -->
          <el-tooltip :content="autoSave.isSaving.value ? '保存中...' : '保存 Ctrl+S'">
            <el-button size="small" @click="saveDocument" :disabled="autoSave.isSaving.value">
              <el-icon><DocumentChecked /></el-icon>
            </el-button>
          </el-tooltip>

          <!-- 预览切换 -->
          <el-tooltip :content="showPreview ? '隐藏预览' : '显示预览 Ctrl+P'">
            <el-button size="small" @click="showPreview = !showPreview" :class="{ active: showPreview }">
              <el-icon><View /></el-icon>
            </el-button>
          </el-tooltip>

          <!-- 更多选项 -->
          <el-dropdown @command="handleHeaderCommand" trigger="click">
            <el-button size="small" link>
              <el-icon><MoreFilled /></el-icon>
            </el-button>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="settings">
                  <el-icon><Setting /></el-icon>
                  设置
                </el-dropdown-item>
                <el-dropdown-item command="layout" divided>
                  <el-icon><Grid /></el-icon>
                  切换布局
                </el-dropdown-item>
                <el-dropdown-item command="shortcut">
                  <el-icon><QuestionFilled /></el-icon>
                  快捷键
                </el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </div>
    </header>

    <!-- 主编辑区域 -->
    <main
      class="editor-main"
      :class="{
        'show-preview': showPreview,
        'is-mobile': isMobile,
        [layoutMode]: true
      }"
      role="main"
    >
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

      <!-- 编辑器工具栏 - 重构版 -->
      <div class="editor-toolbar">
        <!-- 左侧工具 -->
        <div class="toolbar-left">
          <!-- 侧边栏切换 -->
          <el-tooltip content="文档大纲 Ctrl+Shift+O">
            <el-button class="toolbar-btn" size="small" @click="toggleLeftPanel" :class="{ active: showOutline || showProjectTree }">
              <el-icon><Menu /></el-icon>
            </el-button>
          </el-tooltip>

          <el-divider direction="vertical" />

          <!-- 撤销重做 -->
          <el-tooltip content="撤销 Ctrl+Z">
            <el-button class="toolbar-btn" size="small" @click="undoRedo.undo()" :disabled="!undoRedo.canUndo.value">
              <el-icon><RefreshLeft /></el-icon>
            </el-button>
          </el-tooltip>
          <el-tooltip content="重做 Ctrl+Y">
            <el-button class="toolbar-btn" size="small" @click="undoRedo.redo()" :disabled="!undoRedo.canRedo.value">
              <el-icon><RefreshRight /></el-icon>
            </el-button>
          </el-tooltip>

          <el-divider direction="vertical" />

          <!-- 常用格式（移除文字） -->
          <el-tooltip content="粗体 Ctrl+B">
            <el-button class="toolbar-btn" size="small" @click="insertLatexCommand('textbf')">
              <el-icon><Notification /></el-icon>
            </el-button>
          </el-tooltip>
          <el-tooltip content="斜体 Ctrl+I">
            <el-button class="toolbar-btn" size="small" @click="insertLatexCommand('textit')">
              <el-icon><EditPen /></el-icon>
            </el-button>
          </el-tooltip>

          <el-divider direction="vertical" />

          <!-- AI公式识别 -->
          <el-tooltip content="AI公式识别 Ctrl+Alt+I">
            <el-button class="toolbar-btn" size="small" @click="aiRecognizerRef?.open()">
              <el-icon><MagicStick /></el-icon>
            </el-button>
          </el-tooltip>

          <!-- 符号面板 -->
          <el-tooltip content="符号面板">
            <el-button class="toolbar-btn" size="small" @click="showSymbolPalette = !showSymbolPalette" :class="{ active: showSymbolPalette }">
              <el-icon><Tickets /></el-icon>
            </el-button>
          </el-tooltip>

          <!-- 代码片段 -->
          <el-tooltip content="代码片段">
            <el-button class="toolbar-btn" size="small" @click="showSnippets = !showSnippets" :class="{ active: showSnippets }">
              <el-icon><Collection /></el-icon>
            </el-button>
          </el-tooltip>
        </div>

        <!-- 右侧：功能面板标签 -->
        <div class="toolbar-right">
          <div class="panel-tabs">
            <button
              v-for="panel in functionPanels"
              :key="panel.key"
              :class="['panel-tab', { active: activePanel === panel.key }]"
              @click="activePanel = panel.key"
            >
              <el-icon><component :is="panel.icon" /></el-icon>
              <span>{{ panel.label }}</span>
              <el-badge v-if="panel.count > 0" :value="panel.count" :max="99" />
            </button>
          </div>
        </div>
      </div>

      <!-- 查找替换面板 -->
      <FindReplacePanel
        :show="showFindReplace"
        :find-query="findQuery"
        :replace-query="replaceQuery"
        :current-match-index="currentMatchIndex"
        :total-matches="totalMatches"
        :find-options="findOptions"
        :show-replace="showReplace"
        @close="showFindReplace = false"
        @update:find-query="findQuery = $event"
        @update:replace-query="replaceQuery = $event"
        @update:find-options="findOptions = { ...findOptions, ...$event }"
        @find-input="onFindInput"
        @find-next="findNext"
        @find-previous="findPrevious"
        @replace-current="replaceCurrent"
        @replace-all="replaceAll"
        @toggle-show-replace="showReplace = !showReplace"
      />

      <!-- 编辑器和预览容器 -->
      <div class="editor-preview-container" :class="[layoutMode]">
        <!-- 左侧面板：大纲/文件树 -->
        <LeftPanel
          v-if="showOutline || showProjectTree"
          :mode="showOutline ? 'outline' : 'project-tree'"
          :panel-title="showOutline ? '文档大纲' : '项目文件'"
          :content="editorContent"
          :is-project-mode="isProjectMode"
          :project-files="latexStore.projectFiles"
          :current-file-id="latexStore.currentProjectFile?.id"
          :main-file-path="latexStore.currentProject?.mainFile"
          :project-id="latexStore.currentProject?.id"
          :project-name="latexStore.currentProject?.name"
          @close="showOutline ? showOutline = false : showProjectTree = false"
          @navigate="navigateToSection"
          @file-select="showOutline ? handleOutlineFileSelect($event) : handleFileSelect($event)"
          @file-create="handleFileCreate"
          @file-delete="handleFileDelete"
          @file-rename="handleFileRename"
          @file-duplicate="handleFileDuplicate"
          @file-move="handleFileMove"
          @folder-create="handleFolderCreate"
          @folder-delete="handleFolderDelete"
          @folder-rename="handleFolderRename"
          @main-file-change="handleMainFileChange"
          @refresh="handleRefreshProject"
        />

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
        <div class="editor-content">
          <!-- 编辑器区域 -->
          <div class="editor-area">
            <!-- Use the working LatexEditor component -->
            <div class="editor-wrapper" ref="editorScrollElement">
              <LatexEditor
                ref="editorRef"
                v-model="editorContent"
                @change="handleEditorChange"
                @cursor-change="(position) => (latexStore as any).updateCursorPosition(position)"
              />
            </div>
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
      <PreviewPanel
        ref="previewPanelRef"
        :show="showPreview && !isMobile"
        :layout-mode="layoutMode"
        :editor-panel-width="editorPanelWidth"
        :is-mobile="isMobile"
        :preview-mode="previewMode"
        :preview-scale="previewScale"
        :content="editorContent"
        :theme="theme"
        :pdf-url="pdfUrl"
        :pdf-id="currentPdfId"
        :document-id="currentDocument?.id"
        :project-id="currentProject?.id"
        :errors="errors"
        :warnings="warnings"
        :active-error-tab="activeErrorTab"
        @update:preview-mode="previewMode = $event"
        @zoom-in="zoomIn"
        @zoom-out="zoomOut"
        @reset-zoom="resetZoom"
        @compile="compileDocument"
        @update:active-error-tab="activeErrorTab = $event"
        @navigate-to-error="navigateToError"
      />
      </div>
    </main>

    <!-- 编辑器状态栏 -->
    <EditorStatusBar
      :saving="saving"
      :compiling="compiling"
      :compile-success="compilationStatus === 'success'"
      :compile-error="compilationStatus === 'error'"
      :cursor-line="cursorPosition.line"
      :cursor-column="cursorPosition.column"
      @show-shortcuts="shortcutHelpRef?.open()"
      @show-quick-insert="quickInsertRef?.open()"
      @show-ai-recognize="aiRecognizerRef?.open()"
      ref="statusBarRef"
    />

    <!-- 符号面板 -->
    <SymbolPanel v-model:show="showSymbolPalette" @insert="insertSymbol" />

    <!-- 表格生成器面板 -->
    <TablePanel v-model:show="showTableGenerator" @insert="insertTableCode" />

    <!-- 拼写检查面板 -->
    <SpellCheckPanel v-model:show="showSpellChecker" :content="editorContent" @replace="handleSpellReplace" @goto="handleSpellGoto" />

    <!-- 模板管理面板 -->
    <TemplatesPanel v-model:show="showTemplates" @insert="insertTemplateContent" />

    <!-- 字体选择面板 -->
    <FontPanel v-model:show="showFontSelector" @select="handleFontChange" />

    <!-- 快捷键帮助 -->
    <ShortcutHelpDialog ref="shortcutHelpRef" />

    <!-- 快速插入面板 -->
    <QuickInsertDialog
      ref="quickInsertRef"
      @insert="handleQuickInsert"
    />

    <!-- AI 公式识别 -->
    <AiRecognizerDialog
      ref="aiRecognizerRef"
      @insert="insertFormula"
    />

    <!-- 审阅模式 -->
    <ReviewModeDialog
      ref="reviewModeRef"
      @toggle="handleReviewModeToggle"
      @insert="handleReviewInsert"
    />

    <!-- 导出对话框 -->
    <ExportDialogWrapper
      ref="exportDialogRef"
      @export="handleExport"
    />

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

    <!-- 版本控制面板 -->
    <el-drawer
      v-model="showVersionHistory"
      title="版本控制"
      direction="rtl"
      size="70%"
    >
      <VersionControl
        v-if="currentFileId && currentProjectId"
        :file-id="currentFileId"
        :project-id="currentProjectId"
        :user-id="currentUserId || 'default'"
        :current-content="editorContent"
        @restore="handleVersionRestore"
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
    <SnippetsPanel v-model:show="showSnippets" @insert="handleSnippetInsert" />


    <!-- LaTeX自动补全 -->
    <LatexAutocomplete
      :is-visible="autocomplete.isVisible.value"
      :position="autocomplete.position.value"
      :query="autocomplete.query.value"
      :selected-index="autocomplete.selectedIndex.value"
      :filtered-options="autocomplete.filteredOptions.value"
      @select="() => autocomplete.selectCurrent()"
      @hover="(index) => autocomplete.setSelectedIndex(index)"
    />

    <!-- 欢迎引导 -->
    <WelcomeGuideDialog v-model:show="showWelcomeGuide" @close="handleWelcomeGuideClose" />

    <!-- 最近文档 -->
    <el-drawer
      v-model="showRecentDocuments"
      title="最近文档"
      direction="ltr"
      size="600px"
    >
      <RecentDocuments
        ref="recentDocumentsRef"
        :documents="recentDocs"
        :loading="loadingRecentDocs"
        @open="handleOpenRecentDocument"
        @create-new="handleCreateNewDocument"
      />
    </el-drawer>

    <!-- 编辑器设置 -->
    <EditorSettings
      ref="editorSettingsRef"
      v-model:visible="showSettings"
      @update-settings="handleSettingsUpdate"
      @clear-cache="handleClearCache"
    />

    <!-- BibTeX文献管理器 -->
    <el-drawer
      v-model="showBibTeXManager"
      title="BibTeX文献管理"
      direction="rtl"
      size="500px"
    >
      <BibTeXManager @insert-citation="handleInsertCitation" />
    </el-drawer>

    <!-- LaTeX宏管理器 -->
    <el-drawer
      v-model="showMacroManager"
      title="LaTeX宏管理器"
      direction="rtl"
      size="450px"
    >
      <MacroManager @insert-macro="handleInsertMacro" />
    </el-drawer>

    <!-- 图片资源管理器 -->
    <el-drawer
      v-model="showImageResourceManager"
      title="图片资源管理"
      direction="rtl"
      size="600px"
    >
      <ImageResourceManager @insert-image="handleInsertImage" />
    </el-drawer>

    <!-- Git集成面板 -->
    <el-drawer
      v-model="showGitIntegration"
      title="版本控制 (Git)"
      direction="ltr"
      size="600px"
    >
      <GitIntegrationPanel />
    </el-drawer>

    <!-- 字数统计面板 -->
    <el-drawer
      v-model="showWordCount"
      title="字数统计"
      direction="rtl"
      size="380px"
    >
      <WordCountPanel :content="editorContent" />
    </el-drawer>

    <!-- 投稿前检查面板 -->
    <el-drawer
      v-model="showSubmissionChecker"
      title="投稿前检查"
      direction="rtl"
      size="480px"
    >
      <SubmissionChecker :content="editorContent" @fix="handleCheckerFix" />
    </el-drawer>

    <!-- 代码折叠导航 -->
    <el-drawer
      v-model="showCodeFoldNavigator"
      title="文档大纲"
      direction="ltr"
      size="320px"
    >
      <CodeFoldNavigator
        :content="editorContent"
        @jump-to-line="handleJumpToLine"
      />
    </el-drawer>

    <!-- 命令面板 -->
    <CommandPalette v-model="showCommandPalette" @command-executed="handleCommandExecuted" />

    <!-- 统计仪表板 -->
    <StatsDashboard
      v-model:show="showStatsDashboard"
      :stats="extendedDocumentStats"
      :document-id="currentDocument?.id"
      @refresh="handleRefreshStats"
      @export="handleExportStats"
    />

    <!-- 编译进度 -->
    <CircularProgress
      :visible="compiling"
      :text="compileText"
      :progress="compileProgress"
      :show-percentage="true"
      :show-cancel="true"
      @cancel="cancelCompile"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch, nextTick } from 'vue'
import { useLatexEditorStore } from '@/architecture/stores/latexEditor'
import { useAuthStore } from '@/stores'
import { useAutoSave } from '@/composables/useAutoSave'
import { useKeyboardShortcuts, getLatexShortcuts } from '@/composables/useKeyboardShortcuts'
import { useScrollSync } from '@/composables/useScrollSync'
import { useUndoRedo } from '@/composables/useUndoRedo'
import { useLatexAutocomplete } from '@/composables/useLatexAutocomplete'
import { saveLatexVersion } from '@/api/adapters/latexAdapter'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  DocumentChecked, VideoPlay, View, Menu, Tickets,
  Loading, Edit, RefreshLeft, RefreshRight, Operation,
  ArrowDown, Collection, Close, Document, DocumentAdd,
  Clock, MoreFilled, ArrowRight, Setting, Grid, MagicStick,
  Picture, Notification, QuestionFilled, EditPen,
  DataLine, CircleCheck
} from '@element-plus/icons-vue'
import LatexPreview from '@/components/latex/LatexPreview.vue'
import PdfViewer from '@/components/latex/PdfViewer.vue'
import LatexAutocomplete from '@/components/latex/LatexAutocomplete.vue'
import LatexEditor from '@/components/latex/LatexEditor.vue'
import CircularProgress from '@/components/latex/CircularProgress.vue'
import ShortcutHelpDialog from './latex/components/dialogs/ShortcutHelpDialog.vue'
import QuickInsertDialog from './latex/components/dialogs/QuickInsertDialog.vue'
import AiRecognizerDialog from './latex/components/dialogs/AiRecognizerDialog.vue'
import ReviewModeDialog from './latex/components/dialogs/ReviewModeDialog.vue'
import ExportDialogWrapper from './latex/components/dialogs/ExportDialogWrapper.vue'
import EditorStatusBar from '@/components/latex/EditorStatusBar.vue'
import CollaborationPanel from '@/components/collaboration/CollaborationPanel.vue'
import VersionHistory from '@/components/latex/VersionHistory.vue'
import VersionControl from '@/components/latex/VersionControl.vue'
import WelcomeGuideDialog from './latex/components/dialogs/WelcomeGuideDialog.vue'
import RecentDocuments from '@/components/latex/RecentDocuments.vue'
import EditorSettings from '@/components/latex/EditorSettings.vue'
import StatsDashboard from '@/components/latex/StatsDashboard.vue'
// New extracted components
import { EditorToolbar } from './latex/components'
import FindReplacePanel from './latex/components/editor/FindReplacePanel.vue'
import PreviewPanel from './latex/components/preview/PreviewPanel.vue'
import LeftPanel from './latex/components/panels/LeftPanel.vue'
import SymbolPanel from './latex/components/panels/SymbolPanel.vue'
import SnippetsPanel from './latex/components/panels/SnippetsPanel.vue'
import TemplatesPanel from './latex/components/panels/TemplatesPanel.vue'
import TablePanel from './latex/components/panels/TablePanel.vue'
import SpellCheckPanel from './latex/components/panels/SpellCheckPanel.vue'
import FontPanel from './latex/components/panels/FontPanel.vue'
	// Batch 2 & 3 expansion components
	import BibTeXManager from '@/components/latex/BibTeXManager.vue'
	import CodeFoldNavigator from '@/components/latex/CodeFoldNavigator.vue'
	import MacroManager from '@/components/latex/MacroManager.vue'
	import ImageResourceManager from '@/components/latex/ImageResourceManager.vue'
	import InlineCommentSystem from '@/components/latex/InlineCommentSystem.vue'
	import GitIntegrationPanel from '@/components/latex/GitIntegrationPanel.vue'
import WordCountPanel from '@/components/latex/WordCountPanel.vue'
import SubmissionChecker from '@/components/latex/SubmissionChecker.vue'
	import CommandPalette from '@/components/latex/CommandPalette.vue'
// Monaco editor integration removed - using simple LatexEditor component

// Props and emits
interface Props {
  documentId?: string
}

const props = defineProps<Props>()

// Router and store
const latexStore = useLatexEditorStore()

// Auth store for user info
const authStore = useAuthStore()

// Refs
const previewRef = ref<InstanceType<typeof LatexPreview> | null>(null)
const pdfViewerRef = ref<InstanceType<typeof PdfViewer> | null>(null)
const editorRef = ref<any>(null)
const shortcutHelpRef = ref<InstanceType<typeof ShortcutHelpDialog> | null>(null)
const quickInsertRef = ref<InstanceType<typeof QuickInsertDialog> | null>(null)
const aiRecognizerRef = ref<InstanceType<typeof AiRecognizerDialog> | null>(null)
const reviewModeRef = ref<InstanceType<typeof ReviewModeDialog> | null>(null)
const exportDialogRef = ref<InstanceType<typeof ExportDialogWrapper> | null>(null)
const statusBarRef = ref<InstanceType<typeof EditorStatusBar> | null>(null)
const welcomeGuideRef = ref<InstanceType<typeof WelcomeGuideDialog> | null>(null)
const showWelcomeGuide = ref(false)
const recentDocumentsRef = ref<InstanceType<typeof RecentDocuments> | null>(null)
const editorToolbarRef = ref<InstanceType<typeof EditorToolbar> | null>(null)
const editorSettingsRef = ref<InstanceType<typeof EditorSettings> | null>(null)
const showStatsDashboard = ref(false)
const statsDashboardRef = ref<InstanceType<typeof StatsDashboard> | null>(null)

// Reactive state
const showPreview = ref(true)
const showOutline = ref(false)
const showSymbolPalette = ref(false)
const showTableGenerator = ref(false)
const showSpellChecker = ref(false)
const showTemplates = ref(false)
const showSnippets = ref(false)
const showFontSelector = ref(false)
const showCollaborationPanel = ref(false)
const showKeyboardShortcuts = ref(false) // 新增：快捷键面板
const showSettings = ref(false) // 设置面板
const isFullscreen = ref(false) // 全屏状态
const showProjectTree = ref(false) // 新增：项目文件树
const showVersionHistory = ref(false) // 新增：版本历史面板
	// Batch 2 & 3 expansion states
	const showBibTeXManager = ref(false)
	const showCodeFoldNavigator = ref(false)
	const showMacroManager = ref(false)
	const showImageResourceManager = ref(false)
	const showInlineComments = ref(false)
	const showGitIntegration = ref(false)
	const showCommandPalette = ref(false)
	const showMultiWindow = ref(false)
	const showWordCount = ref(false)
	const showSubmissionChecker = ref(false)
const isProjectMode = computed(() => latexStore.isProjectMode) // 从store读取
const saving = ref(false)
const compiling = ref(false)
const compileProgress = ref(0)
const compileText = ref('')
const activeErrorTab = ref<'errors' | 'warnings'>('errors')
const documentId = ref<string | null>(props.documentId || null)
const mobileActiveTab = ref<'editor' | 'preview'>('editor')
const isMobile = ref(false)

// 新增UI组件状态
const showRecentDocuments = ref(false)
const loadingRecentDocs = ref(false)
const recentDocs = ref<any[]>([])
const cursorPosition = ref({ line: 1, column: 1 })

// PDF预览相关状态
const pdfUrl = ref<string | null>(null)
const previewMode = ref<'html' | 'pdf'>('html') // 预览模式：HTML或PDF

// 当前PDF的唯一ID（用于本地缓存）
const currentPdfId = computed(() => {
  if (isProjectMode.value && currentProject.value) {
    return `project-${currentProject.value.id}`
  }
  if (currentDocument.value) {
    return `document-${currentDocument.value.id}`
  }
  return `pdf-${Date.now()}`
})

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
// 使用本地 ref，完全独立于 store
const editorContent = ref('')
const compilationStatus = computed(() => latexStore.compilationStatus)
const isCollaborating = computed(() => latexStore.isCollaborating)
const collaborationSession = computed(() => latexStore.collaborationSession)
const collaborationUsers = computed(() => Array.from(latexStore.collaborationUsers.values()))
const activeCollaborationUsers = computed(() => latexStore.activeCollaborationUsers)
const documentStats = computed(() => latexStore.documentStats)
const currentProject = computed(() => latexStore.currentProject)

// 扩展的文档统计（用于StatsDashboard组件）
const extendedDocumentStats = computed(() => {
  const content = editorContent.value
  const baseStats = latexStore.documentStats

  // LaTeX元素统计
  const formulas = (content.match(/\\\(|\\\[|\\begin\{equation\}/g) || []).length
  const inlineFormulas = (content.match(/\\\(/g) || []).length
  const displayFormulas = (content.match(/\\\[|\\begin\{equation\}/g) || []).length
  const references = (content.match(/\\cite\{|\\ref\{/g) || []).length
  const images = (content.match(/\\includegraphics|\\begin\{figure\}/g) || []).length
  const tables = (content.match(/\\begin\{tabular\}|\\begin\{table\}/g) || []).length
  const packages = (content.match(/\\usepackage\{/g) || []).length

  // 阅读时间估算
  const wordsPerMinute = 200
  const minutes = Math.ceil(baseStats.words / wordsPerMinute)
  const readingTime = minutes > 60
    ? `${Math.floor(minutes / 60)}小时${minutes % 60}分钟`
    : `${minutes}分钟`

  return {
    ...baseStats,
    totalChars: baseStats.characters,
    totalWords: baseStats.words,
    totalLines: baseStats.lines,
    nonEmptyLines: baseStats.lines,
    paragraphs: baseStats.paragraphs,
    sentences: 0,
    totalPages: 1,
    readingTime,
    formulas,
    inlineFormulas,
    displayFormulas,
    references,
    citations: references,
    refs: 0,
    images,
    figures: images,
    tables,
    tabulars: tables,
    environments: 0,
    packages,
    structure: {},
    topCommands: [],
    totalCommands: 0
  }
})

// 左侧面板标题
const leftPanelTitle = computed(() => {
  if (isProjectMode.value) {
    return showProjectTree.value ? '文件树' : '大纲'
  }
  return showOutline.value ? '大纲' : '菜单'
})

// 功能面板标签
const activePanel = ref('bibtex')
const unsavedChanges = ref(false)

const functionPanels = computed(() => [
  { key: 'bibtex', label: '文献', icon: Document, count: 0 },
  { key: 'macro', label: '宏包', icon: MagicStick, count: 0 },
  { key: 'images', label: '图片', icon: Picture, count: 0 },
  { key: 'wordcount', label: '字数', icon: DataLine, count: 0 },
  { key: 'checker', label: '检查', icon: CircleCheck, count: 0 },
  { key: 'git', label: '版本', icon: Operation, count: 0 }
])

// 监听功能面板切换
watch(activePanel, (newPanel) => {
  switch (newPanel) {
    case 'bibtex':
      showBibTeXManager.value = true
      break
    case 'macro':
      showMacroManager.value = true
      break
    case 'images':
      showImageResourceManager.value = true
      break
    case 'wordcount':
      showWordCount.value = true
      break
    case 'checker':
      showSubmissionChecker.value = true
      break
    case 'git':
      showGitIntegration.value = true
      break
  }
})

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

// 版本历史相关计算属性
const currentUserId = computed(() => 'default')
const currentFileId = computed(() => {
  // 项目模式：返回当前选中的文件ID
  if (isProjectMode.value && latexStore.currentProjectFile) {
    return latexStore.currentProjectFile.id
  }
  // 文档模式：返回文档ID
  if (currentDocument.value) {
    return parseInt(currentDocument.value.id) || 1
  }
  return 1
})
const currentProjectId = computed(() => {
  if (isProjectMode.value && currentProject.value) {
    return currentProject.value.id
  }
  return 1
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

// 处理新建命令
async function handleNewCommand(command: string) {
  switch (command) {
    case 'document':
      await handleCreateNewDocument()
      break
    case 'project':
      await handleCreateNewProject()
      break
  }
}

// 处理header命令
function handleHeaderCommand(command: string) {
  switch (command) {
    case 'layout':
      // 循环切换布局模式
      const modes: LayoutMode[] = ['side-by-side', 'vertical-split', 'editor-only']
      const currentIndex = modes.indexOf(layoutMode.value)
      const nextMode = modes[(currentIndex + 1) % modes.length]
      setLayoutMode(nextMode)
      break
    case 'settings':
      editorSettingsRef.value?.open()
      break
    case 'shortcut':
      showKeyboardShortcuts.value = true
      break
  }
}

// 处理工具栏命令
function handleToolbarCommand(command: string) {
  switch (command) {
    case 'ai':
      aiRecognizerRef.value?.open()
      break
    case 'table':
      showTableGenerator.value = !showTableGenerator.value
      break
    case 'spell':
      showSpellChecker.value = !showSpellChecker.value
      break
    case 'template':
      showTemplates.value = !showTemplates.value
      break
    case 'font':
      showFontSelector.value = !showFontSelector.value
      break
    case 'shortcut':
      showKeyboardShortcuts.value = true
      break
    case 'version':
      showVersionHistory.value = true
      break
    case 'bibtex':
      showBibTeXManager.value = !showBibTeXManager.value
      break
    case 'macro':
      showMacroManager.value = !showMacroManager.value
      break
    case 'images':
      showImageResourceManager.value = !showImageResourceManager.value
      break
    case 'git':
      showGitIntegration.value = !showGitIntegration.value
      break
    case 'comments':
      showInlineComments.value = !showInlineComments.value
      break
    case 'navigator':
      showCodeFoldNavigator.value = !showCodeFoldNavigator.value
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
const AUTO_SAVE_INTERVAL = 60000 // 1分钟
const autoSaveEnabled = ref(true)

// 创建历史版本
const createVersionSnapshot = async (content: string, isAuto: boolean = true) => {
  try {
    if (!authStore.user?.id) {
      console.warn('[Version] No user logged in, skipping version creation')
      return
    }

    const fileId = currentFileId.value
    const projectId = currentProjectId.value

    if (!fileId || !projectId) {
      console.warn('[Version] Missing file or project ID', { fileId, projectId })
      return
    }

    await saveLatexVersion({
      fileId: typeof fileId === 'string' ? parseInt(fileId) : fileId,
      projectId,
      userId: String(authStore.user?.id || ''),
      content,
      summary: isAuto ? '自动保存' : '手动保存',
      isAutoSave: isAuto
    })

    if (import.meta.env.DEV) {
      console.log(`[Version] ${isAuto ? 'Auto' : 'Manual'} version created`)
    }
  } catch (error) {
    console.error('[Version] Failed to create version:', error)
  }
}

// 设置自动保存 - 现在 editorContent 已经定义
const autoSave = useAutoSave(
  editorContent,
  documentId,
  {
    interval: AUTO_SAVE_INTERVAL,
    debounceDelay: 2000,
    enableLocalStorage: true,
    onSave: async (content) => {
      await latexStore.saveDocument()
      // 创建历史版本
      await createVersionSnapshot(content, true)
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
  },
  onShowWelcome: () => {
    showWelcomeGuide.value = true
  },
  onShowRecent: () => {
    showRecentDocuments.value = true
    loadRecentDocuments()
  },
  onCommandPalette: () => {
    showCommandPalette.value = true
  },
  onShowSettings: () => {
    editorSettingsRef.value?.open()
  },
  onShowStats: () => {
    handleRefreshStats()
    showStatsDashboard.value = true
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

// AbortController用于取消pending请求（防止竞态条件）
let currentSaveController: AbortController | null = null
let currentCompileController: AbortController | null = null

// 编辑器内容变化处理
function handleEditorChange(value: string) {
  isModified.value = true
  latexStore.updateDocumentContent(value)
}

// Methods
async function saveDocument() {
  // 取消之前的保存请求
  if (currentSaveController) {
    currentSaveController.abort()
  }
  currentSaveController = new AbortController()

  if (import.meta.env.DEV) {
    console.log('Save document clicked, project mode:', isProjectMode.value)
  }
  saving.value = true
  try {
    if (isProjectMode.value) {
      await latexStore.saveCurrentProjectFile()
    } else {
      await latexStore.saveDocument()
    }
    isModified.value = false

    // 创建历史版本（手动保存）
    await createVersionSnapshot(editorContent.value, false)

    if (import.meta.env.DEV) {
      console.log('Document saved successfully')
    }
    // Show success message
    ElMessage.success('文档保存成功')
  } catch (error: any) {
    // 忽略取消的错误
    if (error.name === 'AbortError') {
      if (import.meta.env.DEV) {
        console.log('Save aborted')
      }
      return
    }

    console.error('Save failed:', error)

    // 改进的错误处理
    let userMessage = '文档保存失败'
    if (error.response) {
      // HTTP错误响应
      const status = error.response.status
      if (status === 401) {
        userMessage = '登录已过期，请重新登录'
      } else if (status === 403) {
        userMessage = '没有权限保存此文档'
      } else if (status === 404) {
        userMessage = '文档不存在，可能已被删除'
      } else if (status >= 500) {
        userMessage = '服务器错误，请稍后重试'
      } else {
        userMessage = `保存失败: ${error.response.data?.message || '未知错误'}`
      }
    } else if (error.code === 'ECONNABORTED') {
      userMessage = '请求超时，请检查网络连接'
    } else if (error.message) {
      userMessage = `保存失败: ${error.message}`
    }

    ElMessage.error(userMessage)
  } finally {
    saving.value = false
    currentSaveController = null
  }
}

async function compileDocument() {
  // 取消之前的编译请求
  if (currentCompileController) {
    currentCompileController.abort()
  }
  currentCompileController = new AbortController()

  compiling.value = true
  compileProgress.value = 0
  compileText.value = isProjectMode.value ? '正在编译LaTeX项目...' : '正在编译LaTeX文档...'

  // 模拟编译进度
  const progressInterval = setInterval(() => {
    if (compileProgress.value < 90) {
      compileProgress.value += Math.random() * 15
      if (compileProgress.value > 90) {
        compileProgress.value = 90
      }
    }
  }, 300)

  try {
    let result: any

    if (isProjectMode.value) {
      // 项目模式：先保存当前文件再编译项目
      if (isModified.value) {
        compileText.value = '正在保存...'
        compileProgress.value = 20
        await latexStore.saveCurrentProjectFile()
        compileText.value = '正在编译...'
        compileProgress.value = 40
      }
      result = await latexStore.compileProject()
    } else {
      // 单文档模式：编译单个文档
      compileText.value = '正在编译...'
      compileProgress.value = 50
      result = await latexStore.compileDocument()
    }

    clearInterval(progressInterval)
    compileProgress.value = 100
    compileText.value = '编译完成！'

    if (result.success) {
      // 设置PDF URL并切换到PDF预览模式
      if (result.pdfPath) {
        let pdfUrlValue = ''

        if (isProjectMode.value) {
          // 使用项目ID获取PDF
          const projectId = currentProject.value?.id
          if (projectId) {
            pdfUrlValue = `/api/latex/projects/${projectId}/pdf?t=${Date.now()}`
          }
        } else {
          // 使用文档ID获取PDF
          const documentId = currentDocument.value?.id
          if (documentId) {
            pdfUrlValue = `/api/latex/documents/${documentId}/pdf?t=${Date.now()}`
          }
        }

        if (pdfUrlValue) {
          pdfUrl.value = pdfUrlValue
        }
        previewMode.value = 'pdf'

        // 显示成功消息，包含编译时间和文件大小信息
        const compileTime = result.compileTimeMs || 0
        const modePrefix = isProjectMode.value ? '项目' : '文档'
        let message = `${modePrefix}编译成功！耗时${compileTime}ms`

        // 如果编译时间较长，显示不同的提示
        if (compileTime > 5000) {
          message = `${modePrefix}编译完成（耗时${(compileTime / 1000).toFixed(1)}秒）`
        } else if (compileTime > 1000) {
          message = `${modePrefix}编译成功（耗时${(compileTime / 1000).toFixed(1)}秒）`
        }

        ElMessage({
          message,
          type: 'success',
          duration: 3000,
          showClose: true
        })
      } else {
        // 如果没有PDF路径，刷新HTML预览
        if (previewRef.value) {
          previewRef.value.refresh()
        }
        ElMessage.success('预览已更新')
      }
    } else {
      // 处理不同类型的错误
      const errorMsg = result.error || '编译失败'

      // 检查是否是配额限制错误
      if (errorMsg.includes('quota') || errorMsg.includes('limit') || errorMsg.includes('配额')) {
        ElMessage({
          message: '编译配额已用完，请升级套餐或等待配额重置',
          type: 'warning',
          duration: 5000,
          showClose: true
        })

        // 可以显示配额信息的对话框
        ElMessageBox.alert(
          '您的编译配额已用完。\n\n免费用户每日10次，每月100次。\n\n请升级到Pro版获得更多编译次数，或等待配额自动重置。',
          '编译配额已用完',
          {
            confirmButtonText: '知道了',
            type: 'warning'
          }
        )
      } else if (errorMsg.includes('timeout') || errorMsg.includes('超时')) {
        ElMessage.error('编译超时，请检查文档是否有复杂内容或减少文件大小')
      } else if (errorMsg.includes('syntax') || errorMsg.includes('语法')) {
        ElMessage.error('LaTeX语法错误，请检查文档')
      } else {
        ElMessage.error({
          message: `编译失败: ${errorMsg}`,
          duration: 5000,
          showClose: true
        })
      }

      // 如果有详细日志，显示在控制台
      if (result.log && import.meta.env.DEV) {
        console.log('Compilation log:', result.log)
      }
    }
  } catch (error: any) {
    clearInterval(progressInterval)

    // 忽略取消的错误
    if (error.name === 'AbortError') {
      if (import.meta.env.DEV) {
        console.log('Compilation aborted')
      }
      return
    }

    console.error('Compilation failed:', error)

    let userMessage = '编译失败'
    const errorMsg = error instanceof Error ? error.message : '未知错误'

    // 改进的错误处理
    if (error.response) {
      const status = error.response.status
      if (status === 401) {
        userMessage = '登录已过期，请重新登录'
      } else if (status === 403) {
        userMessage = '没有权限编译此文档'
      } else if (status === 404) {
        userMessage = '文档或项目不存在'
      } else if (status >= 500) {
        userMessage = '服务器错误，请稍后重试'
      } else {
        userMessage = `编译失败: ${error.response.data?.message || errorMsg}`
      }
    } else if (error.code === 'ECONNABORTED') {
      userMessage = '请求超时，请检查网络连接'
    } else if (errorMsg.includes('network') || errorMsg.includes('Network') || errorMsg.includes('fetch')) {
      userMessage = '网络错误，请检查网络连接后重试'
    } else {
      userMessage = `编译失败: ${errorMsg}`
    }

    ElMessage.error(userMessage)
  } finally {
    compiling.value = false
    compileProgress.value = 0
    currentCompileController = null
  }
}

function cancelCompile() {
  if (currentCompileController) {
    currentCompileController.abort()
    ElMessage.info('已取消编译')
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

function insertTableCode(code: string) {
  if (!editorRef.value) return

  if (import.meta.env.DEV) {
    console.log('Insert table code:', code)
  }

  // Get current textarea
  const textarea = editorRef.value.$el?.querySelector('textarea')
  if (!textarea) return

  const start = textarea.selectionStart
  const end = textarea.selectionEnd
  const content = editorContent.value

  const newContent = content.substring(0, start) + '\n' + code + '\n' + content.substring(end)
  editorContent.value = newContent

  // Focus and set cursor position
  nextTick(() => {
    textarea.focus()
    const newCursorPos = start + code.length + 2
    textarea.selectionStart = newCursorPos
    textarea.selectionEnd = newCursorPos
  })

  showTableGenerator.value = false
  ElMessage.success('表格已插入')
}

function handleSpellReplace(data: any) {
  if (!editorRef.value) return

  const { from, to } = data || {}
  if (!from || !to) return

  const textarea = editorRef.value.$el?.querySelector('textarea')
  if (!textarea) return

  const content = editorContent.value
  const newContent = content.split(from).join(to)
  editorContent.value = newContent
}

function handleSpellGoto(position: any) {
  if (!editorRef.value) return

  const line = typeof position === 'number' ? position : position?.line
  const column = position?.column || 0

  editorRef.value.navigateTo?.({ line, column })
}

function insertTemplateContent(content: string) {
  editorContent.value = content
  showTemplates.value = false
  ElMessage.success('模板已应用')
}

function handleFontChange(fontFamily: string) {
  // Font change is handled by FontSelector component via store
  if (import.meta.env.DEV) {
    console.log('Font changed to:', fontFamily)
  }
}

// 快速插入处理
function handleQuickInsert(code: string) {
  insertLatexCommand(code)
}

// AI公式识别插入
function insertFormula(latex: string) {
  insertLatexCommand(latex)
}

// 审阅模式切换
function handleReviewModeToggle(enabled: boolean) {
  if (import.meta.env.DEV) {
    console.log('Review mode:', enabled)
  }
}

// 审阅模式插入
function handleReviewInsert(text: string) {
  insertLatexCommand(text)
}

// 导出处理
function handleExport(data: any) {
  const { format, options, filename } = data || {}

  if (import.meta.env.DEV) {
    console.log('Export:', format, options, filename)
  }

  // 根据格式执行导出
  switch (format) {
    case 'pdf':
      compileDocument()
      break
    case 'latex':
      downloadAsTex(filename || 'document.tex')
      break
    case 'markdown':
      convertToMarkdown(filename || 'document.md')
      break
    default:
      ElMessage.info(`导出为 ${format?.toUpperCase()} 功能开发中`)
  }
}

// 下载为 .tex 文件
function downloadAsTex(filename: string) {
  const blob = new Blob([editorContent.value], { type: 'text/plain' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `${filename}.tex`
  document.body.appendChild(a)
  a.click()
  document.body.removeChild(a)
  URL.revokeObjectURL(url)
  ElMessage.success('LaTeX 文件已下载')
}

// 转换为 Markdown
function convertToMarkdown(filename: string) {
  // 简单的 LaTeX 到 Markdown 转换
  let markdown = editorContent.value
    .replace(/\\section\{([^}]+)\}/g, '# $1\n')
    .replace(/\\subsection\{([^}]+)\}/g, '## $1\n')
    .replace(/\\subsubsection\{([^}]+)\}/g, '### $1\n')
    .replace(/\\textbf\{([^}]+)\}/g, '**$1**')
    .replace(/\\textit\{([^}]+)\}/g, '*$1*')
    .replace(/\\emph\{([^}]+)\}/g, '*$1*')
    .replace(/\$\$([^$]+)\$\$/g, '\n$$\n$1\n$$\n')
    .replace(/\$([^$]+)\$/g, '$$$1$$')
    .replace(/\\begin\{itemize\}[\s\S]*?\\end\{itemize\}/g, (match) => {
      const items = match.match(/\\item\s+([^\n]+)/g) || []
      return items.map(item => `- ${item.replace(/\\item\s+/, '')}`).join('\n')
    })
    .replace(/\\begin\{enumerate\}[\s\S]*?\\end\{enumerate\}/g, (match) => {
      const items = match.match(/\\item\s+([^\n]+)/g) || []
      return items.map((item, i) => `${i + 1}. ${item.replace(/\\item\s+/, '')}`).join('\n')
    })
    .replace(/\\[a-zA-Z]+/g, '') // 移除剩余的LaTeX命令
    .replace(/[{}]/g, '') // 移除花括号

  const blob = new Blob([markdown], { type: 'text/markdown' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `${filename}.md`
  document.body.appendChild(a)
  a.click()
  document.body.removeChild(a)
  URL.revokeObjectURL(url)
  ElMessage.success('Markdown 文件已下载')
}

function toggleLeftPanel() {
  if (import.meta.env.DEV) {
    console.log('Toggle left panel, project mode:', isProjectMode.value, 'outline:', showOutline.value, 'tree:', showProjectTree.value)
  }

  // 在项目模式下，切换文件树和大纲
  if (isProjectMode.value) {
    if (showProjectTree.value) {
      // 当前显示文件树，切换到大纲
      showProjectTree.value = false
      showOutline.value = true
    } else {
      // 当前显示大纲或都没显示，切换到文件树
      showProjectTree.value = true
      showOutline.value = false
    }
  } else {
    // 非项目模式，切换大纲
    showOutline.value = !showOutline.value
  }

  if (import.meta.env.DEV) {
    console.log('After toggle - outline:', showOutline.value, 'tree:', showProjectTree.value)
  }
}

function toggleOutline() {
  if (import.meta.env.DEV) {
    console.log('Toggle outline clicked, current state:', showOutline.value, 'project mode:', isProjectMode.value)
  }

  // 在项目模式下，大纲和项目文件树互斥
  if (isProjectMode.value) {
    if (!showOutline.value) {
      // 打开大纲，关闭项目文件树
      showOutline.value = true
      showProjectTree.value = false
      if (import.meta.env.DEV) {
        console.log('Opening outline, closing project tree')
      }
    } else {
      // 关闭大纲，重新打开项目文件树
      showOutline.value = false
      showProjectTree.value = true
      if (import.meta.env.DEV) {
        console.log('Closing outline, opening project tree')
      }
    }
  } else {
    // 非项目模式，正常切换大纲
    showOutline.value = !showOutline.value
  }

  if (import.meta.env.DEV) {
    console.log('New outline state:', showOutline.value, 'project tree state:', showProjectTree.value)
  }
}

function togglePanel(panel: string) {
  switch (panel) {
    case 'snippets':
      showSnippets.value = !showSnippets.value
      break
    case 'symbols':
      showSymbolPalette.value = !showSymbolPalette.value
      break
    case 'templates':
      showTemplates.value = !showTemplates.value
      break
    default:
      if (import.meta.env.DEV) {
        console.warn('Unknown panel:', panel)
      }
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

function handleOutlineFileSelect(file: any) {
  // 从大纲选择文件
  if (import.meta.env.DEV) {
    console.log('Outline file selected:', file)
  }

  // 切换到选中的文件
  latexStore.switchProjectFile(file)
  ElMessage.success(`已切换到文件: ${file.name}`)
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

// Watchers in setup scope for auto-cleanup on unmount
watch(() => theme.value, () => {
  // Theme changes handled by LatexEditor component
})

watch(() => editorContent.value, (newContent, oldContent) => {
  if (oldContent !== undefined && newContent !== oldContent) {
    isModified.value = true
  }
}, { immediate: true })

watch(() => latexStore.currentDocument, (newDoc) => {
  if (newDoc && newDoc.content !== editorContent.value) {
    editorContent.value = newDoc.content
  }
}, { immediate: true })

watch(() => latexStore.currentProjectFile, (newFile, oldFile) => {
  if (newFile && newFile !== oldFile) {
    if (newFile.content !== editorContent.value) {
      editorContent.value = newFile.content
    }
    isModified.value = false
  }
}, { immediate: true })

watch(() => latexStore.isProjectMode, (isProjectMode) => {
  showProjectTree.value = isProjectMode
})

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
    // F1 - 打开快捷键帮助
    if (e.key === 'F1') {
      shortcutHelpRef.value?.open()
      e.preventDefault()
      return
    }

    // Ctrl+Alt+X - 打开快速插入面板
    if (e.ctrlKey && e.altKey && e.key === 'x') {
      quickInsertRef.value?.open()
      e.preventDefault()
      return
    }

    // Ctrl+Alt+I - 打开AI公式识别
    if (e.ctrlKey && e.altKey && e.key === 'i') {
      aiRecognizerRef.value?.open()
      e.preventDefault()
      return
    }

    // Ctrl+E - 打开导出对话框
    if (e.ctrlKey && e.key === 'e') {
      exportDialogRef.value?.open(currentDocument.value?.name || currentProject.value?.name || 'document')
      e.preventDefault()
      return
    }

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
  }
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

function handleFileSelect(file: any) {
  if (import.meta.env.DEV) {
    console.log('[LaTeX Editor] File selected:', file.name, 'path:', file.path)
  }

  try {
    // 切换到选中的文件
    latexStore.switchProjectFile(file)
    ElMessage.success(`已切换到文件: ${file.name}`)
  } catch (error: any) {
    ElMessage.error('切换文件失败: ' + (error.message || '未知错误'))
  }
}

async function handleFileRename(fileId: number | string, newName: string) {
  try {
    await latexStore.renameProjectFile(Number(fileId), newName)
    ElMessage.success('文件重命名成功')
  } catch (error: any) {
    ElMessage.error('文件重命名失败: ' + (error.message || '未知错误'))
  }
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
// 文件夹操作
// ==========================================

async function handleFolderCreate(folderPath: string) {
  try {
    // 在LaTeX项目中，通过在目录下创建一个默认文件来实现目录创建
    const folderName = folderPath.split('/').pop() || 'new_folder'
    const defaultFileName = 'README.tex'

    await latexStore.createProjectFile({
      name: defaultFileName,
      path: `${folderPath}/${defaultFileName}`,
      type: 'other'
    })

    ElMessage.success(`目录 "${folderName}" 创建成功（已添加 README.tex）`)
  } catch (error: any) {
    ElMessage.error('创建目录失败: ' + (error.message || '未知错误'))
  }
}

async function handleFolderDelete(folderPath: string) {
  try {
    // 删除文件夹需要删除该文件夹下的所有文件
    const filesInFolder = latexStore.projectFiles.filter(f => {
      const filePath = f.path.startsWith('/') ? f.path.substring(1) : f.path
      return filePath.startsWith(folderPath) || f.path.startsWith(folderPath + '/')
    })

    if (filesInFolder.length === 0) {
      ElMessage.warning('该文件夹为空或不存在')
      return
    }

    // 确认删除
    await ElMessageBox.confirm(
      `将删除 ${filesInFolder.length} 个文件，确定继续吗？`,
      '删除文件夹',
      {
        confirmButtonText: '删除',
        cancelButtonText: '取消',
        type: 'warning'
      }
    )

    // 删除所有文件
    for (const file of filesInFolder) {
      await latexStore.deleteProjectFile(Number(file.id))
    }

    await latexStore.loadProject(latexStore.currentProject!.id)
    ElMessage.success(`已删除 ${filesInFolder.length} 个文件`)
  } catch (error: any) {
    // 用户取消或其他错误
    if (error !== 'cancel' && error !== 'close') {
      ElMessage.error('删除文件夹失败: ' + (error.message || '未知错误'))
    }
  }
}

async function handleFolderRename(oldPath: string, newName: string) {
  try {
    // 重命名文件夹需要更新所有该文件夹下文件的路径
    const filesInFolder = latexStore.projectFiles.filter(f => f.path.startsWith(oldPath))

    for (const file of filesInFolder) {
      const newPath = file.path.replace(oldPath, newName)
      await latexStore.renameProjectFile(Number(file.id), file.name)
    }

    await latexStore.loadProject(latexStore.currentProject!.id)
    ElMessage.success('文件夹重命名成功')
  } catch (error: any) {
    ElMessage.error('重命名文件夹失败: ' + (error.message || '未知错误'))
  }
}

// ==========================================
// 文件操作
// ==========================================

async function handleFileDuplicate(fileId: number | string) {
  try {
    const file = latexStore.projectFiles.find(f => f.id === fileId)
    if (!file) {
      ElMessage.error('文件不存在')
      return
    }

    // 创建副本
    await latexStore.createProjectFile({
      name: file.name.replace(/(\.[^.]+)$/, '_copy$1'),
      path: file.path.replace(/(\.[^.]+)$/, '_copy$1'),
      type: file.type
    })

    ElMessage.success('文件复制成功')
  } catch (error: any) {
    ElMessage.error('复制文件失败: ' + (error.message || '未知错误'))
  }
}

async function handleFileMove(fileId: number | string, targetPath: string) {
  ElMessage.info('移动文件功能开发中，敬请期待')
}

// 版本历史处理函数
function handleVersionRestore(content: string) {
  // 恢复版本内容到编辑器
  latexStore.updateDocumentContent(content)
  isModified.value = true
  ElMessage.success('版本已恢复，请记得保存更改')
  showVersionHistory.value = false
}

// 新增UI组件事件处理
function handleWelcomeGuideClose() {
  // 欢迎引导关闭时的处理
  localStorage.setItem('latex-welcome-seen', 'true')
}

function handleOpenRecentDocument(doc: any) {
  // 打开最近文档
  if (doc.type === 'project') {
    // 处理项目打开
  } else {
    // 处理单个文档打开
  }
  showRecentDocuments.value = false
}

function handleCreateNewDocument() {
  // 创建新文档
  showRecentDocuments.value = false
  handleFileCommand('blank')
}

async function handleCreateNewProject() {
  try {
	const { value: projectName } = await ElMessageBox.prompt('请输入项目名称', '新建LaTeX项目', {
	  confirmButtonText: '创建',
	  cancelButtonText: '取消',
	  inputPattern: /^.{1,50}$/,
	  inputErrorMessage: '项目名称长度为1-50个字符'
	})

	if (!projectName) return

	// 调用store创建项目
	await latexStore.createNewProject(projectName || 'Untitled')
	ElMessage.success(`项目 "${projectName}" 创建成功`)
  } catch (error) {
	if (error !== 'cancel') {
	 	console.error('Failed to create project:', error)
	  ElMessage.error('创建项目失败')
	}
  }
}

function handleSettingsUpdate(settings: any) {
  // 更新编辑器设置
  localStorage.setItem('latex-editor-settings', JSON.stringify(settings))
  // 应用设置到编辑器
  ElMessage.success('设置已更新')
}

function handleClearCache() {
  // 清除缓存
  localStorage.removeItem('latex-editor-settings')
  localStorage.removeItem('latex-autosave-backup')
  ElMessage.success('缓存已清除')
}

function handleRefreshStats() {
  // 统计已自动更新（通过computed属性）
  ElMessage.success('统计数据已刷新')
}

function handleExportStats(stats: any) {
  // 导出统计数据
  const dataStr = JSON.stringify(stats, null, 2)
  const dataBlob = new Blob([dataStr], { type: 'application/json' })
  const url = URL.createObjectURL(dataBlob)
  const link = document.createElement('a')
  link.href = url
  link.download = `stats-${Date.now()}.json`
  link.click()
  URL.revokeObjectURL(url)
  ElMessage.success('统计数据已导出')
}

function loadRecentDocuments() {
  // 加载最近文档列表
  loadingRecentDocs.value = true
  try {
    const stored = localStorage.getItem('latex-recent-docs')
    if (stored) {
      recentDocs.value = JSON.parse(stored)
    } else {
      recentDocs.value = []
    }
  } catch (e) {
    console.error('Failed to load recent documents:', e)
    recentDocs.value = []
  } finally {
    loadingRecentDocs.value = false
  }
}

// Batch 2 & 3 component event handlers
function handleInsertCitation(citationCommand: string) {
  // 插入BibTeX引用命令
  const editor = editorRef.value
  if (editor && editor.insertText) {
    editor.insertText(citationCommand)
  } else {
    editorContent.value += citationCommand
  }
  ElMessage.success('引用已插入')
}

function handleInsertMacro(macroCode: string) {
  // 插入LaTeX宏
  const editor = editorRef.value
  if (editor && editor.insertText) {
    editor.insertText(macroCode)
  } else {
    editorContent.value += macroCode
  }
  ElMessage.success('宏已插入')
}

function handleInsertImage(imageCode: string) {
  // 插入图片代码
  const editor = editorRef.value
  if (editor && editor.insertText) {
    editor.insertText(imageCode)
  } else {
    editorContent.value += imageCode
  }
  ElMessage.success('图片已插入')
}

function handleCheckerFix(fix: { type: string; replacement: string }) {
  if (fix.replacement) {
    editorContent.value = fix.replacement
    ElMessage.success('已自动修复')
  }
}

function handleJumpToLine(lineNumber: number) {
  // 跳转到指定行
  const editor = editorRef.value
  if (editor && editor.gotoLine) {
    editor.gotoLine(lineNumber)
  }
  // 如果使用的是Monaco编辑器
  if ((window as any).monacoEditor) {
    (window as any).monacoEditor.revealLineInCenter(lineNumber)
    (window as any).monacoEditor.setPosition({ lineNumber, column: 1 })
    (window as any).monacoEditor.focus()
  }
}

function handleCommandExecuted(command: any) {
  console.log('Command executed:', command)
  // 可以根据命令执行特定操作
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
  width: 100%;
  max-width: 100%; // 防止超出父容器
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);
  position: relative;
  isolation: isolate; // 创建新的层叠上下文，防止子元素重叠
  overflow: hidden; // 防止内容溢出导致重影
  box-sizing: border-box;

  // 确保EditorToolbar和FindReplacePanel占满宽度
  > .editor-toolbar {
    width: 100%;
    border-bottom: 1px solid var(--el-border-color-lighter);
    background: linear-gradient(
      to bottom,
      var(--el-bg-color-page) 0%,
      rgba(0, 0, 0, 0.02) 100%
    );
    flex-shrink: 0;
  }

  > :deep(.find-replace-panel) {
    width: 100%;
    flex-shrink: 0;
  }

  // 确保所有子元素正确渲染
  > * {
    position: relative;
    z-index: 1;
  }
}

// ==========================================
// 1. 按钮状态优化 - 简洁交互
// ==========================================

:deep(.el-button) {
  transition: background-color 150ms ease, border-color 150ms ease, color 150ms ease;
  position: relative;

  &:hover:not(:disabled) {
    // 移除 transform，只保留颜色变化
  }

  &:active:not(:disabled) {
    // 移除 transform
  }

  &.is-loading {
    position: relative;
    color: transparent !important;
    pointer-events: none;

    &::after {
      content: '';
      position: absolute;
      inset: 0;
      width: 14px;
      height: 14px;
      top: 50%;
      left: 50%;
      margin: -7px 0 0 -7px;
      border: 2px solid currentColor;
      border-right-color: transparent;
      border-radius: 50%;
      animation: spin 0.6s linear infinite;
    }
  }

  // 小尺寸按钮优化
  &.el-button--small {
    height: 28px;
    padding: 4px 10px;
    font-size: 13px;
  }
}

@keyframes spin {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

// ==========================================
// 2. 头部区域 - 极简设计
// ==========================================

.editor-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 20px;
  height: 44px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color-lighter);
  flex-shrink: 0;

  .header-left {
    flex: 1;
    display: flex;
    align-items: center;
    min-width: 0;

    .doc-title {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 14px;
      font-weight: 500;
      color: var(--el-text-color-primary);

      .title-text {
        max-width: 300px;
        overflow: hidden;
        text-overflow: ellipsis;
        white-space: nowrap;
      }

      .el-icon {
        font-size: 16px;
        color: var(--el-color-primary);
      }
    }
  }

  .header-right {
    display: flex;
    align-items: center;
    gap: 6px;
    flex-shrink: 0;

    .header-actions {
      display: flex;
      align-items: center;
      gap: 6px;

      .el-button {
        padding: 6px;
        border-radius: 8px;
        transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);

        &:hover {
          transform: translateY(-1px);
        }

        &.active {
          color: var(--el-color-primary);
          background: var(--el-color-primary-light-9);
        }

        .el-icon {
          font-size: 15px;
        }
      }
    }
  }
}

// ==========================================
// 3. 面板布局
// ==========================================

.editor-main {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  width: 100%;
  min-width: 0;

  // 编辑器和预览容器 - 水平布局
  .editor-preview-container {
    flex: 1 1 0;
    display: flex;
    flex-direction: row;
    overflow: hidden;
    min-height: 0;
    width: 100%;

    // 左右分屏模式（默认）
    &.side-by-side {
      flex-direction: row;
    }

    // 上下分屏模式：切换为纵向布局
    &.vertical-split {
      flex-direction: column;
    }
  }

  // 上下分屏模式：整个main容器也垂直
  &.vertical-split {
    flex-direction: column;
  }

  // 左右分屏模式（默认）
  &.side-by-side {
    flex-direction: column;
  }

  // 仅编辑器模式
  &.editor-only {
    flex-direction: column;

    .preview-panel {
      display: none;
    }
  }

  // 仅预览模式
  &.preview-only {
    flex-direction: column;

    .editor-panel {
      display: none;
    }
  }

  &.show-preview {
    .editor-panel {
      border-right: 1px solid var(--el-border-color-lighter);
    }
  }

  // vertical-split模式下的边框处理
  &.vertical-split.show-preview {
    .editor-panel {
      border-right: none;
      border-bottom: 1px solid var(--el-border-color-lighter);
    }
  }
}

.editor-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);
  width: 100%;
  min-width: 0; // 允许flex子项正确收缩

  // 左右分屏模式：编辑器占60%
  &.side-by-side.with-preview {
    flex: 0.6;
    width: 60%;
  }

  // 上下分屏模式：编辑器占50%高度
  &.vertical-split.with-preview {
    flex: 0.5;
    height: 50%;
    width: 100%;
  }

  .editor-content {
    flex: 1;
    display: flex;
    flex-direction: column;
    overflow: hidden;
    width: 100%;
    min-width: 0; // 允许flex子项正确收缩
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
    padding: 4px 12px;
    border-top: 1px solid var(--el-border-color-lighter);
    background: var(--el-fill-color-lighter);
    font-size: 12px;
    color: var(--el-text-color-secondary);
    min-height: 28px;
    flex-shrink: 0;

    .status-left {
      display: flex;
      gap: 12px;
    }

    .status-right {
      display: flex;
      gap: 12px;
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
// 4. 预览面板优化
// ==========================================

.preview-panel {
  flex: 0.4;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);
  width: 100%;
  min-width: 0; // 允许flex子项正确收缩

  // 左右分屏模式：预览占40%
  &.side-by-side {
    flex: 0.4;
    width: 40%;
  }

  // 上下分屏模式：预览占50%高度
  &.vertical-split {
    flex: 0.5;
    height: 50%;
    width: 100%;
  }

  // 仅预览模式：占满整个空间
  &.preview-only {
    flex: 1;
    width: 100%;
  }

  .preview-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 8px;
    padding: 10px 12px;
    border-bottom: 1px solid var(--el-border-color-lighter);
    background: linear-gradient(
      to bottom,
      var(--el-bg-color-page) 0%,
      rgba(0, 0, 0, 0.02) 100%
    );
    flex-wrap: nowrap;
    min-width: 0;

    h3 {
      margin: 0;
      font-size: 14px;
      font-weight: 600;
      color: var(--el-text-color-primary);
      display: flex;
      align-items: center;
      gap: 6px;
      white-space: nowrap;
      flex-shrink: 0;

      &::before {
        content: '';
        width: 3px;
        height: 14px;
        background: var(--el-color-primary);
        border-radius: 2px;
      }
    }

    .preview-controls {
      display: flex;
      gap: 4px;
      flex-shrink: 0;

      :deep(.el-button) {
        min-width: 28px;
        height: 28px;
        padding: 0 6px;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 13px;
        font-weight: 500;
      }
    }

    .preview-mode-toggle {
      display: flex;
      align-items: center;
      gap: 4px;
      margin-left: auto;
      flex-shrink: 0;

      :deep(.el-radio-group) {
        display: flex;
        gap: 2px;

        .el-radio-button {
          --el-radio-button-checked-bg-color: var(--el-color-primary);
          --el-radio-button-checked-text-color: #fff;

          .el-radio-button__inner {
            padding: 6px 10px;
            font-size: 12px;
            height: 28px;
          }
        }
      }
    }
  }

  .preview-content {
    flex: 1;
    overflow-y: auto;
    overflow-x: hidden;

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

    .pdf-placeholder {
      display: flex;
      align-items: center;
      justify-content: center;
      height: 100%;
      padding: 20px;
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
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color-light);
  padding: 8px 12px;
  box-shadow: 0 1px 2px rgba(0, 0, 0, 0.04);
  flex-shrink: 0;

  .mobile-tab {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    padding: 10px;
    border-radius: 8px;
    cursor: pointer;
    transition: all 0.2s ease;
    color: var(--el-text-color-secondary);
    font-weight: 500;
    font-size: 14px;
    min-height: 40px;
    position: relative;

    &.active {
      background: var(--el-color-primary);
      color: white;
      box-shadow: 0 2px 8px rgba(64, 158, 255, 0.3);
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
    // 移动端强制垂直布局
    .editor-main {
      flex-direction: column !important;

      &.vertical-split,
      &.side-by-side {
        flex-direction: column !important;
      }
    }

    // 编辑器和预览面板在移动端始终全宽
    .editor-panel,
    .preview-panel {
      width: 100% !important;
      flex: 1 !important;

      &.side-by-side,
      &.vertical-split {
        width: 100% !important;
        height: auto !important;
      }
    }

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
  flex-shrink: 1;
  min-width: 0;
  overflow: hidden;

  .el-breadcrumb__item {
    font-size: 13px;
    font-weight: 500;

    &:last-child {
      color: var(--el-text-color-primary);
      font-weight: 600;
      overflow: hidden;
      text-overflow: ellipsis;
    }
  }

  .el-breadcrumb__separator {
    color: var(--el-text-color-secondary);
    margin: 0 6px;
    flex-shrink: 0;
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

// 编辑器工具栏 - 优化紧凑设计
.editor-toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  background: var(--el-fill-color-extra-light);
  border-bottom: 1px solid var(--el-border-color-lighter);
  min-height: 42px;
  gap: 8px;
  flex-shrink: 0;

  .toolbar-left,
  .toolbar-right {
    display: flex;
    align-items: center;
    gap: 2px;
  }

  .toolbar-btn {
    border: none;
    background: transparent;
    color: var(--el-text-color-regular);
    font-size: 13px;
    padding: 5px 10px;
    height: 28px;
    border-radius: 4px;
    display: inline-flex;
    align-items: center;
    gap: 4px;
    transition: all 0.15s ease;
    white-space: nowrap;
    cursor: pointer;

    .el-icon {
      font-size: 15px;
    }

    &:hover:not(:disabled) {
      background: var(--el-fill-color);
      color: var(--el-color-primary);
    }

    &.active {
      background: var(--el-color-primary-light-9);
      color: var(--el-color-primary);
      font-weight: 500;
    }

    &:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }

    // 图标按钮（仅图标无文字）
    &:has(> .el-icon:only-child) {
      padding: 5px;
      width: 28px;
      justify-content: center;
    }
  }

  .el-divider--vertical {
    margin: 0 6px;
    height: 18px;
    border-color: var(--el-border-color);
  }

  // 下拉菜单优化
  :deep(.el-dropdown) {
    .toolbar-btn {
      .el-icon {
        font-size: 12px;
        margin-left: -2px;
      }
    }
  }

  // 工具提示优化
  :deep(.el-tooltip__trigger) {
    display: inline-flex;
  }
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