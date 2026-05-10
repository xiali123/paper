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
)
