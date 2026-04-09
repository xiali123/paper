/**
 * LaTeX编译Web Worker
 * 用于在后台线程中处理LaTeX编译任务，避免阻塞UI
 */

export interface CompilationTask {
  id: string;
  type: 'compile' | 'validate' | 'preview';
  content: string;
  options: {
    engine: 'pdflatex' | 'xelatex' | 'lualatex';
    format: 'pdf' | 'html' | 'svg';
    packages?: string[];
    timeout?: number;
  };
  priority: 'high' | 'normal' | 'low';
  timestamp: number;
}

export interface CompilationResult {
  taskId: string;
  success: boolean;
  output?: string | ArrayBuffer;
  errors?: CompilationError[];
  warnings?: CompilationWarning[];
  metadata?: {
    pages?: number;
    size?: number;
    duration: number;
  };
}

export interface CompilationError {
  line: number;
  column?: number;
  message: string;
  type: 'error' | 'warning' | 'info';
  file?: string;
  suggestion?: string;
}

export interface CompilationWarning {
  line: number;
  message: string;
  type: 'warning' | 'info';
}

export class CompilationWorker {
  private taskQueue: CompilationTask[] = [];
  private isProcessing = false;
  private currentTask: CompilationTask | null = null;
  private taskTimeout: NodeJS.Timeout | null = null;

  constructor() {
    this.setupMessageHandler();
  }

  private setupMessageHandler() {
    self.onmessage = (event: MessageEvent) => {
      const { type, payload } = event.data;

      switch (type) {
        case 'ADD_TASK':
          this.addTask(payload as CompilationTask);
          break;
        case 'CANCEL_TASK':
          this.cancelTask(payload.taskId);
          break;
        case 'CLEAR_QUEUE':
          this.clearQueue();
          break;
        case 'GET_STATUS':
          this.sendStatus();
          break;
        default:
          console.warn('Unknown message type:', type);
      }
    };
  }

  private addTask(task: CompilationTask) {
    // 设置默认值
    task.priority = task.priority || 'normal';
    task.timestamp = task.timestamp || Date.now();
    task.options.timeout = task.options.timeout || 30000; // 30秒默认超时

    // 根据优先级插入队列
    if (task.priority === 'high') {
      this.taskQueue.unshift(task);
    } else {
      this.taskQueue.push(task);
    }

    this.log(`Task ${task.id} added to queue. Queue length: ${this.taskQueue.length}`);

    // 如果没有在处理任务，开始处理
    if (!this.isProcessing) {
      this.processNextTask();
    }
  }

  private async processNextTask() {
    if (this.taskQueue.length === 0) {
      this.isProcessing = false;
      return;
    }

    this.isProcessing = true;
    this.currentTask = this.taskQueue.shift()!;

    this.log(`Processing task ${this.currentTask.id}`);

    try {
      const result = await this.executeTask(this.currentTask);
      this.sendResult(result);
    } catch (error) {
      this.sendError(this.currentTask.id, error);
    } finally {
      this.currentTask = null;

      // 处理下一个任务
      setTimeout(() => this.processNextTask(), 100); // 小延迟避免过度占用CPU
    }
  }

  private async executeTask(task: CompilationTask): Promise<CompilationResult> {
    const startTime = Date.now();

    // 设置超时
    const timeoutPromise = new Promise<never>((_, reject) => {
      this.taskTimeout = setTimeout(() => {
        reject(new Error(`Task ${task.id} timed out after ${task.options.timeout}ms`));
      }, task.options.timeout);
    });

    try {
      // 根据任务类型执行不同操作
      let result: CompilationResult;

      switch (task.type) {
        case 'compile':
          result = await this.compileLatex(task);
          break;
        case 'validate':
          result = await this.validateLatex(task);
          break;
        case 'preview':
          result = await this.generatePreview(task);
          break;
        default:
          throw new Error(`Unknown task type: ${task.type}`);
      }

      // 清除超时定时器
      if (this.taskTimeout) {
        clearTimeout(this.taskTimeout);
        this.taskTimeout = null;
      }

      const duration = Date.now() - startTime;
      result.metadata = {
        ...result.metadata,
        duration
      };

      this.log(`Task ${task.id} completed in ${duration}ms`);
      return result;

    } catch (error) {
      // 清除超时定时器
      if (this.taskTimeout) {
        clearTimeout(this.taskTimeout);
        this.taskTimeout = null;
      }

      throw error;
    }
  }

  private async compileLatex(task: CompilationTask): Promise<CompilationResult> {
    const { content, options } = task;

    // 模拟LaTeX编译过程
    await this.simulateWork(1000 + Math.random() * 2000); // 1-3秒模拟编译时间

    // 语法验证
    const errors = this.validateLatexSyntax(content);
    const warnings = this.findLatexWarnings(content);

    // 模拟编译成功/失败
    const hasErrors = errors.some(e => e.type === 'error');
    const success = !hasErrors;

    let output: ArrayBuffer | undefined;
    let metadata: CompilationResult['metadata'];

    if (success) {
      // 模拟PDF输出
      const mockPdfContent = this.generateMockPdf(content);
      output = mockPdfContent;
      metadata = {
        pages: this.estimatePageCount(content),
        size: output.byteLength,
        duration: 0 // 将在调用处设置
      };
    }

    return {
      taskId: task.id,
      success,
      output,
      errors,
      warnings,
      metadata
    };
  }

  private async validateLatex(task: CompilationTask): Promise<CompilationResult> {
    const { content } = task;

    // 快速验证
    await this.simulateWork(100 + Math.random() * 400);

    const errors = this.validateLatexSyntax(content);
    const warnings = this.findLatexWarnings(content);

    return {
      taskId: task.id,
      success: !errors.some(e => e.type === 'error'),
      errors,
      warnings,
      metadata: {
        duration: 0
      }
    };
  }

  private async generatePreview(task: CompilationTask): Promise<CompilationResult> {
    const { content, options } = task;

    // 生成预览（HTML格式）
    await this.simulateWork(500 + Math.random() * 1000);

    const errors = this.validateLatexSyntax(content);
    const htmlContent = this.convertLatexToHtml(content);

    return {
      taskId: task.id,
      success: !errors.some(e => e.type === 'error'),
      output: htmlContent,
      errors,
      metadata: {
        duration: 0
      }
    };
  }

  private validateLatexSyntax(content: string): CompilationError[] {
    const errors: CompilationError[] = [];
    const lines = content.split('\n');

    lines.forEach((line, lineIndex) => {
      const lineNumber = lineIndex + 1;

      // 检查未闭合的花括号
      const openBraces = (line.match(/\{/g) || []).length;
      const closeBraces = (line.match(/\}/g) || []).length;
      if (openBraces !== closeBraces) {
        errors.push({
          line: lineNumber,
          message: 'Unmatched braces: { and } must be balanced',
          type: 'error',
          suggestion: 'Check that all opening braces { have corresponding closing braces }'
        });
      }

      // 检查未闭合的数学环境
      const dollarCount = (line.match(/\$/g) || []).length;
      if (dollarCount % 2 !== 0 && !line.includes('\\$')) {
        errors.push({
          line: lineNumber,
          message: 'Unmatched math delimiters: $ must be paired',
          type: 'error',
          suggestion: 'Ensure every $ has a matching $ for math mode'
        });
      }

      // 检查未知命令
      const unknownCommands = line.match(/\\[a-zA-Z@]+/g);
      if (unknownCommands) {
        unknownCommands.forEach(cmd => {
          if (!this.isKnownCommand(cmd)) {
            errors.push({
              line: lineNumber,
              message: `Unknown command: ${cmd}`,
              type: 'warning',
              suggestion: 'Check spelling or add required package'
            });
          }
        });
      }

      // 检查环境匹配
      const beginMatches = line.match(/\\begin\{([^}]+)\}/g);
      const endMatches = line.match(/\\end\{([^}]+)\}/g);

      if (beginMatches || endMatches) {
        // 简化的环境匹配检查
        if (beginMatches && !endMatches) {
          errors.push({
            line: lineNumber,
            message: 'Environment opened but not closed',
            type: 'error',
            suggestion: 'Add corresponding \\end{environment}'
          });
        }
      }
    });

    return errors;
  }

  private findLatexWarnings(content: string): CompilationWarning[] {
    const warnings: CompilationWarning[] = [];
    const lines = content.split('\n');

    lines.forEach((line, lineIndex) => {
      const lineNumber = lineIndex + 1;

      // 检查过长的行
      if (line.length > 80) {
        warnings.push({
          line: lineNumber,
          message: 'Line too long (recommended max: 80 characters)',
          type: 'warning'
        });
      }

      // 检查硬编码的尺寸
      if (line.match(/\\(vspace|hspace)\s*\{[^}]*cm[^}]*\}/)) {
        warnings.push({
          line: lineNumber,
          message: 'Consider using relative units instead of absolute cm units',
          type: 'info'
        });
      }

      // 检查重复空格
      if (line.match(/\s{3,}/)) {
        warnings.push({
          line: lineNumber,
          message: 'Multiple consecutive spaces detected',
          type: 'info'
        });
      }
    });

    return warnings;
  }

  private convertLatexToHtml(content: string): string {
    let html = content;

    // 简单的LaTeX到HTML转换
    html = html
      // 标题
      .replace(/\\section\*?\{([^}]+)\}/g, '<h1>$1</h1>')
      .replace(/\\subsection\*?\{([^}]+)\}/g, '<h2>$1</h2>')
      .replace(/\\subsubsection\*?\{([^}]+)\}/g, '<h3>$1</h3>')

      // 文本格式
      .replace(/\\textbf\{([^}]+)\}/g, '<strong>$1</strong>')
      .replace(/\\textit\{([^}]+)\}/g, '<em>$1</em>')
      .replace(/\\underline\{([^}]+)\}/g, '<u>$1</u>')

      // 数学公式（简化处理）
      .replace(/\$([^$]+)\$/g, '<span class="math-inline">$1</span>')
      .replace(/\$\$([^$]+)\$\$/g, '<div class="math-display">$1</div>')

      // 列表
      .replace(/\\begin\{itemize\}([\s\S]*?)\\end\{itemize\}/g,
        '<ul>$1</ul>')
      .replace(/\\begin\{enumerate\}([\s\S]*?)\\end\{enumerate\}/g,
        '<ol>$1</ol>')
      .replace(/\\item\s+([^\n]+)/g, '<li>$1</li>')

      // 换行
      .replace(/\\\\/g, '<br>')
      .replace(/\n\n/g, '</p><p>');

    return `<div class="latex-preview">${html}</div>`;
  }

  private generateMockPdf(content: string): ArrayBuffer {
    // 生成模拟的PDF内容
    const mockPdfHeader = '%PDF-1.4\n';
    const mockContent = `Mock PDF generated from LaTeX content (${content.length} characters)`;
    const encoder = new TextEncoder();
    return encoder.encode(mockPdfHeader + mockContent).buffer;
  }

  private estimatePageCount(content: string): number {
    const lines = content.split('\n').length;
    const words = content.split(/\s+/).length;
    // 粗略估计：每页约30行或500词
    return Math.max(1, Math.ceil(Math.min(lines / 30, words / 500)));
  }

  private isKnownCommand(command: string): boolean {
    const knownCommands = new Set([
      // 基础命令
      'documentclass', 'usepackage', 'begin', 'end', 'maketitle',
      'section', 'subsection', 'subsubsection', 'paragraph',
      'textbf', 'textit', 'underline', 'emph',

      // 数学命令
      'frac', 'sqrt', 'sum', 'int', 'lim', 'alpha', 'beta', 'gamma',
      'delta', 'epsilon', 'theta', 'lambda', 'mu', 'pi', 'sigma', 'phi',

      // 环境
      'document', 'abstract', 'itemize', 'enumerate', 'equation',
      'align', 'figure', 'table', 'center', 'flushleft', 'flushright'
    ]);

    return knownCommands.has(command.slice(1)); // 去掉反斜杠
  }

  private async simulateWork(ms: number): Promise<void> {
    // 模拟CPU密集型工作
    return new Promise(resolve => {
      const start = Date.now();
      const iterations = Math.floor(ms / 10); // 粗略计算迭代次数

      let count = 0;
      const chunkSize = 10000;

      const processChunk = () => {
        const chunkEnd = Math.min(count + chunkSize, iterations);
        for (; count < chunkEnd; count++) {
          // 模拟一些计算工作
          Math.sqrt(count * Math.PI);
        }

        if (count < iterations) {
          // 让出控制权给其他任务
          setTimeout(processChunk, 0);
        } else {
          resolve();
        }
      };

      processChunk();
    });
  }

  private cancelTask(taskId: string) {
    // 从队列中移除任务
    const index = this.taskQueue.findIndex(task => task.id === taskId);
    if (index !== -1) {
      this.taskQueue.splice(index, 1);
      this.log(`Task ${taskId} cancelled from queue`);
    }

    // 如果是当前任务，标记为取消（实际取消需要任务支持）
    if (this.currentTask?.id === taskId) {
      this.log(`Current task ${taskId} requested to cancel`);
    }
  }

  private clearQueue() {
    const count = this.taskQueue.length;
    this.taskQueue = [];
    this.log(`Queue cleared. Removed ${count} tasks`);
  }

  private sendResult(result: CompilationResult) {
    self.postMessage({
      type: 'TASK_COMPLETED',
      payload: result
    });
  }

  private sendError(taskId: string, error: unknown) {
    self.postMessage({
      type: 'TASK_ERROR',
      payload: {
        taskId,
        error: error instanceof Error ? error.message : String(error)
      }
    });
  }

  private sendStatus() {
    self.postMessage({
      type: 'WORKER_STATUS',
      payload: {
        isProcessing: this.isProcessing,
        queueLength: this.taskQueue.length,
        currentTask: this.currentTask?.id,
        timestamp: Date.now()
      }
    });
  }

  private log(message: string) {
    if (process.env.NODE_ENV === 'development') {
      console.log(`[CompilationWorker] ${message}`);
    }
  }
}

// 创建并启动Worker
const worker = new CompilationWorker();

// 导出Worker类供其他模块使用
export default CompilationWorker;