<template>
  <el-drawer
    :model-value="show"
    @update:model-value="$emit('update:show', $event)"
    title="可视化表格编辑器"
    direction="btt"
    size="80%"
  >
    <div class="table-editor">
      <!-- 表格工具栏 -->
      <div class="editor-toolbar">
        <el-button-group>
          <el-tooltip content="插入行 (在上方)">
            <el-button @click="insertRow('above')">
              <el-icon><Top /></el-icon>
            </el-button>
          </el-tooltip>
          <el-tooltip content="插入行 (在下方)">
            <el-button @click="insertRow('below')">
              <el-icon><Bottom /></el-icon>
            </el-button>
          </el-tooltip>
          <el-tooltip content="删除行">
            <el-button @click="deleteRow" :disabled="selectedCell.row === -1">
              <el-icon><Delete /></el-icon>
            </el-button>
          </el-tooltip>
        </el-button-group>

        <el-divider direction="vertical" />

        <el-button-group>
          <el-tooltip content="插入列 (在左侧)">
            <el-button @click="insertColumn('left')">
              <el-icon><Back /></el-icon>
            </el-button>
          </el-tooltip>
          <el-tooltip content="插入列 (在右侧)">
            <el-button @click="insertColumn('right')">
              <el-icon><Right /></el-icon>
            </el-button>
          </el-tooltip>
          <el-tooltip content="删除列">
            <el-button @click="deleteColumn" :disabled="selectedCell.col === -1">
              <el-icon><Delete /></el-icon>
            </el-button>
          </el-tooltip>
        </el-button-group>

        <el-divider direction="vertical" />

        <el-button-group>
          <el-tooltip content="合并单元格">
            <el-button @click="mergeCells" :disabled="!canMergeCells">
              <el-icon><Grid /></el-icon>
            </el-button>
          </el-tooltip>
          <el-tooltip content="拆分单元格">
            <el-button @click="splitCell" :disabled="!canSplitCell">
              <el-icon><Grid /></el-icon>
            </el-button>
          </el-tooltip>
        </el-button-group>

        <el-divider direction="vertical" />

        <el-select v-model="tableStyle" @change="updateTableStyle" style="width: 120px">
          <el-option label="普通表格" value="standard" />
          <el-option label="三线表" value="threeline" />
          <el-option label="学术表" value="academic" />
        </el-select>

        <el-button type="primary" @click="generateCode">
          <el-icon><Document /></el-icon>
          生成代码
        </el-button>
      </div>

      <!-- 表格编辑区 -->
      <div class="editor-content">
        <el-table
          :data="tableData"
          border
          @cell-click="handleCellClick"
          @cell-dblclick="handleCellDoubleClick"
          class="editable-table"
        >
          <el-table-column
            v-for="(col, colIndex) in columnCount"
            :key="colIndex"
            :label="getColumnLabel(colIndex)"
            :width="getColumnWidth(colIndex)"
          >
            <template #default="{ row, $index }">
              <div
                class="table-cell"
                :class="{
                  'is-selected': isSelected($index, colIndex),
                  'is-header': $index === 0 && tableStyle !== 'standard',
                  'is-focused': focusedCell.row === $index && focusedCell.col === colIndex
                }"
              >
                <input
                  v-if="editingCell.row === $index && editingCell.col === colIndex"
                  ref="cellInput"
                  v-model="tableData[$index][colIndex]"
                  @blur="finishEditing"
                  @keydown.enter="finishEditing"
                  @keydown.esc="cancelEditing"
                  @keydown.tab="handleTab($event, $index, colIndex)"
                  class="cell-input"
                />
                <span v-else @click="startEditing($index, colIndex)">
                  {{ tableData[$index][colIndex] || `(${index},${colIndex})` }}
                </span>
              </div>
            </template>
          </el-table-column>
        </el-table>
      </div>

      <!-- 表格属性面板 -->
      <div class="table-properties">
        <el-collapse v-model="activePanels">
          <el-collapse-item title="表格设置" name="settings">
            <el-form label-width="100px" size="small">
              <el-row :gutter="16">
                <el-col :span="12">
                  <el-form-item label="行数">
                    <el-input-number
                      v-model="rowCount"
                      :min="1"
                      :max="50"
                      @change="updateTableSize"
                    />
                  </el-form-item>
                </el-col>
                <el-col :span="12">
                  <el-form-item label="列数">
                    <el-input-number
                      v-model="columnCount"
                      :min="1"
                      :max="20"
                      @change="updateTableSize"
                    />
                  </el-form-item>
                </el-col>
              </el-row>
              <el-form-item label="表格位置">
                <el-radio-group v-model="tablePosition">
                  <el-radio label="h">居中</el-radio>
                  <el-radio label="t">顶部</el-radio>
                  <el-radio label="b">底部</el-radio>
                </el-radio-group>
              </el-form-item>
              <el-form-item label="表格标题">
                <el-input v-model="tableCaption" placeholder="表格标题（可选）" />
              </el-form-item>
              <el-form-item label="标签">
                <el-input v-model="tableLabel" placeholder="用于引用的标签（可选）" />
              </el-form-item>
            </el-form>
          </el-collapse-item>

          <el-collapse-item title="单元格属性" name="cell">
            <el-form v-if="selectedCell.row !== -1" label-width="100px" size="small">
              <el-form-item label="水平对齐">
                <el-radio-group v-model="cellAlign.h">
                  <el-radio label="l">左</el-radio>
                  <el-radio label="c">中</el-radio>
                  <el-radio label="r">右</el-radio>
                </el-radio-group>
              </el-form-item>
              <el-form-item label="垂直对齐">
                <el-radio-group v-model="cellAlign.v">
                  <el-radio label="t">顶</el-radio>
                  <el-radio label="m">中</el-radio>
                  <el-radio label="b">底</el-radio>
                </el-radio-group>
              </el-form-item>
              <el-form-item label="加粗">
                <el-switch v-model="cellBold" @change="applyCellStyle" />
              </el-form-item>
              <el-form-item label="列宽">
                <el-input v-model="cellWidth" placeholder="auto" />
              </el-form-item>
            </el-form>
            <el-empty v-else description="请选择一个单元格" :image-size="60" />
          </el-collapse-item>

          <el-collapse-item title="高级选项" name="advanced">
            <el-form label-width="120px" size="small">
              <el-form-item label="使用booktabs">
                <el-switch v-model="useBooktabs" />
              </el-form-item>
              <el-form-item label="行间距">
                <el-input v-model="rowSpacing" placeholder="1.5" />
              </el-form-item>
              <el-form-item label="字体大小">
                <el-select v-model="fontSize">
                  <el-option label="默认" value="" />
                  <el-option label="\small" value="small" />
                  <el-option label="\footnotesize" value="footnotesize" />
                </el-select>
              </el-form-item>
            </el-form>
          </el-collapse-item>
        </el-collapse>
      </div>
    </div>

    <!-- 代码预览对话框 -->
    <el-dialog v-model="showCodePreview" title="生成的LaTeX代码" width="700px">
      <el-input
        v-model="generatedCode"
        type="textarea"
        :rows="15"
        readonly
        class="code-preview"
      />
      <template #footer>
        <el-button @click="showCodePreview = false">关闭</el-button>
        <el-button type="primary" @click="copyCode">
          <el-icon><DocumentCopy /></el-icon>
          复制代码
        </el-button>
        <el-button type="success" @click="insertCode">
          <el-icon><Plus /></el-icon>
          插入到编辑器
        </el-button>
      </template>
    </el-dialog>
  </el-drawer>
</template>

<script setup lang="ts">
import { ref, computed, watch, nextTick } from 'vue'
import {
  Top, Bottom, Back, Right, Delete, Grid, Document, DocumentCopy, Plus
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface Props {
  show: boolean
}

defineProps<Props>()

const emit = defineEmits<{
  'update:show': [value: boolean]
  'insert': [code: string]
}>()

// 表格数据
const rowCount = ref(5)
const columnCount = ref(4)
const tableData = ref<string[][]>([])
const selectedCell = ref({ row: -1, col: -1 })
const focusedCell = ref({ row: -1, col: -1 })
const editingCell = ref({ row: -1, col: -1 })

// 表格样式
const tableStyle = ref('threeline')
const tablePosition = ref('h')
const tableCaption = ref('')
const tableLabel = ref('')
const useBooktabs = ref(true)
const rowSpacing = ref('1.5')
const fontSize = ref('')

// 单元格样式
const cellAlign = ref({ h: 'c', v: 'm' })
const cellBold = ref(false)
const cellWidth = ref('')

// UI状态
const activePanels = ref(['settings'])
const showCodePreview = ref(false)
const generatedCode = ref('')

// 初始化表格数据
function initTableData() {
  const data: string[][] = []
  for (let i = 0; i < rowCount.value; i++) {
    data[i] = []
    for (let j = 0; j < columnCount.value; j++) {
      data[i][j] = ''
    }
  }
  tableData.value = data
}

watch([rowCount, columnCount], () => {
  // 保留现有数据
  const newData: string[][] = []
  for (let i = 0; i < rowCount.value; i++) {
    newData[i] = []
    for (let j = 0; j < columnCount.value; j++) {
      newData[i][j] = tableData.value[i]?.[j] || ''
    }
  }
  tableData.value = newData
})

// 表格操作
function handleCellClick(row: any, column: any, cell: any) {
  selectedCell.value = {
    row: row.$index,
    col: column.index
  }
}

function handleCellDoubleClick(row: any, column: any) {
  startEditing(row.$index, column.index)
}

function startEditing(row: number, col: number) {
  editingCell.value = { row, col }
  focusedCell.value = { row, col }
  nextTick(() => {
    const input = document.querySelector('.cell-input') as HTMLInputElement
    input?.focus()
  })
}

function finishEditing() {
  editingCell.value = { row: -1, col: -1 }
}

function cancelEditing() {
  // 恢复原始值
  editingCell.value = { row: -1, col: -1 }
}

function handleTab(event: KeyboardEvent, row: number, col: number) {
  event.preventDefault()
  finishEditing()
  if (event.shiftKey) {
    // Tab backwards
    if (col > 0) {
      startEditing(row, col - 1)
    } else if (row > 0) {
      startEditing(row - 1, columnCount.value - 1)
    }
  } else {
    // Tab forward
    if (col < columnCount.value - 1) {
      startEditing(row, col + 1)
    } else if (row < rowCount.value - 1) {
      startEditing(row + 1, 0)
    }
  }
}

function isSelected(row: number, col: number): boolean {
  return selectedCell.value.row === row && selectedCell.value.col === col
}

function insertRow(position: 'above' | 'below') {
  const newRow = new Array(columnCount.value).fill('')
  const targetRow = selectedCell.value.row !== -1 ? selectedCell.value.row : 0
  const insertIndex = position === 'above' ? targetRow : targetRow + 1
  tableData.value.splice(insertIndex, 0, newRow)
  rowCount.value++
}

function deleteRow() {
  if (selectedCell.value.row === -1) {
    ElMessage.warning('请先选择一行')
    return
  }
  if (rowCount.value <= 1) {
    ElMessage.warning('表格至少需要一行')
    return
  }
  tableData.value.splice(selectedCell.value.row, 1)
  rowCount.value--
  selectedCell.value = { row: -1, col: -1 }
}

function insertColumn(position: 'left' | 'right') {
  const targetCol = selectedCell.value.col !== -1 ? selectedCell.value.col : 0
  const insertIndex = position === 'left' ? targetCol : targetCol + 1

  tableData.value.forEach(row => {
    row.splice(insertIndex, 0, '')
  })
  columnCount.value++
}

function deleteColumn() {
  if (selectedCell.value.col === -1) {
    ElMessage.warning('请先选择一列')
    return
  }
  if (columnCount.value <= 1) {
    ElMessage.warning('表格至少需要一列')
    return
  }

  tableData.value.forEach(row => {
    row.splice(selectedCell.value.col, 1)
  })
  columnCount.value--
  selectedCell.value = { row: -1, col: -1 }
}

function canMergeCells(): boolean {
  return selectedCell.value.row !== -1 && selectedCell.value.col !== -1
}

function canSplitCell(): boolean {
  return false // 需要更复杂的单元格合并状态跟踪
}

function mergeCells() {
  ElMessage.info('单元格合并功能开发中')
}

function splitCell() {
  ElMessage.info('单元格拆分功能开发中')
}

function updateTableSize() {
  initTableData()
}

function getColumnLabel(index: number): string {
  if (tableStyle.value === 'threeline' || tableStyle.value === 'academic') {
    return index === 0 ? '项目' : `列 ${index + 1}`
  }
  return `列 ${index + 1}`
}

function getColumnWidth(index: number): number {
  return 120
}

function updateTableStyle() {
  // 应用表格样式
}

function applyCellStyle() {
  if (selectedCell.value.row !== -1) {
    ElMessage.success('单元格样式已应用')
  }
}

// 生成LaTeX代码
function generateCode() {
  let code = ''

  // 表格环境
  if (useBooktabs.value) {
    code += '\\usepackage{booktabs}\n\n'
  }

  code += `\\begin{table}[${tablePosition.value}]\n`

  if (tableCaption.value) {
    code += `  \\caption{${tableCaption.value}}\n`
  }

  if (tableLabel.value) {
    code += `  \\label{${tableLabel.value}}\n`
  }

  code += '  \\begin{tabular}{'

  // 生成列格式
  const columns: string[] = []
  for (let i = 0; i < columnCount.value; i++) {
    if (tableStyle.value === 'threeline' || tableStyle.value === 'academic') {
      if (i === 0) {
        columns.push(useBooktabs.value ? 'l' : 'l')
      } else if (i === columnCount.value - 1) {
        columns.push(useBooktabs.value ? 'r' : 'r')
      } else {
        columns.push(useBooktabs.value ? 'c' : 'c')
      }
    } else {
      columns.push('c')
    }
  }
  code += columns.join(' ') + '\n'

  // 生成表格行
  tableData.value.forEach((row, rowIndex) => {
    code += '    '

    // 根据表格样式添加线
    if (tableStyle.value === 'threeline') {
      if (rowIndex === 0) {
        code += useBooktabs.value ? '\\toprule ' : '\\hline '
      } else if (rowIndex === tableData.value.length - 1) {
        code += useBooktabs.value ? '\\bottomrule ' : '\\hline '
      } else {
        code += useBooktabs.value ? '\\midrule ' : '\\hline '
      }
    } else if (tableStyle.value === 'standard') {
      code += '\\hline '
    }

    // 生成单元格内容
    code += row.join(' & ') + ' \\\\\n'
  })

  code += '  \\end{tabular}\n'
  code += '\\end{table}\n'

  generatedCode.value = code
  showCodePreview.value = true
}

function copyCode() {
  navigator.clipboard.writeText(generatedCode.value)
  ElMessage.success('代码已复制到剪贴板')
}

function insertCode() {
  emit('insert', generatedCode.value)
  showCodePreview.value = false
  ElMessage.success('代码已插入到编辑器')
}

// 初始化
initTableData()
</script>

<style scoped lang="scss">
.table-editor {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.editor-toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px;
  background: var(--el-fill-color-blank);
  border-bottom: 1px solid var(--el-border-color);
}

.editor-content {
  flex: 1;
  overflow: auto;
  padding: 20px;
}

.editable-table {
  width: 100%;

  .el-table__cell {
    padding: 0;
  }
}

.table-cell {
  width: 100%;
  height: 40px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  position: relative;

  &:hover {
    background: var(--el-fill-color-light);
  }

  &.is-selected {
    background: var(--el-color-primary-light-9);
  }

  &.is-header {
    font-weight: 600;
    background: var(--el-fill-color);
  }

  &.is-focused {
    box-shadow: inset 0 0 0 2px var(--el-color-primary);
  }

  .cell-input {
    width: 100%;
    height: 100%;
    border: none;
    background: var(--el-color-primary-light-9);
    text-align: center;
    font-family: inherit;
    font-size: inherit;
    outline: none;
  }
}

.table-properties {
  padding: 16px;
  background: var(--el-fill-color-blank);
  border-top: 1px solid var(--el-border-color);
  max-height: 300px;
  overflow-y: auto;
}

.code-preview {
  font-family: 'Courier New', monospace;
  font-size: 13px;
}
</style>
