/**
 * useEditorActions - Composable for LaTeX editor action functions
 *
 * Extracted from LatexEditorView.vue to reduce its size and improve maintainability.
 * Provides all editor content manipulation actions (insert commands, symbols, tables,
 * citations, macros, images, etc.) and related handlers.
 */
import { ref, nextTick } from 'vue'
import { ElMessage } from 'element-plus'

export function useEditorActions(
  editorContent: Ref<string>,
  editorRef: Ref<any>,
  isModified: Ref<boolean>,
  showSymbolPalette: Ref<boolean>,
  showTableGenerator: Ref<boolean>,
  showTemplates: Ref<boolean>
) {
  // ==========================================
  // Insert LaTeX Command
  // ==========================================

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

  // ==========================================
  // Insert LaTeX Environment
  // ==========================================

  function insertLatexEnvironment(env: string) {
    if (!editorRef.value) return

    const environments: Record<string, [string, string]> = {
      itemize: ['\\begin{itemize}\n  \\item ', '\n\\end{itemize}'],
      enumerate: ['\\begin{enumerate}\n  \\item ', '\n\\end{enumerate}'],
      equation: ['\\begin{equation}\n  ', '\n\\end{equation}'],
      equationstar: ['\\begin{equation*}\n  ', '\n\\end{equation*}'],
      align: ['\\begin{align}\n  ', '\n\\end{align}'],
      alignstar: ['\\begin{align*}\n  ', '\n\\end{align*}'],
      figure: ['\\begin{figure}[h]\n  \\centering\n  \\includegraphics[width=0.8\\textwidth]{', '}\n  \\caption{Caption}\n  \\label{fig:label}\n\\end{figure}'],
      table: ['\\begin{table}[h]\n  \\centering\n  \\begin{tabular}{', '}\n    \\hline\n    % Add your table content here\n    \\hline\n  \\end{tabular}\n  \\caption{Caption}\n  \\label{tab:label}\n\\end{table}'],
      center: ['\\begin{center}\n  ', '\n\\end{center}'],
      flushleft: ['\\begin{flushleft}\n  ', '\n\\end{flushleft}'],
      flushright: ['\\begin{flushright}\n  ', '\n\\end{flushright}']
    }

    const [before, after] = environments[env] || ['\\begin{' + env + '}\n  ', '\n\\end{' + env + '}']

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

  // ==========================================
  // Insert Symbol
  // ==========================================

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

  // ==========================================
  // Insert Table Code
  // ==========================================

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

  // ==========================================
  // Spell Check Handlers
  // ==========================================

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

  // ==========================================
  // Template Content Insert
  // ==========================================

  function insertTemplateContent(content: string) {
    editorContent.value = content
    showTemplates.value = false
    ElMessage.success('模板已应用')
  }

  // ==========================================
  // Quick Insert / Formula / Review
  // ==========================================

  function handleQuickInsert(code: string) {
    insertLatexCommand(code)
  }

  function insertFormula(latex: string) {
    insertLatexCommand(latex)
  }

  function handleFontChange(fontFamily: string) {
    // Font change is handled by FontSelector component via store
    if (import.meta.env.DEV) {
      console.log('Font changed to:', fontFamily)
    }
  }

  function handleReviewModeToggle(enabled: boolean) {
    if (import.meta.env.DEV) {
      console.log('Review mode:', enabled)
    }
  }

  function handleReviewInsert(text: string) {
    insertLatexCommand(text)
  }

  // ==========================================
  // Batch 2 & 3 Component Event Handlers
  // ==========================================

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
      ;(window as any).monacoEditor.setPosition({ lineNumber, column: 1 })
      ;(window as any).monacoEditor.focus()
    }
  }

  function handleCommandExecuted(command: any) {
    console.log('Command executed:', command)
    // 可以根据命令执行特定操作
  }

  // ==========================================
  // Export Helpers
  // ==========================================

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

  return {
    insertLatexCommand,
    insertLatexEnvironment,
    insertSymbol,
    insertTableCode,
    handleSpellReplace,
    handleSpellGoto,
    insertTemplateContent,
    handleQuickInsert,
    insertFormula,
    handleFontChange,
    handleReviewModeToggle,
    handleReviewInsert,
    handleInsertCitation,
    handleInsertMacro,
    handleInsertImage,
    handleCheckerFix,
    handleJumpToLine,
    handleCommandExecuted,
    downloadAsTex,
    convertToMarkdown
  }
}

// Need to import Ref type
import type { Ref } from 'vue'
