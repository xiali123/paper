<template>
  <BaseDialog
    :show="show"
    title="AI 写作助手"
    width="900px"
    @update:show="$emit('update:show', $event)"
  >
    <div class="ai-assistant">
      <!-- 功能选择 -->
      <el-tabs v-model="activeTab" class="assistant-tabs">
        <!-- 智能补全 -->
        <el-tab-pane label="智能补全" name="autocomplete">
          <div class="tab-content">
            <div class="input-section">
              <el-input
                v-model="autocompleteInput"
                type="textarea"
                :rows="3"
                placeholder="输入开头，AI将帮您补全内容..."
                @input="debounceAutocomplete"
              />
              <el-button
                type="primary"
                @click="generateAutocomplete"
                :loading="generating"
                style="margin-top: 8px"
              >
                <el-icon><MagicStick /></el-icon>
                AI 补全
              </el-button>
            </div>

            <div v-if="autocompleteResult" class="result-section">
              <div class="result-header">
                <h4>AI 补全建议</h4>
                <el-button size="small" @click="insertAutocomplete">
                  <el-icon><Plus /></el-icon>
                  插入
                </el-button>
              </div>
              <div class="result-content">
                <p>{{ autocompleteResult }}</p>
              </div>
              <div class="result-metrics">
                <el-tag size="small" type="success">
                  <el-icon><Check /></el-icon>
                  置信度: {{ autocompleteConfidence }}%
                </el-tag>
              </div>
            </div>
          </div>
        </el-tab-pane>

        <!-- 语法检查 -->
        <el-tab-pane label="语法检查" name="grammar">
          <div class="tab-content">
            <div class="check-toolbar">
              <el-button type="primary" @click="checkGrammar" :loading="checking">
                <el-icon><DocumentChecked /></el-icon>
                检查语法
              </el-button>
              <el-select v-model="grammarLanguage" style="width: 120px">
                <el-option label="中文" value="zh" />
                <el-option label="英文" value="en" />
                <el-option label="学术" value="academic" />
              </el-select>
            </div>

            <div class="issues-list">
              <div
                v-for="issue in grammarIssues"
                :key="issue.id"
                class="issue-item"
                :class="`severity-${issue.severity}`"
              >
                <div class="issue-header">
                  <el-icon :class="getIssueIcon(issue.severity)">
                    <Warning v-if="issue.severity === 'error'" />
                    <InfoFilled v-else />
                  </el-icon>
                  <span class="issue-type">{{ issue.type }}</span>
                  <el-tag :type="getSeverityColor(issue.severity)" size="small">
                    {{ issue.severity }}
                  </el-tag>
                  <span class="issue-line">第{{ issue.line }}行</span>
                </div>
                <div class="issue-message">
                  <p>{{ issue.message }}</p>
                  <p v-if="issue.suggestion" class="suggestion">
                    <strong>建议:</strong> {{ issue.suggestion }}
                  </p>
                </div>
                <div class="issue-actions">
                  <el-button size="small" @click="navigateToIssue(issue.line)">
                    <el-icon><Position /></el-icon>
                    跳转
                  </el-button>
                  <el-button size="small" type="primary" @click="fixIssue(issue)">
                    <el-icon><CircleCheck /></el-icon>
                    自动修复
                  </el-button>
                </div>
              </div>
              <el-empty v-if="grammarIssues.length === 0 && !checking" description="没有发现语法问题" />
            </div>
          </div>
        </el-tab-pane>

        <!-- 润色优化 -->
        <el-tab-pane label="润色优化" name="polish">
          <div class="tab-content">
            <div class="input-section">
              <el-input
                v-model="polishInput"
                type="textarea"
                :rows="5"
                placeholder="输入需要润色的文本..."
              />
              <div class="polish-options">
                <el-select v-model="polishStyle" placeholder="润色风格">
                  <el-option label="学术正式" value="academic" />
                  <el-option label="简洁清晰" value="concise" />
                  <el-option label="详细描述" value="detailed" />
                  <el-option label="母语化" value="native" />
                </el-select>
                <el-select v-model="polishTone" placeholder="语气">
                  <el-option label="客观" value="objective" />
                  <el-option label="主观" value="subjective" />
                  <el-option label="中性" value="neutral" />
                </el-select>
              </div>
              <el-button
                type="primary"
                @click="generatePolish"
                :loading="polishing"
                style="margin-top: 8px"
              >
                <el-icon><BrushFilled /></el-icon>
                AI 润色
              </el-button>
            </div>

            <div v-if="polishResult" class="result-section">
              <div class="result-header">
                <h4>润色结果</h4>
                <div class="result-actions">
                  <el-button size="small" @click="showDiff = !showDiff">
                    <el-icon><View /></el-icon>
                    {{ showDiff ? '隐藏' : '显示' }}差异
                  </el-button>
                  <el-button size="small" @click="insertPolish">
                    <el-icon><Plus /></el-icon>
                    插入
                  </el-button>
                </div>
              </div>

              <!-- 原文对比 -->
              <div v-if="showDiff" class="diff-view">
                <div class="diff-original">
                  <h5>原文</h5>
                  <p>{{ polishInput }}</p>
                </div>
                <div class="diff-result">
                  <h5>润色后</h5>
                  <p>{{ polishResult }}</p>
                </div>
              </div>

              <!-- 直接显示结果 -->
              <div v-else class="result-content">
                <p>{{ polishResult }}</p>
              </div>

              <div class="improvements">
                <h5>改进点:</h5>
                <ul>
                  <li v-for="improvement in polishImprovements" :key="improvement">
                    {{ improvement }}
                  </li>
                </ul>
              </div>
            </div>
          </div>
        </el-tab-pane>

        <!-- 文献推荐 -->
        <el-tab-pane label="文献推荐" name="references">
          <div class="tab-content">
            <div class="search-section">
              <el-input
                v-model="referenceQuery"
                placeholder="描述研究主题，AI推荐相关文献..."
                @keyup.enter="searchReferences"
              >
                <template #append>
                  <el-button :icon="Search" @click="searchReferences" :loading="searching" />
                </template>
              </el-input>
            </div>

            <div class="references-list">
              <div
                v-for="ref in recommendedReferences"
                :key="ref.doi"
                class="reference-card"
              >
                <div class="ref-header">
                  <h5>{{ ref.title }}</h5>
                  <el-button size="small" @click="addReference(ref)">
                    <el-icon><Plus /></el-icon>
                    添加引用
                  </el-button>
                </div>
                <div class="ref-meta">
                  <p><strong>作者:</strong> {{ ref.authors }}</p>
                  <p><strong>期刊:</strong> {{ ref.journal }} ({{ ref.year }})</p>
                  <p><strong>引用:</strong> {{ ref.citationCount }} 次</p>
                </div>
                <div class="ref-abstract">
                  {{ ref.abstract }}
                </div>
                <div class="ref-actions">
                  <el-button size="small" text @click="openReference(ref)">
                    <el-icon><Link /></el-icon>
                    查看详情
                  </el-button>
                  <el-button size="small" text @click="previewReference(ref)">
                    <el-icon><View /></el-icon>
                    预览
                  </el-button>
                </div>
              </div>
              <el-empty v-if="recommendedReferences.length === 0" description="输入主题搜索文献" />
            </div>
          </div>
        </el-tab-pane>

        <!-- 翻译助手 -->
        <el-tab-pane label="翻译助手" name="translate">
          <div class="tab-content">
            <div class="translate-section">
              <el-select v-model="translateDirection" style="width: 150px; margin-bottom: 12px">
                <el-option label="中 → 英" value="zh2en" />
                <el-option label="英 → 中" value="en2zh" />
              </el-select>

              <el-input
                v-model="translateInput"
                type="textarea"
                :rows="4"
                :placeholder="translateDirection === 'zh2en' ? '输入中文...' : 'Input English...'"
              />

              <el-button
                type="primary"
                @click="translate"
                :loading="translating"
                style="margin-top: 8px"
              >
                <el-icon><Switch /></el-icon>
                AI 翻译
              </el-button>
            </div>

            <div v-if="translateResult" class="result-section">
              <div class="result-header">
                <h4>翻译结果</h4>
                <el-button size="small" @click="insertTranslate">
                  <el-icon><Plus /></el-icon>
                  插入
                </el-button>
              </div>
              <div class="result-content">
                <p>{{ translateResult }}</p>
              </div>
              <div class="alternatives" v-if="translateAlternatives.length > 0">
                <h5>其他翻译:</h5>
                <div
                  v-for="(alt, index) in translateAlternatives"
                  :key="index"
                  class="alternative-item"
                  @click="translateResult = alt"
                >
                  {{ alt }}
                </div>
              </div>
            </div>
          </div>
        </el-tab-pane>
      </el-tabs>
    </div>

    <template #footer>
      <div class="footer-actions">
        <el-button @click="$emit('update:show', false)">关闭</el-button>
        <el-button type="primary" @click="saveToHistory">
          <el-icon><Clock /></el-icon>
          保存到历史
        </el-button>
      </div>
    </template>
  </BaseDialog>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import {
  MagicStick, DocumentChecked, Warning, InfoFilled, Check, Position,
  CircleCheck, BrushFilled, View, Plus, Search, Link, Switch, Clock
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import BaseDialog from './BaseDialog.vue'

interface GrammarIssue {
  id: string
  line: number
  type: string
  severity: 'error' | 'warning' | 'info'
  message: string
  suggestion?: string
}

interface Reference {
  title: string
  authors: string
  journal: string
  year: string
  doi: string
  abstract: string
  citationCount: number
}

interface Props {
  show: boolean
  content?: string
}

const props = withDefaults(defineProps<Props>(), {
  content: ''
})

const emit = defineEmits<{
  'update:show': [value: boolean]
  'insert': [text: string]
  'navigate': [line: number]
}>()

// 状态
const activeTab = ref('autocomplete')
const generating = ref(false)
const checking = ref(false)
const polishing = ref(false)
const searching = ref(false)
const translating = ref(false)

// 智能补全
const autocompleteInput = ref('')
const autocompleteResult = ref('')
const autocompleteConfidence = ref(0)

// 语法检查
const grammarLanguage = ref('academic')
const grammarIssues = ref<GrammarIssue[]>([])

// 润色优化
const polishInput = ref('')
const polishResult = ref('')
const polishStyle = ref('academic')
const polishTone = ref('objective')
const polishImprovements = ref<string[]>([])
const showDiff = ref(false)

// 文献推荐
const referenceQuery = ref('')
const recommendedReferences = ref<Reference[]>([])

// 翻译
const translateDirection = ref('zh2en')
const translateInput = ref('')
const translateResult = ref('')
const translateAlternatives = ref<string[]>([])

// 方法
async function generateAutocomplete() {
  if (!autocompleteInput.value) {
    ElMessage.warning('请输入开头内容')
    return
  }

  generating.value = true
  try {
    // 模拟AI补全
    await new Promise(resolve => setTimeout(resolve, 1000))
    autocompleteResult.value = autocompleteInput.value + ' (AI补全的内容示例)'
    autocompleteConfidence.value = 85
    ElMessage.success('AI补全完成')
  } finally {
    generating.value = false
  }
}

function debounceAutocomplete() {
  // 防抖处理
}

async function checkGrammar() {
  checking.value = true
  try {
    // 模拟语法检查
    await new Promise(resolve => setTimeout(resolve, 1500))

    // 示例问题
    grammarIssues.value = [
      {
        id: '1',
        line: 10,
        type: '拼写错误',
        severity: 'error',
        message: '发现拼写错误: "teh" 应为 "the"',
        suggestion: '将 "teh" 替换为 "the"'
      },
      {
        id: '2',
        line: 25,
        type: '语法建议',
        severity: 'warning',
        message: '建议使用学术正式用语',
        suggestion: '将 "show" 替换为 "demonstrate"'
      }
    ]

    ElMessage.success(`检查完成，发现 ${grammarIssues.value.length} 个问题`)
  } finally {
    checking.value = false
  }
}

function getIssueIcon(severity: string): string {
  return severity === 'error' ? 'text-danger' : 'text-warning'
}

function getSeverityColor(severity: string): string {
  const colors = { error: 'danger', warning: 'warning', info: 'info' }
  return colors[severity as keyof typeof colors] || 'info'
}

function navigateToIssue(line: number) {
  emit('navigate', line)
}

function fixIssue(issue: GrammarIssue) {
  if (issue.suggestion) {
    ElMessage.success(`已修复: ${issue.message}`)
  }
}

async function generatePolish() {
  if (!polishInput.value) {
    ElMessage.warning('请输入需要润色的文本')
    return
  }

  polishing.value = true
  try {
    await new Promise(resolve => setTimeout(resolve, 2000))
    polishResult.value = polishInput.value + ' (AI润色后的内容示例)'
    polishImprovements.value = [
      '使用更正式的表达方式',
      '增加了细节描述',
      '改进了句子结构'
    ]
    ElMessage.success('润色完成')
  } finally {
    polishing.value = false
  }
}

function insertPolish() {
  emit('insert', polishResult.value)
  ElMessage.success('已插入润色内容')
}

async function searchReferences() {
  if (!referenceQuery.value) {
    ElMessage.warning('请输入研究主题')
    return
  }

  searching.value = true
  try {
    await new Promise(resolve => setTimeout(resolve, 1500))

    // 示例推荐文献
    recommendedReferences.value = [
      {
        title: 'Deep Learning for Natural Language Processing',
        authors: 'Young, T., Hazarika, D., Poria, S., & Cambria, E.',
        journal: 'IEEE Transactions on Neural Networks and Learning Systems',
        year: '2018',
        doi: '10.1109/TNNLS.2018.2848621',
        abstract: 'This paper provides a comprehensive overview of deep learning approaches...',
        citationCount: 15234
      }
    ]
  } finally {
    searching.value = false
  }
}

function addReference(ref: Reference) {
  const citeKey = ref.authors.split(',')[0].trim().split(' ')[1] + ref.year
  const bibEntry = `@article{${citeKey},
  title={${ref.title}},
  author={${ref.authors}},
  journal={${ref.journal}},
  year={${ref.year}},
  doi={${ref.doi}}
}`

  emit('insert', `\\cite{${citeKey}}`)
  ElMessage.success('已插入引用')
}

async function translate() {
  if (!translateInput.value) {
    ElMessage.warning('请输入要翻译的文本')
    return
  }

  translating.value = true
  try {
    await new Promise(resolve => setTimeout(resolve, 1500))
    translateResult.value = '(AI翻译结果)'
    translateAlternatives.value = [
      '(翻译备选方案1)',
      '(翻译备选方案2)'
    ]
  } finally {
    translating.value = false
  }
}

function insertTranslate() {
  emit('insert', translateResult.value)
}

function saveToHistory() {
  ElMessage.success('已保存到历史记录')
}
</script>

<style scoped lang="scss">
.ai-assistant {
  min-height: 500px;
}

.assistant-tabs {
  :deep(.el-tabs__content) {
    min-height: 400px;
  }
}

.tab-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.input-section {
  padding: 16px;
  background: var(--el-fill-color-blank);
  border-radius: 8px;
}

.polish-options {
  display: flex;
  gap: 8px;
  margin-top: 8px;
}

.result-section {
  padding: 16px;
  background: var(--el-fill-color-blank);
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
}

.result-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;

  h4 {
    margin: 0;
  }
}

.result-content {
  padding: 12px;
  background: var(--el-bg-color);
  border-radius: 4px;

  p {
    margin: 0;
    line-height: 1.6;
  }
}

.result-metrics {
  margin-top: 12px;
  display: flex;
  gap: 8px;
}

.check-toolbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.issues-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.issue-item {
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  padding: 12px;
  background: var(--el-fill-color-blank);

  &.severity-error {
    border-left: 3px solid var(--el-color-danger);
  }

  &.severity-warning {
    border-left: 3px solid var(--el-color-warning);
  }
}

.issue-header {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
}

.issue-type {
  font-weight: 500;
}

.issue-line {
  margin-left: auto;
  font-size: 12px;
  color: var(--el-text-color-secondary);
}

.issue-message {
  margin-bottom: 8px;

  p {
    margin: 4px 0;
  }

  .suggestion {
    color: var(--el-color-success);
  }
}

.issue-actions {
  display: flex;
  gap: 8px;
}

.diff-view {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 16px;
  margin-top: 12px;
}

.diff-original,
.diff-result {
  padding: 12px;
  background: var(--el-bg-color);
  border-radius: 4px;

  h5 {
    margin: 0 0 8px 0;
    color: var(--el-text-color-secondary);
  }

  p {
    margin: 0;
    line-height: 1.6;
  }
}

.improvements {
  margin-top: 12px;

  h5 {
    margin: 0 0 8px 0;
  }

  ul {
    margin: 0;
    padding-left: 20px;

    li {
      margin: 4px 0;
    }
  }
}

.reference-card {
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  padding: 16px;
  background: var(--el-fill-color-blank);
}

.ref-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 12px;

  h5 {
    margin: 0;
    flex: 1;
  }
}

.ref-meta {
  p {
    margin: 4px 0;
    font-size: 13px;
    color: var(--el-text-color-secondary);
  }
}

.ref-abstract {
  margin: 12px 0;
  padding: 8px;
  background: var(--el-bg-color);
  border-radius: 4px;
  font-size: 13px;
  line-height: 1.5;
}

.ref-actions {
  display: flex;
  gap: 8px;
}

.alternatives {
  margin-top: 12px;

  h5 {
    margin: 0 0 8px 0;
  }
}

.alternative-item {
  padding: 8px;
  background: var(--el-bg-color);
  border-radius: 4px;
  margin-bottom: 8px;
  cursor: pointer;
  transition: background 0.2s;

  &:hover {
    background: var(--el-fill-color-light);
  }
}

.footer-actions {
  display: flex;
  justify-content: space-between;
  width: 100%;
}
</style>
