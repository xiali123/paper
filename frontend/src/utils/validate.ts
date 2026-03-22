/**
 * 验证是否为有效的搜索关键词
 * @param keyword 关键词
 * @returns 是否有效
 */
export function isValidSearchKeyword(keyword: string): boolean {
  if (!keyword || typeof keyword !== 'string') {
    return false
  }
  const trimmed = keyword.trim()
  return trimmed.length >= 2 && trimmed.length <= 100
}

/**
 * 验证邮箱格式
 * @param email 邮箱地址
 * @returns 是否有效
 */
export function isValidEmail(email: string): boolean {
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/
  return emailRegex.test(email)
}

/**
 * 验证 URL 格式
 * @param url URL 地址
 * @returns 是否有效
 */
export function isValidURL(url: string): boolean {
  try {
    new URL(url)
    return true
  } catch {
    return false
  }
}

/**
 * 验证年份
 * @param year 年份字符串
 * @returns 是否有效
 */
export function isValidYear(year: string): boolean {
  const yearNum = parseInt(year, 10)
  const currentYear = new Date().getFullYear()
  return !isNaN(yearNum) && yearNum >= 1900 && yearNum <= currentYear + 1
}

/**
 * 验证论文等级
 * @param level 等级
 * @returns 是否有效
 */
export function isValidLevel(level: string): boolean {
  return ['A', 'B', 'C'].includes(level.toUpperCase())
}

/**
 * 清理和规范化搜索关键词
 * @param keyword 原始关键词
 * @returns 清理后的关键词
 */
export function sanitizeSearchKeyword(keyword: string): string {
  return keyword
    .trim()
    .replace(/[^\w\s\u4e00-\u9fa5-]/g, ' ') // 保留字母、数字、中文、连字符
    .replace(/\s+/g, ' ') // 合并多个空格
    .trim()
}
