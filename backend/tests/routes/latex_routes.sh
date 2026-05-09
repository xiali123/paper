#!/bin/bash
# Route definitions for LatexApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="LatexApi"
ROUTES=(
    # Documents
    "GET|/api/latex/documents||200|List all LaTeX documents"
    "GET|/api/latex/documents/1||200,404|Get LaTeX document by ID"
    "POST|/api/latex/documents|{\"title\":\"Test Doc\",\"content\":\"\\\\documentclass{article}\"}|200,201|Create LaTeX document"
    "PUT|/api/latex/documents/1|{\"title\":\"Updated\"}|200,404|Update LaTeX document"
    "DELETE|/api/latex/documents/1||200,404|Delete LaTeX document"
    "POST|/api/latex/documents/1/compile|{}|200,404|Compile LaTeX document"
    "GET|/api/latex/documents/1/pdf||200,404|Get LaTeX document PDF"

    # Projects
    "GET|/api/latex/projects||200|List all LaTeX projects"
    "GET|/api/latex/projects/1||200,404|Get LaTeX project by ID"
    "POST|/api/latex/projects|{\"name\":\"Test Project\"}|200,201|Create LaTeX project"
    "GET|/api/latex/projects/1/files||200,404|List project files"
    "POST|/api/latex/projects/files|{\"projectId\":1,\"name\":\"main.tex\",\"content\":\"test\"}|200,201|Create project file"
    "GET|/api/latex/projects/files/1||200,404|Get project file by ID"
    "PUT|/api/latex/projects/files/1|{\"content\":\"updated\"}|200,404|Update project file"
    "DELETE|/api/latex/projects/files/1||200,404|Delete project file"
    "POST|/api/latex/projects/1/upload|{}|200,201,400,404|Upload file to project"
    "POST|/api/latex/projects/1/batch-upload|{}|200,201,400,404|Batch upload files to project"
    "POST|/api/latex/projects/1/import|{\"source\":\"test\"}|200,201,404|Import project"
    "POST|/api/latex/projects/1/compile|{}|200,404|Compile LaTeX project"
    "GET|/api/latex/projects/1/pdf||200,404|Get LaTeX project PDF"

    # Templates
    "GET|/api/latex/templates||200|List LaTeX templates"

    # Cache
    "GET|/api/latex/cache/stats||200|Get cache stats"
    "POST|/api/latex/cache/clear|{}|200|Clear cache"

    # Stats
    "GET|/api/latex/stats||200|Get LaTeX stats"

    # Versioning
    "GET|/api/latex/versions/history||200|Get version history"
    "GET|/api/latex/versions/tree||200|Get version tree"
    "GET|/api/latex/versions/compare||200|Compare versions"
    "POST|/api/latex/versions/save|{\"documentId\":1,\"content\":\"test\"}|200,201,404|Save version"
    "POST|/api/latex/versions/restore|{\"versionId\":1}|200,404|Restore version"
    "POST|/api/latex/versions/branch|{\"versionId\":1,\"name\":\"feature\"}|200,201,404|Create version branch"
    "POST|/api/latex/versions/merge|{\"sourceId\":1,\"targetId\":2}|200,404|Merge versions"
    "DELETE|/api/latex/versions/1||200,404|Delete version"

    # Debug
    "GET|/api/latex/debug/pdf/document/1||200,404|Debug get PDF for document"

    # Collaboration
    "POST|/api/latex/collaboration/sessions|{\"documentId\":1,\"userId\":\"user1\",\"userName\":\"Test\"}|200|Join collaboration session"
    "DELETE|/api/latex/collaboration/sessions/1|{\"sessionId\":\"collab_1\",\"userId\":\"user1\"}|200|Leave collaboration session"
    "PUT|/api/latex/collaboration/cursor|{\"sessionId\":\"collab_1\",\"userId\":\"user1\",\"line\":5,\"column\":10}|200|Update cursor position"
    "POST|/api/latex/collaboration/broadcast|{\"sessionId\":\"collab_1\",\"content\":\"update\"}|200|Broadcast document update"
    "GET|/api/latex/collaboration/sessions||200|List collaboration sessions"

    # Validate
    "GET|/api/latex/validate||200|Validate LaTeX syntax"
)
