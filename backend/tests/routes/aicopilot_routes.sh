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
    "POST|/api/ai-co-pilot/sessions/1/rename|{\"name\":\"New Session Name\"}|200|Rename session"
    "GET|/api/ai-co-pilot/sessions/recent||200|Get recent sessions"
    "POST|/api/ai-co-pilot/export|{\"sessionId\":\"session_001\",\"format\":\"markdown\"}|200|Export conversation"

    # --- v5 Additions ---
    "POST|/api/ai-co-pilot/sessions/1/pin|{\"pinned\":true}|200|Pin/unpin a session"
    "GET|/api/ai-co-pilot/sessions/pinned||200|Get pinned sessions"
    "POST|/api/ai-co-pilot/sessions/1/summarize||200|Summarize a conversation session"

    # --- v6 Additions ---
    "POST|/api/ai-co-pilot/sessions/1/bookmark|{\"messageId\":\"1\",\"bookmarked\":true}|200|Bookmark/unbookmark a message"
    "GET|/api/ai-co-pilot/bookmarks||200|Get all bookmarked messages"
    "POST|/api/ai-co-pilot/sessions/merge|{\"sessionIds\":[\"s1\",\"s2\"],\"newName\":\"Merged\"}|200|Merge multiple sessions"

    # --- Round 21 Additions ---
    "GET|/api/ai-co-pilot/sessions/1/context||200,404|Get session context"
    "POST|/api/ai-co-pilot/sessions/1/clear|{}|200|Clear session messages"
    "GET|/api/ai-co-pilot/sessions/search?q=test||200|Search across sessions"

    # --- Round 25 Additions ---
    "POST|/api/ai-co-pilot/analyze|{\"text\":\"This is text\",\"type\":\"grammar\"}|200|Analyze text for writing"
    "GET|/api/ai-co-pilot/sessions/stats||200|Get AI CoPilot usage stats"
    "POST|/api/ai-co-pilot/prompts/custom|{\"name\":\"My Prompt\",\"template\":\"Summarize\"}|200|Save custom prompt"

    # --- Round 28 Additions ---
    "POST|/api/ai-co-pilot/sessions/1/export|{\"format\":\"markdown\"}|200|Export session conversation"
    "GET|/api/ai-co-pilot/prompts/popular||200|Get popular prompt templates"
)
