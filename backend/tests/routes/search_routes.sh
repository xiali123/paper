#!/bin/bash
# Route definitions for SearchApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="SearchApi"
ROUTES=(
    "GET|/api/search?query=test||200|Search papers"
    "GET|/api/search/suggest?query=test||200|Search suggestions"
    "GET|/api/search/trending||200|Trending searches"
    "GET|/api/search/history||200,401|Search history"
    "GET|/api/search/saved||200,401|Saved searches"
    "GET|/api/search/stats||200|Search stats"
    "POST|/api/search/advanced|{\"query\":\"test\",\"filters\":{}}|200|Advanced search"
    "POST|/api/search/save|{\"query\":\"test\",\"name\":\"My Search\"}|200,201|Save search"
    "DELETE|/api/search/history||200,401|Clear search history"
    "DELETE|/api/search/saved/1||200,401,404|Delete saved search"
    "GET|/api/search?query=||200|Search with empty query"
    "GET|/api/search/suggest?query=%E2%86%90%E2%86%92||200|Suggest with unicode chars"
    "GET|/api/search/export?query=test||200|Export search results"
    "GET|/api/search/export?query=test&format=csv||200|Export search as CSV"
    "GET|/api/search/autocomplete?q=BERT||200|Autocomplete suggestions"
    "POST|/api/search/filters|{\"name\":\"My Filter\"}|200|Save search filter"
    "DELETE|/api/search/cache||200|Clear search cache"
    "GET|/api/search/suggest-advanced?q=test||200|Advanced suggestions"
    "GET|/api/search/saved||200|List saved searches"
    "POST|/api/search/saved|{\"user_id\":1,\"query\":\"test\"}|200,201|Save search"
    "GET|/api/search/trending||200|Trending from DB"
)
