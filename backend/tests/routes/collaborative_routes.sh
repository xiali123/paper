#!/bin/bash
# Route definitions for CollaborativeWriting module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="CollaborativeWriting"
ROUTES=(
    "GET|/api/writing/documents||200,401|List all collaborative documents"
    "POST|/api/writing/documents|{\"title\":\"Test Doc\",\"content\":\"Hello\"}|200,201,401|Create collaborative document"
    "GET|/api/writing/documents/1||200,401,404|Get collaborative document by ID"
    "PUT|/api/writing/documents/1|{\"title\":\"Updated\"}|200,401,404|Update collaborative document"
    "DELETE|/api/writing/documents/1||200,401,404|Delete collaborative document"
    "POST|/api/writing/documents/1/operations|{\"type\":\"insert\",\"position\":0,\"text\":\"test\"}|200,401,404|Submit operation to document"
    "GET|/api/writing/documents/1/versions||200,401,404|List document versions"
    "POST|/api/writing/documents/1/versions|{}|200,401,404|Create document version"
    "GET|/api/writing/documents/1/suggestions||200,401,404|List document suggestions"
    "POST|/api/writing/documents/1/suggestions/generate|{}|200,401,404|Generate suggestions for document"
    "PUT|/api/writing/suggestions/1/accept|{}|200,401,404|Accept suggestion"
    "PUT|/api/writing/suggestions/1/reject|{}|200,401,404|Reject suggestion"
    "GET|/api/writing/documents/1/comments||200,401,404|List document comments"
    "POST|/api/writing/documents/1/comments|{\"content\":\"Nice work\"}|200,401,404|Add comment to document"
    "PUT|/api/writing/comments/1/resolve|{}|200,401,404|Resolve comment"
)
