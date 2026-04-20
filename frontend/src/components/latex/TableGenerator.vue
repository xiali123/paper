<template>
  <div class="table-generator">
    <div class="generator-header">
      <h4>表格生成器</h4>
      <el-space>
        <el-button size="small" @click="insertRow" :icon="Plus">行</el-button>
        <el-button size="small" @click="insertCol" :icon="Plus">列</el-button>
        <el-button size="small" @click="resetTable" :icon="Refresh">重置</el-button>
      </el-space>
    </div>

    <div class="generator-preview">
      <div class="table-wrapper">
        <table class="preview-table">
          <colgroup>
            <col v-for="(align, i) in columnAlignments" :key="i" :class="'align-' + align">
          </colgroup>
          <tbody>
            <tr v-for="(row, ri) in tableData" :key="ri">
              <td
                v-for="(cell, ci) in row"
                :key="ci"
                :colspan="cell.colspan"
                :rowspan="cell.rowspan"
                :class="{ 'merged': cell.merged, 'selected': selectedCell.row === ri && selectedCell.col === ci }"
                @click="selectCell(ri, ci)"
              >
                <input
                  v-if="!cell.merged"
                  v-model="cell.text"
                  @focus="selectCell(ri, ci)"
                  placeholder="内容"
                >
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <div class="generator-controls">
      <div class="control-section">
        <div class="section-title">列对齐</div>
        <div class="alignment-controls">
          <div
            v-for="(align, i) in columnAlignments"
            :key="i"
            class="align-toggle"
          >
            <el-select v-model="columnAlignments[i]" size="small">
              <el-option label="左对齐" value="l"></el-option>
              <el-option label="居中" value="c"></el-option>
              <el-option label="右对齐" value="r"></el-option>
            </el-select>
            <span class="col-label">列{{ i + 1 }}</span>
          </div>
        </div>
      </div>

      <div class="control-section" v-if="hasSelection">
        <div class="section-title">单元格操作</div>
        <el-space wrap>
          <el-button size="small" @click="mergeRight" :disabled="!canMergeRight">向右合并</el-button>
          <el-button size="small" @click="mergeDown" :disabled="!canMergeDown">向下合并</el-button>
          <el-button size="small" @click="splitCell" :disabled="!canSplit">拆分</el-button>
          <el-button size="small" type="danger" @click="deleteRow" :icon="Delete">删行</el-button>
          <el-button size="small" type="danger" @click="deleteCol" :icon="Delete">删列</el-button>
        </el-space>
      </div>

      <div class="control-section">
        <div class="section-title">表格选项</div>
        <el-space wrap>
          <el-checkbox v-model="options.booktabs">使用booktabs</el-checkbox>
          <el-checkbox v-model="options.caption">添加标题</el-checkbox>
          <el-checkbox v-model="options.label">添加标签</el-checkbox>
        </el-space>
        <el-input
          v-if="options.caption"
          v-model="options.captionText"
          placeholder="表格标题"
          size="small"
          style="margin-top: 8px;"
        />
        <el-input
          v-if="options.label"
          v-model="options.labelText"
          placeholder="标签 (tab:example)"
          size="small"
          style="margin-top: 8px;"
        />
      </div>

      <div class="control-section">
        <div class="section-title">生成代码</div>
        <el-button type="primary" @click="generateAndInsert" :icon="Check">插入表格</el-button>
        <el-button @click="copyCode" :icon="CopyDocument">复制代码</el-button>
      </div>
    </div>

    <div class="code-preview" v-if="generatedCode">
      <div class="preview-header">LaTeX代码预览</div>
      <pre><code>{{ generatedCode }}</code></pre>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { Plus, Refresh, Delete, Check, CopyDocument } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface Cell {
  text: string
  colspan: number
  rowspan: number
  merged: boolean
}

interface Emits {
  insert: [code: string]
}

const emit = defineEmits<Emits>()

const defaultRows = 3
const defaultCols = 3

const tableData = ref<Cell[][]>([])
const columnAlignments = ref<string[]>([])
const selectedCell = ref<{ row: number; col: number }>({ row: -1, col: -1 })

const options = ref({
  booktabs: true,
  caption: true,
  captionText: '示例表格',
  label: true,
  labelText: 'tab:example'
})

const generatedCode = ref('')

function initTable() {
  tableData.value = []
  columnAlignments.value = []

  for (let i = 0; i < defaultRows; i++) {
    const row: Cell[] = []
    for (let j = 0; j < defaultCols; j++) {
      row.push({
        text: '',
        colspan: 1,
        rowspan: 1,
        merged: false
      })
    }
    tableData.value.push(row)
  }

  for (let i = 0; i < defaultCols; i++) {
    columnAlignments.value.push('c')
  }

  updateGeneratedCode()
}

initTable()

function insertRow() {
  const newRow: Cell[] = []
  for (let i = 0; i < columnAlignments.value.length; i++) {
    newRow.push({
      text: '',
      colspan: 1,
      rowspan: 1,
      merged: false
    })
  }
  tableData.value.push(newRow)
  updateGeneratedCode()
}

function insertCol() {
  columnAlignments.value.push('c')
  tableData.value.forEach(row => {
    row.push({
      text: '',
      colspan: 1,
      rowspan: 1,
      merged: false
    })
  })
  updateGeneratedCode()
}

function deleteRow() {
  if (tableData.value.length <= 1) {
    ElMessage.warning('至少保留一行')
    return
  }
  if (selectedCell.value.row >= 0) {
    tableData.value.splice(selectedCell.value.row, 1)
    selectedCell.value = { row: -1, col: -1 }
    updateGeneratedCode()
  }
}

function deleteCol() {
  if (columnAlignments.value.length <= 1) {
    ElMessage.warning('至少保留一列')
    return
  }
  if (selectedCell.value.col >= 0) {
    tableData.value.forEach(row => {
      row.splice(selectedCell.value.col, 1)
    })
    columnAlignments.value.splice(selectedCell.value.col, 1)
    selectedCell.value = { row: -1, col: -1 }
    updateGeneratedCode()
  }
}

function resetTable() {
  initTable()
  ElMessage.success('已重置表格')
}

function selectCell(row: number, col: number) {
  selectedCell.value = { row, col }
}

const hasSelection = computed(() => selectedCell.value.row >= 0 && selectedCell.value.col >= 0)

const canMergeRight = computed(() => {
  if (!hasSelection.value) return false
  const { row, col } = selectedCell.value
  const cell = tableData.value[row][col]
  return col + cell.colspan < tableData.value[row].length
})

const canMergeDown = computed(() => {
  if (!hasSelection.value) return false
  const { row, col } = selectedCell.value
  const cell = tableData.value[row][cell]
  return row + cell.rowspan < tableData.value.length
})

const canSplit = computed(() => {
  if (!hasSelection.value) return false
  const { row, col } = selectedCell.value
  const cell = tableData.value[row][col]
  return cell.colspan > 1 || cell.rowspan > 1
})

function mergeRight() {
  if (!canMergeRight.value) return
  const { row, col } = selectedCell.value
  const cell = tableData.value[row][col]
  const nextCol = col + cell.colspan

  cell.colspan++
  tableData.value[row][nextCol].merged = true
  tableData.value[row][nextCol].colspan = 0

  updateGeneratedCode()
}

function mergeDown() {
  if (!canMergeDown.value) return
  const { row, col } = selectedCell.value
  const cell = tableData.value[row][col]
  const nextRow = row + cell.rowspan

  cell.rowspan++
  tableData.value[nextRow][col].merged = true
  tableData.value[nextRow][col].rowspan = 0

  updateGeneratedCode()
}

function splitCell() {
  if (!canSplit.value) return
  const { row, col } = selectedCell.value
  const cell = tableData.value[row][col]

  if (cell.colspan > 1) {
    for (let c = col + 1; c < col + cell.colspan; c++) {
      if (tableData.value[row][c].merged) {
        tableData.value[row][c].merged = false
        tableData.value[row][c].colspan = 1
      }
    }
    cell.colspan = 1
  }

  if (cell.rowspan > 1) {
    for (let r = row + 1; r < row + cell.rowspan; r++) {
      if (tableData.value[r][col].merged) {
        tableData.value[r][col].merged = false
        tableData.value[r][col].rowspan = 1
      }
    }
    cell.rowspan = 1
  }

  updateGeneratedCode()
}

function generateLatexCode(): string {
  const lines: string[] = []
  const cols = columnAlignments.value.join('')

  lines.push('\\begin{table}[h]')
  lines.push('  \\centering')
  lines.push(`  \\begin{tabular}{${options.value.booktabs ? cols : '|' + cols + '|'}}`)

  if (options.value.booktabs) {
    lines.push('    \\toprule')
  } else {
    lines.push('    \\hline')
  }

  tableData.value.forEach((row, ri) => {
    const cells: string[] = []
    row.forEach((cell, ci) => {
      if (!cell.merged) {
        cells.push(cell.text || ' ')
      }
    })
    lines.push('    ' + cells.join(' & '))

    if (options.value.booktabs) {
      if (ri === 0) {
        lines.push('    \\midrule')
      } else if (ri < tableData.value.length - 1) {
        // Optional: add midrule for specific rows
      }
    } else {
      lines.push('    \\hline')
    }
  })

  if (options.value.booktabs) {
    lines.push('    \\bottomrule')
  } else {
    lines.push('    \\hline')
  }

  lines.push('  \\end{tabular}')

  if (options.value.caption) {
    lines.push(`  \\caption{${options.value.captionText}}`)
  }

  if (options.value.label) {
    lines.push(`  \\label{${options.value.labelText}}`)
  }

  lines.push('\\end{table}')

  return lines.join('\n')
}

function updateGeneratedCode() {
  generatedCode.value = generateLatexCode()
}

function generateAndInsert() {
  emit('insert', generatedCode.value)
}

function copyCode() {
  navigator.clipboard.writeText(generatedCode.value).then(() => {
    ElMessage.success('代码已复制到剪贴板')
  }).catch(() => {
    ElMessage.error('复制失败')
  })
}

watch([tableData, columnAlignments, options], () => {
  updateGeneratedCode()
}, { deep: true })
</script>

<style scoped lang="scss">
.table-generator {
  display: flex;
  flex-direction: column;
  gap: 16px;
  padding: 16px;
}

.generator-header {
  display: flex;
  justify-content: space-between;
  align-items: center;

  h4 {
    margin: 0;
    font-size: 16px;
    font-weight: 600;
  }
}

.generator-preview {
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 8px;
  padding: 16px;
  background: var(--el-fill-color-blank);
  overflow-x: auto;

  .table-wrapper {
    min-width: 100%;
    display: inline-block;
  }

  .preview-table {
    border-collapse: collapse;
    margin: 0 auto;

    col {
      &.align-l {
        text-align: left;
      }
      &.align-c {
        text-align: center;
      }
      &.align-r {
        text-align: right;
      }
    }

    td {
      border: 1px solid var(--el-border-color);
      padding: 8px;
      min-width: 60px;
      width: 80px;
      height: 36px;
      position: relative;
      cursor: pointer;
      transition: all 0.2s;

      &:hover {
        background: var(--el-fill-color-light);
      }

      &.selected {
        outline: 2px solid var(--el-color-primary);
        outline-offset: -2px;
      }

      &.merged {
        background: var(--el-fill-color);
        cursor: not-allowed;
      }

      input {
        width: 100%;
        border: none;
        background: transparent;
        text-align: inherit;
        outline: none;
        font-size: 13px;
        color: var(--el-text-color-primary);
      }
    }
  }
}

.generator-controls {
  display: flex;
  flex-direction: column;
  gap: 16px;

  .control-section {
    border: 1px solid var(--el-border-color-lighter);
    border-radius: 8px;
    padding: 12px;

    .section-title {
      font-size: 13px;
      font-weight: 600;
      color: var(--el-text-color-secondary);
      margin-bottom: 8px;
    }
  }

  .alignment-controls {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;

    .align-toggle {
      display: flex;
      align-items: center;
      gap: 4px;

      .el-select {
        width: 100px;
      }

      .col-label {
        font-size: 12px;
        color: var(--el-text-color-secondary);
      }
    }
  }
}

.code-preview {
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 8px;
  overflow: hidden;

  .preview-header {
    padding: 8px 12px;
    background: var(--el-fill-color-light);
    font-size: 12px;
    font-weight: 600;
    color: var(--el-text-color-secondary);
    border-bottom: 1px solid var(--el-border-color-lighter);
  }

  pre {
    margin: 0;
    padding: 12px;
    background: var(--el-fill-color-blank);
    overflow-x: auto;
    font-size: 12px;
    line-height: 1.5;

    code {
      font-family: 'Courier New', monospace;
      color: var(--el-text-color-primary);
    }
  }
}
</style>
