<template>
  <div class="submission-checker">
    <div class="checker-header">
      <h4>提交检查</h4>
      <el-space>
        <el-tag
          v-if="results.length > 0"
          :type="errorCount > 0 ? 'danger' : warningCount > 0 ? 'warning' : 'success'"
          size="small"
        >{{ summaryText }}</el-tag>
        <el-button size="small" type="primary" @click="runChecks" :loading="checking">检查</el-button>
      </el-space>
    </div>

    <div v-if="results.length > 0" class="checker-body">
      <div class="filter-bar">
        <el-radio-group v-model="filter" size="small">
          <el-radio-button value="all">全部</el-radio-button>
          <el-radio-button value="error">错误</el-radio-button>
          <el-radio-button value="warning">警告</el-radio-button>
          <el-radio-button value="info">建议</el-radio-button>
          <el-radio-button value="passed">通过</el-radio-button>
        </el-radio-group>
      </div>
      <div class="check-list">
        <div v-for="item in filteredResults" :key="item.id"
          class="check-item" :class="[`severity-${item.severity}`, { 'is-passed': item.passed }]">
          <div class="check-indicator">
            <el-icon v-if="item.passed" class="icon-pass"><CircleCheckFilled /></el-icon>
            <el-icon v-else class="icon-fail"><CircleCloseFilled /></el-icon>
          </div>
          <div class="check-content">
            <div class="check-title">
              <span class="check-label">{{ item.label }}</span>
              <el-tag :type="tagType(item.severity, item.passed)" size="small" effect="light">
                {{ item.passed ? '通过' : sevLabel(item.severity) }}
              </el-tag>
            </div>
            <div v-if="!item.passed && item.detail" class="check-detail">{{ item.detail }}</div>
          </div>
          <el-button v-if="!item.passed && item.fix" size="small" type="primary" plain
            @click="emit('fix', item.fix)">修复</el-button>
        </div>
      </div>
    </div>

    <div v-else class="checker-empty">
      <el-empty description="点击检查按钮开始提交前检查" :image-size="80">
        <template #description><p>检查文档结构、引用、环境匹配等常见问题</p></template>
      </el-empty>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { CircleCheckFilled, CircleCloseFilled } from '@element-plus/icons-vue'

type Severity = 'error' | 'warning' | 'info'

interface CheckResult {
  id: string
  label: string
  severity: Severity
  passed: boolean
  detail?: string
  fix?: { type: string; replacement: string }
}

const props = defineProps<{ content: string }>()
const emit = defineEmits<{ fix: [payload: { type: string; replacement: string }] }>()

const checking = ref(false)
const results = ref<CheckResult[]>([])
const filter = ref<'all' | 'error' | 'warning' | 'info' | 'passed'>('all')

const errorCount = computed(() => results.value.filter(r => !r.passed && r.severity === 'error').length)
const warningCount = computed(() => results.value.filter(r => !r.passed && r.severity === 'warning').length)
const passedCount = computed(() => results.value.filter(r => r.passed).length)
const summaryText = computed(() =>
  errorCount.value > 0 ? `${errorCount.value} 个错误` :
  warningCount.value > 0 ? `${warningCount.value} 个警告` : `${passedCount.value} 项通过`
)
const filteredResults = computed(() => {
  if (filter.value === 'all') return results.value
  if (filter.value === 'passed') return results.value.filter(r => r.passed)
  return results.value.filter(r => !r.passed && r.severity === filter.value)
})

function tagType(s: Severity, p: boolean) { return p ? 'success' : s === 'error' ? 'danger' : s }
function sevLabel(s: Severity) { return s === 'error' ? '错误' : s === 'warning' ? '警告' : '建议' }

// ---------- Check helpers ----------
function stripComments(src: string) { return src.replace(/(?<!\\)%.*$/gm, '') }

function countItems(arr: string[]): Record<string, number> {
  const m: Record<string, number> = {}
  for (const k of arr) m[k] = (m[k] ?? 0) + 1
  return m
}

function escRe(s: string) { return s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') }

function makeCheck(id: string, label: string, severity: Severity, passed: boolean,
  detail?: string, fix?: { type: string; replacement: string }): CheckResult {
  return { id, label, severity, passed, detail: passed ? undefined : detail, fix: passed ? undefined : fix }
}

// ---------- Individual checks ----------
function checkDocumentclass(s: string) {
  const ok = /\\documentclass/.test(s)
  return makeCheck('documentclass', '\\documentclass 声明', 'error', ok,
    '缺少 \\documentclass 声明，文档无法编译。', { type: 'prepend', replacement: '\\documentclass{article}\n' })
}

function checkTitle(s: string) {
  const ok = /\\title\{/.test(s)
  return makeCheck('title', '\\title{} 标题', 'error', ok,
    '缺少 \\title{} 命令。', { type: 'insert-before-preamble-end', replacement: '\\title{Your Title Here}\n' })
}

function checkAuthor(s: string) {
  const ok = /\\author\{/.test(s)
  return makeCheck('author', '\\author{} 作者', 'warning', ok,
    '缺少 \\author{} 命令。', { type: 'insert-before-preamble-end', replacement: '\\author{Author Name}\n' })
}

function checkAbstract(s: string) {
  const ok = /\\begin\{abstract\}/.test(s)
  return makeCheck('abstract', 'abstract 环境', 'warning', ok,
    '缺少 abstract 环境，多数期刊要求提供摘要。',
    { type: 'insert-after-begin-document', replacement: '\\begin{abstract}\nYour abstract here.\n\\end{abstract}\n' })
}

function checkBibliography(s: string) {
  const ok = /\\bibliography\{|\\addbibresource\{|\\bibliographystyle\{/.test(s)
  return makeCheck('bibliography', '参考文献配置', 'warning', ok,
    '缺少 \\bibliography 或 \\addbibresource 命令，引用将无法解析。',
    { type: 'append', replacement: '\n\\bibliographystyle{plain}\n\\bibliography{references}\n' })
}

function checkEnvironmentMatch(src: string) {
  const clean = src.split('\n').map(l => l.replace(/(?<!\\)%.*/, '')).join('\n')
  const begins = [...clean.matchAll(/\\begin\{(\w+)\*?\}/g)].map(m => m[1])
  const ends = [...clean.matchAll(/\\end\{(\w+)\*?\}/g)].map(m => m[1])
  const bc = countItems(begins), ec = countItems(ends)
  const mm: string[] = []
  for (const env of new Set([...Object.keys(bc), ...Object.keys(ec)])) {
    if ((bc[env] ?? 0) !== (ec[env] ?? 0)) mm.push(`${env} (\\begin:${bc[env] ?? 0}, \\end:${ec[env] ?? 0})`)
  }
  return makeCheck('env-match', '\\begin{}/\\end{} 环境匹配', 'error', mm.length === 0,
    `环境不匹配: ${mm.join('; ')}`)
}

function checkUndefinedRefs(src: string) {
  const s = stripComments(src)
  const labels = new Set([...s.matchAll(/\\label\{([^}]+)\}/g)].map(m => m[1]))
  const refs = [...s.matchAll(/\\(?:ref|eqref|autoref|pageref|cref|Cref|nameref)\{([^}]+)\}/g)].map(m => m[1])
  const cites = [...s.matchAll(/\\(?:cite|citep|citet|citeauthor|citeyear|nocite|textcite|parencite|autocite)\{([^}]+)\}/g)]
    .flatMap(m => m[1].split(',').map(x => x.trim()))
  const undefRefs = refs.filter(r => !labels.has(r))
  const hasBib = /\\bibliography\{|\\addbibresource\{/.test(s)
  const undefCites = hasBib ? [] : cites
  const parts: string[] = []
  if (undefRefs.length) parts.push(`${undefRefs.length} 个未定义的 \\ref`)
  if (undefCites.length) parts.push(`${undefCites.length} 个 \\cite 无对应参考文献`)
  return makeCheck('undefined-refs', '引用定义检查', undefRefs.length > 0 ? 'error' : 'warning',
    parts.length === 0, parts.join('；'))
}

function checkOrphanedFigures(src: string) {
  const s = stripComments(src)
  const figLabels = [...s.matchAll(/\\begin\{figure\}[\s\S]*?\\label\{([^}]+)\}/g)].map(m => m[1])
  const orphaned = figLabels.filter(l => !new RegExp(`\\\\(?:ref|eqref|autoref|cref|Cref)\\{${escRe(l)}\\}`).test(s))
  return makeCheck('orphaned-figures', '孤立图片检查', 'info', orphaned.length === 0,
    `${orphaned.length} 个图片未被引用: ${orphaned.join(', ')}`)
}

// ---------- Run all checks ----------
function runChecks() {
  checking.value = true
  results.value = []
  setTimeout(() => {
    const s = props.content ?? ''
    results.value = [checkDocumentclass(s), checkTitle(s), checkAuthor(s), checkAbstract(s),
      checkBibliography(s), checkEnvironmentMatch(s), checkUndefinedRefs(s), checkOrphanedFigures(s)]
    checking.value = false
  }, 50)
}

// Auto re-check on content change (debounced)
let timer: ReturnType<typeof setTimeout> | null = null
watch(() => props.content, () => {
  if (!results.value.length) return
  if (timer) clearTimeout(timer)
  timer = setTimeout(runChecks, 600)
})
</script>

<style scoped lang="scss">
.submission-checker {
  display: flex;
  flex-direction: column;
  gap: 16px;
  padding: 16px;
  height: 100%;
}
.checker-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  h4 { margin: 0; font-size: 16px; font-weight: 600; color: var(--el-text-color-primary); }
}
.checker-body { flex: 1; display: flex; flex-direction: column; gap: 12px; overflow: hidden; }
.filter-bar { flex-shrink: 0; }
.check-list {
  flex: 1; overflow-y: auto; display: flex; flex-direction: column; gap: 6px; padding-right: 4px;
  &::-webkit-scrollbar { width: 5px; }
  &::-webkit-scrollbar-track { background: var(--el-fill-color-lighter); border-radius: 3px; }
  &::-webkit-scrollbar-thumb { background: var(--el-border-color); border-radius: 3px; }
}
.check-item {
  display: flex; align-items: center; gap: 10px; padding: 10px 12px;
  border-radius: 8px; background: var(--el-fill-color-light);
  border-left: 3px solid var(--el-border-color); transition: background 0.2s;
  &:hover { background: var(--el-fill-color); }
  &.severity-error:not(.is-passed) { border-left-color: var(--el-color-danger); }
  &.severity-warning:not(.is-passed) { border-left-color: var(--el-color-warning); }
  &.severity-info:not(.is-passed) { border-left-color: var(--el-color-info); }
  &.is-passed { border-left-color: var(--el-color-success); opacity: 0.85; }
}
.check-indicator {
  flex-shrink: 0; font-size: 18px; line-height: 1;
  .icon-pass { color: var(--el-color-success); }
  .icon-fail { color: var(--el-color-danger); }
  .severity-warning:not(.is-passed) & .icon-fail { color: var(--el-color-warning); }
  .severity-info:not(.is-passed) & .icon-fail { color: var(--el-color-info); }
}
.check-content { flex: 1; min-width: 0; }
.check-title { display: flex; align-items: center; gap: 8px; }
.check-label { font-size: 13px; font-weight: 500; color: var(--el-text-color-primary); }
.check-detail { margin-top: 4px; font-size: 12px; color: var(--el-text-color-secondary); line-height: 1.4; }
.checker-empty {
  flex: 1; display: flex; align-items: center; justify-content: center;
  p { font-size: 13px; color: var(--el-text-color-secondary); margin-top: 8px; }
}

/* Dark mode */
[data-theme='dark'] .submission-checker { background: transparent; }
[data-theme='dark'] .check-item { background: rgba(40, 40, 45, 0.6); }
[data-theme='dark'] .check-item:hover { background: rgba(50, 50, 55, 0.8); }
[data-theme='dark'] .check-label { color: #e5e7eb; }
[data-theme='dark'] .check-detail { color: #9ca3af; }
</style>
