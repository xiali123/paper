#!/bin/bash
# Route definitions for CollaborativeWriting module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="CollaborativeWriting"
ROUTES=(
    "GET|/api/collaborative/documents||200|List all collaborative documents"
    "POST|/api/collaborative/documents|{\"title\":\"Test Doc\",\"content\":\"Hello\"}|200,201|Create collaborative document"
    "GET|/api/collaborative/documents/1||200,404|Get collaborative document by ID"
    "PUT|/api/collaborative/documents/1|{\"title\":\"Updated\"}|200,404|Update collaborative document"
    "DELETE|/api/collaborative/documents/1||200,404|Delete collaborative document"
    "POST|/api/collaborative/documents/1/operations|{\"type\":\"insert\",\"position\":0,\"text\":\"test\"}|200,404|Submit operation to document"
    "GET|/api/collaborative/documents/1/versions||200,404|List document versions"
    "POST|/api/collaborative/documents/1/versions|{}|200,404|Create document version"
    "GET|/api/collaborative/documents/1/suggestions||200,404|List document suggestions"
    "POST|/api/collaborative/documents/1/suggestions/generate|{}|200,404|Generate suggestions for document"
    "PUT|/api/collaborative/suggestions/1/accept|{}|200,404|Accept suggestion"
    "PUT|/api/collaborative/suggestions/1/reject|{}|200,404|Reject suggestion"
    "GET|/api/collaborative/documents/1/comments||200,404|List document comments"
    "POST|/api/collaborative/documents/1/comments|{\"content\":\"Nice work\"}|200,404|Add comment to document"
    "PUT|/api/collaborative/comments/1/resolve|{}|200,404|Resolve comment"
)
