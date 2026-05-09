#!/bin/bash
# Route definitions for ExportApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="ExportApi"
ROUTES=(
    "GET|/api/export||200|List exports"
    "POST|/api/export|{\"paperIds\":[1],\"format\":\"JSON\"}|200,201|Create export"
    "GET|/api/export/formats||200|List export formats"
    "GET|/api/export/stats||200|Export stats"
    "GET|/api/export/csv?paperIds=1,2||200,400|Export as CSV"
    "GET|/api/export/json?paperIds=1,2||200,400|Export as JSON"
    "GET|/api/export/excel?paperIds=1||200,400|Export as Excel"
    "GET|/api/export/pdf?paperIds=1||200,400|Export as PDF"
    "GET|/api/export/word?paperIds=1||200,400|Export as Word"
    "GET|/api/export/bibtex?paperIds=1||200,400|Export as BibTeX"
    "GET|/api/export/history||200|Export history"
    "POST|/api/export/track|{\"user_id\":1,\"format\":\"json\"}|200|Track export"
    "GET|/api/export/stats||200|Export stats per format"
    "GET|/api/export/user/1||200|User exports"
)
