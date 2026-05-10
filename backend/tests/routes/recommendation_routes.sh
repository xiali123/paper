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
    "GET|/api/recommendation/similar/1||200|Get similar papers by paper id"
    "POST|/api/recommendation/blocklist|{\"userId\":\"1\",\"paperId\":\"2\"}|200|Add paper to blocklist"
    "GET|/api/recommendation/blocklist|{\"userId\":\"1\"}|200|Get user blocklist"
    "GET|/api/recommendation/trending||200|Get trending papers (weighted score)"
    "POST|/api/recommendation/feedback|{\"userId\":1,\"paperId\":1,\"rating\":5,\"feedback\":\"helpful\"}|200|Submit recommendation feedback"
    "GET|/api/recommendation/stats||200|Get recommendation system statistics"
    "GET|/api/recommendation/recently-viewed||200|Get recently viewed papers"
    "POST|/api/recommendation/collaborative|{\"userId\":\"1\"}|200,400|Get collaborative filtering recommendations"
    "GET|/api/recommendation/diverse||200|Get diverse recommendations across categories"

    # --- v6 Additions ---
    "GET|/api/recommendation/by-reading||200|Recommend based on reading history"
    "POST|/api/recommendation/reset|{\"userId\":\"1\"}|200|Reset recommendation model for user"
    "GET|/api/recommendation/explain/1||200|Explain why a paper was recommended"

    # --- Round 21 Additions ---
    "POST|/api/recommendations/train|{\"algorithm\":\"collaborative\"}|200|Trigger model retrain"
    "GET|/api/recommendations/quality||200|Get recommendation quality metrics"
    "POST|/api/recommendations/cross-domain|{\"paperId\":1,\"domains\":[\"ml\",\"nlp\"]}|200|Get cross-domain recommendations"

    # --- Round 25 Additions ---
    "GET|/api/recommendations/engines||200|List recommendation engines"
    "POST|/api/recommendations/explain/1|{}|200|Explain recommendation"
    "POST|/api/recommendations/a-b-test|{\"name\":\"test1\",\"engineA\":\"collaborative\",\"engineB\":\"content\",\"duration\":7}|200|Create A/B test"

    # --- Round 28 Additions ---
    "POST|/api/recommendations/weights|{\"collaborative\":0.4,\"content\":0.3}|200|Set recommendation weights"
    "GET|/api/recommendations/user/1/profile||200|Get user recommendation profile"
    "DELETE|/api/recommendations/cache||200|Clear recommendation cache"
)
