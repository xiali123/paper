#!/bin/bash
# Route definitions for StatsApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="StatsApi"
ROUTES=(
    "GET|/api/stats||200|Get stats"
    "GET|/api/stats/system||200,500|System stats"
    "GET|/api/stats/resources||200|Resource stats"
    "GET|/api/stats/uptime||200|Uptime stats"
    "GET|/api/stats/modules||200|Module stats"
    "GET|/api/stats/performance||200|Performance stats"
    "GET|/api/stats/all||200|Aggregated all stats"
    "GET|/api/stats/journals||200|Journal distribution"
    "GET|/api/stats/years||200|Year distribution"
    "GET|/api/stats/authors||200|Author stats"
    "GET|/api/stats/citation-trends||200|Citation trends"
    "GET|/api/stats/top-authors||200|Top authors"
    "GET|/api/stats/research-trends||200|Research trends"
    "GET|/api/stats/growth||200|Growth timeline"
    "GET|/api/stats/ccf||200|CCF distribution"

    # New analytics
    "GET|/api/stats/institutions||200|Institution distribution"
    "GET|/api/stats/keywords||200|Keyword cloud"
    "GET|/api/stats/compare||200|Period comparison"
    "GET|/api/stats/compare?period=week||200|Weekly comparison"
    "GET|/api/stats/all||200|Aggregated all stats"
    "GET|/api/stats/fields||200|Research field distribution"

    # Geographic & timeline analytics
    "GET|/api/stats/authors/top||200|Top authors by paper count (ranked)"
    "GET|/api/stats/timeline||200|Paper publication timeline"
    "GET|/api/stats/timeline?years=10||200|Paper publication timeline (10 years)"
    "GET|/api/stats/geography||200|Geographic distribution of papers"

    # Citation, growth-rate & keyword analytics
    "GET|/api/stats/citations||200|Citation statistics"
    "GET|/api/stats/growth-rate||200|Growth rate calculation"
    "GET|/api/stats/top-keywords||200|Top keywords in papers"

    # Reading progress, collection & activity analytics
    "GET|/api/stats/reading-progress||200|User reading progress stats"
    "GET|/api/stats/collection-size||200|Collection size distribution"
    "GET|/api/stats/activity-heatmap||200|Activity heatmap data"

    # Export summary, user activity, and search analytics
    "GET|/api/stats/export-summary||200|Export statistics summary"
    "GET|/api/stats/user-activity||200|User activity statistics"
    "GET|/api/stats/search-analytics||200|Search analytics"

    # Journal ranking, year-over-year, engagement
    "GET|/api/stats/journal-ranking||200|Journal ranking by paper count"
    "GET|/api/stats/year-over-year||200|Year-over-year growth comparison"
    "GET|/api/stats/engagement||200|User engagement metrics"

    # --- Round 22 Additions ---
    "GET|/api/stats/papers/by-source||200|Papers grouped by source"
    "GET|/api/stats/users/growth||200|User registration growth"
    "POST|/api/stats/export|{\"type\":\"papers\",\"format\":\"json\",\"dateRange\":\"30d\"}|200|Export statistics report"
)
