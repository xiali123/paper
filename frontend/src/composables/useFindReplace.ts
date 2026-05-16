/**
 * useFindReplace - Composable for Find & Replace functionality
 *
 * Extracted from LatexEditorView.vue to reduce its size.
 * Provides find, replace, match navigation, and highlight functionality.
 */
import { ref, type Ref } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'

interface FindOptions {
  caseSensitive: boolean
  wholeWord: boolean
  useRegex: boolean
}

interface Match {
  start: number
  end: number
  text: string
}

export function useFindReplace(
  editorContent: Ref<string>,
  editorRef: Ref<any>,
  isModified: Ref<boolean>
) {
  const showFindReplace = ref(false)
  const showReplace = ref(false)
  const findQuery = ref('')
  const replaceQuery = ref('')
  const currentMatchIndex = ref(0)
  const totalMatches = ref(0)
  const matches = ref<Match[]>([])

  const findOptions = ref<FindOptions>({
    caseSensitive: false,
    wholeWord: false,
    useRegex: false
  })

  function performFind() {
    if (!findQuery.value) {
      matches.value = []
      totalMatches.value = 0
      currentMatchIndex.value = 0
      return
    }

    const content = editorContent.value
    const foundMatches: Match[] = []

    let searchPattern: string | RegExp
    try {
      if (findOptions.value.useRegex) {
        const flags = findOptions.value.caseSensitive ? 'g' : 'gi'
        searchPattern = new RegExp(findQuery.value, flags)
      } else {
        searchPattern = findQuery.value
      }

      let match: RegExpExecArray | null | string
      if (searchPattern instanceof RegExp) {
        while ((match = searchPattern.exec(content)) !== null) {
          if (match.index !== undefined && match[0]) {
            foundMatches.push({
              start: match.index,
              end: match.index + match[0].length,
              text: match[0]
            })
          }
        }
      } else {
        // Simple string search
        let searchIndex = 0
        const searchContent = findOptions.value.caseSensitive ? content : content.toLowerCase()
        const searchQuery = findOptions.value.caseSensitive ? searchPattern : (searchPattern as string).toLowerCase()

        while (true) {
          const index = searchContent.indexOf(searchQuery, searchIndex)
          if (index === -1) break

          const actualText = content.substring(index, index + (searchPattern as string).length)

          // Check for whole word match
          if (findOptions.value.wholeWord) {
            const before = index > 0 ? content[index - 1] : ' '
            const after = index + (searchPattern as string).length < content.length
              ? content[index + (searchPattern as string).length]
              : ' '
            if (!/\W/.test(before) || !/\W/.test(after)) {
              searchIndex = index + 1
              continue
            }
          }

          foundMatches.push({
            start: index,
            end: index + (searchPattern as string).length,
            text: actualText
          })
          searchIndex = index + (searchPattern as string).length
        }
      }
    } catch (e) {
      // Invalid regex pattern
      console.error('Find error:', e)
    }

    matches.value = foundMatches
    totalMatches.value = foundMatches.length
    currentMatchIndex.value = foundMatches.length > 0 ? 1 : 0
  }

  function onFindInput() {
    performFind()
  }

  function findNext() {
    if (matches.value.length === 0) {
      performFind()
      return
    }

    if (currentMatchIndex.value < matches.value.length) {
      highlightMatch(currentMatchIndex.value)
      currentMatchIndex.value++
    } else {
      // Wrap around to first match
      currentMatchIndex.value = 1
      highlightMatch(0)
    }
  }

  function findPrevious() {
    if (matches.value.length === 0) {
      performFind()
      return
    }

    if (currentMatchIndex.value > 1) {
      currentMatchIndex.value--
      highlightMatch(currentMatchIndex.value - 1)
    } else {
      // Wrap around to last match
      currentMatchIndex.value = matches.value.length
      highlightMatch(matches.value.length - 1)
    }
  }

  function highlightMatch(matchIndex: number) {
    const match = matches.value[matchIndex]
    if (!match || !editorRef.value) return

    const textarea = editorRef.value.$el?.querySelector('textarea')
    if (!textarea) return

    textarea.focus()
    textarea.setSelectionRange(match.start, match.end)

    // Scroll to the match position
    const textBefore = textarea.value.substring(0, match.start)
    const linesBefore = textBefore.split('\n').length
    const lineHeight = 24 // Approximate line height
    textarea.scrollTop = (linesBefore - 10) * lineHeight
  }

  function replaceCurrent() {
    if (matches.value.length === 0 || currentMatchIndex.value === 0) return

    const match = matches.value[currentMatchIndex.value - 1]
    if (!match) return

    const content = editorContent.value
    const newContent = content.substring(0, match.start) + replaceQuery.value + content.substring(match.end)
    editorContent.value = newContent
    isModified.value = true

    // Re-find after replace
    performFind()

    // Move to next match
    if (matches.value.length > 0) {
      findNext()
    } else {
      ElMessage.success('已完成替换')
    }
  }

  function replaceAll() {
    if (matches.value.length === 0) return

    ElMessageBox.confirm(
      `确定要替换所有 ${totalMatches.value} 个匹配项吗？`,
      '全部替换',
      {
        confirmButtonText: '替换',
        cancelButtonText: '取消',
        type: 'warning'
      }
    ).then(() => {
      let content = editorContent.value
      let replaceCount = 0

      // Process matches in reverse order to maintain indices
      for (let i = matches.value.length - 1; i >= 0; i--) {
        const match = matches.value[i]
        content = content.substring(0, match.start) + replaceQuery.value + content.substring(match.end)
        replaceCount++
      }

      editorContent.value = content
      isModified.value = true
      matches.value = []
      totalMatches.value = 0
      currentMatchIndex.value = 0

      ElMessage.success(`已替换 ${replaceCount} 处`)
    }).catch(() => {
      // User cancelled
    })
  }

  return {
    showFindReplace,
    showReplace,
    findQuery,
    replaceQuery,
    currentMatchIndex,
    totalMatches,
    matches,
    findOptions,
    performFind,
    onFindInput,
    findNext,
    findPrevious,
    replaceCurrent,
    replaceAll
  }
}
