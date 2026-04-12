import { ref, computed, watch, onMounted, onUnmounted } from 'vue'

export interface AutocompleteOption {
  /** 显示文本 */
  label: string
  /** 插入内容 */
  value: string
  /** 描述 */
  description?: string
  /** 类型 */
  type: 'command' | 'environment' | 'symbol' | 'snippet'
  /** 图标 */
  icon?: string
}

export interface AutocompleteOptions {
  /** 编辑器元素 */
  editorElement: Ref<HTMLElement | undefined>
  /** 当前内容 */
  content: Ref<string>
  /** 选择选项回调 */
  onSelect: (option: AutocompleteOption) => void
  /** 是否启用 */
  enabled?: Ref<boolean> | boolean
  /** 触发字符 */
  triggerChar?: string
}

/**
 * LaTeX 命令定义 - 扩展版
 */
const LATEX_COMMANDS: AutocompleteOption[] = [
  // ==========================================
  // 文档结构 (Document Structure)
  // ==========================================
  { label: '\\documentclass', value: '\\documentclass[$1]{$2}', type: 'command', description: '文档类' },
  { label: '\\usepackage', value: '\\usepackage{$1}', type: 'command', description: '加载宏包' },
  { label: '\\begin{document}', value: '\\begin{document}\n$1\n\\end{document}', type: 'environment', description: '文档环境' },
  { label: '\\section', value: '\\section{$1}', type: 'command', description: '章节' },
  { label: '\\subsection', value: '\\subsection{$1}', type: 'command', description: '子章节' },
  { label: '\\subsubsection', value: '\\subsubsection{$1}', type: 'command', description: '次子章节' },
  { label: '\\paragraph', value: '\\paragraph{$1}', type: 'command', description: '段落标题' },
  { label: '\\subparagraph', value: '\\subparagraph{$1}', type: 'command', description: '次段落' },
  { label: '\\part', value: '\\part{$1}', type: 'command', description: '部分' },
  { label: '\\chapter', value: '\\chapter{$1}', type: 'command', description: '章' },
  { label: '\\title', value: '\\title{$1}', type: 'command', description: '标题' },
  { label: '\\author', value: '\\author{$1}', type: 'command', description: '作者' },
  { label: '\\date', value: '\\date{$1}', type: 'command', description: '日期' },
  { label: '\\maketitle', value: '\\maketitle', type: 'command', description: '生成标题' },
  { label: '\\abstract', value: '\\begin{abstract}\n$1\n\\end{abstract}', type: 'environment', description: '摘要' },

  // ==========================================
  // 文本格式 (Text Formatting)
  // ==========================================
  { label: '\\textbf', value: '\\textbf{$1}', type: 'command', description: '粗体' },
  { label: '\\textit', value: '\\textit{$1}', type: 'command', description: '斜体' },
  { label: '\\textmd', value: '\\textmd{$1}', type: 'command', description: '中等字体' },
  { label: '\\texttt', value: '\\texttt{$1}', type: 'command', description: '等宽字体' },
  { label: '\\textup', value: '\\textup{$1}', type: 'command', description: '直立字体' },
  { label: '\\textsl', value: '\\textsl{$1}', type: 'command', description: '倾斜字体' },
  { label: '\\textsc', value: '\\textsc{$1}', type: 'command', description: '小型大写' },
  { label: '\\textsf', value: '\\textsf{$1}', type: 'command', description: '无衬线' },
  { label: '\\emph', value: '\\emph{$1}', type: 'command', description: '强调' },
  { label: '\\underline', value: '\\underline{$1}', type: 'command', description: '下划线' },
  { label: '\\uppercase', value: '{\\uppercase $1}', type: 'snippet', description: '大写' },
  { label: '\\lowercase', value: '{\\lowercase $1}', type: 'snippet', description: '小写' },
  { label: '\\textcolor', value: '\\textcolor{$1}{$2}', type: 'command', description: '文本颜色' },
  { label: '\\colorbox', value: '\\colorbox{$1}{$2}', type: 'command', description: '彩色背景框' },
  { label: '\\fbox', value: '\\fbox{$1}', type: 'command', description: '边框框' },
  { label: '\\framebox', value: '\\framebox{$1}', type: 'command', description: '框架框' },

  // ==========================================
  // 字体大小 (Font Sizes)
  // ==========================================
  { label: '\\tiny', value: '{\\tiny $1}', type: 'snippet', description: '极小' },
  { label: '\\scriptsize', value: '{\\scriptsize $1}', type: 'snippet', description: '极小号' },
  { label: '\\footnotesize', value: '{\\footnotesize $1}', type: 'snippet', description: '脚注号' },
  { label: '\\small', value: '{\\small $1}', type: 'snippet', description: '小号' },
  { label: '\\normalsize', value: '{\\normalsize $1}', type: 'snippet', description: '正常号' },
  { label: '\\large', value: '{\\large $1}', type: 'snippet', description: '大号' },
  { label: '\\Large', value: '{\\Large $1}', type: 'snippet', description: '大号' },
  { label: '\\LARGE', value: '{\\LARGE $1}', type: 'snippet', description: '特大号' },
  { label: '\\huge', value: '{\\huge $1}', type: 'snippet', description: '巨大号' },
  { label: '\\Huge', value: '{\\Huge $1}', type: 'snippet', description: '最大号' },

  // ==========================================
  // 对齐 (Alignment)
  // ==========================================
  { label: '\\centering', value: '\\centering', type: 'command', description: '居中对齐' },
  { label: '\\raggedright', value: '\\raggedright', type: 'command', description: '右对齐' },
  { label: '\\raggedleft', value: '\\raggedleft', type: 'command', description: '左对齐' },
  { label: '\\hfill', value: '\\hfill', type: 'command', description: '水平填充' },
  { label: '\\vfill', value: '\\vfill', type: 'command', description: '垂直填充' },
  { label: '\\hrulefill', value: '\\hrulefill', type: 'command', description: '水平线填充' },
  { label: '\\dotfill', value: '\\dotfill', type: 'command', description: '点线填充' },

  // ==========================================
  // 空白 (Spacing)
  // ==========================================
  { label: '\\quad', value: '\\quad', type: 'command', description: '1em空格' },
  { label: '\\qquad', value: '\\qquad', type: 'command', description: '2em空格' },
  { label: '\\!', value: '\\!', type: 'command', description: '普通空格' },
  { label: '\\:', value: '\\:', type: 'command', description: '中等空格' },
  { label: '\\;', value: '\\;', type: 'command', description: '较大空格' },
  { label: '\\\\', value: '\\\\', type: 'command', description: '换行' },
  { label: '\\newline', value: '\\newline', type: 'command', description: '新行' },
  { label: '\\linebreak', value: '\\linebreak', type: 'command', description: '断行' },
  { label: '\\pagebreak', value: '\\pagebreak', type: 'command', description: '分页' },
  { label: '\\nopagebreak', value: '\\nopagebreak', type: 'command', description: '禁止分页' },
  { label: '\\vspace', value: '\\vspace{$1}', type: 'command', description: '垂直空间' },
  { label: '\\hspace', value: '\\hspace{$1}', type: 'command', description: '水平空间' },
  { label: '\\vspace*', value: '\\vspace*{$1}', type: 'command', description: '强制垂直空间' },
  { label: '\\hspace*', value: '\\hspace*{$1}', type: 'command', description: '强制水平空间' },

  // ==========================================
  // 列表环境 (List Environments)
  // ==========================================
  { label: '\\begin{itemize}', value: '\\begin{itemize}\n  \\item $1\n  \\item \n\\end{itemize}', type: 'environment', description: '无序列表' },
  { label: '\\begin{enumerate}', value: '\\begin{enumerate}\n  \\item $1\n  \\item \n\\end{enumerate}', type: 'environment', description: '有序列表' },
  { label: '\\begin{description}', value: '\\begin{description}\n  \\item[$1] $2\n\\end{description}', type: 'environment', description: '描述列表' },
  { label: '\\item', value: '\\item', type: 'command', description: '列表项' },

  // ==========================================
  // 数学环境 (Math Environments)
  // ==========================================
  { label: '\\begin{equation}', value: '\\begin{equation}\n  $1\n\\end{equation}', type: 'environment', description: '行间公式(编号)' },
  { label: '\\begin{equation*}', value: '\\begin{equation*}\n  $1\n\\end{equation*}', type: 'environment', description: '行间公式(无编号)' },
  { label: '\\begin{align}', value: '\\begin{align}\n  $1\n\\end{align}', type: 'environment', description: '对齐公式(编号)' },
  { label: '\\begin{align*}', value: '\\begin{align*}\n  $1\n\\end{align*}', type: 'environment', description: '对齐公式(无编号)' },
  { label: '\\begin{gather}', value: '\\begin{gather}\n  $1\n\\end{gather}', type: 'environment', description: '聚集公式' },
  { label: '\\begin{multline}', value: '\\begin{multline}\n  $1\n\\end{multline}', type: 'environment', description: '多行公式' },
  { label: '\\begin{split}', value: '\\begin{split}\n  $1\n\\end{split}', type: 'environment', description: '分割公式' },
  { label: '\\begin{matrix}', value: '\\begin{matrix}\n  $1 & $2 \\\\\n  $3 & $4\n\\end{matrix}', type: 'environment', description: '矩阵' },
  { label: '\\begin{pmatrix}', value: '\\begin{pmatrix}\n  $1 & $2 \\\\\n  $3 & $4\n\\end{pmatrix}', type: 'environment', description: '圆括号矩阵' },
  { label: '\\begin{bmatrix}', value: '\\begin{bmatrix}\n  $1 & $2 \\\\\n  $3 & $4\n\\end{bmatrix}', type: 'environment', description: '方括号矩阵' },
  { label: '\\begin{vmatrix}', value: '\\begin{vmatrix}\n  $1 & $2 \\\\\n  $3 & $4\n\\end{vmatrix}', type: 'environment', description: '竖线矩阵' },
  { label: '\\begin{Vmatrix}', value: '\\begin{Vmatrix}\n  $1 & $2 \\\\\n  $3 & $4\n\\end{Vmatrix}', type: 'environment', description: '双竖线矩阵' },
  { label: '\\begin{cases}', value: '\\begin{cases}\n  $1 & $2 \\\\\n  $3 & $4\n\\end{cases}', type: 'environment', description: '分段函数' },

  // ==========================================
  // 数学符号 - 希腊字母 (Greek Letters)
  // ==========================================
  { label: '\\alpha', value: '\\alpha', type: 'symbol', description: 'α' },
  { label: '\\beta', value: '\\beta', type: 'symbol', description: 'β' },
  { label: '\\gamma', value: '\\gamma', type: 'symbol', description: 'γ' },
  { label: '\\delta', value: '\\delta', type: 'symbol', description: 'δ' },
  { label: '\\epsilon', value: '\\epsilon', type: 'symbol', description: 'ε' },
  { label: '\\varepsilon', value: '\\varepsilon', type: 'symbol', description: 'ε (变体)' },
  { label: '\\zeta', value: '\\zeta', type: 'symbol', description: 'ζ' },
  { label: '\\eta', value: '\\eta', type: 'symbol', description: 'η' },
  { label: '\\theta', value: '\\theta', type: 'symbol', description: 'θ' },
  { label: '\\vartheta', value: '\\vartheta', type: 'symbol', description: 'θ (变体)' },
  { label: '\\iota', value: '\\iota', type: 'symbol', description: 'ι' },
  { label: '\\kappa', value: '\\kappa', type: 'symbol', description: 'κ' },
  { label: '\\lambda', value: '\\lambda', type: 'symbol', description: 'λ' },
  { label: '\\mu', value: '\\mu', type: 'symbol', description: 'μ' },
  { label: '\\nu', value: '\\nu', type: 'symbol', description: 'ν' },
  { label: '\\xi', value: '\\xi', type: 'symbol', description: 'ξ' },
  { label: '\\pi', value: '\\pi', type: 'symbol', description: 'π' },
  { label: '\\rho', value: '\\rho', type: 'symbol', description: 'ρ' },
  { label: '\\sigma', value: '\\sigma', type: 'symbol', description: 'σ' },
  { label: '\\varsigma', value: '\\varsigma', type: 'symbol', description: 'σ (变体)' },
  { label: '\\tau', value: '\\tau', type: 'symbol', description: 'τ' },
  { label: '\\upsilon', value: '\\upsilon', type: 'symbol', description: 'υ' },
  { label: '\\phi', value: '\\phi', type: 'symbol', description: 'φ' },
  { label: '\\varphi', value: '\\varphi', type: 'symbol', description: 'φ (变体)' },
  { label: '\\chi', value: '\\chi', type: 'symbol', description: 'χ' },
  { label: '\\psi', value: '\\psi', type: 'symbol', description: 'ψ' },
  { label: '\\omega', value: '\\omega', type: 'symbol', description: 'ω' },

  // 大写希腊字母
  { label: '\\Gamma', value: '\\Gamma', type: 'symbol', description: 'Γ' },
  { label: '\\Delta', value: '\\Delta', type: 'symbol', description: 'Δ' },
  { label: '\\Theta', value: '\\Theta', type: 'symbol', description: 'Θ' },
  { label: '\\Lambda', value: '\\Lambda', type: 'symbol', description: 'Λ' },
  { label: '\\Xi', value: '\\Xi', type: 'symbol', description: 'Ξ' },
  { label: '\\Pi', value: '\\Pi', type: 'symbol', description: 'Π' },
  { label: '\\Sigma', value: '\\Sigma', type: 'symbol', description: 'Σ' },
  { label: '\\Upsilon', value: '\\Upsilon', type: 'symbol', description: 'Υ' },
  { label: '\\Phi', value: '\\Phi', type: 'symbol', description: 'Φ' },
  { label: '\\Psi', value: '\\Psi', type: 'symbol', description: 'Ψ' },
  { label: '\\Omega', value: '\\Omega', type: 'symbol', description: 'Ω' },

  // ==========================================
  // 数学运算符 (Math Operators)
  // ==========================================
  { label: '\\sum', value: '\\sum', type: 'symbol', description: '求和' },
  { label: '\\prod', value: '\\prod', type: 'symbol', description: '乘积' },
  { label: '\\coprod', value: '\\coprod', type: 'symbol', description: '余积' },
  { label: '\\int', value: '\\int', type: 'symbol', description: '积分' },
  { label: '\\iint', value: '\\iint', type: 'symbol', description: '二重积分' },
  { label: '\\iiint', value: '\\iiint', type: 'symbol', description: '三重积分' },
  { label: '\\oint', value: '\\oint', type: 'symbol', description: '环积分' },
  { label: '\\partial', value: '\\partial', type: 'symbol', description: '偏微分' },
  { label: '\\nabla', value: '\\nabla', type: 'symbol', description: '哈密顿算子' },
  { label: '\\infty', value: '\\infty', type: 'symbol', description: '无穷' },

  // ==========================================
  // 数学关系符 (Math Relations)
  // ==========================================
  { label: '\\le', value: '\\le', type: 'symbol', description: '≤' },
  { label: '\\ge', value: '\\ge', type: 'symbol', description: '≥' },
  { label: '\\leq', value: '\\leq', type: 'symbol', description: '≤' },
  { label: '\\geq', value: '\\geq', type: 'symbol', description: '≥' },
  { label: '\\ne', value: '\\ne', type: 'symbol', description: '≠' },
  { label: '\\neq', value: '\\neq', type: 'symbol', description: '≠' },
  { label: '\\approx', value: '\\approx', type: 'symbol', description: '≈' },
  { label: '\\equiv', value: '\\equiv', type: 'symbol', description: '≡' },
  { label: '\\sim', value: '\\sim', type: 'symbol', description: '~' },
  { label: '\\simeq', value: '\\simeq', type: 'symbol', description: '≃' },
  { label: '\\cong', value: '\\cong', type: 'symbol', description: '≌' },
  { label: '\\propto', value: '\\propto', type: 'symbol', description: '∝' },

  // ==========================================
  // 数学运算 (Math Operations)
  // ==========================================
  { label: '\\frac', value: '\\frac{$1}{$2}', type: 'command', description: '分数' },
  { label: '\\sqrt', value: '\\sqrt{$1}', type: 'command', description: '平方根' },
  { label: '\\sqrt[n]', value: '\\sqrt[$1]{$2}', type: 'command', description: 'n次方根' },
  { label: '\\hat', value: '\\hat{$1}', type: 'command', description: '抑扬符' },
  { label: '\\bar', value: '\\bar{$1}', type: 'command', description: '上划线' },
  { label: '\\vec', value: '\\vec{$1}', type: 'command', description: '向量' },
  { label: '\\dot', value: '\\dot{$1}', type: 'command', description: '导数点' },
  { label: '\\ddot', value: '\\ddot{$1}', type: 'command', description: '二阶导数' },
  { label: '\\overrightarrow', value: '\\overrightarrow{$1}', type: 'command', description: '右箭头' },
  { label: '\\overleftarrow', value: '\\overleftarrow{$1}', type: 'command', description: '左箭头' },
  { label: '\\overline', value: '\\overline{$1}', type: 'command', description: '上划线' },
  { label: '\\underline', value: '\\underline{$1}', type: 'command', description: '下划线' },
  { label: '\\overbrace', value: '\\overbrace{$1}', type: 'command', description: '上花括号' },
  { label: '\\underbrace', value: '\\underbrace{$1}', type: 'command', description: '下花括号' },
  { label: '\\overset', value: '\\overset{$1}{$2}', type: 'command', description: '上标符号' },
  { label: '\\underset', value: '\\underset{$1}{$2}', type: 'command', description: '下标符号' },

  // ==========================================
  // 常用数学符号
  // ==========================================
  { label: '\\pm', value: '\\pm', type: 'symbol', description: '±' },
  { label: '\\mp', value: '\\mp', type: 'symbol', description: '∓' },
  { label: '\\times', value: '\\times', type: 'symbol', description: '×' },
  { label: '\\div', value: '\\div', type: 'symbol', description: '÷' },
  { label: '\\cdot', value: '\\cdot', type: 'symbol', description: '·' },
  { label: '\\ast', value: '\\ast', type: 'symbol', description: '∗' },
  { label: '\\star', value: '\\star', type: 'symbol', description: '⋆' },
  { label: '\\circ', value: '\\circ', type: 'symbol', description: '∘' },
  { label: '\\oplus', value: '\\oplus', type: 'symbol', description: '⊕' },
  { label: '\\ominus', value: '\\ominus', type: 'symbol', description: '⊖' },
  { label: '\\otimes', value: '\\otimes', type: 'symbol', description: '⊗' },
  { label: '\\oslash', value: '\\oslash', type: 'symbol', description: '⊘' },
  { label: '\\cap', value: '\\cap', type: 'symbol', description: '∩' },
  { label: '\\cup', value: '\\cup', type: 'symbol', description: '∪' },
  { label: '\\setminus', value: '\\setminus', type: 'symbol', description: '∖' },

  // ==========================================
  // 括号 (Brackets & Delimiters)
  // ==========================================
  { label: '\\left(', value: '\\left($1\\right)', type: 'command', description: '自适应左括号' },
  { label: '\\right)', value: '\\left($1\\right)', type: 'command', description: '自适应右括号' },
  { label: '\\left[', value: '\\left[$1\\right]', type: 'command', description: '自适应左方括号' },
  { label: '\\right]', value: '\\left[$1\\right]', type: 'command', description: '自适应右方括号' },
  { label: '\\left\\{', value: '\\left\\{$1\\right\\}', type: 'command', description: '自适应左花括号' },
  { label: '\\right\\}', value: '\\left\\{$1\\right\\}', type: 'command', description: '自适应右花括号' },
  { label: '\\langle', value: '\\langle', type: 'symbol', description: '⟨' },
  { label: '\\rangle', value: '\\rangle', type: 'symbol', description: '⟩' },
  { label: '\\lvert', value: '\\lvert', type: 'symbol', description: '|' },
  { label: '\\rvert', value: '\\rvert', type: 'symbol', description: '|' },

  // ==========================================
  // 表格 (Tables)
  // ==========================================
  { label: '\\begin{tabular}', value: '\\begin{tabular}{$1}\n  $2 \\\\\n  \\hline\n\\end{tabular}', type: 'environment', description: '表格环境' },
  { label: '\\begin{table}', value: '\\begin{table}[h]\n  \\centering\n  \\caption{$1}\n  \\begin{tabular}{|c|c|}\n  \\hline\n  $2 \\\\n  \\hline\n  \\end{tabular}\n\\end{table}', type: 'environment', description: '浮动表格' },
  { label: '\\hline', value: '\\hline', type: 'command', description: '表格横线' },
  { label: '\\cline', value: '\\cline{$1-$2}', type: 'command', description: '部分横线' },
  { label: '\\multicolumn', value: '\\multicolumn{$1}{$2}{$3}', type: 'command', description: '多列单元格' },

  // ==========================================
  // 图片 (Figures)
  // ==========================================
  { label: '\\begin{figure}', value: '\\begin{figure}[h]\n  \\centering\n  \\includegraphics[width=0.8\\textwidth]{$1}\n  \\caption{$2}\n  \\label{fig:$3}\n\\end{figure}', type: 'environment', description: '图片环境' },
  { label: '\\includegraphics', value: '\\includegraphics[width=0.8\\textwidth]{$1}', type: 'command', description: '插入图片' },
  { label: '\\caption', value: '\\caption{$1}', type: 'command', description: '标题' },
  { label: '\\label', value: '\\label{$1}', type: 'command', description: '标签' },

  // ==========================================
  // 引用 (References)
  // ==========================================
  { label: '\\cite', value: '\\cite{$1}', type: 'command', description: '引用文献' },
  { label: '\\ref', value: '\\ref{$1}', type: 'command', description: '引用标签' },
  { label: '\\eqref', value: '\\eqref{$1}', type: 'command', description: '引用公式' },
  { label: '\\pageref', value: '\\pageref{$1}', type: 'command', description: '引用页码' },
  { label: '\\bibliographystyle', value: '\\bibliographystyle{$1}', type: 'command', description: '参考文献样式' },
  { label: '\\bibliography', value: '\\bibliography{$1}\n\\end{bibliography}', type: 'environment', description: '参考文献环境' },
  { label: '\\thebibliography', value: '\\begin{thebibliography}{$1}\n  \\bibitem\n\\end{thebibliography}', type: 'environment', description: '参考文献列表' },

  // ==========================================
  // 定理环境 (Theorem Environments)
  // ==========================================
  { label: '\\begin{theorem}', value: '\\begin{theorem}\n  $1\n\\end{theorem}', type: 'environment', description: '定理' },
  { label: '\\begin{lemma}', value: '\\begin{lemma}\n  $1\n\\end{lemma}', type: 'environment', description: '引理' },
  { label: '\\begin{proposition}', value: '\\begin{proposition}\n  $1\n\\end{proposition}', type: 'environment', description: '命题' },
  { label: '\\begin{corollary}', value: '\\begin{corollary}\n  $1\n\\end{corollary}', type: 'environment', description: '推论' },
  { label: '\\begin{definition}', value: '\\begin{definition}\n  $1\n\\end{definition}', type: 'environment', description: '定义' },
  { label: '\\begin{example}', value: '\\begin{example}\n  $1\n\\end{example}', type: 'environment', description: '示例' },
  { label: '\\begin{proof}', value: '\\begin{proof}\n  $1\n\\end{proof}', type: 'environment', description: '证明' },

  // ==========================================
  // 代码 (Code)
  // ==========================================
  { label: '\\begin{verbatim}', value: '\\begin{verbatim}\n$1\n\\end{verbatim}', type: 'environment', description: '逐字记录' },
  { label: '\\begin{verbatim*}', value: '\\begin{verbatim*}\n$1\n\\end{verbatim*}', type: 'environment', description: '逐字记录(无格式)' },
  { label: '\\texttt', value: '\\texttt{$1}', type: 'command', description: '等宽字体' },
  { label: '\\textmd', value: '\\textmd{$1}', type: 'command', description: '中等字体' },
  { label: '\\verb', value: '\\verb|$1|', type: 'command', description: '逐字' },
  { label: '\\lstinline', value: '\\lstinline|$1|', type: 'command', description: '行内代码' },
  { label: '\\begin{lstlisting}', value: '\\begin{lstlisting}\n$1\n\\end{lstlisting}', type: 'environment', description: '代码块' },

  // ==========================================
  // 其他有用命令
  // ==========================================
  { label: '\\href', value: '\\href{$1}{$2}', type: 'command', description: '超链接' },
  { label: '\\url', value: '\\url{$1}', type: 'command', description: 'URL' },
  { label: '\\footnote', value: '\\footnote{$1}', type: 'command', description: '脚注' },
  { label: '\\marginpar', value: '\\marginpar{$1}', type: 'command', description: '边注' },
  { label: '\\index', value: '\\index{$1}', type: 'command', description: '索引条目' },
  { label: '\\printindex', value: '\\printindex', type: 'command', description: '打印索引' },
  { label: '\\tableofcontents', value: '\\tableofcontents', type: 'command', description: '目录' },
  { label: '\\listoffigures', value: '\\listoffigures', type: 'command', description: '图片列表' },
  { label: '\\listoftables', value: '\\listoftables', type: 'command', description: '表格列表' },
  { label: '\\appendix', value: '\\appendix', type: 'command', description: '附录开始' },
  { label: '\\bibliographystyle', value: '\\bibliographystyle{$1}', type: 'command', description: '参考文献样式' },
  { label: '\\newpage', value: '\\newpage', type: 'command', description: '新页面' },
  { label: '\\clearpage', value: '\\clearpage', type: 'command', description: '清除浮动元素并分页' },
  { label: '\\thispagestyle', value: '\\thispagestyle{$1}', type: 'command', description: '当前页样式' },
]

/**
 * LaTeX 自动补全 Hook
 */
export function useLatexAutocomplete(options: AutocompleteOptions) {
  const {
    editorElement,
    content,
    onSelect,
    enabled = ref(true),
    triggerChar = '\\'
  } = options

  // 状态
  const isVisible = ref(false)
  const position = ref({ x: 0, y: 0 })
  const query = ref('')
  const selectedIndex = ref(0)
  const isEnabled = ref(typeof enabled === 'boolean' ? enabled : enabled.value)

  // 监听 enabled 变化
  if (typeof enabled !== 'boolean') {
    watch(enabled, (value) => {
      isEnabled.value = value
    })
  }

  // 过滤选项
  const filteredOptions = computed(() => {
    if (!query.value) return []

    const q = query.value.toLowerCase()
    return LATEX_COMMANDS.filter(option =>
      option.label.toLowerCase().includes(q) ||
      option.description?.toLowerCase().includes(q)
    ).slice(0, 10) // 限制显示数量
  })

  /**
   * 显示自动补全
   */
  function showAutocomplete(x: number, y: number, q: string) {
    if (!isEnabled.value) return

    position.value = { x, y }
    query.value = q
    selectedIndex.value = 0
    isVisible.value = true
  }

  /**
   * 隐藏自动补全
   */
  function hideAutocomplete() {
    isVisible.value = false
    query.value = ''
  }

  /**
   * 选择当前高亮的选项
   */
  function selectCurrent() {
    const options = filteredOptions.value
    if (options.length === 0) return

    const option = options[selectedIndex.value]
    if (option) {
      onSelect(option)
      hideAutocomplete()
    }
  }

  /**
   * 选择指定选项
   */
  function selectOption(index: number) {
    const options = filteredOptions.value
    if (index >= 0 && index < options.length) {
      selectedIndex.value = index
      selectCurrent()
    }
  }

  /**
   * 导航到上一个选项
   */
  function navigateUp() {
    const options = filteredOptions.value
    if (options.length === 0) return

    selectedIndex.value = (selectedIndex.value - 1 + options.length) % options.length
  }

  /**
   * 导航到下一个选项
   */
  function navigateDown() {
    const options = filteredOptions.value
    if (options.length === 0) return

    selectedIndex.value = (selectedIndex.value + 1) % options.length
  }

  /**
   * 处理输入事件
   */
  function handleInput(event: KeyboardEvent) {
    if (!isEnabled.value) return

    const textarea = event.target as HTMLTextAreaElement
    if (!textarea) return

    const cursorPos = textarea.selectionStart
    const textBefore = textarea.value.substring(0, cursorPos)

    // 检查是否在 triggerChar 之后
    const triggerIndex = textBefore.lastIndexOf(triggerChar)

    if (triggerIndex !== -1) {
      // 检查 triggerChar 后面是否有空格或换行
      const afterTrigger = textBefore.substring(triggerIndex + 1)
      if (/\s/.test(afterTrigger)) {
        hideAutocomplete()
        return
      }

      // 获取查询字符串
      const queryStr = triggerChar + afterTrigger

      // 获取光标位置
      const rect = textarea.getBoundingClientRect()
      const lineHeight = parseInt(getComputedStyle(textarea).lineHeight)
      const charWidth = parseInt(getComputedStyle(textarea).fontSize) * 0.6

      // 计算弹出位置（简化版）
      const lines = textBefore.split('\n')
      const line = lines.length
      const col = lines[lines.length - 1].length

      const x = rect.left + col * charWidth
      const y = rect.top + line * lineHeight + 30

      showAutocomplete(x, y, queryStr)
    } else {
      hideAutocomplete()
    }
  }

  /**
   * 处理键盘事件
   */
  function handleKeydown(event: KeyboardEvent) {
    if (!isVisible.value) return

    switch (event.key) {
      case 'ArrowDown':
        event.preventDefault()
        navigateDown()
        break
      case 'ArrowUp':
        event.preventDefault()
        navigateUp()
        break
      case 'Enter':
      case 'Tab':
        event.preventDefault()
        selectCurrent()
        break
      case 'Escape':
        event.preventDefault()
        hideAutocomplete()
        break
    }
  }

  /**
   * 设置事件监听
   */
  function setupListeners() {
    const textarea = editorElement.value
    if (!textarea) return

    textarea.addEventListener('input', handleInput as EventListener)
    textarea.addEventListener('keydown', handleKeydown)
  }

  /**
   * 移除事件监听
   */
  function removeListeners() {
    const textarea = editorElement.value
    if (!textarea) return

    textarea.removeEventListener('input', handleInput as EventListener)
    textarea.removeEventListener('keydown', handleKeydown)
  }

  // 监听编辑器元素变化
  watch(editorElement, () => {
    removeListeners()
    setupListeners()
  })

  // 点击外部关闭
  const handleClickOutside = (event: MouseEvent) => {
    if (!isVisible.value) return

    const autocomplete = document.querySelector('.latex-autocomplete')
    if (autocomplete && !autocomplete.contains(event.target as Node)) {
      hideAutocomplete()
    }
  }

  onMounted(() => {
    document.addEventListener('click', handleClickOutside)
  })

  onUnmounted(() => {
    document.removeEventListener('click', handleClickOutside)
    removeListeners()
  })

  return {
    // 状态
    isVisible,
    position,
    query,
    selectedIndex,
    filteredOptions,
    isEnabled,

    // 方法
    showAutocomplete,
    hideAutocomplete,
    selectOption,
    selectCurrent,
    navigateUp,
    navigateDown
  }
}
