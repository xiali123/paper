// 前端数据适配层
// 文件位置: frontend/src/utils/adapter.ts

/**
 * 论文数据适配器
 * 将后端返回的扁平格式转换为前端期望的嵌套格式
 */

interface BackendPaper {
  id: number
  title: string
  journal?: string          // 扁平格式：后端返回
  journal_full?: string     // 备选字段
  journal_short?: string    // 备选字段
  year: string
  level: string
  author?: string           // 旧字段名
  authors?: string          // 新字段名
  doi_url?: string          // 扁平格式
  journal_url?: string      // 扁平格式
}

interface FrontendPaper {
  id: number
  title: string
  journal: {
    full: string
    short: string
  }
  year: string
  level: string
  authors: string
  urls?: {
    doi?: string
    journal?: string
  }
}

/**
 * 转换单个论文数据
 */
export function adaptPaper(raw: BackendPaper): FrontendPaper {
  // 处理 journal 字段
  let journalFull = raw.journal_full || ''
  let journalShort = raw.journal_short || ''

  // 如果 journal 是字符串（扁平格式），提取信息
  if (raw.journal && typeof raw.journal === 'string') {
    // 尝试解析: "CVPR 2024" -> full="CVPR 2024", short="CVPR"
    const match = raw.journal.match(/^([A-Z]+)\s+(\d{4})$/)
    if (match) {
      journalFull = raw.journal
      journalShort = match[1]
    } else {
      journalFull = raw.journal
      journalShort = raw.journal
    }
  }

  // 处理 authors 字段
  const authors = raw.authors || raw.author || ''

  // 处理 urls 字段
  const urls: { doi?: string; journal?: string } = {}
  if (raw.doi_url) urls.doi = raw.doi_url
  if (raw.journal_url) urls.journal = raw.journal_url

  return {
    id: raw.id,
    title: raw.title,
    journal: {
      full: journalFull,
      short: journalShort
    },
    year: raw.year,
    level: raw.level as 'A' | 'B' | 'C',
    authors,
    urls: Object.keys(urls).length > 0 ? urls : undefined
  }
}

/**
 * 转换论文数组
 */
export function adaptPapers(papers: BackendPaper[]): FrontendPaper[] {
  return papers.map(adaptPaper)
}

/**
 * 适配搜索结果
 */
export function adaptSearchResult(raw: any) {
  return {
    papers: adaptPapers(raw.papers || []),
    total: raw.total || 0,
    keyword: raw.keyword || raw.query?.keyword || '',
    duration: raw.duration || raw.duration_ms || 0
  }
}
