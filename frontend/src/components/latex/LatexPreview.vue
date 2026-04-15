<template>
  <div class="latex-preview">
    <div class="preview-header">
      <h4 class="preview-title">预览</h4>
      <el-tag v-if="renderError" type="danger" size="small">渲染错误</el-tag>
    </div>

    <div class="preview-content" ref="contentRef">
      <div v-if="!props.content" class="preview-empty">
        <el-empty description="输入 LaTeX 内容以预览" :image-size="60" />
      </div>
      <div v-else-if="renderError" class="preview-error">
        <el-alert type="error" :closable="false">
          <template #title>
            {{ renderError }}
          </template>
        </el-alert>
      </div>
      <div v-else v-html="renderedHtml" class="preview-rendered" @click="handleMathClick"></div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted, onUnmounted } from 'vue'
import { Files } from '@element-plus/icons-vue'
import DOMPurify from 'dompurify'
import { performanceMonitor, checkPerformanceThreshold, PERFORMANCE_THRESHOLDS } from '@/utils/performance'
import { debounce } from '@/utils/performance'

// 动态导入 katex（仅在前端使用时）
let katex: any = null

interface Props {
  content: string
  theme?: 'light' | 'dark'
  scale?: number
}

const props = withDefaults(defineProps<Props>(), {
  theme: 'light',
  scale: 1.0
})

const contentRef = ref<HTMLElement>()
const renderError = ref<string | null>(null)
const renderedHtml = ref('')

// 渲染 LaTeX 内容
const isRendering = ref(false)

async function renderLatex() {
  const endTimer = performanceMonitor.startTimer('latex_rendering', {
    contentLength: props.content?.length || 0
  })

  isRendering.value = true

  let contentToRender = props.content?.trim()

  if (!contentToRender) {
    renderedHtml.value = ''
    renderError.value = null
    endTimer()
    isRendering.value = false
    return
  }

  try {
    // 动态导入 katex
    if (!katex) {
      try {
        const katexModule = await import('katex')
        katex = katexModule.default || katexModule
        if (import.meta.env.DEV) {
          console.log('[LatexPreview] KaTeX loaded successfully')
        }
      } catch (e) {
        if (import.meta.env.DEV) {
          console.warn('[LatexPreview] KaTeX import failed:', e)
        }
      }
    }

    // 过滤LaTeX导言区 - 只渲染 \begin{document} 之后的内容
    const documentBeginMatch = contentToRender.match(/\\begin\{document\}([\s\S]*)/i)
    if (documentBeginMatch) {
      // 有 \begin{document}，只渲染之后的内容
      contentToRender = documentBeginMatch[1]
      // 移除 \end{document}
      contentToRender = contentToRender.replace(/\\end\{document\}.*$/i, '')
    } else {
      // 没有 \begin{document}，检查是否是完整文档结构
      const documentClassMatch = contentToRender.match(/\\documentclass\{[^}]+\}/i)
      if (documentClassMatch) {
        // 这是导言区或只有配置的文档，显示提示
        renderedHtml.value = `
          <div class="latex-preamble-info">
            <el-empty description="请输入 \\begin{document} 之后的内容">
              <template #image>
                <el-icon :size="60"><Files /></el-icon>
              </template>
            </el-empty>
            <div class="preamble-hint">
              <p><strong>不会显示的内容：</strong></p>
              <ul class="preamble-list">
                <li><code>\\documentclass</code> - 文档类声明</li>
                <li><code>\\usepackage</code> - 宏包引用</li>
                <li><code>\\begin{abstract}</code> - 摘要环境</li>
                <li><code>\\title</code>, <code>\\author</code>, <code>\\date</code> - 文档元数据</li>
                <li><code>\\maketitle</code> - 标题生成命令</li>
              </ul>
              <p><strong>预览仅显示：</strong></p>
              <ul class="preamble-list">
                <li><code>\\begin{document}</code> 之后的正文内容</li>
                <li>章节、段落、公式、图表等</li>
              </ul>
            </div>
          </div>
        `
        endTimer()
        isRendering.value = false
        return
      }
    }

    let html = contentToRender

    // 过滤掉不应该在预览中显示的环境和命令
    // abstract环境通常出现在导言区，不直接显示在正文中
    html = html.replace(/\\begin\{abstract\}[\s\S]*?\\end\{abstract\}/gi, '')
    // 过滤掉其他常见的导言区命令
    html = html.replace(/\\title\s*\{[^}]*\}/gi, '')
    html = html.replace(/\\author\s*\{[^}]*\}/gi, '')
    html = html.replace(/\\date\s*\{[^}]*\}/gi, '')
    html = html.replace(/\\maketitle/gi, '')
    // 过滤掉tagging相关命令
    html = html.replace(/\\tag\*?\s*\{[^}]*\}/gi, '')
    html = html.replace(/\\tag\s*\*\{[^}]*\}/gi, '')
    // 过滤掉其他文档结构命令
    html = html.replace(/\\tableofcontents/gi, '')
    html = html.replace(/\\listoffigures/gi, '')
    html = html.replace(/\\listoftables/gi, '')
    html = html.replace(/\\bibliographystyle\s*\{[^}]*\}/gi, '')
    html = html.replace(/\\bibliography\{[^}]*\}/gi, '')

    // 处理行内数学公式 $...$
    if (katex) {
      html = html.replace(/\$([^$\n]+?)\$/g, (match, math) => {
        try {
          return katex.renderToString(math, {
            displayMode: false,
            throwOnError: false,
            output: 'html'
          })
        } catch (e) {
          return `<span class="math-inline">${match}</span>`
        }
      })

      // 处理数学公式 $$...$$
      html = html.replace(/\$\$([^$]+?)\$\$/g, (match, math) => {
        try {
          return katex.renderToString(math, {
            displayMode: true,
            throwOnError: false,
            output: 'html'
          })
        } catch (e) {
          return `<div class="math-display">$$${math}$$</div>`
        }
      })
    }

    // 处理LaTeX结构
    html = html.replace(/\\section\*?\{([^}]+)\}/g, '<h2>$1</h2>')
    html = html.replace(/\\subsection\*?\{([^}]+)\}/g, '<h3>$1</h3>')
    html = html.replace(/\\subsubsection\*?\{([^}]+)\}/g, '<h4>$1</h4>')

    // 处理文本格式
    html = html.replace(/\\textbf\{([^}]+)\}/g, '<strong>$1</strong>')
    html = html.replace(/\\textit\{([^}]+)\}/g, '<em>$1</em>')
    html = html.replace(/\\underline\{([^}]+)\}/g, '<u>$1</u>')
    html = html.replace(/\\emph\{([^}]+)\}/g, '<em>$1</em>')

    // 处理列表
    html = html.replace(/\\begin\{itemize\}([\s\S]*?)\\end\{itemize\}/g, (_match, content) => {
      const items = content.split('\\item').filter((s: string) => s.trim())
      return '<ul class="latex-list">' + items.map((item: string) => `<li>${item}</li>`).join('') + '</ul>'
    })

    html = html.replace(/\\begin\{enumerate\}([\s\S]*?)\\end\{enumerate\}/g, (_match, content) => {
      const items = content.split('\\item').filter((s: string) => s.trim())
      return '<ol class="latex-list">' + items.map((item: string) => `<li>${item}</li>`).join('') + '</ol>'
    })

    // ==========================================
    // 处理表格环境
    // ==========================================
    html = html.replace(/\\begin\{tabular\}\{[^}]*\}([\s\S]*?)\\end\{tabular\}/g, (_match, content) => {
      const rows = content.split('\\\\').filter((s: string) => s.trim())
      let tableHtml = '<table class="latex-table"><tbody>'
      rows.forEach((row: string) => {
        const cells = row.split('&').map((cell: string) => cell.trim()).filter((c: string) => c)
        if (cells.length > 0) {
          tableHtml += '<tr>'
          cells.forEach((cell: string) => {
            tableHtml += `<td>${cell}</td>`
          })
          tableHtml += '</tr>'
        }
      })
      tableHtml += '</tbody></table>'
      return tableHtml
    })

    html = html.replace(/\\begin\{table\}([\s\S]*?)\\end\{table\}/g, (_match, content) => {
      const captionMatch = content.match(/\\caption\{([^}]+)\}/)
      const caption = captionMatch ? captionMatch[1] : ''
      const tabularContent = content.replace(/\\caption\{[^}]+\}/g, '').replace(/\\centering/g, '').replace(/\\hline/g, '')
      return `<figure class="latex-figure">${tabularContent}${caption ? `<figcaption>${caption}</figcaption>` : ''}</figure>`
    })

    // ==========================================
    // 处理图片环境
    // ==========================================
    html = html.replace(/\\includegraphics(?:\[[^\]]*\])?\{([^}]+)\}/g, (_match, path) => {
      const alt = path.split('/').pop() || path
      return `<img src="${path}" alt="${alt}" class="latex-image" loading="lazy" onerror="this.style.display='none';this.nextElementSibling?.style.display='inline';" /><span style="display:none;color:var(--el-color-warning);font-size:0.9em;">[图片: ${alt}]</span>`
    })

    html = html.replace(/\\begin\{figure\}([\s\S]*?)\\end\{figure\}/g, (_match, content) => {
      const captionMatch = content.match(/\\caption\{([^}]+)\}/)
      const caption = captionMatch ? captionMatch[1] : ''
      const imageMatch = content.match(/\\includegraphics(?:\[[^\]]*\])?\{([^}]+)\}/)
      const imageHtml = imageMatch ? _match : ''
      const cleanContent = content
        .replace(/\\caption\{[^}]+\}/g, '')
        .replace(/\\centering/g, '')
        .replace(/\\includegraphics[^}]*\}/g, '')
      return `<figure class="latex-figure latex-figure-float">${imageHtml}${caption ? `<figcaption>${caption}</figcaption>` : ''}</figure>`
    })

    // ==========================================
    // 处理引用和参考文献
    // ==========================================
    html = html.replace(/\\cite\{([^}]+)\}/g, '<span class="latex-cite" title="引用: $1">[$1]</span>')
    html = html.replace(/\\ref\{([^}]+)\}/g, '<a href="#$1" class="latex-ref" title="引用: $1">[$1]</a>')
    html = html.replace(/\\eqref\{([^}]+)\}/g, '<a href="#$1" class="latex-ref latex-eqref" title="公式引用: $1">($1)</a>')

    html = html.replace(/\\begin\{thebibliography\}\{[^}]*\}([\s\S]*?)\\end\{thebibliography\}/g, (_match, content) => {
      const items = content.split('\\bibitem').filter((s: string) => s.trim())
      let bibHtml = '<div class="latex-bibliography"><h3>参考文献</h3><ol class="latex-bib-list">'
      items.forEach((item: string) => {
        const labelMatch = item.match(/\{([^}]+)\}/)
        const label = labelMatch ? labelMatch[1] : ''
        const text = item.replace(/\{[^}]+\}/, '').trim()
        if (text) {
          bibHtml += `<li id="${label}">${text}</li>`
        }
      })
      bibHtml += '</ol></div>'
      return bibHtml
    })

    // ==========================================
    // 处理更多数学环境
    // ==========================================
    html = html.replace(/\\begin\{equation\}([^]*?)\\end\{equation\}/g, (_match, math) => {
      if (katex) {
        try {
          const num = math.match(/\\tag\{([^}]+)\}/)
          const cleanMath = math.replace(/\\tag\{[^}]+\}/, '').trim()
          const rendered = katex.renderToString(cleanMath, { displayMode: true, throwOnError: false })
          return `<div class="math-display math-numbered">${rendered}${num ? `<span class="equation-number">(${num[1]})</span>` : ''}</div>`
        } catch (e) {
          return `<div class="math-display">\\begin{equation}${math}\\end{equation}</div>`
        }
      }
      return `<div class="math-display">\\begin{equation}${math}\\end{equation}</div>`
    })

    html = html.replace(/\\begin\{align\}([^]*?)\\end\{align\}/g, (_match, math) => {
      if (katex) {
        try {
          const cleanMath = math.replace(/\\label\{[^}]+\}/g, '').replace(/\\tag\{[^}]+\}/g, '').trim()
          const lines = cleanMath.split('\\\\').filter((s: string) => s.trim())
          return '<div class="math-align">' + lines.map((line: string) => {
            const rendered = katex.renderToString(line.trim(), { displayMode: true, throwOnError: false })
            return `<div class="math-align-row">${rendered}</div>`
          }).join('') + '</div>'
        } catch (e) {
          return `<div class="math-display">\\begin{align}${math}\\end{align}</div>`
        }
      }
      return `<div class="math-display">\\begin{align}${math}\\end{align}</div>`
    })

    // ==========================================
    // 处理其他环境
    // ==========================================
    html = html.replace(/\\begin\{quote\}([\s\S]*?)\\end\{quote\}/g, '<blockquote class="latex-quote">$1</blockquote>')
    html = html.replace(/\\begin\{verbatim\}([\s\S]*?)\\end\{verbatim\}/g, (_match, content) => {
      return `<pre class="latex-verbatim"><code>${content}</code></pre>`
    })
    html = html.replace(/\\begin\{verbatim\*?\}([\s\S]*?)\\end\{verbatim\*?\}/g, (_match, content) => {
      return `<pre class="latex-verbatim"><code>${content}</code></pre>`
    })

    // ==========================================
    // 处理更多文本格式
    // ==========================================
    html = html.replace(/\\texttt\{([^}]+)\}/g, '<code class="inline-code">$1</code>')
    html = html.replace(/\\textsc\{([^}]+)\}/g, '<span style="font-variant: small-caps;">$1</span>')
    html = html.replace(/\\textsuperscript\{([^}]+)\}/g, '<sup>$1</sup>')
    html = html.replace(/\\textsubscript\{([^}]+)\}/g, '<sub>$1</sub>')
    html = html.replace(/\^\{([^}]+)\}/g, '<sup>$1</sup>')
    html = html.replace(/_\{([^}]+)\}/g, '<sub>$1</sub>')

    // 处理换行 - 在段落之前处理
    html = html.replace(/\\\\/g, '\n') // 先将LaTeX换行转为实际换行

    // 按段落分割内容
    const paragraphs = html.split(/\n\n+/).filter((p: string) => p.trim())

    // 处理每个段落
    html = paragraphs.map((para: string) => {
      // 检查是否已经被HTML标签包裹（如表格、列表等）
      if (para.trim().startsWith('<') && !para.trim().startsWith('<p>')) {
        return para
      }
      // 普通文本段落
      return `<p>${para}</p>`
    }).join('\n')

    // 清理未处理的LaTeX命令 - 标记为灰色
    html = html.replace(/\\[a-zA-Z]+(?:\[[^\]]*\])?\{[^}]*\}/g, (match) => {
      // 跳过已处理的命令（已经被替换为HTML）
      if (match.startsWith('<')) return match
      return `<code class="unprocessed-latex" title="未处理的LaTeX命令">${match}</code>`
    })

    // 处理剩余的换行
    html = html.replace(/\n/g, '<br>')

    // 使用DOMPurify清理HTML
    renderedHtml.value = DOMPurify.sanitize(html, {
      ALLOWED_TAGS: ['p', 'br', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'strong', 'em', 'u', 'sub', 'sup', 'code', 'pre', 'blockquote', 'ul', 'ol', 'li', 'table', 'tbody', 'thead', 'tr', 'td', 'th', 'figure', 'figcaption', 'img', 'span', 'a', 'div', 'sup', 'sub', 'center'],
      ALLOWED_ATTR: ['class', 'id', 'style', 'href', 'title', 'src', 'alt', 'loading', 'onerror', 'target'],
      ALLOW_DATA_ATTR: false
    })
    renderError.value = null
  } catch (error) {
    const errorMsg = error instanceof Error ? error.message : String(error)
    renderError.value = errorMsg
    console.warn('[LatexPreview] Render failed:', error)

    // 简单fallback
    let html = contentToRender
    html = html.replace(/\\section\*?\{([^}]+)\}/g, '<h2>$1</h2>')
    html = html.replace(/\\textbf\{([^}]+)\}/g, '<strong>$1</strong>')
    html = html.replace(/\\textit\{([^}]+)\}/g, '<em>$1</em>')
    html = html.replace(/\\\\/g, '<br>')
    renderedHtml.value = DOMPurify.sanitize(html)
  } finally {
    endTimer()
    isRendering.value = false
  }
}

function handleMathClick(event: MouseEvent) {
  const target = event.target as HTMLElement
  if (target.classList.contains('katex-error')) {
    console.warn('Math render error:', target.title)
  }
}

// 监听内容变化 - optimized to prevent memory leaks
const renderTimeout = ref<number | null>(null)

watch(() => props.content, (newContent, oldContent) => {
  if (import.meta.env.DEV) {
    console.log('LatexPreview content changed:', {
      newLength: newContent?.length ?? 0,
      oldLength: oldContent?.length ?? 0
    })
  }

  // Clear any pending render to prevent memory buildup
  if (renderTimeout.value) {
    clearTimeout(renderTimeout.value)
  }

  // Debounced rendering to prevent excessive processing
  renderTimeout.value = setTimeout(() => {
    renderLatex()
    renderTimeout.value = null
  }, newContent && newContent.length > 5000 ? 500 : 200) // Longer delay for large documents
}, { immediate: true })

// 暴露刷新方法
defineExpose({
  refresh: renderLatex
})

// 组件挂载时强制渲染一次
onMounted(() => {
  if (import.meta.env.DEV) {
    console.log('LatexPreview mounted, forcing initial render')
  }
  renderLatex()
})

// 清理定时器防止内存泄漏
onUnmounted(() => {
  if (renderTimeout.value) {
    clearTimeout(renderTimeout.value)
  }
})
</script>

<style scoped lang="scss">
.latex-preview {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);
  overflow: hidden;
}

.preview-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);
}

.preview-title {
  margin: 0;
  font-size: 14px;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.preview-content {
  flex: 1;
  overflow-y: auto;
  padding: 16px;
}

.preview-empty {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 100%;
}

.latex-preamble-info {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100%;
  padding: 40px 20px;
  text-align: center;

  .preamble-hint {
    max-width: 500px;
    margin: 24px auto 0;
    padding: 20px;
    background: var(--el-fill-color-light);
    border-radius: 8px;
    text-align: left;

    p {
      margin: 8px 0;
      font-size: 14px;
      color: var(--el-text-color-regular);
      line-height: 1.6;

      &:first-child {
        margin-top: 0;
      }

      code {
        padding: 2px 6px;
        background: var(--el-fill-color);
        border-radius: 4px;
        font-family: 'Courier New', monospace;
        font-size: 13px;
        color: var(--el-color-primary);
      }
    }

    .preamble-list {
      margin: 12px 0;
      padding-left: 20px;
      text-align: left;

      li {
        margin: 6px 0;
        font-size: 13px;
        color: var(--el-text-color-regular);

        code {
          margin-left: 4px;
        }
      }
    }
  }
}

.preview-error {
  margin: 16px 0;
}

.preview-rendered {
  font-family: 'Latin Modern Math', 'Times New Roman', serif;
  font-size: calc(14px * v-bind('props.scale'));
  line-height: 1.8;
  color: var(--el-text-color-primary);

  // KaTeX 样式调整
  :deep(.katex) {
    font-size: 1.1em;
  }

  :deep(.katex-display) {
    margin: 1em 0;
    overflow-x: auto;
    overflow-y: hidden;
  }

  // 标题样式
  :deep(h2) {
    font-size: 1.5em;
    font-weight: 600;
    margin: 1em 0 0.5em;
    padding-bottom: 0.3em;
    border-bottom: 1px solid var(--el-border-color-lighter);
  }

  :deep(h3) {
    font-size: 1.3em;
    font-weight: 600;
    margin: 0.8em 0 0.4em;
  }

  :deep(h4) {
    font-size: 1.1em;
    font-weight: 600;
    margin: 0.6em 0 0.3em;
  }

  // 列表样式
  :deep(ul), :deep(ol) {
    padding-left: 2em;
    margin: 0.5em 0;
  }

  :deep(li) {
    margin: 0.3em 0;
  }

  // 段落样式
  :deep(p) {
    margin: 0.5em 0;
    text-align: justify;
  }

  // ==========================================
  // 表格样式
  // ==========================================
  :deep(.latex-table) {
    width: 100%;
    border-collapse: collapse;
    margin: 1em 0;
    font-size: 0.95em;

    td {
      border: 1px solid var(--el-border-color-lighter);
      padding: 8px 12px;
      text-align: left;
    }

    tr:first-child td {
      border-top-width: 2px;
      background-color: var(--el-fill-color-light);
      font-weight: 600;
    }

    tr:hover td {
      background-color: var(--el-fill-color-lighter);
    }
  }

  // ==========================================
  // 图片样式
  // ==========================================
  :deep(.latex-image) {
    max-width: 100%;
    height: auto;
    display: block;
    margin: 1em auto;
    border-radius: 4px;
  }

  :deep(.latex-figure) {
    margin: 1.5em 0;
    text-align: center;
  }

  :deep(.latex-figure-float) {
    display: inline-block;
    padding: 1em;
    border: 1px solid var(--el-border-color-lighter);
    border-radius: 4px;
    background-color: var(--el-fill-color-blank);
  }

  :deep(figcaption) {
    margin-top: 0.5em;
    font-size: 0.9em;
    color: var(--el-text-color-secondary);
    font-style: italic;
  }

  // ==========================================
  // 引用和参考文献样式
  // ==========================================
  :deep(.latex-cite) {
    color: var(--el-color-primary);
    font-size: 0.9em;
    cursor: help;
    border-radius: 3px;
    padding: 0 4px;
    background-color: var(--el-fill-color-light);
  }

  :deep(.latex-ref) {
    color: var(--el-color-primary);
    text-decoration: none;
    border-bottom: 1px dotted var(--el-color-primary);
    font-size: 0.9em;
    cursor: pointer;
    transition: all 0.2s;

    &:hover {
      color: var(--el-color-primary-light-3);
      border-bottom-style: solid;
    }
  }

  :deep(.latex-eqref) {
    font-weight: 600;
  }

  :deep(.latex-bibliography) {
    margin-top: 2em;
    padding: 1em;
    background-color: var(--el-fill-color-light);
    border-radius: 4px;

    h3 {
      margin: 0 0 1em 0;
      font-size: 1.2em;
      color: var(--el-text-color-primary);
    }
  }

  :deep(.latex-bib-list) {
    padding-left: 1.5em;
    margin: 0;

    li {
      margin: 0.5em 0;
      padding-left: 0.5em;
      color: var(--el-text-color-regular);
    }
  }

  // ==========================================
  // 数学环境样式
  // ==========================================
  :deep(.math-numbered) {
    position: relative;
    padding-right: 3em;

    .equation-number {
      position: absolute;
      right: 0;
      top: 50%;
      transform: translateY(-50%);
      font-size: 0.9em;
      color: var(--el-text-color-secondary);
    }
  }

  :deep(.math-align) {
    margin: 1em 0;

    .math-align-row {
      display: flex;
      justify-content: center;
      margin: 0.5em 0;
    }
  }

  // ==========================================
  // 其他环境样式
  // ==========================================
  :deep(.latex-quote) {
    margin: 1em 0;
    padding: 0.5em 1em;
    border-left: 4px solid var(--el-border-color);
    background-color: var(--el-fill-color-lighter);
    font-style: italic;
  }

  :deep(.latex-verbatim) {
    margin: 1em 0;
    padding: 1em;
    background-color: var(--el-fill-color);
    border: 1px solid var(--el-border-color);
    border-radius: 4px;
    overflow-x: auto;

    code {
      font-family: 'Courier New', monospace;
      font-size: 0.9em;
      line-height: 1.5;
      color: var(--el-text-color-primary);
    }
  }

  :deep(.inline-code) {
    padding: 2px 6px;
    background-color: var(--el-fill-color);
    border: 1px solid var(--el-border-color-light);
    border-radius: 3px;
    font-family: 'Courier New', monospace;
  }

  :deep(.unprocessed-latex) {
    padding: 2px 4px;
    background-color: #fff3cd;
    border: 1px solid #ffc107;
    border-radius: 3px;
    font-family: 'Courier New', monospace;
    font-size: 0.9em;
    color: #856404;
    white-space: pre-wrap;
    word-break: break-all;
  }

  // 列表样式增强
  :deep(.latex-list) {
    margin: 0.8em 0;
    padding-left: 2em;

    li {
      margin: 0.4em 0;
      line-height: 1.6;
    }
  }

  // 错误样式
  :deep(.katex-error) {
    color: var(--el-color-danger);
    border-bottom: 1px dotted var(--el-color-danger);
    cursor: help;
  }
}

// 深色模式
.dark {
  .preview-rendered {
    :deep(h2), :deep(h3), :deep(h4) {
      border-color: var(--el-border-color);
    }
  }
}
</style>
