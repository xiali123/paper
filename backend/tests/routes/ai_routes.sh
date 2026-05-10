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
    "POST|/api/ai/compare|{\"paperId1\":1,\"paperId2\":2}|200,400,404|Compare two papers via AI"
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
    "POST|/api/ai/batch-analyze|{\"paperIds\":[1,2,3],\"analyses\":[\"summary\",\"keywords\"]}|200,400|Batch analyze papers"

    # Session messages
    "GET|/api/ai/sessions/1/messages||200|Get session messages"
    "POST|/api/ai/sessions/1/messages|{\"content\":\"Hello\",\"role\":\"user\"}|200|Send session message"
    "GET|/api/ai/analyze/1||200,404|Get analysis result by ID"
    "POST|/api/ai/papers/1/summary||200,404|Summarize paper by ID"
    "GET|/api/ai/papers/1/keywords||200,404|Extract paper keywords"
    "GET|/api/ai/stats||200|AI module stats"
    "POST|/api/ai/paraphrase|{\"text\":\"This is a test sentence\",\"style\":\"academic\"}|200,400|Paraphrase text"
    "GET|/api/ai/usage||200|Get AI API usage statistics"
    "POST|/api/ai/extract-entities|{\"text\":\"Apple was founded by Steve Jobs\"}|200,400|Extract named entities"
    "POST|/api/ai/generate-abstract|{\"title\":\"Test Paper\",\"content\":\"This is the content of a test paper.\"}|200,400|Generate abstract"

    # New AI routes (v2)
    "POST|/api/ai/translate|{\"text\":\"Hello world\",\"sourceLang\":\"en\",\"targetLang\":\"zh\"}|200,400|Translate text v2"
    "GET|/api/ai/session/sess_123||200|Get AI session details"
    "POST|/api/ai/sentiment|{\"text\":\"This paper is excellent and groundbreaking\"}|200,400|Analyze sentiment"

    # Outline, conversations, and abstract scoring
    "POST|/api/ai/outline|{\"topic\":\"deep learning\",\"depth\":3,\"style\":\"academic\"}|200,400|Generate paper outline"
    "GET|/api/ai/conversations||200|List AI conversations"
    "POST|/api/ai/score-abstract|{\"abstract\":\"This paper proposes a novel method for deep learning.\"}|200,400|Score abstract quality"

    # Glossary, batch summarize, quota
    "POST|/api/ai/glossary|{\"terms\":[\"BERT\",\"Transformer\"],\"context\":\"NLP research\"}|200,400|Generate glossary"
    "POST|/api/ai/summarize-batch|{\"texts\":[\"text1\",\"text2\"],\"maxLength\":100}|200,400|Summarize batch texts"
    "GET|/api/ai/quota||200|Get AI usage quota"

    # --- Round 23 Additions ---
    "POST|/api/ai/code-explain|{\"code\":\"def hello(): pass\",\"language\":\"python\"}|200|Explain code snippet"
    "GET|/api/ai/models||200|List available AI models"
    "POST|/api/ai/compare|{\"text1\":\"hello\",\"text2\":\"world\",\"aspect\":\"similarity\"}|200|Compare two texts"

    # --- Round 26 Additions ---
    "POST|/api/ai/detect-language|{\"text\":\"Bonjour\"}|200|Detect language of text"
    "POST|/api/ai/keywords|{\"text\":\"Machine learning papers\",\"maxKeywords\":10}|200|Extract keywords"
    "GET|/api/ai/usage/history?days=30||200|Get AI usage history"
)
