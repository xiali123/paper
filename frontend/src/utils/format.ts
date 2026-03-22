/**
 * 格式化数字，添加千位分隔符
 * @param num 数字
 * @returns 格式化后的字符串
 */
export function formatNumber(num: number): string {
  if (num >= 1000000) {
    return (num / 1000000).toFixed(1) + 'M'
  }
  if (num >= 1000) {
    return (num / 1000).toFixed(1) + 'K'
  }
  return num.toString()
}

/**
 * 格式化日期
 * @param date 日期字符串或 Date 对象
 * @param format 格式字符串（默认：'YYYY-MM-DD'）
 * @returns 格式化后的日期字符串
 */
export function formatDate(
  date: string | Date,
  format: string = 'YYYY-MM-DD'
): string {
  const d = typeof date === 'string' ? new Date(date) : date

  const year = d.getFullYear()
  const month = String(d.getMonth() + 1).padStart(2, '0')
  const day = String(d.getDate()).padStart(2, '0')
  const hours = String(d.getHours()).padStart(2, '0')
  const minutes = String(d.getMinutes()).padStart(2, '0')
  const seconds = String(d.getSeconds()).padStart(2, '0')

  return format
    .replace('YYYY', year.toString())
    .replace('MM', month)
    .replace('DD', day)
    .replace('HH', hours)
    .replace('mm', minutes)
    .replace('ss', seconds)
}

/**
 * 格式化时间间隔
 * @param milliseconds 毫秒数
 * @returns 格式化后的时间字符串
 */
export function formatDuration(milliseconds: number): string {
  if (milliseconds < 1000) {
    return `${milliseconds}ms`
  }
  if (milliseconds < 60000) {
    return `${(milliseconds / 1000).toFixed(1)}s`
  }
  const minutes = Math.floor(milliseconds / 60000)
  const seconds = Math.floor((milliseconds % 60000) / 1000)
  return `${minutes}m ${seconds}s`
}

/**
 * 格式化论文等级
 * @param level 等级 (A/B/C)
 * @returns 格式化后的等级字符串
 */
export function formatLevel(level: string): string {
  const levelMap: Record<string, string> = {
    'A': 'CCF-A',
    'B': 'CCF-B',
    'C': 'CCF-C',
    'a': 'CCF-A',
    'b': 'CCF-B',
    'c': 'CCF-C'
  }
  return levelMap[level] || level
}

/**
 * 格式化作者列表
 * @param authors 作者字符串（逗号分隔）或数组
 * @param maxCount 最大显示数量
 * @returns 格式化后的作者字符串
 */
export function formatAuthors(authors: string | string[], maxCount: number = 3): string {
  // 如果是数组，直接使用；如果是字符串，分割成数组
  const authorList = Array.isArray(authors)
    ? authors
    : authors.split(',').map(a => a.trim())

  if (authorList.length === 0) {
    return 'Unknown'
  }

  if (authorList.length <= maxCount) {
    return authorList.join(', ')
  }

  return authorList.slice(0, maxCount).join(', ') + ` et al. (${authorList.length} authors)`
}

/**
 * 截断文本
 * @param text 原文本
 * @param maxLength 最大长度
 * @param suffix 后缀（默认：'...'）
 * @returns 截断后的文本
 */
export function truncateText(text: string, maxLength: number, suffix: string = '...'): string {
  if (text.length <= maxLength) {
    return text
  }
  return text.substring(0, maxLength - suffix.length) + suffix
}

/**
 * 高亮关键词
 * @param text 原文本
 * @param keyword 关键词
 * @param className 高亮样式类名
 * @returns HTML 字符串
 */
export function highlightKeyword(text: string, keyword: string, className: string = 'highlight'): string {
  if (!keyword || !text) {
    return text
  }
  const regex = new RegExp(`(${keyword})`, 'gi')
  return text.replace(regex, `<span class="${className}">$1</span>`)
}

/**
 * 计算百分比
 * @param value 数值
 * @param total 总数
 * @param decimals 小数位数
 * @returns 百分比字符串
 */
export function calculatePercentage(value: number, total: number, decimals: number = 1): string {
  if (total === 0) {
    return '0%'
  }
  return ((value / total) * 100).toFixed(decimals) + '%'
}

/**
 * 下载文件
 * @param url 文件 URL
 * @param filename 文件名
 */
export function downloadFile(url: string, filename?: string): void {
  const link = document.createElement('a')
  link.href = url
  if (filename) {
    link.download = filename
  }
  link.target = '_blank'
  document.body.appendChild(link)
  link.click()
  document.body.removeChild(link)
}

/**
 * 复制文本到剪贴板
 * @param text 要复制的文本
 * @returns Promise<boolean> 是否成功
 */
export async function copyToClipboard(text: string): Promise<boolean> {
  try {
    if (navigator.clipboard && navigator.clipboard.writeText) {
      await navigator.clipboard.writeText(text)
      return true
    } else {
      // 兼容旧浏览器
      const textArea = document.createElement('textarea')
      textArea.value = text
      textArea.style.position = 'fixed'
      textArea.style.left = '-999999px'
      document.body.appendChild(textArea)
      textArea.select()
      const success = document.execCommand('copy')
      document.body.removeChild(textArea)
      return success
    }
  } catch (err) {
    console.error('Failed to copy text:', err)
    return false
  }
}

/**
 * 格式化文件大小
 * @param bytes 字节数
 * @returns 格式化后的文件大小
 */
export function formatFileSize(bytes: number): string {
  if (bytes === 0) return '0 Bytes'
  const k = 1024
  const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i]
}

/**
 * 生成唯一的 ID
 * @returns 唯一 ID 字符串
 */
export function generateId(): string {
  return `${Date.now()}-${Math.random().toString(36).substr(2, 9)}`
}
