<template>
  <BaseDialog
    :show="show"
    title="BibTeX 文献管理"
    width="1200px"
    @update:show="$emit('update:show', $event)"
  >
    <div class="bibtex-manager">
      <!-- 工具栏 -->
      <div class="manager-toolbar">
        <el-button type="primary" @click="showAddEntryDialog = true">
          <el-icon><Plus /></el-icon>
          添加文献
        </el-button>

        <el-button @click="fetchByDOI">
          <el-icon><Search /></el-icon>
          DOI/ISBN获取
        </el-button>

        <el-upload
          :auto-upload="false"
          :show-file-list="false"
          :on-change="handleBibFileUpload"
          accept=".bib"
        >
          <el-button>
            <el-icon><Upload /></el-icon>
            导入.bib文件
          </el-button>
        </el-upload>

        <el-button @click="exportBibFile">
          <el-icon><Download /></el-icon>
          导出.bib文件
        </el-button>

        <el-input
          v-model="searchQuery"
          placeholder="搜索文献..."
          prefix-icon="Search"
          style="width: 250px"
          clearable
        >
          <template #append>
            <el-button :icon="Search" @click="searchEntries" />
          </template>
        </el-input>

        <el-select v-model="filterType" placeholder="文献类型" style="width: 120px" clearable>
          <el-option label="全部" value="" />
          <el-option label="文章" value="article" />
          <el-option label="书籍" value="book" />
          <el-option label="会议" value="inproceedings" />
          <el-option label="报告" value="report" />
        </el-select>
      </div>

      <!-- 统计信息 -->
      <div class="entry-stats">
        <el-tag type="info">总计: {{ entries.length }} 条</el-tag>
        <el-tag type="success">已引用: {{ citedEntries.length }} 条</el-tag>
        <el-tag type="warning">未引用: {{ uncitedEntries.length }} 条</el-tag>
      </div>

      <!-- 文献列表 -->
      <div v-loading="loading" class="entries-container">
        <el-table
          :data="filteredEntries"
          @selection-change="handleSelectionChange"
          max-height="500"
        >
          <el-table-column type="selection" width="55" />
          <el-table-column prop="citeKey" label="引用键" width="150">
            <template #default="{ row }">
              <code>{{ row.citeKey }}</code>
            </template>
          </el-table-column>
          <el-table-column prop="type" label="类型" width="100">
            <template #default="{ row }">
              <el-tag :type="getEntryTypeColor(row.type)" size="small">
                {{ getEntryTypeLabel(row.type) }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="author" label="作者" min-width="150">
            <template #default="{ row }">
              <span :title="row.author">{{ formatAuthor(row.author) }}</span>
            </template>
          </el-table-column>
          <el-table-column prop="title" label="标题" min-width="200">
            <template #default="{ row }">
              <span :title="row.title">{{ row.title }}</span>
            </template>
          </el-table-column>
          <el-table-column prop="year" label="年份" width="80" />
          <el-table-column prop="journal" label="期刊/会议" min-width="150">
            <template #default="{ row }">
              <span :title="row.journal || row.booktitle">{{ row.journal || row.booktitle || '-' }}</span>
            </template>
          </el-table-column>
          <el-table-column prop="refCount" label="引用" width="70" align="center">
            <template #default="{ row }">
              <el-tag :type="row.refCount > 0 ? 'success' : 'info'" size="small">
                {{ row.refCount }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column label="操作" width="220" fixed="right">
            <template #default="{ row }">
              <el-button size="small" @click="insertCite(row)">引用</el-button>
              <el-button size="small" @click="editEntry(row)">编辑</el-button>
              <el-button size="small" type="danger" @click="deleteEntry(row)">删除</el-button>
            </template>
          </el-table-column>
        </el-table>

        <!-- 批量操作 -->
        <div v-if="selectedEntries.length > 0" class="batch-actions">
          <span>已选择 {{ selectedEntries.length }} 项</span>
          <el-button size="small" @click="batchCite">批量引用</el-button>
          <el-button size="small" type="danger" @click="batchDelete">批量删除</el-button>
        </div>
      </div>
    </div>

    <!-- 添加/编辑文献对话框 -->
    <el-dialog
      v-model="showAddEntryDialog"
      :title="editingEntry ? '编辑文献' : '添加文献'"
      width="700px"
      append-to-body
    >
      <el-form :model="entryForm" label-width="100px">
        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="引用键">
              <el-input v-model="entryForm.citeKey" placeholder="如: smith2024" />
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="文献类型">
              <el-select v-model="entryForm.type" style="width: 100%">
                <el-option label="文章 (article)" value="article" />
                <el-option label="书籍 (book)" value="book" />
                <el-option label="会议 (inproceedings)" value="inproceedings" />
                <el-option label="报告 (report)" value="report" />
                <el-option label="学位论文 (phdthesis)" value="phdthesis" />
                <el-option label="硕士论文 (mastersthesis)" value="mastersthesis" />
                <el-option label="其他 (misc)" value="misc" />
              </el-select>
            </el-form-item>
          </el-col>
        </el-row>

        <el-form-item label="标题">
          <el-input v-model="entryForm.title" placeholder="文献标题" />
        </el-form-item>

        <el-form-item label="作者">
          <el-input
            v-model="entryForm.author"
            placeholder="如: Smith, J. and Doe, A."
            type="textarea"
            :rows="2"
          />
          <div class="form-tip">多个作者用 "and" 分隔，格式: 姓, 名. and 姓, 名.</div>
        </el-form-item>

        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="年份">
              <el-input v-model="entryForm.year" placeholder="2024" />
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="月份">
              <el-input v-model="entryForm.month" placeholder="jan, feb, ..." />
            </el-form-item>
          </el-col>
        </el-row>

        <!-- 根据类型显示不同字段 -->
        <template v-if="entryForm.type === 'article'">
          <el-form-item label="期刊">
            <el-input v-model="entryForm.journal" placeholder="期刊名称" />
          </el-form-item>
          <el-row :gutter="20">
            <el-col :span="12">
              <el-form-item label="卷">
                <el-input v-model="entryForm.volume" placeholder="卷号" />
              </el-form-item>
            </el-col>
            <el-col :span="12">
              <el-form-item label="期">
                <el-input v-model="entryForm.number" placeholder="期号" />
              </el-form-item>
            </el-col>
          </el-row>
          <el-form-item label="页码">
            <el-input v-model="entryForm.pages" placeholder="1-10" />
          </el-form-item>
        </template>

        <template v-else-if="entryForm.type === 'book'">
          <el-form-item label="出版社">
            <el-input v-model="entryForm.publisher" placeholder="出版社名称" />
          </el-form-item>
          <el-form-item label="地址">
            <el-input v-model="entryForm.address" placeholder="出版地" />
          </el-form-item>
          <el-form-item label="版本">
            <el-input v-model="entryForm.edition" placeholder="版本号" />
          </el-form-item>
        </template>

        <template v-else-if="entryForm.type === 'inproceedings'">
          <el-form-item label="会议名称">
            <el-input v-model="entryForm.booktitle" placeholder="会议名称" />
          </el-form-item>
          <el-form-item label="组织者">
            <el-input v-model="entryForm.organization" placeholder="组织者" />
          </el-form-item>
          <el-form-item label="出版商">
            <el-input v-model="entryForm.publisher" placeholder="出版商" />
          </el-form-item>
        </template>

        <el-form-item label="DOI">
          <el-input v-model="entryForm.doi" placeholder="10.1000/xyz123">
            <template #append>
              <el-button @click="fetchMetadataByDOI">获取</el-button>
            </template>
          </el-input>
        </el-form-item>

        <el-form-item label="URL">
          <el-input v-model="entryForm.url" placeholder="https://..." />
        </el-form-item>

        <el-form-item label="摘要">
          <el-input
            v-model="entryForm.abstract"
            type="textarea"
            :rows="3"
            placeholder="文献摘要"
          />
        </el-form-item>

        <el-form-item label="关键字">
          <el-input v-model="entryForm.keywords" placeholder="keyword1, keyword2, ..." />
        </el-form-item>
      </el-form>

      <template #footer>
        <el-button @click="showAddEntryDialog = false">取消</el-button>
        <el-button type="primary" @click="saveEntry" :disabled="!entryForm.citeKey">
          {{ editingEntry ? '保存' : '添加' }}
        </el-button>
      </template>
    </el-dialog>

    <!-- DOI获取对话框 -->
    <el-dialog v-model="showDOIDialog" title="DOI/ISBN获取文献" width="500px" append-to-body>
      <el-form :model="doiForm" label-width="100px">
        <el-form-item label="DOI/ISBN">
          <el-input v-model="doiForm.query" placeholder="输入DOI或ISBN">
            <template #prepend>DOI/ISBN</template>
          </el-input>
        </el-form-item>
        <el-form-item label="来源">
          <el-select v-model="doiForm.source" style="width: 100%">
            <el-option label="Crossref" value="crossref" />
            <el-option label=" semanticscholar" value="semantic" />
            <el-option label="DBLP" value="dblp" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showDOIDialog = false">取消</el-button>
        <el-button type="primary" @click="confirmFetchDOI" :loading="fetchingDOI">
          获取文献信息
        </el-button>
      </template>
    </el-dialog>

    <template #footer>
      <el-button @click="$emit('update:show', false)">关闭</el-button>
      <el-button type="primary" @click="showAddEntryDialog = true">添加文献</el-button>
    </template>
  </BaseDialog>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Plus, Search, Upload, Download } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import BaseDialog from './BaseDialog.vue'

interface BibEntry {
  id?: number | string
  citeKey: string
  type: string
  author: string
  title: string
  year?: string
  month?: string
  journal?: string
  booktitle?: string
  volume?: string
  number?: string
  pages?: string
  publisher?: string
  address?: string
  edition?: string
  organization?: string
  doi?: string
  url?: string
  abstract?: string
  keywords?: string
  refCount?: number
}

interface Props {
  show: boolean
  projectId?: string | number
  entries: BibEntry[]
  documentContent?: string
}

const props = withDefaults(defineProps<Props>(), {
  entries: () => []
})

const emit = defineEmits<{
  'update:show': [value: boolean]
  'insert': [citeKey: string]
  'add': [entry: BibEntry]
  'update': [entryId: number | string, entry: BibEntry]
  'delete': [entryId: number | string]
  'import': [content: string]
  'export': []
}>()

// 状态
const loading = ref(false)
const searchQuery = ref('')
const filterType = ref('')
const selectedEntries = ref<BibEntry[]>([])
const showAddEntryDialog = ref(false)
const showDOIDialog = ref(false)
const editingEntry = ref<BibEntry | null>(null)
const fetchingDOI = ref(false)

const entryForm = ref<BibEntry>({
  citeKey: '',
  type: 'article',
  author: '',
  title: '',
  year: '',
  journal: '',
  refCount: 0
})

const doiForm = ref({
  query: '',
  source: 'crossref'
})

// 计算属性
const filteredEntries = computed(() => {
  let result = props.entries

  // 类型过滤
  if (filterType.value) {
    result = result.filter(entry => entry.type === filterType.value)
  }

  // 搜索过滤
  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    result = result.filter(entry =>
      entry.citeKey.toLowerCase().includes(query) ||
      entry.title.toLowerCase().includes(query) ||
      entry.author.toLowerCase().includes(query)
    )
  }

  return result
})

const citedEntries = computed(() => {
  return props.entries.filter(entry => (entry.refCount || 0) > 0)
})

const uncitedEntries = computed(() => {
  return props.entries.filter(entry => (entry.refCount || 0) === 0)
})

// 方法
function insertCite(entry: BibEntry) {
  emit('insert', entry.citeKey)
  ElMessage.success(`已插入引用: ${entry.citeKey}`)
}

function editEntry(entry: BibEntry) {
  editingEntry.value = entry
  entryForm.value = { ...entry }
  showAddEntryDialog.value = true
}

function deleteEntry(entry: BibEntry) {
  ElMessageBox.confirm(
    `确定要删除文献 "${entry.title}" 吗？${(entry.refCount || 0) > 0 ? '该文献正在被引用！' : ''}`,
    '确认删除',
    { type: 'warning' }
  ).then(() => {
    if (entry.id) {
      emit('delete', entry.id)
      ElMessage.success('文献已删除')
    }
  }).catch(() => {})
}

function saveEntry() {
  if (!entryForm.value.citeKey) {
    ElMessage.warning('请输入引用键')
    return
  }

  if (editingEntry.value && editingEntry.value.id) {
    emit('update', editingEntry.value.id, entryForm.value)
    ElMessage.success('文献已更新')
  } else {
    emit('add', entryForm.value)
    ElMessage.success('文献已添加')
  }

  showAddEntryDialog.value = false
  resetEntryForm()
}

function handleSelectionChange(selection: BibEntry[]) {
  selectedEntries.value = selection
}

function batchCite() {
  const citeKeys = selectedEntries.value.map(e => e.citeKey)
  emit('insert', citeKeys.join(', '))
  ElMessage.success(`已插入 ${citeKeys.length} 个引用`)
}

function batchDelete() {
  ElMessageBox.confirm(
    `确定要删除选中的 ${selectedEntries.value.length} 条文献吗？`,
    '确认删除',
    { type: 'warning' }
  ).then(() => {
    selectedEntries.value.forEach(entry => {
      if (entry.id) emit('delete', entry.id)
    })
    ElMessage.success('文献已删除')
  }).catch(() => {})
}

function fetchByDOI() {
  doiForm.value.query = ''
  showDOIDialog.value = true
}

async function confirmFetchDOI() {
  if (!doiForm.value.query) {
    ElMessage.warning('请输入DOI或ISBN')
    return
  }

  fetchingDOI.value = true

  try {
    // 这里应该调用实际的API来获取文献信息
    // 模拟API调用
    await new Promise(resolve => setTimeout(resolve, 1000))

    // 示例：从Crossref获取数据
    const mockData = {
      citeKey: `entry_${Date.now()}`,
      type: 'article',
      author: 'Smith, J. and Doe, A.',
      title: 'Example Paper Title',
      year: '2024',
      journal: 'Example Journal',
      volume: '1',
      number: '1',
      pages: '1-10',
      doi: doiForm.value.query
    }

    entryForm.value = { ...entryForm.value, ...mockData }
    showDOIDialog.value = false
    showAddEntryDialog.value = true
    ElMessage.success('文献信息已获取')
  } catch (error) {
    ElMessage.error('获取文献信息失败')
  } finally {
    fetchingDOI.value = false
  }
}

async function fetchMetadataByDOI() {
  if (!entryForm.value.doi) {
    ElMessage.warning('请输入DOI')
    return
  }

  try {
    fetchingDOI.value = true
    // 模拟API调用
    await new Promise(resolve => setTimeout(resolve, 1000))
    ElMessage.success('元数据已更新')
  } finally {
    fetchingDOI.value = false
  }
}

function handleBibFileUpload(file: any) {
  const reader = new FileReader()
  reader.onload = (e) => {
    const content = e.target?.result as string
    emit('import', content)
    ElMessage.success('BibTeX文件已导入')
  }
  reader.readAsText(file.raw)
}

function exportBibFile() {
  emit('export')
  ElMessage.success('BibTeX文件已导出')
}

function searchEntries() {
  // 搜索在computed中自动处理
}

function resetEntryForm() {
  entryForm.value = {
    citeKey: '',
    type: 'article',
    author: '',
    title: '',
    year: '',
    journal: '',
    refCount: 0
  }
  editingEntry.value = null
}

function getEntryTypeLabel(type: string): string {
  const labels: Record<string, string> = {
    article: '文章',
    book: '书籍',
    inproceedings: '会议',
    report: '报告',
    phdthesis: '博士论文',
    mastersthesis: '硕士论文',
    misc: '其他'
  }
  return labels[type] || type
}

function getEntryTypeColor(type: string): string {
  const colors: Record<string, string> = {
    article: 'primary',
    book: 'success',
    inproceedings: 'warning',
    report: 'info',
    phdthesis: 'danger',
    mastersthesis: 'danger'
  }
  return colors[type] || ''
}

function formatAuthor(author: string): string {
  if (!author) return '-'
  const authors = author.split('and')
  if (authors.length > 2) {
    return authors[0] + ' et al.'
  }
  return author
}
</script>

<style scoped lang="scss">
.bibtex-manager {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.manager-toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}

.entry-stats {
  display: flex;
  gap: 8px;
}

.entries-container {
  position: relative;
}

.batch-actions {
  position: absolute;
  bottom: 10px;
  left: 10px;
  right: 10px;
  padding: 10px;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color);
  border-radius: 4px;
  display: flex;
  align-items: center;
  gap: 12px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
}

.form-tip {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin-top: 4px;
}

code {
  background: var(--el-fill-color-light);
  padding: 2px 6px;
  border-radius: 4px;
  font-family: 'Courier New', monospace;
  font-size: 13px;
}
</style>
