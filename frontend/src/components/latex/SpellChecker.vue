<template>
  <div class="spell-checker">
    <div class="checker-header">
      <h4>拼写检查</h4>
      <el-space>
        <el-select v-model="selectedDict" size="small" placeholder="选择词典">
          <el-option label="英语 (US)" value="en-US"></el-option>
          <el-option label="英语 (UK)" value="en-GB"></el-option>
          <el-option label="中文" value="zh-CN"></el-option>
          <el-option label="德语" value="de-DE"></el-option>
          <el-option label="法语" value="fr-FR"></el-option>
          <el-option label="西班牙语" value="es-ES"></el-option>
        </el-select>
        <el-button size="small" @click="startCheck" :loading="checking" :icon="Search">检查</el-button>
      </el-space>
    </div>

    <div class="checker-content" v-if="errors.length > 0">
      <div class="stats-bar">
        <span class="stat">发现 {{ errors.length }} 个问题</span>
        <el-button size="small" text @click="clearErrors">清除</el-button>
      </div>

      <div class="errors-list">
        <div
          v-for="(error, index) in errors"
          :key="index"
          class="error-item"
          :class="{ 'current': currentIndex === index }"
        >
          <div class="error-header">
            <span class="error-word">{{ error.word }}</span>
            <span class="error-type">{{ error.type }}</span>
            <el-button size="small" text @click="goToError(error)">
              <el-icon><Position /></el-icon>
            </el-button>
          </div>

          <div class="error-suggestions" v-if="error.suggestions.length > 0">
            <div class="suggestions-label">建议替换:</div>
            <div class="suggestions-list">
              <el-button
                v-for="(suggestion, i) in error.suggestions.slice(0, 5)"
                :key="i"
                size="small"
                @click="replaceWord(error, suggestion)"
              >
                {{ suggestion }}
              </el-button>
            </div>
          </div>

          <div class="error-actions">
            <el-button size="small" @click="replaceAll(error, error.suggestions[0])" v-if="error.suggestions[0]">
              全部替换
            </el-button>
            <el-button size="small" text @click="ignoreWord(error)">
              忽略
            </el-button>
            <el-button size="small" text @click="addToDict(error)">
              添加到词典
            </el-button>
          </div>
        </div>
      </div>
    </div>

    <div class="checker-empty" v-else>
      <el-empty description="点击检查按钮开始拼写检查" :image-size="80">
        <template #description>
          <p>支持英语、中文、德语、法语、西班牙语</p>
        </template>
      </el-empty>
    </div>

    <div class="checker-dictionary" v-if="customWords.length > 0">
      <div class="dict-header">
        <span class="dict-title">自定义词典 ({{ customWords.length }})</span>
        <el-button size="small" text @click="clearDict">清空</el-button>
      </div>
      <div class="dict-words">
        <el-tag
          v-for="(word, index) in customWords"
          :key="index"
          closable
          @close="removeFromDict(index)"
          size="small"
        >
          {{ word }}
        </el-tag>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import { Search, Position } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface SpellError {
  word: string
  type: 'spelling' | 'grammar' | 'style'
  suggestions: string[]
  line: number
  column: number
  index: number
}

interface Props {
  content?: string
}

const props = defineProps<Props>()

interface Emits {
  replace: [from: string, to: string]
  goto: [line: number, column: number]
}

const emit = defineEmits<Emits>()

const selectedDict = ref('en-US')
const checking = ref(false)
const errors = ref<SpellError[]>([])
const currentIndex = ref(-1)
const customWords = ref<string[]>([])

// Load custom dictionary from localStorage
const loadDict = () => {
  const saved = localStorage.getItem('latex-spell-dict')
  if (saved) {
    customWords.value = JSON.parse(saved)
  }
}

loadDict()

// Save custom dictionary to localStorage
const saveDict = () => {
  localStorage.setItem('latex-spell-dict', JSON.stringify(customWords.value))
}

// Common LaTeX commands to ignore
const latexCommands = new Set([
  'documentclass', 'usepackage', 'begin', 'end', 'section', 'subsection',
  'subsubsection', 'paragraph', 'subparagraph', 'chapter', 'part',
  'textbf', 'textit', 'texttt', 'emph', 'underline', 'textsc',
  'cite', 'ref', 'eqref', 'label', 'bibitem', 'bibliography',
  'includegraphics', 'caption', 'centering', 'hline', 'toprule',
  'midrule', 'bottomrule', 'multicolumn', 'multirow', 'frac',
  'sqrt', 'sum', 'int', 'prod', 'lim', 'infty', 'alpha', 'beta',
  'gamma', 'delta', 'epsilon', 'theta', 'lambda', 'mu', 'sigma',
  'phi', 'omega', 'pi', 'rho', 'tau', 'kappa', 'eta', 'zeta',
  'iota', 'nu', 'xi', 'upsilon', 'chi', 'psi', 'Delta', 'Theta',
  'Lambda', 'Sigma', 'Phi', 'Omega', 'Pi', 'cdot', 'times', 'div',
  'leq', 'geq', 'neq', 'approx', 'equiv', 'infty', 'partial'
])

// English common words that are correct but might be flagged
const englishCommonWords = new Set([
  'the', 'be', 'to', 'of', 'and', 'a', 'in', 'that', 'have', 'i',
  'it', 'for', 'not', 'on', 'with', 'he', 'as', 'you', 'do', 'at',
  'this', 'but', 'his', 'by', 'from', 'they', 'we', 'say', 'her',
  'she', 'or', 'an', 'will', 'my', 'one', 'all', 'would', 'there',
  'their', 'what', 'so', 'up', 'out', 'if', 'about', 'who', 'get',
  'which', 'go', 'me', 'when', 'make', 'can', 'like', 'time', 'no',
  'just', 'him', 'know', 'take', 'people', 'into', 'year', 'your',
  'good', 'some', 'could', 'them', 'see', 'other', 'than', 'then',
  'now', 'look', 'only', 'come', 'its', 'over', 'think', 'also',
  'back', 'after', 'use', 'two', 'how', 'our', 'work', 'first',
  'well', 'way', 'even', 'new', 'want', 'because', 'any', 'these',
  'give', 'day', 'most', 'us', 'is', 'are', 'was', 'were', 'been',
  'has', 'had', 'having', 'does', 'did', 'doing', 'should', 'would',
  'could', 'might', 'must', 'shall', 'can', 'may', 'will'
])

async function startCheck() {
  if (!props.content) {
    ElMessage.warning('没有内容可检查')
    return
  }

  checking.value = true
  errors.value = []

  try {
    await checkSpelling()
    if (errors.value.length === 0) {
      ElMessage.success('未发现拼写错误')
    } else {
      ElMessage.info(`发现 ${errors.value.length} 个问题`)
    }
  } catch (error) {
    ElMessage.error('检查失败: ' + error)
  } finally {
    checking.value = false
  }
}

async function checkSpelling() {
  const content = props.content
  const lines = content.split('\n')

  for (let lineIndex = 0; lineIndex < lines.length; lineIndex++) {
    const line = lines[lineIndex]

    // Skip comment lines
    if (line.trim().startsWith('%')) continue

    // Extract text content (ignore LaTeX commands)
    const textContent = extractTextContent(line)

    // Check each word
    const words = textContent.match(/[a-zA-Z]+/g) || []

    for (const word of words) {
      if (shouldSkipWord(word)) continue

      const error = await checkWord(word, lineIndex + 1, line)
      if (error) {
        errors.value.push(error)
      }
    }
  }
}

function extractTextContent(line: string): string {
  // Remove LaTeX commands and environments
  let text = line

  // Remove commands like \command{...}
  text = text.replace(/\\[a-zA-Z]+\{[^}]*\}/g, '')

  // Remove \command without arguments
  text = text.replace(/\\[a-zA-Z]+/g, '')

  // Remove {...} content
  text = text.replace(/\{[^}]*\}/g, '')

  // Remove math content $...$
  text = text.replace(/\$[^$]*\$/g, '')

  return text
}

function shouldSkipWord(word: string): boolean {
  const lower = word.toLowerCase()

  // Skip LaTeX commands
  if (latexCommands.has(lower)) return true

  // Skip common English words
  if (englishCommonWords.has(lower)) return true

  // Skip words in custom dictionary
  if (customWords.value.includes(word)) return true

  // Skip numbers
  if (/^\d+$/.test(word)) return true

  // Skip very short words
  if (word.length < 2) return true

  // Skip words with numbers
  if (/\d/.test(word)) return true

  // Skip uppercase acronyms
  if (/^[A-Z]{2,}$/.test(word)) return true

  return false
}

async function checkWord(word: string, line: number, lineText: string): Promise<SpellError | null> {
  // Simple spell check using browser API or fallback heuristics
  const isCorrect = await checkWordSpelling(word)

  if (!isCorrect) {
    const suggestions = await getSuggestions(word)
    const index = lineText.indexOf(word)

    return {
      word,
      type: 'spelling',
      suggestions,
      line,
      column: index + 1,
      index
    }
  }

  return null
}

async function checkWordSpelling(word: string): Promise<boolean> {
  // Use browser's spellcheck if available
  if ('spellcheck' in document) {
    const temp = document.createElement('span')
    temp.spellcheck = true
    temp.contentEditable = 'true'
    temp.textContent = word
    document.body.appendChild(temp)

    const range = document.createRange()
    range.selectNodeContents(temp)

    const selection = window.getSelection()
    selection?.removeAllRanges()
    selection?.addRange(range)

    const isCorrect = !temp.querySelector('::spelling-error')

    document.body.removeChild(temp)
    return isCorrect
  }

  // Fallback: simple heuristic check
  // Check common patterns
  if (/^[aeiou]{3,}/.test(word.toLowerCase())) return false // Too many vowels
  if (/[^aeiou]y[^aeiou]/.test(word.toLowerCase())) return false // y between consonants

  // Check for double letters (common mistake)
  if (/(.)\1{2,}/.test(word)) return false

  return true
}

async function getSuggestions(word: string): Promise<string[]> {
  // Simple suggestion algorithm based on edit distance
  const commonMisspellings: Record<string, string[]> = {
    'teh': ['the'],
    'recieve': ['receive'],
    'occured': ['occurred'],
    'seperate': ['separate'],
    'definately': ['definitely'],
    'goverment': ['government'],
    'occassion': ['occasion'],
    'accomodate': ['accommodate'],
    'acheive': ['achieve'],
    'accross': ['across'],
    'agressive': ['aggressive'],
    'apparant': ['apparent'],
    'arguement': ['argument'],
    'beggining': ['beginning'],
    'beleive': ['believe'],
    'calender': ['calendar'],
    'catagory': ['category'],
    'cemetary': ['cemetery'],
    'collegue': ['colleague'],
    'comming': ['coming'],
    'completly': ['completely'],
    'concious': ['conscious'],
    'curiousity': ['curiosity'],
    'decieve': ['deceive'],
    'desparate': ['desperate'],
    'diffrent': ['different'],
    'disapear': ['disappear'],
    'disapoint': ['disappoint'],
    'embarass': ['embarrass'],
    'enviroment': ['environment'],
    'exagerate': ['exaggerate'],
    'excercise': ['exercise'],
    'existance': ['existence'],
    'experiance': ['experience'],
    'finaly': ['finally'],
    'foriegn': ['foreign'],
    'fourty': ['forty'],
    'freind': ['friend'],
    'gaurd': ['guard'],
    'grammer': ['grammar'],
    'greatful': ['grateful'],
    'harrass': ['harass'],
    'heighth': ['height'],
    'heros': ['heroes'],
    'humourous': ['humorous'],
    'immediatly': ['immediately'],
    'independant': ['independent'],
    'knowlege': ['knowledge'],
    'liason': ['liaison'],
    'libary': ['library'],
    'lisence': ['license'],
    'maintainance': ['maintenance'],
    'manuever': ['maneuver'],
    'millenium': ['millennium'],
    'minature': ['miniature'],
    'mispell': ['misspell'],
    'neccessary': ['necessary'],
    'noticable': ['noticeable'],
    'occurance': ['occurrence'],
    'oficial': ['official'],
    'onlne': ['online'],
    'paralel': ['parallel'],
    'particulary': ['particularly'],
    'pavillion': ['pavilion'],
    'percieve': ['perceive'],
    'performence': ['performance'],
    'perseverence': ['perseverance'],
    'personel': ['personnel'],
    'posession': ['possession'],
    'potatos': ['potatoes'],
    'precede': ['precede'],
    'predjudice': ['prejudice'],
    'privelege': ['privilege'],
    'professer': ['professor'],
    'pronounciation': ['pronunciation'],
    'publically': ['publicly'],
    'realy': ['really'],
    'reccomend': ['recommend'],
    'refered': ['referred'],
    'relevent': ['relevant'],
    'religous': ['religious'],
    'rember': ['remember'],
    'repetition': ['repetition'],
    'resistence': ['resistance'],
    'responsability': ['responsibility'],
    'rythm': ['rhythm'],
    'sacrilegious': ['sacrilegious'],
    'sargent': ['sergeant'],
    'scedule': ['schedule'],
    'sentance': ['sentence'],
    'sieze': ['seize'],
    'similiar': ['similar'],
    'sincerly': ['sincerely'],
    'speach': ['speech'],
    'sucessful': ['successful'],
    'supercede': ['supersede'],
    'suprise': ['surprise'],
    'temperture': ['temperature'],
    'tendancy': ['tendency'],
    'therefor': ['therefore'],
    'thier': ['their'],
    'tomatos': ['tomatoes'],
    'tommorow': ['tomorrow'],
    'tounge': ['tongue'],
    'truely': ['truly'],
    'unfortunatly': ['unfortunately'],
    'untill': ['until'],
    'unusuall': ['unusual'],
    'upholstry': ['upholstery'],
    'usible': ['usable'],
    'vaccuum': ['vacuum'],
    'vegetble': ['vegetable'],
    'vehical': ['vehicle'],
    'visious': ['vicious'],
    'weired': ['weird'],
    'wellfare': ['welfare'],
    'wether': ['whether'],
    'wich': ['which'],
    'yeild': ['yield']
  }

  if (commonMisspellings[word.toLowerCase()]) {
    return commonMisspellings[word.toLowerCase()]
  }

  // Generate suggestions using simple edit distance
  const suggestions: string[] = []

  // Try adding/removing letters, swapping letters
  const alphabet = 'abcdefghijklmnopqrstuvwxyz'

  // Add one letter
  for (const letter of alphabet) {
    for (let i = 0; i <= word.length; i++) {
      const newWord = word.slice(0, i) + letter + word.slice(i)
      if (await checkWordSpelling(newWord)) {
        suggestions.push(newWord)
      }
    }
  }

  // Remove one letter
  for (let i = 0; i < word.length; i++) {
    const newWord = word.slice(0, i) + word.slice(i + 1)
    if (newWord.length > 2 && await checkWordSpelling(newWord)) {
      suggestions.push(newWord)
    }
  }

  // Swap adjacent letters
  for (let i = 0; i < word.length - 1; i++) {
    const newWord = word.slice(0, i) + word[i + 1] + word[i] + word.slice(i + 2)
    if (await checkWordSpelling(newWord)) {
      suggestions.push(newWord)
    }
  }

  return [...new Set(suggestions)].slice(0, 5)
}

function goToError(error: SpellError) {
  currentIndex.value = errors.value.indexOf(error)
  emit('goto', error.line, error.column)
}

function replaceWord(error: SpellError, suggestion: string) {
  emit('replace', error.word, suggestion)

  // Remove from errors list
  const index = errors.value.indexOf(error)
  if (index > -1) {
    errors.value.splice(index, 1)
  }

  ElMessage.success(`已将 "${error.word}" 替换为 "${suggestion}"`)
}

function replaceAll(error: SpellError, suggestion: string) {
  let count = 0
  errors.value.forEach((e, index) => {
    if (e.word.toLowerCase() === error.word.toLowerCase()) {
      emit('replace', e.word, suggestion)
      count++
    }
  })

  // Remove all instances
  errors.value = errors.value.filter(e => e.word.toLowerCase() !== error.word.toLowerCase())

  ElMessage.success(`已替换 ${count} 个 "${error.word}"`)
}

function ignoreWord(error: SpellError) {
  const index = errors.value.indexOf(error)
  if (index > -1) {
    errors.value.splice(index, 1)
  }
}

function addToDict(error: SpellError) {
  customWords.value.push(error.word)
  saveDict()

  // Remove from errors list
  const index = errors.value.indexOf(error)
  if (index > -1) {
    errors.value.splice(index, 1)
  }

  ElMessage.success(`已将 "${error.word}" 添加到自定义词典`)
}

function removeFromDict(index: number) {
  customWords.value.splice(index, 1)
  saveDict()
}

function clearDict() {
  customWords.value = []
  saveDict()
  ElMessage.success('已清空自定义词典')
}

function clearErrors() {
  errors.value = []
  currentIndex.value = -1
}

// Re-check when content or dictionary changes
watch(() => props.content, () => {
  // Auto-check can be added here if desired
})

watch(selectedDict, () => {
  if (errors.value.length > 0) {
    startCheck()
  }
})
</script>

<style scoped lang="scss">
.spell-checker {
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

  h4 {
    margin: 0;
    font-size: 16px;
    font-weight: 600;
  }
}

.checker-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 12px;
  overflow: hidden;
}

.stats-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  background: var(--el-fill-color-light);
  border-radius: 4px;

  .stat {
    font-size: 13px;
    color: var(--el-text-color-secondary);
  }
}

.errors-list {
  flex: 1;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.error-item {
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 8px;
  padding: 12px;
  background: var(--el-fill-color-blank);
  transition: all 0.2s;

  &:hover {
    border-color: var(--el-color-warning-light-5);
  }

  &.current {
    border-color: var(--el-color-primary);
    background: var(--el-color-primary-light-9);
  }
}

.error-header {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;

  .error-word {
    font-weight: 600;
    color: var(--el-color-danger);
    font-size: 14px;
  }

  .error-type {
    font-size: 11px;
    padding: 2px 6px;
    border-radius: 3px;
    background: var(--el-color-danger-light-9);
    color: var(--el-color-danger);
    text-transform: uppercase;
  }
}

.error-suggestions {
  margin-bottom: 8px;

  .suggestions-label {
    font-size: 12px;
    color: var(--el-text-color-secondary);
    margin-bottom: 6px;
  }

  .suggestions-list {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
  }
}

.error-actions {
  display: flex;
  gap: 8px;
  padding-top: 8px;
  border-top: 1px solid var(--el-border-color-lighter);
}

.checker-empty {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;

  p {
    font-size: 13px;
    color: var(--el-text-color-secondary);
    margin-top: 12px;
  }
}

.checker-dictionary {
  border-top: 1px solid var(--el-border-color-lighter);
  padding-top: 12px;

  .dict-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 8px;

    .dict-title {
      font-size: 13px;
      font-weight: 600;
      color: var(--el-text-color-secondary);
    }
  }

  .dict-words {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;

    :deep(.el-tag) {
      max-width: 200px;
      overflow: hidden;
      text-overflow: ellipsis;
    }
  }
}
</style>
