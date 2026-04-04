/**
 * Collaborative Store
 * 协作写作状态管理
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { collaborativeApi } from '@/api/modules/collaborative'
import type { CollaborativeDocument, OTOperation, WritingSuggestion } from '@/types/collaborative'

export const useCollaborativeStore = defineStore('collaborative', () => {
  // State
  const documents = ref<CollaborativeDocument[]>([])
  const currentDocument = ref<CollaborativeDocument | null>(null)
  const activeUsers = ref<Array<{ id: string; name: string; color: string }>>([])
  const operations = ref<OTOperation[]>([])
  const suggestions = ref<WritingSuggestion[]>([])
  const isLoading = ref(false)
  const isConnected = ref(false)

  // Computed
  const hasDocuments = computed(() => documents.value.length > 0)
  const documentCount = computed(() => documents.value.length)

  // Actions
  const loadDocuments = async () => {
    isLoading.value = true
    try {
      // 这里应该有API调用
      // const docs = await collaborativeApi.getDocuments()
      // documents.value = docs
      return documents.value
    } catch (error) {
      console.error('Failed to load documents:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const loadDocument = async (documentId: number) => {
    isLoading.value = true
    try {
      const doc = await collaborativeApi.getDocument(documentId)
      currentDocument.value = doc
      return doc
    } catch (error) {
      console.error('Failed to load document:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const createDocument = async (data: {
    title: string
    documentType: string
    templateId?: number
  }) => {
    isLoading.value = true
    try {
      const doc = await collaborativeApi.createDocument(data)
      documents.value.unshift(doc)
      currentDocument.value = doc
      return doc
    } catch (error) {
      console.error('Failed to create document:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const updateDocument = async (documentId: number, updates: any) => {
    try {
      const doc = await collaborativeApi.updateDocument(documentId, updates)

      // 更新当前文档
      if (currentDocument.value?.id === documentId) {
        currentDocument.value = doc
      }

      // 更新文档列表
      const index = documents.value.findIndex(d => d.id === documentId)
      if (index !== -1) {
        documents.value[index] = doc
      }

      return doc
    } catch (error) {
      console.error('Failed to update document:', error)
      throw error
    }
  }

  const applyOperation = async (documentId: number, operation: OTOperation) => {
    try {
      const result = await collaborativeApi.applyOperation({
        documentId,
        operation
      })

      // 更新文档内容
      if (currentDocument.value?.id === documentId && result.newContent) {
        currentDocument.value.content = result.newContent
      }

      // 记录操作
      operations.value.push(operation)

      return result
    } catch (error) {
      console.error('Failed to apply operation:', error)
      throw error
    }
  }

  const loadSuggestions = async (documentId: number) => {
    try {
      const result = await collaborativeApi.getSuggestions(documentId)
      suggestions.value = result
      return result
    } catch (error) {
      console.error('Failed to load suggestions:', error)
      throw error
    }
  }

  const generateSuggestion = async (data: {
    documentId: number
    suggestionType: string
    positionStart: number
    positionEnd: number
  }) => {
    try {
      const suggestion = await collaborativeApi.generateSuggestion(data)
      suggestions.value.push(suggestion)
      return suggestion
    } catch (error) {
      console.error('Failed to generate suggestion:', error)
      throw error
    }
  }

  const addActiveUser = (user: { id: string; name: string; color: string }) => {
    const exists = activeUsers.value.find(u => u.id === user.id)
    if (!exists) {
      activeUsers.value.push(user)
    }
  }

  const removeActiveUser = (userId: string) => {
    activeUsers.value = activeUsers.value.filter(u => u.id !== userId)
  }

  const setConnectionStatus = (status: boolean) => {
    isConnected.value = status
  }

  const clearCurrentDocument = () => {
    currentDocument.value = null
    operations.value = []
    suggestions.value = []
  }

  return {
    // State
    documents,
    currentDocument,
    activeUsers,
    operations,
    suggestions,
    isLoading,
    isConnected,

    // Computed
    hasDocuments,
    documentCount,

    // Actions
    loadDocuments,
    loadDocument,
    createDocument,
    updateDocument,
    applyOperation,
    loadSuggestions,
    generateSuggestion,
    addActiveUser,
    removeActiveUser,
    setConnectionStatus,
    clearCurrentDocument
  }
})
