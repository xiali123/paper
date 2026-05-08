#!/bin/bash
# Route definitions for RecommendationApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="RecommendationApi"
ROUTES=(
    "GET|/api/recommendations/papers||200|Get paper recommendations"
    "GET|/api/recommendations/trending||200|Get trending recommendations"
    "GET|/api/recommendations/similar/1||200,404|Get similar papers"
    "GET|/api/recommendations/explain/1||200,404|Get recommendation explanation"
    "GET|/api/recommendations/stats||200|Get recommendation stats"
    "POST|/api/recommendations/feedback|{\"paperId\":1,\"rating\":5}|200,400|Submit recommendation feedback"
)
