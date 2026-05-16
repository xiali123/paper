#!/bin/bash
# Route definitions for CollaborativeWriting module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="CollaborativeWriting"
ROUTES=(
    "GET|/api/writing/documents||200,401|List all collaborative documents"
    "POST|/api/writing/documents|{\"title\":\"Test Doc\",\"content\":\"Hello\"}|200,201,401|Create collaborative document"
    "GET|/api/writing/documents/1||200,401,404|Get collaborative document by ID"
    "PUT|/api/writing/documents/1|{\"title\":\"Updated\"}|200,401,404|Update collaborative document"
    "DELETE|/api/writing/documents/1||200,401,404|Delete collaborative document"
    "POST|/api/writing/documents/1/operations|{\"type\":\"insert\",\"position\":0,\"text\":\"test\"}|200,401,404|Submit operation to document"
    "GET|/api/writing/documents/1/versions||200,401,404|List document versions"
    "POST|/api/writing/documents/1/versions|{}|200,401,404|Create document version"
    "GET|/api/writing/documents/1/suggestions||200,401,404|List document suggestions"
    "POST|/api/writing/documents/1/suggestions/generate|{}|200,401,404|Generate suggestions for document"
    "PUT|/api/writing/suggestions/1/accept|{}|200,401,404|Accept suggestion"
    "PUT|/api/writing/suggestions/1/reject|{}|200,401,404|Reject suggestion"
    "GET|/api/writing/documents/1/comments||200,401,404|List document comments"
    "POST|/api/writing/documents/1/comments|{\"content\":\"Nice work\"}|200,401,404|Add comment to document"
    "PUT|/api/writing/comments/1/resolve|{}|200,401,404|Resolve comment"
    "POST|/api/writing/documents/1/export|{\"format\":\"markdown\"}|200,404|Export document"
    "PUT|/api/writing/comments/resolve-all|{\"documentId\":\"1\"}|200|Resolve all comments"
    "POST|/api/writing/documents/1/restore|{\"versionId\":\"1\"}|200,404|Restore document version"

    # --- v4 Additions ---
    "GET|/api/writing/documents/1/versions||200,401|Get document version history"
    "POST|/api/writing/comments/1/reply|{\"content\":\"Reply text\",\"userId\":1}|200|Reply to a comment"
    "GET|/api/writing/documents/1/comments||200,401|Get all comments for document"
    "PUT|/api/writing/documents/1/title|{\"title\":\"New Title\"}|200|Update document title"
    "GET|/api/writing/documents/1/word-count||200|Get word count stats"
    "POST|/api/writing/documents/1/duplicate|{\"title\":\"Copy of Doc\"}|200|Duplicate a document"

    # --- v5 Additions ---
    "GET|/api/writing/documents/1/collaborators||200|Get document collaborators"
    "POST|/api/writing/documents/1/share-link|{\"expiresIn\":24}|200|Generate share link for document"
    "GET|/api/writing/documents/recent||200|Get recently edited documents"

    # --- v6 Additions ---
    "PUT|/api/writing/documents/1/permissions|{\"userId\":2,\"role\":\"editor\"}|200|Update document permissions"
    "GET|/api/writing/documents/1/stats||200|Get document editing stats"
    "POST|/api/writing/documents/1/lock|{\"locked\":true,\"userId\":1}|200|Lock/unlock document for editing"

    # --- v7 Additions ---
    "GET|/api/writing/documents/1/diff?versionId1=1&versionId2=2||200|Get diff between two versions"
    "POST|/api/writing/comments/1/react|{\"userId\":1,\"emoji\":\"\xf0\x9f\x91\x8d\"}|200|Add reaction to a comment"
    "GET|/api/writing/documents/search?q=test||200|Search within documents"

    # --- Round 22 Additions ---
    "POST|/api/writing/documents/1/autosave|{\"content\":\"test\",\"userId\":1}|200|Autosave document content"
    "GET|/api/writing/documents/1/activity||200|Get document activity log"
    "POST|/api/writing/documents/1/invite|{\"email\":\"user@test.com\",\"role\":\"editor\"}|200|Invite user to collaborate"

    # --- Round 25 Additions ---
    "POST|/api/writing/documents/1/merge|{\"sourceVersion\":2,\"targetVersion\":3}|200|Merge document changes"
    "GET|/api/writing/documents/1/export/pdf||200|Export document as PDF"
    "POST|/api/writing/documents/1/tag|{\"tag\":\"important\"}|200|Add tag to document"

    # --- Round 30 Additions ---
    "POST|/api/writing/documents/1/transform|{\"fromFormat\":\"markdown\",\"toFormat\":\"html\"}|200|Transform document format"
    "GET|/api/writing/templates||200|Get writing templates"

    # --- Round 32 Additions ---
    "POST|/api/writing/documents/1/clone|{\"title\":\"Copy\",\"includeComments\":true}|200|Clone document"
    "GET|/api/writing/documents/1/stats/detailed||200|Get detailed document stats"

    # --- Round 33 Additions ---
    "POST|/api/writing/documents/1/archive|{\"archived\":true}|200|Archive document"
    "GET|/api/writing/documents/1/revisions?limit=10||200|Get document revisions"

    # --- Round 34 Additions ---
    "PUT|/api/writing/documents/1/publish|{\"version\":\"1.0\",\"changelog\":\"Initial\"}|200|Publish document"
    "GET|/api/writing/documents/1/changelog?limit=10||200|Get document changelog"

    # --- Round 35 Additions ---
    "POST|/api/writing/templates/1/instantiate|{\"title\":\"My Doc\",\"variables\":{\"author\":\"John\"}}|200|Instantiate template"
    "GET|/api/writing/documents/1/contributors||200|Get document contributors"

    # --- Round 36 Additions ---
    "PUT|/api/writing/documents/1/metadata|{\"tags\":[\"draft\"],\"category\":\"research\"}|200|Update document metadata"
    "GET|/api/writing/documents/1/related||200|Find related documents"

    # --- Round 37 Additions ---
    "POST|/api/writing/comments/1/pin|{\"pinned\":true}|200|Pin comment"
    "GET|/api/writing/documents/1/comments/threaded||200|Get threaded comments"

    # --- Round 38 Additions ---
    "POST|/api/writing/documents/1/table-of-contents|{\"maxDepth\":3,\"style\":\"numbered\"}|200|Generate TOC"
    "GET|/api/writing/documents/1/export/markdown||200|Export as Markdown"

    # --- Round 39 Additions ---
    "POST|/api/writing/documents/1/review/request|{\"reviewers\":[2,3],\"deadline\":\"2024-12-31\"}|200|Request peer review"
    "GET|/api/writing/reviews/pending?status=pending||200|Get pending reviews"

    # --- Round 40 Additions ---
    "POST|/api/writing/reviews/1/submit|{\"rating\":4,\"decision\":\"approve\",\"comments\":\"Good\"}|200|Submit review"
    "GET|/api/writing/documents/1/reviews||200|Get document reviews"

    # --- Round 41 Additions ---
    "POST|/api/writing/documents/1/export/docx|{\"template\":\"default\",\"includeComments\":true}|200|Export as DOCX"
    "GET|/api/writing/documents/1/access-log?limit=20||200|Get access log"

    # --- Round 42 Additions ---
    "POST|/api/writing/documents/1/sections/reorder|{\"sections\":[{\"id\":\"s1\",\"newPosition\":0}]}|200|Reorder sections"
    "GET|/api/writing/documents/1/sections||200|Get document sections"

    # --- Round 43 Additions ---
    "POST|/api/writing/documents/1/sections/s1/move|{\"afterSection\":\"s2\"}|200|Move section"
    "GET|/api/writing/documents/1/word-count/history?days=30||200|Get word count history"

    # --- Round 44 Additions ---
    "PUT|/api/writing/documents/1/sections/s1|{\"title\":\"Updated\",\"content\":\"New content\"}|200|Update section"
    "DELETE|/api/writing/documents/1/sections/s1||200|Delete section"

    # --- Round 45 Additions ---
    "POST|/api/writing/documents/1/sections/s1/clone|{\"insertAfter\":\"s3\"}|200|Clone section"
    "GET|/api/writing/documents/1/export/html?includeStyles=true||200|Export as HTML"

    # --- Round 46 Additions ---
    "POST|/api/writing/documents/1/comments/1/resolve|{\"resolution\":\"Fixed\"}|200|Resolve comment"
    "GET|/api/writing/templates/1/usage||200|Get template usage"

    # --- Round 47 Additions ---
    "POST|/api/writing/documents/1/collaborators/invite|{\"email\":\"user@example.com\",\"role\":\"editor\"}|200|Invite collaborator"
    "GET|/api/writing/documents/1/permissions/matrix||200|Get permissions matrix"

    # --- Round 48 Additions ---
    "POST|/api/writing/documents/1/export/pdf|{\"format\":\"pdf\",\"includeComments\":true}|200|Export document as PDF"
    "GET|/api/writing/documents/1/versions/diff?from=1&to=2||200|Get diff between two document versions"

    # --- Round 49 Additions ---
    "POST|/api/writing/documents/1/lock|{\"userId\":1}|200|Lock document for editing"
    "GET|/api/writing/documents/1/references||200|Get document references/bibliography"

    # --- Round 50 Additions ---
    "POST|/api/writing/documents/1/tags/batch|{\"add\":[\"draft\",\"review\"],\"remove\":[\"archived\"]}|200|Batch update document tags"
    "GET|/api/writing/documents/1/changelog?limit=10||200|Get document changelog/history"

    # --- Round 51 Additions ---
    "POST|/api/writing/templates/1/duplicate|{\"title\":\"Copy of Template\"}|200|Duplicate a writing template"
    "GET|/api/writing/documents/1/export/html?includeStyles=true||200|Export document as HTML"

    # --- Round 52 Additions ---
    "POST|/api/writing/documents/1/ai-assist|{\"context\":\"introduction\",\"cursorPosition\":42}|200|AI-assisted writing suggestion"
    "GET|/api/writing/documents/1/collaborators/active||200|Get currently active collaborators"

    # --- Round 53 Additions ---
    "POST|/api/writing/documents/1/review/request|{\"reviewerId\":\"reviewer_1\",\"focusAreas\":[\"grammar\",\"structure\"]}|200|Request document review"
    "GET|/api/writing/documents/1/review/status||200|Get review status"

    # --- Round 54 Additions ---
    "POST|/api/writing/documents/1/grammar-check|{\"language\":\"en\",\"checkSpelling\":true,\"checkGrammar\":true}|200|Run grammar and spellcheck on document"
    "GET|/api/writing/documents/1/outline||200|Get document structural outline"

    # --- Round 55 Additions ---
    "GET|/api/writing/documents/1/bookmarks||200|Get document bookmarks"
    "POST|/api/writing/documents/1/bookmark|{\"position\":42,\"label\":\"Key Insight\",\"color\":\"#FF5733\"}|200|Add bookmark to document"

    # --- Round 56 Additions ---
    "GET|/api/writing/documents/1/annotations||200|Get document annotations"
    "POST|/api/writing/documents/1/annotate|{\"startOffset\":10,\"endOffset\":50,\"content\":\"Important passage\",\"type\":\"highlight\",\"color\":\"#FFFF00\"}|200|Add annotation to document"

    # --- Round 57 Additions ---
    "GET|/api/writing/documents/1/snapshots||200|Get document snapshots"
    "POST|/api/writing/documents/1/snapshot|{\"label\":\"Pre-edit backup\",\"description\":\"Snapshot before major edits\"}|200|Create document snapshot"

    # --- Round 58 Additions ---
    "GET|/api/writing/documents/1/readability||200|Get document readability metrics"
    "POST|/api/writing/documents/1/citation|{\"sourceId\":\"src_42\",\"citationKey\":\"smith2024\",\"format\":\"APA\"}|200|Add citation to document"

    # --- Round 59 Additions ---
    "GET|/api/writing/documents/1/coauthors||200|Get document co-authors list"
    "POST|/api/writing/documents/1/writing-session|{\"userId\":1,\"sessionType\":\"editing\",\"durationMinutes\":30}|200|Record writing session"

    # --- Round 60 Additions ---
    "GET|/api/writing/documents/1/footnotes||200|Get document footnotes"
    "POST|/api/writing/documents/1/structure|{\"structure\":[{\"id\":\"s1\",\"order\":1},{\"id\":\"s2\",\"order\":2}],\"description\":\"Reordered sections\"}|200|Update document structure"

    # --- Round 61 Additions ---
    "GET|/api/writing/documents/1/endnotes||200|Get document endnotes"
    "POST|/api/writing/documents/1/endnote|{\"content\":\"See appendix A\",\"label\":\"Note 1\",\"position\":1}|200|Add endnote to document"

    # --- Round 62 Additions ---
    "GET|/api/writing/documents/1/marginalia||200|Get document marginalia"
    "POST|/api/writing/documents/1/marginalia|{\"content\":\"Sidebar note\",\"positionX\":100,\"positionY\":200,\"anchorText\":\"key paragraph\",\"authorId\":1}|200|Add marginalia to document"

    # --- Round 63 Additions ---
    "GET|/api/writing/documents/1/cross-references||200|Get document cross-references"
    "POST|/api/writing/documents/1/sticky-note|{\"content\":\"Reminder note\",\"color\":\"#FFEB3B\",\"positionX\":150,\"positionY\":300,\"authorId\":1}|200|Add sticky note to document"

    # --- Round 64 Additions ---
    "GET|/api/writing/documents/1/highlights||200|Get document highlights"
    "POST|/api/writing/documents/1/highlight|{\"startOffset\":10,\"endOffset\":50,\"color\":\"#FFFF00\",\"label\":\"Important\",\"authorId\":1}|200|Add highlight to document"

    # --- Round 65 Additions ---
    "GET|/api/writing/documents/1/formatting||200|Get document formatting/styles"
    "POST|/api/writing/documents/1/formatting|{\"styleType\":\"bold\",\"styleValue\":\"true\",\"startOffset\":10,\"endOffset\":50,\"authorId\":1}|200|Apply formatting to document"

    # --- Round 66 Additions ---
    "GET|/api/writing/documents/1/writing-style||200|Get document writing style analysis"
    "POST|/api/writing/documents/1/typography|{\"fontFamily\":\"serif\",\"fontSize\":12,\"lineHeight\":1.5,\"marginSize\":\"normal\"}|200|Update document typography settings"

    # --- Round 67 Additions ---
    "GET|/api/writing/documents/1/reading-progress||200|Get document reading progress"
    "POST|/api/writing/documents/1/subscribe|{\"notifyType\":\"all\",\"userId\":1}|200|Subscribe to document notifications"

    # --- Round 68 Additions ---
    "GET|/api/writing/documents/1/track-changes||200|Get document track changes history"
    "POST|/api/writing/documents/1/track-changes/accept|{\"changeId\":\"all\",\"action\":\"accept\",\"userId\":1}|200|Accept track changes"

    # --- Round 69 Additions ---
    "GET|/api/writing/documents/1/review-history||200|Get document review history"
    "POST|/api/writing/documents/1/track-changes/reject|{\"changeId\":\"all\",\"userId\":1,\"reason\":\"Not needed\"}|200|Reject track changes"

    # --- Round 70 Additions ---
    "GET|/api/writing/documents/1/track-changes/summary||200|Get track changes summary"
    "POST|/api/writing/documents/1/track-changes/resolve-all|{\"action\":\"accept\",\"userId\":1}|200|Resolve all track changes"

    # --- Round 71 Additions ---
    "GET|/api/writing/documents/1/table-of-contents/refresh||200|Refresh document table of contents"
    "POST|/api/writing/documents/1/track-changes/toggle|{\"enabled\":true,\"userId\":1}|200|Toggle track changes on/off"

    # --- Round 72 Additions ---
    "GET|/api/writing/documents/1/track-changes/count||200|Get track changes count"
    "POST|/api/writing/documents/1/notify|{\"message\":\"Updated\",\"userId\":1,\"notifyType\":\"info\"}|200|Send document notification"

    # --- Round 73 Additions ---
    "GET|/api/writing/documents/1/track-changes/active||200|Get active track changes"
    "POST|/api/writing/documents/1/auto-merge|{\"mergeStrategy\":\"recursive\",\"resolveConflicts\":true}|200|Auto-merge document changes"

    # --- Round 74 Additions ---
    "GET|/api/writing/documents/1/conflicts||200|Get document merge conflicts"
    "POST|/api/writing/documents/1/auto-format|{\"formatStyle\":\"default\",\"fixGrammar\":true}|200|Auto-format document content"

    # --- Round 75 Additions ---
    "GET|/api/writing/documents/1/export-status||200|Get document export status"
    "POST|/api/writing/documents/1/ai-translate|{\"targetLang\":\"en\",\"sourceLang\":\"zh\"}|200|AI translate document"

    # --- Round 76 Additions ---
    "GET|/api/writing/documents/1/auto-save/config||200|Get auto-save configuration"
    "POST|/api/writing/documents/1/auto-save/config|{\"enabled\":true,\"intervalSeconds\":30,\"retentionPolicy\":\"7d\"}|200|Update auto-save configuration"

    # --- Round 77 Additions ---
    "GET|/api/writing/documents/1/reading-time||200|Get estimated reading time for document"
    "POST|/api/writing/documents/1/focus-mode|{\"enabled\":true,\"theme\":\"minimal\",\"hideToolbar\":true,\"typewriterScroll\":false,\"fontSize\":16}|200|Configure focus mode for document"

    # --- Round 78-79 Additions ---
    "GET|/api/writing/documents/1/writing-goals||200|Get writing goals and progress"
    "POST|/api/writing/documents/1/branch|{\"branchName\":\"Alternative Version\",\"description\":\"Exploring new angle\",\"includeComments\":true}|200|Branch/fork document"

    # --- Round 80 Additions ---
    "GET|/api/writing/documents/1/branches||200|List all branches of a document"
    "POST|/api/writing/documents/1/merge-branch|{\"branchId\":\"branch_123\",\"mergeStrategy\":\"replace\",\"createBackup\":true}|200|Merge a branch into document"

    # --- Round 81 Additions ---
    "GET|/api/writing/documents/1/voice-notes||200|Get voice notes for document"
    "POST|/api/writing/documents/1/voice-note|{\"authorId\":1,\"durationSeconds\":30,\"transcript\":\"This paragraph needs revision\",\"audioFormat\":\"webm\"}|200|Add voice note to document"

    # --- Round 82 Additions ---
    "GET|/api/writing/documents/1/revision-timeline||200|Get document revision timeline"
    "POST|/api/writing/documents/1/compare-versions|{\"fromVersion\":1,\"toVersion\":2}|200|Compare two document versions"

    # --- Round 83 Additions ---
    "GET|/api/writing/documents/1/sentiment-analysis||200|Get document sentiment analysis"
    "POST|/api/writing/documents/1/smart-outline|{\"maxDepth\":3,\"style\":\"numbered\",\"includeWordCounts\":true}|200|Generate smart outline from document"

    # --- Round 84 Additions ---
    "GET|/api/writing/documents/1/keyboard-shortcuts||200|Get keyboard shortcuts for document editor"
    "POST|/api/writing/documents/1/compare-side-by-side|{\"leftVersionId\":\"1\",\"rightVersionId\":\"2\",\"includeMetadata\":true,\"highlightDiffs\":true}|200|Side-by-side comparison of two versions"

    # --- Round 85 Additions ---
    "POST|/api/writing/documents/1/ai-summarize|{\"summaryType\":\"abstract\",\"maxSentences\":5,\"includeKeyPhrases\":true,\"targetAudience\":\"general\"}|200|AI-powered document summarization"
    "GET|/api/writing/documents/1/plagiarism-report?sensitivity=standard&sourcesLimit=10||200|Get plagiarism detection report"

    # --- Round 86 Additions ---
    "GET|/api/writing/documents/1/citations?format=all||200|Get all citations in document"
    "POST|/api/writing/documents/1/schedule-publish|{\"publishDate\":\"2026-06-01T10:00:00Z\",\"timezone\":\"UTC\",\"version\":\"1.0\",\"notifyCollaborators\":\"true\"}|200|Schedule document for future publication"

    # --- Round 87 Additions ---
    "GET|/api/writing/documents/1/word-frequency?topN=20&excludeStopWords=true||200|Get word frequency analysis for document"
    "POST|/api/writing/documents/1/co-author|{\"name\":\"Dr. Jane Smith\",\"email\":\"jane@university.edu\",\"contribution\":\"writing\",\"order\":2,\"affiliation\":\"MIT\"}|200|Add co-author to document"

    # --- Round 88 Additions ---
    "GET|/api/writing/documents/1/conflict-resolution||200|Get conflict resolution suggestions for document"
    "POST|/api/writing/documents/1/rename-section|{\"sectionId\":\"s1\",\"newTitle\":\"Introduction\",\"userId\":\"1\"}|200|Rename a section within document"

    # --- Round 89 Additions ---
    "GET|/api/writing/documents/1/writing-sessions?limit=10&groupBy=day||200|Get writing session analytics for document"
    "POST|/api/writing/documents/1/merge-request|{\"sourceBranchId\":\"branch_42\",\"mergeStrategy\":\"replace\",\"description\":\"Integrate revised intro\",\"requesterId\":1,\"createBackup\":true,\"reviewerIds\":[2,3]}|200|Create merge request for document"

    # --- Round 90 Additions ---
    "GET|/api/writing/documents/1/ai-tone-suggest?audience=academic||200|AI tone and style analysis for document"
    "POST|/api/writing/documents/1/auto-rewrite|{\"sectionId\":\"s1\",\"style\":\"clarity\",\"tone\":\"formal\",\"targetWordCount\":200,\"preserveTerminology\":true,\"userId\":1}|200|AI-powered auto-rewrite of document section"

    # --- Round 91 Additions ---
    "GET|/api/writing/documents/1/ai-rewrite-history?limit=10||200|Get AI rewrite history for document"
    "POST|/api/writing/documents/1/apply-rewrite|{\"rewriteId\":\"rw_1_1234\",\"createBackup\":true,\"userId\":1}|200|Apply a previously generated AI rewrite"

    # --- Round 92 Additions ---
    "GET|/api/writing/documents/1/vocabulary?topN=25||200|Get vocabulary analysis for document"
    "POST|/api/writing/documents/1/writing-sprint|{\"userId\":1,\"goalWords\":500,\"durationMinutes\":25,\"sprintType\":\"focused\"}|200|Start a writing sprint session"

    # --- Round 93 Additions ---
    "GET|/api/collaborative-writing/documents/1/style-consistency?sections=intro,method||200|Analyze document style consistency"
    "POST|/api/collaborative-writing/documents/1/section-reorder|{\"userId\":1,\"sectionOrder\":[3,1,4,2],\"reason\":\"logical flow improvement\"}|200|Reorder document sections"
    "POST|/api/collaborative-writing/documents/1/compare-versions|{\"userId\":1,\"versionA\":3,\"versionB\":5,\"diffFormat\":\"unified\"}|200|Compare document versions"
    "GET|/api/collaborative-writing/documents/1/export-metadata?format=all||200|Get document export metadata"

    # --- Round 94 Additions ---
    "POST|/api/collaborative-writing/documents/1/comment/resolve|{\"userId\":1,\"commentId\":\"cmt_123\",\"resolution\":\"fixed\",\"resolutionNote\":\"Updated paragraph\"}|200|Resolve document comment"
    "GET|/api/collaborative-writing/documents/1/comments/statistics?groupBy=author||200|Get comment statistics"

    # --- Round 95 Additions ---
    "POST|/api/collaborative-writing/documents/1/lock-section|{\"userId\":1,\"sectionId\":\"sec_3\",\"lockDuration\":30}|200|Lock document section"
    "GET|/api/collaborative-writing/documents/1/revision-history?limit=20&offset=0||200|Get document revision history"

    # --- Round 96 Additions ---
    "POST|/api/collaborative-writing/documents/1/merge-conflicts/resolve|{\"userId\":1,\"conflictId\":\"conflict_1\",\"resolution\":\"accept_mine\",\"mergedContent\":\"resolved text\"}|200|Resolve merge conflict"
    "GET|/api/collaborative-writing/documents/1/contributions?period=all&sortBy=edits||200|Get document contributions"

    # --- Round 97 Additions ---
    "GET|/api/collaborative-writing/activity-feed?limit=20||200|Get collaborative activity feed"
    "POST|/api/collaborative-writing/document/merge-preview|{\"sourceVersion\":2,\"targetVersion\":3,\"mergeStrategy\":\"auto\"}|200|Preview merge of document changes"

    # --- Round 98 Additions ---
    "GET|/api/collaborative-writing/document/word-count?documentId=1||200|Get word count statistics for document"
    "POST|/api/collaborative-writing/comment/pin|{\"commentId\":\"cmt_1\",\"documentId\":\"1\",\"pinned\":true}|200|Pin a comment in document"

    # --- Round 99 Additions ---
    "GET|/api/collaborative-writing/revision/graph?documentId=1||200|Get revision graph data for document"
    "POST|/api/collaborative-writing/section/move|{\"sectionId\":\"sec_1\",\"fromPosition\":0,\"toPosition\":2}|200|Move a section within document"

    # --- Round 100 Additions ---
    "GET|/api/collaborative-writing/document/search?query=test&documentId=1||200|Search within collaborative documents"
    "POST|/api/collaborative-writing/template/create|{\"name\":\"Research Paper\",\"description\":\"Template for research papers\",\"content\":\"# Title\"}|200|Create a document template"

    # --- Round 101 Additions ---
    "GET|/api/collaborative-writing/collaboration/stats?userId=1||200|Get collaboration statistics"
    "POST|/api/collaborative-writing/review/assign|{\"documentId\":\"1\",\"reviewerId\":\"reviewer_1\",\"deadline\":\"2026-06-30\"}|200|Assign review task"

    # --- Round 102 Additions ---
    "GET|/api/collaborative-writing/version/diff?fromVersion=1&toVersion=2||200|Get diff between document versions"
    "POST|/api/collaborative-writing/permission/update|{\"documentId\":\"1\",\"userId\":\"2\",\"permission\":\"editor\"}|200|Update document permissions"

    # --- Round 103 Additions ---
    "GET|/api/collaborative-writing/document/export?documentId=1&format=markdown||200|Export collaborative document"
    "POST|/api/collaborative-writing/comment/resolve|{\"commentId\":\"cmt_1\",\"resolution\":\"fixed\"}|200|Resolve a comment thread"

    # --- Route 188-189 Additions ---
    "GET|/api/collaborative-writing/editor/presence?documentId=1||200|Get current editor presence for a document"
    "POST|/api/collaborative-writing/document/fork|{\"documentId\":\"1\",\"newTitle\":\"Forked Doc\"}|200|Fork a document to create a new copy"

    # --- Route 190-191 Additions ---
    "GET|/api/collaborative-writing/conflict/list?documentId=1||200|List unresolved conflicts"
    "POST|/api/collaborative-writing/branch/create|{\"documentId\":\"1\",\"branchName\":\"Feature Branch\",\"sourceVersion\":\"3\"}|200|Create a document branch"

    # --- Route 192-193 Additions ---
    "GET|/api/collaborative-writing/merge/status?mergeRequestId=mr_1||200|Get merge request status"
    "POST|/api/collaborative-writing/lock/acquire|{\"documentId\":\"1\",\"userId\":\"1\"}|200|Acquire document lock for editing"

    # --- Route 194-195 Additions ---
    "GET|/api/collaborative-writing/mention/list?userId=1||200|List user mentions in documents"
    "POST|/api/collaborative-writing/autosave/trigger|{\"documentId\":\"1\",\"content\":\"Test content\"}|200|Trigger autosave for document"

    # --- Route 196-197 Additions ---
    "GET|/api/collaborative-writing/document/changelog?documentId=1&limit=10||200|Get document changelog"
    "POST|/api/collaborative-writing/comment/react|{\"commentId\":\"cmt_1\",\"reactionType\":\"like\"}|200|Add reaction to a comment"

    # --- Route 198-199 Additions ---
    "GET|/api/collaborative-writing/document/permissions?documentId=1||200|Get document permissions"
    "POST|/api/collaborative-writing/reaction/add|{\"targetId\":\"doc_1\",\"targetType\":\"document\",\"reaction\":\"👍\"}|200|Add emoji reaction to content"

    # --- Route 200-201 Additions ---
    "GET|/api/collaborative-writing/stats/personal?userId=1||200|Get personal contribution stats"
    "POST|/api/collaborative-writing/notification/subscribe|{\"documentId\":\"1\",\"notificationTypes\":[\"edit\",\"comment\",\"review\"]}|200|Subscribe to document notifications"
)
