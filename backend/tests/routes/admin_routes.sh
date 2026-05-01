#!/bin/bash
# Route definitions for AdminApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="AdminApi"

ROUTES=(
    # --- Stats & Dashboard ---
    "GET|/api/admin/stats||200|Get admin stats"
    "GET|/api/admin/dashboard||200|Get admin dashboard"

    # --- User Management ---
    "GET|/api/admin/users||200|List all users"
    "GET|/api/admin/users/1||200,404|Get user by ID"
    "GET|/api/admin/users/1/history||200,404|Get user history"
    "GET|/api/admin/users/1/sessions||200,404|Get user sessions"
    "POST|/api/admin/users|{\"username\":\"newadmin\",\"password\":\"Pass1234\",\"email\":\"admin@test.com\"}|200,201|Create user"
    "PUT|/api/admin/users/1|{\"username\":\"updated\"}|200,404|Update user"
    "DELETE|/api/admin/users/1||200,404|Delete user"
    "POST|/api/admin/users/1/activate|{}|200,404|Activate user"
    "POST|/api/admin/users/1/deactivate|{}|200,404|Deactivate user"
    "POST|/api/admin/users/1/change-password|{\"new_password\":\"NewPass123\"}|200,404|Change user password"
    "POST|/api/admin/users/1/reset-password|{}|200,404|Reset user password"
    "DELETE|/api/admin/users/1/sessions/1||200,404|Delete user session"

    # --- Module Management ---
    "GET|/api/admin/modules||200|List all modules"
    "GET|/api/admin/modules/scan||200|Scan for modules"
    "POST|/api/admin/modules/AuthApiModule/enable|{}|200,404|Enable module"
    "POST|/api/admin/modules/AuthApiModule/disable|{}|200,404|Disable module"
    "POST|/api/admin/modules/upload|{}|200,400|Upload module"
    "POST|/api/admin/modules/install|{\"name\":\"TestModule\"}|200,404|Install module"
    "POST|/api/admin/modules/AuthApiModule/reload|{}|200,404|Reload module"
    "DELETE|/api/admin/modules/TestModule/uninstall||200,404|Uninstall module"

    # --- Audit & Monitoring ---
    "GET|/api/admin/audit-logs||200|Get audit logs"
    "GET|/api/admin/monitor/system||200|Get system monitor"
    "GET|/api/admin/monitor/services||200|Get services monitor"
    "GET|/api/admin/monitor/logs||200|Get monitor logs"
    "GET|/api/admin/monitor/logs/stats||200|Get monitor log stats"
    "DELETE|/api/admin/monitor/logs/before/2024-01-01||200|Delete monitor logs before date"

    # --- Performance ---
    "GET|/api/admin/performance/metrics||200|Get performance metrics"
    "GET|/api/admin/performance/slow-queries||200|Get slow queries"
    "GET|/api/admin/performance/bottlenecks||200|Get performance bottlenecks"

    # --- Security ---
    "GET|/api/admin/security/login-history||200|Get login history"
    "GET|/api/admin/security/login-stats||200|Get login stats"
    "GET|/api/admin/security/suspicious||200|Get suspicious activities"
    "GET|/api/admin/security/ip-blacklist||200|Get IP blacklist"
    "GET|/api/admin/security/account-lockouts||200|Get account lockouts"
    "POST|/api/admin/security/ip-blacklist|{\"ip\":\"1.2.3.4\"}|200|Add IP to blacklist"
    "POST|/api/admin/security/lock-user|{\"userId\":1}|200,404|Lock user account"
    "POST|/api/admin/security/unlock-user|{\"userId\":1}|200,404|Unlock user account"
    "POST|/api/admin/security/suspicious/1/handle|{\"action\":\"resolved\"}|200,404|Handle suspicious activity"

    # --- Config ---
    "GET|/api/admin/config/categories||200|Get config categories"
    "GET|/api/admin/config||200|Get all config"
    "GET|/api/admin/config/history||200|Get config history"
    "GET|/api/admin/config/summary||200|Get config summary"
    "POST|/api/admin/config/reload|{}|200|Reload config"
    "PUT|/api/admin/config|{\"key\":\"value\"}|200|Update config"

    # --- Backup ---
    "GET|/api/admin/backup/jobs||200|List backup jobs"
    "GET|/api/admin/backup/records||200|List backup records"
    "GET|/api/admin/backup/stats||200|Get backup stats"
    "POST|/api/admin/backup/jobs|{\"name\":\"daily backup\"}|200,201|Create backup job"
    "POST|/api/admin/backup/jobs/1/trigger|{}|200,404|Trigger backup job"
    "PUT|/api/admin/backup/jobs/1|{\"name\":\"updated\"}|200,404|Update backup job"
    "DELETE|/api/admin/backup/jobs/1||200,404|Delete backup job"
    "DELETE|/api/admin/backup/records/1||200,404|Delete backup record"

    # --- RBAC ---
    "GET|/api/admin/roles||200|List all roles"
    "GET|/api/admin/permissions||200|List all permissions"
    "GET|/api/admin/permission-matrix||200|Get permission matrix"
    "GET|/api/admin/roles/1/permissions||200,404|Get role permissions"
    "GET|/api/admin/users/1/roles||200,404|Get user roles"
    "POST|/api/admin/roles|{\"name\":\"editor\"}|200,201|Create role"
    "PUT|/api/admin/roles/1/permissions|{\"permissions\":[\"read\"]}|200,404|Assign permissions to role"
    "POST|/api/admin/users/1/roles|{\"roleIds\":[1]}|200,404|Assign roles to user"
    "PUT|/api/admin/roles/1|{\"name\":\"updated\"}|200,404|Update role"
    "DELETE|/api/admin/roles/1||200,404|Delete role"
    "DELETE|/api/admin/users/1/roles/1||200,404|Remove role from user"

    # --- Notifications ---
    "GET|/api/admin/notifications/templates||200|List notification templates"
    "GET|/api/admin/notifications||200|List notifications"
    "GET|/api/admin/notifications/history||200|Get notification history"
    "GET|/api/admin/notifications/stats||200|Get notification stats"
    "POST|/api/admin/notifications/templates|{\"name\":\"welcome\",\"content\":\"Hello\"}|200,201|Create notification template"
    "POST|/api/admin/notifications/send|{\"userId\":1,\"templateId\":1}|200,404|Send notification"
    "PUT|/api/admin/notifications/templates/1|{\"name\":\"updated\"}|200,404|Update notification template"
    "DELETE|/api/admin/notifications/templates/1||200,404|Delete notification template"

    # --- Announcements ---
    "GET|/api/admin/announcements||200|List announcements"
    "POST|/api/admin/announcements|{\"title\":\"Test\",\"content\":\"Content\"}|200,201|Create announcement"
    "POST|/api/admin/announcements/1/toggle|{}|200,404|Toggle announcement"
    "PUT|/api/admin/announcements/1|{\"title\":\"Updated\"}|200,404|Update announcement"
    "DELETE|/api/admin/announcements/1||200,404|Delete announcement"

    # --- Cleanup ---
    "GET|/api/admin/cleanup/tasks||200|List cleanup tasks"
    "GET|/api/admin/cleanup/history||200|Get cleanup history"
    "GET|/api/admin/cleanup/storage-stats||200|Get cleanup storage stats"
    "POST|/api/admin/cleanup/tasks|{\"name\":\"temp cleanup\"}|200,201|Create cleanup task"
    "POST|/api/admin/cleanup/tasks/1/trigger|{}|200,404|Trigger cleanup task"
    "PUT|/api/admin/cleanup/tasks/1|{\"name\":\"updated\"}|200,404|Update cleanup task"
    "DELETE|/api/admin/cleanup/tasks/1||200,404|Delete cleanup task"

    # --- Content Management ---
    "GET|/api/admin/content/pending||200|List pending content"
    "GET|/api/admin/content/pending/1||200,404|Get pending content by ID"
    "GET|/api/admin/content/reports||200|List content reports"
    "GET|/api/admin/content/sensitive-words||200|List sensitive words"
    "GET|/api/admin/content/sensitive-words/stats||200|Get sensitive word stats"
    "POST|/api/admin/content/pending/1/approve|{}|200,404|Approve pending content"
    "POST|/api/admin/content/pending/1/reject|{\"reason\":\"inappropriate\"}|200,404|Reject pending content"
    "POST|/api/admin/content/reports/1/resolve|{\"action\":\"resolved\"}|200,404|Resolve content report"
    "POST|/api/admin/content/sensitive-words|{\"word\":\"badword\"}|200,201|Add sensitive word"
    "POST|/api/admin/content/sensitive-words/check|{\"text\":\"some content\"}|200|Check sensitive words"
    "DELETE|/api/admin/content/sensitive-words/1||200,404|Delete sensitive word"

    # --- API Keys ---
    "GET|/api/admin/api-keys||200|List API keys"
    "GET|/api/admin/api-keys/usage||200|Get API key usage"
    "GET|/api/admin/api-keys/stats||200|Get API key stats"
    "POST|/api/admin/api-keys|{\"name\":\"test-key\"}|200,201|Create API key"
    "POST|/api/admin/api-keys/1/regenerate|{}|200,404|Regenerate API key"
    "DELETE|/api/admin/api-keys/1||200,404|Delete API key"

    # --- Export ---
    "POST|/api/admin/export/users|{\"format\":\"csv\"}|200|Export users"

    # Auto-synced 2026-05-02
    "DELETE|/api/admin/security/ip-blacklist/1||200,201,404|Auto: DELETE security/ip-blacklist/:id"
    "POST|/api/admin/permissions/check|{"text":"test"}|200,201,404|Auto: POST permissions/check"
)
