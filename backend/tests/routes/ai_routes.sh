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
)
