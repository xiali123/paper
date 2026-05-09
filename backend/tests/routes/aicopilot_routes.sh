#!/bin/bash
# Route definitions for AiCoPilot module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="AiCoPilot"
ROUTES=(
    "POST|/api/ai-co-pilot/review|{\"paperId\":1}|200,404|Submit paper for AI review"
    "POST|/api/ai-co-pilot/literature-review/generate|{\"topic\":\"machine learning\"}|200|Generate literature review"
    "POST|/api/ai-co-pilot/research-plan/generate|{\"topic\":\"deep learning\"}|200|Generate research plan"
    "POST|/api/ai-co-pilot/chat|{\"message\":\"hello\"}|200|Send chat message"
    "GET|/api/ai-co-pilot/reviews/1||200,404|Get AI review by ID"
    "GET|/api/ai-co-pilot/literature-reviews||200|List all literature reviews"
    "GET|/api/ai-co-pilot/research-plans||200|List all research plans"
    "GET|/api/ai-co-pilot/conversations||200|List all conversations"
    "GET|/api/ai-co-pilot/recommendations||200|Get AI recommendations"
    "GET|/api/ai-co-pilot/stats||200|Get AI co-pilot stats"
    "GET|/api/ai-co-pilot/costs||200|Get AI co-pilot costs"
    "GET|/api/ai-co-pilot/reviews||200|Review history"
    "GET|/api/ai-co-pilot/literature-reviews||200|Literature review history"
    "GET|/api/ai-co-pilot/plans||200|Research plan history"
    "GET|/api/ai-co-pilot/sessions/1/messages||200|Get session messages"
    "POST|/api/ai-co-pilot/regenerate/1||200|Regenerate review"
    "POST|/api/ai-co-pilot/feedback|{\"type\":\"review\",\"rating\":5,\"comment\":\"Good\"}|200|Submit AI feedback"
)
