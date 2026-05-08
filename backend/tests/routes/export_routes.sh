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
)
