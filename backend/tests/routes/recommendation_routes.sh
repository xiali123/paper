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
    "GET|/api/recommendations/profile/1||200|User recommendation profile"
    "GET|/api/recommendations/collaborators/1||200|Get collaborator recommendations"
    "POST|/api/recommendations/batch|{\"paper_ids\":[1,2],\"limit\":5}|200|Batch recommendations"
    "GET|/api/recommendations/feedback/history/1||200|Get feedback history"
    "POST|/api/recommendations/feedback|{\"user_id\":1,\"paper_id\":1,\"type\":\"like\"}|200,201|Submit feedback"
    "GET|/api/recommendations/feedback/1||200|User feedback list"
    "GET|/api/recommendations/personalized/1||200|Personalized recommendations"
    "POST|/api/recommendations/refresh||200|Refresh recommendation cache"
    "DELETE|/api/recommendations/feedback/1||200|Delete feedback"
    "GET|/api/recommendation/history|{\"userId\":\"1\"}|200|Get user recommendation history"
    "POST|/api/recommendation/ignore|{\"recommendationId\":\"1\",\"userId\":\"1\"}|200|Ignore a recommendation"
    "GET|/api/recommendation/categories||200|Get recommendation categories"
    "POST|/api/recommendation/preference|{\"userId\":\"1\",\"categories\":[\"ml\",\"nlp\"],\"minScore\":0.5}|200|Set recommendation preferences"
)
