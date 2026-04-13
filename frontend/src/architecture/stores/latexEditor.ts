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
  pdfPath?: string;
  compileTimeMs?: number;
  error?: string;
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

  // 自动保存状态
  const isModified: Ref<boolean> = ref(false);
  const lastSavedTime: Ref<number | null> = ref(null);
  const autoSaveEnabled: Ref<boolean> = ref(true);
  const autoSaveInterval: Ref<number> = ref(30000); // 30秒

  // ==========================================
  // 项目状态（多文件支持）
  // ==========================================
  const isProjectMode: Ref<boolean> = ref(false);
  const currentProject: Ref<any | null> = ref(null);
  const currentProjectFile: Ref<any | null> = ref(null);
  const projectFiles: Ref<any[]> = ref([]);
  const allProjects: Ref<any[]> = ref([]); // 所有项目列表

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
    const template = getLatexTemplate();

    // 临时文档，无ID（null表示新建）
    const tempDoc: LatexDocument = {
      id: '',  // 空字符串表示待创建
      name,
      content: template,
      path: `/${name}`,
      lastModified: Date.now(),
      size: template.length,
      metadata: {
        documentClass: 'article',
        packages: ['amsmath', 'amsfonts', 'amssymb']
      }
    };

    setCurrentDocument(tempDoc);

    // 立即保存到后端获取真实ID
    try {
      const { latexApi } = await import('@/api/modules/latex');
      const result = await latexApi.saveDocument({
        name: tempDoc.name,
        content: tempDoc.content,
        path: tempDoc.path,
        metadata: tempDoc.metadata
      });

      if (result.success && result.document?.id) {
        // 更新为后端分配的真实ID
        tempDoc.id = String(result.document.id);
        setCurrentDocument(tempDoc);

        if (import.meta.env.DEV) {
          console.log('[LaTeX Store] New document created with backend ID:', tempDoc.id);
        }
      }
    } catch (error) {
      console.error('[LaTeX Store] Failed to create document on backend:', error);
    }

    return tempDoc;
  }

  function updateDocumentContent(content: string) {
    editorContent.value = content;
    markModified();

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
        if (import.meta.env.DEV) {
          console.log('Document saved successfully:', currentDocument.value.name);
        }
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
        // 确保pdfPath和compileTimeMs被正确传递
        if (response.result.output && !response.result.pdfPath) {
          // 兼容旧格式：output可能是pdfPath
          result.pdfPath = response.result.output;
        }
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

  // ==========================================
  // 自动保存系统
  // ==========================================
  let autoSaveTimer: NodeJS.Timeout | null = null;
  let saveDebounceTimer: NodeJS.Timeout | null = null;

  const AUTO_SAVE_STORAGE_KEY = 'latex_autosave_backup';

  function saveToLocalBackup() {
    if (!currentDocument.value) return;

    const backup = {
      id: currentDocument.value.id,
      name: currentDocument.value.name,
      content: editorContent.value,
      path: currentDocument.value.path,
      lastModified: Date.now(),
      metadata: currentDocument.value.metadata
    };

    try {
      localStorage.setItem(AUTO_SAVE_STORAGE_KEY, JSON.stringify(backup));
    } catch (error) {
      console.error('Failed to save to local backup:', error);
    }
  }

  function loadFromLocalBackup(): LatexDocument | null {
    try {
      const backup = localStorage.getItem(AUTO_SAVE_STORAGE_KEY);
      if (!backup) return null;

      return JSON.parse(backup) as LatexDocument;
    } catch (error) {
      console.error('Failed to load from local backup:', error);
      return null;
    }
  }

  function clearLocalBackup() {
    try {
      localStorage.removeItem(AUTO_SAVE_STORAGE_KEY);
    } catch (error) {
      console.error('Failed to clear local backup:', error);
    }
  }

  async function performAutoSave() {
    if (!autoSaveEnabled.value || !isModified.value || !currentDocument.value) {
      return;
    }

    await saveDocument();
    saveToLocalBackup();
    isModified.value = false;
    lastSavedTime.value = Date.now();

    if (import.meta.env.DEV) {
      console.log('[AutoSave] Document saved automatically at', new Date(lastSavedTime.value).toLocaleTimeString());
    }
  }

  function startAutoSave() {
    stopAutoSave();

    if (!autoSaveEnabled.value) return;

    autoSaveTimer = setInterval(() => {
      performAutoSave();
    }, autoSaveInterval.value);

    if (import.meta.env.DEV) {
      console.log(`[AutoSave] Started with ${autoSaveInterval.value / 1000}s interval`);
    }
  }

  function stopAutoSave() {
    if (autoSaveTimer) {
      clearInterval(autoSaveTimer);
      autoSaveTimer = null;
    }
    if (saveDebounceTimer) {
      clearTimeout(saveDebounceTimer);
      saveDebounceTimer = null;
    }
  }

  function triggerAutoSave(debounceMs: number = 1000) {
    if (!autoSaveEnabled.value) return;

    if (saveDebounceTimer) {
      clearTimeout(saveDebounceTimer);
    }

    saveDebounceTimer = setTimeout(() => {
      performAutoSave();
    }, debounceMs);
  }

  function markModified() {
    isModified.value = true;
  }

  function setAutoSaveEnabled(enabled: boolean) {
    autoSaveEnabled.value = enabled;
    if (enabled) {
      startAutoSave();
    } else {
      stopAutoSave();
    }
  }

  function setAutoSaveInterval(intervalMs: number) {
    autoSaveInterval.value = Math.max(5000, intervalMs);
    if (autoSaveEnabled.value) {
      startAutoSave();
    }
  }

  // 清理函数
  function cleanup() {
    stopAutoSave();
    stopCollaboration();
  }

  // ==========================================
  // 项目管理方法（多文件支持）
  // ==========================================

  async function loadProject(projectId: number) {
    try {
      isLoading.value = true
      const { listLatexProjects, getLatexProject } = await import('@/api/adapters/latexAdapter')
      const project = await getLatexProject(projectId)

      if (import.meta.env.DEV) {
        console.log('[LaTeX Store] Loading project:', project.name, 'files:', project.files.length)
      }

      currentProject.value = project
      projectFiles.value = project.files
      isProjectMode.value = true

      // 清除单文档模式状态, 避免干扰computedEditorContent
      currentDocument.value = null

      // 默认打开主文件 (匹配 mainFile 或 mainFile.tex)
      const mainFile = project.files.find(f =>
        f.path === project.mainFile ||
        f.path === `${project.mainFile}.tex` ||
        f.name === project.mainFile ||
        f.name === `${project.mainFile}.tex`
      )
      if (mainFile) {
        currentProjectFile.value = mainFile

        if (import.meta.env.DEV) {
          console.log('[LaTeX Store] Main file found:', mainFile.path, 'content length:', mainFile.content?.length || 0)
        }

        editorContent.value = mainFile.content || ''
      } else {
        console.warn('[LaTeX Store] Main file not found:', project.mainFile, 'available files:', project.files.map(f => f.path))

        // 如果没有找到主文件，使用第一个文件
        if (project.files.length > 0) {
          currentProjectFile.value = project.files[0]
          editorContent.value = project.files[0].content || ''
          console.log('[LaTeX Store] Using first file as fallback:', project.files[0].path)
        }
      }

      return project
    } catch (error) {
      console.error('Failed to load project:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  function switchProjectFile(file: any) {
    currentProjectFile.value = file
    editorContent.value = file.content || ''
  }

  async function saveCurrentProjectFile() {
    if (!currentProject.value || !currentProjectFile.value) return

    try {
      const { updateProjectFile } = await import('@/api/adapters/latexAdapter')
      await updateProjectFile(currentProjectFile.value.id, editorContent.value)

      currentProjectFile.value.content = editorContent.value
      // 更新项目文件列表中的内容
      const idx = projectFiles.value.findIndex(f => f.id === currentProjectFile.value!.id)
      if (idx !== -1) {
        projectFiles.value[idx].content = editorContent.value
      }
    } catch (error) {
      console.error('Failed to save project file:', error)
      throw error
    }
  }

  async function createProjectFile(fileData: { name: string; path: string; type: string }) {
    if (!currentProject.value) return

    try {
      const { addProjectFile } = await import('@/api/adapters/latexAdapter')
      const newFile = await addProjectFile({
        projectId: currentProject.value.id,
        name: fileData.name,
        path: fileData.path,
        content: '',
        type: fileData.type
      })

      projectFiles.value.push(newFile)
      return newFile
    } catch (error) {
      console.error('Failed to create project file:', error)
      throw error
    }
  }

  async function deleteProjectFile(fileId: number) {
    if (!currentProject.value) return

    try {
      const { deleteProjectFile } = await import('@/api/adapters/latexAdapter')
      await deleteProjectFile(fileId)

      projectFiles.value = projectFiles.value.filter(f => f.id !== fileId)

      // 如果删除的是当前文件，切换到主文件
      if (currentProjectFile.value?.id === fileId) {
        const mainFile = projectFiles.value.find(f => f.path === currentProject.value.mainFile)
        if (mainFile) {
          switchProjectFile(mainFile)
        }
      }
    } catch (error) {
      console.error('Failed to delete project file:', error)
      throw error
    }
  }

  async function compileProject() {
    if (!currentProject.value) return

    try {
      compilationStatus.value = 'compiling'
      const startTime = Date.now()

      const { compileLatexProject } = await import('@/api/adapters/latexAdapter')
      const result = await compileLatexProject(currentProject.value.id)

      const duration = Date.now() - startTime
      compilationResult.value = {
        success: result.success,
        output: result.log,
        pdfPath: result.pdfPath,
        compileTimeMs: result.compileTimeMs,
        error: result.error,
        errors: result.error ? [{
          line: 0,
          message: result.error,
          type: 'error'
        }] : [],
        warnings: [],
        log: result.log || '',
        duration
      }

      compilationStatus.value = result.success ? 'success' : 'error'
      lastCompilationTime.value = duration

      return compilationResult.value
    } catch (error) {
      compilationStatus.value = 'error'
      console.error('Failed to compile project:', error)
      throw error
    }
  }

  function setProjectMode(enabled: boolean) {
    isProjectMode.value = enabled
    if (!enabled) {
      currentProject.value = null
      currentProjectFile.value = null
      projectFiles.value = []
    }
  }

  // ==========================================
  // 多项目列表管理
  // ==========================================

  async function loadAllProjects() {
    try {
      isLoading.value = true
      const { listLatexProjects } = await import('@/api/adapters/latexAdapter')
      const result = await listLatexProjects({ limit: 100 })
      allProjects.value = result.items
      return result.items
    } catch (error) {
      console.error('Failed to load projects list:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  async function switchProject(projectId: number) {
    try {
      isLoading.value = true

      if (import.meta.env.DEV) {
        console.log('[LaTeX Store] Switching to project:', projectId, 'current project:', currentProject.value?.id)
      }

      // 先保存当前项目
      if (currentProject.value && isModified.value) {
        if (import.meta.env.DEV) {
          console.log('[LaTeX Store] Saving current project before switch')
        }
        await saveCurrentProjectFile()
      }

      // 重置状态
      isModified.value = false
      currentProjectFile.value = null

      // 加载新项目
      const project = await loadProject(projectId)

      if (import.meta.env.DEV) {
        console.log('[LaTeX Store] Project switched successfully:', project.name, 'isProjectMode:', isProjectMode.value)
      }

      return currentProject.value
    } catch (error) {
      console.error('[LaTeX Store] Failed to switch project:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  async function createNewProject(name: string, description: string = '') {
    try {
      isLoading.value = true
      const { createLatexProject } = await import('@/api/adapters/latexAdapter')
      const project = await createLatexProject({
        name,
        mainFile: 'main.tex',
        description,
        isPublic: false
      })

      // 刷新项目列表
      await loadAllProjects()

      // 加载新创建的项目
      await loadProject(project.id)
      isProjectMode.value = true

      return project
    } catch (error) {
      console.error('Failed to create project:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  async function deleteProject(projectId: number) {
    try {
      isLoading.value = true
      const { deleteLatexProject } = await import('@/api/adapters/latexAdapter')
      await deleteLatexProject(projectId)

      // 从列表中移除
      allProjects.value = allProjects.value.filter(p => p.id !== projectId)

      // 如果删除的是当前项目，退出项目模式
      if (currentProject.value?.id === projectId) {
        setProjectMode(false)
      }

      return true
    } catch (error) {
      console.error('Failed to delete project:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  async function refreshProjectsList() {
    return await loadAllProjects()
  }

  return {
    // 状态
    currentDocument,
    documents,
    editorContent, // 导出原始 ref，允许读写
    computedEditorContent, // 导出 computed 版本供只读使用
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

    // 项目状态
    isProjectMode,
    currentProject,
    currentProjectFile,
    projectFiles,
    allProjects,

    // 自动保存状态
    isModified,
    lastSavedTime,
    autoSaveEnabled,
    autoSaveInterval,

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
    cleanup,

    // 项目方法
    loadProject,
    switchProjectFile,
    saveCurrentProjectFile,
    createProjectFile,
    deleteProjectFile,
    compileProject,
    setProjectMode,

    // 多项目列表方法
    loadAllProjects,
    switchProject,
    createNewProject,
    deleteProject,
    refreshProjectsList,

    // 自动保存方法
    startAutoSave,
    stopAutoSave,
    performAutoSave,
    triggerAutoSave,
    markModified,
    setAutoSaveEnabled,
    setAutoSaveInterval,
    saveToLocalBackup,
    loadFromLocalBackup,
    clearLocalBackup
  };
});

// 持久化配置 - 需要在main.ts中配置pinia-plugin-persistedstate