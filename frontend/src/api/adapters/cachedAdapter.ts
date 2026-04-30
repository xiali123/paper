/**
 * 带缓存的API适配器
 * 包装现有API，自动处理缓存
 */

import { withCache, latexCache, paperCache, userCache, cacheControl } from '@/utils/apiCache'
import * as latexAdapter from './latexAdapter'
import * as paperAdapter from './paperAdapter'

// LaTeX API - 带缓存
export const cachedLatexAdapter = {
  // 列表文档 - 使用缓存
  listLatexDocuments: withCache(latexAdapter.listLatexDocuments, latexCache, 'GET'),

  // 获取文档 - 使用缓存
  getLatexDocument: withCache(latexAdapter.getLatexDocument, latexCache, 'GET'),

  // 创建文档 - 不缓存，会清除LaTeX缓存
  createLatexDocument: async (...args: any[]) => {
    const result = await latexAdapter.createLatexDocument(...args)
    cacheControl.clearLatex() // 清除缓存
    return result
  },

  // 更新文档 - 不缓存，会清除LaTeX缓存
  updateLatexDocument: async (...args: any[]) => {
    const result = await latexAdapter.updateLatexDocument(...args)
    cacheControl.clearLatex() // 清除缓存
    return result
  },

  // 删除文档 - 不缓存，会清除LaTeX缓存
  deleteLatexDocument: async (...args: any[]) => {
    const result = await latexAdapter.deleteLatexDocument(...args)
    cacheControl.clearLatex() // 清除缓存
    return result
  },

  // 编译文档 - 不缓存
  compileLatexDocument: latexAdapter.compileLatexDocument,

  // 自动保存 - 不缓存
  autoSaveLatexDocument: latexAdapter.autoSaveLatexDocument,

  // 列表模板 - 使用缓存
  listLatexTemplates: withCache(latexAdapter.listLatexTemplates, latexCache, 'GET'),

  // 从模板创建 - 不缓存，会清除LaTeX缓存
  createFromTemplate: async (...args: any[]) => {
    const result = await latexAdapter.createFromTemplate(...args)
    cacheControl.clearLatex() // 清除缓存
    return result
  },

  // 下载PDF - 不缓存
  downloadLatexPDF: latexAdapter.downloadLatexPDF
}

// 论文API - 带缓存
export const cachedPaperAdapter = {
  // 列表论文 - 使用缓存
  listPapers: withCache(paperAdapter.listPapers, paperCache, 'GET'),

  // 获取论文 - 使用缓存
  getPaper: withCache(paperAdapter.getPaper, paperCache, 'GET'),

  // 创建论文 - 不缓存，会清除论文缓存
  createPaper: async (...args: any[]) => {
    const result = await paperAdapter.createPaper(...args)
    cacheControl.clearPaper() // 清除缓存
    return result
  },

  // 更新论文 - 不缓存，会清除论文缓存
  updatePaper: async (...args: any[]) => {
    const result = await paperAdapter.updatePaper(...args)
    cacheControl.clearPaper() // 清除缓存
    return result
  },

  // 删除论文 - 不缓存，会清除论文缓存
  deletePaper: async (...args: any[]) => {
    const result = await paperAdapter.deletePaper(...args)
    cacheControl.clearPaper() // 清除缓存
    return result
  }
}

export default {
  latex: cachedLatexAdapter,
  paper: cachedPaperAdapter,
  cacheControl
}
