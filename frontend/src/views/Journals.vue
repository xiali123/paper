<template>
  <div class="journals-page">
    <!-- Header Section -->
    <div class="page-header">
      <div class="header-content">
        <div class="header-title-group">
          <span class="header-icon">📰</span>
          <h1 class="page-title">期刊浏览</h1>
        </div>
        <p class="page-description">浏览CCF分级期刊，按研究领域筛选，了解期刊详情</p>
      </div>
    </div>

    <!-- Filters Section -->
    <div class="filters-section">
      <div class="filter-card">
        <div class="filter-header">
          <span class="filter-icon">🔍</span>
          <h3 class="filter-title">搜索期刊</h3>
        </div>
        <input
          v-model="searchQuery"
          @input="searchJournals"
          type="text"
          class="search-input"
          placeholder="输入期刊名称、ISSN或缩写"
        >
      </div>

      <div class="filter-card">
        <div class="filter-header">
          <span class="filter-icon">🏆</span>
          <h3 class="filter-title">CCF分级</h3>
        </div>
        <div class="ccf-buttons">
          <button
            @click="selectedRank = 'all'"
            :class="['ccf-btn', { active: selectedRank === 'all' }]"
          >
            全部
          </button>
          <button
            @click="selectedRank = 'A'"
            :class="['ccf-btn', 'ccf-a', { active: selectedRank === 'A' }]"
          >
            A类
          </button>
          <button
            @click="selectedRank = 'B'"
            :class="['ccf-btn', 'ccf-b', { active: selectedRank === 'B' }]"
          >
            B类
          </button>
          <button
            @click="selectedRank = 'C'"
            :class="['ccf-btn', 'ccf-c', { active: selectedRank === 'C' }]"
          >
            C类
          </button>
        </div>
      </div>

      <div class="filter-card">
        <div class="filter-header">
          <span class="filter-icon">🔬</span>
          <h3 class="filter-title">研究领域</h3>
        </div>
        <div class="field-buttons">
          <button
            v-for="field in researchFields"
            :key="field.key"
            @click="selectedField = field.key"
            :class="['field-btn', { active: selectedField === field.key }]"
          >
            {{ field.icon }} {{ field.name }}
          </button>
        </div>
      </div>
    </div>

    <!-- Journals Grid -->
    <div v-if="loading" class="loading-state">
      <LoadingSpinner size="large" variant="primary" text="加载期刊列表..." />
    </div>

    <div v-else-if="filteredJournals.length === 0" class="empty-state">
      <EmptyState
        icon="📰"
        title="暂无符合条件的期刊"
        description="请尝试调整筛选条件或搜索关键词"
      />
    </div>

    <div v-else>
      <!-- Results Summary -->
      <div class="results-summary">
        <span class="results-count">找到 {{ filteredJournals.length }} 个期刊</span>
        <div class="view-toggles">
          <button
            @click="viewMode = 'grid'"
            :class="['view-toggle', { active: viewMode === 'grid' }]"
          >
            ⊞ 网格
          </button>
          <button
            @click="viewMode = 'list'"
            :class="['view-toggle', { active: viewMode === 'list' }]"
          >
            ☰ 列表
          </button>
        </div>
      </div>

      <!-- Grid View -->
      <div v-if="viewMode === 'grid'" class="journals-grid">
        <div
          v-for="journal in paginatedJournals"
          :key="journal.id"
          class="journal-card"
          @click="viewJournalDetail(journal)"
        >
          <div class="journal-header">
            <div :class="['ccf-badge', `ccf-${journal.rank?.toLowerCase()}`]">
              CCF {{ journal.rank }}
            </div>
            <div class="journal-issn">{{ journal.issn }}</div>
          </div>
          <div class="journal-content">
            <h3 class="journal-name">{{ journal.name }}</h3>
            <p class="journal-fullname">{{ journal.fullName }}</p>
            <div class="journal-meta">
              <span class="journal-field">{{ journal.field }}</span>
              <span class="journal-impact" v-if="journal.impactFactor">
                影响因子: {{ journal.impactFactor }}
              </span>
            </div>
            <div class="journal-tags">
              <span v-for="tag in journal.tags" :key="tag" class="tag">
                {{ tag }}
              </span>
            </div>
          </div>
        </div>
      </div>

      <!-- List View -->
      <div v-else class="journals-list">
        <div
          v-for="journal in paginatedJournals"
          :key="journal.id"
          class="journal-list-item"
          @click="viewJournalDetail(journal)"
        >
          <div class="journal-list-header">
            <div :class="['ccf-badge', `ccf-${journal.rank?.toLowerCase()}`]">
              CCF {{ journal.rank }}
            </div>
            <div class="journal-list-info">
              <h3 class="journal-list-name">{{ journal.name }}</h3>
              <p class="journal-list-fullname">{{ journal.fullName }}</p>
            </div>
            <div class="journal-list-meta">
              <span class="journal-issn">{{ journal.issn }}</span>
              <span class="journal-field">{{ journal.field }}</span>
              <span v-if="journal.impactFactor" class="journal-impact">
                IF: {{ journal.impactFactor }}
              </span>
            </div>
          </div>
        </div>
      </div>

      <!-- Pagination -->
      <div v-if="totalPages > 1" class="pagination">
        <button
          @click="currentPage--"
          :disabled="currentPage === 1"
          class="pagination-btn"
        >
          上一页
        </button>
        <span class="pagination-info">{{ currentPage }} / {{ totalPages }}</span>
        <button
          @click="currentPage++"
          :disabled="currentPage === totalPages"
          class="pagination-btn"
        >
          下一页
        </button>
      </div>
    </div>

    <!-- Journal Detail Modal -->
    <Transition name="modal">
      <div v-if="selectedJournal" class="modal-overlay" @click="selectedJournal = null">
        <div class="modal-content large" @click.stop>
          <div class="modal-header">
            <div class="modal-title-group">
              <div :class="['ccf-badge-large', `ccf-${selectedJournal.rank?.toLowerCase()}`]">
                CCF {{ selectedJournal.rank }}
              </div>
              <h2 class="modal-title">{{ selectedJournal.fullName }}</h2>
            </div>
            <button @click="selectedJournal = null" class="close-button">✕</button>
          </div>
          <div class="modal-body">
            <!-- Basic Info -->
            <div class="detail-section">
              <h3 class="section-title">基本信息</h3>
              <div class="info-grid">
                <div class="info-item">
                  <strong>期刊缩写:</strong> {{ selectedJournal.name }}
                </div>
                <div class="info-item">
                  <strong>ISSN:</strong> {{ selectedJournal.issn }}
                </div>
                <div class="info-item">
                  <strong>研究领域:</strong> {{ selectedJournal.field }}
                </div>
                <div class="info-item" v-if="selectedJournal.impactFactor">
                  <strong>影响因子:</strong> {{ selectedJournal.impactFactor }}
                </div>
                <div class="info-item" v-if="selectedJournal.publisher">
                  <strong>出版社:</strong> {{ selectedJournal.publisher }}
                </div>
                <div class="info-item" v-if="selectedJournal.frequency">
                  <strong>出版频率:</strong> {{ selectedJournal.frequency }}
                </div>
              </div>
            </div>

            <!-- Description -->
            <div class="detail-section" v-if="selectedJournal.description">
              <h3 class="section-title">期刊简介</h3>
              <p class="journal-description">{{ selectedJournal.description }}</p>
            </div>

            <!-- Topics -->
            <div class="detail-section" v-if="selectedJournal.topics">
              <h3 class="section-title">研究方向</h3>
              <div class="topics-list">
                <span v-for="topic in selectedJournal.topics" :key="topic" class="topic-tag">
                  {{ topic }}
                </span>
              </div>
            </div>

            <!-- Statistics -->
            <div class="detail-section" v-if="selectedJournal.stats">
              <h3 class="section-title">统计信息</h3>
              <div class="stats-grid">
                <div class="stat-card">
                  <div class="stat-value">{{ selectedJournal.stats.submissionRate || 'N/A' }}</div>
                  <div class="stat-label">投稿录用率</div>
                </div>
                <div class="stat-card">
                  <div class="stat-value">{{ selectedJournal.stats.reviewTime || 'N/A' }}</div>
                  <div class="stat-label">审稿周期</div>
                </div>
                <div class="stat-card">
                  <div class="stat-value">{{ selectedJournal.stats.paperCount || 'N/A' }}</div>
                  <div class="stat-label">年发文量</div>
                </div>
                <div class="stat-card">
                  <div class="stat-value">{{ selectedJournal.stats.citeScore || 'N/A' }}</div>
                  <div class="stat-label">CiteScore</div>
                </div>
              </div>
            </div>

            <!-- Related Papers -->
            <div class="detail-section">
              <h3 class="section-title">相关论文</h3>
              <div v-if="relatedPapers.length === 0" class="empty-papers">
                暂无相关论文数据
              </div>
              <div v-else class="related-papers-list">
                <div
                  v-for="paper in relatedPapers.slice(0, 5)"
                  :key="paper.id"
                  class="related-paper-item"
                  @click="viewPaper(paper)"
                >
                  <h4 class="paper-title">{{ paper.title }}</h4>
                  <p class="paper-authors">{{ paper.authors }}</p>
                  <p class="paper-year">{{ paper.year }}</p>
                </div>
              </div>
            </div>
          </div>
          <div class="modal-footer">
            <button @click="selectedJournal = null" class="btn btn-secondary">关闭</button>
            <button @click="searchPapersInJournal" class="btn btn-primary">
              查找该期刊论文
            </button>
          </div>
        </div>
      </div>
    </Transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import LoadingSpinner from '@/components/common/LoadingSpinner.vue'
import EmptyState from '@/components/common/EmptyState.vue'

const router = useRouter()

// State
const journals = ref<any[]>([])
const loading = ref(false)
const selectedJournal = ref<any>(null)
const relatedPapers = ref<any[]>([])

// Filters
const searchQuery = ref('')
const selectedRank = ref('all')
const selectedField = ref('all')
const viewMode = ref<'grid' | 'list'>('grid')

// Pagination
const currentPage = ref(1)
const pageSize = ref(12)

// Research fields
const researchFields = [
  { key: 'all', name: '全部', icon: '🔬' },
  { key: 'ai', name: '人工智能', icon: '🤖' },
  { key: 'cv', name: '计算机视觉', icon: '👁️' },
  { key: 'ml', name: '机器学习', icon: '🧠' },
  { key: 'nlp', name: '自然语言处理', icon: '💬' },
  { key: 'network', name: '计算机网络', icon: '🌐' },
  { key: 'system', name: '计算机系统', icon: '⚙️' },
  { key: 'security', name: '网络安全', icon: '🔒' },
  { key: 'database', name: '数据库', icon: '🗄️' },
  { key: 'theory', name: '理论计算机', icon: '📐' }
]

// Mock journal data
const mockJournals = [
  {
    id: 1,
    name: 'JACM',
    fullName: 'Journal of the ACM',
    issn: '0004-5411',
    rank: 'A',
    field: '理论计算机',
    impactFactor: 4.5,
    publisher: 'ACM',
    frequency: '双月刊',
    description: 'ACM官方期刊，涵盖计算机科学所有领域的原创研究论文。',
    topics: ['算法', '计算理论', '计算机科学基础', '复杂性理论'],
    tags: ['理论', '算法', '顶级'],
    stats: {
      submissionRate: '15%',
      reviewTime: '6-12个月',
      paperCount: '60-80',
      citeScore: 5.2
    }
  },
  {
    id: 2,
    name: 'IEEE Trans. Pattern Anal. Mach. Intell.',
    fullName: 'IEEE Transactions on Pattern Analysis and Machine Intelligence',
    issn: '0162-8828',
    rank: 'A',
    field: '计算机视觉',
    impactFactor: 24.314,
    publisher: 'IEEE',
    frequency: '月刊',
    description: '计算机视觉和模式识别领域的顶级期刊，涵盖图像处理、机器学习等相关研究。',
    topics: ['计算机视觉', '模式识别', '机器学习', '图像处理'],
    tags: ['CV', 'ML', '顶刊'],
    stats: {
      submissionRate: '12%',
      reviewTime: '6-10个月',
      paperCount: '150-200',
      citeScore: 28.5
    }
  },
  {
    id: 3,
    name: 'IEEE Trans. Software Eng.',
    fullName: 'IEEE Transactions on Software Engineering',
    issn: '0098-5589',
    rank: 'A',
    field: '软件工程',
    impactFactor: 6.7,
    publisher: 'IEEE',
    frequency: '月刊',
    description: '软件工程领域的重要期刊，涵盖软件开发、测试、维护等各个方面。',
    topics: ['软件工程', '软件测试', '软件维护', '软件架构'],
    tags: ['SE', '工程'],
    stats: {
      submissionRate: '18%',
      reviewTime: '8-14个月',
      paperCount: '100-120',
      citeScore: 8.3
    }
  },
  {
    id: 4,
    name: 'ACM Trans. Graph.',
    fullName: 'ACM Transactions on Graphics',
    issn: '0730-0301',
    rank: 'A',
    field: '计算机图形学',
    impactFactor: 6.2,
    publisher: 'ACM',
    frequency: '月刊',
    description: '计算机图形学领域的顶级期刊，SIGGRAPH会议论文的延伸期刊。',
    topics: ['计算机图形学', '渲染', '动画', '几何建模'],
    tags: ['Graphics', 'SIGGRAPH'],
    stats: {
      submissionRate: '20%',
      reviewTime: '4-8个月',
      paperCount: '80-100',
      citeScore: 7.8
    }
  },
  {
    id: 5,
    name: 'NeurIPS',
    fullName: 'Advances in Neural Information Processing Systems',
    issn: '1049-5258',
    rank: 'A',
    field: '机器学习',
    impactFactor: 8.9,
    publisher: 'Curran Associates',
    frequency: '年刊',
    description: '神经信息处理系统会议论文集，机器学习领域顶级会议。',
    topics: ['深度学习', '神经网络', '强化学习', '优化'],
    tags: ['ML', 'AI', '顶会'],
    stats: {
      submissionRate: '25%',
      reviewTime: '3-4个月',
      paperCount: '1500-2000',
      citeScore: 12.3
    }
  },
  {
    id: 6,
    name: 'ACL',
    fullName: 'Association for Computational Linguistics',
    issn: '2329-4222',
    rank: 'A',
    field: '自然语言处理',
    impactFactor: 7.8,
    publisher: 'ACL',
    frequency: '年刊',
    description: '自然语言处理领域顶级会议，涵盖NLP所有研究方向。',
    topics: ['NLP', '计算语言学', '文本分析', '语言模型'],
    tags: ['NLP', 'AI', '顶会'],
    stats: {
      submissionRate: '23%',
      reviewTime: '3-4个月',
      paperCount: '500-600',
      citeScore: 10.5
    }
  },
  {
    id: 7,
    name: 'IEEE/ACM Trans. Netw.',
    fullName: 'IEEE/ACM Transactions on Networking',
    issn: '1063-6692',
    rank: 'A',
    field: '计算机网络',
    impactFactor: 5.6,
    publisher: 'IEEE/ACM',
    frequency: '双月刊',
    description: '计算机网络领域顶级期刊，涵盖网络协议、架构、性能等。',
    topics: ['网络协议', '网络架构', '无线网络', '网络性能'],
    tags: ['Network', '通信'],
    stats: {
      submissionRate: '20%',
      reviewTime: '8-12个月',
      paperCount: '80-100',
      citeScore: 6.9
    }
  },
  {
    id: 8,
    name: 'J. Cryptology',
    fullName: 'Journal of Cryptology',
    issn: '0933-2790',
    rank: 'A',
    field: '网络安全',
    impactFactor: 3.2,
    publisher: 'Springer',
    frequency: '季刊',
    description: '密码学领域的顶级期刊，涵盖密码理论和应用。',
    topics: ['密码学', '加密算法', '安全协议', '密码分析'],
    tags: ['Security', 'Crypto'],
    stats: {
      submissionRate: '15%',
      reviewTime: '10-14个月',
      paperCount: '40-60',
      citeScore: 4.1
    }
  },
  {
    id: 9,
    name: 'ACM Trans. Database Syst.',
    fullName: 'ACM Transactions on Database Systems',
    issn: '0362-5915',
    rank: 'B',
    field: '数据库',
    impactFactor: 2.8,
    publisher: 'ACM',
    frequency: '季刊',
    description: '数据库系统领域重要期刊，涵盖数据管理、查询优化等。',
    topics: ['数据库', '数据管理', '查询优化', '数据挖掘'],
    tags: ['Database', 'Data'],
    stats: {
      submissionRate: '25%',
      reviewTime: '6-10个月',
      paperCount: '30-50',
      citeScore: 3.9
    }
  },
  {
    id: 10,
    name: 'Inf. Process. Manage.',
    fullName: 'Information Processing & Management',
    issn: '0306-4573',
    rank: 'B',
    field: '信息检索',
    impactFactor: 5.2,
    publisher: 'Elsevier',
    frequency: '双月刊',
    description: '信息处理和管理领域期刊，涵盖信息检索、文本分析等。',
    topics: ['信息检索', '文本分析', '自然语言处理', '数据管理'],
    tags: ['IR', 'NLP'],
    stats: {
      submissionRate: '30%',
      reviewTime: '4-8个月',
      paperCount: '100-120',
      citeScore: 6.5
    }
  },
  {
    id: 11,
    name: 'Pattern Recognit.',
    fullName: 'Pattern Recognition',
    issn: '0031-3203',
    rank: 'B',
    field: '模式识别',
    impactFactor: 8.5,
    publisher: 'Elsevier',
    frequency: '月刊',
    description: '模式识别领域期刊，涵盖计算机视觉、机器学习应用等。',
    topics: ['模式识别', '计算机视觉', '机器学习', '图像分析'],
    tags: ['CV', 'ML', 'PR'],
    stats: {
      submissionRate: '28%',
      reviewTime: '5-9个月',
      paperCount: '300-350',
      citeScore: 9.8
    }
  },
  {
    id: 12,
    name: 'Neural Comput. & Applic.',
    fullName: 'Neural Computing and Applications',
    issn: '0941-0643',
    rank: 'C',
    field: '神经网络',
    impactFactor: 5.6,
    publisher: 'Springer',
    frequency: '月刊',
    description: '神经网络计算和应用期刊，涵盖神经网络理论和应用。',
    topics: ['神经网络', '深度学习', '神经计算', 'AI应用'],
    tags: ['NN', 'DL', 'AI'],
    stats: {
      submissionRate: '35%',
      reviewTime: '3-6个月',
      paperCount: '400-500',
      citeScore: 6.7
    }
  },
  {
    id: 13,
    name: 'Cogn. Neurodynamics',
    fullName: 'Cognitive Neurodynamics',
    issn: '1871-4080',
    rank: 'C',
    field: '认知神经',
    impactFactor: 2.4,
    publisher: 'Springer',
    frequency: '双月刊',
    description: '认知神经动力学期刊，涵盖认知科学和神经动力学。',
    topics: ['认知科学', '神经动力学', '脑科学', '认知计算'],
    tags: ['Cognitive', 'Neuro'],
    stats: {
      submissionRate: '40%',
      reviewTime: '4-7个月',
      paperCount: '80-100',
      citeScore: 3.2
    }
  },
  {
    id: 14,
    name: 'Comput. Intell. Neurosci.',
    fullName: 'Computational Intelligence and Neuroscience',
    issn: '1687-5265',
    rank: 'C',
    field: '计算智能',
    impactFactor: 3.1,
    publisher: 'Hindawi',
    frequency: '月刊',
    description: '计算智能和神经科学交叉领域期刊。',
    topics: ['计算智能', '神经科学', 'AI', '神经网络'],
    tags: ['CI', 'Neuro', 'AI'],
    stats: {
      submissionRate: '45%',
      reviewTime: '2-4个月',
      paperCount: '500-600',
      citeScore: 4.2
    }
  },
  {
    id: 15,
    name: 'Appl. Intell.',
    fullName: 'Applied Intelligence',
    issn: '0924-669X',
    rank: 'C',
    field: '人工智能',
    impactFactor: 3.9,
    publisher: 'Springer',
    frequency: '月刊',
    description: '人工智能应用期刊，涵盖AI在各领域的应用研究。',
    topics: ['人工智能', '机器学习应用', '智能系统', 'AI应用'],
    tags: ['AI', 'ML', '应用'],
    stats: {
      submissionRate: '38%',
      reviewTime: '3-6个月',
      paperCount: '300-400',
      citeScore: 5.1
    }
  }
]

// Computed
const filteredJournals = computed(() => {
  let result = journals.value

  // Filter by CCF rank
  if (selectedRank.value !== 'all') {
    result = result.filter(j => j.rank === selectedRank.value)
  }

  // Filter by research field
  if (selectedField.value !== 'all') {
    const fieldMap: Record<string, string> = {
      'ai': '人工智能',
      'cv': '计算机视觉',
      'ml': '机器学习',
      'nlp': '自然语言处理',
      'network': '计算机网络',
      'system': '计算机系统',
      'security': '网络安全',
      'database': '数据库',
      'theory': '理论计算机'
    }
    const fieldName = fieldMap[selectedField.value]
    if (fieldName) {
      result = result.filter(j => j.field.includes(fieldName) || j.tags.some((t: string) => t.includes(fieldName.toUpperCase())))
    }
  }

  // Filter by search query
  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    result = result.filter(j =>
      j.name.toLowerCase().includes(query) ||
      j.fullName.toLowerCase().includes(query) ||
      j.issn.includes(query)
    )
  }

  return result
})

const totalPages = computed(() => Math.ceil(filteredJournals.value.length / pageSize.value))

const paginatedJournals = computed(() => {
  const start = (currentPage.value - 1) * pageSize.value
  const end = start + pageSize.value
  return filteredJournals.value.slice(start, end)
})

// Methods
const fetchJournals = async () => {
  loading.value = true
  try {
    const response = await fetch('http://localhost:8080/api/journals')
    const result = await response.json()

    if (result.success) {
      journals.value = result.data.journals || []
    } else {
      // Use mock data if API fails
      journals.value = mockJournals
    }
  } catch (error: any) {
    // Use mock data for demo
    journals.value = mockJournals
  } finally {
    loading.value = false
  }
}

const searchJournals = () => {
  currentPage.value = 1
}

const viewJournalDetail = async (journal: any) => {
  selectedJournal.value = journal

  // Fetch related papers
  try {
    const response = await fetch(`http://localhost:8080/api/papers/search?journal=${encodeURIComponent(journal.name)}`)
    const result = await response.json()

    if (result.success) {
      relatedPapers.value = result.data.papers || []
    }
  } catch (error) {
    relatedPapers.value = []
  }
}

const viewPaper = (paper: any) => {
  router.push(`/papers/${paper.id}`)
}

const searchPapersInJournal = () => {
  if (selectedJournal.value) {
    router.push({
      path: '/search-advanced',
      query: { journal: selectedJournal.value.name }
    })
  }
}

// Lifecycle
onMounted(() => {
  fetchJournals()
})
</script>

<style scoped>
.journals-page {
  max-width: 1400px;
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
  background: linear-gradient(135deg, #60a5fa 0%, #3b82f6 100%);
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

/* Filters Section */
.filters-section {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
  gap: 24px;
  margin-bottom: 32px;
}

.filter-card {
  background: white;
  border-radius: 16px;
  padding: 24px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  border: 1px solid #e5e7eb;
}

.filter-header {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 16px;
}

.filter-icon {
  font-size: 24px;
}

.filter-title {
  font-size: 16px;
  font-weight: 700;
  color: #1f2937;
  margin: 0;
}

.search-input {
  width: 100%;
  padding: 12px 16px;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  font-size: 15px;
  transition: all 0.3s;
}

.search-input:focus {
  outline: none;
  border-color: #60a5fa;
  box-shadow: 0 0 0 3px rgba(96, 165, 250, 0.1);
}

.ccf-buttons {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.ccf-btn {
  padding: 8px 20px;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  background: white;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
}

.ccf-btn:hover {
  border-color: #60a5fa;
  box-shadow: 0 2px 8px rgba(96, 165, 250, 0.2);
}

.ccf-btn.active {
  border-color: #60a5fa;
  background: #60a5fa;
  color: white;
}

.ccf-a.active {
  background: #dc2626;
  border-color: #dc2626;
}

.ccf-b.active {
  background: #f59e0b;
  border-color: #f59e0b;
}

.ccf-c.active {
  background: #10b981;
  border-color: #10b981;
}

.field-buttons {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.field-btn {
  padding: 8px 16px;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  background: white;
  font-size: 14px;
  cursor: pointer;
  transition: all 0.3s;
}

.field-btn:hover {
  border-color: #60a5fa;
}

.field-btn.active {
  border-color: #60a5fa;
  background: linear-gradient(135deg, #60a5fa 0%, #3b82f6 100%);
  color: white;
}

/* Results Summary */
.results-summary {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
  padding: 16px 24px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.05);
}

.results-count {
  font-size: 16px;
  font-weight: 600;
  color: #374151;
}

.view-toggles {
  display: flex;
  gap: 8px;
}

.view-toggle {
  padding: 8px 16px;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  background: white;
  font-size: 14px;
  cursor: pointer;
  transition: all 0.2s;
}

.view-toggle:hover {
  border-color: #60a5fa;
}

.view-toggle.active {
  border-color: #60a5fa;
  background: #60a5fa;
  color: white;
}

/* Journals Grid */
.journals-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
  gap: 20px;
}

.journal-card {
  background: white;
  border-radius: 16px;
  padding: 24px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  border: 2px solid #e5e7eb;
  cursor: pointer;
  transition: all 0.3s;
}

.journal-card:hover {
  border-color: #60a5fa;
  box-shadow: 0 8px 24px rgba(96, 165, 250, 0.15);
  transform: translateY(-4px);
}

.journal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.ccf-badge {
  padding: 6px 12px;
  border-radius: 6px;
  font-size: 13px;
  font-weight: 700;
  color: white;
}

.ccf-a {
  background: linear-gradient(135deg, #dc2626 0%, #ef4444 100%);
}

.ccf-b {
  background: linear-gradient(135deg, #f59e0b 0%, #fbbf24 100%);
}

.ccf-c {
  background: linear-gradient(135deg, #10b981 0%, #34d399 100%);
}

.journal-issn {
  font-size: 12px;
  color: #9ca3af;
}

.journal-content {
  flex: 1;
}

.journal-name {
  font-size: 18px;
  font-weight: 700;
  color: #1f2937;
  margin: 0 0 8px 0;
}

.journal-fullname {
  font-size: 13px;
  color: #6b7280;
  margin: 0 0 12px 0;
  line-height: 1.5;
}

.journal-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  margin-bottom: 12px;
  font-size: 13px;
}

.journal-field {
  color: #60a5fa;
  font-weight: 600;
}

.journal-impact {
  color: #9ca3af;
}

.journal-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}

.tag {
  padding: 4px 10px;
  border-radius: 6px;
  font-size: 12px;
  background: #f3f4f6;
  color: #6b7280;
}

/* Journals List */
.journals-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.journal-list-item {
  background: white;
  border-radius: 12px;
  padding: 20px 24px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.05);
  border: 2px solid #e5e7eb;
  cursor: pointer;
  transition: all 0.3s;
}

.journal-list-item:hover {
  border-color: #60a5fa;
  box-shadow: 0 4px 12px rgba(96, 165, 250, 0.15);
}

.journal-list-header {
  display: flex;
  align-items: center;
  gap: 20px;
}

.journal-list-info {
  flex: 1;
}

.journal-list-name {
  font-size: 18px;
  font-weight: 700;
  color: #1f2937;
  margin: 0 0 4px 0;
}

.journal-list-fullname {
  font-size: 14px;
  color: #6b7280;
  margin: 0;
}

.journal-list-meta {
  display: flex;
  gap: 16px;
  font-size: 13px;
  color: #9ca3af;
}

/* Pagination */
.pagination {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 16px;
  margin-top: 32px;
}

.pagination-btn {
  padding: 8px 16px;
  border: 1px solid #e5e7eb;
  border-radius: 6px;
  background: white;
  cursor: pointer;
  transition: all 0.2s;
}

.pagination-btn:hover:not(:disabled) {
  background: #f9fafb;
}

.pagination-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.pagination-info {
  font-size: 14px;
  color: #6b7280;
  font-weight: 500;
}

/* Modal */
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
  max-width: 800px;
  max-height: 80vh;
  display: flex;
  flex-direction: column;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
}

.modal-content.large {
  max-width: 1000px;
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 24px;
  border-bottom: 1px solid #e5e7eb;
}

.modal-title-group {
  display: flex;
  align-items: center;
  gap: 16px;
  flex: 1;
}

.ccf-badge-large {
  padding: 8px 16px;
  border-radius: 8px;
  font-size: 16px;
  font-weight: 700;
  color: white;
  flex-shrink: 0;
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
  flex-shrink: 0;
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

.detail-section {
  margin-bottom: 24px;
  padding-bottom: 24px;
  border-bottom: 1px solid #e5e7eb;
}

.detail-section:last-of-type {
  border-bottom: none;
}

.section-title {
  font-size: 18px;
  font-weight: 700;
  color: #1f2937;
  margin: 0 0 16px 0;
}

.info-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 16px;
}

.info-item {
  font-size: 14px;
  color: #374151;
  line-height: 1.6;
}

.journal-description {
  font-size: 15px;
  color: #4b5563;
  line-height: 1.8;
  margin: 0;
}

.topics-list {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.topic-tag {
  padding: 6px 14px;
  border-radius: 8px;
  font-size: 14px;
  background: #dbeafe;
  color: #1e40af;
  font-weight: 500;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
  gap: 16px;
}

.stat-card {
  padding: 16px;
  background: #f9fafb;
  border-radius: 8px;
  text-align: center;
}

.stat-value {
  font-size: 24px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 4px;
}

.stat-label {
  font-size: 12px;
  color: #9ca3af;
}

.empty-papers {
  padding: 32px;
  text-align: center;
  color: #9ca3af;
}

.related-papers-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.related-paper-item {
  padding: 16px;
  background: #f9fafb;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s;
}

.related-paper-item:hover {
  background: #e5e7eb;
}

.paper-title {
  font-size: 14px;
  font-weight: 600;
  color: #1f2937;
  margin: 0 0 4px 0;
}

.paper-authors {
  font-size: 12px;
  color: #6b7280;
  margin: 0 0 4px 0;
}

.paper-year {
  font-size: 12px;
  color: #9ca3af;
  margin: 0;
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

.btn-primary {
  background: linear-gradient(135deg, #60a5fa 0%, #3b82f6 100%);
  color: white;
}

.btn-primary:hover {
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(96, 165, 250, 0.4);
}

.btn-secondary {
  background: #f3f4f6;
  color: #374151;
}

.btn-secondary:hover {
  background: #e5e7eb;
}

/* Empty State */
.empty-state {
  padding: 48px 24px;
  text-align: center;
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
