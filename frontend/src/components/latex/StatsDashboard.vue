<template>
  <el-drawer
    v-model="visible"
    title="文档统计"
    direction="rtl"
    size="700px"
    class="stats-drawer"
  >
    <div class="stats-content">
      <!-- 概览卡片 -->
      <div class="stats-overview">
        <div class="overview-card">
          <div class="card-icon">📄</div>
          <div class="card-content">
            <div class="card-value">{{ formatNumber(stats.totalChars) }}</div>
            <div class="card-label">总字符数</div>
          </div>
        </div>

        <div class="overview-card">
          <div class="card-icon">📝</div>
          <div class="card-content">
            <div class="card-value">{{ formatNumber(stats.totalWords) }}</div>
            <div class="card-label">单词数</div>
          </div>
        </div>

        <div class="overview-card">
          <div class="card-icon">📊</div>
          <div class="card-content">
            <div class="card-value">{{ stats.totalPages || 1 }}</div>
            <div class="card-label">预计页数</div>
          </div>
        </div>

        <div class="overview-card">
          <div class="card-icon">⏱️</div>
          <div class="card-content">
            <div class="card-value">{{ stats.readingTime }}</div>
            <div class="card-label">阅读时间</div>
          </div>
        </div>
      </div>

      <!-- 详细统计 -->
      <el-tabs v-model="activeTab" class="stats-tabs">
        <!-- 基础统计 -->
        <el-tab-pane label="基础" name="basic">
          <div class="stats-section">
            <h4>内容统计</h4>
            <div class="stats-grid">
              <div class="stat-card">
                <div class="stat-label">字符数</div>
                <div class="stat-value">{{ formatNumber(stats.totalChars) }}</div>
              </div>
              <div class="stat-card">
                <div class="stat-label">单词数</div>
                <div class="stat-value">{{ formatNumber(stats.totalWords) }}</div>
              </div>
              <div class="stat-card">
                <div class="stat-label">行数</div>
                <div class="stat-value">{{ formatNumber(stats.totalLines) }}</div>
              </div>
              <div class="stat-card">
                <div class="stat-label">非空行</div>
                <div class="stat-value">{{ formatNumber(stats.nonEmptyLines) }}</div>
              </div>
              <div class="stat-card">
                <div class="stat-label">段落数</div>
                <div class="stat-value">{{ formatNumber(stats.paragraphs) }}</div>
              </div>
              <div class="stat-card">
                <div class="stat-label">句子数</div>
                <div class="stat-value">{{ formatNumber(stats.sentences) }}</div>
              </div>
            </div>
          </div>
        </el-tab-pane>

        <!-- LaTeX元素 -->
        <el-tab-pane label="LaTeX元素" name="latex">
          <div class="stats-section">
            <h4>LaTeX 元素统计</h4>
            <div class="latex-stats-grid">
              <div class="latex-stat-item">
                <div class="latex-stat-icon" style="color: #6366f1;">∑</div>
                <div class="latex-stat-content">
                  <div class="latex-stat-label">公式</div>
                  <div class="latex-stat-value">{{ stats.formulas }}</div>
                  <div class="latex-stat-detail">
                    行内: {{ stats.inlineFormulas }} |
                    显示: {{ stats.displayFormulas }}
                  </div>
                </div>
              </div>

              <div class="latex-stat-item">
                <div class="latex-stat-icon" style="color: #8b5cf6;">🔗</div>
                <div class="latex-stat-content">
                  <div class="latex-stat-label">引用</div>
                  <div class="latex-stat-value">{{ stats.references }}</div>
                  <div class="latex-stat-detail">
                    文献: {{ stats.citations }} |
                    标签: {{ stats.refs }}
                  </div>
                </div>
              </div>

              <div class="latex-stat-item">
                <div class="latex-stat-icon" style="color: #ec4899;">🖼️</div>
                <div class="latex-stat-content">
                  <div class="latex-stat-label">图片</div>
                  <div class="latex-stat-value">{{ stats.images }}</div>
                  <div class="latex-stat-detail">
                    figure 环境: {{ stats.figures }}
                  </div>
                </div>
              </div>

              <div class="latex-stat-item">
                <div class="latex-stat-icon" style="color: #14b8a6;">▦</div>
                <div class="latex-stat-content">
                  <div class="latex-stat-label">表格</div>
                  <div class="latex-stat-value">{{ stats.tables }}</div>
                  <div class="latex-stat-detail">
                    tabular 环境: {{ stats.tabulars }}
                  </div>
                </div>
              </div>

              <div class="latex-stat-item">
                <div class="latex-stat-icon" style="color: #f59e0b;">📚</div>
                <div class="latex-stat-content">
                  <div class="latex-stat-label">环境</div>
                  <div class="latex-stat-value">{{ stats.environments }}</div>
                  <div class="latex-stat-detail">
                    自定义环境: {{ stats.customEnvs || 0 }}
                  </div>
                </div>
              </div>

              <div class="latex-stat-item">
                <div class="latex-stat-icon" style="color: #10b981;">📦</div>
                <div class="latex-stat-content">
                  <div class="latex-stat-label">包</div>
                  <div class="latex-stat-value">{{ stats.packages }}</div>
                  <div class="latex-stat-detail">
                    宏包: {{ stats.macros || 0 }}
                  </div>
                </div>
              </div>
            </div>
          </div>

          <!-- 命令使用频率 -->
          <h4>常用命令</h4>
          <div class="command-frequency">
            <div
              v-for="cmd in topCommands"
              :key="cmd.command"
              class="command-item"
            >
              <code class="command-code">{{ cmd.command }}</code>
              <el-progress
                :percentage="(cmd.count / stats.totalCommands) * 100"
                :show-text="false"
                :stroke-width="4"
              />
              <span class="command-count">{{ cmd.count }}次</span>
            </div>
          </div>
        </el-tab-pane>

        <!-- 文档结构 -->
        <el-tab-pane label="结构" name="structure">
          <div class="stats-section">
            <h4>文档结构</h4>
            <div class="structure-tree">
              <div
                v-for="(level, name) in Object.entries(stats.structure || {})"
                :key="name"
                v-if="level.count > 0"
                class="structure-item"
                :style="{ marginLeft: getStructureMargin(name) }"
              >
                <div class="structure-level">{{ getStructureLabel(name) }}</div>
                <div class="structure-count">{{ level.count }}</div>
              </div>

              <el-empty
                v-if="!stats.structure || Object.keys(stats.structure).every(k => stats.structure[k] === 0)"
                description="未检测到文档结构"
                :image-size="60"
              />
            </div>
          </div>
        </el-tab-pane>

        <!-- 阅读分析 -->
        <el-tab-pane label="阅读分析" name="reading">
          <div class="stats-section">
            <h4>阅读时间估算</h4>
            <div class="reading-time-cards">
              <div class="reading-card fast">
                <div class="reading-icon">⚡</div>
                <div class="reading-content">
                  <div class="reading-label">快速阅读</div>
                  <div class="reading-time">{{ stats.readingTimeFast }}</div>
                  <div class="reading-desc">约 300 词/分钟</div>
                </div>
              </div>

              <div class="reading-card normal">
                <div class="reading-icon">📖</div>
                <div class="reading-content">
                  <div class="reading-label">正常阅读</div>
                  <div class="reading-time">{{ stats.readingTimeNormal }}</div>
                  <div class="reading-desc">约 200 词/分钟</div>
                </div>
              </div>

              <div class="reading-card slow">
                <div class="reading-icon">📚</div>
                <div class="reading-content">
                  <div class="reading-label">仔细阅读</div>
                  <div class="reading-time">{{ stats.readingTimeSlow }}</div>
                  <div class="reading-desc">约 100 词/分钟</div>
                </div>
              </div>
            </div>

            <h4>内容复杂度</h4>
            <div class="complexity-meter">
              <div class="complexity-label">复杂度</div>
              <el-progress
                :percentage="stats.complexity || 50"
                :format="() => getComplexityLabel(stats.complexity || 50)"
                :color="getComplexityColor(stats.complexity || 50)"
              />
            </div>

            <div class="complexity-factors">
              <div class="factor-item" :class="{ 'high': stats.formulas > 50 }">
                <span>公式数量</span>
                <el-tag :type="stats.formulas > 50 ? 'danger' : 'info'">
                  {{ stats.formulas }}
                </el-tag>
              </div>
              <div class="factor-item" :class="{ 'high': stats.totalWords > 10000 }">
                <span>文档长度</span>
                <el-tag :type="stats.totalWords > 10000 ? 'warning' : 'success'">
                  {{ stats.totalWords }} 词
                </el-tag>
              </div>
              <div class="factor-item" :class="{ 'high': stats.images > 20 }">
                <span>图片数量</span>
                <el-tag :type="stats.images > 20 ? 'warning' : 'success'">
                  {{ stats.images }}
                </el-tag>
              </div>
            </div>
          </div>
        </el-tab-pane>
      </el-tabs>

      <!-- 导出操作 -->
      <div class="stats-actions">
        <el-button @click="exportStats">
          <el-icon><Download /></el-icon>
          导出统计
        </el-button>
        <el-button @click="refreshStats">
          <el-icon><RefreshRight /></el-icon>
          刷新
        </el-button>
      </div>
    </div>
  </el-drawer>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Download, RefreshRight } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface Stats {
  totalChars: number
  totalWords: number
  totalLines: number
  nonEmptyLines: number
  paragraphs: number
  sentences: number
  totalPages: number
  readingTime: string
  formulas: number
  inlineFormulas: number
  displayFormulas: number
  references: number
  citations: number
  refs: number
  images: number
  figures: number
  tables: number
  tabulars: number
  environments: number
  customEnvs?: number
  packages: number
  macros?: number
  structure?: Record<string, number>
  topCommands?: Array<{ command: string; count: number }>
  totalCommands: number
  readingTimeFast?: string
  readingTimeNormal?: string
  readingTimeSlow?: string
  complexity?: number
}

interface Props {
  stats: Stats
  documentId?: string
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'refresh': []
  'export': [stats: Stats]
}>()

const visible = ref(false)
const activeTab = ref('basic')

const topCommands = computed(() => {
  return (props.stats.topCommands || []).slice(0, 10)
})

function formatNumber(num: number): string {
  if (num >= 1000000) {
    return `${(num / 1000000).toFixed(1)}M`
  } else if (num >= 1000) {
    return `${(num / 1000).toFixed(1)}K`
  }
  return num.toString()
}

function getStructureMargin(level: string): string {
  const depths: Record<string, number> = {
    part: 0,
    chapter: 1,
    section: 2,
    subsection: 3,
    subsubsection: 4
  }
  return `${depths[level] || 0 * 16}px`
}

function getStructureLabel(level: string): string {
  const labels: Record<string, string> = {
    part: '篇',
    chapter: '章',
    section: '节',
    subsection: '小节',
    subsubsection: '次小节',
    paragraph: '段'
  }
  return labels[level] || level
}

function getComplexityLabel(value: number): string {
  if (value < 30) return '简单'
  if (value < 50) return '中等'
  if (value < 70) return '复杂'
  return '非常复杂'
}

function getComplexityColor(value: number): string {
  if (value < 30) return '#67c23a'
  if (value < 50) return '#e6a23c'
  if (value < 70) return '#f56c6c'
  return '#f56c6c'
}

function exportStats() {
  emit('export', props.stats)
  ElMessage.success('统计数据已导出')
}

function refreshStats() {
  emit('refresh')
  ElMessage.success('统计数据已刷新')
}

const open = () => {
  visible.value = true
}

defineExpose({ open })
</script>

<style scoped lang="scss">
.stats-drawer {
  :deep(.el-drawer__body) {
    padding: 0;
    display: flex;
    flex-direction: column;
  }
}

.stats-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.stats-overview {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 16px;
  padding: 20px;
  border-bottom: 1px solid var(--el-border-color-light);
}

.overview-card {
  display: flex;
  gap: 12px;
  padding: 16px;
  background: var(--el-fill-color-light);
  border-radius: 8px;
  border: 1px solid var(--el-border-color);
  transition: all 0.3s;

  &:hover {
    background: var(--el-fill-color);
    border-color: var(--el-color-primary);
    transform: translateY(-2px);
  }

  .card-icon {
    font-size: 32px;
  }

  .card-content {
    flex: 1;
  }

  .card-value {
    font-size: 24px;
    font-weight: 600;
    color: var(--el-text-color-primary);
    line-height: 1;
    margin-bottom: 4px;
  }

  .card-label {
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }
}

.stats-tabs {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;

  :deep(.el-tabs__content) {
    flex: 1;
    overflow-y: auto;
  }
}

.stats-section {
  padding: 20px;
}

.stats-section h4 {
  font-size: 14px;
  font-weight: 600;
  color: var(--el-text-color-primary);
  margin: 0 0 16px 0;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
}

.stat-card {
  padding: 12px;
  background: var(--el-fill-color-light);
  border-radius: 6px;
  text-align: center;

  .stat-label {
    font-size: 12px;
    color: var(--el-text-color-secondary);
    margin-bottom: 4px;
  }

  .stat-value {
    font-size: 18px;
    font-weight: 600;
    color: var(--el-text-color-primary);
  }
}

// LaTeX 统计
.latex-stats-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}

.latex-stat-item {
  display: flex;
  gap: 12px;
  padding: 16px;
  background: var(--el-fill-color-light);
  border-radius: 8px;
  border-left: 4px solid transparent;

  .latex-stat-icon {
    flex-shrink: 0;
    font-size: 24px;
  }

  .latex-stat-content {
    flex: 1;
  }

  .latex-stat-label {
    font-size: 13px;
    color: var(--el-text-color-regular);
    margin-bottom: 2px;
  }

  .latex-stat-value {
    font-size: 20px;
    font-weight: 600;
    color: var(--el-text-color-primary);
    margin-bottom: 2px;
  }

  .latex-stat-detail {
    font-size: 11px;
    color: var(--el-text-color-secondary);
  }
}

// 命令频率
.command-frequency {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.command-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 0;
}

.command-code {
  font-family: 'Consolas', monospace;
  font-size: 13px;
  color: var(--el-color-primary);
  background: var(--el-fill-color);
  padding: 2px 6px;
  border-radius: 3px;
  min-width: 80px;
}

.command-item .el-progress {
  flex: 1;
}

.command-count {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  min-width: 40px;
  text-align: right;
}

// 文档结构
.structure-tree {
  background: var(--el-fill-color-light);
  border-radius: 8px;
  padding: 16px;
}

.structure-item {
  display: flex;
  justify-content: space-between;
  padding: 8px 0;
  border-bottom: 1px solid var(--el-border-color-light);

  &:last-child {
    border-bottom: none;
  }
}

.structure-level {
  font-weight: 500;
  color: var(--el-text-color-primary);
}

.structure-count {
  font-size: 13px;
  color: var(--el-text-color-secondary);
}

// 阅读分析
.reading-time-cards {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16px;
  margin-bottom: 24px;
}

.reading-card {
  display: flex;
  gap: 12px;
  padding: 16px;
  border-radius: 8px;
  border: 2px solid transparent;

  &.fast {
    background: var(--el-color-success-light-9);
    border-color: var(--el-color-success-light-8);
  }

  &.normal {
    background: var(--el-color-info-light-9);
    border-color: var(--el-color-info-light-8);
  }

  &.slow {
    background: var(--el-color-warning-light-9);
    border-color: var(--el-color-warning-light-8);
  }

  .reading-icon {
    font-size: 24px;
  }

  .reading-content {
    flex: 1;
  }

  .reading-label {
    font-size: 12px;
    color: var(--el-text-color-secondary);
    margin-bottom: 2px;
  }

  .reading-time {
    font-size: 18px;
    font-weight: 600;
    color: var(--el-text-color-primary);
    margin-bottom: 2px;
  }

  .reading-desc {
    font-size: 11px;
    color: var(--el-text-color-secondary);
  }
}

.complexity-meter {
  padding: 16px;
  background: var(--el-fill-color-light);
  border-radius: 8px;
  margin-bottom: 16px;

  .complexity-label {
    font-size: 13px;
    color: var(--el-text-color-regular);
    margin-bottom: 8px;
  }
}

.complexity-factors {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.factor-item {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  color: var(--el-text-color-regular);
  padding: 4px 8px;
  background: var(--el-fill-color);
  border-radius: 4px;

  &.high {
    background: var(--el-color-danger-light-9);
    color: var(--el-color-danger);
  }
}

// 统计操作
.stats-actions {
  display: flex;
  gap: 8px;
  padding: 16px 20px;
  border-top: 1px solid var(--el-border-color-light);
  background: var(--el-fill-color-light);
}
</style>
