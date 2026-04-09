import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import type { Ref } from 'vue'
import { latexApi } from '@/api/modules/latex'

export interface LatexDocument {
  id: string;
  name: string;
  content: string;
  path: string;
  lastModified: number;
  size: number;
  metadata: {
    title?: string;
    authors?: string[];
    abstract?: string;
    keywords?: string[];
    documentClass?: string;
    packages?: string[];
  };
}

export interface CompilationResult {
  success: boolean;
  output?: string;
  errors?: CompilationError[];
  warnings?: CompilationWarning[];
  log?: string;
  duration: number;
}

export interface CompilationError {
  line: number;
  column?: number;
  message: string;
  type: 'error' | 'warning' | 'info';
  file?: string;
}

export interface CompilationWarning {
  line: number;
  message: string;
  type: 'warning' | 'info';
}

export interface EditorSettings {
  fontSize: number;
  fontFamily: string;
  lineHeight: number;
  wordWrap: 'on' | 'off' | 'wordWrapColumn' | 'bounded';
  wordWrapColumn: number;
  minimap: {
    enabled: boolean;
    size: 'proportional' | 'fill' | 'fit';
    showSlider: 'always' | 'mouseover';
  };
  lineNumbers: 'on' | 'off' | 'relative' | 'interval';
  rulers: number[];
  bracketPairColorization: {
    enabled: boolean;
  };
  autoIndent: 'none' | 'keep' | 'brackets' | 'advanced' | 'full';
  formatOnType: boolean;
  formatOnPaste: boolean;
  suggest: {
    showKeywords: boolean;
    showSnippets: boolean;
    showClasses: boolean;
    showFunctions: boolean;
    showVariables: boolean;
    showConstants: boolean;
  };
}

export interface PreviewSettings {
  theme: 'light' | 'dark';
  scale: number;
  autoRefresh: boolean;
  refreshDelay: number;
  showSourceMap: boolean;
  scrollSync: boolean;
}

export interface CollaborationUser {
  id: string;
  name: string;
  color: string;
  cursor?: {
    line: number;
    column: number;
  };
  selection?: {
    startLine: number;
    startColumn: number;
    endLine: number;
    endColumn: number;
  };
  isOnline: boolean;
  lastSeen: number;
}

export interface CollaborationSession {
  id: string;
  documentId: string;
  users: Map<string, CollaborationUser>;
  activeUsers: string[];
  lastActivity: number
  createdAt: number;
  permissions: {
    canEdit: boolean;
    canComment: boolean;
    canShare: boolean;
  };
}

export type CompilationStatus = 'idle' | 'compiling' | 'success' | 'error' | 'warning';
export type PreviewMode = 'split' | 'preview-only' | 'editor-only';

export const useLatexEditorStore = defineStore('latexEditor', () => {
  // 当前文档状态
  const currentDocument: Ref<LatexDocument | null> = ref(null);
  const documents: Ref<Map<string, LatexDocument>> = ref(new Map());

  // 编辑器状态
  const editorContent: Ref<string> = ref('');
  const editorSelection: Ref<{ start: number; end: number } | null> = ref(null);
  const editorCursor: Ref<{ line: number; column: number }> = ref({ line: 1, column: 1 });

  // 编译状态
  const compilationStatus: Ref<CompilationStatus> = ref('idle');
  const compilationResult: Ref<CompilationResult | null> = ref(null);
  const compilationProgress: Ref<number> = ref(0);
  const lastCompilationTime: Ref<number | null> = ref(null);

  // 预览状态
  const previewMode: Ref<PreviewMode> = ref('split');
  const previewContent: Ref<string> = ref('');
  const previewScale: Ref<number> = ref(1.0);
  const previewTheme: Ref<'light' | 'dark'> = ref('light');

  // 协作状态
  const collaborationSession: Ref<CollaborationSession | null> = ref(null);
  const isCollaborating: Ref<boolean> = ref(false);
  const collaborationUsers: Ref<Map<string, CollaborationUser>> = ref(new Map());

  // 设置状态
  const editorSettings: Ref<EditorSettings> = ref({
    fontSize: 14,
    fontFamily: 'Fira Code, Consolas, Monaco, "Courier New", monospace',
    lineHeight: 1.6,
    wordWrap: 'on',
    wordWrapColumn: 80,
    minimap: {
      enabled: true,
      size: 'proportional',
      showSlider: 'mouseover'
    },
    lineNumbers: 'on',
    rulers: [],
    bracketPairColorization: {
      enabled: true
    },
    autoIndent: 'full',
    formatOnType: true,
    formatOnPaste: true,
    suggest: {
      showKeywords: true,
      showSnippets: true,
      showClasses: true,
      showFunctions: true,
      showVariables: true,
      showConstants: true
    }
  });

  const previewSettings: Ref<PreviewSettings> = ref({
    theme: 'light',
    scale: 1.0,
    autoRefresh: true,
    refreshDelay: 1000,
    showSourceMap: false,
    scrollSync: true
  });

  // 错误和警告状态
  const errors: Ref<CompilationError[]> = ref([]);
  const warnings: Ref<CompilationWarning[]> = ref([]);
  const hasErrors: Ref<boolean> = ref(false);
  const hasWarnings: Ref<boolean> = ref(false);

  // UI状态
  const isLoading: Ref<boolean> = ref(false);
  const sidebarVisible: Ref<boolean> = ref(true);
  const toolbarVisible: Ref<boolean> = ref(true);
  const statusBarVisible: Ref<boolean> = ref(true);
  const currentTheme: Ref<'light' | 'dark'> = ref('light');

  // 计算属性
  const computedEditorContent = computed(() => {
    return currentDocument.value?.content || editorContent.value;
  });

  const computedPreviewContent = computed(() => {
    return compilationResult.value?.success ? previewContent.value : '';
  });

  const compilationSuccess = computed(() => {
    return compilationStatus.value === 'success' && !hasErrors.value;
  });

  const activeCollaborationUsers = computed(() => {
    return Array.from(collaborationUsers.value.values())
      .filter(user => user.isOnline)
      .sort((a, b) => a.name.localeCompare(b.name));
  });

  const documentStats = computed(() => {
    const content = computedEditorContent.value;
    const lines = content.split('\n');
    const words = content.trim() ? content.trim().split(/\s+/).length : 0;
    const characters = content.length;
    const charactersNoSpaces = content.replace(/\s/g, '').length;

    return {
      lines: lines.length,
      words,
      characters,
      charactersNoSpaces,
      paragraphs: content.split(/\n\s*\n/).filter(p => p.trim()).length
    };
  });

  // 操作方法

  // 文档管理
  function setCurrentDocument(document: LatexDocument) {
    currentDocument.value = document;
    editorContent.value = document.content;
    documents.value.set(document.id, document);
  }

  async function createNewDocument(name: string = 'Untitled.tex'): Promise<LatexDocument> {
    // 直接使用默认模板，避免API调用
    const template = getLatexTemplate();

    const newDocument: LatexDocument = {
      id: `doc_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
      name,
      content: template,
      path: `/${name}`,
      lastModified: Date.now(),
      size: 0,
      metadata: {
        documentClass: 'article',
        packages: ['amsmath', 'amsfonts', 'amssymb']
      }
    };

    setCurrentDocument(newDocument);
    return newDocument;
  }

  function updateDocumentContent(content: string) {
    editorContent.value = content;

    if (currentDocument.value) {
      currentDocument.value.content = content;
      currentDocument.value.lastModified = Date.now();
      currentDocument.value.size = new Blob([content]).size;
    }
  }

  async function saveDocument() {
    if (!currentDocument.value) return;

    try {
      currentDocument.value.lastModified = Date.now();
      currentDocument.value.content = editorContent.value;

      const response = await latexApi.saveDocument({
        id: currentDocument.value.id,
        name: currentDocument.value.name,
        content: currentDocument.value.content,
        path: currentDocument.value.path,
        metadata: currentDocument.value.metadata
      });

      if (response.success) {
        console.log('Document saved successfully:', currentDocument.value.name);
      } else {
        console.error('Document save failed:', response.error);
      }
    } catch (error) {
      console.error('Document save error:', error);
    }
  }

  // 编译控制
  async function compileDocument(): Promise<CompilationResult> {
    compilationStatus.value = 'compiling';
    compilationProgress.value = 0;
    const startTime = Date.now();

    try {
      const content = computedEditorContent.value;

      // 调用后端API进行编译
      const response = await latexApi.compile({
        content,
        documentClass: currentDocument.value?.metadata.documentClass || 'article',
        packages: currentDocument.value?.metadata.packages || ['amsmath', 'amsfonts', 'amssymb'],
        options: {
          timeout: 30000,
          format: 'pdf'
        }
      });

      const duration = Date.now() - startTime;

      let result: CompilationResult;

      if (response.success && response.result) {
        result = response.result;
        result.duration = duration;
      } else {
        // 如果API调用失败，使用客户端验证作为后备
        const mockErrors = validateLatexContent(content);
        result = {
          success: mockErrors.length === 0,
          output: mockErrors.length === 0 ? 'Compilation successful' : 'Compilation failed',
          errors: mockErrors,
          warnings: [],
          log: 'Client-side compilation log',
          duration
        };
      }

      compilationResult.value = result;
      compilationStatus.value = result.success ? 'success' : 'error';
      lastCompilationTime.value = Date.now();

      errors.value = result.errors || [];
      warnings.value = result.warnings || [];
      hasErrors.value = (result.errors?.length || 0) > 0;
      hasWarnings.value = (result.warnings?.length || 0) > 0;

      return result;
    } catch (error) {
      compilationStatus.value = 'error';
      const result: CompilationResult = {
        success: false,
        errors: [{
          line: 1,
          message: error instanceof Error ? error.message : 'Compilation failed',
          type: 'error'
        }],
        duration: Date.now() - startTime
      };

      compilationResult.value = result;
      return result;
    }
  }

  function validateLatexContent(content: string): CompilationError[] {
    const errors: CompilationError[] = [];
    const lines = content.split('\n');

    lines.forEach((line, index) => {
      // 检查未闭合的花括号
      const openBraces = (line.match(/\{/g) || []).length;
      const closeBraces = (line.match(/\}/g) || []).length;
      if (openBraces !== closeBraces) {
        errors.push({
          line: index + 1,
          message: 'Unmatched braces',
          type: 'error'
        });
      }

      // 检查未闭合的数学环境
      if (line.includes('$') && (line.match(/\$/g) || []).length % 2 !== 0) {
        errors.push({
          line: index + 1,
          message: 'Unmatched math delimiters',
          type: 'warning'
        });
      }
    });

    return errors;
  }

  // 预览控制
  function setPreviewMode(mode: PreviewMode) {
    previewMode.value = mode;
  }

  function updatePreview(content: string) {
    previewContent.value = content;
  }

  function setPreviewScale(scale: number) {
    previewScale.value = Math.max(0.5, Math.min(2.0, scale));
  }

  // 协作功能
  function startCollaboration(documentId: string) {
    isCollaborating.value = true;
    collaborationSession.value = {
      id: `session_${Date.now()}`,
      documentId,
      users: new Map(),
      activeUsers: [],
      lastActivity: Date.now(),
      createdAt: Date.now(),
      permissions: {
        canEdit: true,
        canComment: true,
        canShare: true
      }
    };
  }

  function stopCollaboration() {
    isCollaborating.value = false;
    collaborationSession.value = null;
    collaborationUsers.value.clear();
  }

  function addCollaborationUser(user: CollaborationUser) {
    collaborationUsers.value.set(user.id, user);
  }

  function removeCollaborationUser(userId: string) {
    collaborationUsers.value.delete(userId);
  }

  function updateUserCursor(userId: string, position: { line: number; column: number }) {
    const user = collaborationUsers.value.get(userId);
    if (user) {
      user.cursor = position;
      user.lastSeen = Date.now();
    }
  }

  function updateCursorPosition(position: { line: number; column: number }) {
    editorCursor.value = position;
  }

  // 设置管理
  function updateEditorSettings(settings: Partial<EditorSettings>) {
    editorSettings.value = {
      ...editorSettings.value,
      ...settings
    };
  }

  function updatePreviewSettings(settings: Partial<PreviewSettings>) {
    previewSettings.value = {
      ...previewSettings.value,
      ...settings
    };
  }

  function setTheme(theme: 'light' | 'dark') {
    currentTheme.value = theme;
    previewTheme.value = theme;
  }

  // 工具函数
  function getLatexTemplate(): string {
    return `\\documentclass{article}
\\usepackage{amsmath}
\\usepackage{amsfonts}
\\usepackage{amssymb}
\\usepackage{graphicx}
\\usepackage{hyperref}

\\title{Your Document Title}
\\author{Your Name}
\\date{\\today}

\\begin{document}

\\maketitle

\\begin{abstract}
Your abstract here.
\\end{abstract}

\\section{Introduction}
Your introduction here.

\\section{Main Content}
Write your main content here.

\\subsection{Mathematical Content}
Here is an example of mathematical content:

$$
E = mc^2
$$

Or inline math: $a^2 + b^2 = c^2$

\\section{Conclusion}
Your conclusion here.

\\bibliographystyle{plain}
\\bibliography{references}

\\end{document}`;
  }

  // 自动保存 (将在第7-10周实现)
  let autoSaveTimer: NodeJS.Timeout | null = null;

  function stopAutoSave() {
    if (autoSaveTimer) {
      clearInterval(autoSaveTimer);
      autoSaveTimer = null;
    }
  }

  // 清理函数
  function cleanup() {
    stopAutoSave();
    stopCollaboration();
  }

  return {
    // 状态
    currentDocument,
    documents,
    editorContent: computedEditorContent,
    editorSelection,
    editorCursor,
    compilationStatus,
    compilationResult,
    compilationProgress,
    lastCompilationTime,
    previewMode,
    previewContent: computedPreviewContent,
    previewScale,
    previewTheme,
    collaborationSession,
    isCollaborating,
    collaborationUsers,
    editorSettings,
    previewSettings,
    errors,
    warnings,
    hasErrors,
    hasWarnings,
    isLoading,
    sidebarVisible,
    toolbarVisible,
    statusBarVisible,
    currentTheme,

    // 计算属性
    compilationSuccess,
    activeCollaborationUsers,
    documentStats,

    // 方法
    setCurrentDocument,
    createNewDocument,
    updateDocumentContent,
    saveDocument,
    compileDocument,
    setPreviewMode,
    updatePreview,
    setPreviewScale,
    startCollaboration,
    stopCollaboration,
    addCollaborationUser,
    removeCollaborationUser,
    updateUserCursor,
    updateCursorPosition,
    updateEditorSettings,
    updatePreviewSettings,
    setTheme,
    cleanup
  };
});

// 持久化配置 - 需要在main.ts中配置pinia-plugin-persistedstate