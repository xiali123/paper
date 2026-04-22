<template>
  <div class="editor-stats" v-if="stats">
    <div class="stat-item" @click="showDetail = true">
      <el-icon><Document /></el-icon>
      <span class="stat-label">字符</span>
      <span class="stat-value">{{ stats.characters.toLocaleString() }}</span>
    </div>

    <div class="stat-item" @click="showDetail = true">
      <el-icon><Notebook /></el-icon>
      <span class="stat-label">单词</span>
      <span class="stat-value">{{ stats.words.toLocaleString() }}</span>
    </div>

    <div class="stat-item" @click="showDetail = true">
      <el-icon><EditPen /></el-icon>
      <span class="stat-label">行数</span>
      <span class="stat-value">{{ stats.lines.toLocaleString() }}</span>
    </div>

    <div class="stat-item math" @click="showDetail = true">
      <span class="stat-icon">∑</span>
      <span class="stat-label">公式</span>
      <span class="stat-value">{{ stats.formulas.toLocaleString() }}</span>
    </div>

    <div class="stat-item ref" @click="showDetail = true">
      <el-icon><Link /></el-icon>
      <span class="stat-label">引用</span>
      <span class="stat-value">{{ stats.references.toLocaleString() }}</span>
    </div>

    <div class="stat-item img" @click="showDetail = true">
      <el-icon><Picture /></el-icon>
      <span class="stat-label">图片</span>
      <span class="stat-value">{{ stats.images.toLocaleString() }}</span>
    </div>

    <!-- 详细统计对话框 -->
    <el-dialog
      v-model="showDetail"
      title="详细统计"
      width="600px"
      :close-on-click-modal="false"
      :z-index="9999"
      append-to-body
    >
      <div class="stats-detail">
        <div class="detail-section">
          <h4>基础统计</h4>
          <div class="stat-grid">
            <div class="stat-card">
              <div class="card-label">总字符数</div>
              <div class="card-value">{{ stats.characters.toLocaleString() }}</div>
            </div>
            <div class="stat-card">
              <div class="card-label">单词数</div>
              <div class="card-value">{{ stats.words.toLocaleString() }}</div>
            </div>
            <div class="stat-card">
              <div class="card-label">行数</div>
              <div class="card-value">{{ stats.lines.toLocaleString() }}</div>
            </div>
            <div class="stat-card">
              <div class="card-label">非空行数</div>
              <div class="card-value">{{ stats.nonEmptyLines.toLocaleString() }}</div>
            </div>
          </div>
        </div>

        <div class="detail-section">
          <h4>LaTeX 元素</h4>
          <div class="stat-grid">
            <div class="stat-card math">
              <div class="card-icon">∑</div>
              <div class="card-content">
                <div class="card-label">公式</div>
                <div class="card-value">{{ stats.formulas.toLocaleString() }}</div>
              </div>
            </div>
            <div class="stat-card ref">
              <div class="card-icon">🔗</div>
              <div class="card-content">
                <div class="card-label">引用</div>
                <div class="card-value">{{ stats.references.toLocaleString() }}</div>
              </div>
            </div>
            <div class="stat-card img">
              <div class="card-icon">🖼️</div>
              <div class="card-content">
                <div class="card-label">图片</div>
                <div class="card-value">{{ stats.images.toLocaleString() }}</div>
              </div>
            </div>
            <div class="stat-card table">
              <div class="card-icon">▦</div>
              <div class="card-content">
                <div class="card-label">表格</div>
                <div class="card-value">{{ stats.tables.toLocaleString() }}</div>
              </div>
            </div>
          </div>
        </div>

        <div class="detail-section">
          <h4>文档结构</h4>
          <div class="structure-list">
            <div class="structure-item" v-for="(count, level) in stats.structure" :key="level">
              <span class="structure-level">{{ level }}</span>
              <span class="structure-count">{{ count }}</span>
            </div>
            <div class="structure-item" v-if="Object.keys(stats.structure).length === 0">
              <span class="structure-empty">未检测到文档结构</span>
            </div>
          </div>
        </div>

        <div class="detail-section">
          <h4>阅读时间估算</h4>
          <div class="reading-time">
            <div class="time-item">
              <span class="time-label">快速阅读</span>
              <span class="time-value">{{ stats.readingTime.fast }} 分钟</span>
            </div>
            <div class="time-item">
              <span class="time-label">正常阅读</span>
              <span class="time-value">{{ stats.readingTime.normal }} 分钟</span>
            </div>
            <div class="time-item">
              <span class="time-label">仔细阅读</span>
              <span class="time-value">{{ stats.readingTime.slow }} 分钟</span>
            </div>
          </div>
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Document, Notebook, EditPen, Link, Picture } from '@element-plus/icons-vue'

interface Stats {
  characters: number
  words: number
  lines: number
  nonEmptyLines: number
  formulas: number
  references: number
  images: number
  tables: number
  structure: Record<string, number>
  readingTime: {
    fast: number
    normal: number
    slow: number
  }
}

interface Props {
  content: string
}

const props = defineProps<Props>()
const showDetail = ref(false)

// 计算统计信息
const stats = computed<Stats>(() => {
  const content = props.content || ''

  // 基础统计
  const lines = content.split('\n')
  const characters = content.length
  const nonEmptyLines = lines.filter(line => line.trim().length > 0).length

  // 单词统计（LaTeX 文档比较特殊，需要过滤命令）
  const cleanContent = content
    .replace(/\\[a-zA-Z]+/g, '') // 移除命令
    .replace(/[{}[\]$%&#_]/g, ' ') // 替换特殊字符
    .replace(/\s+/g, ' ')
    .trim()

  const words = cleanContent ? cleanContent.split(' ').length : 0

  // 公式统计
  const inlineMath = (content.match(/\$[^$]+\$/g) || []).length
  const displayMath = (content.match(/\\[\[\(].*?\\[\]\)]/gs) || []).length
  const envMath = (content.match(/\\begin\{(equation|align|gather|multline)\}/g) || []).length
  const formulas = inlineMath + displayMath + envMath

  // 引用统计
  const citations = (content.match(/\\cite[a-z]*\{[^}]+\}/g) || []).length
  const refs = (content.match(/\\ref\{[^}]+\}/g) || []).length
  const references = citations + refs

  // 图片统计
  const images = (content.match(/\\includegraphics/g) || []).length

  // 表格统计
  const tables = (content.match(/\\begin\{tabular\}/g) || []).length

  // 文档结构
  const structure: Record<string, number> = {
    part: (content.match(/\\part\{/g) || []).length,
    chapter: (content.match(/\\chapter\{/g) || []).length,
    section: (content.match(/\\section\{/g) || []).length,
    subsection: (content.match(/\\subsection\{/g) || []).length,
    subsubsection: (content.match(/\\subsubsection\{/g) || []).length,
    paragraph: (content.match(/\\paragraph\{/g) || []).length,
  }

  // 阅读时间估算（基于单词数）
  const wordsPerMinute = {
    fast: 300,
    normal: 200,
    slow: 100
  }

  const readingTime = {
    fast: Math.max(1, Math.round(words / wordsPerMinute.fast)),
    normal: Math.max(1, Math.round(words / wordsPerMinute.normal)),
    slow: Math.max(1, Math.round(words / wordsPerMinute.slow))
  }

  return {
    characters,
    words,
    lines: lines.length,
    nonEmptyLines,
    formulas,
    references,
    images,
    tables,
    structure,
    readingTime
  }
})
</script>

<style scoped lang="scss">
.editor-stats {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px 12px;
  background: var(--el-fill-color-light);
  border-radius: 6px;
  font-size: 12px;
}

.stat-item {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px 8px;
  border-radius: 4px;
  cursor: pointer;
  transition: all 0.2s;
  color: var(--el-text-color-regular);

  .el-icon {
    font-size: 14px;
  }

  &:hover {
    background: var(--el-fill-color);
    color: var(--el-color-primary);
  }

  &.math .stat-icon {
    color: #6366f1;
  }

  &.ref .el-icon {
    color: #8b5cf6;
  }

  &.img .el-icon {
    color: #ec4899;
  }
}

.stat-icon {
  font-size: 14px;
  font-weight: bold;
}

.stat-label {
  color: var(--el-text-color-secondary);
}

.stat-value {
  font-weight: 600;
  color: var(--el-text-color-primary);
}

// 详细统计对话框
.stats-detail {
  padding: 8px 0;
}

.detail-section {
  margin-bottom: 24px;

  &:last-child {
    margin-bottom: 0;
  }

  h4 {
    font-size: 14px;
    font-weight: 600;
    color: var(--el-text-color-primary);
    margin: 0 0 12px 0;
    padding-bottom: 8px;
    border-bottom: 2px solid var(--el-border-color-light);
  }
}

.stat-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 12px;
}

.stat-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 16px;
  background: var(--el-fill-color-light);
  border-radius: 8px;
  transition: all 0.2s;

  &:hover {
    background: var(--el-fill-color);
    transform: translateY(-2px);
  }

  &.math {
    background: linear-gradient(135deg, rgba(99, 102, 241, 0.1) 0%, rgba(99, 102, 241, 0.05) 100%);
  }

  &.ref {
    background: linear-gradient(135deg, rgba(139, 92, 246, 0.1) 0%, rgba(139, 92, 246, 0.05) 100%);
  }

  &.img {
    background: linear-gradient(135deg, rgba(236, 72, 153, 0.1) 0%, rgba(236, 72, 153, 0.05) 100%);
  }

  &.table {
    background: linear-gradient(135deg, rgba(14, 165, 233, 0.1) 0%, rgba(14, 165, 233, 0.05) 100%);
  }
}

.stat-card:not(.math):not(.ref):not(.img):not(.table) {
  .card-value {
    color: var(--el-color-primary);
  }
}

.card-icon {
  font-size: 24px;
  margin-bottom: 8px;
}

.card-content {
  text-align: center;
}

.card-label {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin-bottom: 4px;
}

.card-value {
  font-size: 20px;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

// 文档结构列表
.structure-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.structure-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px 16px;
  background: var(--el-fill-color-light);
  border-radius: 6px;
}

.structure-level {
  font-size: 13px;
  color: var(--el-text-color-regular);
  text-transform: capitalize;
}

.structure-count {
  font-size: 14px;
  font-weight: 600;
  color: var(--el-color-primary);
}

.structure-empty {
  font-size: 13px;
  color: var(--el-text-color-secondary);
  font-style: italic;
}

// 阅读时间
.reading-time {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.time-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: var(--el-fill-color-light);
  border-radius: 6px;
}

.time-label {
  font-size: 13px;
  color: var(--el-text-color-regular);
}

.time-value {
  font-size: 14px;
  font-weight: 600;
  color: var(--el-color-primary);
}
</style>
