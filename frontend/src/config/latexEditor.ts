/**
 * LaTeX 编辑器配置
 * 统一管理所有编辑器相关的常量和配置项
 */

export const LATEX_EDITOR_CONFIG = {
  // 性能配置
  PERFORMANCE: {
    // 大文档阈值（字符数）
    LARGE_DOCUMENT_THRESHOLD: 50000,
    // 超大文档阈值
    HUGE_DOCUMENT_THRESHOLD: 200000,
    // 小文档防抖延迟（毫秒）
    DEBOUNCE_DELAY_SMALL: 200,
    // 大文档防抖延迟（毫秒）
    DEBOUNCE_DELAY_LARGE: 500,
    // 最大预览字符数（超出则分块渲染）
    MAX_PREVIEW_CHARS: 100000,
    // 渲染块大小（行数）
    RENDER_CHUNK_SIZE: 500,
    // 增量渲染缓冲区（块数）
    RENDER_BUFFER_SIZE: 2,
    // Worker超时时间（毫秒）
    WORKER_TIMEOUT: 10000,
  },

  // 自动保存配置
  AUTOSAVE: {
    // 默认自动保存间隔（毫秒）
    DEFAULT_INTERVAL: 30000,
    // 最小自动保存间隔
    MIN_INTERVAL: 5000,
    // 自动保存防抖延迟
    DEBOUNCE_DELAY: 1000,
    // 本地备份最大保留数量
    MAX_LOCAL_BACKUPS: 5,
  },

  // PDF 查看器配置
  PDF_VIEWER: {
    // 最小缩放比例
    MIN_SCALE: 0.5,
    // 最大缩放比例
    MAX_SCALE: 3.0,
    // 缩放步长
    SCALE_STEP: 0.25,
    // 默认缩放比例
    DEFAULT_SCALE: 1.0,
    // 页面加载超时（毫秒）
    PAGE_LOAD_TIMEOUT: 10000,
  },

  // 编辑器配置
  EDITOR: {
    // 默认字体大小
    DEFAULT_FONT_SIZE: 14,
    // 最小字体大小
    MIN_FONT_SIZE: 10,
    // 最大字体大小
    MAX_FONT_SIZE: 24,
    // Tab 大小
    TAB_SIZE: 2,
    // 默认字体族
    DEFAULT_FONT_FAMILY: "'Fira Code', 'Consolas', 'Monaco', monospace",
    // 最大撤销历史
    MAX_HISTORY: 100,
    // 光标闪烁速度（毫秒）
    CURSOR_BLINK_SPEED: 530,
  },

  // 快捷键配置
  SHORTCUTS: {
    // 保存
    SAVE: 'Ctrl+S',
    // 编译
    COMPILE: 'Ctrl+Enter',
    // 查找
    FIND: 'Ctrl+F',
    // 替换
    REPLACE: 'Ctrl+H',
    // 跳转到行
    GOTO_LINE: 'Ctrl+G',
    // 自动补全
    AUTOCOMPLETE: 'Ctrl+Space',
    // 快速插入
    QUICK_INSERT: 'Ctrl+Alt+X',
    // AI 公式识别
    AI_RECOGNIZE: 'Ctrl+Alt+I',
    // 导出
    EXPORT: 'Ctrl+E',
    // 切换注释
    TOGGLE_COMMENT: 'Ctrl+/',
    // 格式化
    FORMAT: 'Shift+Alt+F',
  },

  // UI 配置
  UI: {
    // 工具栏按钮大小
    TOOLBAR_BUTTON_SIZE: 'small',
    // 最小面板宽度（像素）
    MIN_PANEL_WIDTH: 300,
    // 最大面板宽度（像素）
    MAX_PANEL_WIDTH: 800,
    // 默认面板宽度（像素）
    DEFAULT_PANEL_WIDTH: 500,
    // 侧边栏宽度（像素）
    SIDEBAR_WIDTH: 280,
    // 大纲面板宽度（像素）
    OUTLINE_WIDTH: 250,
  },

  // 限制配置
  LIMITS: {
    // 最大文档大小（字符数）
    MAX_DOCUMENT_SIZE: 1000000,
    // 最大单行长度（字符数）
    MAX_LINE_LENGTH: 10000,
    // 最大文件数量（项目中）
    MAX_PROJECT_FILES: 100,
    // 最大图片大小（MB）
    MAX_IMAGE_SIZE: 10,
    // 最大图片宽度（像素）
    MAX_IMAGE_WIDTH: 5000,
    // 最大图片高度（像素）
    MAX_IMAGE_HEIGHT: 5000,
  },

  // 支持的图片格式
  SUPPORTED_IMAGE_FORMATS: [
    '.png',
    '.jpg',
    '.jpeg',
    '.gif',
    '.pdf',
    '.eps',
    '.svg',
  ],

  // LaTeX 命令分类
  LATEX_COMMANDS: {
    // 文档结构
    STRUCTURE: [
      '\\part',
      '\\chapter',
      '\\section',
      '\\subsection',
      '\\subsubsection',
      '\\paragraph',
      '\\subparagraph',
    ],
    // 文本格式
    FORMATTING: [
      '\\textbf',
      '\\textit',
      '\\texttt',
      '\\underline',
      '\\emph',
      '\\textsc',
      '\\textsf',
    ],
    // 环境
    ENVIRONMENTS: [
      'document',
      'itemize',
      'enumerate',
      'description',
      'figure',
      'table',
      'equation',
      'align',
      'gather',
      'multline',
    ],
    // 数学
    MATH: [
      '\\frac',
      '\\sqrt',
      '\\sum',
      '\\prod',
      '\\int',
      '\\lim',
      '\\infty',
      '\\alpha',
      '\\beta',
      '\\gamma',
      '\\delta',
      // ... 更多数学符号
    ],
  },

  // 错误消息
  ERROR_MESSAGES: {
    // 网络错误
    NETWORK_ERROR: '网络错误，请检查网络连接后重试',
    // 超时错误
    TIMEOUT_ERROR: '请求超时，请检查网络连接',
    // 未授权
    UNAUTHORIZED: '登录已过期，请重新登录',
    // 禁止访问
    FORBIDDEN: '没有权限执行此操作',
    // 未找到
    NOT_FOUND: '请求的资源不存在',
    // 服务器错误
    SERVER_ERROR: '服务器错误，请稍后重试',
    // 配额超限
    QUOTA_EXCEEDED: '配额已用完，请升级套餐或等待重置',
    // 语法错误
    SYNTAX_ERROR: 'LaTeX语法错误，请检查文档',
    // 编译超时
    COMPILE_TIMEOUT: '编译超时，请检查文档是否有复杂内容',
  },

  // 成功消息
  SUCCESS_MESSAGES: {
    SAVE_SUCCESS: '文档保存成功',
    COMPILE_SUCCESS: '编译成功',
    AUTO_SAVE_SUCCESS: '已自动保存',
    TEMPLATE_APPLIED: '模板已应用',
  },

  // 警告消息
  WARNING_MESSAGES: {
    UNSAVED_CHANGES: '您有未保存的更改',
    LARGE_DOCUMENT: '文档较大，渲染可能需要一些时间',
    CACHE_CLEARED: '缓存已清除',
  },
} as const

// 类型定义
export type LatexConfig = typeof LATEX_EDITOR_CONFIG

// 辅助函数
/**
 * 根据文档大小获取防抖延迟
 */
export function getDebounceDelay(contentLength: number): number {
  return contentLength > LATEX_EDITOR_CONFIG.PERFORMANCE.LARGE_DOCUMENT_THRESHOLD
    ? LATEX_EDITOR_CONFIG.PERFORMANCE.DEBOUNCE_DELAY_LARGE
    : LATEX_EDITOR_CONFIG.PERFORMANCE.DEBOUNCE_DELAY_SMALL
}

/**
 * 检查文件是否为支持的图片格式
 */
export function isSupportedImageFormat(filename: string): boolean {
  const ext = filename.toLowerCase().slice(filename.lastIndexOf('.'))
  return LATEX_EDITOR_CONFIG.SUPPORTED_IMAGE_FORMATS.includes(ext as any)
}

/**
 * 验证图片路径
 */
export function validateImagePath(path: string): {
  valid: boolean
  error?: string
} {
  if (!path) {
    return { valid: false, error: '图片路径为空' }
  }

  // 检查危险字符
  if (/[<>:"|?*\x00-\x1F]/.test(path)) {
    return { valid: false, error: '图片路径包含非法字符' }
  }

  // 检查文件扩展名
  const ext = path.toLowerCase().slice(path.lastIndexOf('.'))
  if (!LATEX_EDITOR_CONFIG.SUPPORTED_IMAGE_FORMATS.includes(ext as any)) {
    return { valid: false, error: `不支持的图片格式: ${ext}` }
  }

  return { valid: true }
}

/**
 * 格式化文件大小
 */
export function formatFileSize(bytes: number): string {
  const units = ['B', 'KB', 'MB', 'GB']
  let size = bytes
  let unitIndex = 0

  while (size >= 1024 && unitIndex < units.length - 1) {
    size /= 1024
    unitIndex++
  }

  return `${size.toFixed(1)} ${units[unitIndex]}`
}

/**
 * 格式化时间
 */
export function formatTime(milliseconds: number): string {
  const seconds = Math.floor(milliseconds / 1000)

  if (seconds < 60) {
    return `${seconds}秒`
  }

  const minutes = Math.floor(seconds / 60)
  const remainingSeconds = seconds % 60

  if (minutes < 60) {
    return `${minutes}分${remainingSeconds}秒`
  }

  const hours = Math.floor(minutes / 60)
  const remainingMinutes = minutes % 60

  return `${hours}小时${remainingMinutes}分钟`
}
