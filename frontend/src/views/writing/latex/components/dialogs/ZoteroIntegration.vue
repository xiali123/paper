<template>
  <el-drawer
    v-model="show"
    title="Zotero 引用集成"
    direction="rtl"
    size="500px"
  >
    <div class="zotero-integration">
      <!-- 连接配置 -->
      <div class="config-section">
        <h4>Zotero 连接</h4>
        <el-alert
          type="info"
          :closable="false"
          show-icon
        >
          需要 <a href="https://www.zotero.org/support/dev/web_api" target="_blank">Zotero API Key</a>
          和 <a href="https://www.zotero.org/support/kb/group_library_access" target="_blank">用户/组ID</a>
        </el-alert>
        <el-form :model="zoteroConfig" label-width="100px" size="small">
          <el-form-item label="API Key">
            <el-input
              v-model="zoteroConfig.apiKey"
              type="password"
              show-password
              placeholder="输入Zotero API Key"
            />
          </el-form-item>
          <el-form-item label="用户/组ID">
            <el-input
              v-model="zoteroConfig.libraryID"
              placeholder="输入用户ID或组ID"
            />
          </el-form-item>
          <el-form-item label="Library类型">
            <el-radio-group v-model="zoteroConfig.libraryType">
              <el-radio value="user">个人库</el-radio>
              <el-radio value="group">群组库</el-radio>
            </el-radio-group>
          </el-form-item>
          <el-form-item>
            <el-button type="primary" @click="testConnection" :loading="testing">
              <el-icon><Connection /></el-icon>
              测试连接
            </el-button>
            <el-button @click="openZoteroHelp" :icon="QuestionFilled">
              获取帮助
            </el-button>
          </el-form-item>
        </el-form>
      </div>

      <!-- 引用库 -->
      <div v-if="isConnected" class="library-section">
        <div class="section-header">
          <h4>Zotero 引用库</h4>
          <div class="header-actions">
            <el-input
              v-model="searchQuery"
              placeholder="搜索引用..."
              size="small"
              :prefix-icon="Search"
              clearable
              style="width: 200px"
            />
            <el-button size="small" @click="fetchLibrary" :loading="loading">
              <el-icon><Refresh /></el-icon>
            </el-button>
          </div>
        </div>

        <!-- 分类筛选 -->
        <div class="collection-filter">
          <el-select
            v-model="selectedCollection"
            placeholder="全部合集"
            size="small"
            clearable
            @change="fetchLibrary"
          >
            <el-option
              v-for="collection in collections"
              :key="collection.key"
              :label="collection.name"
              :value="collection.key"
            />
          </el-select>
        </div>

        <!-- 引用列表 -->
        <div class="references-list">
          <div
            v-for="item in filteredReferences"
            :key="item.key"
            class="reference-item"
          >
            <div class="reference-icon">
              <el-icon>
                <Document v-if="item.itemType === 'book'" />
                <DocumentCopy v-else-if="item.itemType === 'article'" />
                <Files v-else />
              </el-icon>
            </div>
            <div class="reference-content">
              <div class="reference-title">{{ item.title }}</div>
              <div class="reference-meta">
                <span>{{ item.creators?.map((c: any) => c.lastName || c.name).join(', ') }}</span>
                <span v-if="item.date">• {{ item.date }}</span>
                <span v-if="item.publicationTitle">• {{ item.publicationTitle }}</span>
              </div>
              <div class="reference-tags">
                <el-tag
                  v-for="tag in item.tags?.slice(0, 3)"
                  :key="tag.tag"
                  size="small"
                  type="info"
                >
                  {{ tag.tag }}
                </el-tag>
              </div>
            </div>
            <div class="reference-actions">
              <el-dropdown trigger="click" @command="(cmd) => handleReferenceAction(cmd, item)">
                <el-button text>
                  <el-icon><MoreFilled /></el-icon>
                </el-button>
                <template #dropdown>
                  <el-dropdown-menu>
                    <el-dropdown-item command="cite">
                      <el-icon><Position /></el-icon>
                      插入引用 \cite{}
                    </el-dropdown-item>
                    <el-dropdown-item command="bib">
                      <el-icon><Download /></el-icon>
                      导出BibTeX
                    </el-dropdown-item>
                    <el-dropdown-item command="details">
                      <el-icon><View /></el-icon>
                      查看详情
                    </el-dropdown-item>
                  </el-dropdown-menu>
                </template>
              </el-dropdown>
            </div>
          </div>
        </div>

        <el-empty
          v-if="filteredReferences.length === 0"
          description="暂无引用"
        >
          <el-button @click="fetchLibrary">
            <el-icon><Refresh /></el-icon>
            刷新引用库
          </el-button>
        </el-empty>
      </div>

      <!-- 批量导入 -->
      <div v-if="isConnected" class="batch-section">
        <div class="section-header">
          <h4>批量操作</h4>
        </div>
        <div class="batch-actions">
          <el-button @click="exportAllBibTeX" :loading="exporting">
            <el-icon><Download /></el-icon>
            导出全部BibTeX
          </el-button>
          <el-button @click="syncToLocalBib" :loading="syncing">
            <el-icon><Upload /></el-icon>
            同步到本地.bib文件
          </el-button>
        </div>
      </div>

      <!-- 引用详情对话框 -->
      <el-dialog
        v-model="showDetails"
        title="引用详情"
        width="600px"
      >
        <div v-if="selectedReference" class="reference-details">
          <el-descriptions :column="1" border>
            <el-descriptions-item label="类型">
              {{ selectedReference.itemType }}
            </el-descriptions-item>
            <el-descriptions-item label="标题">
              {{ selectedReference.title }}
            </el-descriptions-item>
            <el-descriptions-item label="作者">
              {{ formatCreators(selectedReference.creators) }}
            </el-descriptions-item>
            <el-descriptions-item v-if="selectedReference.date" label="日期">
              {{ selectedReference.date }}
            </el-descriptions-item>
            <el-descriptions-item v-if="selectedReference.publicationTitle" label="出版物">
              {{ selectedReference.publicationTitle }}
            </el-descriptions-item>
            <el-descriptions-item v-if="selectedReference.volume" label="卷">
              {{ selectedReference.volume }}
            </el-descriptions-item>
            <el-descriptions-item v-if="selectedReference.issue" label="期">
              {{ selectedReference.issue }}
            </el-descriptions-item>
            <el-descriptions-item v-if="selectedReference.pages" label="页码">
              {{ selectedReference.pages }}
            </el-descriptions-item>
            <el-descriptions-item v-if="selectedReference.DOI" label="DOI">
              {{ selectedReference.DOI }}
            </el-descriptions-item>
            <el-descriptions-item v-if="selectedReference.url" label="URL">
              <a :href="selectedReference.url" target="_blank">{{ selectedReference.url }}</a>
            </el-descriptions-item>
            <el-descriptions-item label="标签">
              <el-tag
                v-for="tag in selectedReference.tags"
                :key="tag.tag"
                size="small"
                style="margin-right: 4px"
              >
                {{ tag.tag }}
              </el-tag>
            </el-descriptions-item>
          </el-descriptions>
          <div class="details-bibtex">
            <h4>BibTeX 预览</h4>
            <el-input
              :model-value="generateBibTeX(selectedReference)"
              type="textarea"
              :rows="8"
              readonly
            />
            <el-button
              type="primary"
              size="small"
              @click="copyBibTeX(selectedReference)"
              style="margin-top: 8px"
            >
              <el-icon><DocumentCopy /></el-icon>
              复制BibTeX
            </el-button>
          </div>
        </div>
      </el-dialog>
    </div>
  </el-drawer>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import {
  Connection, QuestionFilled, Search, Refresh, MoreFilled,
  Position, Download, View, Upload, Document, DocumentCopy, Files
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface ZoteroItem {
  key: string
  version: number
  itemType: string
  title: string
  creators?: Array<{
    creatorType: string
    firstName?: string
    lastName?: string
    name?: string
  }>
  date?: string
  publicationTitle?: string
  volume?: string
  issue?: string
  pages?: string
  DOI?: string
  url?: string
  tags?: Array<{ tag: string }>
  abstractNote?: string
}

interface ZoteroCollection {
  key: string
  name: string
}

interface ZoteroConfig {
  apiKey: string
  libraryID: string
  libraryType: 'user' | 'group'
}

interface Props {
  show: boolean
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'update:show': [value: boolean]
  'insert-citation': [citeKey: string]
  'export-bibtex': [entries: string]
}>()

// 状态
const testing = ref(false)
const isConnected = ref(false)
const loading = ref(false)
const exporting = ref(false)
const syncing = ref(false)

const zoteroConfig = ref<ZoteroConfig>({
  apiKey: '',
  libraryID: '',
  libraryType: 'user'
})

const references = ref<ZoteroItem[]>([])
const collections = ref<ZoteroCollection[]>([])
const selectedCollection = ref('')
const searchQuery = ref('')
const selectedReference = ref<ZoteroItem | null>(null)
const showDetails = ref(false)

// 计算属性
const filteredReferences = computed(() => {
  if (!searchQuery.value) {
    return references.value
  }
  const query = searchQuery.value.toLowerCase()
  return references.value.filter(item =>
    item.title?.toLowerCase().includes(query) ||
    item.creators?.some(c =>
      (c.lastName || c.name || '').toLowerCase().includes(query)
    ) ||
    item.tags?.some(t =>
      t.tag.toLowerCase().includes(query)
    )
  )
})

// 方法
function openZoteroHelp() {
  window.open('https://www.zotero.org/support/dev/web_api', '_blank')
}

async function testConnection() {
  if (!zoteroConfig.value.apiKey || !zoteroConfig.value.libraryID) {
    ElMessage.warning('请输入API Key和用户/组ID')
    return
  }

  testing.value = true
  try {
    // 模拟API调用
    await new Promise(resolve => setTimeout(resolve, 1500))

    // 实际应该调用:
    // const response = await fetch(
    //   `https://api.zotero.org/${zoteroConfig.value.libraryType}s/${zoteroConfig.value.libraryID}/keys?key=${zoteroConfig.value.apiKey}`
    // )

    isConnected.value = true
    ElMessage.success('连接成功')
    fetchLibrary()
    fetchCollections()
  } catch (error) {
    isConnected.value = false
    ElMessage.error('连接失败：' + (error as Error).message)
  } finally {
    testing.value = false
  }
}

async function fetchLibrary() {
  if (!isConnected.value) return

  loading.value = true
  try {
    // 模拟获取引用库
    await new Promise(resolve => setTimeout(resolve, 1000))

    references.value = [
      {
        key: 'ABC123',
        version: 5,
        itemType: 'journalArticle',
        title: 'Deep Learning for Natural Language Processing',
        creators: [
          { creatorType: 'author', firstName: 'Yoshua', lastName: 'Bengio' },
          { creatorType: 'author', firstName: 'Ian', lastName: 'Goodfellow' }
        ],
        date: '2023',
        publicationTitle: 'Journal of Machine Learning Research',
        volume: '24',
        pages: '1-45',
        DOI: '10.1234/jmlr.2023.001',
        tags: [
          { tag: 'deep learning' },
          { tag: 'NLP' },
          { tag: 'AI' }
        ]
      },
      {
        key: 'DEF456',
        version: 3,
        itemType: 'book',
        title: 'Attention Is All You Need',
        creators: [
          { creatorType: 'author', firstName: 'Ashish', lastName: 'Vaswani' }
        ],
        date: '2017',
        publicationTitle: 'NeurIPS',
        tags: [
          { tag: 'transformer' },
          { tag: 'attention' }
        ]
      }
    ]
  } catch (error) {
    ElMessage.error('获取引用库失败')
  } finally {
    loading.value = false
  }
}

async function fetchCollections() {
  try {
    // 模拟获取合集列表
    await new Promise(resolve => setTimeout(resolve, 500))

    collections.value = [
      { key: '', name: '全部引用' },
      { key: 'COLL1', name: 'Thesis References' },
      { key: 'COLL2', name: 'Background Research' }
    ]
  } catch (error) {
    ElMessage.error('获取合集列表失败')
  }
}

async function handleReferenceAction(command: string, item: ZoteroItem) {
  switch (command) {
    case 'cite':
      insertCitation(item)
      break
    case 'bib':
      exportSingleBibTeX(item)
      break
    case 'details':
      showReferenceDetails(item)
      break
  }
}

function insertCitation(item: ZoteroItem) {
  const citeKey = generateCiteKey(item)
  emit('insert-citation', `\\cite{${citeKey}}`)
  ElMessage.success(`已插入引用: \\cite{${citeKey}}`)
}

function generateCiteKey(item: ZoteroItem): string {
  // 生成BibTeX citation key
  const firstAuthor = item.creators?.[0]
  const lastName = firstAuthor?.lastName || firstAuthor?.name || 'anon'
  const year = item.date?.match(/\d{4}/)?.[0] || 'n.d.'
  const titleWords = item.title?.split(/\s+/).slice(0, 2).join('') || ''
  return `${lastName}${year}${titleWords}`.replace(/[^a-zA-Z0-9]/g, '')
}

function generateBibTeX(item: ZoteroItem): string {
  const citeKey = generateCiteKey(item)
  const typeMap: Record<string, string> = {
    journalArticle: 'article',
    book: 'book',
    conferencePaper: 'inproceedings'
  }
  const entryType = typeMap[item.itemType] || 'misc'

  let bibtex = `@${entryType}{${citeKey},\n`
  bibtex += `  title = {${item.title}},\n`

  if (item.creators) {
    const authors = item.creators
      .filter(c => c.creatorType === 'author')
      .map(c => `${c.firstName || ''} ${c.lastName || c.name || ''}`.trim())
      .join(' and ')
    if (authors) {
      bibtex += `  author = {${authors}},\n`
    }
  }

  if (item.date) {
    const year = item.date.match(/\d{4}/)?.[0] || ''
    if (year) {
      bibtex += `  year = {${year}},\n`
    }
  }

  if (item.publicationTitle) {
    const journalKey = item.itemType === 'journalArticle' ? 'journal' : 'publisher'
    bibtex += `  ${journalKey} = {${item.publicationTitle}},\n`
  }

  if (item.volume) {
    bibtex += `  volume = {${item.volume}},\n`
  }

  if (item.issue) {
    bibtex += `  number = {${item.issue}},\n`
  }

  if (item.pages) {
    bibtex += `  pages = {${item.pages}},\n`
  }

  if (item.DOI) {
    bibtex += `  doi = {${item.DOI}},\n`
  }

  if (item.url) {
    bibtex += `  url = {${item.url}},\n`
  }

  if (item.abstractNote) {
    bibtex += `  abstract = {${item.abstractNote}},\n`
  }

  bibtex = bibtex.slice(0, -2) + '\n}'
  return bibtex
}

function exportSingleBibTeX(item: ZoteroItem) {
  const bibtex = generateBibTeX(item)
  navigator.clipboard.writeText(bibtex)
  ElMessage.success('BibTeX已复制到剪贴板')
}

async function exportAllBibTeX() {
  exporting.value = true
  try {
    const allBibTeX = references.value.map(item => generateBibTeX(item)).join('\n\n')
    emit('export-bibtex', allBibTeX)
    ElMessage.success(`已导出${references.value.length}条引用`)
  } catch (error) {
    ElMessage.error('导出失败')
  } finally {
    exporting.value = false
  }
}

async function syncToLocalBib() {
  syncing.value = true
  try {
    const allBibTeX = references.value.map(item => generateBibTeX(item)).join('\n\n')
    emit('export-bibtex', allBibTeX)
    ElMessage.success('已同步到本地.bib文件')
  } catch (error) {
    ElMessage.error('同步失败')
  } finally {
    syncing.value = false
  }
}

function showReferenceDetails(item: ZoteroItem) {
  selectedReference.value = item
  showDetails.value = true
}

function copyBibTeX(item: ZoteroItem) {
  const bibtex = generateBibTeX(item)
  navigator.clipboard.writeText(bibtex)
  ElMessage.success('BibTeX已复制到剪贴板')
}

function formatCreators(creators?: Array<any>): string {
  if (!creators) return ''
  return creators
    .map(c => `${c.firstName || ''} ${c.lastName || c.name || ''}`.trim())
    .join(', ')
}

// 监听对话框打开
watch(() => props.show, (show) => {
  if (show && isConnected.value) {
    fetchLibrary()
  }
})
</script>

<style scoped lang="scss">
.zotero-integration {
  display: flex;
  flex-direction: column;
  gap: 20px;
  padding: 16px;
}

.config-section,
.library-section,
.batch-section {
  h4 {
    margin: 0 0 12px 0;
    font-size: 14px;
    font-weight: 500;
  }
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;

  .header-actions {
    display: flex;
    gap: 8px;
  }
}

.collection-filter {
  margin-bottom: 12px;
}

.references-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
  max-height: 500px;
  overflow-y: auto;
}

.reference-item {
  display: flex;
  gap: 12px;
  padding: 12px;
  background: var(--el-fill-color-blank);
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  transition: all 0.2s;

  &:hover {
    border-color: var(--el-color-primary);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  }
}

.reference-icon {
  flex-shrink: 0;
  width: 36px;
  height: 36px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--el-fill-color-light);
  border-radius: 8px;
  color: var(--el-color-primary);
}

.reference-content {
  flex: 1;
  min-width: 0;
}

.reference-title {
  font-weight: 500;
  margin-bottom: 4px;
}

.reference-meta {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin-bottom: 6px;
}

.reference-tags {
  display: flex;
  gap: 4px;
  flex-wrap: wrap;
}

.reference-actions {
  flex-shrink: 0;
}

.batch-actions {
  display: flex;
  gap: 8px;
}

.reference-details {
  .details-bibtex {
    margin-top: 16px;

    h4 {
      margin: 0 0 8px 0;
      font-size: 14px;
      font-weight: 500;
    }
  }
}

a {
  color: var(--el-color-primary);

  &:hover {
    text-decoration: underline;
  }
}
</style>
