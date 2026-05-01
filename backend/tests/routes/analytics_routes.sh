#!/bin/bash
# Route definitions for AnalyticsIntelligence module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="AnalyticsIntelligence"
ROUTES=(
    "GET|/api/analytics/impact/1||200,404|Get impact analytics"
    "GET|/api/analytics/interests/1||200,404|Get interest analytics"
    "POST|/api/analytics/briefings/generate|{\"userId\":1}|200,404|Generate briefing"
)
