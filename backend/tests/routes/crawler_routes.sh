#!/bin/bash
# Route definitions for CrawlerApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="CrawlerApi"

# --- Template Management ---
ROUTES=(
    "POST|/api/crawler/templates|{\"name\":\"test\",\"baseUrl\":\"https://example.com\"}|200,201|Create crawler template"
    "GET|/api/crawler/templates||200|List all crawler templates"
    "GET|/api/crawler/templates/test_tpl_001||200,404|Get crawler template by ID"
    "PUT|/api/crawler/templates/test_tpl_001|{\"name\":\"updated\"}|200,404|Update crawler template"
    "DELETE|/api/crawler/templates/test_tpl_001||200,404|Delete crawler template"
    "POST|/api/crawler/templates/validate|{\"name\":\"test\",\"baseUrl\":\"https://example.com\"}|200,404|Validate crawler template"
    "POST|/api/crawler/templates/test_tpl_001/test|{}|200,400,404|Test crawler template"

    # --- Task Management ---
    "POST|/api/crawler/tasks|{\"templateId\":\"test_tpl_001\",\"priority\":\"NORMAL\"}|200,201|Create crawler task"
    "GET|/api/crawler/tasks||200|List all crawler tasks"
    "GET|/api/crawler/tasks/task_123||200,404|Get crawler task by ID"
    "DELETE|/api/crawler/tasks/task_123||200,404|Delete crawler task"
    "POST|/api/crawler/tasks/task_123/retry|{}|200,404|Retry crawler task"
    "GET|/api/crawler/statistics||200|Get crawler task statistics"

    # --- Schedule Management ---
    "POST|/api/crawler/schedules|{\"name\":\"daily\",\"templateId\":\"test_tpl_001\",\"cronExpression\":\"0 2 * * *\"}|200,201|Create crawler schedule"
    "GET|/api/crawler/schedules||200|List all crawler schedules"
    "POST|/api/crawler/schedules/schedule_123/trigger|{}|200,404|Trigger crawler schedule"

    # --- Worker Management ---
    "GET|/api/crawler/workers||200|List all crawler workers"
    "GET|/api/crawler/workers/worker_001||200,404|Get crawler worker by ID"

    # --- Dashboard ---
    "GET|/api/crawler/dashboard||200|Get crawler dashboard"

    # --- Marketplace ---
    "POST|/api/crawler/marketplace/publish|{\"templateId\":\"test_tpl_001\"}|200,401|Publish template to marketplace"
    "GET|/api/crawler/marketplace/templates||200,401|Browse marketplace templates"
    "POST|/api/crawler/marketplace/templates/test_tpl_001/install|{}|200,401|Install marketplace template"
    "POST|/api/crawler/marketplace/templates/test_tpl_001/rate|{\"rating\":4}|200,401|Rate marketplace template"
    "GET|/api/crawler/marketplace/search?query=test||200,401|Search marketplace templates"

    # --- v4 Additions ---
    "GET|/api/crawler/statistics/summary||200|Get crawler statistics summary"
    "POST|/api/crawler/tasks/task_123/cancel|{}|200|Cancel a crawl task"

    # --- v4 Additional ---
    "GET|/api/crawler/tasks/task_123/logs||200|Get task execution logs"
    "POST|/api/crawler/tasks/task_123/retry-v2|{}|200|Retry a failed task (v2)"
    "GET|/api/crawler/health||200|Crawler health check"
    "GET|/api/crawler/sources||200|List crawler sources"
    "POST|/api/crawler/sources/add|{\"url\":\"https://example.com\",\"name\":\"Test Source\"}|200|Add new crawl source"
    "GET|/api/crawler/performance||200|Get crawler performance metrics"
    "GET|/api/crawler/queue||200|Get current crawl queue"
    "POST|/api/crawler/prioritize|{\"taskId\":\"task_123\",\"priority\":5}|200|Change task priority"
    "GET|/api/crawler/errors||200|Get recent crawl errors"
)
