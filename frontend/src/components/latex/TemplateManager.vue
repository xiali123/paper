<template>
  <div class="template-manager">
    <div class="manager-header">
      <h4>文档模板</h4>
      <el-input
        v-model="searchQuery"
        placeholder="搜索模板..."
        size="small"
        clearable
        prefix-icon="Search"
        class="template-search"
      />
    </div>

    <div class="template-categories">
      <div
        v-for="category in categories"
        :key="category.id"
        class="category-item"
        :class="{ active: selectedCategory === category.id }"
        @click="selectedCategory = category.id"
      >
        <span class="category-icon">{{ category.icon }}</span>
        <span class="category-name">{{ category.name }}</span>
      </div>
    </div>

    <div class="templates-list">
      <div
        v-for="template in filteredTemplates"
        :key="template.id"
        class="template-card"
        @click="selectTemplate(template)"
      >
        <div class="template-preview">
          <div class="preview-icon">{{ template.icon }}</div>
          <div class="preview-info">
            <div class="template-name">{{ template.name }}</div>
            <div class="template-desc">{{ template.description }}</div>
          </div>
        </div>
        <div class="template-meta">
          <el-tag size="small" type="info">{{ template.category }}</el-tag>
          <span class="template-size">{{ template.size }}</span>
        </div>
      </div>

      <div v-if="filteredTemplates.length === 0" class="templates-empty">
        <el-empty description="没有找到匹配的模板" :image-size="60" />
      </div>
    </div>

    <!-- Template Detail Dialog -->
    <el-dialog
      v-model="showDetail"
      :title="selectedTemplate?.name"
      width="700px"
    >
      <div class="template-detail" v-if="selectedTemplate">
        <div class="detail-header">
          <div class="detail-icon">{{ selectedTemplate.icon }}</div>
          <div class="detail-info">
            <h3>{{ selectedTemplate.name }}</h3>
            <p>{{ selectedTemplate.description }}</p>
            <div class="detail-tags">
              <el-tag size="small">{{ selectedTemplate.category }}</el-tag>
              <el-tag size="small" type="info">{{ selectedTemplate.size }}</el-tag>
              <el-tag size="small" type="success">{{ selectedTemplate.difficulty }}</el-tag>
            </div>
          </div>
        </div>

        <div class="detail-content">
          <div class="content-section">
            <h4>模板预览</h4>
            <pre class="code-preview"><code>{{ selectedTemplate.preview }}</code></pre>
          </div>

          <div class="content-section" v-if="selectedTemplate.variables.length > 0">
            <h4>模板变量</h4>
            <div class="variables-list">
              <div
                v-for="variable in selectedTemplate.variables"
                :key="variable.name"
                class="variable-item"
              >
                <span class="variable-name">{{ variable.name }}</span>
                <el-input
                  v-model="variableValues[variable.name]"
                  :placeholder="variable.default"
                  size="small"
                />
              </div>
            </div>
          </div>

          <div class="content-section" v-if="selectedTemplate.requirements.length > 0">
            <h4>需要的包</h4>
            <div class="requirements-list">
              <el-tag
                v-for="req in selectedTemplate.requirements"
                :key="req"
                size="small"
                type="warning"
              >
                {{ req }}
              </el-tag>
            </div>
          </div>
        </div>
      </div>

      <template #footer>
        <el-space>
          <el-button @click="showDetail = false">取消</el-button>
          <el-button @click="copyTemplate">复制代码</el-button>
          <el-button type="primary" @click="useTemplate">使用模板</el-button>
        </el-space>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { ElMessage } from 'element-plus'

interface TemplateVariable {
  name: string
  default: string
  description: string
}

interface Template {
  id: string
  name: string
  description: string
  category: string
  icon: string
  size: string
  difficulty: string
  content: string
  preview: string
  variables: TemplateVariable[]
  requirements: string[]
}

interface Emits {
  insert: [content: string]
}

const emit = defineEmits<Emits>()

const searchQuery = ref('')
const selectedCategory = ref('all')
const showDetail = ref(false)
const selectedTemplate = ref<Template | null>(null)
const variableValues = ref<Record<string, string>>({})

const categories = [
  { id: 'all', name: '全部', icon: '📦' },
  { id: 'academic', name: '学术论文', icon: '🎓' },
  { id: 'report', name: '报告', icon: '📊' },
  { id: 'presentation', name: '演示', icon: '📽️' },
  { id: 'letter', name: '信函', icon: '✉️' },
  { id: 'book', name: '书籍', icon: '📚' },
  { id: 'thesis', name: '学位论文', icon: '📜' },
  { id: 'cv', name: '简历', icon: '📄' }
]

const templates: Template[] = [
  // ==========================================
  // 学术论文模板
  // ==========================================
  {
    id: 'academic-article',
    name: '学术论文',
    description: '标准学术论文模板，适合期刊投稿',
    category: 'academic',
    icon: '📄',
    size: '中等',
    difficulty: '简单',
    variables: [
      { name: 'title', default: '论文标题', description: '论文标题' },
      { name: 'author', default: '作者姓名', description: '作者姓名' },
      { name: 'institute', default: '所属机构', description: '所属机构' },
      { name: 'date', default: '\\today', description: '日期' }
    ],
    requirements: ['amsmath', 'graphicx', 'cite'],
    content: `\\documentclass[12pt,a4paper]{article}

% 包引入
\\usepackage[utf8]{inputenc}
\\usepackage{amsmath,amsfonts,amssymb}
\\usepackage{graphicx}
\\usepackage{cite}
\\usepackage{hyperref}

% 论文元数据
\\title{{{title}}}
\\author{{{author}\\\\\\small{{{institute}}}}}
\\date{{{date}}}

\\begin{document}

\\maketitle

\\begin{abstract}
这里是摘要内容。简要描述研究背景、方法、结果和结论。通常包括：
\\begin{itemize}
    \\item 研究背景和动机
    \\item 研究方法和创新点
    \\item 主要结果和贡献
    \\item 结论和意义
\\end{itemize}
\\end{abstract}

\\section{引言}
介绍研究背景、相关工作、研究问题和本文贡献。

\\section{相关工作}
综述领域内相关研究，指出本文的创新点。

\\section{方法}
详细描述研究方法、算法设计、实验设置等。

\\section{实验结果}
展示实验结果，包括图表、数据分析等。

\\section{讨论}
分析结果，讨论局限性和未来工作。

\\section{结论}
总结全文，强调主要贡献。

\\bibliographystyle{plain}
\\bibliography{references}

\\end{document}`,
    preview: `\\documentclass[12pt,a4paper]{article}
\\usepackage{amsmath,graphicx}
\\title{论文标题}
\\author{作者}
\\begin{document}
\\maketitle
\\begin{abstract}
摘要内容...
\\end{abstract}
\\section{引言}
...
\\end{document}`
  },
  {
    id: 'ieee-conference',
    name: 'IEEE会议论文',
    description: 'IEEE会议论文格式模板',
    category: 'academic',
    icon: '⚡',
    size: '标准',
    difficulty: '中等',
    variables: [
      { name: 'title', default: 'Paper Title', description: '论文标题' },
      { name: 'author', default: 'Author Name', description: '作者姓名' }
    ],
    requirements: ['IEEEtran', 'cite'],
    content: `\\documentclass[conference]{IEEEtran}

\\usepackage{cite}
\\usepackage{amsmath,amssymb,amsfonts}
\\usepackage{graphicx}
\\usepackage{textcomp}

\\title{{{title}}}

\\author{\\IEEEauthorblockN{{{author}}}
\\IEEEauthorblockA{\\textit{Department Name}\\\\
\\textit{Institution Name}\\\\
City, Country\\\\
email@example.com}}

\\begin{document}

\\maketitle

\\begin{abstract}
This document is a model and instructions for LaTeX.
This and the IEEEtran.cls file define the structure.
This should be in 9 point type.
\\end{abstract}

\\section{Introduction}
This template is for conference papers.

\\section{Related Work}
Discuss related work here.

\\section{Methodology}
Describe your methods.

\\section{Results}
Present your results.

\\section{Conclusion}
Conclude the paper.

\\begin{thebibliography}{99}
\\bibitem{b1} Author, ``Title,'' Journal, vol.~x, no.~x, pp.~xxx--xxx, Month~Year.
\\end{thebibliography}

\\end{document}`,
    preview: `\\documentclass[conference]{IEEEtran}
\\title{Paper Title}
\\author{Author Name}
\\begin{document}
\\maketitle
\\begin{abstract}
...
\\end{abstract}
\\section{Introduction}
...
\\end{document}`
  },

  // ==========================================
  // 报告模板
  // ==========================================
  {
    id: 'lab-report',
    name: '实验报告',
    description: '标准实验报告模板',
    category: 'report',
    icon: '🔬',
    size: '中等',
    difficulty: '简单',
    variables: [
      { name: 'course', default: '课程名称', description: '课程名称' },
      { name: 'title', default: '实验名称', description: '实验名称' },
      { name: 'student', default: '学生姓名', description: '学生姓名' },
      { name: 'id', default: '学号', description: '学号' }
    ],
    requirements: ['amsmath', 'graphicx', 'booktabs'],
    content: `\\documentclass[12pt,a4paper]{article}

\\usepackage[utf8]{inputenc}
\\usepackage{amsmath,amsfonts,amssymb}
\\usepackage{graphicx}
\\usepackage{booktabs}

\\title{{{course}\\\\{{{title}}}}}
\\author{{{student} (学号:{{{id}})}}}
\\date{\\today}

\\begin{document}

\\maketitle

\\section{实验目的}
描述实验的目的和要实现的目标。

\\section{实验原理}
\\subsection{理论基础}
介绍相关的理论知识。

\\subsection{实验方法}
描述实验的具体方法。

\\section{实验步骤}
\\begin{enumerate}
    \\item 第一步
    \\item 第二步
    \\item 第三步
\\end{enumerate}

\\section{实验结果}
\\subsection{数据记录}
\\begin{table}[h]
\\centering
\\begin{tabular}{ccc}
\\toprule
参数 & 测量值 & 单位 \\\\
\\midrule
参数1 & 0.00 & 0 \\\\
参数2 & 0.00 & 0 \\\\
\\bottomrule
\\end{tabular}
\\caption{实验数据}
\\end{table}

\\subsection{结果分析}
对实验结果进行分析。

\\section{实验结论}
总结实验结果，得出结论。

\\section{思考题}
\\begin{enumerate}
    \\item 问题1
    \\item 问题2
\\end{enumerate}

\\end{document}`,
    preview: `\\documentclass[12pt,a4paper]{article}
\\title{课程名称\\\\实验名称}
\\author{学生姓名}
\\begin{document}
\\section{实验目的}
...
\\section{实验结果}
...
\\section{实验结论}
...
\\end{document}`
  },

  // ==========================================
  // 演示模板
  // ==========================================
  {
    id: 'beamer-basic',
    name: 'Beamer演示',
    description: '基础演示文稿模板',
    category: 'presentation',
    icon: '📽️',
    size: '简单',
    difficulty: '简单',
    variables: [
      { name: 'title', default: '演示标题', description: '演示标题' },
      { name: 'author', default: '演讲者', description: '演讲者' },
      { name: 'institute', default: '所属机构', description: '所属机构' },
      { name: 'date', default: '\\today', description: '日期' }
    ],
    requirements: ['beamer'],
    content: `\\documentclass{beamer}

\\usetheme{Madrid}
\\usecolortheme{default}

\\title{{{title}}}
\\author{{{author}}}
\\institute{{{institute}}}
\\date{{{date}}}

\\begin{document}

\\frame{\\titlepage}

\\begin{frame}
\\frametitle{目录}
\\tableofcontents
\\end{frame}

\\section{引言}

\\begin{frame}
\\frametitle{引言}
\\begin{itemize}
    \\item 要点1
    \\item 要点2
    \\item 要点3
\\end{itemize}
\\end{frame}

\\section{主要内容}

\\begin{frame}
\\frametitle{主要内容}
\\begin{block}{定义}
这里是定义或概念。
\\end{block}

\\begin{alertblock}{重要提示}
这里是重要提示。
\\end{alertblock}

\\begin{exampleblock}{示例}
这里是示例。
\\end{exampleblock}
\\end{frame}

\\begin{frame}
\\frametitle{公式展示}
\\begin{equation}
E = mc^2
\\end{equation}
\\end{frame}

\\section{结论}

\\begin{frame}
\\frametitle{结论}
\\begin{itemize}
    \\item 总结1
    \\item 总结2
    \\item 未来工作
\\end{itemize}
\\end{frame}

\\begin{frame}
\\frametitle{谢谢}
\\centering
\\Huge 谢谢！
\\end{frame}

\\end{document}`,
    preview: `\\documentclass{beamer}
\\usetheme{Madrid}
\\title{演示标题}
\\author{演讲者}
\\begin{document}
\\frame{\\titlepage}
\\begin{frame}
\\frametitle{标题}
内容...
\\end{frame}
\\end{document}`
  },

  // ==========================================
  // 学位论文模板
  // ==========================================
  {
    id: 'thesis-template',
    name: '学位论文',
    description: '本科/研究生毕业论文模板',
    category: 'thesis',
    icon: '📜',
    size: '大型',
    difficulty: '复杂',
    variables: [
      { name: 'title', default: '论文题目', description: '论文题目' },
      { name: 'author', default: '作者姓名', description: '作者姓名' },
      { name: 'advisor', default: '指导教师', description: '指导教师' },
      { name: 'degree', default: '学士', description: '学位类型' },
      { name: 'major', default: '专业名称', description: '专业名称' },
      { name: 'institute', default: '学校名称', description: '学校名称' }
    ],
    requirements: ['amsmath', 'graphicx', 'cite', 'algorithm', 'algorithmic'],
    content: `\\documentclass[12pt,a4paper]{report}

\\usepackage[utf8]{inputenc}
\\usepackage{amsmath,amsfonts,amssymb}
\\usepackage{graphicx}
\\usepackage{cite}
\\usepackage{algorithm}
\\usepackage{algorithmic}
\\usepackage{hyperref}

\\title{{{title}}}
\\author{{{author}}}
\\date{\\today}

\\begin{document}

\\input{chapters/cover.tex}
\\input{chapters/abstract.tex}

\\tableofcontents
\\listoffigures
\\listoftables

\\chapter{绪论}
\\section{研究背景}
\\section{研究意义}
\\section{国内外研究现状}
\\section{本文主要工作}
\\section{论文结构安排}

\\chapter{相关技术与理论}
\\section{相关技术}
\\section{相关理论}

\\chapter{系统设计}
\\section{需求分析}
\\section{总体设计}
\\section{详细设计}

\\chapter{系统实现}
\\section{开发环境}
\\section{核心功能实现}
\\section{关键代码分析}

\\chapter{系统测试}
\\section{测试环境}
\\section{测试用例}
\\section{测试结果}

\\chapter{总结与展望}
\\section{全文总结}
\\section{工作展望}

\\bibliographystyle{plain}
\\bibliography{references}

\\appendix
\\chapter{附录代码}

\\end{document}`,
    preview: `\\documentclass[12pt,a4paper]{report}
\\title{论文题目}
\\author{作者姓名}
\\begin{document}
\\maketitle
\\tableofcontents
\\chapter{绪论}
...
\\chapter{系统设计}
...
\\chapter{系统实现}
...
\\chapter{总结与展望}
...
\\end{document}`
  },

  // ==========================================
  // 简历模板
  // ==========================================
  {
    id: 'cv-template',
    name: '学术简历',
    description: '学术简历/CV模板',
    category: 'cv',
    icon: '📄',
    size: '中等',
    difficulty: '简单',
    variables: [
      { name: 'name', default: '姓名', description: '姓名' },
      { name: 'email', default: 'email@example.com', description: '邮箱' },
      { name: 'phone', default: '电话', description: '电话' },
      { name: 'address', default: '地址', description: '地址' }
    ],
    requirements: [],
    content: `\\documentclass[11pt,a4paper]{article}

\\usepackage[utf8]{inputenc}
\\usepackage[margin=1in]{geometry}

\\begin{document}

\\begin{center}
\\LARGE \\textbf{{{name}}}

\\medskip
\\small
{{{email}}} \\textbar~{{{phone}}} \\textbar~{{{address}}}
\\end{center}

\\vspace{0.2in}

\\section*{教育背景}
\\textbf{学位名称} \hfill 年份-年份\\\\
\\textit{学校名称}，城市

\\section*{研究兴趣}
研究方向1、研究方向2、研究方向3

\\section*{发表论文}
\\begin{itemize}
    \\item 作者1, 作者2, 论文标题, 期刊名称, 卷号, 页码, 年份
    \\item 作者1, 作者2, 论文标题, 会议名称, 年份
\\end{itemize}

\\section*{工作经历}
\\textbf{职位名称} \hfill 年份-年份\\\\
\\textit{公司名称}，城市
\\begin{itemize}
    \\item 工作内容1
    \\item 工作内容2
\\end{itemize}

\\section*{获奖情况}
\\begin{itemize}
    \\item 奖项名称，年份
    \\item 奖项名称，年份
\\end{itemize}

\\section*{技能}
\\textbf{编程语言:} 语言1、语言2、语言3\\\\
\\textbf{工具:} 工具1、工具2、工具3

\\section*{语言}
\\begin{itemize}
    \\item 语言1（母语）
    \\item 语言2（流利）
    \\item 语言3（基础）
\\end{itemize}

\\end{document}`,
    preview: `\\documentclass[11pt,a4paper]{article}
\\begin{document}
\\begin{center}
\\LARGE \\textbf{姓名}
\\end{center}
\\section*{教育背景}
...
\\section*{发表论文}
...
\\section*{工作经历}
...
\\end{document}`
  }
]

const filteredTemplates = computed(() => {
  let filtered = templates

  // 按分类过滤
  if (selectedCategory.value !== 'all') {
    filtered = filtered.filter(t => t.category === selectedCategory.value)
  }

  // 按搜索关键词过滤
  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    filtered = filtered.filter(t =>
      t.name.toLowerCase().includes(query) ||
      t.description.toLowerCase().includes(query) ||
      t.content.toLowerCase().includes(query)
    )
  }

  return filtered
})

function selectTemplate(template: Template) {
  selectedTemplate.value = template
  variableValues.value = {}

  // Initialize variable values with defaults
  template.variables.forEach(v => {
    variableValues.value[v.name] = v.default
  })

  showDetail.value = true
}

function generateContent(template: Template): string {
  let content = template.content

  // Replace variables
  template.variables.forEach(v => {
    const value = variableValues.value[v.name] || v.default
    content = content.replace(new RegExp(`{{{${v.name}}}}`, 'g'), value)
  })

  return content
}

function useTemplate() {
  if (!selectedTemplate.value) return

  const content = generateContent(selectedTemplate.value)
  emit('insert', content)
  showDetail.value = false
  ElMessage.success('模板已应用')
}

function copyTemplate() {
  if (!selectedTemplate.value) return

  const content = generateContent(selectedTemplate.value)
  navigator.clipboard.writeText(content).then(() => {
    ElMessage.success('模板代码已复制到剪贴板')
  }).catch(() => {
    ElMessage.error('复制失败')
  })
}
</script>

<style scoped lang="scss">
.template-manager {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.manager-header {
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);

  h4 {
    margin: 0 0 8px 0;
    font-size: 16px;
    font-weight: 600;
  }

  .template-search {
    width: 100%;
  }
}

.template-categories {
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

.templates-list {
  flex: 1;
  overflow-y: auto;
  padding: 8px;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.template-card {
  padding: 12px;
  background: var(--el-fill-color-blank);
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    border-color: var(--el-color-primary-light-7);
    box-shadow: 0 2px 8px rgba(var(--el-color-primary-rgb), 0.1);
  }

  .template-preview {
    display: flex;
    align-items: flex-start;
    gap: 10px;
    margin-bottom: 8px;

    .preview-icon {
      font-size: 28px;
    }

    .preview-info {
      flex: 1;
      min-width: 0;

      .template-name {
        font-weight: 600;
        color: var(--el-text-color-primary);
        font-size: 14px;
        margin-bottom: 2px;
      }

      .template-desc {
        font-size: 12px;
        color: var(--el-text-color-secondary);
        line-height: 1.4;
      }
    }
  }

  .template-meta {
    display: flex;
    justify-content: space-between;
    align-items: center;

    .template-size {
      font-size: 11px;
      color: var(--el-text-color-placeholder);
    }
  }
}

.templates-empty {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 200px;
}

.template-detail {
  .detail-header {
    display: flex;
    align-items: flex-start;
    gap: 16px;
    margin-bottom: 20px;

    .detail-icon {
      font-size: 48px;
    }

    .detail-info {
      flex: 1;

      h3 {
        margin: 0 0 8px 0;
        font-size: 18px;
        font-weight: 600;
      }

      p {
        margin: 0 0 12px 0;
        color: var(--el-text-color-secondary);
        font-size: 14px;
      }

      .detail-tags {
        display: flex;
        gap: 6px;
      }
    }
  }

  .detail-content {
    .content-section {
      margin-bottom: 16px;

      h4 {
        margin: 0 0 8px 0;
        font-size: 14px;
        font-weight: 600;
      }
    }

    .code-preview {
      margin: 0;
      padding: 12px;
      background: var(--el-fill-color);
      border-radius: 4px;
      overflow-x: auto;
      font-size: 12px;
      line-height: 1.5;
      max-height: 300px;
    }

    .variables-list {
      display: flex;
      flex-direction: column;
      gap: 8px;

      .variable-item {
        display: flex;
        align-items: center;
        gap: 8px;

        .variable-name {
          min-width: 100px;
          font-size: 13px;
          font-weight: 500;
          color: var(--el-text-color-secondary);
        }

        .el-input {
          flex: 1;
        }
      }
    }

    .requirements-list {
      display: flex;
      flex-wrap: wrap;
      gap: 6px;
    }
  }
}
</style>
