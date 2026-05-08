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
)
