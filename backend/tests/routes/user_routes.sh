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

    # --- Round 30 Additions ---
    "POST|/api/users/1/verify-email|{\"email\":\"test@test.com\"}|200|Send email verification"
    "GET|/api/users/1/oauth/connections||200|Get OAuth connections"

    # --- Round 32 Additions ---
    "POST|/api/users/1/notifications/settings|{\"email\":true,\"push\":false}|200|Update notification settings"
    "GET|/api/users/1/export/status||200|Get export status"

    # --- Round 33 Additions ---
    "POST|/api/users/1/preferences/reset|{\"categories\":[\"notifications\"]}|200|Reset user preferences"
    "GET|/api/users/1/notifications/unread-count||200|Get unread notification count"

    # --- Round 34 Additions ---
    "POST|/api/users/1/reading-list|{\"paperId\":42,\"priority\":\"high\",\"notes\":\"Must read\"}|200|Add to reading list"
    "GET|/api/users/1/reading-list?status=unread||200|Get reading list"

    # --- Round 35 Additions ---
    "PUT|/api/users/1/reading-list/1/status|{\"status\":\"reading\",\"progress\":50}|200|Update reading list status"
    "GET|/api/users/1/reading-stats/summary?period=month||200|Get reading stats summary"

    # --- Round 36 Additions ---
    "POST|/api/users/1/api-keys|{\"name\":\"My App\",\"permissions\":[\"read\"],\"expiresIn\":90}|200|Generate API key"
    "GET|/api/users/1/api-keys||200|List API keys"

    # --- Round 37 Additions ---
    "DELETE|/api/users/1/api-keys/key_123||200|Revoke API key"
    "GET|/api/users/1/activity/stats?period=month||200|Get activity stats"

    # --- Round 38 Additions ---
    "POST|/api/users/1/connections/link|{\"provider\":\"orcid\",\"accessToken\":\"tok\",\"profileUrl\":\"https://orcid.org/0000\"}|200|Link external account"
    "GET|/api/users/1/connections||200|Get linked accounts"

    # --- Round 39 Additions ---
    "DELETE|/api/users/1/connections/orcid||200|Unlink external account"
    "POST|/api/users/1/connections/orcid/sync||200|Sync external account"

    # --- Round 40 Additions ---
    "GET|/api/users/1/security/log?limit=20||200|Get security audit log"
    "POST|/api/users/1/security/2fa/enable|{\"method\":\"totp\"}|200|Enable 2FA"

    # --- Round 41 Additions ---
    "POST|/api/users/1/security/2fa/verify|{\"code\":\"123456\"}|200|Verify 2FA"
    "POST|/api/users/1/security/2fa/disable|{\"password\":\"pass\",\"code\":\"123456\"}|200|Disable 2FA"

    # --- Round 42 Additions ---
    "GET|/api/users/1/labels||200|Get user labels"
    "POST|/api/users/1/labels|{\"name\":\"Important\",\"color\":\"#FF5733\"}|200|Create label"

    # --- Round 43 Additions ---
    "PUT|/api/users/1/labels/1|{\"name\":\"Updated\",\"color\":\"#00FF00\"}|200|Update label"
    "DELETE|/api/users/1/labels/1||200|Delete label"

    # --- Round 44 Additions ---
    "POST|/api/users/1/papers/42/labels|{\"labelIds\":[1,2]}|200|Apply labels to paper"
    "GET|/api/users/1/papers/labels?labelId=1||200|Get papers by labels"

    # --- Round 45 Additions ---
    "DELETE|/api/users/1/papers/42/labels/1||200|Remove label from paper"
    "GET|/api/users/1/stats/reading-speed?period=month||200|Get reading speed stats"

    # --- Round 46 Additions ---
    "GET|/api/users/1/papers/42/notes||200|Get paper notes"
    "POST|/api/users/1/papers/42/notes|{\"content\":\"Important\",\"page\":5}|200|Add paper note"

    # --- Round 47 Additions ---
    "PUT|/api/users/1/papers/42/notes/1|{\"content\":\"Updated\"}|200|Update paper note"
    "DELETE|/api/users/1/papers/42/notes/1||200|Delete paper note"

    # --- Round 48 Additions ---
    "POST|/api/users/1/preferences/reset|{\"categories\":[\"notifications\",\"display\"]}|200|Reset user preferences to defaults"
    "GET|/api/users/1/activity/summary?period=week||200|Get user activity summary"

    # --- Round 49 Additions ---
    "POST|/api/users/1/avatar/upload|{\"imageData\":\"iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==\",\"format\":\"png\",\"width\":1,\"height\":1}|200|Upload user avatar"
    "GET|/api/users/1/notifications/preferences||200|Get notification preferences"

    # --- Round 50 Additions ---
    "POST|/api/users/1/security/2fa/toggle|{\"enable\":true,\"method\":\"totp\"}|200|Toggle 2FA"
    "GET|/api/users/1/security/sessions||200|Get active sessions"

    # --- Round 51 Additions ---
    "POST|/api/users/1/data/export|{\"format\":\"json\",\"categories\":[\"profile\",\"papers\"]}|200|Export user data"
    "GET|/api/users/1/security/audit-log?limit=10||200|Get security audit log"

    # --- Round 52 Additions ---
    "POST|/api/users/1/blocked/add|{\"targetUserId\":42,\"reason\":\"Harassment\"}|200|Block a user"
    "GET|/api/users/1/blocked?limit=10||200|Get blocked users list"

    # --- Round 53 Additions ---
    "POST|/api/users/1/preferences/privacy|{\"profileVisibility\":true,\"showEmail\":false,\"showActivity\":true}|200|Update privacy settings"
    "GET|/api/users/1/reading/stats?period=month||200|Get reading statistics"

    # --- Round 54 Additions ---
    "POST|/api/users/1/devices/register|{\"deviceName\":\"Work Laptop\",\"deviceType\":\"laptop\",\"os\":\"Windows 11\"}|200|Register user device"
    "GET|/api/users/1/devices||200|List user devices"

    # --- Round 55 Additions ---
    "POST|/api/users/1/devices/token|{\"token\":\"fcm_token_abc123\",\"platform\":\"android\",\"deviceId\":\"dev_001\"}|200|Register device push token"
    "GET|/api/users/1/subscriptions||200|Get user subscriptions"

    # --- Round 56 Additions ---
    "POST|/api/users/1/subscriptions|{\"type\":\"journal\",\"name\":\"Science\",\"frequency\":\"daily\"}|200|Subscribe to content"
    "GET|/api/users/1/recommendation-history||200|Get recommendation history"

    # --- Round 57 Additions ---
    "GET|/api/users/1/metadata||200|Get user metadata"
    "POST|/api/users/1/metadata|{\"researchInterests\":[\"deep learning\"],\"hIndex\":15,\"citationCount\":500}|200|Update user metadata"

    # --- Round 58 Additions ---
    "GET|/api/users/1/research-profiles||200|Get user research profiles"
    "POST|/api/users/1/research-profiles|{\"title\":\"ML Research\",\"specialization\":\"Computer Vision\",\"affiliations\":[\"MIT\"]}|200|Create research profile"

    # --- Round 59 Additions ---
    "GET|/api/users/1/research-profiles/rp_001||200|Get specific research profile"
    "PUT|/api/users/1/research-profiles/rp_001|{\"title\":\"Updated Research\",\"specialization\":\"NLP\",\"affiliations\":[\"Stanford\"]}|200|Update research profile"

    # --- Round 60 Additions ---
    "GET|/api/users/1/research-interests||200|Get user research interests"
    "POST|/api/users/1/research-interests|{\"interests\":[\"deep learning\",\"NLP\"],\"categories\":[\"AI\",\"NLP\"]}|200|Update user research interests"

    # --- Round 61 Additions ---
    "GET|/api/users/1/research-feed?limit=10&offset=0||200|Get user research feed"
    "POST|/api/users/1/citation-alerts|{\"paperId\":42,\"citedBy\":\"Smith et al.\"}|200|Create citation alert"

    # --- Round 62 Additions ---
    "GET|/api/users/1/reading-list/shared?limit=10&offset=0||200|Get shared reading lists"
    "POST|/api/users/1/collaboration-requests|{\"targetUserId\":42,\"message\":\"Let's collaborate\",\"projectTitle\":\"AI Research\"}|200|Send collaboration request"

    # --- Round 63 Additions ---
    "GET|/api/users/1/workspaces?limit=10&offset=0||200|Get user workspaces"
    "POST|/api/users/1/workspaces|{\"name\":\"Research Lab\",\"description\":\"My workspace\",\"visibility\":\"private\"}|200|Create user workspace"

    # --- Round 64 Additions ---
    "GET|/api/users/1/workspace-templates?limit=10&offset=0||200|Get user workspace templates"
    "POST|/api/users/1/invitations|{\"targetEmail\":\"colleague@example.com\",\"role\":\"editor\",\"workspaceId\":\"ws_123\",\"message\":\"Join my workspace\"}|200|Send user invitation"

    # --- Round 65 Additions ---
    "GET|/api/users/1/feedback?limit=10&offset=0||200|Get user feedback list"
    "POST|/api/users/1/highlights|{\"title\":\"Key Finding\",\"content\":\"Important result from paper\",\"color\":\"#FF5733\"}|200|Create user highlight"

    # --- Round 66 Additions ---
    "GET|/api/users/1/skills?limit=10&offset=0||200|Get user skills"
    "POST|/api/users/1/endorsements|{\"targetUserId\":42,\"skillName\":\"Machine Learning\",\"comment\":\"Excellent researcher\"}|200|Create user endorsement"

    # --- Round 67 Additions ---
    "GET|/api/users/1/research-groups?limit=10&offset=0||200|Get user research groups"
    "POST|/api/users/1/mentorship-requests|{\"mentorId\":42,\"topic\":\"Deep Learning\",\"message\":\"I would like mentoring\",\"goals\":\"Publish a paper\"}|200|Create mentorship request"

    # --- Round 68 Additions ---
    "GET|/api/users/1/timeline?limit=10&offset=0||200|Get user timeline"
    "POST|/api/users/1/conferences|{\"name\":\"NeurIPS 2026\",\"location\":\"Vancouver\",\"startDate\":\"2026-12-08\",\"endDate\":\"2026-12-14\",\"role\":\"presenter\"}|200|Add user conference"

    # --- Round 69 Additions ---
    "GET|/api/users/1/grants?limit=10&offset=0||200|Get user research grants"
    "POST|/api/users/1/grants|{\"title\":\"AI Research Grant\",\"agency\":\"NSF\",\"amount\":\"500000\",\"startDate\":\"2026-01-01\",\"endDate\":\"2027-12-31\",\"status\":\"pending\"}|200|Create user research grant"

    # --- Round 70 Additions ---
    "GET|/api/users/1/patents?limit=10&offset=0||200|Get user patents"
    "POST|/api/users/1/patents|{\"title\":\"Neural Network Accelerator\",\"patentNumber\":\"US12345678\",\"filingDate\":\"2026-03-15\",\"status\":\"pending\",\"abstract\":\"A novel hardware accelerator\",\"inventors\":\"John Doe, Jane Smith\"}|200|Create user patent"

    # --- Round 71 Additions ---
    "GET|/api/users/1/publications?limit=10&offset=0||200|Get user publications"
    "POST|/api/users/1/publications|{\"title\":\"Deep Learning for NLP\",\"year\":\"2026\",\"venue\":\"NeurIPS\",\"doi\":\"10.1234/test\",\"type\":\"conference\",\"authors\":\"John Doe, Jane Smith\"}|200|Create user publication"

    # --- Round 72 Additions ---
    "GET|/api/users/1/certifications?limit=10&offset=0||200|Get user certifications"
    "POST|/api/users/1/certifications|{\"name\":\"AWS Solutions Architect\",\"issuer\":\"Amazon Web Services\",\"issuedAt\":\"2026-01-15\",\"expiresAt\":\"2029-01-15\",\"credentialUrl\":\"https://aws.amazon.com/certification/verify\"}|200|Create user certification"

    # --- Round 73 Additions ---
    "GET|/api/users/1/affiliations||200|Get user affiliations"
    "POST|/api/users/1/affiliations|{\"institution\":\"MIT\",\"department\":\"CSAIL\",\"role\":\"Research Scientist\",\"startDate\":\"2026-01-01\",\"endDate\":\"2027-12-31\"}|200|Create user affiliation"

    # --- Round 74 Additions ---
    "GET|/api/users/1/storage?limit=20||200|Get user storage usage"
    "POST|/api/users/1/storage/cleanup|{\"categories\":\"all\",\"olderThanDays\":30}|200|Request storage cleanup"

    # --- Round 75 Additions ---
    "GET|/api/users/1/references?limit=10&offset=0||200|Get user references"
    "POST|/api/users/1/references|{\"referrerName\":\"Dr. Smith\",\"referrerEmail\":\"smith@university.edu\",\"relationship\":\"advisor\",\"recommendation\":\"Excellent researcher\"}|200|Create user reference"

    # --- Round 76 Additions ---
    "GET|/api/users/1/api-keys?limit=10&offset=0||200|Get user API keys"
    "POST|/api/users/1/api-keys|{\"name\":\"My API Key\",\"permissions\":\"read\"}|200|Create user API key"

    # --- Round 77 Additions ---
    "GET|/api/users/1/research-impact?periodMonths=12||200|Get user research impact metrics"
    "POST|/api/users/1/folders|{\"name\":\"AI Papers\",\"description\":\"My collection\",\"visibility\":\"private\",\"color\":\"#FF5733\"}|200|Create user folder"

    # --- Round 78 Additions ---
    "GET|/api/users/1/folders?limit=10&offset=0||200|Get user folders"
    "POST|/api/users/1/feedback|{\"type\":\"bug\",\"subject\":\"Crash on export\",\"message\":\"App crashes when exporting PDF\",\"category\":\"bug_report\",\"priority\":\"high\"}|200|Submit user feedback"

    # --- Round 79 Additions ---
    "GET|/api/users/1/reputation||200|Get user reputation and contribution score"
    "POST|/api/users/1/reputation/endorse|{\"skillName\":\"Machine Learning\",\"comment\":\"Excellent researcher\",\"endorserId\":\"user_42\",\"weight\":2}|200|Endorse user skill"

    # --- Round 80 Additions ---
    "GET|/api/users/1/endorsements?limit=10&offset=0||200|Get user endorsements"
    "POST|/api/users/1/scheduled-reports|{\"reportType\":\"activity_summary\",\"frequency\":\"weekly\",\"format\":\"json\",\"dayOfWeek\":\"monday\"}|200|Schedule periodic report"

    # --- Round 81 Additions ---
    "GET|/api/users/1/research-collaboration-metrics?period=all||200|Get research collaboration metrics"
    "POST|/api/users/1/communication-preferences|{\"emailDigest\":\"daily\",\"notificationFrequency\":\"immediate\",\"quietHoursStart\":\"22:00\",\"quietHoursEnd\":\"08:00\",\"mentionNotifications\":true,\"followerNotifications\":true,\"paperUpdateNotifications\":false}|200|Set communication preferences"

    # --- Round 82 Additions ---
    "GET|/api/users/1/research-timeline?period=year&limit=20||200|Get user research timeline"
    "POST|/api/users/1/institution-transfer|{\"currentInstitution\":\"MIT\",\"targetInstitution\":\"Stanford\",\"targetDepartment\":\"CS\",\"targetRole\":\"Professor\",\"reason\":\"Career growth\",\"effectiveDate\":\"2026-09-01\"}|200|Request institution transfer"

    # --- Round 83 Additions ---
    "GET|/api/users/1/research-milestones?limit=10&offset=0||200|Get user research milestones"
    "POST|/api/users/1/research-milestones|{\"title\":\"First Publication\",\"description\":\"Published first peer-reviewed paper\",\"category\":\"publication\",\"achievedDate\":\"2026-03-15\",\"source\":\"self-reported\"}|200|Create research milestone"

    # --- Round 84 Additions ---
    "GET|/api/users/1/research-awards||200|Get user research awards"
    "POST|/api/users/1/research-awards|{\"title\":\"Best Paper Award\",\"issuer\":\"IEEE\",\"category\":\"honor\",\"awardedDate\":\"2026-05-01\",\"description\":\"Outstanding contribution to the field\"}|200|Create research award"

    # --- Round 85 Additions ---
    "POST|/api/users/1/digital-business-card|{\"style\":\"professional\",\"language\":\"en\",\"includeQR\":true,\"includePublications\":true,\"includeContact\":true}|200|Generate digital business card"
    "GET|/api/users/1/research-network?depth=2&limit=50||200|Get user research collaboration network"

    # --- Round 86 Additions ---
    "GET|/api/users/1/endorsements/given?limit=10&offset=0||200|Get endorsements given by user"
    "POST|/api/users/1/data-import|{\"source\":\"orcid\",\"format\":\"json\",\"overwrite\":false,\"profiles\":[{\"displayName\":\"Dr. Smith\",\"bio\":\"Researcher\",\"affiliation\":\"MIT\"}],\"skills\":[{\"name\":\"Machine Learning\",\"level\":\"expert\"}]}|200|Import user data from external source"

    # --- Round 87 Additions ---
    "GET|/api/users/1/research-analytics?period=year||200|Get user research analytics"
    "POST|/api/users/1/account-recovery|{\"method\":\"email\",\"backupEmail\":\"backup@test.com\"}|200|Initiate account recovery"

    # --- Round 88 Additions ---
    "GET|/api/users/1/skill-map||200|Get user skill map"
    "POST|/api/users/1/availability|{\"status\":\"available\",\"scope\":\"collaboration\",\"message\":\"Open to new projects\",\"maxConcurrent\":3}|200|Set user availability"

    # --- Round 89 Additions ---
    "GET|/api/users/1/reading-streaks||200|Get user reading streaks"
    "POST|/api/users/1/feedback/reactions|{\"feedbackId\":\"fb_001\",\"reaction\":\"helpful\",\"comment\":\"Great insight\"}|200|Submit feedback reaction"

    # --- Round 90 Additions ---
    "GET|/api/users/1/research-snapshots?limit=10&offset=0||200|Get user research snapshots"
    "POST|/api/users/1/delegate-access|{\"delegateToUserId\":42,\"scope\":\"papers\",\"permissions\":\"read\",\"expiresAt\":\"2027-01-01T00:00:00Z\",\"reason\":\"Collaboration\"}|200|Grant delegated access"

    # --- Round 91 Additions ---
    "GET|/api/users/1/delegations||200|Get user delegations (granted and received)"
    "POST|/api/users/1/mood-log|{\"mood\":\"focused\",\"energyLevel\":8,\"note\":\"Great flow state\",\"activity\":\"writing\"}|200|Log user research mood"

    # --- Round 92 Additions ---
    "GET|/api/users/1/research-timeline?year=2024&limit=20||200|Get user research timeline"
    "POST|/api/users/1/notification-preferences|{\"emailNotifications\":true,\"paperAlerts\":true,\"weeklyDigest\":false,\"customCategories\":[\"AI\",\"NLP\"],\"quietHoursStart\":\"22:00\",\"quietHoursEnd\":\"08:00\"}|200|Update notification preferences"

    # --- Round 93 Additions ---
    "GET|/api/users/1/achievements?limit=20||200|Get user achievements"
    "POST|/api/users/1/research-collaboration/request|{\"targetUserId\":2,\"paperId\":\"paper_123\",\"message\":\"Would like to collaborate\",\"collaborationType\":\"co-author\"}|200|Send collaboration request"

    # --- Round 94 Additions ---
    "GET|/api/users/1/reading-preferences||200|Get user reading preferences"
    "POST|/api/users/1/export-data|{\"format\":\"json\",\"includePrivate\":true,\"dateRange\":{\"start\":\"2024-01-01\",\"end\":\"2024-12-31\"},\"sections\":[\"papers\",\"bookmarks\",\"history\"]}|200|Export user data"

    # --- Round 95 Additions ---
    "GET|/api/users/1/collaboration-network?depth=1&limit=20||200|Get user collaboration network"
    "POST|/api/users/1/feedback/submit|{\"feedbackType\":\"bug_report\",\"title\":\"Search not working\",\"description\":\"Detailed description\",\"priority\":\"high\",\"tags\":[\"search\",\"ui\"]}|200|Submit user feedback"

    # --- Round 96 Additions ---
    "GET|/api/users/1/paper-statistics||200,500|paper_statistics"
    "POST|/api/users/1/reading-list/create|{\"name\":\"My List\",\"description\":\"test\"}|200,500|reading_list_create"

    # --- Round 97 Additions ---
    "GET|/api/users/1/notification-summary||200,500|notification_summary"
    "POST|/api/users/1/preferences/batch-update|{\"preferences\":[{\"key\":\"theme\",\"value\":\"dark\"}]}|200,500|preferences_batch_update"

    # --- Round 98 Additions ---
    "GET|/api/users/1/export-history?limit=10||200|Get user export history"
    "POST|/api/users/1/calendar/sync|{\"calendarType\":\"google\",\"syncDirection\":\"export\"}|200|Sync user calendar with reading schedule"

    # --- Round 99 Additions ---
    "GET|/api/users/1/reading-streak||200|Get user reading streak data"
    "POST|/api/users/1/social/link|{\"platform\":\"twitter\",\"accessToken\":\"token123\"}|200|Link social account"

    # --- Round 100 Additions ---
    "GET|/api/users/1/citation-count||200|Get citation count statistics for user papers"
    "POST|/api/users/1/research-goal/create|{\"title\":\"Publish 3 papers\",\"description\":\"Goal to publish in top venues\",\"targetDate\":\"2027-12-31\"}|200|Create a research goal"

    # --- Round 101 Additions ---
    "GET|/api/users/1/paper-comparison?compareWithUserId=2||200|Compare user papers with another researcher"
    "POST|/api/users/1/session/export|{\"format\":\"json\",\"dateRange\":{\"start\":\"2024-01-01\",\"end\":\"2024-12-31\"}}|200|Export user session data"

    # --- Round 102 Additions ---
    "GET|/api/users/1/storage/usage||200|Get user storage usage statistics"
    "POST|/api/users/1/tag/create|{\"name\":\"Important\",\"color\":\"#FF5733\"}|200|Create a custom tag"

    # --- Round 103 Additions ---
    "GET|/api/users/1/reading-speed||200|Get user reading speed analytics"
    "POST|/api/users/1/filter/save|{\"name\":\"My Filter\",\"criteria\":{\"field\":\"AI\",\"year\":\"2026\"}}|200|Save custom filter preset"

    # --- Round 104 Additions ---
    "GET|/api/users/1/following/tags||200|Get tags the user is following"
    "POST|/api/users/1/backup/request|{\"format\":\"json\",\"includeOptions\":{\"papers\":true,\"bookmarks\":true}}|200|Request a data backup"

    # --- Round 105 Additions ---
    "GET|/api/users/1/word-cloud||200|Get user research word cloud data"
    "POST|/api/users/1/subscription/update|{\"plan\":\"pro\",\"billingCycle\":\"monthly\"}|200|Update user subscription"

    # --- Round 106 Additions ---
    "GET|/api/users/1/language/preference||200|Get user language and locale preferences"
    "POST|/api/users/1/device/register|{\"deviceToken\":\"abc123\",\"platform\":\"ios\",\"deviceName\":\"iPhone 15\"}|200|Register device for push notifications"

    # --- Round 107 Additions ---
    "GET|/api/users/1/engagement/score||200|Get user engagement score based on activity"
    "POST|/api/users/1/avatar/upload|{\"imageData\":\"iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==\",\"mimeType\":\"image/png\"}|200|Upload user avatar"

    # --- Route 200-201 ---
    "GET|/api/users/1/connection/stats||200|Get user social connection statistics"
    "POST|/api/users/1/api-key/generate|{\"keyName\":\"My Key\",\"permissions\":[\"read\",\"write\"]}|200|Generate API key for user"

    # --- Route 202-203 ---
    "GET|/api/users/1/notification/rules||200|Get user notification rules and preferences"
    "POST|/api/users/1/invite/generate|{\"role\":\"editor\",\"expiresInDays\":7}|200|Generate an invitation code"
)
