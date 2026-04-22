/**
 * LaTeX 渲染器 - 统一的渲染逻辑
 * 避免代码重复，提供增量渲染支持
 */

interface RenderOptions {
  enableCache?: boolean
  chunkSize?: number
  timeout?: number
}

interface RenderChunk {
  index: number
  content: string
  html?: string
  rendered: boolean
}

export class LatexRenderer {
  private cache = new Map<string, string>()
  private chunks: RenderChunk[] = []
  private chunkSize = 500 // 每块500行

  constructor(options?: RenderOptions) {
    if (options?.chunkSize) {
      this.chunkSize = options.chunkSize
    }
  }

  /**
   * 将内容分割成块
   */
  private splitIntoChunks(content: string): RenderChunk[] {
    const lines = content.split('\n')
    const chunks: RenderChunk[] = []

    for (let i = 0; i < lines.length; i += this.chunkSize) {
      const chunkLines = lines.slice(i, i + this.chunkSize)
      chunks.push({
        index: Math.floor(i / this.chunkSize),
        content: chunkLines.join('\n'),
        rendered: false
      })
    }

    return chunks
  }

  /**
   * 渲染单个块
   */
  private renderChunk(chunk: RenderChunk): string {
    const cacheKey = `chunk-${chunk.index}-${chunk.content.length}`

    if (this.cache.has(cacheKey)) {
      return this.cache.get(cacheKey)!
    }

    let html = chunk.content

    // 处理章节
    html = html.replace(/\\section\*?\{([^}]+)\}/g, '<h2>$1</h2>')
    html = html.replace(/\\subsection\*?\{([^}]+)\}/g, '<h3>$1</h3>')
    html = html.replace(/\\subsubsection\*?\{([^}]+)\}/g, '<h4>$1</h4>')

    // 处理文本格式
    html = html.replace(/\\textbf\{([^}]+)\}/g, '<strong>$1</strong>')
    html = html.replace(/\\textit\{([^}]+)\}/g, '<em>$1</em>')
    html = html.replace(/\\underline\{([^}]+)\}/g, '<u>$1</u>')
    html = html.replace(/\\emph\{([^}]+)\}/g, '<em>$1</em>')

    // 处理列表
    html = html.replace(/\\begin\{itemize\}[\s\S]*?\\end\{itemize\}/g, (match) => {
      const items = match.match(/\\item\s+([^\n\\]+)/g) || []
      return '<ul>' + items.map(item => `<li>${item.replace(/\\item\s+/, '')}</li>`).join('') + '</ul>'
    })

    html = html.replace(/\\begin\{enumerate\}[\s\S]*?\\end\{enumerate\}/g, (match) => {
      const items = match.match(/\\item\s+([^\n\\]+)/g) || []
      return '<ol>' + items.map((item, i) => `<li>${item.replace(/\\item\s+/, '')}</li>`).join('') + '</ol>'
    })

    // 处理换行
    html = html.replace(/\\\\/g, '<br>')
    html = html.replace(/\n/g, '<br>')

    // 缓存结果
    this.cache.set(cacheKey, html)

    return html
  }

  /**
   * 增量渲染 - 只渲染可见区域
   */
  async renderIncremental(
    content: string,
    visibleRange: { start: number; end: number }
  ): Promise<string> {
    this.chunks = this.splitIntoChunks(content)

    // 计算需要渲染的块（包含缓冲区）
    const buffer = 2 // 前后各缓冲2块
    const startChunk = Math.max(0, Math.floor(visibleRange.start / this.chunkSize) - buffer)
    const endChunk = Math.min(
      this.chunks.length - 1,
      Math.floor(visibleRange.end / this.chunkSize) + buffer
    )

    const chunksToRender = this.chunks.slice(startChunk, endChunk + 1)

    // 并行渲染块
    const renderedChunks = await Promise.all(
      chunksToRender.map(async (chunk) => {
        if (!chunk.rendered) {
          chunk.html = await this.renderWithTimeout(chunk)
          chunk.rendered = true
        }
        return chunk.html
      })
    )

    return renderedChunks.join('')
  }

  /**
   * 带超时的渲染
   */
  private async renderWithTimeout(
    chunk: RenderChunk,
    timeout = 100
  ): Promise<string> {
    return new Promise((resolve) => {
      const timer = setTimeout(() => {
        resolve(this.renderChunk(chunk.content))
      }, timeout)

      // 如果浏览器支持requestIdleCallback，优先使用
      if ('requestIdleCallback' in window) {
        // @ts-ignore
        window.requestIdleCallback(() => {
          clearTimeout(timer)
          resolve(this.renderChunk(chunk.content))
        })
      }
    })
  }

  /**
   * 渲染全部内容（用于小文档）
   */
  async renderFull(content: string): Promise<string> {
    // 小文档直接渲染
    if (content.length < 10000) {
      return this.renderChunk({ index: 0, content, rendered: false })
    }

    // 大文档使用增量渲染
    return this.renderIncremental(content, { start: 0, end: content.length })
  }

  /**
   * 清理缓存
   */
  clearCache(): void {
    this.cache.clear()
    this.chunks.forEach(chunk => {
      chunk.rendered = false
      chunk.html = undefined
    })
  }

  /**
   * 获取缓存大小
   */
  getCacheSize(): number {
    return this.cache.size
  }
}

// 单例实例
let rendererInstance: LatexRenderer | null = null

export function getLatexRenderer(): LatexRenderer {
  if (!rendererInstance) {
    rendererInstance = new LatexRenderer()
  }
  return rendererInstance
}

/**
 * 快速渲染函数（兼容旧代码）
 */
export async function renderLatex(content: string): Promise<string> {
  const renderer = getLatexRenderer()
  return renderer.renderFull(content)
}
