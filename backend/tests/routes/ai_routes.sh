#!/bin/bash
# Route definitions for AiApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="AiApi"
ROUTES=(
    "POST|/api/ai/summarize|{\"text\":\"test paper content\"}|200,400|Summarize text"
    "POST|/api/ai/chat|{\"message\":\"hello\"}|200,400|AI chat"
    "POST|/api/ai/keywords|{\"text\":\"test content\"}|200,400|Extract keywords"
    "POST|/api/ai/contributions|{\"paperId\":1}|200,400,404|Get contributions"
    "POST|/api/ai/compare|{\"paperIds\":[1,2]}|200,404|Compare papers"
    "GET|/api/ai/status||200|AI service status"
    "GET|/api/ai/models||200|List available AI models"
    "GET|/api/ai/history||200|Get AI operation history"
    "GET|/api/ai/costs||200|Get AI usage costs"
    "POST|/api/ai/translate|{\"text\":\"Hello world\",\"source_lang\":\"en\",\"target_lang\":\"zh\"}|200,400|Translate text"
    "GET|/api/ai/queue||200|Get task queue status"
    "POST|/api/ai/batch-summarize|{\"paper_ids\":[1,2]}|200,400|Batch summarize papers"
    "GET|/api/ai/sessions||200|List chat sessions"
    "POST|/api/ai/sessions|{\"user_id\":1,\"title\":\"Test session\"}|200,201|Create chat session"
    "DELETE|/api/ai/sessions/1||200|Delete chat session"
    "POST|/api/ai/batch-analyze|{\"paper_ids\":[1,2],\"type\":\"summary\"}|200,400|Batch analyze papers"

    # Session messages
    "GET|/api/ai/sessions/1/messages||200|Get session messages"
    "POST|/api/ai/sessions/1/messages|{\"content\":\"Hello\",\"role\":\"user\"}|200|Send session message"
    "GET|/api/ai/analyze/1||200,404|Get analysis result by ID"
    "POST|/api/ai/papers/1/summary||200,404|Summarize paper by ID"
    "GET|/api/ai/papers/1/keywords||200,404|Extract paper keywords"
    "GET|/api/ai/stats||200|AI module stats"
)
