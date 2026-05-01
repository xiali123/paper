<template>
  <div class="bibtex-manager">
    <!-- 工具栏 -->
    <div class="manager-toolbar">
      <div class="toolbar-left">
        <el-input
          v-model="searchQuery"
          placeholder="搜索文献..."
          :prefix-icon="Search"
          clearable
          style="width: 300px"
          @input="handleSearch"
        />
        <el-select v-model="filterType" placeholder="筛选类型" clearable style="width: 150px">
          <el-option label="全部" value="" />
          <el-option label="文章" value="article" />
          <el-option label="书籍" value="book" />
          <el-option label="会议" value="inproceedings" />
          <el-option label="论文集" value="incollection" />
          <el-option label="博士论文" value="phdthesis" />
          <el-option label="硕士论文" value="mastersthesis" />
        </el-select>
      </div>
      <div class="toolbar-right">
        <el-button-group>
          <el-button :icon="Upload" @click="handleImport">导入</el-button>
          <el-button :icon="Download" @click="handleExport" :disabled="selectedEntries.size === 0">
            导出
          </el-button>
          <el-button :icon="Delete" @click="handleDelete" :disabled="selectedEntries.size === 0">
            删除
          </el-button>
        </el-button-group>
        <el-button :icon="Plus" type="primary" @click="handleCreateNew">新建文献</el-button>
      </div>
    </div>

    <!-- 统计栏 -->
    <div class="stats-bar">
      <el-tag size="small">总计 {{ entries.length }} 条</el-tag>
      <el-tag size="small" type="success">已选择 {{ selectedEntries.size }} 条</el-tag>
      <el-tag size="small" type="warning">{{ duplicates.length }} 条重复</el-tag>
      <el-button size="small" text @click="showDuplicates = true" v-if="duplicates.length > 0">
        查看重复
      </el-button>
    </div>

    <!-- 文献列表 -->
    <div class="entries-container">
      <div
        v-for="entry in displayedEntries"
        :key="entry.id"
        class="entry-card"
        :class="{
          'is-selected': selectedEntries.has(entry.id),
          'has-warning': entry.warnings.length > 0,
          'has-error': entry.errors.length > 0
        }"
        @click="selectEntry(entry)"
        @dblclick="editEntry(entry)"
      >
        <div class="entry-header">
          <el-checkbox
            :model-value="selectedEntries.has(entry.id)"
            @change="toggleSelect(entry, $event)"
            @click.stop
          />
          <el-tag :type="getTypeColor(entry.type)" size="small">
            {{ entry.type }}
          </el-tag>
          <span class="entry-id">{{ entry.id }}</span>
          <div class="entry-actions">
            <el-button size="small" text @click.stop="insertCitation(entry)">
              <el-icon><Plus /></el-icon>
              引用
            </el-button>
            <el-button size="small" text @click.stop="editEntry(entry)">
              <el-icon><Edit /></el-icon>
            </el-button>
            <el-button size="small" text type="danger" @click.stop="deleteEntry(entry)">
              <el-icon><Delete /></el-icon>
            </el-button>
          </div>
        </div>

        <div class="entry-content">
          <div class="entry-title">{{ entry.fields.get('title') || '无标题' }}</div>
          <div class="entry-meta">
            <span class="entry-author">{{ formatAuthor(entry.fields.get('author')) }}</span>
            <span class="entry-year">{{ entry.fields.get('year') || '' }}</span>
          </div>
          <div v-if="entry.fields.get('journal')" class="entry-journal">
            {{ entry.fields.get('journal') }}
          </div>
        </div>

        <!-- 警告和错误 -->
        <div v-if="entry.warnings.length > 0 || entry.errors.length > 0" class="entry-issues">
          <el-tag
            v-for="(error, idx) in entry.errors"
            :key="'err-' + idx"
            size="small"
            type="danger"
          >
            {{ error }}
          </el-tag>
          <el-tag
            v-for="(warning, idx) in entry.warnings"
            :key="'warn-' + idx"
            size="small"
            type="warning"
          >
            {{ warning }}
          </el-tag>
        </div>
      </div>

      <el-empty v-if="displayedEntries.length === 0" description="暂无文献数据">
        <el-button type="primary" @click="handleImport">导入BibTeX文件</el-button>
      </el-empty>
    </div>

    <!-- 编辑对话框 -->
    <el-dialog
      v-model="showEditDialog"
      :title="editingEntry ? '编辑文献' : '新建文献'"
      width="700px"
      @close="handleDialogClose"
    >
      <el-form ref="entryFormRef" :model="currentEntryData" label-width="120px">
        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="文献ID" prop="id">
              <el-input v-model="currentEntryData.id" placeholder="例如: smith2024" />
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="文献类型" prop="type">
              <el-select v-model="currentEntryData.type" style="width: 100%">
                <el-option label="文章" value="article" />
                <el-option label="书籍" value="book" />
                <el-option label="会议" value="inproceedings" />
                <el-option label="论文集" value="incollection" />
                <el-option label="博士论文" value="phdthesis" />
                <el-option label="硕士论文" value="mastersthesis" />
                <el-option label="技术报告" value="techreport" />
                <el-option label="未发表" value="unpublished" />
                <el-option label="其他" value="misc" />
              </el-select>
            </el-form-item>
          </el-col>
        </el-row>

        <el-form-item label="标题" prop="title">
          <el-input v-model="currentEntryData.title" type="textarea" :rows="2" />
        </el-form-item>

        <el-form-item label="作者" prop="author">
          <el-input v-model="currentEntryData.author" type="textarea" :rows="2"
            placeholder="作者之间用 and 分隔，例如: John Doe and Jane Smith" />
        </el-form-item>

        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="年份">
              <el-input v-model="currentEntryData.year" placeholder="2024" />
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="期刊/出版物">
              <el-input v-model="currentEntryData.journal" />
            </el-form-item>
          </el-col>
        </el-row>

        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="卷">
              <el-input v-model="currentEntryData.volume" />
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="期">
              <el-input v-model="currentEntryData.number" />
            </el-form-item>
          </el-col>
        </el-row>

        <el-row :gutter="20">
          <el-col :span="12">
            <el-form-item label="页码">
              <el-input v-model="currentEntryData.pages" placeholder="1-10" />
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="DOI">
              <el-input v-model="currentEntryData.doi" placeholder="10.1000/..." />
            </el-form-item>
          </el-col>
        </el-row>

        <el-form-item label="摘要">
          <el-input v-model="currentEntryData.abstract" type="textarea" :rows="3" />
        </el-form-item>

        <el-form-item label="关键字">
          <el-input v-model="currentEntryData.keywords" />
        </el-form-item>

        <el-form-item label="URL">
          <el-input v-model="currentEntryData.url" />
        </el-form-item>
      </el-form>

      <template #footer>
        <el-button @click="showEditDialog = false">取消</el-button>
        <el-button type="primary" @click="saveEntry" :loading="saving">
          保存
        </el-button>
      </template>
    </el-dialog>

    <!-- 重复项对话框 -->
    <el-dialog v-model="showDuplicates" title="重复文献" width="800px">
      <div class="duplicates-list">
        <div
          v-for="dup in duplicates"
          :key="dup.entry.id"
          class="duplicate-item"
        >
          <div class="duplicate-info">
            <div class="duplicate-title">{{ dup.entry.fields.get('title') }}</div>
            <div class="duplicate-id">重复于: {{ dup.duplicateOf }}</div>
          </div>
          <el-button size="small" type="danger" @click="removeDuplicate(dup)">
            移除
          </el-button>
        </div>
      </div>
    </el-dialog>

    <!-- 隐藏的文件输入 -->
    <input
      ref="fileInputRef"
      type="file"
      accept=".bib,.txt"
      style="display: none"
      @change="handleFileSelect"
    >
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from 'vue'
import { Search, Upload, Download, Delete, Plus, Edit } from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  parseBibTeX,
  generateBibTeXEntry,
  validateBibTeXEntry,
  formatAuthor,
  createCitationCommand,
  deduplicateEntries,
  type BibTeXEntry
} from '@/utils/bibtexParser'

interface Props {
  documentId?: string
}

interface Emits {
  (e: 'insert-citation', command: string): void
}

const props = defineProps<Props>()
const emit = defineEmits<Emits>()

// 状态
const entries = ref<Array<BibTeXEntry & { warnings: string[]; errors: string[] }>>([])
const selectedEntries = ref<Set<string>>(new Set())
const searchQuery = ref('')
const filterType = ref('')
const duplicates = ref<Array<{ entry: BibTeXEntry; duplicateOf: string }>>([])

// 编辑对话框
const showEditDialog = ref(false)
const showDuplicates = ref(false)
const editingEntry = ref<BibTeXEntry | null>(null)
const saving = ref(false)
const entryFormRef = ref()

// 表单数据
const currentEntryData = ref({
  id: '',
  type: 'article',
  title: '',
  author: '',
  year: '',
  journal: '',
  booktitle: '',
  volume: '',
  number: '',
  pages: '',
  doi: '',
  abstract: '',
  keywords: '',
  url: '',
  publisher: ''
})

// 文件输入
const fileInputRef = ref<HTMLInputElement>()

// 计算属性
const displayedEntries = computed(() => {
  let filtered = entries.value

  // 类型筛选
  if (filterType.value) {
    filtered = filtered.filter(e => e.type === filterType.value)
  }

  // 搜索筛选
  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    filtered = filtered.filter(entry => {
      if (entry.id.toLowerCase().includes(query)) return true
      if (entry.fields.get('title')?.toLowerCase().includes(query)) return true
      if (entry.fields.get('author')?.toLowerCase().includes(query)) return true
      return false
    })
  }

  return filtered
})

// 方法
const loadEntries = () => {
  const saved = localStorage.getItem('bibtex_entries')
  if (saved) {
    try {
      const parsed = JSON.parse(saved)
      entries.value = parsed.map((e: any) => {
        const validation = validateBibTeXEntry(e)
        return {
          ...e,
          fields: new Map(Object.entries(e.fields)),
          warnings: validation.warnings,
          errors: validation.errors
        }
      })
      checkDuplicates()
    } catch (error) {
      console.error('Failed to load BibTeX entries:', error)
    }
  }
}

const saveEntries = () => {
  const toSave = entries.value.map(e => ({
    ...e,
    fields: Object.fromEntries(e.fields)
  }))
  localStorage.setItem('bibtex_entries', JSON.stringify(toSave))
}

const checkDuplicates = () => {
  const result = deduplicateEntries(entries.value)
  duplicates.value = result.duplicates
}

const handleSearch = () => {
  // 搜索由computed自动处理
}

const handleImport = () => {
  fileInputRef.value?.click()
}

const handleFileSelect = async (event: Event) => {
  const target = event.target as HTMLInputElement
  const file = target.files?.[0]
  if (!file) return

  try {
    const content = await file.text()
    const parsed = parseBibTeX(content)

    entries.value = [
      ...entries.value,
      ...parsed.map(entry => {
        const validation = validateBibTeXEntry(entry)
        return {
          ...entry,
          warnings: validation.warnings,
          errors: validation.errors
        }
      })
    ]

    saveEntries()
    checkDuplicates()

    ElMessage.success(`成功导入 ${parsed.length} 条文献`)
  } catch (error) {
    ElMessage.error('导入失败: ' + error)
  }

  // 清空input以允许重新选择同一文件
  target.value = ''
}

const handleExport = () => {
  if (selectedEntries.value.size === 0) return

  const selected = entries.value.filter(e => selectedEntries.value.has(e.id))
  const content = selected.map(e => generateBibTeXEntry(e)).join('\n\n')

  const blob = new Blob([content], { type: 'text/plain' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = 'references.bib'
  link.click()
  URL.revokeObjectURL(url)

  ElMessage.success('导出成功')
}

const handleDelete = async () => {
  if (selectedEntries.value.size === 0) return

  try {
    await ElMessageBox.confirm(
      `确定删除选中的 ${selectedEntries.value.size} 条文献吗？`,
      '确认删除',
      { type: 'warning' }
    )

    entries.value = entries.value.filter(e => !selectedEntries.value.has(e.id))
    selectedEntries.value.clear()

    saveEntries()
    checkDuplicates()

    ElMessage.success('删除成功')
  } catch {
    // 用户取消
  }
}

const handleCreateNew = () => {
  editingEntry.value = null
  currentEntryData.value = {
    id: '',
    type: 'article',
    title: '',
    author: '',
    year: '',
    journal: '',
    booktitle: '',
    volume: '',
    number: '',
    pages: '',
    doi: '',
    abstract: '',
    keywords: '',
    url: '',
    publisher: ''
  }
  showEditDialog.value = true
}

const editEntry = (entry: BibTeXEntry) => {
  editingEntry.value = entry
  currentEntryData.value = {
    id: entry.id,
    type: entry.type,
    title: entry.fields.get('title') || '',
    author: entry.fields.get('author') || '',
    year: entry.fields.get('year') || '',
    journal: entry.fields.get('journal') || '',
    booktitle: entry.fields.get('booktitle') || '',
    volume: entry.fields.get('volume') || '',
    number: entry.fields.get('number') || '',
    pages: entry.fields.get('pages') || '',
    doi: entry.fields.get('doi') || '',
    abstract: entry.fields.get('abstract') || '',
    keywords: entry.fields.get('keywords') || '',
    url: entry.fields.get('url') || '',
    publisher: entry.fields.get('publisher') || ''
  }
  showEditDialog.value = true
}

const saveEntry = () => {
  if (!currentEntryData.value.id) {
    ElMessage.warning('请输入文献ID')
    return
  }

  saving.value = true

  setTimeout(() => {
    const fields = new Map<string, string>()
    if (currentEntryData.value.title) fields.set('title', currentEntryData.value.title)
    if (currentEntryData.value.author) fields.set('author', currentEntryData.value.author)
    if (currentEntryData.value.year) fields.set('year', currentEntryData.value.year)
    if (currentEntryData.value.journal) fields.set('journal', currentEntryData.value.journal)
    if (currentEntryData.value.booktitle) fields.set('booktitle', currentEntryData.value.booktitle)
    if (currentEntryData.value.volume) fields.set('volume', currentEntryData.value.volume)
    if (currentEntryData.value.number) fields.set('number', currentEntryData.value.number)
    if (currentEntryData.value.pages) fields.set('pages', currentEntryData.value.pages)
    if (currentEntryData.value.doi) fields.set('doi', currentEntryData.value.doi)
    if (currentEntryData.value.abstract) fields.set('abstract', currentEntryData.value.abstract)
    if (currentEntryData.value.keywords) fields.set('keywords', currentEntryData.value.keywords)
    if (currentEntryData.value.url) fields.set('url', currentEntryData.value.url)
    if (currentEntryData.value.publisher) fields.set('publisher', currentEntryData.value.publisher)

    const newEntry: BibTeXEntry = {
      id: currentEntryData.value.id,
      type: currentEntryData.value.type,
      fields,
      raw: ''
    }

    const validation = validateBibTeXEntry(newEntry)

    if (editingEntry.value) {
      // 更新
      const index = entries.value.findIndex(e => e.id === editingEntry.value!.id)
      if (index >= 0) {
        entries.value[index] = {
          ...newEntry,
          warnings: validation.warnings,
          errors: validation.errors
        }
      }
    } else {
      // 新建
      entries.value.unshift({
        ...newEntry,
        warnings: validation.warnings,
        errors: validation.errors
      })
    }

    saveEntries()
    checkDuplicates()

    showEditDialog.value = false
    saving.value = false

    ElMessage.success(editingEntry.value ? '更新成功' : '创建成功')
  }, 500)
}

const handleDialogClose = () => {
  editingEntry.value = null
}

const selectEntry = (entry: BibTeXEntry) => {
  // 单击不执行任何操作，由复选框处理
}

const toggleSelect = (entry: BibTeXEntry, checked: boolean) => {
  if (checked) {
    selectedEntries.value.add(entry.id)
  } else {
    selectedEntries.value.delete(entry.id)
  }
}

const deleteEntry = async (entry: BibTeXEntry) => {
  try {
    await ElMessageBox.confirm(
      `确定删除文献 "${entry.id}" 吗？`,
      '确认删除',
      { type: 'warning' }
    )

    entries.value = entries.value.filter(e => e.id !== entry.id)
    selectedEntries.value.delete(entry.id)

    saveEntries()
    checkDuplicates()

    ElMessage.success('删除成功')
  } catch {
    // 用户取消
  }
}

const removeDuplicate = (dup: { entry: BibTeXEntry; duplicateOf: string }) => {
  entries.value = entries.value.filter(e => e.id !== dup.entry.id)
  saveEntries()
  checkDuplicates()
  ElMessage.success('已移除重复项')
}

const insertCitation = (entry: BibTeXEntry) => {
  const command = createCitationCommand(entry.id)
  emit('insert-citation', command)
  ElMessage.success(`已插入引用命令: ${command}`)
}

const getTypeColor = (type: string) => {
  const colors: Record<string, any> = {
    article: '',
    book: 'success',
    inproceedings: 'warning',
    incollection: 'info',
    phdthesis: 'danger',
    mastersthesis: 'danger',
    techreport: 'info',
    unpublished: 'info',
    misc: ''
  }
  return colors[type] || ''
}

const formatAuthor = (author?: string): string => {
  if (!author) return '未知作者'
  return author.split('and')[0].trim() +
    (author.split('and').length > 1 ? ' 等' : '')
}

// 生命周期
onMounted(() => {
  loadEntries()
})

// 监听props变化
watch(() => props.documentId, () => {
  loadEntries()
})
</script>

<style scoped lang="scss">
.bibtex-manager {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #f5f7fa;

  .manager-toolbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 16px;
    background: #fff;
    border-bottom: 1px solid #e4e7ed;

    .toolbar-left,
    .toolbar-right {
      display: flex;
      align-items: center;
      gap: 12px;
    }
  }

  .stats-bar {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 16px;
    background: #fff;
    border-bottom: 1px solid #e4e7ed;
  }

  .entries-container {
    flex: 1;
    overflow-y: auto;
    padding: 16px;

    .entry-card {
      background: #fff;
      border-radius: 8px;
      padding: 16px;
      margin-bottom: 12px;
      border: 2px solid transparent;
      cursor: pointer;
      transition: all 0.2s;

      &:hover {
        border-color: #c0c4cc;
        box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
      }

      &.is-selected {
        border-color: #409eff;
        background: #ecf5ff;
      }

      &.has-warning {
        border-left: 4px solid #e6a23c;
      }

      &.has-error {
        border-left: 4px solid #f56c6c;
      }

      .entry-header {
        display: flex;
        align-items: center;
        gap: 12px;
        margin-bottom: 12px;

        .entry-id {
          flex: 1;
          font-family: monospace;
          font-weight: 500;
          color: #303133;
        }

        .entry-actions {
          display: flex;
          gap: 4px;
          opacity: 0;
          transition: opacity 0.2s;
        }
      }

      &:hover .entry-actions {
        opacity: 1;
      }

      .entry-content {
        .entry-title {
          font-size: 16px;
          font-weight: 500;
          color: #303133;
          margin-bottom: 8px;
        }

        .entry-meta {
          display: flex;
          gap: 12px;
          font-size: 14px;
          color: #606266;
          margin-bottom: 4px;

          .entry-author {
            font-weight: 500;
          }
        }

        .entry-journal {
          font-size: 14px;
          color: #909399;
          font-style: italic;
        }
      }

      .entry-issues {
        margin-top: 12px;
        display: flex;
        flex-wrap: wrap;
        gap: 6px;
      }
    }
  }

  .duplicates-list {
    .duplicate-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 12px;
      border-bottom: 1px solid #e4e7ed;

      &:last-child {
        border-bottom: none;
      }

      .duplicate-info {
        .duplicate-title {
          font-weight: 500;
          color: #303133;
          margin-bottom: 4px;
        }

        .duplicate-id {
          font-size: 12px;
          color: #909399;
        }
      }
    }
  }
}

// Dark mode overrides
[data-theme="dark"] {
  .bibtex-manager {
    background: rgba(30, 30, 35, 0.95);

    .manager-toolbar {
      background: rgba(40, 40, 45, 0.98);
      border-bottom-color: rgba(102, 126, 234, 0.3);
    }

    .stats-bar {
      background: rgba(40, 40, 45, 0.98);
      border-bottom-color: rgba(102, 126, 234, 0.3);
    }

    .entries-container {
      .entry-card {
        background: rgba(40, 40, 45, 0.98);
        box-shadow: 0 2px 12px rgba(0, 0, 0, 0.3);

        &:hover {
          border-color: rgba(102, 126, 234, 0.3);
        }

        &.is-selected {
          border-color: var(--el-color-primary);
          background: rgba(64, 158, 255, 0.1);
        }

        .entry-header .entry-id {
          color: #f3f4f6;
        }

        .entry-content {
          .entry-title {
            color: #f3f4f6;
          }

          .entry-meta {
            color: #9ca3af;
          }

          .entry-journal {
            color: #9ca3af;
          }
        }
      }
    }

    .duplicates-list {
      .duplicate-item {
        border-bottom-color: rgba(102, 126, 234, 0.3);

        .duplicate-info {
          .duplicate-title {
            color: #f3f4f6;
          }

          .duplicate-id {
            color: #9ca3af;
          }
        }
      }
    }
  }
}
</style>
