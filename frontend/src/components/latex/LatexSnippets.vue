<template>
  <div class="latex-snippets" :class="{ 'is-floating': floating }">
    <div class="snippets-header">
      <h4 class="snippets-title">代码片段</h4>
      <el-input
        v-model="searchQuery"
        placeholder="搜索片段..."
        size="small"
        clearable
        class="snippets-search"
      >
        <template #prefix>
          <el-icon><Search /></el-icon>
        </template>
      </el-input>
    </div>

    <div class="snippets-categories">
      <div
        v-for="category in filteredCategories"
        :key="category.id"
        class="category-item"
        :class="{ active: selectedCategory === category.id }"
        @click="selectedCategory = category.id"
      >
        <span class="category-icon">{{ category.icon }}</span>
        <span class="category-name">{{ category.name }}</span>
      </div>
    </div>

    <div class="snippets-content">
      <div
        v-for="snippet in filteredSnippets"
        :key="snippet.id"
        class="snippet-item"
        @click="insertSnippet(snippet)"
      >
        <div class="snippet-header">
          <span class="snippet-name">{{ snippet.name }}</span>
          <el-button size="small" text @click.stop="insertSnippet(snippet)">
            <el-icon><Plus /></el-icon>
          </el-button>
        </div>
        <div class="snippet-description">{{ snippet.description }}</div>
        <div class="snippet-preview">
          <pre><code>{{ snippet.preview }}</code></pre>
        </div>
      </div>

      <div v-if="filteredSnippets.length === 0" class="snippets-empty">
        <el-empty description="没有找到匹配的代码片段" :image-size="60" />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Search, Plus } from '@element-plus/icons-vue'

interface Props {
  floating?: boolean
}

interface Snippet {
  id: string
  name: string
  description: string
  category: string
  content: string
  preview: string
}

const props = withDefaults(defineProps<Props>(), {
  floating: false
})

const emit = defineEmits<{
  insert: [snippet: Snippet]
}>()

const searchQuery = ref('')
const selectedCategory = ref('all')

interface Category {
  id: string
  name: string
  icon: string
}

const categories: Category[] = [
  { id: 'all', name: '全部', icon: '📦' },
  { id: 'document', name: '文档结构', icon: '📄' },
  { id: 'formatting', name: '格式化', icon: '✨' },
  { id: 'math', name: '数学公式', icon: '∑' },
  { id: 'lists', name: '列表', icon: '📋' },
  { id: 'tables', name: '表格', icon: '📊' },
  { id: 'figures', name: '图片', icon: '🖼️' },
  { id: 'code', name: '代码', icon: '💻' },
  { id: 'references', name: '引用', icon: '📚' }
]

const snippets: Snippet[] = [
  // ==========================================
  // 文档结构
  // ==========================================
  {
    id: 'doc-basic',
    name: '基础文档',
    description: '创建一个基础LaTeX文档结构',
    category: 'document',
    content: `\\documentclass[12pt,a4paper]{article}
\\usepackage[utf8]{inputenc}
\\usepackage{amsmath,amsfonts,amssymb}

\\title{文档标题}
\\author{作者姓名}
\\date{\\today}

\\begin{document}

\\maketitle

\\section{引言}
在这里开始编写...

\\end{document}`,
    preview: '\\documentclass...\n\\begin{document}...\n\\end{document}'
  },
  {
    id: 'doc-title',
    name: '标题页',
    description: '添加标题、作者和日期',
    category: 'document',
    content: `\\title{文档标题}
\\author{作者姓名 \\\\ \\texttt{author@example.com}}
\\date{\\today}

\\maketitle`,
    preview: '\\title{...}\n\\author{...}\n\\maketitle'
  },
  {
    id: 'doc-abstract',
    name: '摘要',
    description: '添加摘要部分',
    category: 'document',
    content: `\\begin{abstract}
  这里是摘要内容。简要描述研究背景、方法、结果和结论。
\\end{abstract}`,
    preview: '\\begin{abstract}\n  ...\n\\end{abstract}'
  },
  {
    id: 'doc-sections',
    name: '章节结构',
    description: '完整的章节层次结构',
    category: 'document',
    content: `\\section{一级标题}
  这里是一级节的内容。

  \\subsection{二级标题}
  这里是二级节的内容。

  \\subsubsection{三级标题}
  这里是三级节的内容。

  \\paragraph{段落标题}
  这里是段落的内容。`,
    preview: '\\section{...}\n\\subsection{...}\n\\subsubsection{...}'
  },

  // ==========================================
  // 格式化
  // ==========================================
  {
    id: 'fmt-text',
    name: '文本格式',
    description: '粗体、斜体、下划线等',
    category: 'formatting',
    content: `\\textbf{粗体文本}
\\textit{斜体文本}
\\underline{下划线文本}
\\emph{强调文本}
\\texttt{等宽字体}
\\textsc{小型大写字母}
\\textsuperscript{上标}
\\textsubscript{下标}`,
    preview: '\\textbf{...} \\textit{...} \\underline{...}'
  },
  {
    id: 'fmt-colors',
    name: '颜色',
    description: '使用不同颜色的文本',
    category: 'formatting',
    content: `% 需要加载 \\usepackage{xcolor}
\\textcolor{red}{红色文本}
\\textcolor{blue}{蓝色文本}
\\textcolor{green!50!black}{深绿色文本}

% 也可以用RGB值
\\textcolor[RGB]{255,0,0}{红色文本}

% 背景色
\\colorbox{yellow}{黄色背景}`,
    preview: '\\textcolor{red}{...} \\colorbox{yellow}{...}'
  },
  {
    id: 'fmt-alignment',
    name: '对齐方式',
    description: '文本对齐环境',
    category: 'formatting',
    content: `\\begin{center}
  居中文本
\\end{center}

\\begin{flushleft}
  左对齐文本
\\end{flushleft}

\\begin{flushright}
  右对齐文本
\\end{flushright}`,
    preview: '\\begin{center}...\n\\begin{flushleft}...'
  },

  // ==========================================
  // 数学公式
  // ==========================================
  {
    id: 'math-equation',
    name: '单行公式',
    description: '带编号的单行公式',
    category: 'math',
    content: `\\begin{equation}
  E = mc^2
  \\label{eq:einstein}
\\end{equation}

引用公式: \\eqref{eq:einstein}`,
    preview: '\\begin{equation}\n  E = mc^2\n\\end{equation}'
  },
  {
    id: 'math-align',
    name: '多行公式',
    description: '对齐的多行公式',
    category: 'math',
    content: `\\begin{align}
  f(x) &= x^2 + 2x + 1 \\\\
       &= (x + 1)^2
\\end{align}`,
    preview: '\\begin{align}\n  f(x) &= ...\n\\end{align}'
  },
  {
    id: 'math-common',
    name: '常用数学符号',
    description: '常用数学运算和符号',
    category: 'math',
    content: `% 分数
\\frac{a}{b}

% 根号
\\sqrt{x}
\\sqrt[n]{x}

% 求和、积分、极限
\\sum_{i=1}^{n} x_i
\\int_{a}^{b} f(x) dx
\\lim_{x \\to \\infty}

% 希腊字母
\\alpha, \\beta, \\gamma, \\delta, \\theta, \\lambda, \\mu, \\sigma, \\phi, \\omega
\\Delta, \\Theta, \\Lambda, \\Sigma, \\Phi, \\Omega

% 关系符号
\\approx, \\equiv, \\leq, \\geq, \\neq, \\in, \\subset`,
    preview: '\\frac{a}{b}\n\\sqrt{x}\n\\sum_{i=1}^{n}'
  },
  {
    id: 'math-matrix',
    name: '矩阵',
    description: '各种矩阵环境',
    category: 'math',
    content: `% 需要加载 \\usepackage{amsmath}

% 无括号矩阵
\\begin{matrix}
  a & b \\\\
  c & d
\\end{matrix}

% 圆括号矩阵
\\begin{pmatrix}
  a & b \\\\
  c & d
\\end{pmatrix}

% 方括号矩阵
\\begin{bmatrix}
  a & b \\\\
  c & d
\\end{bmatrix}

% 带省略号
\\begin{pmatrix}
  a_{11} & \\cdots & a_{1n} \\\\
  \\vdots & \\ddots & \\vdots \\\\
  a_{m1} & \\cdots & a_{mn}
\\end{pmatrix}`,
    preview: '\\begin{pmatrix}\n  a & b \\\\\n  c & d\n\\end{pmatrix}'
  },

  // ==========================================
  // 列表
  // ==========================================
  {
    id: 'list-itemize',
    name: '无序列表',
    description: '项目符号列表',
    category: 'lists',
    content: `\\begin{itemize}
  \\item 第一项
  \\item 第二项
  \\item 第三项
\\end{itemize}`,
    preview: '\\begin{itemize}\n  \\item ...\n\\end{itemize}'
  },
  {
    id: 'list-enumerate',
    name: '有序列表',
    description: '编号列表',
    category: 'lists',
    content: `\\begin{enumerate}
  \\item 第一项
  \\item 第二项
  \\item 第三项
\\end{enumerate}`,
    preview: '\\begin{enumerate}\n  \\item ...\n\\end{enumerate}'
  },
  {
    id: 'list-nested',
    name: '嵌套列表',
    description: '多级嵌套列表',
    category: 'lists',
    content: `\\begin{enumerate}
  \\item 第一项
  \\begin{itemize}
    \\item 子项1
    \\item 子项2
  \\end{itemize}
  \\item 第二项
\\end{enumerate}`,
    preview: '列表嵌套示例'
  },
  {
    id: 'list-description',
    name: '描述列表',
    description: '术语定义列表',
    category: 'lists',
    content: `\\begin{description}
  \\item[术语1] 定义1
  \\item[术语2] 定义2
  \\item[术语3] 定义3
\\end{description}`,
    preview: '\\begin{description}\n  \\item[...]\n\\end{description}'
  },

  // ==========================================
  // 表格
  // ==========================================
  {
    id: 'table-basic',
    name: '基础表格',
    description: '简单的表格',
    category: 'tables',
    content: `\\begin{table}[h]
  \\centering
  \\begin{tabular}{|c|c|c|}
    \\hline
    列1 & 列2 & 列3 \\\\
    \\hline
    数据1 & 数据2 & 数据3 \\\\
    \\hline
  \\end{tabular}
  \\caption{表格标题}
\\end{table}`,
    preview: '\\begin{tabular}{|c|c|c|}\n  ...\n\\end{tabular}'
  },
  {
    id: 'table-booktabs',
    name: '专业表格',
    description: '使用booktabs的专业表格',
    category: 'tables',
    content: `% 需要加载 \\usepackage{booktabs}
\\begin{table}[h]
  \\centering
  \\begin{tabular}{ccc}
    \\toprule
    列1 & 列2 & 列3 \\\\
    \\midrule
    数据1 & 数据2 & 数据3 \\\\
    数据4 & 数据5 & 数据6 \\\\
    \\bottomrule
  \\end{tabular}
  \\caption{专业表格标题}
\\end{table}`,
    preview: '\\begin{tabular}{ccc}\n  \\toprule\n  ...\n  \\bottomrule\n\\end{tabular}'
  },
  {
    id: 'table-multipage',
    name: '长表格',
    description: '跨页表格',
    category: 'tables',
    content: `% 需要加载 \\usepackage{longtable}
\\begin{longtable}{|c|c|c|}
  \\hline
  列1 & 列2 & 列3 \\\\
  \\hline
  \\endfirsthead

  \\hline
  列1 & 列2 & 列3 \\\\
  \\hline
  \\endhead

  \\hline
  \\multicolumn{3}{|r|}{{续表}} \\\\
  \\hline
  \\endfoot

  \\hline
  \\endlastfoot

  数据1 & 数据2 & 数据3 \\\\
  \\hline
  % 更多数据...
\\end{longtable}`,
    preview: '\\begin{longtable}{|c|c|c|}\n  ...\n\\end{longtable}'
  },

  // ==========================================
  // 图片
  // ==========================================
  {
    id: 'figure-basic',
    name: '插入图片',
    description: '插入单张图片',
    category: 'figures',
    content: `% 需要加载 \\usepackage{graphicx}
\\begin{figure}[h]
  \\centering
  % 支持的格式: pdf, png, jpg, eps
  \\includegraphics[width=0.8\\textwidth]{filename.png}
  \\caption{图片标题}
  \\label{fig:label}
\\end{figure}

引用图片: 见图\\ref{fig:label}`,
    preview: '\\includegraphics[width=0.8\\textwidth]{...}'
  },
  {
    id: 'figure-side',
    name: '并排图片',
    description: '两张图片并排显示',
    category: 'figures',
    content: `% 需要加载 \\usepackage{subfig} 或 \\usepackage{subcaption}
\\begin{figure}[h]
  \\centering
  \\begin{minipage}{0.45\\textwidth}
    \\centering
    \\includegraphics[width=\\textwidth]{fig1.png}
    \\caption{图片1}
  \\end{minipage}
  \\hfill
  \\begin{minipage}{0.45\\textwidth}
    \\centering
    \\includegraphics[width=\\textwidth]{fig2.png}
    \\caption{图片2}
  \\end{minipage}
  \\caption{并排图片}
\\end{figure}`,
    preview: '两张图片并排'
  },
  {
    id: 'figure-wrap',
    name: '图文混排',
    description: '文本环绕图片',
    category: 'figures',
    content: `% 需要加载 \\usepackage{wrapfig}
\\begin{wrapfigure}{r}{0.4\\textwidth}
  \\centering
  \\includegraphics[width=0.35\\textwidth]{filename.png}
  \\caption{图片标题}
\\end{wrapfigure}

这里是一些文本内容，会环绕在图片周围。图片位于右侧。`,
    preview: '\\begin{wrapfigure}{r}{0.4\\textwidth}\n  ...\n\\end{wrapfigure}'
  },

  // ==========================================
  // 代码
  // ==========================================
  {
    id: 'code-verbatim',
    name: '代码块',
    description: '原始代码块',
    category: 'code',
    content: `\\begin{verbatim}
#include <iostream>
int main() {
    std::cout << "Hello, World!";
    return 0;
}
\\end{verbatim}`,
    preview: '\\begin{verbatim}\n...\n\\end{verbatim}'
  },
  {
    id: 'code-listings',
    name: '高亮代码',
    description: '带语法高亮的代码块',
    category: 'code',
    content: `% 需要加载 \\usepackage{listings}
\\begin{lstlisting}[language=Python, caption=Python代码示例]
def hello_world():
    print("Hello, World!")
    return True
\\end{lstlisting}`,
    preview: '\\begin{lstlisting}[language=Python]\n...\n\\end{lstlisting}'
  },
  {
    id: 'code-inline',
    name: '行内代码',
    description: '行内代码和命令',
    category: 'code',
    content: `使用 \\texttt{\\textbackslash textbf\{\}} 命令可以加粗文本。

也可以用 \\verb|ls -la| 显示命令。`,
    preview: '\\texttt{code} 和 \\verb|command|'
  },

  // ==========================================
  // 引用
  // ==========================================
  {
    id: 'ref-cite',
    name: '文献引用',
    description: '引用文献',
    category: 'references',
    content: `% 在正文中引用
这里是正文内容 \\cite{ref1}。

也可以引用多个文献 \\cite{ref1,ref2,ref3}。

或者指定页码 \\cite[第10页]{ref1}。

% 在文档末尾添加参考文献
\\begin{thebibliography}{9}
  \\bibitem{ref1}
  作者. 文章标题. 期刊名, 年份.
  \\bibitem{ref2}
  作者. 书名. 出版社, 年份.
\\end{thebibliography}`,
    preview: '\\cite{ref1} 和 \\begin{thebibliography}\n...\n\\end{thebibliography}'
  },
  {
    id: 'ref-bibtex',
    name: 'BibTeX引用',
    description: '使用BibTeX管理文献',
    category: 'references',
    content: `% 在文档中引用
\\cite{einstein1905}

% 在文档末尾设置参考文献样式和数据库
\\bibliographystyle{plain}
\\bibliography{references}

% 创建 references.bib 文件，内容示例:
% @article{einstein1905,
%   title={关于光的产生和转化的一个试探性观点},
%   author={Einstein, Albert},
%   journal={Annalen der Physik},
%   volume={322},
%   number={6},
%   pages={132--148},
%   year={1905}
% }`,
    preview: '\\bibliographystyle{plain}\n\\bibliography{references}'
  },
  {
    id: 'ref-cross',
    name: '交叉引用',
    description: '引用图表公式',
    category: 'references',
    content: `% 标签和引用
见图 \\ref{fig:example} 和表 \\ref{tab:example}。

公式 \\eqref{eq:example} 显示了...

% 为元素添加标签
\\begin{figure}
  ...
  \\caption{示例图片}
  \\label{fig:example}
\\end{figure}

\\begin{table}
  ...
  \\caption{示例表格}
  \\label{tab:example}
\\end{table}

\\begin{equation}
  E = mc^2
  \\label{eq:example}
\\end{equation}`,
    preview: '\\label{...} 和 \\ref{...}, \\eqref{...}'
  },
  {
    id: 'ref-footnote',
    name: '脚注',
    description: '添加脚注',
    category: 'references',
    content: `这是一段带脚注的文本\\footnote{这是脚注内容}。

也可以在段落后添加脚注\\footnote{另一个脚注}。

脚注会自动编号并显示在页面底部。`,
    preview: '文本\\footnote{脚注内容}'
  }
]

// 计算属性
const filteredCategories = computed(() => {
  if (!searchQuery.value) {
    return categories
  }
  return categories.filter(cat =>
    cat.name.toLowerCase().includes(searchQuery.value.toLowerCase()) ||
    snippets.some(s => s.category === cat.id && s.name.toLowerCase().includes(searchQuery.value.toLowerCase()))
  )
})

const filteredSnippets = computed(() => {
  let filtered = snippets

  // 按分类过滤
  if (selectedCategory.value !== 'all') {
    filtered = filtered.filter(s => s.category === selectedCategory.value)
  }

  // 按搜索关键词过滤
  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    filtered = filtered.filter(s =>
      s.name.toLowerCase().includes(query) ||
      s.description.toLowerCase().includes(query) ||
      s.content.toLowerCase().includes(query)
    )
  }

  return filtered
})

// 方法
function insertSnippet(snippet: Snippet) {
  emit('insert', snippet)
}
</script>

<style scoped lang="scss">
.latex-snippets {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: var(--el-bg-color);
  border-radius: 4px;

  &.is-floating {
    position: absolute;
    right: 16px;
    top: 60px;
    width: 350px;
    max-height: calc(100vh - 100px);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
    border: 1px solid var(--el-border-color);
    z-index: 1000;
  }
}

.snippets-header {
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);

  .snippets-title {
    margin: 0 0 8px 0;
    font-size: 14px;
    font-weight: 600;
    color: var(--el-text-color-primary);
  }

  .snippets-search {
    :deep(.el-input__wrapper) {
      border-radius: 4px;
    }
  }
}

.snippets-categories {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
  padding: 8px 12px;
  border-bottom: 1px solid var(--el-border-color-lighter);
  background: var(--el-fill-color-blank);

  .category-item {
    display: flex;
    align-items: center;
    gap: 4px;
    padding: 4px 8px;
    font-size: 12px;
    border-radius: 4px;
    cursor: pointer;
    transition: all 0.2s;
    user-select: none;

    &:hover {
      background: var(--el-fill-color-light);
    }

    &.active {
      background: var(--el-color-primary-light-9);
      color: var(--el-color-primary);
      font-weight: 600;
    }

    .category-icon {
      font-size: 14px;
    }
  }
}

.snippets-content {
  flex: 1;
  overflow-y: auto;
  padding: 8px;

  .snippet-item {
    padding: 12px;
    margin-bottom: 8px;
    background: var(--el-fill-color-blank);
    border: 1px solid var(--el-border-color-lighter);
    border-radius: 4px;
    cursor: pointer;
    transition: all 0.2s;

    &:hover {
      border-color: var(--el-color-primary-light-7);
      box-shadow: 0 2px 8px rgba(var(--el-color-primary-rgb), 0.1);
    }

    .snippet-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 4px;

      .snippet-name {
        font-weight: 600;
        color: var(--el-text-color-primary);
        font-size: 13px;
      }
    }

    .snippet-description {
      font-size: 12px;
      color: var(--el-text-color-secondary);
      margin-bottom: 8px;
      line-height: 1.4;
    }

    .snippet-preview {
      padding: 8px;
      background: var(--el-fill-color);
      border-radius: 4px;
      overflow: hidden;

      pre {
        margin: 0;
        font-family: 'Courier New', monospace;
        font-size: 11px;
        color: var(--el-text-color-regular);
        white-space: pre-wrap;
        word-break: break-all;
      }

      code {
        font-family: inherit;
      }
    }
  }

  .snippets-empty {
    display: flex;
    align-items: center;
    justify-content: center;
    height: 200px;
  }
}

// 深色模式适配
.dark {
  .snippet-item {
    &:hover {
      border-color: var(--el-color-primary-light-5);
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
    }
  }
}
</style>
