import * as monaco from 'monaco-editor';
import { ref, onMounted, onUnmounted, watch } from 'vue';

export interface MonacoEditorProps {
  modelValue: string;
  language?: string;
  theme?: string;
  options?: monaco.editor.IStandaloneEditorConstructionOptions;
  readonly?: boolean;
}

export interface MonacoEditorEmits {
  'update:modelValue': [value: string];
  'change': [value: string];
  'editor-ready': [editor: monaco.editor.IStandaloneCodeEditor];
}

export interface CollaborationCursor {
  userId: string;
  userName: string;
  color: string;
  position: { line: number; column: number };
  selection?: {
    startLine: number;
    startColumn: number;
    endLine: number;
    endColumn: number;
  };
}

export class MonacoEditorManager {
  private editor: monaco.editor.IStandaloneCodeEditor | null = null;
  private container: HTMLElement | null = null;
  private disposables: monaco.IDisposable[] = [];
  private collaborationCursors: Map<string, monaco.editor.IEditorDecorationsCollection> = new Map();

  constructor(
    private containerRef: Ref<HTMLElement | null>,
    private props: MonacoEditorProps,
    private emit: (event: keyof MonacoEditorEmits, ...args: any[]) => void
  ) {}

  async initialize() {
    if (!this.containerRef.value) {
      throw new Error('Container element not found');
    }

    this.container = this.containerRef.value;

    console.log('Initializing Monaco editor...', {
      container: this.container,
      props: this.props
    });

    try {
      // 配置Monaco Editor
      this.configureMonaco();

      // 创建编辑器实例
      this.editor = monaco.editor.create(this.container, {
        value: this.props.modelValue,
        language: this.props.language || 'latex',
        theme: this.props.theme || 'vs-light',
        readOnly: this.props.readonly || false,
        automaticLayout: true,
        minimap: { enabled: true },
        scrollBeyondLastLine: false,
        fontSize: 14,
        lineNumbers: 'on',
        wordWrap: 'on',
        wrappingIndent: 'indent',
        scrollbar: {
          vertical: 'visible',
          horizontal: 'visible',
          useShadows: false,
          verticalScrollbarSize: 10,
          horizontalScrollbarSize: 10
        },
        ...this.props.options
      });

      console.log('Monaco editor created successfully');

      // 设置事件监听器
      this.setupEventListeners();

      // 注册LaTeX语言支持
      await this.registerLatexLanguage();

      this.emit('editor-ready', this.editor);

      return this.editor;
    } catch (error) {
      console.error('Failed to initialize Monaco editor:', error);
      throw error;
    }
  }

  private configureMonaco() {
    // 配置Monaco环境 - 对于Vite，使用CDN或静态资源
    if (typeof window !== 'undefined') {
      (window as any).MonacoEnvironment = {
        getWorkerUrl: function (_moduleId: string, label: string) {
          const baseUrl = 'https://cdn.jsdelivr.net/npm/monaco-editor@0.55.1/min/vs';
          if (label === 'typescript' || label === 'javascript') {
            return `${baseUrl}/language/typescript/tsWorker.js`;
          }
          if (label === 'json') {
            return `${baseUrl}/language/json/jsonWorker.js`;
          }
          if (label === 'css' || label === 'scss' || label === 'less') {
            return `${baseUrl}/language/css/cssWorker.js`;
          }
          if (label === 'html' || label === 'handlebars' || label === 'razor') {
            return `${baseUrl}/language/html/htmlWorker.js`;
          }
          return `${baseUrl}/base/worker/workerMain.js`;
        }
      };
    }
  }

  private setupEventListeners() {
    if (!this.editor) return;

    // 内容变化事件
    const contentChangeListener = this.editor.onDidChangeModelContent((_e) => {
      const value = this.editor?.getValue() || '';
      this.emit('update:modelValue', value);
      this.emit('change', value);
    });

    // 光标位置变化事件
    const cursorChangeListener = this.editor.onDidChangeCursorPosition((e) => {
      // 同步协作光标位置
      const position = { line: e.position.lineNumber, column: e.position.column };
      // 这里可以通过事件或回调传递光标位置用于协作
      console.log('Cursor moved:', position);
    });

    // 选择变化事件
    const selectionChangeListener = this.editor.onDidChangeCursorSelection((_e) => {
      // 可以在这里添加选择同步逻辑
    });

    this.disposables.push(
      contentChangeListener,
      cursorChangeListener,
      selectionChangeListener
    );
  }

  private async registerLatexLanguage() {
    try {
      // 注册LaTeX语言
      monaco.languages.register({ id: 'latex' });

      // 配置LaTeX语言特性
      monaco.languages.setMonarchTokensProvider('latex', {
        tokenizer: {
          root: [
            // LaTeX命令
            [/\\[a-zA-Z@]+/, 'keyword'],
            [/\\[^a-zA-Z@]/, 'keyword'],

            // 注释
            [/%.+$/, 'comment'],

            // 数学环境
            [/\$[^$]*\$/, 'string'],
            [/\$\$[^$]*\$\$/, 'string'],

            // 分组
            [/[{}]/, 'delimiter.curly'],
            [/[\[\]]/, 'delimiter.square'],

            // 特殊字符
            [/[{}[\],.]/, 'delimiter'],
          ]
        }
      });

      // 设置主题
      monaco.editor.defineTheme('latex-light', {
        base: 'vs',
        inherit: true,
        rules: [
          { token: 'keyword', foreground: '0000FF', fontStyle: 'bold' },
          { token: 'comment', foreground: '008000', fontStyle: 'italic' },
          { token: 'string', foreground: 'FF0000' },
          { token: 'delimiter', foreground: '800080' }
        ],
        colors: {
          'editor.background': '#ffffff',
          'editor.foreground': '#000000'
        }
      });

      monaco.editor.defineTheme('latex-dark', {
        base: 'vs-dark',
        inherit: true,
        rules: [
          { token: 'keyword', foreground: '569CD6', fontStyle: 'bold' },
          { token: 'comment', foreground: '6A9955', fontStyle: 'italic' },
          { token: 'string', foreground: 'CE9178' },
          { token: 'delimiter', foreground: 'D4D4D4' }
        ],
        colors: {
          'editor.background': '#1e1e1e',
          'editor.foreground': '#d4d4d4'
        }
      });

      // 应用主题
      monaco.editor.setTheme(this.props.theme || 'latex-light');

      console.log('LaTeX language registered successfully');
    } catch (error) {
      console.error('Failed to register LaTeX language:', error);
      // 继续执行，不阻止编辑器初始化
    }
  }

  private getLatexSuggestions(range: monaco.IRange): monaco.languages.CompletionItem[] {
    return [
      // 基础命令
      {
        label: '\\documentclass',
        kind: monaco.languages.CompletionItemKind.Snippet,
        insertText: '\\documentclass{article}',
        documentation: 'Document class declaration',
        range: range
      },
      {
        label: '\\begin',
        kind: monaco.languages.CompletionItemKind.Snippet,
        insertText: '\\begin{${1:environment}}\n\t$0\n\\end{${1:environment}}',
        insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
        documentation: 'Environment begin/end',
        range: range
      },
      {
        label: '\\section',
        kind: monaco.languages.CompletionItemKind.Snippet,
        insertText: '\\section{${1:title}}',
        insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
        documentation: 'Section heading',
        range: range
      },
      {
        label: '\\textbf',
        kind: monaco.languages.CompletionItemKind.Snippet,
        insertText: '\\textbf{${1:text}}',
        insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
        documentation: 'Bold text',
        range: range
      },
      {
        label: '\\textit',
        kind: monaco.languages.CompletionItemKind.Snippet,
        insertText: '\\textit{${1:text}}',
        insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
        documentation: 'Italic text',
        range: range
      },
      // 数学命令
      {
        label: '\\frac',
        kind: monaco.languages.CompletionItemKind.Snippet,
        insertText: '\\frac{${1:numerator}}{${2:denominator}}',
        insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
        documentation: 'Fraction',
        range: range
      },
      {
        label: '\\sqrt',
        kind: monaco.languages.CompletionItemKind.Snippet,
        insertText: '\\sqrt{${1:expression}}',
        insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
        documentation: 'Square root',
        range: range
      },
      {
        label: '\\sum',
        kind: monaco.languages.CompletionItemKind.Snippet,
        insertText: '\\sum_{${1:i=1}}^{${2:n}} ${3:expression}',
        insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
        documentation: 'Summation',
        range: range
      }
    ];
  }

  private getLatexHoverInfo(command: string): string | null {
    const hoverInfo: Record<string, string> = {
      '\\documentclass': 'Specifies the document class (article, report, book, etc.)',
      '\\usepackage': 'Loads additional packages for extended functionality',
      '\\begin': 'Begins an environment',
      '\\end': 'Ends an environment',
      '\\section': 'Creates a new section',
      '\\subsection': 'Creates a new subsection',
      '\\textbf': 'Makes text bold',
      '\\textit': 'Makes text italic',
      '\\underline': 'Underlines text',
      '\\emph': 'Emphasizes text (usually italic)',
      '\\frac': 'Creates a fraction',
      '\\sqrt': 'Creates a square root',
      '\\sum': 'Creates a summation symbol',
      '\\int': 'Creates an integral symbol',
      '\\lim': 'Creates a limit expression',
      '\\alpha': 'Greek letter alpha (α)',
      '\\beta': 'Greek letter beta (β)',
      '\\gamma': 'Greek letter gamma (γ)',
      '\\delta': 'Greek letter delta (δ)',
      '\\theta': 'Greek letter theta (θ)',
      '\\lambda': 'Greek letter lambda (λ)',
      '\\mu': 'Greek letter mu (μ)',
      '\\pi': 'Greek letter pi (π)',
      '\\sigma': 'Greek letter sigma (σ)',
      '\\phi': 'Greek letter phi (φ)',
      '\\omega': 'Greek letter omega (ω)',
      '\\leq': 'Less than or equal to (≤)',
      '\\geq': 'Greater than or equal to (≥)',
      '\\neq': 'Not equal to (≠)',
      '\\approx': 'Approximately equal to (≈)',
      '\\equiv': 'Equivalent to (≡)',
      '\\rightarrow': 'Right arrow (→)',
      '\\leftarrow': 'Left arrow (←)',
      '\\infty': 'Infinity symbol (∞)'
    };

    return hoverInfo[command] || null;
  }

  private getLatexFoldingRanges(model: monaco.editor.ITextModel): monaco.languages.FoldingRange[] {
    const ranges: monaco.languages.FoldingRange[] = [];
    const lines = model.getLinesContent();
    const stack: { line: number; type: string }[] = [];

    for (let i = 0; i < lines.length; i++) {
      const line = lines[i];

      // Check for begin environment
      const beginMatch = line.match(/^\\begin\{([^}]+)\}/);
      if (beginMatch) {
        stack.push({ line: i, type: beginMatch[1] });
      }

      // Check for end environment
      const endMatch = line.match(/^\\end\{([^}]+)\}/);
      if (endMatch && stack.length > 0) {
        const last = stack[stack.length - 1];
        if (last.type === endMatch[1]) {
          const startLine = last.line;
          const endLine = i;

          if (endLine > startLine) {
            ranges.push({
              start: startLine + 1,
              end: endLine + 1,
              kind: monaco.languages.FoldingRangeKind.Region
            });
          }

          stack.pop();
        }
      }

      // Check for section commands
      const sectionMatch = line.match(/^\\(sub)*section\{/);
      if (sectionMatch) {
        // Find the end of this section (next section or end of document)
        let endLine = lines.length;
        for (let j = i + 1; j < lines.length; j++) {
          const nextLine = lines[j];
          if (nextLine.match(/^\\(sub)*section\{/)) {
            endLine = j;
            break;
          }
        }

        if (endLine > i + 1) {
          ranges.push({
            start: i + 1,
            end: endLine,
            kind: monaco.languages.FoldingRangeKind.Region
          });
        }
      }
    }

    return ranges;
  }

  updateContent(value: string) {
    if (this.editor && value !== this.editor.getValue()) {
      this.editor.setValue(value);
    }
  }

  updateOptions(options: monaco.editor.IStandaloneEditorConstructionOptions) {
    if (this.editor) {
      this.editor.updateOptions(options);
    }
  }

  layout() {
    if (this.editor) {
      this.editor.layout();
    }
  }

  focus() {
    if (this.editor) {
      this.editor.focus();
    }
  }

  getValue(): string {
    return this.editor?.getValue() || '';
  }

  setValue(value: string) {
    if (this.editor) {
      this.editor.setValue(value);
    }
  }

  getSelection(): monaco.Selection | null {
    return this.editor?.getSelection() || null;
  }

  setSelection(selection: monaco.Selection) {
    if (this.editor) {
      this.editor.setSelection(selection);
    }
  }

  // 协作功能
  updateCollaborationCursor(cursor: CollaborationCursor) {
    if (!this.editor) return;

    const decorations = this.editor.createDecorationsCollection([
      {
        range: new monaco.Range(
          cursor.position.line,
          cursor.position.column,
          cursor.position.line,
          cursor.position.column + 1
        ),
        options: {
          className: `collaboration-cursor collaboration-cursor-${cursor.userId}`,
          beforeContentClassName: `collaboration-cursor-before collaboration-cursor-before-${cursor.userId}`,
          afterContentClassName: `collaboration-cursor-after collaboration-cursor-after-${cursor.userId}`,
          zIndex: 100
        }
      }
    ]);

    this.collaborationCursors.set(cursor.userId, decorations);
  }

  removeCollaborationCursor(userId: string) {
    const decorations = this.collaborationCursors.get(userId);
    if (decorations) {
      decorations.clear();
      this.collaborationCursors.delete(userId);
    }
  }

  updateCollaborationSelection(cursor: CollaborationCursor) {
    if (!this.editor || !cursor.selection) return;

    const decorations = this.editor.createDecorationsCollection([
      {
        range: new monaco.Range(
          cursor.selection.startLine,
          cursor.selection.startColumn,
          cursor.selection.endLine,
          cursor.selection.endColumn
        ),
        options: {
          className: `collaboration-selection collaboration-selection-${cursor.userId}`,
          zIndex: 50
        }
      }
    ]);

    this.collaborationCursors.set(`${cursor.userId}-selection`, decorations);
  }

  clearAllCollaborationCursors() {
    this.collaborationCursors.forEach(decorations => {
      decorations.clear();
    });
    this.collaborationCursors.clear();
  }

  destroy() {
    // 清理协作光标
    this.clearAllCollaborationCursors();

    this.disposables.forEach(disposable => disposable.dispose());
    this.disposables = [];

    if (this.editor) {
      this.editor.dispose();
      this.editor = null;
    }
  }
}

// Vue Composition API 封装
export function useMonacoEditor(
  containerRef: Ref<HTMLElement | null>,
  props: MonacoEditorProps,
  emit: (event: keyof MonacoEditorEmits, ...args: any[]) => void
) {
  const editorManager = ref<MonacoEditorManager | null>(null);
  const isReady = ref(false);
  const error = ref<Error | null>(null);

  onMounted(async () => {
    try {
      console.log('Creating MonacoEditorManager...', { containerRef: containerRef.value, props });
      editorManager.value = new MonacoEditorManager(containerRef, props, emit);

      // 添加超时处理
      await Promise.race([
        editorManager.value.initialize(),
        new Promise((_, reject) =>
          setTimeout(() => reject(new Error('Monaco editor initialization timeout')), 10000)
        )
      ]);

      isReady.value = true;
      console.log('Monaco Editor initialized successfully');
    } catch (e) {
      error.value = e instanceof Error ? e : new Error(String(e));
      console.error('Failed to initialize Monaco Editor:', e);
    }
  });

  onUnmounted(() => {
    if (editorManager.value) {
      editorManager.value.destroy();
    }
  });

  // 监听props变化
  watch(() => props.modelValue, (newValue) => {
    if (editorManager.value && isReady.value) {
      editorManager.value.updateContent(newValue);
    }
  });

  watch(() => props.theme, (newTheme) => {
    if (editorManager.value && isReady.value && newTheme) {
      monaco.editor.setTheme(newTheme);
    }
  });

  watch(() => props.readonly, (newReadonly) => {
    if (editorManager.value && isReady.value) {
      editorManager.value.updateOptions({ readOnly: newReadonly });
    }
  });

  return {
    editorManager,
    isReady,
    error,
    layout: () => editorManager.value?.layout(),
    focus: () => editorManager.value?.focus(),
    getValue: () => editorManager.value?.getValue() || '',
    setValue: (value: string) => editorManager.value?.setValue(value),
    getSelection: () => editorManager.value?.getSelection(),
    setSelection: (selection: monaco.Selection) => editorManager.value?.setSelection(selection),
    // 协作方法
    updateCollaborationCursor: (cursor: CollaborationCursor) => editorManager.value?.updateCollaborationCursor(cursor),
    removeCollaborationCursor: (userId: string) => editorManager.value?.removeCollaborationCursor(userId),
    updateCollaborationSelection: (cursor: CollaborationCursor) => editorManager.value?.updateCollaborationSelection(cursor),
    clearAllCollaborationCursors: () => editorManager.value?.clearAllCollaborationCursors()
  };
}