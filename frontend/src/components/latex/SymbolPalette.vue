<template>
  <div class="symbol-palette">
    <div class="palette-header">
      <h3>LaTeX 符号</h3>
      <el-input
        v-model="searchQuery"
        placeholder="搜索符号..."
        size="small"
        clearable
        prefix-icon="Search"
      />
    </div>

    <div class="palette-content">
      <el-tabs v-model="activeTab" class="symbol-tabs">
        <el-tab-pane
          v-for="category in categories"
          :key="category.name"
          :name="category.name"
          :label="category.label"
        >
          <div class="symbol-grid" v-once>
            <div
              v-for="symbol in getFilteredSymbols(category.symbols)"
              :key="symbol.command"
              class="symbol-item"
              :title="`${symbol.command} - ${symbol.description}`"
              @click="insertSymbol(symbol.command)"
            >
              <span class="symbol-preview" v-html="renderSymbol(symbol)"></span>
            </div>
          </div>
        </el-tab-pane>
      </el-tabs>
    </div>

    <div class="palette-footer">
      <el-button size="small" @click="insertSymbol('\\frac{ }{ }')">
        插入分数
      </el-button>
      <el-button size="small" @click="insertSymbol('\\sqrt{ }')">
        插入根号
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import katex from 'katex'
import 'katex/dist/katex.min.css'

interface Symbol {
  command: string
  description: string
  category: string
  mathMode?: boolean
}

interface Emits {
  insert: [symbol: string]
}

const emit = defineEmits<Emits>()

const searchQuery = ref('')
const activeTab = ref('operators')

// 符号分类数据
const categories = [
  {
    name: 'operators',
    label: '运算符',
    symbols: [
      { command: '\\alpha', description: 'Alpha', category: 'operators' },
      { command: '\\beta', description: 'Beta', category: 'operators' },
      { command: '\\gamma', description: 'Gamma', category: 'operators' },
      { command: '\\delta', description: 'Delta', category: 'operators' },
      { command: '\\epsilon', description: 'Epsilon', category: 'operators' },
      { command: '\\zeta', description: 'Zeta', category: 'operators' },
      { command: '\\eta', description: 'Eta', category: 'operators' },
      { command: '\\theta', description: 'Theta', category: 'operators' },
      { command: '\\iota', description: 'Iota', category: 'operators' },
      { command: '\\kappa', description: 'Kappa', category: 'operators' },
      { command: '\\lambda', description: 'Lambda', category: 'operators' },
      { command: '\\mu', description: 'Mu', category: 'operators' },
      { command: '\\nu', description: 'Nu', category: 'operators' },
      { command: '\\xi', description: 'Xi', category: 'operators' },
      { command: '\\pi', description: 'Pi', category: 'operators' },
      { command: '\\rho', description: 'Rho', category: 'operators' },
      { command: '\\sigma', description: 'Sigma', category: 'operators' },
      { command: '\\tau', description: 'Tau', category: 'operators' },
      { command: '\\upsilon', description: 'Upsilon', category: 'operators' },
      { command: '\\phi', description: 'Phi', category: 'operators' },
      { command: '\\chi', description: 'Chi', category: 'operators' },
      { command: '\\psi', description: 'Psi', category: 'operators' },
      { command: '\\omega', description: 'Omega', category: 'operators' }
    ]
  },
  {
    name: 'relations',
    label: '关系符',
    symbols: [
      { command: '\\leq', description: '小于等于', category: 'relations' },
      { command: '\\geq', description: '大于等于', category: 'relations' },
      { command: '\\neq', description: '不等于', category: 'relations' },
      { command: '\\approx', description: '约等于', category: 'relations' },
      { command: '\\equiv', description: '恒等于', category: 'relations' },
      { command: '\\propto', description: '正比于', category: 'relations' },
      { command: '\\sim', description: '相似于', category: 'relations' },
      { command: '\\simeq', description: '渐进等于', category: 'relations' },
      { command: '\\cong', description: '全等于', category: 'relations' },
      { command: '\\prec', description: '先于', category: 'relations' },
      { command: '\\succ', description: '后于', category: 'relations' },
      { command: '\\subset', description: '子集', category: 'relations' },
      { command: '\\supset', description: '超集', category: 'relations' },
      { command: '\\subseteq', description: '子集等于', category: 'relations' },
      { command: '\\supseteq', description: '超集等于', category: 'relations' },
      { command: '\\in', description: '属于', category: 'relations' },
      { command: '\\ni', description: '包含', category: 'relations' },
      { command: '\\notin', description: '不属于', category: 'relations' }
    ]
  },
  {
    name: 'operations',
    label: '运算',
    symbols: [
      { command: '\\sum', description: '求和', category: 'operations' },
      { command: '\\prod', description: '乘积', category: 'operations' },
      { command: '\\int', description: '积分', category: 'operations' },
      { command: '\\iint', description: '二重积分', category: 'operations' },
      { command: '\\iiint', description: '三重积分', category: 'operations' },
      { command: '\\oint', description: '环路积分', category: 'operations' },
      { command: '\\lim', description: '极限', category: 'operations' },
      { command: '\\frac{ }{ }', description: '分数', category: 'operations' },
      { command: '\\sqrt{ }', description: '平方根', category: 'operations' },
      { command: '\\sqrt[n]{ }', description: 'n次方根', category: 'operations' },
      { command: '\\binom{}{}', description: '二项式系数', category: 'operations' },
      { command: '\\pm', description: '加减', category: 'operations' },
      { command: '\\mp', description: '减加', category: 'operations' },
      { command: '\\times', description: '乘号', category: 'operations' },
      { command: '\\div', description: '除号', category: 'operations' },
      { command: '\\cdot', description: '点乘', category: 'operations' },
      { command: '\\ast', description: '星号', category: 'operations' },
      { command: '\\star', description: '星形', category: 'operations' }
    ]
  },
  {
    name: 'arrows',
    label: '箭头',
    symbols: [
      { command: '\\rightarrow', description: '右箭头', category: 'arrows' },
      { command: '\\leftarrow', description: '左箭头', category: 'arrows' },
      { command: '\\uparrow', description: '上箭头', category: 'arrows' },
      { command: '\\downarrow', description: '下箭头', category: 'arrows' },
      { command: '\\leftrightarrow', description: '双向箭头', category: 'arrows' },
      { command: '\\Rightarrow', description: '右双箭头', category: 'arrows' },
      { command: '\\Leftarrow', description: '左双箭头', category: 'arrows' },
      { command: '\\Leftrightarrow', description: '双向双箭头', category: 'arrows' },
      { command: '\\mapsto', description: '映射箭头', category: 'arrows' },
      { command: '\\hookrightarrow', description: '嵌入箭头', category: 'arrows' },
      { command: '\\rightharpoonup', description: '右 harpoon', category: 'arrows' },
      { command: '\\leftharpoonup', description: '左 harpoon', category: 'arrows' },
      { command: '\\nearrow', description: '东北箭头', category: 'arrows' },
      { command: '\\searrow', description: '东南箭头', category: 'arrows' },
      { command: '\\swarrow', description: '西南箭头', category: 'arrows' },
      { command: '\\nwarrow', description: '西北箭头', category: 'arrows' }
    ]
  },
  {
    name: 'delimiters',
    label: '分隔符',
    symbols: [
      { command: '\\{', description: '左花括号', category: 'delimiters' },
      { command: '\\}', description: '右花括号', category: 'delimiters' },
      { command: '\\langle', description: '左尖括号', category: 'delimiters' },
      { command: '\\rangle', description: '右尖括号', category: 'delimiters' },
      { command: '\\lvert', description: '左竖线', category: 'delimiters' },
      { command: '\\rvert', description: '右竖线', category: 'delimiters' },
      { command: '\\lVert', description: '左双竖线', category: 'delimiters' },
      { command: '\\rVert', description: '右双竖线', category: 'delimiters' },
      { command: '\\lfloor', description: '左向下取整', category: 'delimiters' },
      { command: '\\rfloor', description: '右向下取整', category: 'delimiters' },
      { command: '\\lceil', description: '左向上取整', category: 'delimiters' },
      { command: '\\rceil', description: '右向上取整', category: 'delimiters' },
      { command: '\\vert', description: '竖线', category: 'delimiters' },
      { command: '\\Vert', description: '双竖线', category: 'delimiters' },
      { command: '\\|', description: '平行', category: 'delimiters' }
    ]
  }
]

function getFilteredSymbols(symbols: Symbol[]): Symbol[] {
  if (!searchQuery.value) return symbols

  const query = searchQuery.value.toLowerCase()
  return symbols.filter(symbol =>
    symbol.command.toLowerCase().includes(query) ||
    symbol.description.toLowerCase().includes(query)
  )
}

function renderSymbol(symbol: Symbol): string {
  try {
    if (symbol.command.includes('{ }') || symbol.command.includes('{}')) {
      // 对于需要参数的命令，显示简化版本
      const simplified = symbol.command.replace(/\{ ?\}?/g, 'x')
      return katex.renderToString(simplified, {
        displayMode: false,
        throwOnError: false
      })
    }

    return katex.renderToString(symbol.command, {
      displayMode: false,
      throwOnError: false
    })
  } catch {
    return symbol.command
  }
}

function insertSymbol(symbol: string) {
  emit('insert', symbol)
}
</script>

<style scoped lang="scss">
.symbol-palette {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.palette-header {
  padding: 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);

  h3 {
    margin: 0 0 12px 0;
    font-size: 16px;
    font-weight: 600;
    color: var(--el-text-color-primary);
  }
}

.palette-content {
  flex: 1;
  overflow-y: auto;
  padding: 0;
}

.symbol-tabs {
  height: 100%;

  :deep(.el-tabs__content) {
    height: calc(100% - 40px);
    overflow-y: auto;
  }

  :deep(.el-tab-pane) {
    height: 100%;
  }
}

.symbol-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(60px, 1fr));
  gap: 8px;
  padding: 16px;
}

.symbol-item {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 50px;
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s ease;
  background: var(--el-bg-color);

  &:hover {
    background: var(--el-color-primary-light-9);
    border-color: var(--el-color-primary);
    transform: translateY(-1px);
  }

  .symbol-preview {
    font-size: 16px;
    color: var(--el-text-color-primary);

    :deep(.katex) {
      font-size: 14px;
    }
  }
}

.palette-footer {
  padding: 16px;
  border-top: 1px solid var(--el-border-color-lighter);
  display: flex;
  gap: 8px;
  justify-content: center;
}

// 响应式调整
@media (max-width: 768px) {
  .symbol-grid {
    grid-template-columns: repeat(auto-fill, minmax(50px, 1fr));
    gap: 6px;
    padding: 12px;
  }

  .symbol-item {
    height: 45px;

    .symbol-preview {
      font-size: 14px;

      :deep(.katex) {
        font-size: 12px;
      }
    }
  }
}

// Dark mode overrides
[data-theme="dark"] {
  .palette-header {
    border-bottom-color: rgba(102, 126, 234, 0.3);

    h3 {
      color: #f3f4f6;
    }
  }

  .symbol-item {
    background: rgba(40, 40, 45, 0.98);
    border-color: rgba(102, 126, 234, 0.3);

    &:hover {
      background: rgba(64, 158, 255, 0.15);
      border-color: var(--el-color-primary);
    }

    .symbol-preview {
      color: #f3f4f6;
    }
  }

  .palette-footer {
    border-top-color: rgba(102, 126, 234, 0.3);
  }
}
</style>