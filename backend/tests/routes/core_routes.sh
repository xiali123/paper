#!/bin/bash
# Route definitions for Core (System) module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="Core (System)"
ROUTES=(
    "GET|/api/health||200|Health check"
    "GET|/api/modules||200|List all modules"
    "GET|/api/modules/AuthApiModule||200,404|Get module by name"
    "GET|/api/system/info||200|Get system info"
    "POST|/api/modules/AuthApiModule/reload|{}|200,404|Reload module"
)
