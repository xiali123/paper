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
)
