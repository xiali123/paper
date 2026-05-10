#!/bin/bash
# Route definitions for UserApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="UserApi"
ROUTES=(
    "GET|/api/users||200|List all users"
    "GET|/api/users/1||200,404|Get user by id"
    "GET|/api/users/me||200,401|Get current user profile"
    "GET|/api/users/stats||200,401|Get user statistics"
    "POST|/api/users|{\"username\":\"newuser\",\"password\":\"Pass1234\",\"email\":\"new@test.com\"}|200,201,400|Create new user"
    "POST|/api/users/1/activate|{}|200,401,404|Activate user"
    "POST|/api/users/1/suspend|{}|200,401,404|Suspend user"
    "POST|/api/users/1/password|{\"new_password\":\"NewPass123\"}|200,401|Reset user password"
    "PUT|/api/users/1|{\"username\":\"updated\"}|200,401,404|Update user"
    "DELETE|/api/users/1||200,401,404|Delete user"
    "POST|/api/users|{\"username\":\"\"}|200,201,400|Create user with empty username"
    "PUT|/api/users/99999|{\"username\":\"ghost\"}|200,404|Update non-existent user"
    "GET|/api/users/1/activity||200,400|Get user activity log"
    "GET|/api/users/1/login-history||200,400|Get user login history"
    "GET|/api/users/1/permissions||200,400|Get user permissions"
    "PUT|/api/users/1/permissions|{\"roles\":[\"admin\"]}|200,400|Update user permissions"
    "POST|/api/users/batch|{\"action\":\"activate\",\"ids\":[1,2]}|200|Batch user operation"
    "GET|/api/users/export||200|Export users"
    "GET|/api/users/1/notifications||200,400|Get user notifications"
    "GET|/api/users/1/preferences||200,400|Get user preferences"
    "PUT|/api/users/1/preferences|{\"theme\":\"dark\"}|200,400|Update user preferences"
    "POST|/api/users/1/notifications|{\"type\":\"info\",\"title\":\"Test\",\"message\":\"Hello\"}|200|Create notification"
    "PUT|/api/users/1/notifications/1||200|Mark notification read"

    # Activity & preferences (dedicated table routes)
    "GET|/api/users/1/activity||200,400|Get user activity log"
    "PUT|/api/users/1/preferences|{\"theme\":\"dark\",\"language\":\"en\",\"notifications\":false}|200,400|Update user preferences"
    "GET|/api/users/1/preferences||200,400|Get user preferences"

    # Reading history & data export
    "GET|/api/users/1/reading-history||200,400|Get user reading history"
    "DELETE|/api/users/1/reading-history/1||200|Delete reading history entry"
    "GET|/api/users/1/export-data||200|Export user data (GDPR)"

    # Avatar upload, bookmarks, account deletion
    "POST|/api/users/1/avatar-upload|{\"filename\":\"avatar.png\",\"mimeType\":\"image/png\",\"size\":1024}|200|Upload user avatar"
    "GET|/api/users/1/bookmarks||200|Get user bookmarked papers"
    "DELETE|/api/users/1/account|{\"password\":\"confirm_pass\"}|200|Soft delete user account"

    # User notes
    "POST|/api/users/1/notes|{\"title\":\"Research Note\",\"content\":\"Important findings\",\"tags\":[\"research\"]}|200|Create user note"
    "GET|/api/users/1/notes||200|Get user notes"
    "DELETE|/api/users/1/notes/1||200|Delete user note"

    # --- Round 20 Additions ---
    "GET|/api/users/1/following||200|Get users this user follows"
    "POST|/api/users/1/follow|{\"targetUserId\":2}|200|Follow a user"
    "GET|/api/users/1/stats||200|Get user statistics summary"

    # --- Round 23 Additions ---
    "GET|/api/users/1/achievements||200|Get user achievements"
    "POST|/api/users/1/deactivate|{\"reason\":\"break\"}|200|Deactivate user account"
    "GET|/api/users/1/security||200|Get user security settings"

    # --- Round 26 Additions ---
    "GET|/api/users/1/paper-stats||200|Get user paper statistics"
    "POST|/api/users/1/export-data|{}|200|Export all user data"
    "GET|/api/users/1/reading-goals||200|Get reading goals"

    # --- Round 28 Additions ---
    "POST|/api/users/1/avatar/remove|{}|200|Remove user avatar"
    "GET|/api/users/1/collaborations||200|Get user collaborations"
)
