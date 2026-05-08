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
)
