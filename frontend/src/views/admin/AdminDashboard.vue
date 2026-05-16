<template>
  <div class="admin-dashboard">
    <!-- 顶部导航 -->
    <el-row :gutter="20" class="mb-4">
      <el-col :span="18">
        <h2 class="page-title">
          <el-icon><Setting /></el-icon>
          超级用户管理控制台
        </h2>
      </el-col>
      <el-col :span="6" class="text-right">
        <el-tag v-if="authStore.isSuperAdmin" type="danger" size="large">超级管理员</el-tag>
        <el-tag v-else-if="authStore.isAdminOrSuper" type="warning" size="large">管理员</el-tag>
      </el-col>
    </el-row>

    <!-- 选项卡内容 -->
    <el-tabs v-model="activeTab" class="admin-tabs">
      <!-- 概览 -->
      <el-tab-pane label="概览" name="overview">
        <div class="tab-content" v-loading="dashboardLoading">
          <!-- 统计卡片 -->
          <el-row :gutter="20" class="mb-4">
            <el-col :span="6">
              <el-card shadow="hover" class="stat-card">
                <div class="stat-content">
                  <div class="stat-icon" style="background: #409eff">
                    <el-icon><User /></el-icon>
                  </div>
                  <div class="stat-info">
                    <div class="stat-value">{{ stats.totalUsers }}</div>
                    <div class="stat-label">总用户数</div>
                  </div>
                </div>
              </el-card>
            </el-col>
            <el-col :span="6">
              <el-card shadow="hover" class="stat-card">
                <div class="stat-content">
                  <div class="stat-icon" style="background: #67c23a">
                    <el-icon><CircleCheck /></el-icon>
                  </div>
                  <div class="stat-info">
                    <div class="stat-value">{{ stats.recentlyActiveUsers }}</div>
                    <div class="stat-label">近30天活跃</div>
                  </div>
                </div>
              </el-card>
            </el-col>
            <el-col :span="6">
              <el-card shadow="hover" class="stat-card">
                <div class="stat-content">
                  <div class="stat-icon" style="background: #e6a23c">
                    <el-icon><Star /></el-icon>
                  </div>
                  <div class="stat-info">
                    <div class="stat-value">{{ stats.premiumUsers }}</div>
                    <div class="stat-label">高级用户</div>
                  </div>
                </div>
              </el-card>
            </el-col>
            <el-col :span="6">
              <el-card shadow="hover" class="stat-card">
                <div class="stat-content">
                  <div class="stat-icon" style="background: #f56c6c">
                    <el-icon><Memo /></el-icon>
                  </div>
                  <div class="stat-info">
                    <div class="stat-value">{{ stats.enabledModules }}/{{ stats.totalModules }}</div>
                    <div class="stat-label">已启用模块</div>
                  </div>
                </div>
              </el-card>
            </el-col>
          </el-row>

          <!-- Charts row -->
          <el-row :gutter="20" class="mb-4">
            <el-col :span="12">
              <el-card shadow="hover">
                <template #header>用户增长趋势</template>
                <div class="chart-container">
                  <LineChart v-if="userTrendChartData" :data="userTrendChartData" :options="chartOptions" />
                  <el-empty v-else description="暂无数据" />
                </div>
              </el-card>
            </el-col>
            <el-col :span="12">
              <el-card shadow="hover">
                <template #header>系统健康</template>
                <div v-if="dashboardData?.systemHealth">
                  <el-descriptions :column="1" border>
                    <el-descriptions-item label="数据库连接">
                      <el-tag :type="dashboardData.systemHealth.dbConnected ? 'success' : 'danger'">
                        {{ dashboardData.systemHealth.dbConnected ? '正常' : '异常' }}
                      </el-tag>
                    </el-descriptions-item>
                    <el-descriptions-item label="模块状态">
                      {{ dashboardData.systemHealth.modulesHealthy }}/{{ dashboardData.systemHealth.modulesTotal }}
                    </el-descriptions-item>
                  </el-descriptions>
                </div>
                <el-empty v-else description="暂无健康数据" />
              </el-card>
            </el-col>
          </el-row>
        </div>
      </el-tab-pane>

      <!-- 用户管理 -->
      <el-tab-pane label="用户管理" name="users">
        <AdminUserManagement
          :auth-store="authStore"
          :loading="loading"
          :users="users"
          :pagination="pagination"
          :search-query="searchQuery"
          :role-filter="roleFilter"
          :create-dialog-visible="createDialogVisible"
          :create-form="createForm"
          :create-rules="createRules"
          :edit-dialog-visible="editDialogVisible"
          :edit-form="editForm"
          :saving="saving"
          :can-change-role="canChangeRole"
          :user-detail-visible="userDetailVisible"
          :detail-user="detailUser"
          :user-history-tab="userHistoryTab"
          :login-history="loginHistory"
          :user-sessions="userSessions"
          :get-role-label="getRoleLabel"
          :get-role-badge-type="getRoleBadgeType"
          :get-activity-tag-type="getActivityTagType"
          :get-activity-label="getActivityLabel"
          :format-date-time="formatDateTime"
          :can-manage-role="canManageRole"
          @search="handleSearch"
          @load-users="loadUsers"
          @page-change="handlePageChange"
          @size-change="handleSizeChange"
          @create-user="handleCreateUser"
          @confirm-create="handleConfirmCreate"
          @edit-user="handleEditUser"
          @save-user="handleSaveUser"
          @toggle-user-status="handleToggleUserStatus"
          @export-users="handleExportUsers"
          @open-user-detail="openUserDetail"
          @kick-session="handleKickSession"
          @update:search-query="searchQuery = $event"
          @update:role-filter="roleFilter = $event"
          @update:create-dialog-visible="createDialogVisible = $event"
          @update:edit-dialog-visible="editDialogVisible = $event"
          @update:user-detail-visible="userDetailVisible = $event"
          @update:user-history-tab="userHistoryTab = $event"
          @update:create-form-ref="createFormRef = $event"
          @update:create-form="(field, value) => { (createForm as any)[field] = value }"
          @update:edit-form="(field, value) => { (editForm as any)[field] = value }"
        />
      </el-tab-pane>

      <!-- 审计日志 -->
      <el-tab-pane label="审计日志" name="audit" v-if="authStore.isSuperAdmin">
        <div class="tab-content">
          <el-table :data="auditLogs" stripe v-loading="loading" style="width: 100%">
            <el-table-column prop="id" label="ID" width="70" />
            <el-table-column prop="action" label="操作" min-width="120" />
            <el-table-column prop="entityType" label="实体类型" width="110" />
            <el-table-column prop="actorUsername" label="操作者" min-width="120" />
            <el-table-column prop="details" label="详情" min-width="200" show-overflow-tooltip />
            <el-table-column prop="ipAddress" label="IP地址" width="130" />
            <el-table-column prop="createdAt" label="时间" min-width="160">
              <template #default="{ row }">
                {{ formatDateTime(row.createdAt) }}
              </template>
            </el-table-column>
          </el-table>

          <el-pagination
            v-model:current-page="auditPagination.page"
            v-model:page-size="auditPagination.limit"
            :total="auditPagination.total"
            :page-sizes="[10, 20, 50, 100]"
            layout="total, sizes, prev, pager, next, jumper"
            @size-change="handleAuditSizeChange"
            @current-change="handleAuditPageChange"
            class="mt-3"
          />
        </div>
      </el-tab-pane>

      <!-- 模块管理 -->
      <el-tab-pane label="模块管理" name="modules" v-if="authStore.isSuperAdmin">
        <div class="tab-content">
          <el-row :gutter="20" class="mb-3">
            <el-col :span="18">
              <el-space>
                <el-button type="primary" @click="handleScanModules">
                  <el-icon><Search /></el-icon>
                  扫描模块
                </el-button>
                <el-button type="success" @click="handleUploadModule">
                  <el-icon><Upload /></el-icon>
                  上传模块
                </el-button>
                <el-button @click="loadModules">
                  <el-icon><Refresh /></el-icon>
                  刷新
                </el-button>
              </el-space>
            </el-col>
            <el-col :span="6" class="text-right">
              <el-tag>{{ modules.length }} 个模块</el-tag>
            </el-col>
          </el-row>

          <el-table :data="modules" stripe v-loading="loading" style="width: 100%">
            <el-table-column prop="name" label="模块名称" min-width="180" />
            <el-table-column prop="version" label="版本" width="90" />
            <el-table-column prop="description" label="描述" min-width="250" show-overflow-tooltip />
            <el-table-column label="类型" width="90">
              <template #default="{ row }">
                <el-tag :type="row.type === 'business' ? 'primary' : 'info'" size="small">
                  {{ row.type === 'business' ? '业务' : '功能' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="状态" width="90">
              <template #default="{ row }">
                <el-tag :type="row.enabled ? 'success' : 'warning'" size="small">
                  {{ row.enabled ? '已启用' : '已禁用' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="操作" width="280" fixed="right">
              <template #default="{ row }">
                <el-space>
                  <el-button
                    v-if="!row.enabled"
                    type="success"
                    size="small"
                    @click="handleToggleModule(row, true)"
                  >
                    启用
                  </el-button>
                  <el-button
                    v-else
                    type="warning"
                    size="small"
                    @click="handleToggleModule(row, false)"
                  >
                    禁用
                  </el-button>
                  <el-button
                    type="primary"
                    size="small"
                    @click="handleReloadModule(row)"
                  >
                    重载
                  </el-button>
                  <el-button
                    v-if="canUninstallModule(row.name)"
                    type="danger"
                    size="small"
                    @click="handleUninstallModule(row)"
                  >
                    卸载
                  </el-button>
                </el-space>
              </template>
            </el-table-column>
          </el-table>
        </div>
      </el-tab-pane>

      <!-- 公告管理 -->
      <el-tab-pane label="公告管理" name="announcements" v-if="authStore.isSuperAdmin">
        <div class="tab-content">
          <el-row :gutter="20" class="mb-3">
            <el-col :span="18">
              <el-button type="primary" @click="handleCreateAnnouncement">
                <el-icon><Plus /></el-icon> 创建公告
              </el-button>
            </el-col>
          </el-row>
          <el-table :data="announcements" stripe v-loading="loading" style="width: 100%">
            <el-table-column prop="title" label="标题" min-width="180" />
            <el-table-column prop="content" label="内容" min-width="250" show-overflow-tooltip />
            <el-table-column prop="type" label="类型" width="90">
              <template #default="{ row }">
                <el-tag :type="row.type === 'warning' ? 'warning' : row.type === 'maintenance' ? 'danger' : 'info'" size="small">
                  {{ ({ info: '通知', warning: '警告', maintenance: '维护' } as Record<string, string>)[row.type] || row.type }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="targetRole" label="目标角色" width="110">
              <template #default="{ row }">
                {{ ({ all: '全部', user: '用户', premium: '高级', admin: '管理员', superadmin: '超管' } as Record<string, string>)[row.targetRole] || row.targetRole }}
              </template>
            </el-table-column>
            <el-table-column label="状态" width="90">
              <template #default="{ row }">
                <el-tag :type="row.isActive ? 'success' : 'info'" size="small">
                  {{ row.isActive ? '启用' : '停用' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="createdAt" label="创建时间" min-width="150">
              <template #default="{ row }">
                {{ row.createdAt || '-' }}
              </template>
            </el-table-column>
            <el-table-column label="操作" width="260" fixed="right">
              <template #default="{ row }">
                <el-button size="small" @click="handleEditAnnouncement(row)">编辑</el-button>
                <el-button size="small" @click="handleToggleAnnouncement(row.id)">
                  {{ row.isActive ? '停用' : '启用' }}
                </el-button>
                <el-button size="small" type="danger" @click="handleDeleteAnnouncement(row.id)">删除</el-button>
              </template>
            </el-table-column>
          </el-table>
          <el-pagination
            v-model:current-page="announcementPagination.page"
            v-model:page-size="announcementPagination.limit"
            :total="announcementPagination.total"
            layout="total, prev, pager, next"
            @current-change="(p: number) => { announcementPagination.page = p; loadAnnouncements() }"
            class="mt-3"
          />
        </div>
      </el-tab-pane>

      <!-- 系统监控 -->
      <el-tab-pane label="系统监控" name="monitoring" v-if="authStore.isSuperAdmin">
        <AdminSystemMonitoring
          :monitoring-sub-tab="monitoringSubTab"
          :metrics-loading="metricsLoading"
          :system-metrics="systemMetrics"
          :service-health-list="serviceHealthList"
          :system-logs="systemLogs"
          :log-level-filter="logLevelFilter"
          :log-module-filter="logModuleFilter"
          :log-pagination="logPagination"
          :slow-queries="slowQueries"
          :bottlenecks="bottlenecks"
          :get-progress-color="getProgressColor"
          :format-bytes="formatBytes"
          :format-uptime="formatUptime"
          :get-log-level-tag-type="getLogLevelTagType"
          @update:monitoring-sub-tab="monitoringSubTab = $event"
          @update:log-level-filter="logLevelFilter = $event"
          @update:log-module-filter="logModuleFilter = $event"
          @load-system-metrics="loadSystemMetrics"
          @load-service-health="loadServiceHealth"
          @load-system-logs="loadSystemLogs"
          @load-performance-data="loadPerformanceData"
          @clean-logs="handleCleanLogs"
          @log-page-change="(p) => { logPagination.page = p; loadSystemLogs() }"
        />
      </el-tab-pane>

      <!-- 登录安全 -->
      <el-tab-pane label="登录安全" name="security" v-if="authStore.isSuperAdmin">
        <AdminLoginSecurity
          :security-sub-tab="securitySubTab"
          :security-loading="securityLoading"
          :login-history-username-filter="loginHistoryUsernameFilter"
          :all-login-history="allLoginHistory"
          :login-history-pagination="loginHistoryPagination"
          :suspicious-status-filter="suspiciousStatusFilter"
          :suspicious-logins="suspiciousLogins"
          :suspicious-pagination="suspiciousPagination"
          :ip-blacklist="ipBlacklist"
          :ip-blacklist-pagination="ipBlacklistPagination"
          :add-ip-blacklist-dialog-visible="addIpBlacklistDialogVisible"
          :add-ip-blacklist-form="addIpBlacklistForm"
          :account-lockouts="accountLockouts"
          :account-lockouts-pagination="accountLockoutsPagination"
          :lock-user-dialog-visible="lockUserDialogVisible"
          :lock-user-form="lockUserForm"
          :get-suspicious-status-label="getSuspiciousStatusLabel"
          :get-suspicious-status-tag-type="getSuspiciousStatusTagType"
          :get-threat-level-label="getThreatLevelLabel"
          :get-threat-level-tag-type="getThreatLevelTagType"
          @update:security-sub-tab="securitySubTab = $event"
          @update:login-history-username-filter="loginHistoryUsernameFilter = $event"
          @update:suspicious-status-filter="suspiciousStatusFilter = $event"
          @update:add-ip-blacklist-dialog-visible="addIpBlacklistDialogVisible = $event"
          @update:add-ip-blacklist-form="(field, value) => { (addIpBlacklistForm as any)[field] = value }"
          @update:lock-user-dialog-visible="lockUserDialogVisible = $event"
          @update:lock-user-form="(field, value) => { (lockUserForm as any)[field] = value }"
          @load-login-history="loadLoginHistory"
          @load-suspicious-logins="loadSuspiciousLogins"
          @load-ip-blacklist="loadIpBlacklist"
          @load-account-lockouts="loadAccountLockouts"
          @show-add-ip-blacklist-dialog="showAddIpBlacklistDialog"
          @add-ip-blacklist="handleAddIpBlacklist"
          @remove-ip-blacklist="handleRemoveIpBlacklist"
          @show-lock-user-dialog="showLockUserDialog"
          @lock-user-account-confirm="handleLockUserAccountConfirm"
          @unlock-user-account="handleUnlockUserAccount"
          @suspicious-login-action="handleSuspiciousLoginAction"
          @login-history-page-change="(p) => { loginHistoryPagination.page = p; loadLoginHistory() }"
          @suspicious-page-change="(p) => { suspiciousPagination.page = p; loadSuspiciousLogins() }"
          @ip-blacklist-page-change="(p) => { ipBlacklistPagination.page = p; loadIpBlacklist() }"
          @account-lockouts-page-change="(p) => { accountLockoutsPagination.page = p; loadAccountLockouts() }"
        />
      </el-tab-pane>

      <!-- Data management tabs (config, backup, permissions, notifications, cleanup, moderation, API keys) -->
      <AdminDataManagement
        :auth-store="authStore"
        :config-sub-tab="configSubTab"
        :config-loading="configLoading"
        :config-category-filter="configCategoryFilter"
        :configs="configs"
        :selected-config="selectedConfig"
        :config-history="configHistory"
        :config-history-key-filter="configHistoryKeyFilter"
        :config-history-pagination="configHistoryPagination"
        :edit-config-dialog-visible="editConfigDialogVisible"
        :edit-config-form="editConfigForm"
        :get-config-change-type-tag="getConfigChangeTypeTag"
        :backup-sub-tab="backupSubTab"
        :backup-loading="backupLoading"
        :backup-jobs="backupJobs"
        :backup-records="backupRecords"
        :backup-records-pagination="backupRecordsPagination"
        :backup-records-job-filter="backupRecordsJobFilter"
        :create-backup-job-dialog-visible="createBackupJobDialogVisible"
        :create-backup-job-form="createBackupJobForm"
        :get-backup-job-type-label="getBackupJobTypeLabel"
        :get-backup-status-tag-type="getBackupStatusTagType"
        :format-file-size="formatFileSize"
        :permissions-sub-tab="permissionsSubTab"
        :permissions-loading="permissionsLoading"
        :permission-matrix="permissionMatrix"
        :roles="roles"
        :roles-loading="rolesLoading"
        :user-roles-loading="userRolesLoading"
        :user-role-search-user-id="userRoleSearchUserId"
        :user-roles="userRoles"
        :notifications-sub-tab="notificationsSubTab"
        :notifications-loading="notificationsLoading"
        :system-notifications="systemNotifications"
        :notifications-pagination="notificationsPagination"
        :notification-templates-loading="notificationTemplatesLoading"
        :notification-templates="notificationTemplates"
        :get-notification-status-tag-type="getNotificationStatusTagType"
        :get-notification-status-label="getNotificationStatusLabel"
        :cleanup-sub-tab="cleanupSubTab"
        :cleanup-tasks-loading="cleanupTasksLoading"
        :cleanup-tasks="cleanupTasks"
        :cleanup-history-loading="cleanupHistoryLoading"
        :cleanup-history="cleanupHistory"
        :cleanup-history-pagination="cleanupHistoryPagination"
        :storage-stats-loading="storageStatsLoading"
        :storage-stats="storageStats"
        :get-cleanup-task-type-label="getCleanupTaskTypeLabel"
        :get-cleanup-status-tag-type="getCleanupStatusTagType"
        :get-cleanup-status-label="getCleanupStatusLabel"
        :moderation-sub-tab="moderationSubTab"
        :pending-papers-loading="pendingPapersLoading"
        :pending-papers="pendingPapers"
        :pending-papers-pagination="pendingPapersPagination"
        :user-reports-loading="userReportsLoading"
        :user-reports="userReports"
        :user-reports-pagination="userReportsPagination"
        :user-reports-status-filter="userReportsStatusFilter"
        :sensitive-words-loading="sensitiveWordsLoading"
        :sensitive-words="sensitiveWords"
        :get-moderation-status-tag-type="getModerationStatusTagType"
        :get-moderation-status-label="getModerationStatusLabel"
        :get-report-target-type-label="getReportTargetTypeLabel"
        :get-report-priority-tag-type="getReportPriorityTagType"
        :get-report-priority-label="getReportPriorityLabel"
        :get-sensitive-word-category-label="getSensitiveWordCategoryLabel"
        :get-sensitive-word-severity-tag-type="getSensitiveWordSeverityTagType"
        :get-sensitive-word-severity-label="getSensitiveWordSeverityLabel"
        :api-keys-sub-tab="apiKeysSubTab"
        :api-keys-loading="apiKeysLoading"
        :api-keys="apiKeys"
        :api-keys-pagination="apiKeysPagination"
        :api-key-usage-loading="apiKeyUsageLoading"
        :api-key-usage="apiKeyUsage"
        :api-key-stats="apiKeyStats"
        @update:config-sub-tab="configSubTab = $event"
        @update:config-category-filter="configCategoryFilter = $event"
        @update:config-history-key-filter="configHistoryKeyFilter = $event"
        @update:edit-config-dialog-visible="editConfigDialogVisible = $event"
        @update:edit-config-form="(field, value) => { (editConfigForm as any)[field] = value }"
        @load-configs="loadConfigs"
        @load-config-history="loadConfigHistory"
        @config-selection-change="handleConfigSelectionChange"
        @show-edit-config-dialog="showEditConfigDialog"
        @update-config="handleUpdateConfig"
        @config-history-page-change="(p) => { configHistoryPagination.page = p; loadConfigHistory() }"
        @update:backup-sub-tab="backupSubTab = $event"
        @update:backup-records-job-filter="backupRecordsJobFilter = $event"
        @update:create-backup-job-dialog-visible="createBackupJobDialogVisible = $event"
        @update:create-backup-job-form="(field, value) => { (createBackupJobForm as any)[field] = value }"
        @load-backup-jobs="loadBackupJobs"
        @load-backup-records="loadBackupRecords"
        @show-create-backup-job-dialog="showCreateBackupJobDialog"
        @create-backup-job="handleCreateBackupJob"
        @trigger-backup-job="triggerBackupJob"
        @delete-backup-job="handleDeleteBackupJob"
        @delete-backup-file="handleDeleteBackupFile"
        @download-backup="downloadBackup"
        @backup-records-page-change="(p) => { backupRecordsPagination.page = p; loadBackupRecords() }"
        @update:permissions-sub-tab="permissionsSubTab = $event"
        @update:user-role-search-user-id="userRoleSearchUserId = $event"
        @load-permission-matrix="loadPermissionMatrix"
        @load-user-roles="loadUserRoles"
        @show-create-role-dialog="showCreateRoleDialog"
        @show-assign-role-dialog="showAssignRoleDialog"
        @edit-role="editRole"
        @delete-role="handleDeleteRole"
        @remove-user-role="handleRemoveUserRole"
        @update:notifications-sub-tab="notificationsSubTab = $event"
        @load-system-notifications="loadSystemNotifications"
        @show-send-notification-dialog="showSendNotificationDialog"
        @notifications-page-change="(p) => { notificationsPagination.page = p; loadSystemNotifications() }"
        @update:cleanup-sub-tab="cleanupSubTab = $event"
        @load-cleanup-tasks="loadCleanupTasks"
        @show-create-cleanup-task-dialog="showCreateCleanupTaskDialog"
        @trigger-cleanup-task="triggerCleanupTask"
        @delete-cleanup-task="handleDeleteCleanupTask"
        @toggle-cleanup-task="toggleCleanupTask"
        @cleanup-history-page-change="(p) => { cleanupHistoryPagination.page = p; loadCleanupHistory() }"
        @update:moderation-sub-tab="moderationSubTab = $event"
        @update:user-reports-status-filter="userReportsStatusFilter = $event"
        @load-user-reports="loadUserReports"
        @show-create-sensitive-word-dialog="showCreateSensitiveWordDialog"
        @approve-paper="handleApprovePaper"
        @show-reject-paper-dialog="showRejectPaperDialog"
        @show-resolve-report-dialog="showResolveReportDialog"
        @delete-sensitive-word="handleDeleteSensitiveWord"
        @pending-papers-page-change="(p) => { pendingPapersPagination.page = p; loadPendingPapers() }"
        @user-reports-page-change="(p) => { userReportsPagination.page = p; loadUserReports() }"
        @update:api-keys-sub-tab="apiKeysSubTab = $event"
        @show-create-api-key-dialog="showCreateApiKeyDialog"
        @show-api-key-usage="showApiKeyUsage"
        @regenerate-api-key="handleRegenerateApiKey"
        @delete-api-key="handleDeleteApiKey"
        @toggle-api-key="toggleApiKey"
        @api-keys-page-change="(p) => { apiKeysPagination.page = p; loadApiKeys() }"
      />

      <!-- 上传模块对话框 -->
      <el-dialog v-model="uploadDialogVisible" title="上传模块" width="500px">
        <el-form :model="uploadForm" label-width="100px">
          <el-form-item label="模块文件">
            <el-upload
              ref="uploadRef"
              :auto-upload="false"
              :on-change="handleFileChange"
              :limit="1"
              accept=".dll,.so,.dylib"
            >
              <el-button type="primary">选择文件</el-button>
              <template #tip>
                <div class="el-upload__tip">只能上传 .dll/.so/.dylib 文件</div>
              </template>
            </el-upload>
          </el-form-item>
          <el-form-item label="模块名称" v-if="uploadForm.file">
            <el-input v-model="uploadForm.moduleName" placeholder="例如: CustomApiModule" />
          </el-form-item>
        </el-form>
        <template #footer>
          <el-button @click="uploadDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="handleConfirmUpload" :loading="uploading">上传并安装</el-button>
        </template>
      </el-dialog>

      <!-- 公告对话框 -->
      <el-dialog v-model="announcementDialogVisible" :title="isEditingAnnouncement ? '编辑公告' : '创建公告'" width="550px">
        <el-form :model="announcementForm" label-width="100px">
          <el-form-item label="标题">
            <el-input v-model="announcementForm.title" placeholder="公告标题" />
          </el-form-item>
          <el-form-item label="内容">
            <el-input v-model="announcementForm.content" type="textarea" :rows="4" placeholder="公告内容" />
          </el-form-item>
          <el-form-item label="类型">
            <el-select v-model="announcementForm.type">
              <el-option label="通知" value="info" />
              <el-option label="警告" value="warning" />
              <el-option label="维护" value="maintenance" />
            </el-select>
          </el-form-item>
          <el-form-item label="目标角色">
            <el-select v-model="announcementForm.targetRole">
              <el-option label="全部" value="all" />
              <el-option label="普通用户" value="user" />
              <el-option label="高级用户" value="premium" />
              <el-option label="管理员" value="admin" />
              <el-option label="超级管理员" value="superadmin" />
            </el-select>
          </el-form-item>
          <el-form-item label="过期时间">
            <el-input v-model="announcementForm.expiresAt" placeholder="YYYY-MM-DD 或留空" />
          </el-form-item>
        </el-form>
        <template #footer>
          <el-button @click="announcementDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="handleSaveAnnouncement">保存</el-button>
        </template>
      </el-dialog>
    </el-tabs>
  </div>
</template>

<script setup lang="ts">
import {
  Setting,
  User,
  Search,
  Refresh,
  CircleCheck,
  Star,
  Memo,
  Upload,
  Plus
} from '@element-plus/icons-vue'
import { Line as LineChart } from 'vue-chartjs'
import { useAdminDashboard } from './useAdminDashboard'
import AdminUserManagement from './AdminUserManagement.vue'
import AdminSystemMonitoring from './AdminSystemMonitoring.vue'
import AdminLoginSecurity from './AdminLoginSecurity.vue'
import AdminDataManagement from './AdminDataManagement.vue'

const {
  authStore,
  loading,
  activeTab,
  searchQuery,
  roleFilter,
  dashboardData,
  dashboardLoading,
  stats,
  users,
  pagination,
  modules,
  uploadDialogVisible,
  uploadRef,
  uploading,
  uploadForm,
  auditLogs,
  auditPagination,
  createDialogVisible,
  createFormRef,
  createForm,
  createRules,
  editDialogVisible,
  editForm,
  saving,
  userDetailVisible,
  detailUser,
  userHistoryTab,
  loginHistory,
  userSessions,
  announcements,
  announcementPagination,
  announcementDialogVisible,
  announcementForm,
  isEditingAnnouncement,
  monitoringSubTab,
  metricsLoading,
  systemMetrics,
  serviceHealthList,
  systemLogs,
  logLevelFilter,
  logModuleFilter,
  logPagination,
  slowQueries,
  bottlenecks,
  securitySubTab,
  securityLoading,
  loginHistoryUsernameFilter,
  allLoginHistory,
  loginHistoryPagination,
  suspiciousStatusFilter,
  suspiciousLogins,
  suspiciousPagination,
  ipBlacklist,
  ipBlacklistPagination,
  addIpBlacklistDialogVisible,
  addIpBlacklistForm,
  accountLockouts,
  accountLockoutsPagination,
  lockUserDialogVisible,
  lockUserForm,
  configSubTab,
  configLoading,
  configCategoryFilter,
  configs,
  selectedConfig,
  configHistory,
  configHistoryKeyFilter,
  configHistoryPagination,
  editConfigDialogVisible,
  editConfigForm,
  backupSubTab,
  backupLoading,
  backupJobs,
  backupRecords,
  backupRecordsPagination,
  backupRecordsJobFilter,
  createBackupJobDialogVisible,
  createBackupJobForm,
  permissionsSubTab,
  permissionsLoading,
  permissionMatrix,
  roles,
  rolesLoading,
  userRolesLoading,
  userRoleSearchUserId,
  userRoles,
  notificationsSubTab,
  notificationsLoading,
  systemNotifications,
  notificationsPagination,
  notificationTemplatesLoading,
  notificationTemplates,
  cleanupSubTab,
  cleanupTasksLoading,
  cleanupTasks,
  cleanupHistoryLoading,
  cleanupHistory,
  cleanupHistoryPagination,
  storageStatsLoading,
  storageStats,
  moderationSubTab,
  pendingPapersLoading,
  pendingPapers,
  pendingPapersPagination,
  userReportsLoading,
  userReports,
  userReportsPagination,
  userReportsStatusFilter,
  sensitiveWordsLoading,
  sensitiveWords,
  apiKeysSubTab,
  apiKeysLoading,
  apiKeys,
  apiKeysPagination,
  apiKeyUsageLoading,
  apiKeyUsage,
  apiKeyStats,
  canChangeRole,
  userTrendChartData,
  chartOptions,
  loadDashboard,
  loadUsers,
  loadModules,
  loadAuditLogs,
  handleSearch,
  handlePageChange,
  handleSizeChange,
  handleAuditPageChange,
  handleAuditSizeChange,
  handleEditUser,
  handleCreateUser,
  handleConfirmCreate,
  handleSaveUser,
  handleToggleUserStatus,
  openUserDetail,
  loadUserHistory,
  loadUserSessions,
  handleKickSession,
  loadAnnouncements,
  handleCreateAnnouncement,
  handleEditAnnouncement,
  handleSaveAnnouncement,
  handleDeleteAnnouncement,
  handleToggleAnnouncement,
  handleExportUsers,
  getRoleLabel,
  getRoleBadgeType,
  canManageRole,
  getActivityTagType,
  getActivityLabel,
  formatDateTime,
  canUninstallModule,
  handleToggleModule,
  handleReloadModule,
  handleUninstallModule,
  handleScanModules,
  handleUploadModule,
  handleFileChange,
  handleConfirmUpload,
  loadSystemMetrics,
  loadServiceHealth,
  loadSystemLogs,
  handleCleanLogs,
  loadPerformanceData,
  getProgressColor,
  formatBytes,
  formatUptime,
  getLogLevelTagType,
  loadLoginHistory,
  loadSuspiciousLogins,
  loadIpBlacklist,
  showAddIpBlacklistDialog,
  handleAddIpBlacklist,
  handleRemoveIpBlacklist,
  loadAccountLockouts,
  showLockUserDialog,
  handleLockUserAccountConfirm,
  handleUnlockUserAccount,
  handleSuspiciousLoginAction,
  getSuspiciousStatusLabel,
  getSuspiciousStatusTagType,
  getThreatLevelLabel,
  getThreatLevelTagType,
  loadConfigs,
  handleConfigSelectionChange,
  showEditConfigDialog,
  handleUpdateConfig,
  loadConfigHistory,
  getConfigChangeTypeTag,
  loadBackupJobs,
  loadBackupRecords,
  showCreateBackupJobDialog,
  handleCreateBackupJob,
  triggerBackupJob,
  handleDeleteBackupJob,
  handleDeleteBackupFile,
  downloadBackup,
  getBackupJobTypeLabel,
  getBackupStatusTagType,
  formatFileSize,
  loadPermissionMatrix,
  loadRoles,
  handleDeleteRole,
  loadUserRoles,
  handleRemoveUserRole,
  loadSystemNotifications,
  loadNotificationTemplates,
  getNotificationStatusTagType,
  getNotificationStatusLabel,
  loadCleanupTasks,
  loadCleanupHistory,
  loadStorageStats,
  triggerCleanupTask,
  handleDeleteCleanupTask,
  getCleanupTaskTypeLabel,
  getCleanupStatusTagType,
  getCleanupStatusLabel,
  loadPendingPapers,
  handleApprovePaper,
  showRejectPaperDialog,
  loadUserReports,
  showResolveReportDialog,
  loadSensitiveWords,
  handleDeleteSensitiveWord,
  getModerationStatusTagType,
  getModerationStatusLabel,
  getReportTargetTypeLabel,
  getReportPriorityTagType,
  getReportPriorityLabel,
  getSensitiveWordCategoryLabel,
  getSensitiveWordSeverityTagType,
  getSensitiveWordSeverityLabel,
  loadApiKeys,
  loadApiKeyUsage,
  handleDeleteApiKey,
  handleRegenerateApiKey,
  showApiKeyUsage,
  toggleApiKey,
  toggleCleanupTask,
  showCreateRoleDialog,
  showAssignRoleDialog,
  editRole,
  showSendNotificationDialog,
  showCreateCleanupTaskDialog,
  showCreateSensitiveWordDialog,
  showCreateApiKeyDialog
} = useAdminDashboard()
</script>

<style scoped>
.admin-dashboard {
  padding: 16px;
  width: 100%;
  max-width: 100%;
  box-sizing: border-box;
}

.page-title {
  display: flex;
  align-items: center;
  gap: 10px;
  margin: 0;
  font-size: 24px;
  font-weight: 600;
}

.text-right {
  text-align: right;
}

.stat-card {
  margin-bottom: 20px;
}

.stat-content {
  display: flex;
  align-items: center;
  gap: 15px;
}

.stat-icon {
  width: 50px;
  height: 50px;
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: white;
  font-size: 24px;
}

.stat-info {
  flex: 1;
}

.stat-value {
  font-size: 24px;
  font-weight: 600;
  color: #303133;
}

.stat-label {
  font-size: 14px;
  color: #909399;
  margin-top: 4px;
}

.admin-tabs {
  background: white;
  border-radius: 8px;
  padding: 20px;
  width: 100%;
}

.tab-content {
  min-height: 400px;
  width: 100%;
  overflow-x: auto;
  overflow-y: auto;
}

/* Table container with scroll */
.tab-content :deep(.el-table) {
  font-size: 14px;
  width: 100%;
}

.tab-content :deep(.el-table th) {
  font-weight: 600;
}

.tab-content :deep(.el-table .el-table__cell) {
  padding: 12px 0;
}

/* Ensure tables can scroll horizontally when needed */
.tab-content :deep(.el-table__body-wrapper) {
  overflow-x: auto;
}

/* Fix scrollbar for element plus tables */
.tab-content :deep(.el-scrollbar__wrap) {
  overflow-x: auto;
}

/* Card content overflow */
.tab-content :deep(.el-card__body) {
  overflow-x: auto;
}

/* Better drawer content */
:deep(.el-drawer__body) {
  padding: 20px;
}

/* Chart container */
.chart-container {
  width: 100%;
  height: 300px;
}

/* Responsive cards */
.stat-card {
  width: 100%;
}

/* Ensure all rows are full width */
.admin-tabs :deep(.el-row) {
  width: 100%;
}

.mb-3 {
  margin-bottom: 15px;
}

.uptime-display {
  text-align: center;
  padding: 10px;
}

.uptime-label {
  font-size: 14px;
  color: #909399;
  margin-bottom: 8px;
}

.uptime-value {
  font-size: 24px;
  font-weight: 600;
  color: #303133;
}

.mb-4 {
  margin-bottom: 20px;
}

.mt-3 {
  margin-top: 15px;
}
</style>
