// Web Worker utilities for LaTeX editor

interface WorkerResponse<T = any> {
  success: boolean;
  id: string;
  html?: string;
  data?: T;
  error?: string;
  processingTime?: number;
  contentLength?: number;
  sanitized?: boolean;
}

class WorkerPool {
  private workers: Worker[] = [];
  private taskQueue: Array<{
    id: string;
    data: any;
    resolve: (value: any) => void;
    reject: (error: any) => void;
  }> = [];
  private activeWorkers = 0;
  private readonly maxWorkers: number;
  private readonly workerScript: string;

  constructor(workerScript: string, maxWorkers = 2) {
    this.workerScript = workerScript;
    this.maxWorkers = maxWorkers;
    this.initializeWorkers();
  }

  private initializeWorkers() {
    for (let i = 0; i < this.maxWorkers; i++) {
      const worker = new Worker(this.workerScript);
      worker.onmessage = (e) => this.handleWorkerMessage(worker, e.data);
      worker.onerror = (error) => this.handleWorkerError(worker, error);
      this.workers.push(worker);
    }
  }

  private handleWorkerMessage(worker: Worker, response: WorkerResponse) {
    if (import.meta.env.DEV) {
      console.log('[WorkerPool] Worker message received:', response);
    }

    this.activeWorkers--;

    // Find and resolve the corresponding task
    const taskIndex = this.taskQueue.findIndex(task => task.id === response.id);
    if (taskIndex !== -1) {
      const task = this.taskQueue[taskIndex];
      this.taskQueue.splice(taskIndex, 1);

      if (response.success) {
        task.resolve(response);
      } else {
        task.reject(new Error(response.error || 'Worker processing failed'));
      }
    }

    // Process next task in queue
    this.processNextTask();
  }

  private handleWorkerError(worker: Worker, error: ErrorEvent) {
    this.activeWorkers--;

    // Suppress specific known errors from CDN workers that don't affect functionality
    const suppressErrors = [
      'Script error.',
      'JSON.parse',
      'Prism'
    ];

    const shouldSuppress = suppressErrors.some(msg =>
      error.message?.includes(msg)
    );

    if (import.meta.env.DEV && !shouldSuppress) {
      console.error('[WorkerPool] Worker error:', error);
    }

    // Don't reject tasks for known harmless errors - let them complete
    if (!shouldSuppress) {
      // Reject all pending tasks for this worker
      const failedTasks = this.taskQueue.filter(task => {
        return true;
      });

      failedTasks.forEach(task => {
        task.reject(new Error(`Worker error: ${error.message}`));
      });

      // Remove failed tasks from queue
      this.taskQueue = this.taskQueue.filter(task => !failedTasks.includes(task));
    }

    // Process next task
    this.processNextTask();
  }

  private processNextTask() {
    if (this.taskQueue.length === 0 || this.activeWorkers >= this.maxWorkers) {
      return;
    }

    const availableWorker = this.workers.find((_, index) => {
      // Simple check - in production you'd track worker availability more precisely
      return index >= this.activeWorkers;
    });

    if (!availableWorker) {
      return;
    }

    const task = this.taskQueue.shift();
    if (task) {
      this.activeWorkers++;
      availableWorker.postMessage(task.data);
    }
  }

  async process<T = WorkerResponse>(data: any, timeout = 10000): Promise<T> {
    const taskId = `${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;

    return new Promise((resolve, reject) => {
      // Add timeout
      const timeoutId = setTimeout(() => {
        reject(new Error(`Worker task timeout after ${timeout}ms`));
      }, timeout);

      const task = {
        id: taskId,
        data: { ...data, id: taskId },
        resolve: (value: T) => {
          clearTimeout(timeoutId);
          resolve(value);
        },
        reject: (error: any) => {
          clearTimeout(timeoutId);
          reject(error);
        }
      };

      this.taskQueue.push(task);
      this.processNextTask();
    });
  }

  terminate() {
    this.workers.forEach(worker => worker.terminate());
    this.workers = [];
    this.taskQueue = [];
    this.activeWorkers = 0;
  }
}

// Create worker pools for different tasks
class WorkerManager {
  private static instance: WorkerManager;
  private prismWorkerPool: WorkerPool;
  private katexWorkerPool: WorkerPool;

  private constructor() {
    this.prismWorkerPool = new WorkerPool('/workers/prism.worker.js', 2);
    this.katexWorkerPool = new WorkerPool('/workers/katex.worker.js', 2);
  }

  static getInstance(): WorkerManager {
    if (!WorkerManager.instance) {
      WorkerManager.instance = new WorkerManager();
    }
    return WorkerManager.instance;
  }

  async highlightSyntax(content: string, theme = 'default'): Promise<{
    html: string;
    processingTime: number;
    contentLength: number;
  }> {
    try {
      const response = await this.prismWorkerPool.process({
        content,
        theme
      });

      return {
        html: response.html || content,
        processingTime: response.processingTime || 0,
        contentLength: response.contentLength || content.length
      };
    } catch (error) {
      console.warn('Syntax highlighting failed, using fallback:', error);
      return {
        html: content,
        processingTime: 0,
        contentLength: content.length
      };
    }
  }

  async renderLatex(content: string): Promise<{
    html: string;
    processingTime: number;
    contentLength: number;
  }> {
    if (import.meta.env.DEV) {
      console.log('[WorkerManager] renderLatex called, content length:', content.length);
    }

    try {
      const response = await this.katexWorkerPool.process({
        content
      });

      if (import.meta.env.DEV) {
        console.log('[WorkerManager] Worker response:', response);
      }

      // 检查响应是否有 html
      if (!response.html && response.error) {
        throw new Error(response.error);
      }

      return {
        html: response.html || this.fallbackRender(content),
        processingTime: response.processingTime || 0,
        contentLength: response.contentLength || content.length
      };
    } catch (error) {
      if (import.meta.env.DEV) {
        console.warn('[WorkerManager] LaTeX rendering failed, using fallback:', error);
      }
      return {
        html: this.fallbackRender(content),
        processingTime: 0,
        contentLength: content.length
      };
    }
  }

  // Fallback rendering when Worker fails
  private fallbackRender(content: string): string {
    let html = content;

    // Process sections
    html = html.replace(/\\section\*?\{([^}]+)\}/g, '<h2>$1</h2>');
    html = html.replace(/\\subsection\*?\{([^}]+)\}/g, '<h3>$1</h3>');
    html = html.replace(/\\subsubsection\*?\{([^}]+)\}/g, '<h4>$1</h4>');

    // Process text formatting
    html = html.replace(/\\textbf\{([^}]+)\}/g, '<strong>$1</strong>');
    html = html.replace(/\\textit\{([^}]+)\}/g, '<em>$1</em>');
    html = html.replace(/\\underline\{([^}]+)\}/g, '<u>$1</u>');

    // Process line breaks
    html = html.replace(/\\\\/g, '<br>');

    return html;
  }

  terminate() {
    this.prismWorkerPool.terminate();
    this.katexWorkerPool.terminate();
  }
}

export const workerManager = WorkerManager.getInstance();

// Convenience functions with fallback
export async function highlightSyntax(content: string, theme = 'default') {
  // Skip Worker to avoid CDN Prism.js errors
  // Use simple local rendering instead
  const startTime = performance.now();

  try {
    // Simple LaTeX syntax highlighting without external dependencies
    let html = content
      // Escape HTML first
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')

      // Highlight LaTeX commands
      .replace(/(\\[a-zA-Z]+)(\{)?/g, '<span class="token keyword">$1</span>$2')
      .replace(/(\{)([^}]+)(\})/g, '$1<span class="token string">$2</span>$3')
      .replace(/(%[^\n]*)/g, '<span class="token comment">$1</span>')
      .replace(/(\$\$?)([^$]+)(\$\$?)/g, '<span class="token equation">$1$2$3</span>')

    const endTime = performance.now();

    return {
      html,
      processingTime: endTime - startTime,
      contentLength: content.length
    };
  } catch (error) {
    return {
      html: content,
      processingTime: 0,
      contentLength: content.length
    };
  }
}

export async function renderLatex(content: string) {
  const startTime = performance.now();

  if (import.meta.env.DEV) {
    console.log('[renderLatex] Starting render, content length:', content.length);
  }

  // Try Worker first
  if (typeof Worker !== 'undefined') {
    try {
      const result = await workerManager.renderLatex(content);
      if (import.meta.env.DEV) {
        console.log('[renderLatex] Worker render successful');
      }
      return result;
    } catch (error) {
      if (import.meta.env.DEV) {
        console.warn('[renderLatex] Worker render failed:', error);
      }
      // Fall through to local rendering
    }
  }

  // Local fallback rendering
  try {
    const katex = await import('katex');
    const DOMPurify = await import('dompurify');

    let html = content;

    // Process inline math $...$
    html = html.replace(/\$([^$\n]+?)\$/g, (match, math) => {
      try {
        return katex.renderToString(math, {
          displayMode: false,
          throwOnError: false,
          output: 'html'
        });
      } catch {
        return match;
      }
    });

    // Process display math $$...$$
    html = html.replace(/\$\$([^$]+?)\$\$/g, (match, math) => {
      try {
        return katex.renderToString(math, {
          displayMode: true,
          throwOnError: false,
          output: 'html'
        });
      } catch {
        return `<div>$$${math}$$</div>`;
      }
    });

    // Process LaTeX structure
    html = html.replace(/\\section\*?\{([^}]+)\}/g, '<h2>$1</h2>');
    html = html.replace(/\\subsection\*?\{([^}]+)\}/g, '<h3>$1</h3>');
    html = html.replace(/\\textbf\{([^}]+)\}/g, '<strong>$1</strong>');
    html = html.replace(/\\textit\{([^}]+)\}/g, '<em>$1</em>');
    html = html.replace(/\\\\/g, '<br>');

    const sanitizedHtml = DOMPurify.sanitize(html);
    const endTime = performance.now();

    if (import.meta.env.DEV) {
      console.log('[renderLatex] Local render successful');
    }

    return {
      html: sanitizedHtml,
      processingTime: endTime - startTime,
      contentLength: content.length
    };
  } catch (error) {
    if (import.meta.env.DEV) {
      console.error('[renderLatex] All rendering failed:', error);
    }
    // Ultimate fallback - just return the content with basic processing
    let html = content;
    html = html.replace(/\\section\*?\{([^}]+)\}/g, '<h2>$1</h2>');
    html = html.replace(/\\textbf\{([^}]+)\}/g, '<strong>$1</strong>');
    html = html.replace(/\\textit\{([^}]+)\}/g, '<em>$1</em>');
    html = html.replace(/\\\\/g, '<br>');

    const endTime = performance.now();
    return {
      html,
      processingTime: endTime - startTime,
      contentLength: content.length
    };
  }
}