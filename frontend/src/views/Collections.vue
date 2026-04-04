<template>
  <div class="collections-page">
    <!-- Header Section -->
    <div class="page-header">
      <div class="header-content">
        <div class="header-title-group">
          <span class="header-icon">⭐</span>
          <h1 class="page-title">收藏管理</h1>
        </div>
        <p class="page-description">管理您的论文收藏夹，快速访问重要文献</p>
      </div>
    </div>

    <!-- Action Bar -->
    <div class="action-bar">
      <button @click="showCreateDialog = true" class="action-btn btn-primary">
        <span class="btn-icon">➕</span>
        <span class="btn-text">新建收藏夹</span>
      </button>
      <button @click="fetchCollections" class="action-btn btn-secondary">
        <span class="btn-icon">🔄</span>
        <span class="btn-text">刷新</span>
      </button>
    </div>

    <!-- Collections Grid -->
    <div v-if="loading" class="loading-state">
      <LoadingSpinner size="large" variant="primary" text="加载收藏夹..." />
    </div>

    <div v-else-if="collections.length === 0" class="empty-state">
      <EmptyState
        icon="⭐"
        title="暂无收藏夹"
        description="创建您的第一个收藏夹，收藏重要的论文"
        action-text="新建收藏夹"
        @action="showCreateDialog = true"
      />
    </div>

    <div v-else class="collections-grid">
      <div
        v-for="collection in collections"
        :key="collection.id"
        class="collection-card"
        @click="viewCollection(collection)"
      >
        <div class="collection-header">
          <div class="collection-icon">
            {{ collection.icon || '📁' }}
          </div>
          <div class="collection-info">
            <h3 class="collection-name">{{ collection.name }}</h3>
            <p class="collection-desc">{{ collection.description || '暂无描述' }}</p>
          </div>
          <div class="collection-actions">
            <button @click.stop="editCollection(collection)" class="icon-btn">
              ✏️
            </button>
            <button @click.stop="confirmDelete(collection)" class="icon-btn">
              🗑️
            </button>
          </div>
        </div>
        <div class="collection-stats">
          <div class="stat-item">
            <span class="stat-value">{{ collection.paperCount || 0 }}</span>
            <span class="stat-label">篇论文</span>
          </div>
          <div class="stat-item">
            <span class="stat-value">{{ formatTimestamp(collection.created_at) }}</span>
            <span class="stat-label">创建时间</span>
          </div>
        </div>
        <div class="collection-papers">
          <div
            v-for="paper in getPapersPreview(collection)"
            :key="paper.id"
            class="paper-preview"
          >
            <span class="paper-title-preview">{{ paper.title }}</span>
          </div>
          <div v-if="!collection.papers || collection.papers.length === 0" class="no-papers">
            暂无论文
          </div>
        </div>
      </div>
    </div>

    <!-- Collection Detail Modal -->
    <Transition name="modal">
      <div v-if="selectedCollection" class="modal-overlay" @click="selectedCollection = null">
        <div class="modal-content large" @click.stop>
          <div class="modal-header">
            <h2 class="modal-title">
              {{ selectedCollection.icon || '📁' }} {{ selectedCollection.name }}
            </h2>
            <button @click="selectedCollection = null" class="close-button">✕</button>
          </div>
          <div class="modal-body">
            <!-- Collection Info -->
            <div class="collection-info-section">
              <div class="info-item">
                <strong>描述：</strong>{{ selectedCollection.description || '暂无描述' }}
              </div>
              <div class="info-item">
                <strong>创建时间：</strong>{{ formatFullTimestamp(selectedCollection.created_at) }}
              </div>
              <div class="info-item">
                <strong>论文数量：</strong>{{ selectedCollection.paperCount || 0 }} 篇
              </div>
            </div>

            <!-- Papers List -->
            <div class="papers-list-section">
              <div class="section-header">
                <h3>收藏的论文 ({{ selectedCollection.papers?.length || 0 }})</h3>
                <button @click="showAddPaperDialog = true" class="action-btn btn-sm btn-primary">
                  ➕ 添加论文
                </button>
              </div>

              <div v-if="!selectedCollection.papers || selectedCollection.papers.length === 0" class="empty-papers">
                <EmptyState
                  icon="📄"
                  title="暂无论文"
                  description="点击上方按钮添加论文到收藏夹"
                  :show-action="true"
                  action-text="添加论文"
                  @action="showAddPaperDialog = true"
                />
              </div>

              <div v-else class="papers-list">
                <div
                  v-for="paper in selectedCollection.papers"
                  :key="paper.id"
                  class="paper-item"
                >
                  <div class="paper-main">
                    <h4 class="paper-list-title">{{ paper.title }}</h4>
                    <p class="paper-list-authors">{{ paper.authors }}</p>
                    <p class="paper-list-meta">
                      <span v-if="paper.journal">{{ paper.journal }}</span>
                      <span v-if="paper.year">{{ paper.year }}</span>
                    </p>
                  </div>
                  <div class="paper-actions">
                    <button @click="viewPaper(paper)" class="icon-btn" title="查看详情">
                      👁️
                    </button>
                    <button @click="removePaper(paper.id)" class="icon-btn" title="移出收藏">
                      ❌
                    </button>
                  </div>
                </div>
              </div>
            </div>
          </div>
          <div class="modal-footer">
            <button @click="selectedCollection = null" class="btn btn-secondary">关闭</button>
            <button @click="exportCollection" class="btn btn-primary">导出收藏</button>
          </div>
        </div>
      </div>
    </Transition>

    <!-- Create/Edit Collection Dialog -->
    <Transition name="modal">
      <div v-if="showCreateDialog" class="modal-overlay" @click="showCreateDialog = false">
        <div class="modal-content" @click.stop>
          <div class="modal-header">
            <h2 class="modal-title">{{ editingCollection ? '编辑收藏夹' : '新建收藏夹' }}</h2>
            <button @click="showCreateDialog = false" class="close-button">✕</button>
          </div>
          <div class="modal-body">
            <div class="form-group">
              <label>收藏夹名称 *</label>
              <input
                v-model="collectionForm.name"
                type="text"
                class="form-input"
                placeholder="例如: 深度学习研究"
              >
            </div>
            <div class="form-group">
              <label>描述</label>
              <textarea
                v-model="collectionForm.description"
                class="form-textarea"
                rows="3"
                placeholder="简要描述这个收藏夹的用途"
              ></textarea>
            </div>
            <div class="form-group">
              <label>图标</label>
              <div class="icon-selector">
                <button
                  v-for="icon in iconOptions"
                  :key="icon"
                  @click="collectionForm.icon = icon"
                  :class="['icon-option', { selected: collectionForm.icon === icon }]"
                >
                  {{ icon }}
                </button>
              </div>
            </div>
          </div>
          <div class="modal-footer">
            <button @click="showCreateDialog = false" class="btn btn-secondary">取消</button>
            <button @click="saveCollection" class="btn btn-primary">保存</button>
          </div>
        </div>
      </div>
    </Transition>

    <!-- Add Paper Dialog -->
    <Transition name="modal">
      <div v-if="showAddPaperDialog" class="modal-overlay" @click="showAddPaperDialog = false">
        <div class="modal-content" @click.stop>
          <div class="modal-header">
            <h2 class="modal-title">添加论文</h2>
            <button @click="showAddPaperDialog = false" class="close-button">✕</button>
          </div>
          <div class="modal-body">
            <div class="form-group">
              <label>搜索论文</label>
              <input
                v-model="paperSearchQuery"
                @input="searchPapers"
                type="text"
                class="form-input"
                placeholder="输入论文标题或ID"
              >
            </div>
            <div v-if="searchResults.length > 0" class="search-results">
              <div
                v-for="paper in searchResults.slice(0, 5)"
                :key="paper.id"
                @click="addPaper(paper)"
                class="search-result-item"
              >
                <div class="result-main">
                  <h4 class="result-title">{{ paper.title }}</h4>
                  <p class="result-authors">{{ paper.authors }}</p>
                </div>
                <button class="add-btn">➕</button>
              </div>
            </div>
            <div v-else class="search-hint">
              输入关键词搜索论文
            </div>
          </div>
          <div class="modal-footer">
            <button @click="showAddPaperDialog = false" class="btn btn-secondary">关闭</button>
          </div>
        </div>
      </div>
    </Transition>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import LoadingSpinner from '@/components/common/LoadingSpinner.vue'
import EmptyState from '@/components/common/EmptyState.vue'

const router = useRouter()

// State
const collections = ref<any[]>([])
const loading = ref(false)
const selectedCollection = ref<any>(null)
const showCreateDialog = ref(false)
const showAddPaperDialog = ref(false)
const editingCollection = ref<any>(null)
const isSubmitting = ref(false)

// Collection form
const collectionForm = ref({
  name: '',
  description: '',
  icon: '📁'
})

// Paper search
const paperSearchQuery = ref('')
const searchResults = ref<any[]>([])

// Icon options
const iconOptions = ['📁', '📚', '🎯', '💡', '🔬', '📊', '📝', '⭐', '🧠', '🔥']

// Methods
const fetchCollections = async () => {
  loading.value = true
  try {
    const response = await fetch('http://localhost:8080/api/collections')
    const result = await response.json()

    if (result.success) {
      collections.value = result.data.collections || []
      // Mock data for now if API returns empty
      if (collections.value.length === 0) {
        collections.value = [
          {
            id: 1,
            name: '深度学习研究',
            description: '关于深度学习的最新研究论文',
            icon: '🧠',
            paperCount: 12,
            created_at: new Date().toISOString(),
            papers: [
              {
                id: 1001,
                title: 'Attention Is All You Need',
                authors: 'Vaswani et al.',
                journal: 'NeurIPS',
                year: '2017'
              },
              {
                id: 1002,
                title: 'BERT: Pre-training of Deep Bidirectional Transformers',
                authors: 'Devlin et al.',
                journal: 'NAACL',
                year: '2019'
              }
            ]
          }
        ]
      }
    } else {
      alert('获取收藏夹失败：' + result.error)
    }
  } catch (error: any) {
    alert('获取收藏夹失败：' + error.message)
    // Use mock data for demo
    collections.value = [
      {
        id: 1,
        name: '深度学习研究',
        description: '关于深度学习的最新研究论文',
        icon: '🧠',
        paperCount: 12,
        created_at: new Date().toISOString(),
        papers: [
          {
            id: 1001,
            title: 'Attention Is All You Need',
            authors: 'Vaswani et al.',
            journal: 'NeurIPS',
            year: '2017'
          },
          {
            id: 1002,
            title: 'BERT: Pre-training of Deep Bidirectional Transformers',
            authors: 'Devlin et al.',
            journal: 'NAACL',
            year: '2019'
          }
        ]
      }
    ]
  } finally {
    loading.value = false
  }
}

const viewCollection = (collection: any) => {
  selectedCollection.value = collection
}

const editCollection = (collection: any) => {
  editingCollection.value = collection
  collectionForm.value = {
    name: collection.name,
    description: collection.description || '',
    icon: collection.icon || '📁'
  }
  showCreateDialog.value = true
}

const saveCollection = async () => {
  if (!collectionForm.value.name) {
    alert('请输入收藏夹名称')
    return
  }

  isSubmitting.value = true
  try {
    if (editingCollection.value) {
      // Update existing collection
      alert('更新功能开发中...')
    } else {
      // Create new collection
      const newCollection = {
        id: Date.now(),
        ...collectionForm.value,
        paperCount: 0,
        created_at: new Date().toISOString(),
        papers: []
      }
      collections.value.push(newCollection)
      alert('收藏夹创建成功！')
    }
    showCreateDialog.value = false
  } catch (error: any) {
    alert('操作失败：' + error.message)
  } finally {
    isSubmitting.value = false
  }
}

const confirmDelete = (collection: any) => {
  if (confirm(`确定要删除收藏夹"${collection.name}"吗？`)) {
    deleteCollection(collection.id)
  }
}

const deleteCollection = async (id: number) => {
  try {
    // TODO: Call API to delete collection
    collections.value = collections.value.filter(c => c.id !== id)
    alert('收藏夹已删除')
  } catch (error: any) {
    alert('删除失败：' + error.message)
  }
}

const getPapersPreview = (collection: any) => {
  return (collection.papers || []).slice(0, 3)
}

const viewPaper = (paper: any) => {
  router.push(`/papers/${paper.id}`)
}

const removePaper = async (paperId: number) => {
  if (confirm('确定要从收藏夹移除这篇论文吗？')) {
    if (selectedCollection.value) {
      selectedCollection.value.papers = selectedCollection.value.papers.filter((p: any) => p.id !== paperId)
      selectedCollection.value.paperCount--
    }
  }
}

const searchPapers = async () => {
  if (paperSearchQuery.value.length < 2) {
    searchResults.value = []
    return
  }

  try {
    const response = await fetch(`http://localhost:8080/api/papers/search?query=${encodeURIComponent(paperSearchQuery.value)}`)
    const result = await response.json()

    if (result.success) {
      searchResults.value = result.data.papers || []
    }
  } catch (error: any) {
    console.error('搜索失败：', error)
  }
}

const addPaper = async (paper: any) => {
  if (selectedCollection.value) {
    // Check if paper already in collection
    if (selectedCollection.value.papers.some((p: any) => p.id === paper.id)) {
      alert('该论文已在收藏夹中')
      return
    }

    selectedCollection.value.papers.push(paper)
    selectedCollection.value.paperCount++
    showAddPaperDialog.value = false
    paperSearchQuery.value = ''
    searchResults.value = []

    // TODO: Call API to add paper to collection
    alert('论文已添加到收藏夹')
  }
}

const exportCollection = () => {
  if (selectedCollection.value) {
    alert('导出功能开发中...')
  }
}

const formatTimestamp = (timestamp: string) => {
  const date = new Date(timestamp)
  const diffMs = Date.now() - date.getTime()
  const diffDays = Math.floor(diffMs / 86400000)

  if (diffDays < 1) return '今天'
  if (diffDays < 7) return `${diffDays}天前`
  if (diffDays < 30) return `${Math.floor(diffDays / 7)}周前`
  return date.toLocaleDateString('zh-CN')
}

const formatFullTimestamp = (timestamp: string) => {
  return new Date(timestamp).toLocaleString('zh-CN')
}

// Lifecycle
onMounted(() => {
  fetchCollections()
})
</script>

<style scoped>
.collections-page {
  max-width: 1200px;
  margin: 0 auto;
  padding: 24px;
}

/* Header */
.page-header {
  margin-bottom: 32px;
}

.header-content {
  text-align: center;
}

.header-title-group {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16px;
  margin-bottom: 12px;
}

.header-icon {
  font-size: 48px;
}

.page-title {
  font-size: 36px;
  font-weight: 800;
  background: linear-gradient(135deg, #fbbf24 0%, #f59e0b 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  margin: 0;
}

.page-description {
  font-size: 16px;
  color: #6b7280;
  margin: 0;
}

/* Action Bar */
.action-bar {
  display: flex;
  justify-content: center;
  gap: 16px;
  margin-bottom: 32px;
}

.action-btn {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 24px;
  border: none;
  border-radius: 12px;
  font-size: 15px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.action-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.btn-primary {
  background: linear-gradient(135deg, #fbbf24 0%, #f59e0b 100%);
  color: white;
}

.btn-secondary {
  background: #f3f4f6;
  color: #374151;
}

.btn-secondary:hover {
  background: #e5e7eb;
}

.btn-sm {
  padding: 8px 16px;
  font-size: 13px;
}

/* Collections Grid */
.collections-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(350px, 1fr));
  gap: 24px;
}

.collection-card {
  background: white;
  border-radius: 16px;
  padding: 24px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  border: 2px solid #e5e7eb;
  cursor: pointer;
  transition: all 0.3s;
}

.collection-card:hover {
  border-color: #fbbf24;
  box-shadow: 0 8px 24px rgba(251, 191, 36, 0.15);
  transform: translateY(-4px);
}

.collection-header {
  display: flex;
  gap: 16px;
  margin-bottom: 16px;
}

.collection-icon {
  font-size: 48px;
  flex-shrink: 0;
}

.collection-info {
  flex: 1;
}

.collection-name {
  font-size: 18px;
  font-weight: 700;
  color: #1f2937;
  margin: 0 0 4px 0;
}

.collection-desc {
  font-size: 14px;
  color: #6b7280;
  margin: 0;
}

.collection-actions {
  display: flex;
  gap: 8px;
}

.icon-btn {
  width: 32px;
  height: 32px;
  border: none;
  border-radius: 6px;
  background: #f3f4f6;
  cursor: pointer;
  transition: all 0.2s;
  font-size: 16px;
}

.icon-btn:hover {
  background: #e5e7eb;
}

.collection-stats {
  display: flex;
  gap: 24px;
  margin-bottom: 16px;
  padding: 12px 0;
  border-top: 1px solid #e5e7eb;
  border-bottom: 1px solid #e5e7eb;
}

.stat-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.stat-value {
  font-size: 20px;
  font-weight: 700;
  color: #1f2937;
}

.stat-label {
  font-size: 12px;
  color: #9ca3af;
}

.collection-papers {
  min-height: 60px;
}

.paper-preview {
  padding: 6px 12px;
  margin: 4px 0;
  background: #f9fafb;
  border-radius: 6px;
  font-size: 13px;
  color: #6b7280;
}

.paper-title-preview {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.no-papers {
  padding: 12px;
  text-align: center;
  color: #9ca3af;
  font-size: 14px;
}

/* Modal Styles */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
}

.modal-content {
  background: white;
  border-radius: 16px;
  width: 90%;
  max-width: 600px;
  max-height: 80vh;
  display: flex;
  flex-direction: column;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
}

.modal-content.large {
  max-width: 900px;
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 24px;
  border-bottom: 1px solid #e5e7eb;
}

.modal-title {
  font-size: 20px;
  font-weight: 700;
  color: #1f2937;
  margin: 0;
}

.close-button {
  width: 32px;
  height: 32px;
  border-radius: 50%;
  border: none;
  background: #f3f4f6;
  font-size: 20px;
  cursor: pointer;
  transition: all 0.3s;
}

.close-button:hover {
  background: #e5e7eb;
  transform: rotate(90deg);
}

.modal-body {
  flex: 1;
  overflow-y: auto;
  padding: 24px;
}

.collection-info-section {
  margin-bottom: 24px;
  padding: 16px;
  background: #f9fafb;
  border-radius: 8px;
}

.info-item {
  margin-bottom: 8px;
  font-size: 14px;
  color: #374151;
}

.papers-list-section {
  margin-top: 24px;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.papers-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.paper-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  background: white;
  border: 1px solid #e5e7eb;
  border-radius: 8px;
  transition: all 0.2s;
}

.paper-item:hover {
  border-color: #fbbf24;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.paper-main {
  flex: 1;
}

.paper-list-title {
  font-size: 15px;
  font-weight: 600;
  color: #1f2937;
  margin: 0 0 4px 0;
}

.paper-list-authors {
  font-size: 13px;
  color: #6b7280;
  margin: 0 0 4px 0;
}

.paper-list-meta {
  display: flex;
  gap: 12px;
  font-size: 12px;
  color: #9ca3af;
}

.paper-actions {
  display: flex;
  gap: 8px;
}

/* Form Styles */
.form-group {
  margin-bottom: 20px;
}

.form-group label {
  display: block;
  font-weight: 600;
  margin-bottom: 8px;
  color: #374151;
  font-size: 14px;
}

.form-input {
  width: 100%;
  padding: 12px 16px;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  font-size: 15px;
  transition: all 0.3s;
}

.form-input:focus {
  outline: none;
  border-color: #fbbf24;
  box-shadow: 0 0 0 3px rgba(251, 191, 36, 0.1);
}

.form-textarea {
  width: 100%;
  padding: 12px 16px;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  font-size: 14px;
  font-family: inherit;
  resize: vertical;
  transition: all 0.3s;
}

.form-textarea:focus {
  outline: none;
  border-color: #fbbf24;
  box-shadow: 0 0 0 3px rgba(251, 191, 36, 0.1);
}

.icon-selector {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.icon-option {
  width: 48px;
  height: 48px;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  background: white;
  font-size: 24px;
  cursor: pointer;
  transition: all 0.2s;
}

.icon-option:hover {
  border-color: #fbbf24;
}

.icon-option.selected {
  border-color: #fbbf24;
  background: #fffbeb;
}

/* Search Results */
.search-results {
  margin-top: 16px;
  max-height: 300px;
  overflow-y: auto;
}

.search-result-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px;
  background: white;
  border: 1px solid #e5e7eb;
  border-radius: 8px;
  margin-bottom: 8px;
  cursor: pointer;
  transition: all 0.2s;
}

.search-result-item:hover {
  border-color: #fbbf24;
}

.result-main {
  flex: 1;
}

.result-title {
  font-size: 14px;
  font-weight: 600;
  margin: 0 0 4px 0;
}

.result-authors {
  font-size: 12px;
  color: #6b7280;
}

.add-btn {
  padding: 6px 12px;
  border: none;
  border-radius: 6px;
  background: #fbbf24;
  color: white;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
}

.search-hint {
  padding: 20px;
  text-align: center;
  color: #9ca3af;
  font-size: 14px;
}

.modal-footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
  padding: 24px;
  border-top: 1px solid #e5e7eb;
}

.btn {
  padding: 10px 20px;
  border: none;
  border-radius: 8px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
}

/* Empty State */
.empty-state {
  padding: 48px 24px;
}

.loading-state {
  padding: 48px 24px;
  text-align: center;
}

/* Modal Transitions */
.modal-enter-active,
.modal-leave-active {
  transition: all 0.3s;
}

.modal-enter-from,
.modal-leave-to {
  opacity: 0;
}

.modal-enter-from .modal-content,
.modal-leave-to .modal-content {
  transform: scale(0.9);
}
</style>
