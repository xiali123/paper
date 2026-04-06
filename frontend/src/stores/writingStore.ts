/**
 * Writing Store (Pinia)
 * 协作写作模块状态管理
 *
 * @module stores/writing
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { collaborativeApi, type CollaborativeDocument, type WritingSuggestion, type DocumentVersion, type DocumentComment } from '@/api/modules/collaborative'

export const useWritingStore = defineStore(
  'writing',
  () => {
    // ========================================================================
    // State
    // ========================================================================

    /** 文档列表 */
    const documents = ref<CollaborativeDocument[]>([])

    /** 当前文档 */
    const currentDocument = ref<CollaborativeDocument | null>(null)

    /** 加载状态 */
    const loading = ref(false)

    /** 错误信息 */
    const error = ref<string | null>(null)

    /** 保存状态 */
    const saving = ref(false)

    // AI 建议
    const suggestions = ref<WritingSuggestion[]>([])
    const suggestionsLoading = ref(false)

    // 版本历史
    const versions = ref<DocumentVersion[]>([])
    const versionsLoading = ref(false)

    // 评论
    const comments = ref<DocumentComment[]>([])
    const commentsLoading = ref(false)

    // ========================================================================
    // Computed
    // ========================================================================

    /** 待处理的建议 */
    const pendingSuggestions = computed(() =>
      suggestions.value.filter(s => s.status === 'pending')
    )

    /** 当前文档ID */
    const activeDocumentId = computed(() => currentDocument.value?.id ?? null)

    /** 当前文档字数 */
    const documentWordCount = computed(() =>
      currentDocument.value?.word_count ?? 0
    )

    // ========================================================================
    // Actions - 文档管理
    // ========================================================================

    /**
     * 获取文档列表
     */
    async function fetchDocuments(params?: { title?: string; ownerId?: number }) {
      loading.value = true
      error.value = null
      try {
        documents.value = await collaborativeApi.getDocuments(params)
        return documents.value
      } catch (err: any) {
        error.value = err.message || '加载文档列表失败'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * 获取单个文档
     */
    async function fetchDocument(id: number) {
      loading.value = true
      error.value = null
      try {
        const doc = await collaborativeApi.getDocument(id)
        currentDocument.value = doc
        return doc
      } catch (err: any) {
        error.value = err.message || '加载文档失败'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * 创建文档
     */
    async function createDocument(data: { title: string; content?: string; document_type?: string; owner_id?: number }) {
      loading.value = true
      error.value = null
      try {
        const newDoc = await collaborativeApi.createDocument({
          title: data.title,
          content: data.content,
          document_type: data.document_type || 'paper',
          owner_id: data.owner_id
        })
        documents.value.unshift(newDoc)
        return newDoc
      } catch (err: any) {
        error.value = err.message || '创建文档失败'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * 保存文档
     */
    async function saveDocument(id: number, data: { title?: string; content?: string }) {
      saving.value = true
      error.value = null
      try {
        const updated = await collaborativeApi.updateDocument(id, data)
        // 更新列表中的对应项
        const idx = documents.value.findIndex(d => d.id === id)
        if (idx !== -1) documents.value[idx] = updated
        // 更新当前文档
        if (currentDocument.value?.id === id) {
          currentDocument.value = updated
        }
        return updated
      } catch (err: any) {
        error.value = err.message || '保存文档失败'
        throw err
      } finally {
        saving.value = false
      }
    }

    /**
     * 删除文档
     */
    async function deleteDocument(id: number) {
      loading.value = true
      error.value = null
      try {
        await collaborativeApi.deleteDocument(id)
        documents.value = documents.value.filter(d => d.id !== id)
        if (currentDocument.value?.id === id) {
          currentDocument.value = null
        }
      } catch (err: any) {
        error.value = err.message || '删除文档失败'
        throw err
      } finally {
        loading.value = false
      }
    }

    // ========================================================================
    // Actions - AI 建议
    // ========================================================================

    async function fetchSuggestions(documentId: number) {
      suggestionsLoading.value = true
      try {
        suggestions.value = await collaborativeApi.getSuggestions(documentId)
      } catch (err: any) {
        console.error('获取建议失败:', err)
        suggestions.value = []
      } finally {
        suggestionsLoading.value = false
      }
    }

    async function generateSuggestion(documentId: number, type?: string, userId?: number) {
      suggestionsLoading.value = true
      try {
        const suggestion = await collaborativeApi.generateSuggestion(documentId, {
          suggestion_type: (type as any) || 'content',
          user_id: userId
        })
        suggestions.value.push(suggestion)
        return suggestion
      } catch (err: any) {
        console.error('生成建议失败:', err)
        throw err
      } finally {
        suggestionsLoading.value = false
      }
    }

    async function acceptSuggestion(suggestionId: number) {
      try {
        await collaborativeApi.acceptSuggestion(suggestionId)
        const s = suggestions.value.find(s => s.id === suggestionId)
        if (s) s.status = 'accepted'
      } catch (err: any) {
        console.error('接受建议失败:', err)
        throw err
      }
    }

    async function rejectSuggestion(suggestionId: number) {
      try {
        await collaborativeApi.rejectSuggestion(suggestionId)
        const s = suggestions.value.find(s => s.id === suggestionId)
        if (s) s.status = 'rejected'
      } catch (err: any) {
        console.error('拒绝建议失败:', err)
        throw err
      }
    }

    // ========================================================================
    // Actions - 版本历史
    // ========================================================================

    async function fetchVersions(documentId: number) {
      versionsLoading.value = true
      try {
        versions.value = await collaborativeApi.getVersions(documentId)
      } catch (err: any) {
        console.error('获取版本历史失败:', err)
        versions.value = []
      } finally {
        versionsLoading.value = false
      }
    }

    async function createVersion(documentId: number, description?: string) {
      try {
        const version = await collaborativeApi.createVersion(documentId, description)
        versions.value.unshift(version)
        return version
      } catch (err: any) {
        console.error('创建版本失败:', err)
        throw err
      }
    }

    // ========================================================================
    // Actions - 评论
    // ========================================================================

    async function fetchComments(documentId: number) {
      commentsLoading.value = true
      try {
        comments.value = await collaborativeApi.getComments(documentId)
      } catch (err: any) {
        console.error('获取评论失败:', err)
        comments.value = []
      } finally {
        commentsLoading.value = false
      }
    }

    async function addComment(documentId: number, data: { content: string; position_start?: number; position_end?: number; user_id?: number }) {
      try {
        const result = await collaborativeApi.addComment(documentId, data)
        // 重新获取评论列表
        await fetchComments(documentId)
        return result
      } catch (err: any) {
        console.error('添加评论失败:', err)
        throw err
      }
    }

    async function resolveComment(commentId: number) {
      try {
        await collaborativeApi.resolveComment(commentId)
        const c = comments.value.find(c => c.id === commentId)
        if (c) c.is_resolved = true
      } catch (err: any) {
        console.error('解决评论失败:', err)
        throw err
      }
    }

    // ========================================================================
    // Actions - 清理
    // ========================================================================

    function clearCurrentDocument() {
      currentDocument.value = null
      suggestions.value = []
      versions.value = []
      comments.value = []
    }

    function reset() {
      documents.value = []
      currentDocument.value = null
      loading.value = false
      error.value = null
      saving.value = false
      suggestions.value = []
      versions.value = []
      comments.value = []
    }

    // ========================================================================
    // Return
    // ========================================================================

    return {
      // State
      documents,
      currentDocument,
      loading,
      error,
      saving,
      suggestions,
      suggestionsLoading,
      versions,
      versionsLoading,
      comments,
      commentsLoading,
      // Computed
      pendingSuggestions,
      activeDocumentId,
      documentWordCount,
      // Actions
      fetchDocuments,
      fetchDocument,
      createDocument,
      saveDocument,
      deleteDocument,
      fetchSuggestions,
      generateSuggestion,
      acceptSuggestion,
      rejectSuggestion,
      fetchVersions,
      createVersion,
      fetchComments,
      addComment,
      resolveComment,
      clearCurrentDocument,
      reset
    }
  },
  {
    persist: {
      key: 'writing-store',
      storage: localStorage,
      paths: []  // 不持久化，写作数据实时性要求高
    }
  }
)
