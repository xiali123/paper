/**
 * BibTeX解析器
 * 解析.bib文件，提取文献条目
 */

export interface BibTeXEntry {
  id: string
  type: string
  fields: Map<string, string>
  raw: string
}

export interface BibTeXField {
  name: string
  value: string
  braceDepth: number
}

export type BibTeXEntryType =
  | 'article'
  | 'book'
  | 'inproceedings'
  | 'conference'
  | 'incollection'
  | 'phdthesis'
  | 'mastersthesis'
  | 'misc'
  | 'techreport'
  | 'unpublished'

/**
 * BibTeX条目类型定义（必填字段）
 */
const ENTRY_TYPE_FIELDS: Record<string, string[]> = {
  article: ['author', 'title', 'journal', 'year'],
  book: ['author', 'title', 'publisher', 'year'],
  inproceedings: ['author', 'title', 'booktitle', 'year'],
  incollection: ['author', 'title', 'booktitle', 'publisher', 'year'],
  phdthesis: ['author', 'title', 'school', 'year'],
  mastersthesis: ['author', 'title', 'school', 'year'],
  techreport: ['author', 'title', 'institution', 'year'],
  unpublished: ['author', 'title', 'note']
}

/**
 * 解析BibTeX文件内容
 */
export function parseBibTeX(content: string): BibTeXEntry[] {
  const entries: BibTeXEntry[] = []
  let currentEntry: Partial<BibTeXEntry> | null = null
  let currentField: Partial<BibTeXField> | null = null
  let braceDepth = 0
  let inEntry = false
  let inField = false

  const lines = content.split('\n')

  for (let line of lines) {
    line = line.trim()

    // 跳过空行和注释
    if (!line || line.startsWith('%')) continue

    // 开始条目
    const entryMatch = line.match(/^@(\w+)\s*\{\s*([^,]+)\s*,?\s*$/i)
    if (entryMatch && !inEntry) {
      inEntry = true
      currentEntry = {
        type: entryMatch[1].toLowerCase(),
        id: entryMatch[2],
        fields: new Map(),
        raw: line
      }
      continue
    }

    // 结束条目
    if (inEntry && line === '}' && !inField) {
      if (currentEntry && currentEntry.fields) {
        currentEntry.raw += '\n' + line
        entries.push(currentEntry as BibTeXEntry)
      }
      inEntry = false
      currentEntry = null
      continue
    }

    if (!inEntry || !currentEntry) continue

    // 字段赋值
    const fieldMatch = line.match(/^\s*(\w+)\s*=\s*(.+?)(,)?\s*$/)
    if (fieldMatch) {
      // 保存前一个字段
      if (currentField && currentEntry.fields) {
        currentEntry.fields.set(currentField.name, currentField.value)
      }

      // 开始新字段
      const [, name, value, hasComma] = fieldMatch
      const valueTrimmed = value.trim()

      currentEntry.raw += '\n' + line

      // 检查值是否在同一行完成
      if (valueTrimmed.startsWith('{') && valueTrimmed.endsWith('}')) {
        // 单行值
        currentEntry.fields.set(name, valueTrimmed.slice(1, -1))
        currentField = null
        inField = false
      } else if (valueTrimmed.startsWith('"') && valueTrimmed.endsWith('"')) {
        // 单行字符串
        currentEntry.fields.set(name, valueTrimmed.slice(1, -1))
        currentField = null
        inField = false
      } else {
        // 多行值开始
        currentField = {
          name,
          value: valueTrimmed.startsWith('{') || valueTrimmed.startsWith('"')
            ? valueTrimmed.slice(1)
            : valueTrimmed,
          braceDepth: (valueTrimmed.match(/\{/g) || []).length -
                     (valueTrimmed.match(/\}/g) || []).length
        }
        inField = true
      }
      continue
    }

    // 多行值的后续行
    if (inField && currentField) {
      currentEntry.raw += '\n' + line

      // 检查是否结束
      braceDepth += (line.match(/\{/g) || []).length
      braceDepth -= (line.match(/\}/g) || []).length

      currentField.value += '\n' + line

      // 移除末尾的逗号和右括号
      if (line.endsWith('},') || line.endsWith('}')) {
        let finalValue = currentField.value.trim()
        if (finalValue.endsWith(',')) {
          finalValue = finalValue.slice(0, -1)
        }
        if (finalValue.endsWith('}')) {
          finalValue = finalValue.slice(0, -1)
        }

        currentEntry.fields.set(currentField.name, finalValue.trim())
        currentField = null
        inField = false
        braceDepth = 0
      }
    }
  }

  return entries
}

/**
 * 生成BibTeX条目字符串
 */
export function generateBibTeXEntry(entry: BibTeXEntry): string {
  const fields = Array.from(entry.fields.entries())
    .map(([name, value]) => `  ${name} = {${value}}`)
    .join(',\n')

  return `@${entry.type}{${entry.id},
${fields}
}`
}

/**
 * 验证BibTeX条目
 */
export function validateBibTeXEntry(entry: BibTeXEntry): {
  valid: boolean
  errors: string[]
  warnings: string[]
} {
  const errors: string[] = []
  const warnings: string[] = []

  // 检查必填字段
  const requiredFields = ENTRY_TYPE_FIELDS[entry.type] || []
  for (const field of requiredFields) {
    if (!entry.fields.has(field)) {
      errors.push(`缺少必填字段: ${field}`)
    }
  }

  // 检查作者格式
  const author = entry.fields.get('author')
  if (author) {
    if (!author.includes('and')) {
      warnings.push('作者字段可能格式不正确（应使用"and"分隔）')
    }
  }

  // 检查年份格式
  const year = entry.fields.get('year')
  if (year) {
    if (!/^\d{4}$/.test(year)) {
      warnings.push('年份格式可能不正确（应为4位数字）')
    }
  }

  // 检查DOI格式
  const doi = entry.fields.get('doi')
  if (doi) {
    if (!/^10\.\d{4,}/.test(doi)) {
      warnings.push('DOI格式可能不正确（应以10.开头）')
    }
  }

  return {
    valid: errors.length === 0,
    errors,
    warnings
  }
}

/**
 * 格式化作者姓名
 */
export function formatAuthor(authorStr: string): string {
  return authorStr
    .split('and')
    .map(author => {
      author = author.trim()
      // 姓,名 格式
      if (author.includes(',')) {
        const [last, first] = author.split(',').map(s => s.trim())
        return `${first} ${last}`
      }
      return author
    })
    .join(', ')
}

/**
 * 生成引用文本（各种格式）
 */
export function formatCitation(entry: BibTeXEntry, style: 'apa' | 'mla' | 'chicago' | 'ieee' = 'apa'): string {
  const author = entry.fields.get('author') || ''
  const year = entry.fields.get('year') || ''
  const title = entry.fields.get('title') || ''
  const journal = entry.fields.get('journal') || ''
  const volume = entry.fields.get('volume') || ''
  const number = entry.fields.get('number') || ''
  const pages = entry.fields.get('pages') || ''
  const publisher = entry.fields.get('publisher') || ''
  const booktitle = entry.fields.get('booktitle') || ''

  const formattedAuthor = formatAuthor(author)

  switch (style) {
    case 'apa':
      if (entry.type === 'article') {
        return `${formattedAuthor} (${year}). ${title}. ${journal}, ${volume}${number ? `(${number})` : ''}, ${pages}.`
      }
      if (entry.type === 'book') {
        return `${formattedAuthor} (${year}). ${title}. ${publisher}.`
      }
      if (entry.type === 'inproceedings') {
        return `${formattedAuthor} (${year}). ${title}. In ${booktitle}.`
      }
      return `${formattedAuthor} (${year}). ${title}.`

    case 'mla':
      if (entry.type === 'article') {
        return `${formattedAuthor}. "${title}." ${journal}, vol. ${volume}, no. ${number}, ${year}, pp. ${pages}.`
      }
      return `${formattedAuthor}. "${title}." ${year}.`

    case 'chicago':
      if (entry.type === 'article') {
        return `${formattedAuthor}. "${title}." ${journal} ${volume}, no. ${number} (${year}): ${pages}.`
      }
      return `${formattedAuthor}. ${title}. ${year}.`

    case 'ieee':
      if (entry.type === 'article') {
        return `${formattedAuthor}, "${title}," ${journal}, vol. ${volume}${number ? `, no. ${number}` : ''}, pp. ${pages}, ${year}.`
      }
      return `${formattedAuthor}, "${title}," ${year}.`

    default:
      return `${formattedAuthor} (${year}) ${title}`
  }
}

/**
 * 搜索文献条目
 */
export function searchEntries(entries: BibTeXEntry[], query: string): BibTeXEntry[] {
  const lowerQuery = query.toLowerCase()

  return entries.filter(entry => {
    // 搜索ID
    if (entry.id.toLowerCase().includes(lowerQuery)) return true

    // 搜索所有字段
    for (const [name, value] of entry.fields.entries()) {
      if (value.toLowerCase().includes(lowerQuery)) return true
    }

    return false
  })
}

/**
 * 去重文献条目
 */
export function deduplicateEntries(entries: BibTeXEntry[]): {
  unique: BibTeXEntry[]
  duplicates: Array<{ entry: BibTeXEntry; duplicateOf: string }>
} {
  const unique: BibTeXEntry[] = []
  const duplicates: Array<{ entry: BibTeXEntry; duplicateOf: string }> = []
  const seen = new Map<string, string>()

  for (const entry of entries) {
    const title = entry.fields.get('title')?.toLowerCase().trim()
    const doi = entry.fields.get('doi')?.toLowerCase().trim()
    const author = entry.fields.get('author')?.toLowerCase().trim()
    const year = entry.fields.get('year')

    // 生成唯一标识
    const key = doi || (title && `${author}-${year}-${title}`)

    if (key) {
      if (seen.has(key)) {
        duplicates.push({ entry, duplicateOf: seen.get(key)! })
      } else {
        seen.set(key, entry.id)
        unique.push(entry)
      }
    } else {
      unique.push(entry)
    }
  }

  return { unique, duplicates }
}

/**
 * 从条目创建引用命令
 */
export function createCitationCommand(entryId: string, options: {
  bracket?: 'round' | 'square' | 'curly'
  prefix?: string
  suffix?: string
  authorOnly?: boolean
  yearOnly?: boolean
} = {}): string {
  const { bracket = 'round', prefix, suffix, authorOnly, yearOnly } = options

  const bracketMap: Record<string, [string, string]> = {
    round: ['', ''],
    square: ['[', ']'],
    curly: ['{', '}']
  }

  const [open, close] = bracketMap[bracket] || ['', '']

  let command = `\\cite${open}${entryId}${close}`

  if (prefix) command = `\\cite[${prefix}]{${entryId}}`
  if (suffix) command = `\\cite[${suffix}]{${entryId}}`
  if (prefix && suffix) command = `\\cite[${prefix}][${suffix}]{${entryId}}`
  if (authorOnly) command = `\\citeauthor${open}${entryId}${close}`
  if (yearOnly) command = `\\citeyear${open}${entryId}${close}`

  return command
}
