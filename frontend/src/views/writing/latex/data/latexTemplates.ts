/**
 * LaTeX Template Data
 *
 * Extracted from LatexEditorView.vue to reduce its size.
 * Contains template definitions, categories, and shortcut categories.
 */

export interface Template {
  id: string
  name: string
  description: string
  category: string
  content: string
  icon: string
}

export interface Shortcut {
  action: string
  keys: string[]
}

export interface ShortcutCategory {
  name: string
  shortcuts: Shortcut[]
}

// ==========================================
// LaTeX Templates
// ==========================================

export const latexTemplates: Template[] = [
  {
    id: 'article',
    name: '学术论文',
    description: '标准学术论文模板',
    category: '学术论文',
    icon: '📄',
    content: `\\documentclass[12pt,a4paper]{article}

% 导言区
\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}
\\usepackage{amsmath,amsfonts,amssymb,amsthm}
\\usepackage{graphicx}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

% 标题信息
\\title{论文标题}
\\author{作者姓名}
\\date{\\today}

\\begin{document}

\\maketitle

\\begin{abstract}
这里是摘要内容。
\\end{abstract}

\\section{引言}
这里是引言内容...

\\section{方法}
这里是方法部分...

\\section{结果}
这里是结果部分...

\\section{结论}
这里是结论部分...

\\begin{thebibliography}{9}
  \\bibitem{文献1}
  \\bibitem{文献2}
\\end{thebibliography}

\\end{document}`
  },
  {
    id: 'report',
    name: '技术报告',
    description: '技术/工程报告模板',
    category: '学术报告',
    icon: '📋',
    content: `\\documentclass[12pt,a4paper]{report}

\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}
\\usepackage{amsmath,amsfonts,amssymb}
\\usepackage{graphicx}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

\\title{技术报告标题}
\\author{作者姓名}
\\date{\\today}

\\begin{document}

\\maketitle

\\tableofcontents

\\chapter{介绍}
这里是介绍内容...

\\chapter{背景}
这里是背景内容...

\\chapter{方法}
这里是方法部分...

\\chapter{结果}
这里是结果部分...

\\chapter{结论}
这里是结论内容...

\\end{document}`
  },
  {
    id: 'beamer',
    name: '演示文稿',
    description: 'Beamer演示文稿模板',
    category: '演示文稿',
    icon: '📊',
    content: `\\documentclass{beamer}

\\usetheme{Madrid}
\\usecolortheme{default}

\\title{演示文稿标题}
\\author{作者姓名}
\\date{\\today}

\\begin{document}

\\frame{\\titlepage}

\\begin{frame}
  \\frametitle{目录}
  \\tableofcontents
\\end{frame}

\\section{第一部分}

\\begin{frame}
  \\frametitle{第一张幻灯片}
  \\begin{itemize}
    \\item 要点1
    \\item 要点2
    \\item 要点3
  \\end{itemize}
\\end{frame}

\\end{document}`
  },
  {
    id: 'book',
    name: '书籍',
    description: '书籍/教材模板',
    category: '书籍',
    icon: '📚',
    content: `\\documentclass[12pt,a4paper]{book}

\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}
\\usepackage{amsmath,amsfonts,amssymb,amsthm}
\\usepackage{graphicx}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

\\title{书籍标题}
\\author{作者姓名}
\\date{\\today}

\\begin{document}

\\frontmatter
\\maketitle

\\tableofcontents

\\mainmatter
\\chapter{第一章}
这里是第一章内容...

\\chapter{第二章}
这里是第二章内容...

\\backmatter
\\begin{thebibliography}{9}
  \\bibitem{文献1}
  \\bibitem{文献2}
\\end{thebibliography}

\\end{document}`
  },
  {
    id: 'letter',
    name: '信函',
    description: '正式信函模板',
    category: '信函',
    icon: '✉️',
    content: `\\documentclass[12pt]{letter}

\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}

\\signature{发件人姓名}
\\address{发件人地址}
\\date{\\today}

\\begin{document}

\\begin{letter}{收件人姓名}
  这里是信件正文...

  \\vspace{1cm}
  此致

  敬礼
\\end{letter}

\\end{document}`
  },
  {
    id: 'memo',
    name: '备忘录',
    description: '内部备忘录模板',
    category: '办公',
    icon: '📝',
    content: `\\documentclass[12pt,a4paper]{article}

\\usepackage[utf8]{inputenc}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

\\title{备忘录}
\\author{部门名称}
\\date{\\today}

\\begin{document}

\\section*{主题}
这里是主题内容...

\\section*{内容}
这里是详细内容...

\\section*{行动项}
\\begin{itemize}
  \\item 行动项1
  \\item 行动项2
\\end{itemize}

\\end{document}`
  },
  {
    id: 'resume',
    name: '简历',
    description: '简历/CV模板',
    category: '个人',
    icon: '👤',
    content: `\\documentclass[12pt,a4paper]{article}

\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}
\\usepackage{enumitem}

\\begin{document}

\\begin{center}
  {\\LARGE \\textbf{姓名}} \\\\
  \\vspace{0.3cm}
  联系邮箱 | 电话号码 | 网站
\\end{center}

\\vspace{1cm}

\\section*{教育背景}
\\begin{itemize}
  \\item 学位 - 学校名称 (年份)
  \\item 学位 - 学校名称 (年份)
\\end{itemize}

\\section*{工作经历}
\\begin{itemize}
  \\item 职位 - 公司名称 (年份 - 至今)
  \\item 职位 - 公司名称 (年份 - 年份)
\\end{itemize}

\\section*{技能}
\\begin{itemize}
  \\item 技能1
  \\item 技能2
  \\item 技能3
\\end{itemize}

\\end{document}`
  },
  {
    id: 'notes',
    name: '课程笔记',
    description: '课程笔记模板',
    category: '教育',
    icon: '📖',
    content: `\\documentclass[12pt,a4paper]{article}

\\usepackage[utf8]{inputenc}
\\usepackage[T1]{fontenc}
\\usepackage{amsmath,amsfonts,amssymb}
\\usepackage{graphicx}
\\usepackage{geometry}
\\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

\\title{课程名称}
\\author{学生姓名}
\\date{学期}

\\begin{document}

\\maketitle

\\tableofcontents

\\section{第一讲：讲义标题}
这里是课程内容...

\\subsection{要点1}
详细说明...

\\subsection{要点2}
详细说明...

\\section{第二讲：讲义标题}
这里是课程内容...

\\end{document}`
  }
]

// ==========================================
// Shortcut Categories
// ==========================================

export const shortcutCategories: ShortcutCategory[] = [
  {
    name: '文件操作',
    shortcuts: [
      { action: '保存文档', keys: ['Ctrl', 'S'] },
      { action: '编译文档', keys: ['Ctrl', 'Enter'] },
    ]
  },
  {
    name: '编辑操作',
    shortcuts: [
      { action: '撤销', keys: ['Ctrl', 'Z'] },
      { action: '重做', keys: ['Ctrl', 'Shift', 'Z'] },
      { action: '查找', keys: ['Ctrl', 'F'] },
      { action: '查找下一个', keys: ['F3'] },
      { action: '查找上一个', keys: ['Shift', 'F3'] },
      { action: '替换', keys: ['Ctrl', 'H'] },
      { action: '跳转到行', keys: ['Ctrl', 'G'] },
    ]
  },
  {
    name: '格式化',
    shortcuts: [
      { action: '粗体', keys: ['Ctrl', 'B'] },
      { action: '斜体', keys: ['Ctrl', 'I'] },
      { action: '下划线', keys: ['Ctrl', 'U'] },
    ]
  },
  {
    name: '视图控制',
    shortcuts: [
      { action: '切换预览', keys: ['Ctrl', '\\'] },
      { action: '切换大纲', keys: ['Ctrl', 'O'] },
      { action: '放大预览', keys: ['Ctrl', '+'] },
      { action: '缩小预览', keys: ['Ctrl', '-'] },
    ]
  },
  {
    name: '面板',
    shortcuts: [
      { action: '快捷键帮助', keys: ['?'] },
      { action: '符号面板', keys: ['Ctrl', 'Shift', 'S'] },
      { action: '代码片段', keys: ['Ctrl', 'Space'] },
    ]
  }
]
