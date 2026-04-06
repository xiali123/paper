<template>
  <div class="latex-preview">
    <div class="preview-header">
      <h4 class="preview-title">预览</h4>
      <el-tag v-if="renderError" type="danger" size="small">渲染错误</el-tag>
    </div>

    <div class="preview-content" ref="contentRef">
      <div v-if="!content" class="preview-empty">
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
import { ref, computed, watch, nextTick } from 'vue'
import katex from 'katex'
import 'katex/dist/katex.min.css'

interface Props {
  content: string
  theme?: 'light' | 'dark'
}

const props = withDefaults(defineProps<Props>(), {
  theme: 'light'
})

const contentRef = ref<HTMLElement>()
const renderError = ref<string | null>(null)
const renderedHtml = ref('')

// 渲染 LaTeX 内容
function renderLatex() {
  if (!props.content?.trim()) {
    renderedHtml.value = ''
    renderError.value = null
    return
  }

  try {
    // 提取数学公式并渲染
    let html = props.content

    // 渲染行内公式 $...$
    html = html.replace(/\$([^$\n]+?)\$/g, (match, math) => {
      try {
        return katex.renderToString(math, {
          displayMode: false,
          throwOnError: false,
          output: 'html',
          strict: false
        })
      } catch (e) {
        console.warn('KaTeX render error:', e)
        return `<span class="katex-error" title="${e}">$${math}$</span>`
      }
    })

    // 渲染块级公式 $$...$$
    html = html.replace(/\$\$([^$]+?)\$\$/g, (match, math) => {
      try {
        return katex.renderToString(math, {
          displayMode: true,
          throwOnError: false,
          output: 'html',
          strict: false
        })
      } catch (e) {
        console.warn('KaTeX render error:', e)
        return `<div class="katex-error" title="${e}">$$${math}$$</div>`
      }
    })

    // 处理 LaTeX 环境（简单渲染，不使用 KaTeX）
    // 将常见的 LaTeX 命令转换为 HTML
    html = renderLatexStructure(html)

    renderedHtml.value = html
    renderError.value = null
  } catch (e) {
    renderError.value = e instanceof Error ? e.message : String(e)
  }
}

// 简单的 LaTeX 结构渲染
function renderLatexStructure(latex: string): string {
  let html = latex

  // 处理章节
  html = html.replace(/\\section\*?\{([^}]+)\}/g, '<h2>$1</h2>')
  html = html.replace(/\\subsection\*?\{([^}]+)\}/g, '<h3>$1</h3>')
  html = html.replace(/\\subsubsection\*?\{([^}]+)\}/g, '<h4>$1</h4>')

  // 处理文本格式
  html = html.replace(/\\textbf\{([^}]+)\}/g, '<strong>$1</strong>')
  html = html.replace(/\\textit\{([^}]+)\}/g, '<em>$1</em>')
  html = html.replace(/\\underline\{([^}]+)\}/g, '<u>$1</u>')
  html = html.replace(/\\emph\{([^}]+)\}/g, '<em>$1</em>')

  // 处理列表（简化）
  html = html.replace(/\\begin\{itemize\}([\s\S]*?)\\end\{itemize\}/g, (_, content) => {
    const items = content.split('\\item').filter(s => s.trim())
    return '<ul>' + items.map(item => `<li>${item}</li>`).join('') + '</ul>'
  })

  html = html.replace(/\\begin\{enumerate\}([\s\S]*?)\\end\{enumerate\}/g, (_, content) => {
    const items = content.split('\\item').filter(s => s.trim())
    return '<ol>' + items.map(item => `<li>${item}</li>`).join('') + '</ol>'
  })

  // 处理换行
  html = html.replace(/\\\\/g, '<br>')
  html = html.replace(/\n\n/g, '</p><p>')
  html = '<p>' + html + '</p>'

  // 清理空标签
  html = html.replace(/<p>\s*<\/p>/g, '')
  html = html.replace(/<([ou])l>\s*<\/\1l>/g, '')

  return html
}

function handleMathClick(event: MouseEvent) {
  const target = event.target as HTMLElement
  if (target.classList.contains('katex-error')) {
    console.warn('Math render error:', target.title)
  }
}

// 监听内容变化
watch(() => props.content, () => {
  renderLatex()
}, { immediate: true })

// 暴露刷新方法
defineExpose({
  refresh: renderLatex
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

.preview-error {
  margin: 16px 0;
}

.preview-rendered {
  font-family: 'Latin Modern Math', 'Times New Roman', serif;
  font-size: 14px;
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
