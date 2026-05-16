#!/bin/bash
# Route definitions for DashboardApi module
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="DashboardApi"
ROUTES=(
    # Stats
    "GET|/api/dashboard/stats||200|Get dashboard stats"

    # Activities
    "GET|/api/dashboard/activities||200|Get activities"
    "GET|/api/dashboard/activities?limit=5||200|Get activities with limit"

    # Recommendations
    "GET|/api/dashboard/recommendations/papers||200|Get recommended papers"
    "GET|/api/dashboard/recommendations/papers?limit=3||200|Get recommended papers with limit"

    # Trending
    "GET|/api/dashboard/trending/searches||200|Get trending searches"
    "GET|/api/dashboard/trending/searches?limit=5||200|Get trending searches with limit"

    # Todos
    "GET|/api/dashboard/todos||200|Get todos"
    "PUT|/api/dashboard/todos/1/status|{\"status\":\"completed\"}|200|Update todo status"

    # Crawler tasks
    "GET|/api/dashboard/crawler-tasks||200|Get crawler tasks"

    # Growth
    "GET|/api/dashboard/growth||200|Get growth data"
    "GET|/api/dashboard/growth?days=7||200|Get 7-day growth"

    # Distribution
    "GET|/api/dashboard/distribution/journals||200|Get journal distribution"
    "GET|/api/dashboard/distribution/ccf||200|Get CCF distribution"

    # Refresh
    "POST|/api/dashboard/refresh||200|Refresh dashboard"

    # Config
    "GET|/api/dashboard/config||200|Get dashboard config"
    "PUT|/api/dashboard/config|{\"refreshInterval\":600}|200|Update dashboard config"

    # New endpoints
    "POST|/api/dashboard/todos|{\"title\":\"Test\",\"user_id\":1}|200,201|Create todo"
    "DELETE|/api/dashboard/todos/1||200|Delete todo"
    "GET|/api/dashboard/activities||200|Platform activities"
    "GET|/api/dashboard/papers/trending||200|Trending papers"
    "PUT|/api/dashboard/todos/1|{\"title\":\"Updated\",\"priority\":\"high\"}|200|Update todo item"
    "GET|/api/dashboard/notifications||200|Get notifications"
    "GET|/api/dashboard/search-history||200|Recent search history"
    "GET|/api/dashboard/system-health||200|System health check"
    "GET|/api/dashboard/top-papers||200|Top cited papers"

    # New endpoints (batch 3)
    "GET|/api/dashboard/recent-papers||200|Recently added papers"
    "POST|/api/dashboard/widgets/reorder|{\"widgets\":[{\"id\":1,\"position\":0},{\"id\":2,\"position\":1}]}|200|Reorder dashboard widgets"
    "GET|/api/dashboard/reading-stats||200|User reading statistics"

    # Quick stats, pin widget, recent activity
    "GET|/api/dashboard/quick-stats||200|Get lightweight quick stats"
    "POST|/api/dashboard/pin-widget|{\"widgetId\":1,\"pinned\":true}|200|Pin/unpin dashboard widget"
    "GET|/api/dashboard/recent-activity||200|Get compact recent activity feed"

    # Widget management and paper stats
    "POST|/api/dashboard/widgets/add|{\"type\":\"chart\",\"position\":2,\"config\":{\"title\":\"My Chart\"}}|200|Add dashboard widget"
    "DELETE|/api/dashboard/widgets/wgt_123||200|Remove dashboard widget"
    "GET|/api/dashboard/paper-stats||200|Paper statistics for dashboard cards"

    # Layout management and aggregated search history
    "POST|/api/dashboard/layout/save|{\"layout\":[{\"widgetId\":\"stats\",\"x\":0,\"y\":0,\"w\":6,\"h\":4}],\"userId\":\"1\"}|200|Save dashboard layout"
    "GET|/api/dashboard/layout||200|Get saved dashboard layout"
    "GET|/api/dashboard/search-history||200|Aggregated search history for dashboard"

    # --- Round 21 Additions ---
    "GET|/api/dashboard/papers/trending||200|Get trending papers"
    "POST|/api/dashboard/feedback|{\"rating\":5,\"comment\":\"Great\"}|200|Submit dashboard feedback"
    "GET|/api/dashboard/users/active||200|Get active users stats"

    # --- Round 23 Additions ---
    "GET|/api/dashboard/notifications||200|Get dashboard notifications"
    "POST|/api/dashboard/notifications/1/read|{}|200|Mark notification as read"
    "GET|/api/dashboard/papers/recent-views||200|Get recently viewed papers"

    # --- Round 26 Additions ---
    "GET|/api/dashboard/search-history/stats||200|Get search history statistics"
    "POST|/api/dashboard/widgets/reset|{}|200|Reset dashboard widgets"
    "GET|/api/dashboard/system/info||200|Get system info"

    # --- Round 28 Additions ---
    "GET|/api/dashboard/papers/monthly||200|Monthly paper additions"
    "POST|/api/dashboard/quick-note|{\"text\":\"Test note\",\"color\":\"yellow\"}|200|Create quick note"
    "GET|/api/dashboard/export/report||200|Export dashboard report"

    # --- Round 30 Additions ---
    "GET|/api/dashboard/reading/streak||200|Get reading streak"
    "POST|/api/dashboard/preferences|{\"theme\":\"dark\"}|200|Save dashboard preferences"

    # --- Round 32 Additions ---
    "POST|/api/dashboard/notes/1/pin|{\"pinned\":true}|200|Pin dashboard note"
    "GET|/api/dashboard/calendar?month=2024-01||200|Get calendar view"

    # --- Round 33 Additions ---
    "GET|/api/dashboard/papers/comparison?period1=2024-01&period2=2024-02||200|Compare paper stats"
    "POST|/api/dashboard/theme|{\"theme\":\"dark\"}|200|Save dashboard theme"

    # --- Round 34 Additions ---
    "GET|/api/dashboard/activities/export?format=json||200|Export activity log"
    "POST|/api/dashboard/widgets/1/configure|{\"config\":{\"title\":\"Papers\",\"visible\":true}}|200|Configure widget"

    # --- Round 35 Additions ---
    "GET|/api/dashboard/search-history/timeline?days=30&groupBy=day||200|Get search timeline"
    "POST|/api/dashboard/shortcuts|{\"name\":\"My Papers\",\"type\":\"filter\",\"config\":{}}|200|Create shortcut"

    # --- Round 36 Additions ---
    "GET|/api/dashboard/papers/favorites?limit=10||200|Get favorite papers"
    "POST|/api/dashboard/notes/batch|{\"notes\":[{\"title\":\"Note 1\",\"content\":\"Test\"}]}|200|Batch create notes"

    # --- Round 37 Additions ---
    "GET|/api/dashboard/papers/recently-viewed?limit=10||200|Get recently viewed papers"
    "POST|/api/dashboard/pinboard|{\"type\":\"paper\",\"referenceId\":\"123\",\"position\":{\"x\":0,\"y\":0}}|200|Add to pinboard"

    # --- Round 38 Additions ---
    "GET|/api/dashboard/pinboard?type=paper||200|Get pinboard items"
    "DELETE|/api/dashboard/pinboard/pin_123||200|Remove pinboard item"

    # --- Round 39 Additions ---
    "GET|/api/dashboard/notifications/settings||200|Get notification settings"
    "PUT|/api/dashboard/notifications/settings|{\"email\":true,\"push\":false,\"frequency\":\"daily\"}|200|Update notification settings"

    # --- Round 40 Additions ---
    "GET|/api/dashboard/search-history/frequent?limit=10||200|Get frequent searches"
    "POST|/api/dashboard/search-history/clear|{\"olderThan\":\"2024-01-01\"}|200|Clear search history"

    # --- Round 41 Additions ---
    "GET|/api/dashboard/collections||200|Get collections"
    "POST|/api/dashboard/collections|{\"name\":\"ML Papers\",\"description\":\"My list\"}|200|Create collection"

    # --- Round 42 Additions ---
    "PUT|/api/dashboard/collections/1|{\"name\":\"Updated\"}|200|Update collection"
    "DELETE|/api/dashboard/collections/1||200|Delete collection"

    # --- Round 43 Additions ---
    "POST|/api/dashboard/collections/1/papers|{\"paperIds\":[1,2,3]}|200|Add papers to collection"
    "DELETE|/api/dashboard/collections/1/papers/1||200|Remove paper from collection"

    # --- Round 44 Additions ---
    "GET|/api/dashboard/collections/1/papers?sort=date||200|Get collection papers"
    "PUT|/api/dashboard/collections/1/reorder|{\"paperOrder\":[3,1,2]}|200|Reorder collection papers"

    # --- Round 45 Additions ---
    "GET|/api/dashboard/stats/export?format=json&period=month||200|Export dashboard stats"
    "POST|/api/dashboard/shortcuts/sc_1/execute||200|Execute shortcut"

    # --- Round 46 Additions ---
    "DELETE|/api/dashboard/shortcuts/sc_1||200|Delete shortcut"
    "GET|/api/dashboard/shortcuts?type=filter||200|Get shortcuts"

    # --- Round 47 Additions ---
    "PUT|/api/dashboard/shortcuts/sc_1|{\"name\":\"Updated\"}|200|Update shortcut"
    "GET|/api/dashboard/papers/1/related||200|Get related papers"

    # --- Round 48 Additions ---
    "PUT|/api/dashboard/config/layout|{\"layoutMode\":\"flex\",\"refreshInterval\":600,\"widgets\":[{\"id\":\"stats\",\"visible\":true}]}|200|Update dashboard layout config"
    "GET|/api/dashboard/stats/export?format=json||200|Export dashboard statistics as JSON"

    # --- Round 49 Additions ---
    "POST|/api/dashboard/widgets/reorder|{\"widgetOrder\":[{\"id\":1,\"position\":0},{\"id\":2,\"position\":1}]}|200|Reorder dashboard widgets"
    "GET|/api/dashboard/alerts?severity=high&limit=10||200|Get dashboard alerts"

    # --- Round 50 Additions ---
    "POST|/api/dashboard/search/save|{\"query\":\"machine learning\",\"filters\":{\"year\":\"2024\"}}|200|Save search query to dashboard"
    "GET|/api/dashboard/search/saved?limit=10||200|Get saved searches"

    # --- Round 51 Additions ---
    "POST|/api/dashboard/widgets/add|{\"type\":\"chart\",\"position\":2,\"config\":{\"title\":\"My Chart\"}}|200|Add dashboard widget"
    "GET|/api/dashboard/widgets/wgt_123||200|Get widget by ID"

    # --- Round 52 Additions ---
    "POST|/api/dashboard/quick-note|{\"content\":\"Test note\",\"color\":\"yellow\"}|200|Create quick dashboard note"
    "GET|/api/dashboard/notes?limit=10||200|Get all dashboard notes"

    # --- Round 53 Additions ---
    "POST|/api/dashboard/notes/1|{\"content\":\"Updated note\",\"color\":\"blue\"}|200|Update dashboard note"
    "GET|/api/dashboard/papers/recent?limit=5||200|Get recently viewed papers"

    # --- Round 54 Additions ---
    "GET|/api/dashboard/bookmarks?limit=10||200|Get dashboard bookmarks"
    "POST|/api/dashboard/bookmarks|{\"title\":\"ML Paper\",\"url\":\"https://example.com\",\"category\":\"research\"}|200|Create dashboard bookmark"
    "GET|/api/dashboard/bookmarks/1||200|Get bookmark by ID"
    "DELETE|/api/dashboard/bookmarks/1||200|Delete dashboard bookmark"

    # --- Round 55 Additions ---
    "GET|/api/dashboard/tags||200|Get dashboard tags statistics"
    "POST|/api/dashboard/tags/merge|{\"sourceTag\":\"ml\",\"targetTag\":\"machine-learning\"}|200|Merge dashboard tags"

    # --- Round 56 Additions ---
    "GET|/api/dashboard/papers/recommendations?limit=5||200|Get personalized paper recommendations"
    "POST|/api/dashboard/reading/goal|{\"targetCount\":10,\"period\":\"weekly\"}|200|Set reading goal"

    # --- Round 57 Additions ---
    "GET|/api/dashboard/reading/progress?days=30||200|Get reading progress"
    "GET|/api/dashboard/papers/highlights?limit=10||200|Get highlighted papers"

    # --- Round 58 Additions ---
    "GET|/api/dashboard/reading/sessions?limit=10||200|Get reading session history"
    "POST|/api/dashboard/reading/session/start|{\"paperId\":1}|200|Start a new reading session"

    # --- Round 59 Additions ---
    "GET|/api/dashboard/reading/session/active?limit=10||200|Get active reading sessions"
    "POST|/api/dashboard/reading/session/end|{\"sessionId\":\"rs_123_1\",\"durationMinutes\":30,\"notes\":\"Finished\"}|200|End a reading session"

    # --- Round 60 Additions ---
    "GET|/api/dashboard/reading/summary?days=30||200|Get reading summary statistics"
    "POST|/api/dashboard/reading/session/note|{\"sessionId\":\"rs_123_1\",\"note\":\"Important insight\"}|200|Add note to reading session"

    # --- Round 61 Additions ---
    "GET|/api/dashboard/reading/achievements?limit=10||200|Get reading achievements"
    "POST|/api/dashboard/reading/bookmark|{\"sessionId\":\"rs_123_1\",\"label\":\"Important\"}|200|Bookmark a reading session"

    # --- Round 62 Additions ---
    "GET|/api/dashboard/reading/ranking?limit=10||200|Get reading ranking leaderboard"
    "POST|/api/dashboard/annotations|{\"targetType\":\"paper\",\"targetId\":\"123\",\"content\":\"Key insight\"}|200|Create annotation"

    # --- Round 63 Additions ---
    "GET|/api/dashboard/annotations?limit=10||200|List annotations"
    "PUT|/api/dashboard/annotations/ann_1|{\"content\":\"Updated insight\"}|200|Update annotation"

    # --- Round 64 Additions ---
    "DELETE|/api/dashboard/annotations/ann_1||200|Delete annotation"
    "GET|/api/dashboard/reading/stats?days=30||200|Get overall reading statistics"

    # --- Round 65 Additions ---
    "GET|/api/dashboard/reading/weekly-report?weeks=2||200|Get weekly reading report"
    "POST|/api/dashboard/reading/share|{\"type\":\"summary\",\"platform\":\"internal\",\"message\":\"Great week\"}|200|Share reading progress"

    # --- Round 66 Additions ---
    "GET|/api/dashboard/reading/goals?limit=10||200|Get reading goals"
    "POST|/api/dashboard/reading/challenge|{\"name\":\"Spring Reading\",\"targetPapers\":15,\"durationDays\":30}|200|Create reading challenge"

    # --- Round 67 Additions ---
    "GET|/api/dashboard/reading/challenge/progress?limit=10||200|Get reading challenge progress"
    "POST|/api/dashboard/reading/challenge/join|{\"challengeId\":\"ch_123\",\"userId\":1}|200|Join reading challenge"

    # --- Round 68 Additions ---
    "GET|/api/dashboard/reading/challenge/leaderboard?limit=10||200|Get reading challenge leaderboard"
    "POST|/api/dashboard/reading/challenge/leave|{\"challengeId\":\"ch_123\",\"userId\":1}|200|Leave reading challenge"

    # --- Round 69 Additions ---
    "GET|/api/dashboard/reading/digest?days=7||200|Get personalized reading digest"
    "POST|/api/dashboard/reading/insights|{\"userId\":1,\"type\":\"weekly\"}|200|Generate reading insights"

    # --- Round 70 Additions ---
    "GET|/api/dashboard/reading/milestones?limit=10||200|Get reading milestones"
    "POST|/api/dashboard/reading/milestone/claim|{\"milestoneId\":1,\"userId\":1}|200|Claim reading milestone"

    # --- Round 71 Additions ---
    "GET|/api/dashboard/reading/badges?limit=10||200|Get reading badges"
    "POST|/api/dashboard/reading/focus-mode|{\"userId\":1,\"durationMinutes\":25,\"paperId\":\"123\"}|200|Start focus reading session"

    # --- Round 72 Additions ---
    "GET|/api/dashboard/reading/focus-sessions?limit=10||200|Get focus reading sessions"
    "POST|/api/dashboard/reading/focus-session/end|{\"sessionId\":\"focus_123\",\"actualMinutes\":25}|200|End focus reading session"

    # --- Round 73 Additions ---
    "GET|/api/dashboard/reading/time-tracker?days=7||200|Get reading time tracker"
    "POST|/api/dashboard/reading/pause|{\"sessionId\":\"focus_123\",\"reason\":\"break\"}|200|Pause reading session"

    # --- Round 74 Additions ---
    "GET|/api/dashboard/reading/resume?sessionId=focus_123||200|Resume paused reading session"
    "POST|/api/dashboard/reading/session/log|{\"paperId\":\"123\",\"durationMinutes\":15,\"notes\":\"Good paper\"}|200|Log reading session entry"

    # --- Round 75 Additions ---
    "GET|/api/dashboard/reading/session/summary?sessionId=rs_123_1||200|Get reading session summary"
    "POST|/api/dashboard/reading/session/feedback|{\"sessionId\":\"rs_123_1\",\"rating\":5,\"comment\":\"Great session\"}|200|Submit reading session feedback"

    # --- Round 76 Additions ---
    "GET|/api/dashboard/reading/session/analytics?period=7d||200|Get reading session analytics"
    "POST|/api/dashboard/reading/session/export|{\"format\":\"json\",\"startDate\":\"2024-01-01\",\"endDate\":\"2024-12-31\"}|200|Export reading session data"

    # --- Round 77 Additions ---
    "GET|/api/dashboard/reading/session/heatmap?days=90||200|Get reading activity heatmap"
    "POST|/api/dashboard/reading/session/compare|{\"periodAStart\":\"2024-10-01\",\"periodAEnd\":\"2024-10-31\",\"periodBStart\":\"2024-09-01\",\"periodBEnd\":\"2024-09-30\"}|200|Compare reading metrics between periods"

    # --- Round 78 Additions ---
    "GET|/api/dashboard/reading/productivity-score?days=30||200|Get reading productivity score"
    "POST|/api/dashboard/reading/annotations/export|{\"format\":\"json\",\"targetType\":\"paper\"}|200|Export annotations in requested format"

    # --- Round 79 Additions ---
    "GET|/api/dashboard/reading/recommendations-engine?days=30&limit=10||200|Get AI-powered reading recommendations"
    "POST|/api/dashboard/reading/session/tag|{\"sessionId\":\"rs_123_1\",\"tag\":\"important\",\"color\":\"red\"}|200|Tag a reading session"

    # --- Round 80 Additions ---
    "GET|/api/dashboard/reading/vocabulary?days=30&limit=10||200|Get reading vocabulary and keyword exposure"
    "POST|/api/dashboard/reading/session/bookmark|{\"sessionId\":\"rs_123_1\",\"label\":\"Key Formula\",\"page\":12,\"note\":\"Important derivation\"}|200|Bookmark a reading session point"

    # --- Round 81 Additions ---
    "GET|/api/dashboard/reading/citations?limit=10||200|Get citation network summary"
    "POST|/api/dashboard/reading/collaboration|{\"paperId\":\"123\",\"userId\":1,\"message\":\"Let us discuss this\"}|200|Create reading collaboration"

    # --- Round 82 Additions ---
    "GET|/api/dashboard/reading/genre-distribution?limit=10||200|Get reading genre distribution"
    "POST|/api/dashboard/reading/sharing/insight|{\"paperId\":\"123\",\"insight\":\"Key finding on attention mechanism\",\"platform\":\"internal\",\"userId\":1}|200|Share reading insight"

    # --- Round 83 Additions ---
    "GET|/api/dashboard/reading/influence-map?limit=10||200|Get reading influence network map"
    "POST|/api/dashboard/reading/session/highlight|{\"sessionId\":\"rs_123_1\",\"text\":\"Key formula on page 5\",\"color\":\"yellow\",\"page\":5,\"note\":\"Important derivation\"}|200|Create reading session highlight"

    # --- Round 84 Additions ---
    "GET|/api/dashboard/reading/serendipity?limit=10||200|Get cross-disciplinary serendipity discoveries"
    "POST|/api/dashboard/reading/reflection|{\"paperId\":\"123\",\"content\":\"This paper connects transformer attention to cortical columns\",\"mood\":\"inspired\",\"tags\":\"neuroscience,transformers\"}|200|Create reading reflection entry"

    # --- Round 85 Additions ---
    "GET|/api/dashboard/reading/engagement-score?days=30||200|Get reading engagement score"
    "POST|/api/dashboard/reading/insights/generate|{\"period\":\"weekly\",\"topN\":5}|200|Generate reading insights"

    # --- Round 86 Additions ---
    "GET|/api/dashboard/reading/knowledge-graph?limit=15||200|Get research topic knowledge graph"
    "POST|/api/dashboard/reading/study-plan|{\"focusArea\":\"machine learning\",\"targetPapers\":12,\"durationDays\":28,\"difficulty\":\"intermediate\"}|200|Generate personalized study plan"

    # --- Round 87 Additions ---
    "GET|/api/dashboard/reading/abandonment-analysis?days=30&limit=10||200|Get reading abandonment analysis"
    "POST|/api/dashboard/reading/session/rate|{\"sessionId\":\"rs_123_1\",\"clarity\":4,\"relevance\":5,\"difficulty\":3,\"novelty\":4,\"methodology\":5,\"comment\":\"Excellent methodology\"}|200|Rate reading session quality"

    # --- Round 88 Additions ---
    "GET|/api/dashboard/reading/research-radar?days=30&limit=10||200|Get cross-domain research trend radar"
    "POST|/api/dashboard/reading/insights/share|{\"insight\":\"Transformer attention scales quadratically\",\"paperId\":\"123\",\"platform\":\"internal\",\"visibility\":\"public\"}|200|Share reading insight with others"

    # --- Round 89 Additions ---
    "GET|/api/dashboard/reading/diversity-score?days=30&limit=10||200|Get reading diversity score across disciplines"
    "POST|/api/dashboard/reading/zen-mode|{\"paperId\":\"123\",\"durationMinutes\":25,\"ambientMode\":\"focus\",\"theme\":\"dark\",\"disableNotifications\":true}|200|Start zen-mode deep reading session"

    # --- Round 90 Additions ---
    "GET|/api/dashboard/reading/attention-flow?days=30||200|Get reading attention flow analysis"
    "POST|/api/dashboard/reading/revisit|{\"paperId\":\"123\",\"reason\":\"review\",\"notes\":\"Rechecking methodology\"}|200|Record paper revisit"

    # --- Round 91 Additions ---
    "GET|/api/dashboard/reading/memory-map?days=30||200|Get reading memory retention map"
    "POST|/api/dashboard/reading/quizz|{\"scope\":\"recent\",\"questionCount\":5,\"difficulty\":\"mixed\"}|200|Generate comprehension quizz from reading"

    # --- Round 92 Additions ---
    "GET|/api/dashboard/reading/knowledge-retention?days=30&limit=10||200|Get knowledge retention analysis"
    "POST|/api/dashboard/reading/spaced-repetition|{\"paperId\":\"123\",\"userId\":1,\"quality\":4,\"sessionDuration\":15}|200|Schedule spaced repetition review"

    # --- Round 93 Additions ---
    "GET|/api/dashboard/reading/speed-trends?days=30||200|Get reading speed trends"
    "POST|/api/dashboard/reading/goal-set|{\"userId\":1,\"goalType\":\"papers_per_week\",\"targetValue\":5,\"startDate\":\"2024-01-01\",\"endDate\":\"2024-12-31\"}|200|Set reading goal"

    # --- Round 94 Additions ---
    "GET|/api/dashboard/reading/topic-explorer?depth=2&limit=15||200|Explore reading topic tree"
    "POST|/api/dashboard/reading/session/annotate|{\"sessionId\":\"rs_123_1\",\"userId\":1,\"annotations\":[{\"type\":\"highlight\",\"content\":\"Key formula\",\"page\":5}]}|200|Add reading session annotations"

    # --- Round 95 Additions ---
    "GET|/api/dashboard/reading/correlation-matrix?topics=5&days=90||200|Get topic correlation matrix"
    "POST|/api/dashboard/reading/batch-rate|{\"userId\":1,\"ratings\":[{\"paperId\":\"p1\",\"rating\":4,\"tags\":[\"interesting\"]},{\"paperId\":\"p2\",\"rating\":3}]}|200|Batch rate papers"

    # --- Round 96 Additions ---
    "GET|/api/dashboard/reading/consistency-score?days=30||200|Get reading consistency score"
    "POST|/api/dashboard/reading/session/tag-batch|{\"sessionId\":\"rs_123_1\",\"tags\":[{\"label\":\"methodology\",\"color\":\"blue\"},{\"label\":\"results\",\"color\":\"green\"}]}|200|Batch create reading tags"

    # --- Round 97 Additions ---
    "GET|/api/dashboard/data-quality?limit=10||200|Get data quality metrics"
    "POST|/api/dashboard/widget/layout|{\"layout\":[{\"widgetId\":\"stats\",\"x\":0,\"y\":0,\"w\":6,\"h\":4},{\"widgetId\":\"chart\",\"x\":6,\"y\":0,\"w\":6,\"h\":4}]}|200|Save widget layout"

    # --- Round 98 Additions ---
    "GET|/api/dashboard/export/history?limit=10&format=pdf||200|Get export history"
    "POST|/api/dashboard/alert/create|{\"alertName\":\"Paper spike\",\"condition\":\"papers_count >\",\"threshold\":100}|200|Create dashboard alert"

    # --- Round 99 Additions ---
    "GET|/api/dashboard/performance/metrics?limit=10||200|Get system performance metrics"
    "POST|/api/dashboard/schedule/report|{\"reportType\":\"weekly\",\"schedule\":\"0 8 * * 1\",\"format\":\"pdf\"}|200|Schedule dashboard report generation"

    # --- Round 100 Additions ---
    "GET|/api/dashboard/search-analytics?period=7d||200|Get search analytics data"
    "POST|/api/dashboard/bookmark/batch|{\"paperIds\":[\"p1\",\"p2\",\"p3\"],\"action\":\"add\"}|200|Batch bookmark operations"

    # --- Round 101 Additions ---
    "GET|/api/dashboard/collection/stats||200|Get paper collection statistics by category"
    "POST|/api/dashboard/notification/subscribe|{\"notificationType\":\"paper_published\",\"frequency\":\"daily\"}|200|Subscribe to dashboard notifications"

    # --- Route 188-189 Additions ---
    "GET|/api/dashboard/tag/cloud?limit=20||200|Get tag cloud data for papers"
    "POST|/api/dashboard/widget/reorder|{\"widgetOrder\":[{\"id\":1,\"position\":0},{\"id\":2,\"position\":1}]}|200|Reorder dashboard widgets"

    # --- Route 190-191 Additions ---
    "GET|/api/dashboard/reading-progress||200|Get reading progress across collections"
    "GET|/api/dashboard/reading-progress?userId=1||200|Get reading progress filtered by user"
    "POST|/api/dashboard/quick-note/create|{\"content\":\"My quick note\"}|200|Create a quick note"
    "POST|/api/dashboard/quick-note/create|{\"content\":\"Tagged note\",\"tags\":[\"research\",\"ml\"]}|200|Create quick note with tags"

    # --- Route 192-193 Additions ---
    "GET|/api/dashboard/citation/impact||200|Get citation impact metrics"
    "GET|/api/dashboard/citation/impact?year=2024||200|Get citation impact metrics by year"
    "POST|/api/dashboard/theme/apply|{\"themeId\":\"dark-pro\",\"customizations\":{\"accentColor\":\"#6366f1\"}}|200|Apply dashboard theme"

    # --- Route 194-195 Additions ---
    "GET|/api/dashboard/recent/views||200|Get recently viewed papers"
    "GET|/api/dashboard/recent/views?limit=5||200|Get recently viewed papers with limit"
    "POST|/api/dashboard/filter/save|{\"name\":\"My Filter\",\"criteria\":{\"year\":\"2024\"},\"isDefault\":true}|200|Save dashboard filter"

    # --- Route 196-197 Additions ---
    "GET|/api/dashboard/heatmap/data||200|Get activity heatmap data"
    "GET|/api/dashboard/heatmap/data?year=2024||200|Get activity heatmap data by year"
    "POST|/api/dashboard/share/create|{\"recipientEmail\":\"user@example.com\",\"permissions\":\"view\"}|200|Create shared dashboard link"

    # --- Route 198-199 Additions ---
    "GET|/api/dashboard/scorecard||200|Get research scorecard with key metrics"
    "POST|/api/dashboard/milestone/create|{\"title\":\"Complete literature review\",\"targetDate\":\"2024-12-31\",\"description\":\"Review all ML papers\"}|200|Create a research milestone"

    # --- Route 200-201 Additions ---
    "GET|/api/dashboard/productivity/score?period=week||200|Get productivity score over time"
    "GET|/api/dashboard/productivity/score?period=month||200|Get productivity score for month"
    "GET|/api/dashboard/productivity/score?period=year||200|Get productivity score for year"
    "POST|/api/dashboard/layout/reset|{\"userId\":1}|200|Reset dashboard layout to default"

    # --- Route 202-203 Additions ---
    "GET|/api/dashboard/subscription/status||200|Get subscription status and usage limits"
    "POST|/api/dashboard/snapshot/create|{\"name\":\"Weekly Overview\",\"widgets\":[\"stats\",\"chart\",\"reading\"]}|200|Create dashboard snapshot"

    # --- Route 204-205 Additions ---
    "GET|/api/dashboard/activity/timeline?days=7||200|Get activity timeline"
    "GET|/api/dashboard/activity/timeline||200|Get activity timeline default"
    "POST|/api/dashboard/export/custom|{\"format\":\"json\",\"dateRange\":\"2024-01-01:2024-12-31\",\"sections\":[\"stats\",\"activities\"]}|200|Create custom export"
)
