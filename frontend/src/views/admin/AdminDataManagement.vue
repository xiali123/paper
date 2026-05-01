<template>
  <div>
    <!-- 全局配置 -->
    <el-tab-pane label="全局配置" name="config" v-if="authStore.isSuperAdmin">
      <div class="tab-content">
        <el-tabs :model-value="configSubTab" @update:model-value="(val: string) => $emit('update:configSubTab', val)" class="config-sub-tabs">
          <!-- 配置管理 -->
          <el-tab-pane label="配置管理" name="management">
            <el-row :gutter="20" class="mb-3">
              <el-col :span="12">
                <el-select :model-value="configCategoryFilter" placeholder="选择分类" clearable @change="(val: string) => { $emit('update:configCategoryFilter', val); $emit('load-configs') }">
                  <el-option label="全部" value="" />
                  <el-option label="认证" value="auth" />
                  <el-option label="存储" value="storage" />
                  <el-option label="限额" value="limits" />
                  <el-option label="邮件" value="email" />
                  <el-option label="维护" value="maintenance" />
                  <el-option label="系统" value="system" />
                </el-select>
              </el-col>
              <el-col :span="12">
                <el-button @click="$emit('load-configs')" :loading="configLoading">
                  <el-icon><Refresh /></el-icon> 刷新
                </el-button>
                <el-button type="primary" @click="$emit('show-edit-config-dialog')" :disabled="selectedConfig === null">
                  <el-icon><Edit /></el-icon> 编辑选中
                </el-button>
              </el-col>
            </el-row>
            <el-table :data="configs" stripe v-loading="configLoading" style="width: 100%"
                      @selection-change="(sel: any[]) => $emit('config-selection-change', sel)" highlight-current-row>
              <el-table-column type="selection" width="55" />
              <el-table-column prop="key" label="配置键" width="200" />
              <el-table-column prop="value" label="当前值" min-width="200" show-overflow-tooltip />
              <el-table-column prop="value_type" label="类型" width="80" />
              <el-table-column prop="category" label="分类" width="100" />
              <el-table-column prop="description" label="描述" min-width="250" show-overflow-tooltip />
              <el-table-column prop="updated_at" label="更新时间" width="160" />
            </el-table>
          </el-tab-pane>

          <!-- 配置历史 -->
          <el-tab-pane label="配置历史" name="history">
            <el-row :gutter="20" class="mb-3">
              <el-col :span="18">
                <el-input
                  :model-value="configHistoryKeyFilter"
                  placeholder="按配置键过滤"
                  clearable
                  @change="$emit('load-config-history')"
                  @update:model-value="(val: string) => $emit('update:configHistoryKeyFilter', val)"
                  style="width: 300px"
                >
                  <template #prefix>
                    <el-icon><Search /></el-icon>
                  </template>
                </el-input>
              </el-col>
              <el-col :span="6">
                <el-button @click="$emit('load-config-history')" :loading="configLoading">
                  <el-icon><Refresh /></el-icon> 刷新
                </el-button>
              </el-col>
            </el-row>
            <el-table :data="configHistory" stripe v-loading="configLoading" style="width: 100%">
              <el-table-column prop="config_key" label="配置键" width="180" />
              <el-table-column prop="old_value" label="旧值" min-width="150" show-overflow-tooltip />
              <el-table-column prop="new_value" label="新值" min-width="150" show-overflow-tooltip />
              <el-table-column prop="change_type" label="操作类型" width="90">
                <template #default="{ row }">
                  <el-tag :type="getConfigChangeTypeTag(row.change_type)" size="small">
                    {{ row.change_type === 'create' ? '创建' : row.change_type === 'update' ? '更新' : '删除' }}
                  </el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="changed_by" label="操作者" width="120" />
              <el-table-column prop="change_reason" label="原因" min-width="200" show-overflow-tooltip />
              <el-table-column prop="created_at" label="时间" width="160" />
            </el-table>
            <el-pagination
              :current-page="configHistoryPagination.page"
              :page-size="configHistoryPagination.limit"
              :total="configHistoryPagination.total"
              layout="total, prev, pager, next"
              @current-change="(p: number) => $emit('config-history-page-change', p)"
              class="mt-3"
            />
          </el-tab-pane>
        </el-tabs>
      </div>
    </el-tab-pane>

    <!-- 数据备份 -->
    <el-tab-pane label="数据备份" name="backup" v-if="authStore.isSuperAdmin">
      <div class="tab-content">
        <el-tabs :model-value="backupSubTab" @update:model-value="(val: string) => $emit('update:backupSubTab', val)" class="backup-sub-tabs">
          <!-- 备份任务 -->
          <el-tab-pane label="备份任务" name="jobs">
            <el-row :gutter="20" class="mb-3">
              <el-col :span="18">
                <el-button type="primary" @click="$emit('show-create-backup-job-dialog')">
                  <el-icon><Plus /></el-icon> 创建备份任务
                </el-button>
              </el-col>
              <el-col :span="6">
                <el-button @click="$emit('load-backup-jobs')" :loading="backupLoading">
                  <el-icon><Refresh /></el-icon> 刷新
                </el-button>
              </el-col>
            </el-row>
            <el-table :data="backupJobs" stripe v-loading="backupLoading" style="width: 100%">
              <el-table-column prop="name" label="任务名称" width="180" />
              <el-table-column prop="job_type" label="类型" width="100">
                <template #default="{ row }">
                  <el-tag size="small">{{ getBackupJobTypeLabel(row.job_type) }}</el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="schedule_cron" label="计划" width="120" />
              <el-table-column prop="retention_days" label="保留天数" width="90" align="center" />
              <el-table-column label="状态" width="80">
                <template #default="{ row }">
                  <el-tag :type="row.is_enabled ? 'success' : 'info'" size="small">
                    {{ row.is_enabled ? '启用' : '停用' }}
                  </el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="last_run_at" label="上次运行" width="160" />
              <el-table-column prop="last_run_status" label="上次状态" width="90">
                <template #default="{ row }">
                  <el-tag v-if="row.last_run_status" :type="getBackupStatusTagType(row.last_run_status)" size="small">
                    {{ row.last_run_status === 'success' ? '成功' : row.last_run_status === 'failed' ? '失败' : '运行中' }}
                  </el-tag>
                  <span v-else>-</span>
                </template>
              </el-table-column>
              <el-table-column label="操作" width="220" fixed="right">
                <template #default="{ row }">
                  <el-button size="small" @click="$emit('trigger-backup-job', row.id)">立即执行</el-button>
                  <el-button size="small" type="danger" @click="$emit('delete-backup-job', row.id)">删除</el-button>
                </template>
              </el-table-column>
            </el-table>
          </el-tab-pane>

          <!-- 备份记录 -->
          <el-tab-pane label="备份记录" name="records">
            <el-row :gutter="20" class="mb-3">
              <el-col :span="18">
                <el-select :model-value="backupRecordsJobFilter" placeholder="选择任务" clearable @change="(val: number) => { $emit('update:backupRecordsJobFilter', val); $emit('load-backup-records') }">
                  <el-option label="全部" :value="0" />
                  <el-option v-for="job in backupJobs" :key="job.id" :label="job.name" :value="job.id" />
                </el-select>
              </el-col>
              <el-col :span="6">
                <el-button @click="$emit('load-backup-records')" :loading="backupLoading">
                  <el-icon><Refresh /></el-icon> 刷新
                </el-button>
              </el-col>
            </el-row>
            <el-table :data="backupRecords" stripe v-loading="backupLoading" style="width: 100%">
              <el-table-column prop="job_name" label="任务" width="150" />
              <el-table-column prop="filename" label="文件名" min-width="180" />
              <el-table-column prop="backup_type" label="类型" width="80">
                <template #default="{ row }">
                  <el-tag size="small">{{ getBackupJobTypeLabel(row.backup_type) }}</el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="file_size" label="大小" width="100">
                <template #default="{ row }">
                  {{ formatFileSize(row.file_size) }}
                </template>
              </el-table-column>
              <el-table-column prop="status" label="状态" width="90">
                <template #default="{ row }">
                  <el-tag :type="getBackupStatusTagType(row.status)" size="small">
                    {{ row.status === 'success' ? '成功' : row.status === 'failed' ? '失败' : row.status === 'in_progress' ? '进行中' : row.status }}
                  </el-tag>
                </template>
              </el-table-column>
              <el-table-column prop="started_at" label="开始时间" width="160" />
              <el-table-column prop="completed_at" label="完成时间" width="160" />
              <el-table-column prop="duration_seconds" label="耗时(秒)" width="90" align="center" />
              <el-table-column label="操作" width="100" fixed="right">
                <template #default="{ row }">
                  <el-button v-if="row.status === 'success'" size="small" type="primary" @click="$emit('download-backup', row.id)">下载</el-button>
                  <el-button size="small" type="danger" @click="$emit('delete-backup-file', row.id)" :disabled="row.status !== 'success'">删除</el-button>
                </template>
              </el-table-column>
            </el-table>
            <el-pagination
              :current-page="backupRecordsPagination.page"
              :page-size="backupRecordsPagination.limit"
              :total="backupRecordsPagination.total"
              layout="total, prev, pager, next"
              @current-change="(p: number) => $emit('backup-records-page-change', p)"
              class="mt-3"
            />
          </el-tab-pane>
        </el-tabs>
      </div>
    </el-tab-pane>

    <!-- 权限管理 -->
    <el-tab-pane label="权限管理" name="permissions" v-if="authStore.isSuperAdmin">
      <div class="tab-content">
        <el-tabs :model-value="permissionsSubTab" @update:model-value="(val: string) => $emit('update:permissionsSubTab', val)" class="sub-tabs">
          <!-- 权限矩阵 -->
          <el-tab-pane label="权限矩阵" name="matrix">
            <div v-loading="permissionsLoading">
              <el-button type="primary" @click="$emit('load-permission-matrix')" class="mb-3">刷新</el-button>
              <el-table :data="permissionMatrix" stripe>
                <el-table-column prop="roleName" label="角色" width="120" />
                <el-table-column prop="totalPermissions" label="总权限数" width="100" align="center" />
                <el-table-column label="按资源分组" min-width="300">
                  <template #default="{ row }">
                    <el-tag v-for="(count, resource) in row.permissionsByResource" :key="resource" class="mr-1" size="small">
                      {{ resource }}: {{ count }}
                    </el-tag>
                  </template>
                </el-table-column>
              </el-table>
            </div>
          </el-tab-pane>
          <!-- 角色管理 -->
          <el-tab-pane label="角色管理" name="roles">
            <div v-loading="rolesLoading">
              <el-button type="primary" @click="$emit('show-create-role-dialog')" class="mb-3">创建角色</el-button>
              <el-table :data="roles" stripe>
                <el-table-column prop="displayName" label="显示名称" width="150" />
                <el-table-column prop="name" label="标识符" width="100" />
                <el-table-column prop="description" label="描述" min-width="200" />
                <el-table-column prop="level" label="级别" width="80" align="center" />
                <el-table-column prop="isSystem" label="系统角色" width="90" align="center">
                  <template #default="{ row }">
                    <el-tag :type="row.isSystem ? 'danger' : 'success'" size="small">
                      {{ row.isSystem ? '是' : '否' }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column label="操作" width="150">
                  <template #default="{ row }">
                    <el-button size="small" @click="$emit('edit-role', row)">编辑</el-button>
                    <el-button size="small" type="danger" @click="$emit('delete-role', row)" :disabled="row.isSystem">删除</el-button>
                  </template>
                </el-table-column>
              </el-table>
            </div>
          </el-tab-pane>
          <!-- 用户角色 -->
          <el-tab-pane label="用户角色" name="user-roles">
            <div v-loading="userRolesLoading">
              <el-input :model-value="userRoleSearchUserId" @update:model-value="(val: string) => $emit('update:userRoleSearchUserId', val)" placeholder="输入用户ID" style="width: 200px" class="mr-2 mb-3" />
              <el-button type="primary" @click="$emit('load-user-roles')">查询</el-button>
              <el-button type="primary" @click="$emit('show-assign-role-dialog')" :disabled="!userRoleSearchUserId">分配角色</el-button>
              <el-table :data="userRoles" stripe class="mt-3">
                <el-table-column prop="userId" label="用户ID" width="80" />
                <el-table-column prop="username" label="用户名" width="120" />
                <el-table-column prop="roleName" label="角色" width="100" />
                <el-table-column prop="roleLevel" label="级别" width="80" align="center" />
                <el-table-column prop="assignedAt" label="分配时间" width="160" />
                <el-table-column prop="expiresAt" label="过期时间" width="160" />
                <el-table-column label="操作" width="100">
                  <template #default="{ row }">
                    <el-button size="small" type="danger" @click="$emit('remove-user-role', row)">移除</el-button>
                  </template>
                </el-table-column>
              </el-table>
            </div>
          </el-tab-pane>
        </el-tabs>
      </div>
    </el-tab-pane>

    <!-- 通知管理 -->
    <el-tab-pane label="通知管理" name="notifications" v-if="authStore.isSuperAdmin">
      <div class="tab-content">
        <el-tabs :model-value="notificationsSubTab" @update:model-value="(val: string) => $emit('update:notificationsSubTab', val)" class="sub-tabs">
          <!-- 系统通知 -->
          <el-tab-pane label="系统通知" name="system">
            <div v-loading="notificationsLoading">
              <el-button type="primary" @click="$emit('show-send-notification-dialog')" class="mb-3">发送通知</el-button>
              <el-table :data="systemNotifications" stripe>
                <el-table-column prop="title" label="标题" width="200" />
                <el-table-column prop="channel" label="渠道" width="80">
                  <template #default="{ row }">
                    <el-tag size="small">{{ row.channel }}</el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="targetRole" label="目标角色" width="100" />
                <el-table-column prop="totalRecipients" label="目标数" width="80" align="center" />
                <el-table-column prop="sentCount" label="已发送" width="80" align="center" />
                <el-table-column prop="status" label="状态" width="90">
                  <template #default="{ row }">
                    <el-tag :type="getNotificationStatusTagType(row.status)" size="small">
                      {{ getNotificationStatusLabel(row.status) }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="createdAt" label="创建时间" width="160" />
              </el-table>
              <el-pagination
                :current-page="notificationsPagination.page"
                :page-size="notificationsPagination.limit"
                :total="notificationsPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => $emit('notifications-page-change', p)"
                class="mt-3"
              />
            </div>
          </el-tab-pane>
          <!-- 通知模板 -->
          <el-tab-pane label="通知模板" name="templates">
            <div v-loading="notificationTemplatesLoading">
              <el-table :data="notificationTemplates" stripe>
                <el-table-column prop="name" label="名称" width="150" />
                <el-table-column prop="titleTemplate" label="标题模板" min-width="200" />
                <el-table-column prop="channel" label="渠道" width="80">
                  <template #default="{ row }">
                    <el-tag size="small">{{ row.channel }}</el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="language" label="语言" width="80" />
                <el-table-column prop="isActive" label="状态" width="80" align="center">
                  <template #default="{ row }">
                    <el-tag :type="row.isActive ? 'success' : 'info'" size="small">
                      {{ row.isActive ? '启用' : '禁用' }}
                    </el-tag>
                  </template>
                </el-table-column>
              </el-table>
            </div>
          </el-tab-pane>
        </el-tabs>
      </div>
    </el-tab-pane>

    <!-- 数据清理 -->
    <el-tab-pane label="数据清理" name="cleanup" v-if="authStore.isSuperAdmin">
      <div class="tab-content">
        <el-tabs :model-value="cleanupSubTab" @update:model-value="(val: string) => $emit('update:cleanupSubTab', val)" class="sub-tabs">
          <!-- 清理任务 -->
          <el-tab-pane label="清理任务" name="tasks">
            <div v-loading="cleanupTasksLoading">
              <el-button type="primary" @click="$emit('show-create-cleanup-task-dialog')" class="mb-3">创建任务</el-button>
              <el-table :data="cleanupTasks" stripe>
                <el-table-column prop="displayName" label="任务名称" width="150" />
                <el-table-column prop="taskType" label="类型" width="100">
                  <template #default="{ row }">
                    <el-tag size="small">{{ getCleanupTaskTypeLabel(row.taskType) }}</el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="description" label="描述" min-width="200" />
                <el-table-column prop="scheduleCron" label="计划" width="100" />
                <el-table-column prop="isEnabled" label="状态" width="80" align="center">
                  <template #default="{ row }">
                    <el-switch :model-value="row.isEnabled" @change="$emit('toggle-cleanup-task', row)" />
                  </template>
                </el-table-column>
                <el-table-column prop="isSystem" label="系统任务" width="90" align="center">
                  <template #default="{ row }">
                    <el-tag :type="row.isSystem ? 'danger' : 'success'" size="small">
                      {{ row.isSystem ? '是' : '否' }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column label="操作" width="150">
                  <template #default="{ row }">
                    <el-button size="small" @click="$emit('trigger-cleanup-task', row)">执行</el-button>
                    <el-button size="small" type="danger" @click="$emit('delete-cleanup-task', row)" :disabled="row.isSystem">删除</el-button>
                  </template>
                </el-table-column>
              </el-table>
            </div>
          </el-tab-pane>
          <!-- 执行历史 -->
          <el-tab-pane label="执行历史" name="history">
            <div v-loading="cleanupHistoryLoading">
              <el-table :data="cleanupHistory" stripe>
                <el-table-column prop="taskName" label="任务" width="150" />
                <el-table-column prop="status" label="状态" width="90">
                  <template #default="{ row }">
                    <el-tag :type="getCleanupStatusTagType(row.status)" size="small">
                      {{ getCleanupStatusLabel(row.status) }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="startedAt" label="开始时间" width="160" />
                <el-table-column prop="durationSeconds" label="耗时(秒)" width="90" align="center" />
                <el-table-column prop="itemsProcessed" label="处理项" width="80" align="center" />
                <el-table-column prop="spaceFreedMb" label="释放空间(MB)" width="110" align="center" />
                <el-table-column prop="outputMessage" label="输出" min-width="200" />
              </el-table>
              <el-pagination
                :current-page="cleanupHistoryPagination.page"
                :page-size="cleanupHistoryPagination.limit"
                :total="cleanupHistoryPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => $emit('cleanup-history-page-change', p)"
                class="mt-3"
              />
            </div>
          </el-tab-pane>
          <!-- 存储统计 -->
          <el-tab-pane label="存储统计" name="storage">
            <div v-loading="storageStatsLoading">
              <el-table :data="storageStats" stripe>
                <el-table-column prop="tableName" label="表名" width="150" />
                <el-table-column prop="rowCount" label="行数" width="100" align="right" />
                <el-table-column prop="dataLengthMb" label="数据MB" width="100" align="right" />
                <el-table-column prop="indexLengthMb" label="索引MB" width="100" align="right" />
                <el-table-column prop="totalLengthMb" label="总计MB" width="100" align="right" />
                <el-table-column prop="fragmentRatio" label="碎片率%" width="90" align="right" />
              </el-table>
            </div>
          </el-tab-pane>
        </el-tabs>
      </div>
    </el-tab-pane>

    <!-- 内容审核 -->
    <el-tab-pane label="内容审核" name="moderation" v-if="authStore.isSuperAdmin">
      <div class="tab-content">
        <el-tabs :model-value="moderationSubTab" @update:model-value="(val: string) => $emit('update:moderationSubTab', val)" class="sub-tabs">
          <!-- 待审核论文 -->
          <el-tab-pane label="待审核论文" name="pending">
            <div v-loading="pendingPapersLoading">
              <el-table :data="pendingPapers" stripe>
                <el-table-column prop="paperId" label="论文ID" width="80" />
                <el-table-column prop="status" label="状态" width="90">
                  <template #default="{ row }">
                    <el-tag :type="getModerationStatusTagType(row.status)" size="small">
                      {{ getModerationStatusLabel(row.status) }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="createdAt" label="提交时间" width="160" />
                <el-table-column label="操作" width="150">
                  <template #default="{ row }">
                    <el-button size="small" type="success" @click="$emit('approve-paper', row)" :disabled="row.status !== 'pending'">通过</el-button>
                    <el-button size="small" type="danger" @click="$emit('show-reject-paper-dialog', row)" :disabled="row.status !== 'pending'">拒绝</el-button>
                  </template>
                </el-table-column>
              </el-table>
              <el-pagination
                :current-page="pendingPapersPagination.page"
                :page-size="pendingPapersPagination.limit"
                :total="pendingPapersPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => $emit('pending-papers-page-change', p)"
                class="mt-3"
              />
            </div>
          </el-tab-pane>
          <!-- 用户举报 -->
          <el-tab-pane label="用户举报" name="reports">
            <div v-loading="userReportsLoading">
              <el-select :model-value="userReportsStatusFilter" placeholder="状态筛选" class="mb-3" style="width: 150px" clearable @change="(val: string) => { $emit('update:userReportsStatusFilter', val); $emit('load-user-reports') }">
                <el-option label="待处理" value="pending" />
                <el-option label="已处理" value="resolved" />
                <el-option label="已忽略" value="dismissed" />
              </el-select>
              <el-table :data="userReports" stripe class="mt-3">
                <el-table-column prop="reporterUsername" label="举报人" width="100" />
                <el-table-column prop="targetType" label="类型" width="80">
                  <template #default="{ row }">
                    <el-tag size="small">{{ getReportTargetTypeLabel(row.targetType) }}</el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="targetId" label="目标ID" width="80" />
                <el-table-column prop="reason" label="原因" width="100" />
                <el-table-column prop="priority" label="优先级" width="80">
                  <template #default="{ row }">
                    <el-tag :type="getReportPriorityTagType(row.priority)" size="small">
                      {{ getReportPriorityLabel(row.priority) }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="description" label="描述" min-width="200" />
                <el-table-column prop="status" label="状态" width="90" />
                <el-table-column label="操作" width="100">
                  <template #default="{ row }">
                    <el-button size="small" type="primary" @click="$emit('show-resolve-report-dialog', row)" :disabled="row.status !== 'pending'">处理</el-button>
                  </template>
                </el-table-column>
              </el-table>
              <el-pagination
                :current-page="userReportsPagination.page"
                :page-size="userReportsPagination.limit"
                :total="userReportsPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => $emit('user-reports-page-change', p)"
                class="mt-3"
              />
            </div>
          </el-tab-pane>
          <!-- 敏感词 -->
          <el-tab-pane label="敏感词" name="sensitive-words">
            <div v-loading="sensitiveWordsLoading">
              <el-button type="primary" @click="$emit('show-create-sensitive-word-dialog')" class="mb-3">添加敏感词</el-button>
              <el-table :data="sensitiveWords" stripe>
                <el-table-column prop="word" label="敏感词" width="150" />
                <el-table-column prop="category" label="分类" width="100">
                  <template #default="{ row }">
                    <el-tag size="small">{{ getSensitiveWordCategoryLabel(row.category) }}</el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="severity" label="严重程度" width="90">
                  <template #default="{ row }">
                    <el-tag :type="getSensitiveWordSeverityTagType(row.severity)" size="small">
                      {{ getSensitiveWordSeverityLabel(row.severity) }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="isRegex" label="正则" width="70" align="center">
                  <template #default="{ row }">
                    <el-tag :type="row.isRegex ? 'warning' : 'info'" size="small">
                      {{ row.isRegex ? '是' : '否' }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="matchCount" label="匹配次数" width="90" align="center" />
                <el-table-column label="操作" width="100">
                  <template #default="{ row }">
                    <el-button size="small" type="danger" @click="$emit('delete-sensitive-word', row)">删除</el-button>
                  </template>
                </el-table-column>
              </el-table>
            </div>
          </el-tab-pane>
        </el-tabs>
      </div>
    </el-tab-pane>

    <!-- API密钥 -->
    <el-tab-pane label="API密钥" name="api-keys" v-if="authStore.isSuperAdmin">
      <div class="tab-content">
        <el-tabs :model-value="apiKeysSubTab" @update:model-value="(val: string) => $emit('update:apiKeysSubTab', val)" class="sub-tabs">
          <!-- 密钥列表 -->
          <el-tab-pane label="密钥列表" name="list">
            <div v-loading="apiKeysLoading">
              <el-button type="primary" @click="$emit('show-create-api-key-dialog')" class="mb-3">创建密钥</el-button>
              <el-table :data="apiKeys" stripe>
                <el-table-column prop="name" label="名称" width="150" />
                <el-table-column prop="username" label="所属用户" width="120" />
                <el-table-column prop="keyPrefix" label="密钥前缀" width="100" />
                <el-table-column prop="rateLimitPerHour" label="速率限制/时" width="110" align="center" />
                <el-table-column prop="expiresAt" label="过期时间" width="160">
                  <template #default="{ row }">
                    {{ row.expiresAt || '永久' }}
                  </template>
                </el-table-column>
                <el-table-column prop="requestCount" label="请求次数" width="90" align="right" />
                <el-table-column prop="isActive" label="状态" width="80" align="center">
                  <template #default="{ row }">
                    <el-switch :model-value="row.isActive" @change="$emit('toggle-api-key', row)" />
                  </template>
                </el-table-column>
                <el-table-column label="操作" width="150">
                  <template #default="{ row }">
                    <el-button size="small" @click="$emit('show-api-key-usage', row)">使用</el-button>
                    <el-button size="small" type="warning" @click="$emit('regenerate-api-key', row)">重新生成</el-button>
                    <el-button size="small" type="danger" @click="$emit('delete-api-key', row)">删除</el-button>
                  </template>
                </el-table-column>
              </el-table>
              <el-pagination
                :current-page="apiKeysPagination.page"
                :page-size="apiKeysPagination.limit"
                :total="apiKeysPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => $emit('api-keys-page-change', p)"
                class="mt-3"
              />
            </div>
          </el-tab-pane>
          <!-- 使用统计 -->
          <el-tab-pane label="使用统计" name="usage">
            <div v-loading="apiKeyUsageLoading">
              <div v-if="apiKeyStats">
                <el-row :gutter="20" class="mb-4">
                  <el-col :span="6">
                    <el-statistic title="总请求数" :value="apiKeyStats.totalRequests" />
                  </el-col>
                  <el-col :span="6">
                    <el-statistic title="成功率" :value="Math.round(apiKeyStats.successfulRequests / apiKeyStats.totalRequests * 100)" suffix="%" />
                  </el-col>
                  <el-col :span="6">
                    <el-statistic title="平均响应时间" :value="Math.round(apiKeyStats.avgResponseTime)" suffix="ms" />
                  </el-col>
                  <el-col :span="6">
                    <el-statistic title="失败数" :value="apiKeyStats.failedRequests" />
                  </el-col>
                </el-row>
              </div>
              <el-table :data="apiKeyUsage" stripe class="mt-3">
                <el-table-column prop="keyName" label="密钥" width="120" />
                <el-table-column prop="endpoint" label="端点" width="200" />
                <el-table-column prop="method" label="方法" width="70" />
                <el-table-column prop="statusCode" label="状态码" width="80" align="center" />
                <el-table-column prop="responseTimeMs" label="响应时间ms" width="100" align="center" />
                <el-table-column prop="createdAt" label="时间" width="160" />
              </el-table>
            </div>
          </el-tab-pane>
        </el-tabs>
      </div>
    </el-tab-pane>

    <!-- Dialogs for data management -->

    <!-- 编辑配置对话框 -->
    <el-dialog :model-value="editConfigDialogVisible" title="编辑配置" width="500px" @update:model-value="(val: boolean) => $emit('update:editConfigDialogVisible', val)">
      <el-form :model="editConfigForm" label-width="100px">
        <el-form-item label="配置键">
          <el-input :model-value="editConfigForm.key" disabled />
        </el-form-item>
        <el-form-item label="当前值">
          <el-input :model-value="editConfigForm.value" @update:model-value="(val: string) => $emit('update:editConfigForm', 'value', val)" type="textarea" :rows="3" />
        </el-form-item>
        <el-form-item label="原因">
          <el-input :model-value="editConfigForm.reason" @update:model-value="(val: string) => $emit('update:editConfigForm', 'reason', val)" placeholder="修改原因（可选）" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="$emit('update:editConfigDialogVisible', false)">取消</el-button>
        <el-button type="primary" @click="$emit('update-config')" :loading="configLoading">保存</el-button>
      </template>
    </el-dialog>

    <!-- 创建备份任务对话框 -->
    <el-dialog :model-value="createBackupJobDialogVisible" title="创建备份任务" width="500px" @update:model-value="(val: boolean) => $emit('update:createBackupJobDialogVisible', val)">
      <el-form :model="createBackupJobForm" label-width="120px">
        <el-form-item label="任务名称" required>
          <el-input :model-value="createBackupJobForm.name" @update:model-value="(val: string) => $emit('update:createBackupJobForm', 'name', val)" placeholder="例如: Daily Full Backup" />
        </el-form-item>
        <el-form-item label="备份类型">
          <el-select :model-value="createBackupJobForm.jobType" @update:model-value="(val: string) => $emit('update:createBackupJobForm', 'jobType', val)">
            <el-option label="完整备份" value="full" />
            <el-option label="增量备份" value="incremental" />
            <el-option label="仅数据库" value="database_only" />
            <el-option label="仅文件" value="files_only" />
          </el-select>
        </el-form-item>
        <el-form-item label="计划 (Cron)">
          <el-input :model-value="createBackupJobForm.scheduleCron" @update:model-value="(val: string) => $emit('update:createBackupJobForm', 'scheduleCron', val)" placeholder="例如: 0 2 * * * (每天凌晨2点)" />
          <div class="text-xs text-gray-500 mt-1">
            格式: 分 时 日 月 周，留空表示手动执行
          </div>
        </el-form-item>
        <el-form-item label="备份路径">
          <el-input :model-value="createBackupJobForm.backupPath" @update:model-value="(val: string) => $emit('update:createBackupJobForm', 'backupPath', val)" placeholder="/backups" />
        </el-form-item>
        <el-form-item label="保留天数">
          <el-input-number :model-value="createBackupJobForm.retentionDays" @update:model-value="(val: number) => $emit('update:createBackupJobForm', 'retentionDays', val)" :min="1" :max="365" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="$emit('update:createBackupJobDialogVisible', false)">取消</el-button>
        <el-button type="primary" @click="$emit('create-backup-job')" :loading="backupLoading">创建</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { Search, Refresh, Plus, Edit } from '@element-plus/icons-vue'

defineProps<{
  authStore: any
  // Config
  configSubTab: string
  configLoading: boolean
  configCategoryFilter: string
  configs: any[]
  selectedConfig: any
  configHistory: any[]
  configHistoryKeyFilter: string
  configHistoryPagination: { page: number; limit: number; total: number }
  editConfigDialogVisible: boolean
  editConfigForm: { key: string; value: string; reason: string }
  getConfigChangeTypeTag: (type: string) => string
  // Backup
  backupSubTab: string
  backupLoading: boolean
  backupJobs: any[]
  backupRecords: any[]
  backupRecordsPagination: { page: number; limit: number; total: number }
  backupRecordsJobFilter: number
  createBackupJobDialogVisible: boolean
  createBackupJobForm: any
  getBackupJobTypeLabel: (type: string) => string
  getBackupStatusTagType: (status: string) => string
  formatFileSize: (bytes: number) => string
  // Permissions
  permissionsSubTab: string
  permissionsLoading: boolean
  permissionMatrix: any[]
  roles: any[]
  rolesLoading: boolean
  userRolesLoading: boolean
  userRoleSearchUserId: string
  userRoles: any[]
  // Notifications
  notificationsSubTab: string
  notificationsLoading: boolean
  systemNotifications: any[]
  notificationsPagination: { page: number; limit: number; total: number }
  notificationTemplatesLoading: boolean
  notificationTemplates: any[]
  getNotificationStatusTagType: (status: string) => string
  getNotificationStatusLabel: (status: string) => string
  // Cleanup
  cleanupSubTab: string
  cleanupTasksLoading: boolean
  cleanupTasks: any[]
  cleanupHistoryLoading: boolean
  cleanupHistory: any[]
  cleanupHistoryPagination: { page: number; limit: number; total: number }
  storageStatsLoading: boolean
  storageStats: any[]
  getCleanupTaskTypeLabel: (type: string) => string
  getCleanupStatusTagType: (status: string) => string
  getCleanupStatusLabel: (status: string) => string
  // Moderation
  moderationSubTab: string
  pendingPapersLoading: boolean
  pendingPapers: any[]
  pendingPapersPagination: { page: number; limit: number; total: number }
  userReportsLoading: boolean
  userReports: any[]
  userReportsPagination: { page: number; limit: number; total: number }
  userReportsStatusFilter: string
  sensitiveWordsLoading: boolean
  sensitiveWords: any[]
  getModerationStatusTagType: (status: string) => string
  getModerationStatusLabel: (status: string) => string
  getReportTargetTypeLabel: (type: string) => string
  getReportPriorityTagType: (priority: string) => string
  getReportPriorityLabel: (priority: string) => string
  getSensitiveWordCategoryLabel: (category: string) => string
  getSensitiveWordSeverityTagType: (severity: string) => string
  getSensitiveWordSeverityLabel: (severity: string) => string
  // API Keys
  apiKeysSubTab: string
  apiKeysLoading: boolean
  apiKeys: any[]
  apiKeysPagination: { page: number; limit: number; total: number }
  apiKeyUsageLoading: boolean
  apiKeyUsage: any[]
  apiKeyStats: any
}>()

defineEmits<{
  // Config
  (e: 'update:configSubTab', value: string): void
  (e: 'update:configCategoryFilter', value: string): void
  (e: 'update:configHistoryKeyFilter', value: string): void
  (e: 'update:editConfigDialogVisible', value: boolean): void
  (e: 'update:editConfigForm', field: string, value: any): void
  (e: 'load-configs'): void
  (e: 'load-config-history'): void
  (e: 'config-selection-change', selection: any[]): void
  (e: 'show-edit-config-dialog'): void
  (e: 'update-config'): void
  (e: 'config-history-page-change', page: number): void
  // Backup
  (e: 'update:backupSubTab', value: string): void
  (e: 'update:backupRecordsJobFilter', value: number): void
  (e: 'update:createBackupJobDialogVisible', value: boolean): void
  (e: 'update:createBackupJobForm', field: string, value: any): void
  (e: 'load-backup-jobs'): void
  (e: 'load-backup-records'): void
  (e: 'show-create-backup-job-dialog'): void
  (e: 'create-backup-job'): void
  (e: 'trigger-backup-job', jobId: number): void
  (e: 'delete-backup-job', id: number): void
  (e: 'delete-backup-file', id: number): void
  (e: 'download-backup', id: number): void
  (e: 'backup-records-page-change', page: number): void
  // Permissions
  (e: 'update:permissionsSubTab', value: string): void
  (e: 'update:userRoleSearchUserId', value: string): void
  (e: 'load-permission-matrix'): void
  (e: 'load-user-roles'): void
  (e: 'show-create-role-dialog'): void
  (e: 'show-assign-role-dialog'): void
  (e: 'edit-role', role: any): void
  (e: 'delete-role', role: any): void
  (e: 'remove-user-role', userRole: any): void
  // Notifications
  (e: 'update:notificationsSubTab', value: string): void
  (e: 'load-system-notifications'): void
  (e: 'show-send-notification-dialog'): void
  (e: 'notifications-page-change', page: number): void
  // Cleanup
  (e: 'update:cleanupSubTab', value: string): void
  (e: 'load-cleanup-tasks'): void
  (e: 'show-create-cleanup-task-dialog'): void
  (e: 'trigger-cleanup-task', task: any): void
  (e: 'delete-cleanup-task', task: any): void
  (e: 'toggle-cleanup-task', task: any): void
  (e: 'cleanup-history-page-change', page: number): void
  // Moderation
  (e: 'update:moderationSubTab', value: string): void
  (e: 'update:userReportsStatusFilter', value: string): void
  (e: 'load-user-reports'): void
  (e: 'show-create-sensitive-word-dialog'): void
  (e: 'approve-paper', paper: any): void
  (e: 'show-reject-paper-dialog', paper: any): void
  (e: 'show-resolve-report-dialog', report: any): void
  (e: 'delete-sensitive-word', word: any): void
  (e: 'pending-papers-page-change', page: number): void
  (e: 'user-reports-page-change', page: number): void
  // API Keys
  (e: 'update:apiKeysSubTab', value: string): void
  (e: 'show-create-api-key-dialog'): void
  (e: 'show-api-key-usage', key: any): void
  (e: 'regenerate-api-key', key: any): void
  (e: 'delete-api-key', key: any): void
  (e: 'toggle-api-key', key: any): void
  (e: 'api-keys-page-change', page: number): void
}>()
</script>
