<template>
  <el-drawer
    v-model="visible"
    title="快速插入"
    direction="rtl"
    size="400px"
    :close-on-click-modal="false"
    class="quick-insert-drawer"
  >
    <div class="quick-insert-content">
      <!-- 搜索框 -->
      <div class="search-section">
        <el-input
          v-model="searchQuery"
          placeholder="搜索命令..."
          :prefix-icon="Search"
          clearable
          size="large"
        />
      </div>

      <!-- 分类标签 -->
      <div class="category-tabs">
        <el-radio-group v-model="activeCategory" size="small">
          <el-radio-button
            v-for="cat in categories"
            :key="cat.id"
            :label="cat.id"
          >
            {{ cat.label }}
          </el-radio-button>
        </el-radio-group>
      </div>

      <!-- 命令列表 -->
      <div class="commands-section">
        <div
          v-for="group in filteredGroups"
          :key="group.name"
          class="command-group"
        >
          <div class="group-title">{{ group.name }}</div>
          <div class="command-list">
            <div
              v-for="cmd in group.commands"
              :key="cmd.id"
              class="command-item"
              @click="insertCommand(cmd)"
              :title="cmd.preview"
            >
              <div class="command-icon" v-html="cmd.icon"></div>
              <div class="command-info">
                <div class="command-name">{{ cmd.name }}</div>
                <div class="command-code">{{ cmd.code }}</div>
              </div>
              <el-icon class="command-arrow"><ArrowRight /></el-icon>
            </div>
          </div>
        </div>

        <el-empty
          v-if="filteredGroups.length === 0"
          description="没有找到匹配的命令"
          :image-size="80"
        />
      </div>
    </div>
  </el-drawer>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Search, ArrowRight } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface Command {
  id: string
  name: string
  code: string
  icon: string
  preview: string
  category: string
}

interface CommandGroup {
  name: string
  commands: Command[]
}

interface Category {
  id: string
  label: string
}

const emit = defineEmits<{
  insert: [code: string, cursorOffset?: number]
}>()

const visible = ref(false)
const searchQuery = ref('')
const activeCategory = ref('all')

const categories: Category[] = [
  { id: 'all', label: '全部' },
  { id: 'basic', label: '基础' },
  { id: 'math', label: '数学' },
  { id: 'format', label: '格式' },
  { id: 'structure', label: '结构' },
  { id: 'table', label: '表格' },
  { id: 'image', label: '图片' },
]

const commandGroups: CommandGroup[] = [
  {
    name: '文档结构',
    commands: [
      {
        id: 'section',
        name: '章节',
        code: '\\section{${1:title}}',
        icon: '📄',
        preview: '创建新章节',
        category: 'structure'
      },
      {
        id: 'subsection',
        name: '小节',
        code: '\\subsection{${1:title}}',
        icon: '📑',
        preview: '创建小节',
        category: 'structure'
      },
      {
        id: 'paragraph',
        name: '段落',
        code: '\\paragraph{${1:title}}',
        icon: '📝',
        preview: '创建段落标题',
        category: 'structure'
      },
    ]
  },
  {
    name: '数学公式 - 希腊字母',
    commands: [
      { id: 'alpha', name: 'α', code: '\\alpha', icon: 'α', preview: 'alpha', category: 'math' },
      { id: 'beta', name: 'β', code: '\\beta', icon: 'β', preview: 'beta', category: 'math' },
      { id: 'gamma', name: 'γ', code: '\\gamma', icon: 'γ', preview: 'gamma', category: 'math' },
      { id: 'delta', name: 'δ', code: '\\delta', icon: 'δ', preview: 'delta', category: 'math' },
      { id: 'epsilon', name: 'ε', code: '\\epsilon', icon: 'ε', preview: 'epsilon', category: 'math' },
      { id: 'theta', name: 'θ', code: '\\theta', icon: 'θ', preview: 'theta', category: 'math' },
      { id: 'lambda', name: 'λ', code: '\\lambda', icon: 'λ', preview: 'lambda', category: 'math' },
      { id: 'mu', name: 'μ', code: '\\mu', icon: 'μ', preview: 'mu', category: 'math' },
      { id: 'pi', name: 'π', code: '\\pi', icon: 'π', preview: 'pi', category: 'math' },
      { id: 'sigma', name: 'σ', code: '\\sigma', icon: 'σ', preview: 'sigma', category: 'math' },
      { id: 'phi', name: 'φ', code: '\\phi', icon: 'φ', preview: 'phi', category: 'math' },
      { id: 'omega', name: 'ω', code: '\\omega', icon: 'ω', preview: 'omega', category: 'math' },
    ]
  },
  {
    name: '数学公式 - 运算符',
    commands: [
      {
        id: 'frac',
        name: '分数',
        code: '\\frac{${1:numerator}}{${2:denominator}}',
        icon: '⁄',
        preview: 'a/b',
        category: 'math'
      },
      {
        id: 'sqrt',
        name: '平方根',
        code: '\\sqrt{${1:value}}',
        icon: '√',
        preview: '√x',
        category: 'math'
      },
      {
        id: 'sum',
        name: '求和',
        code: '\\sum_{${1:i=1}}^{${2:n}}',
        icon: 'Σ',
        preview: '∑',
        category: 'math'
      },
      {
        id: 'prod',
        name: '乘积',
        code: '\\prod_{${1:i=1}}^{${2:n}}',
        icon: '∏',
        preview: '∏',
        category: 'math'
      },
      {
        id: 'int',
        name: '积分',
        code: '\\int_{${1:a}}^{${2:b}}',
        icon: '∫',
        preview: '∫',
        category: 'math'
      },
      {
        id: 'lim',
        name: '极限',
        code: '\\lim_{${1:x \\to \\infty}}',
        icon: 'lim',
        preview: 'lim',
        category: 'math'
      },
    ]
  },
  {
    name: '数学公式 - 上下标',
    commands: [
      {
        id: 'superscript',
        name: '上标',
        code: '^{$1}',
        icon: 'x²',
        preview: 'x^n',
        category: 'math'
      },
      {
        id: 'subscript',
        name: '下标',
        code: '_{$1}',
        icon: 'x₁',
        preview: 'x_n',
        category: 'math'
      },
      {
        id: 'combined',
        name: '上下标',
        code: '_{$1}^{$2}',
        icon: 'xᵧ',
        preview: 'x_n^m',
        category: 'math'
      },
    ]
  },
  {
    name: '格式化',
    commands: [
      {
        id: 'textbf',
        name: '粗体',
        code: '\\textbf{$1}',
        icon: '<b>B</b>',
        preview: '粗体文本',
        category: 'format'
      },
      {
        id: 'textit',
        name: '斜体',
        code: '\\textit{$1}',
        icon: '<i>I</i>',
        preview: '斜体文本',
        category: 'format'
      },
      {
        id: 'underline',
        name: '下划线',
        code: '\\underline{$1}',
        icon: '<u>U</u>',
        preview: '下划线文本',
        category: 'format'
      },
      {
        id: 'emph',
        name: '强调',
        code: '\\emph{$1}',
        icon: '!',
        preview: '强调文本',
        category: 'format'
      },
    ]
  },
  {
    name: '环境',
    commands: [
      {
        id: 'itemize',
        name: '无序列表',
        code: '\\begin{itemize}\n  \\item $1\n\\end{itemize}',
        icon: '•',
        preview: '无序列表',
        category: 'structure'
      },
      {
        id: 'enumerate',
        name: '有序列表',
        code: '\\begin{enumerate}\n  \\item $1\n\\end{enumerate}',
        icon: '1.',
        preview: '有序列表',
        category: 'structure'
      },
      {
        id: 'description',
        name: '描述列表',
        code: '\\begin{description}\n  \\item[$1] \n\\end{description}',
        icon: ':',
        preview: '描述列表',
        category: 'structure'
      },
      {
        id: 'figure',
        name: '图片环境',
        code: '\\begin{figure}[htbp]\n  \\centering\n  \\includegraphics[width=0.8\\textwidth]{$1}\n  \\caption{$2}\n  \\label{fig:$3}\n\\end{figure}',
        icon: '🖼️',
        preview: '图片环境',
        category: 'image'
      },
      {
        id: 'table',
        name: '表格环境',
        code: '\\begin{table}[htbp]\n  \\centering\n  \\begin{tabular}{$1}\n    $2\n  \\end{tabular}\n  \\caption{$3}\n  \\label{tab:$4}\n\\end{table}',
        icon: '▦',
        preview: '表格环境',
        category: 'table'
      },
      {
        id: 'equation',
        name: '公式环境',
        code: '\\begin{equation}\n  $1\n\\end{equation}',
        icon: '∑',
        preview: '公式环境',
        category: 'math'
      },
      {
        id: 'align',
        name: '对齐公式',
        code: '\\begin{align}\n  $1 &= $2 \\\\\n  $3 &= $4\n\\end{align}',
        icon: '⚌',
        preview: '对齐公式',
        category: 'math'
      },
    ]
  },
  {
    name: '特殊符号',
    commands: [
      { id: 'deg', name: '度数', code: '^\\circ', icon: '°', preview: '°', category: 'basic' },
      { id: 'pm', name: '正负', code: '\\pm', icon: '±', preview: '±', category: 'math' },
      { id: 'times', name: '乘号', code: '\\times', icon: '×', preview: '×', category: 'math' },
      { id: 'div', name: '除号', code: '\\div', icon: '÷', preview: '÷', category: 'math' },
      { id: 'neq', name: '不等', code: '\\neq', icon: '≠', preview: '≠', category: 'math' },
      { id: 'leq', name: '小于等于', code: '\\leq', icon: '≤', preview: '≤', category: 'math' },
      { id: 'geq', name: '大于等于', code: '\\geq', icon: '≥', preview: '≥', category: 'math' },
      { id: 'approx', name: '约等于', code: '\\approx', icon: '≈', preview: '≈', category: 'math' },
      { id: 'infty', name: '无穷', code: '\\infty', icon: '∞', preview: '∞', category: 'math' },
    ]
  },
]

const filteredGroups = computed(() => {
  const query = searchQuery.value.toLowerCase()
  const category = activeCategory.value

  return commandGroups
    .map(group => ({
      name: group.name,
      commands: group.commands.filter(cmd => {
        const matchesSearch =
          !query ||
          cmd.name.toLowerCase().includes(query) ||
          cmd.code.toLowerCase().includes(query) ||
          cmd.preview.toLowerCase().includes(query)

        const matchesCategory =
          category === 'all' || cmd.category === category

        return matchesSearch && matchesCategory
      })
    }))
    .filter(group => group.commands.length > 0)
})

const insertCommand = (command: Command) => {
  emit('insert', command.code)
  ElMessage.success(`已插入: ${command.name}`)
  visible.value = false
}

const open = () => {
  visible.value = true
}

defineExpose({ open })
</script>

<style scoped lang="scss">
.quick-insert-drawer {
  :deep(.el-drawer__body) {
    padding: 0;
    display: flex;
    flex-direction: column;
  }
}

.quick-insert-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.search-section {
  padding: 16px;
  border-bottom: 1px solid var(--el-border-color-light);
  background: var(--el-bg-color);
}

.category-tabs {
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color-light);
  background: var(--el-bg-color);
  overflow-x: auto;

  :deep(.el-radio-group) {
    display: flex;
    flex-wrap: nowrap;
  }
}

.commands-section {
  flex: 1;
  overflow-y: auto;
  padding: 16px;
}

.command-group {
  margin-bottom: 24px;

  &:last-child {
    margin-bottom: 0;
  }
}

.group-title {
  font-size: 13px;
  font-weight: 600;
  color: var(--el-text-color-secondary);
  margin: 0 0 12px 0;
  padding: 0 4px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.command-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.command-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px;
  border-radius: 8px;
  background: var(--el-fill-color-light);
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    background: var(--el-fill-color);
    transform: translateX(2px);

    .command-arrow {
      opacity: 1;
      transform: translateX(4px);
    }
  }

  &:active {
    transform: translateX(0) scale(0.98);
  }
}

.command-icon {
  flex-shrink: 0;
  width: 36px;
  height: 36px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 18px;
  background: var(--el-bg-color);
  border-radius: 6px;
  border: 1px solid var(--el-border-color-light);
}

.command-info {
  flex: 1;
  min-width: 0;
}

.command-name {
  font-size: 14px;
  font-weight: 500;
  color: var(--el-text-color-primary);
  margin-bottom: 2px;
}

.command-code {
  font-size: 12px;
  font-family: 'Consolas', 'Monaco', monospace;
  color: var(--el-text-color-secondary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.command-arrow {
  flex-shrink: 0;
  font-size: 16px;
  color: var(--el-color-primary);
  opacity: 0;
  transition: all 0.2s;
}
</style>
