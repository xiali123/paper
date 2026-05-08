#!/bin/bash
# Route definitions for AuthApi module
# Add new routes: append to ROUTES array
# Format: "METHOD|/path|body_json|expected_codes|test_name"

MODULE_NAME="AuthApi"
ROUTES=(
    "POST|/api/auth/register|{\"username\":\"testuser\",\"password\":\"Pass1234\",\"email\":\"test@test.com\"}|200,201,400,409|Register user (may already exist)"
    "POST|/api/auth/login|{\"username\":\"testuser\",\"password\":\"Pass1234\"}|200,401|Login with valid credentials"
    "POST|/api/auth/logout|{}|200,401|Logout current session"
    "GET|/api/auth/me||200,401|Get current user info"
    "GET|/api/auth/sessions||200,401|List active sessions"
    "DELETE|/api/auth/sessions/1||200,401,404|Delete session by id"
    "POST|/api/auth/refresh|{\"token\":\"test\"}|200,400,401|Refresh auth token"
    "POST|/api/auth/change-password|{\"old_password\":\"old\",\"new_password\":\"NewPass123\"}|200,401|Change password"
    "POST|/api/auth/reset-password|{\"email\":\"test@test.com\"}|200,404|Request password reset"
    "POST|/api/auth/reset-password/complete|{\"token\":\"reset_token\",\"new_password\":\"NewPass123\"}|200,400|Complete password reset"
    "POST|/api/auth/login|{\"username\":\"testuser\",\"password\":\"wrong\"}|401|Login with wrong password"
    "POST|/api/auth/login|{}|400,401|Login with empty body"
    "POST|/api/auth/register|{\"username\":\"weak\",\"password\":\"123\",\"email\":\"w@w.com\"}|400|Register with weak password"
    "POST|/api/auth/login|{\"username\":\"nonexistent_user_xyz\",\"password\":\"wrong\"}|401|Login with non-existent user"
    "POST|/api/auth/register|{\"username\":\"testuser\",\"password\":\"Pass1234\",\"email\":\"test@test.com\"}|200,201,409|Register duplicate user"
    "GET|/api/auth/profile||200,401|Get user profile (no auth=401)"
    "PUT|/api/auth/profile|{\"full_name\":\"Test User\"}|200,401|Update profile (no auth=401)"
)
