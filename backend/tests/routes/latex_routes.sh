#!/bin/bash
# Route definitions for LatexApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="LatexApi"
ROUTES=(
    # Documents
    "GET|/api/latex/documents||200|List all LaTeX documents"
    "GET|/api/latex/documents/1||200,404|Get LaTeX document by ID"
    "POST|/api/latex/documents|{\"title\":\"Test Doc\",\"content\":\"\\\\documentclass{article}\"}|200,201|Create LaTeX document"
    "PUT|/api/latex/documents/1|{\"title\":\"Updated\"}|200,404|Update LaTeX document"
    "DELETE|/api/latex/documents/1||200,404|Delete LaTeX document"
    "POST|/api/latex/documents/1/compile|{}|200,404|Compile LaTeX document"
    "GET|/api/latex/documents/1/pdf||200,404|Get LaTeX document PDF"

    # Projects
    "GET|/api/latex/projects||200|List all LaTeX projects"
    "GET|/api/latex/projects/1||200,404|Get LaTeX project by ID"
    "POST|/api/latex/projects|{\"name\":\"Test Project\"}|200,201|Create LaTeX project"
    "GET|/api/latex/projects/1/files||200,404|List project files"
    "POST|/api/latex/projects/files|{\"projectId\":1,\"name\":\"main.tex\",\"content\":\"test\"}|200,201|Create project file"
    "GET|/api/latex/projects/files/1||200,404|Get project file by ID"
    "PUT|/api/latex/projects/files/1|{\"content\":\"updated\"}|200,404|Update project file"
    "DELETE|/api/latex/projects/files/1||200,404|Delete project file"
    "POST|/api/latex/projects/1/upload|{}|200,201,400,404|Upload file to project"
    "POST|/api/latex/projects/1/batch-upload|{}|200,201,400,404|Batch upload files to project"
    "POST|/api/latex/projects/1/import|{\"source\":\"test\"}|200,201,404|Import project"
    "POST|/api/latex/projects/1/compile|{}|200,404|Compile LaTeX project"
    "GET|/api/latex/projects/1/pdf||200,404|Get LaTeX project PDF"

    # Templates
    "GET|/api/latex/templates||200|List LaTeX templates"

    # Cache
    "GET|/api/latex/cache/stats||200|Get cache stats"
    "POST|/api/latex/cache/clear|{}|200|Clear cache"

    # Stats
    "GET|/api/latex/stats||200|Get LaTeX stats"

    # Versioning
    "GET|/api/latex/versions/history||200|Get version history"
    "GET|/api/latex/versions/tree||200|Get version tree"
    "GET|/api/latex/versions/compare||200|Compare versions"
    "POST|/api/latex/versions/save|{\"documentId\":1,\"content\":\"test\"}|200,201,404|Save version"
    "POST|/api/latex/versions/restore|{\"versionId\":1}|200,404|Restore version"
    "POST|/api/latex/versions/branch|{\"versionId\":1,\"name\":\"feature\"}|200,201,404|Create version branch"
    "POST|/api/latex/versions/merge|{\"sourceId\":1,\"targetId\":2}|200,404|Merge versions"
    "DELETE|/api/latex/versions/1||200,404|Delete version"

    # Debug
    "GET|/api/latex/debug/pdf/document/1||200,404|Debug get PDF for document"

    # Collaboration
    "POST|/api/latex/collaboration/sessions|{\"documentId\":1,\"userId\":\"user1\",\"userName\":\"Test\"}|200|Join collaboration session"
    "DELETE|/api/latex/collaboration/sessions/1|{\"sessionId\":\"collab_1\",\"userId\":\"user1\"}|200|Leave collaboration session"
    "PUT|/api/latex/collaboration/cursor|{\"sessionId\":\"collab_1\",\"userId\":\"user1\",\"line\":5,\"column\":10}|200|Update cursor position"
    "POST|/api/latex/collaboration/broadcast|{\"sessionId\":\"collab_1\",\"content\":\"update\"}|200|Broadcast document update"
    "GET|/api/latex/collaboration/sessions||200|List collaboration sessions"

    # Validate
    "GET|/api/latex/validate||200|Validate LaTeX syntax"

    # --- Round 22 Additions ---
    "POST|/api/latex/templates|{\"name\":\"Test\",\"content\":\"\\\\documentclass{article}\"}|200|Save LaTeX template"
    "GET|/api/latex/templates||200|List LaTeX templates"
    "POST|/api/latex/validate|{\"content\":\"\\\\documentclass{article}\"}|200|Validate LaTeX syntax"

    # --- Round 28 Additions ---
    "POST|/api/latex/compile/check|{\"content\":\"\\\\documentclass{article}\"}|200|Pre-compile check"
    "GET|/api/latex/snippets||200|Get LaTeX snippets library"

    # --- Round 32 Additions ---
    "POST|/api/latex/bibliography/add|{\"type\":\"article\",\"key\":\"smith2024\",\"fields\":{\"title\":\"Test\"}}|200|Add bibliography entry"
    "GET|/api/latex/bibliography||200|List bibliography entries"

    # --- Round 33 Additions ---
    "DELETE|/api/latex/bibliography/smith2024||200|Delete bibliography entry"
    "GET|/api/latex/bibliography/search?q=test||200|Search bibliography entries"

    # --- Round 34 Additions ---
    "PUT|/api/latex/bibliography/smith2024|{\"fields\":{\"title\":\"Updated\"}}|200|Update bibliography entry"
    "GET|/api/latex/bibliography/export?format=bibtex||200|Export bibliography"

    # --- Round 35 Additions ---
    "POST|/api/latex/bibliography/import|{\"format\":\"bibtex\",\"content\":\"@article{test,...}\"}|200|Import bibliography"
    "GET|/api/latex/bibliography/smith2024||200|Get bibliography entry"

    # --- Round 36 Additions ---
    "POST|/api/latex/bibliography/merge|{\"primary\":\"smith2024\",\"duplicates\":[\"smith2024b\"]}|200|Merge bibliography entries"
    "GET|/api/latex/bibliography/stats||200|Get bibliography statistics"

    # --- Round 37 Additions ---
    "POST|/api/latex/snippets|{\"name\":\"Eq Template\",\"content\":\"\\\\begin{equation}\",\"category\":\"math\"}|200|Create snippet"
    "GET|/api/latex/snippets/search?q=equation||200|Search snippets"

    # --- Round 38 Additions ---
    "PUT|/api/latex/snippets/1|{\"name\":\"Updated\",\"content\":\"...\"}|200|Update snippet"
    "DELETE|/api/latex/snippets/1||200|Delete snippet"

    # --- Round 39 Additions ---
    "POST|/api/latex/cross-reference/check|{\"projectId\":1}|200|Check cross-references"
    "GET|/api/latex/projects/1/structure||200|Get project structure"

    # --- Round 40 Additions ---
    "POST|/api/latex/compile/preview|{\"content\":\"\\\\section{Test}\",\"format\":\"html\"}|200|Compile preview"
    "GET|/api/latex/projects/1/dependencies||200|Get dependency graph"

    # --- Round 41 Additions ---
    "POST|/api/latex/packages/install|{\"projectId\":1,\"packages\":[\"amsmath\"]}|200|Install packages"
    "GET|/api/latex/projects/1/packages||200|List project packages"

    # --- Round 42 Additions ---
    "POST|/api/latex/diff|{\"fileA\":{\"id\":1,\"version\":3},\"fileB\":{\"id\":1,\"version\":5}}|200|Compare versions"
    "GET|/api/latex/templates/1||200|Get template details"

    # --- Round 43 Additions ---
    "POST|/api/latex/magic-comments/add|{\"fileId\":1,\"line\":10,\"comment\":\"TODO: fix\",\"type\":\"todo\"}|200|Add magic comment"
    "GET|/api/latex/files/1/magic-comments||200|Get magic comments"

    # --- Round 44 Additions ---
    "PUT|/api/latex/magic-comments/1|{\"status\":\"resolved\"}|200|Update magic comment"
    "GET|/api/latex/projects/1/labels||200|Get project labels"

    # --- Round 45 Additions ---
    "DELETE|/api/latex/magic-comments/1||200|Delete magic comment"
    "GET|/api/latex/projects/1/compile/history?limit=10||200|Get compile history"

    # --- Round 46 Additions ---
    "POST|/api/latex/projects/1/autosave||200|Trigger autosave"
    "GET|/api/latex/projects/1/stats||200|Get project stats"

    # --- Round 47 Additions ---
    "POST|/api/latex/projects/1/clean|{\"targets\":[\"aux\",\"log\"],\"dryRun\":false}|200|Clean build artifacts"
    "GET|/api/latex/projects/1/symbols||200|Get project symbols"

    # --- Round 48 Additions ---
    "POST|/api/latex/projects/1/backup||200|Create project backup"
    "GET|/api/latex/projects/1/comments||200|Get all comments in project"

    # --- Round 49 Additions ---
    "POST|/api/latex/projects/1/restore|{\"backupId\":\"bck_1_latest\"}|200|Restore project from backup"
    "GET|/api/latex/projects/1/metadata||200|Get project metadata"

    # --- Round 50 Additions ---
    "POST|/api/latex/projects/1/sync|{\"remoteUrl\":\"https://git.example.com/project.git\"}|200|Sync project with remote"
    "GET|/api/latex/projects/1/outline||200|Get document outline/TOC"

    # --- Round 51 Additions ---
    "POST|/api/latex/projects/1/share|{\"targetUserId\":\"user2\",\"permission\":\"edit\"}|200|Share project with another user"
    "GET|/api/latex/projects/1/bibliography||200|Get project bibliography"

    # --- Round 52 Additions ---
    "POST|/api/latex/projects/1/validate|{}|200|Validate LaTeX project for errors"
    "GET|/api/latex/projects/1/glossary||200|Get project glossary terms"

    # --- Round 53 Additions ---
    "POST|/api/latex/projects/1/duplicate|{}|200|Duplicate entire project"
    "GET|/api/latex/projects/1/stats/wordcount||200|Get word count breakdown by section"

    # --- Round 54 Additions ---
    "POST|/api/latex/projects/1/export|{\"format\":\"pdf\"}|200|Export project to specified format"
    "GET|/api/latex/projects/1/references||200|Get project cross-references"

    # --- Round 55 Additions ---
    "POST|/api/latex/projects/1/git/init|{\"branch\":\"main\"}|200|Initialize git repository for project"
    "GET|/api/latex/projects/1/git/status||200|Get git status for project"

    # --- Round 56 Additions ---
    "POST|/api/latex/projects/1/git/commit|{\"message\":\"Fix references\",\"author\":\"user1\"}|200|Commit changes to git"
    "GET|/api/latex/projects/1/git/log||200|Get git commit log"

    # --- Round 57 Additions ---
    "POST|/api/latex/projects/1/git/diff|{}|200|Get git diff for project"
    "GET|/api/latex/projects/1/git/branches||200|List git branches for project"

    # --- Round 58 Additions ---
    "POST|/api/latex/spellcheck|{\"content\":\"\\\\documentclass{article} teh recieve\",\"language\":\"en\"}|200|Spell check LaTeX content"
    "GET|/api/latex/projects/1/bookmarks||200|Get project bookmarks"

    # --- Round 59 Additions ---
    "POST|/api/latex/projects/1/git/checkout|{\"branch\":\"feature/new-section\"}|200|Checkout git branch for project"
    "GET|/api/latex/projects/1/activity||200|Get project activity log"

    # --- Round 60 Additions ---
    "POST|/api/latex/projects/1/git/stash|{\"message\":\"WIP feature\"}|200|Stash git changes for project"
    "GET|/api/latex/projects/1/git/remotes||200|Get git remotes for project"

    # --- Round 61 Additions ---
    "POST|/api/latex/projects/1/git/push|{\"remote\":\"origin\",\"branch\":\"main\"}|200|Push git changes for project"
    "GET|/api/latex/projects/1/git/stashes||200|List git stashes for project"

    # --- Round 62 Additions ---
    "POST|/api/latex/projects/1/git/pull|{\"remote\":\"origin\",\"branch\":\"main\"}|200|Pull git changes for project"
    "GET|/api/latex/projects/1/git/config||200|Get git configuration for project"

    # --- Round 63 Additions ---
    "POST|/api/latex/projects/1/git/tag|{\"tagName\":\"v1.0.0\",\"message\":\"Release 1.0\",\"commitRef\":\"abc123\"}|200|Create git tag for project"
    "GET|/api/latex/projects/1/git/tags||200|List git tags for project"

    # --- Round 64 Additions ---
    "POST|/api/latex/projects/1/git/merge|{\"sourceBranch\":\"feature/section\",\"targetBranch\":\"main\",\"message\":\"Merge feature\",\"author\":\"user1\"}|200|Merge git branches for project"
    "GET|/api/latex/projects/1/git/blame||200|Get git blame for project"

    # --- Round 65 Additions ---
    "POST|/api/latex/projects/1/git/cherry-pick|{\"commitRef\":\"abc123\",\"targetBranch\":\"main\"}|200|Cherry-pick git commit for project"
    "GET|/api/latex/projects/1/git/conflicts||200|Get git merge conflicts for project"

    # --- Round 66 Additions ---
    "POST|/api/latex/projects/1/git/reset|{\"commitRef\":\"abc123\",\"mode\":\"mixed\"}|200|Reset git to specific commit for project"
    "GET|/api/latex/projects/1/git/hooks||200|List git hooks for project"

    # --- Round 67 Additions ---
    "POST|/api/latex/projects/1/git/rebase|{\"sourceBranch\":\"feature/section\",\"targetBranch\":\"main\",\"author\":\"user1\"}|200|Rebase git branch for project"
    "GET|/api/latex/projects/1/git/attributes||200|Get git attributes for project"

    # --- Round 68 Additions ---
    "POST|/api/latex/projects/1/git/submodule|{\"url\":\"https://github.com/example/latex-lib.git\",\"path\":\"vendor/lib\",\"branch\":\"main\"}|200|Add git submodule to project"
    "GET|/api/latex/projects/1/git/submodules||200|List git submodules for project"

    # --- Round 69 Additions ---
    "POST|/api/latex/projects/1/annotations|{\"fileId\":\"file_1\",\"content\":\"Check this equation\",\"type\":\"comment\",\"color\":\"#ff0000\",\"page\":\"3\",\"lineNumber\":42}|200|Create annotation for project"
    "GET|/api/latex/projects/1/annotations||200|List annotations for project"

    # --- Round 70 Additions ---
    "PUT|/api/latex/projects/1/annotations/ann_1|{\"content\":\"Updated note\",\"type\":\"highlight\",\"color\":\"#00ff00\"}|200|Update annotation for project"
    "DELETE|/api/latex/projects/1/annotations/ann_1||200|Delete annotation for project"

    # --- Round 71 Additions ---
    "POST|/api/latex/projects/1/git/archive|{\"format\":\"zip\",\"ref\":\"HEAD\"}|200|Create git archive for project"
    "GET|/api/latex/projects/1/git/ignored||200|Get git ignored files for project"

    # --- Round 72 Additions ---
    "POST|/api/latex/projects/1/git/bisect|{\"bad\":\"abc123\",\"good\":\"def456\",\"strategy\":\"default\"}|200|Start git bisect for project"
    "GET|/api/latex/projects/1/git/worktrees||200|List git worktrees for project"

    # --- Round 73 Additions ---
    "GET|/api/latex/projects/1/git/reflog||200|Get git reflog for project"
    "POST|/api/latex/projects/1/git/commit-amend|{\"message\":\"Fix typo\",\"author\":\"user1\"}|200|Amend last git commit for project"

    # --- Round 74 Additions ---
    "GET|/api/latex/projects/1/git/patch||200|Generate git patch for project"
    "POST|/api/latex/projects/1/git/apply-patch|{\"patchContent\":\"diff --git\",\"dryRun\":false}|200|Apply git patch to project"

    # --- Round 75 Additions ---
    "GET|/api/latex/projects/1/git/patch-series||200|List git patch series for project"
    "POST|/api/latex/projects/1/git/format-patch|{\"fromRef\":\"HEAD~5\",\"toRef\":\"HEAD\",\"sendEmail\":false}|200|Generate formatted patch for project"

    # --- Round 76 Additions ---
    "GET|/api/latex/projects/1/git/lfs/objects||200|List Git LFS objects for project"
    "POST|/api/latex/projects/1/git/lfs/track|{\"pattern\":\"*.pdf\",\"lockType\":\"readonly\"}|200|Track files with Git LFS for project"

    # --- Round 77 Additions ---
    "POST|/api/latex/projects/1/macros|{\"name\":\"\\\\mycommand\",\"definition\":\"\\\\textbf{#1}\",\"type\":\"command\",\"numArgs\":1}|200|Define custom LaTeX macro for project"
    "GET|/api/latex/projects/1/macros||200|List custom macros for project"

    # --- Round 78 Additions ---
    "POST|/api/latex/projects/1/extract/figures|{\"includeCaptions\":true,\"includePaths\":true}|200|Extract all figure environments from project"
    "GET|/api/latex/projects/1/math/symbols||200|Catalog all math environments and symbols in project"

    # --- Round 79 Additions ---
    "POST|/api/latex/projects/1/extract/tables|{\"includeCaptions\":true,\"includeColumnSpec\":true}|200|Extract all table environments from project"
    "GET|/api/latex/projects/1/labels/usage||200|Get label usage report with orphaned and undefined references"

    # --- Round 80 Additions ---
    "POST|/api/latex/projects/1/extract/index|{\"includeSubentries\":true,\"includeCrossRefs\":true}|200|Extract all index entries from project"
    "GET|/api/latex/projects/1/environments/stats||200|Get environment usage statistics for project"

    # --- Round 81 Additions ---
    "POST|/api/latex/projects/1/extract/math|{\"includeInlineMath\":true,\"includeDisplayMath\":true,\"maxComplexity\":50}|200|Extract and catalog all math environments from project"
    "GET|/api/latex/projects/1/font-usage||200|Analyze font-related commands and packages in project"

    # --- Round 82 Additions ---
    "POST|/api/latex/projects/1/lint|{\"checkStyle\":true,\"checkReferences\":true,\"checkDeprecated\":true}|200|Lint LaTeX project for issues"
    "GET|/api/latex/projects/1/wordcloud?maxWords=50&minLength=3||200|Generate word frequency data for word cloud"

    # --- Round 83 Additions ---
    "POST|/api/latex/projects/1/autocomplete|{\"prefix\":\"\\\\text\",\"environment\":\"document\",\"maxSuggestions\":10}|200|Get LaTeX command autocomplete suggestions"
    "GET|/api/latex/projects/1/footnotes||200|Extract and catalog all footnotes in project"

    # --- Round 84 Additions ---
    "POST|/api/latex/projects/1/glossary/validate|{\"checkAcronyms\":true,\"checkDefinitions\":true}|200|Validate glossary terms in project"
    "GET|/api/latex/projects/1/structure/tree?maxDepth=4||200|Get hierarchical document structure tree"

    # --- Round 85 Additions ---
    "POST|/api/latex/projects/1/extract/colors|{\"includeUsage\":true,\"includeXcolor\":true}|200|Extract color definitions and usage from project"
    "GET|/api/latex/projects/1/counter||200|Get LaTeX counter values and history for project"

    # --- Round 86 Additions ---
    "POST|/api/latex/projects/1/extract/todos|{\"includeLineNumbers\":true,\"groupByFile\":false}|200|Extract TODO/FIXME/HACK comments from project"
    "GET|/api/latex/projects/1/indentation||200|Analyze indentation consistency in project"

    # --- Round 87 Additions ---
    "POST|/api/latex/projects/1/hyphenation|{\"language\":\"en\",\"enableAutoHyphen\":true,\"minWordLength\":5}|200|Analyze hyphenation patterns for project"
    "GET|/api/latex/projects/1/nomenclature||200|Get nomenclature/symbol list for project"

    # --- Round 88 Additions ---
    "POST|/api/latex/projects/1/extract/listings|{\"includeLineNumbers\":true,\"includeLanguage\":true,\"includeCaption\":true}|200|Extract code listings from project"
    "GET|/api/latex/projects/1/watermark||200|Get watermark settings for project"

    # --- Round 89 Additions ---
    "POST|/api/latex/projects/1/spellcheck/batch|{\"language\":\"en\",\"includeSuggestions\":true,\"maxErrorsPerFile\":50}|200|Batch spell check across project files"
    "GET|/api/latex/projects/1/headers/outline?maxDepth=4||200|Get structured outline of section headers"

    # --- Round 90 Additions ---
    "POST|/api/latex/projects/1/packages/audit|{\"checkDeprecated\":true,\"checkConflicts\":true,\"suggestAlternatives\":true}|200|Audit LaTeX packages for compatibility and deprecation"
    "GET|/api/latex/projects/1/tikz/commands?includePgfplots=true||200|Extract and catalog TikZ/pgfplots commands from project"

    # --- Round 91 Additions ---
    "POST|/api/latex/projects/1/extract/citations|{\"includeContext\":true,\"groupByKey\":true}|200|Extract and catalog all citation commands from project"
    "GET|/api/latex/projects/1/figure-dependencies?includeThumbnails=false||200|Analyze figure dependencies and missing images"

    # --- Round 92 Additions ---
    "POST|/api/latex/projects/1/cross-references|{\"validateLinks\":true,\"reportBroken\":true}|200|Analyze LaTeX cross-references"
    "GET|/api/latex/projects/1/bibliography/stats?sortBy=type||200|Get bibliography statistics"

    # --- Round 93 Additions ---
    "POST|/api/latex/projects/1/math/audit|{\"checkAlignment\":true,\"checkNumbering\":true,\"reportOrphaned\":true}|200|Audit LaTeX math environments"
    "GET|/api/latex/projects/1/acronym/usage?includeUndefined=true||200|Get acronym usage analysis"

    # --- Round 94 Additions ---
    "POST|/api/latex/projects/1/table-of-contents/generate|{\"maxDepth\":3,\"numbering\":true,\"includeAppendices\":true}|200|Generate table of contents"
    "GET|/api/latex/projects/1/font-usage?groupBy=family||200|Analyze font usage"

    # --- Round 95 Additions ---
    "POST|/api/latex/projects/1/float-placement|{\"strictMode\":true,\"checkOverflow\":true,\"suggestFixes\":true}|200|Analyze float placement"
    "GET|/api/latex/projects/1/package-dependencies?depth=1||200|Get package dependency tree"

    # --- Round 96 Additions ---
    "POST|/api/latex/projects/1/caption/validate|{\"checkGrammar\":true,\"checkFormat\":true,\"requireLabels\":true}|200|Validate figure captions"
    "GET|/api/latex/projects/1/line-count?includeComments=true||200|Get project line count"

    # --- Round 97 Additions ---
    "GET|/api/latex/equation/validate?equation=\\\\frac{a}{b}||200|Validate LaTeX equation"
    "POST|/api/latex/template/instantiate|{\"template_id\":\"tpl_1\",\"variables\":{\"title\":\"Test\",\"author\":\"User\"}}|200|Instantiate LaTeX template"

    # --- Round 98 Additions ---
    "GET|/api/latex/references/count?documentId=1||200|Count references in LaTeX document"
    "POST|/api/latex/bibliography/sort|{\"entries\":[{\"key\":\"smith2024\",\"type\":\"article\"},{\"key\":\"doe2023\",\"type\":\"book\"}],\"sortBy\":\"key\"}|200|Sort bibliography entries"

    # --- Round 99 Additions ---
    "GET|/api/latex/document/dependencies?documentId=1||200|Get document dependencies"
    "POST|/api/latex/macro/define|{\"name\":\"\\\\myfunc\",\"definition\":\"\\\\textbf{#1}\",\"arguments\":1}|200|Define custom LaTeX macro"

    # --- Round 100 Additions ---
    "GET|/api/latex/package/search?query=amsmath||200|Search LaTeX packages"
    "POST|/api/latex/document/compile/status|{\"jobId\":\"job_001\"}|200|Check compilation status"

    # --- Round 101 Additions ---
    "GET|/api/latex/table/validate?documentId=1||200|Validate LaTeX table structure"
    "POST|/api/latex/figure/upload|{\"caption\":\"Test Figure\",\"label\":\"fig:test\",\"filePath\":\"/images/test.png\"}|200|Upload figure metadata"

    # --- Round 102 Additions ---
    "GET|/api/latex/structure/analyze?documentId=1||200|Analyze document structure"
    "POST|/api/latex/spellcheck/run|{\"content\":\"\\\\documentclass{article}\",\"language\":\"en\"}|200|Run spellcheck on document content"

    # --- Round 103 Additions ---
    "GET|/api/latex/metadata/extract?documentId=1||200|Extract metadata from LaTeX document"
    "POST|/api/latex/cross-reference/resolve|{\"documentId\":\"1\",\"references\":[\"fig:intro\",\"tab:results\"]}|200|Resolve cross-references in document"

    # --- Round 104 Additions ---
    "GET|/api/latex/document/stats?documentId=1||200|Get document statistics"
    "POST|/api/latex/environment/validate|{\"content\":\"\\\\begin{document}Hello\\\\end{document}\"}|200|Validate LaTeX environments"

    # --- Round 105 Additions ---
    "GET|/api/latex/font/list||200|List available LaTeX fonts"
    "GET|/api/latex/font/list?category=serif||200|List LaTeX fonts filtered by category"
    "POST|/api/latex/snippet/save|{\"name\":\"My Snippet\",\"content\":\"\\\\begin{equation}E=mc^2\\\\end{equation}\",\"category\":\"math\"}|200|Save a LaTeX snippet"

    # --- Round 106 Additions ---
    "GET|/api/latex/version/history?documentId=1||200|Get LaTeX document version history"
    "POST|/api/latex/comment/add|{\"documentId\":\"1\",\"lineNumber\":42,\"text\":\"Check this equation\"}|200|Add comment to LaTeX document"

    # --- Round 107 Additions ---
    "GET|/api/latex/label/list?documentId=1||200|List all labels in a document"
    "POST|/api/latex/import/bibtex|{\"bibtexContent\":\"@article{smith2024, title={Test}}\"}|200|Import BibTeX entries"

    # --- Round 108 Additions ---
    "GET|/api/latex/document/outline?documentId=1||200|Get document outline (section tree)"
    "POST|/api/latex/footnote/add|{\"documentId\":\"1\",\"footnoteText\":\"See reference [1] for details\",\"markText\":\"ref1\"}|200|Add a footnote to document"

    # --- Round 109 Additions ---
    "GET|/api/latex/error/log?documentId=1||200|Get LaTeX compilation error log"
    "POST|/api/latex/export/pdf|{\"documentId\":\"1\",\"options\":{\"outputPath\":\"/output/doc.pdf\",\"includeBibliography\":true,\"includeAppendices\":true,\"paperSize\":\"a4\",\"fontSize\":\"12pt\"}}|200|Export document as PDF"

    # --- Round 110 Additions ---
    "GET|/api/latex/document/diff?fromVersion=1&toVersion=5||200|Get diff between two document versions"
    "POST|/api/latex/watermark/add|{\"documentId\":\"1\",\"text\":\"CONFIDENTIAL\",\"opacity\":0.3}|200|Add watermark to PDF export"
)
