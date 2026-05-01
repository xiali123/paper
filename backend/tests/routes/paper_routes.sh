#!/bin/bash
# Route definitions for PaperApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="PaperApi"
ROUTES=(
    "GET|/api/papers||200|List all papers"
    "GET|/api/papers?page=1&limit=10||200|List papers with pagination"
    "GET|/api/papers/1||200,404|Get paper by id"
    "GET|/api/papers/search?query=test||200|Search papers by query"
    "GET|/api/papers/search?author=test||200|Search papers by author"
    "GET|/api/papers/search?yearFrom=2020&yearTo=2024||200|Search papers by year range"
    "GET|/api/papers/stats||200|Get paper statistics"
    "GET|/api/papers/export?format=json||200|Export papers as json"
    "POST|/api/papers|{\"title\":\"Test Paper\",\"authors\":\"Author\",\"year\":2024}|200,201|Create new paper"
    "POST|/api/papers/1/favorite|{\"favorite\":true}|200,404|Toggle paper favorite"
    "POST|/api/papers/1/read|{\"is_read\":true}|200,404|Mark paper as read"
    "POST|/api/papers/1/tags|{\"tags\":[\"test\"]}|200,404|Add tags to paper"
    "PUT|/api/papers/1|{\"title\":\"Updated\"}|200,404|Update paper"
    "DELETE|/api/papers/1||200,404|Delete paper"
    "DELETE|/api/papers/1/tags/test||200,404|Remove tag from paper"
    "GET|/api/papers/abc||400,404|Get paper with invalid id"
    "POST|/api/papers|{}|400|Create paper with empty body"
    "GET|/api/papers/search?query=%3Cscript%3Ealert(1)%3C%2Fscript%3E||200|Search with XSS characters"
    "GET|/api/papers/search?query=%27%20OR%20%271%27%3D%271||200|Search with SQL injection attempt"
)
