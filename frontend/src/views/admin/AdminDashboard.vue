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
          <!-- 统计卡片 (moved from top) -->
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
        <div class="tab-content">
          <!-- 搜索和过滤 -->
          <el-row :gutter="20" class="mb-3">
            <el-col :span="12">
              <el-input
                v-model="searchQuery"
                placeholder="搜索用户名或邮箱"
                clearable
                @input="handleSearch"
              >
                <template #prefix>
                  <el-icon><Search /></el-icon>
                </template>
              </el-input>
            </el-col>
            <el-col :span="6">
              <el-select v-model="roleFilter" placeholder="按角色筛选" clearable @change="loadUsers">
                <el-option label="所有用户" value="" />
                <el-option label="普通用户" value="user" />
                <el-option label="高级用户" value="premium" />
                <el-option label="管理员" value="admin" />
                <el-option label="超级管理员" value="superadmin" />
              </el-select>
            </el-col>
            <el-col :span="6" class="text-right">
              <el-space>
                <el-button type="primary" @click="handleCreateUser" v-if="authStore.isSuperAdmin">
                  <el-icon><Plus /></el-icon>
                  新增用户
                </el-button>
                <el-button @click="handleExportUsers">
                  <el-icon><Download /></el-icon>
                  导出CSV
                </el-button>
                <el-button @click="loadUsers">
                  <el-icon><Refresh /></el-icon>
                  刷新
                </el-button>
              </el-space>
            </el-col>
          </el-row>

          <!-- 用户列表 -->
          <el-table :data="users" stripe v-loading="loading" style="width: 100%">
            <el-table-column prop="id" label="ID" width="70" />
            <el-table-column prop="username" label="用户名" min-width="120">
              <template #default="{ row }">
                <el-link type="primary" @click="openUserDetail(row)">{{ row.username }}</el-link>
              </template>
            </el-table-column>
            <el-table-column prop="email" label="邮箱" min-width="180" />
            <el-table-column prop="fullName" label="姓名" min-width="120" />
            <el-table-column label="角色" width="100">
              <template #default="{ row }">
                <el-tag :type="getRoleBadgeType(row.role)" size="small">
                  {{ getRoleLabel(row.role) }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="账号状态" width="90">
              <template #default="{ row }">
                <el-tag :type="row.isActive ? 'success' : 'danger'" size="small">
                  {{ row.isActive ? '启用' : '停用' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="活跃度" width="90">
              <template #default="{ row }">
                <el-tag :type="getActivityTagType(row.activityStatus)" size="small">
                  {{ getActivityLabel(row.activityStatus) }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column prop="loginCount" label="登录次数" width="90" align="center" />
            <el-table-column label="注册时间" min-width="150">
              <template #default="{ row }">
                {{ formatDateTime(row.createdAt) }}
              </template>
            </el-table-column>
            <el-table-column label="最后登录" min-width="150">
              <template #default="{ row }">
                {{ row.lastLoginAt ? formatDateTime(row.lastLoginAt) : '未登录' }}
              </template>
            </el-table-column>
            <el-table-column label="操作" width="200" fixed="right">
              <template #default="{ row }">
                <el-button
                  v-if="authStore.isSuperAdmin && row.id !== authStore.user?.id"
                  type="primary"
                  size="small"
                  @click="handleEditUser(row)"
                >
                  编辑
                </el-button>
                <el-button
                  v-if="row.isActive"
                  :type="canManageRole(authStore.user?.role || 'user', row.role) ? 'warning' : 'info'"
                  size="small"
                  @click="handleToggleUserStatus(row)"
                >
                  停用
                </el-button>
                <el-button
                  v-else
                  type="success"
                  size="small"
                  @click="handleToggleUserStatus(row)"
                >
                  启用
                </el-button>
              </template>
            </el-table-column>
          </el-table>

          <!-- 分页 -->
          <el-pagination
            v-model:current-page="pagination.page"
            v-model:page-size="pagination.limit"
            :total="pagination.total"
            :page-sizes="[10, 20, 50, 100]"
            layout="total, sizes, prev, pager, next, jumper"
            @size-change="handleSizeChange"
            @current-change="handlePageChange"
            class="mt-3"
          />
        </div>
      </el-tab-pane>

      <!-- 审计日志 (仅超级管理员) -->
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

          <!-- 分页 -->
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

      <!-- 模块管理 (仅超级管理员) -->
      <el-tab-pane label="模块管理" name="modules" v-if="authStore.isSuperAdmin">
        <div class="tab-content">
          <!-- 操作按钮 -->
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

          <!-- 模块列表 -->
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

      <!-- 公告管理 (仅超级管理员) -->
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

      <!-- 系统监控 (仅超级管理员) -->
      <el-tab-pane label="系统监控" name="monitoring" v-if="authStore.isSuperAdmin">
        <div class="tab-content">
          <!-- 监控子标签 -->
          <el-tabs v-model="monitoringSubTab" class="monitoring-sub-tabs">
            <!-- 系统资源 -->
            <el-tab-pane label="系统资源" name="resources">
              <el-row :gutter="20" class="mb-3">
                <el-col :span="6">
                  <el-card shadow="hover">
                    <el-statistic title="CPU 使用率" :value="systemMetrics.cpu_percent || 0" :precision="1" suffix="%" />
                    <el-progress :percentage="systemMetrics.cpu_percent || 0" :color="getProgressColor(systemMetrics.cpu_percent || 0)" />
                  </el-card>
                </el-col>
                <el-col :span="6">
                  <el-card shadow="hover">
                    <el-statistic title="内存使用率" :value="systemMetrics.memory_percent || 0" :precision="1" suffix="%" />
                    <el-progress :percentage="systemMetrics.memory_percent || 0" :color="getProgressColor(systemMetrics.memory_percent || 0)" />
                    <div class="text-xs text-gray-500 mt-1">
                      {{ formatBytes(systemMetrics.memory_used_mb * 1024 * 1024) }} / {{ formatBytes(systemMetrics.memory_total_mb * 1024 * 1024) }}
                    </div>
                  </el-card>
                </el-col>
                <el-col :span="6">
                  <el-card shadow="hover">
                    <el-statistic title="磁盘使用率" :value="systemMetrics.disk_percent || 0" :precision="1" suffix="%" />
                    <el-progress :percentage="systemMetrics.disk_percent || 0" :color="getProgressColor(systemMetrics.disk_percent || 0)" />
                    <div class="text-xs text-gray-500 mt-1">
                      {{ formatBytes(systemMetrics.disk_used_gb * 1024 * 1024 * 1024) }} / {{ formatBytes(systemMetrics.disk_total_gb * 1024 * 1024 * 1024) }}
                    </div>
                  </el-card>
                </el-col>
                <el-col :span="6">
                  <el-card shadow="hover">
                    <el-statistic title="运行时长" :value="formatUptime(systemMetrics.uptime_seconds || 0)" />
                  </el-card>
                </el-col>
              </el-row>
              <el-row :gutter="20" class="mb-3">
                <el-col :span="12">
                  <el-card shadow="hover">
                    <template #header>
                      <span>网络流量</span>
                    </template>
                    <el-row>
                      <el-col :span="12">
                        <div class="text-center">
                          <div class="text-2xl font-bold text-blue-500">{{ (systemMetrics.network_rx_mbps || 0).toFixed(2) }}</div>
                          <div class="text-xs text-gray-500">下载 (MB/s)</div>
                        </div>
                      </el-col>
                      <el-col :span="12">
                        <div class="text-center">
                          <div class="text-2xl font-bold text-green-500">{{ (systemMetrics.network_tx_mbps || 0).toFixed(2) }}</div>
                          <div class="text-xs text-gray-500">上传 (MB/s)</div>
                        </div>
                      </el-col>
                    </el-row>
                  </el-card>
                </el-col>
                <el-col :span="12">
                  <el-card shadow="hover">
                    <template #header>
                      <span>活跃连接</span>
                    </template>
                    <div class="text-center">
                      <div class="text-4xl font-bold">{{ systemMetrics.active_connections || 0 }}</div>
                    </div>
                  </el-card>
                </el-col>
              </el-row>
              <el-button @click="loadSystemMetrics" :loading="metricsLoading">
                <el-icon><Refresh /></el-icon> 刷新数据
              </el-button>
            </el-tab-pane>

            <!-- 服务健康 -->
            <el-tab-pane label="服务健康" name="services">
              <el-table :data="serviceHealthList" stripe v-loading="metricsLoading" style="width: 100%">
                <el-table-column prop="name" label="服务名称" min-width="180" />
                <el-table-column label="状态" width="100">
                  <template #default="{ row }">
                    <el-tag :type="row.status === 'healthy' ? 'success' : row.status === 'degraded' ? 'warning' : 'danger'" size="small">
                      {{ ({ healthy: '健康', degraded: '降级', down: '离线' } as Record<string, string>)[row.status] || row.status }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="response_time_ms" label="响应时间" width="110" align="center">
                  <template #default="{ row }">
                    {{ row.response_time_ms }} ms
                  </template>
                </el-table-column>
                <el-table-column prop="error_message" label="错误信息" min-width="200" show-overflow-tooltip />
                <el-table-column prop="last_check" label="最后检查" min-width="160" />
              </el-table>
              <el-button @click="loadServiceHealth" class="mt-3" :loading="metricsLoading">
                <el-icon><Refresh /></el-icon> 刷新状态
              </el-button>
            </el-tab-pane>

            <!-- 系统日志 -->
            <el-tab-pane label="系统日志" name="logs">
              <el-row :gutter="20" class="mb-3">
                <el-col :span="6">
                  <el-select v-model="logLevelFilter" placeholder="日志级别" clearable @change="loadSystemLogs">
                    <el-option label="全部" value="" />
                    <el-option label="调试 (debug)" value="debug" />
                    <el-option label="信息 (info)" value="info" />
                    <el-option label="警告 (warning)" value="warning" />
                    <el-option label="错误 (error)" value="error" />
                    <el-option label="严重 (critical)" value="critical" />
                  </el-select>
                </el-col>
                <el-col :span="6">
                  <el-select v-model="logModuleFilter" placeholder="模块" clearable @change="loadSystemLogs">
                    <el-option label="全部" value="" />
                    <el-option label="AdminApiModule" value="AdminApiModule" />
                    <el-option label="AuthApiModule" value="AuthApiModule" />
                    <el-option label="UserApiModule" value="UserApiModule" />
                  </el-select>
                </el-col>
                <el-col :span="12">
                  <el-button type="danger" @click="handleCleanLogs" :disabled="!logLevelFilter && !logModuleFilter">
                    <el-icon><Delete /></el-icon> 清理旧日志
                  </el-button>
                </el-col>
              </el-row>
              <el-table :data="systemLogs" stripe v-loading="metricsLoading" style="width: 100%">
                <el-table-column prop="level" label="级别" width="90">
                  <template #default="{ row }">
                    <el-tag :type="getLogLevelTagType(row.level)" size="small">{{ row.level }}</el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="module" label="模块" width="140" />
                <el-table-column prop="message" label="消息" min-width="300" show-overflow-tooltip />
                <el-table-column prop="file" label="文件" width="150" show-overflow-tooltip />
                <el-table-column prop="line" label="行号" width="70" align="center" />
                <el-table-column prop="created_at" label="时间" min-width="160" />
              </el-table>
              <el-pagination
                v-model:current-page="logPagination.page"
                v-model:page-size="logPagination.limit"
                :total="logPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => { logPagination.page = p; loadSystemLogs() }"
                class="mt-3"
              />
            </el-tab-pane>

            <!-- 性能指标 -->
            <el-tab-pane label="性能指标" name="performance">
              <el-row :gutter="20" class="mb-3">
                <el-col :span="12">
                  <el-card shadow="hover">
                    <template #header>
                      <span>慢查询 TOP 10</span>
                    </template>
                    <el-table :data="slowQueries" stripe v-loading="metricsLoading" size="small" style="width: 100%">
                      <el-table-column prop="endpoint" label="端点" min-width="150" show-overflow-tooltip />
                      <el-table-column prop="execution_time_ms" label="耗时" width="80" align="center">
                        <template #default="{ row }">
                          {{ row.execution_time_ms }} ms
                        </template>
                      </el-table-column>
                      <el-table-column prop="module" label="模块" width="120" />
                    </el-table>
                  </el-card>
                </el-col>
                <el-col :span="12">
                  <el-card shadow="hover">
                    <template #header>
                      <span>性能瓶颈</span>
                    </template>
                    <el-table :data="bottlenecks" stripe v-loading="metricsLoading" size="small" style="width: 100%">
                      <el-table-column prop="type" label="类型" width="100">
                        <template #default="{ row }">
                          <el-tag :type="row.severity === 'high' ? 'danger' : row.severity === 'medium' ? 'warning' : 'info'" size="small">
                            {{ ({ slow_query: '慢查询', high_error_rate: '高错误率', unhealthy_service: '服务异常' } as Record<string, string>)[row.type] || row.type }}
                          </el-tag>
                        </template>
                      </el-table-column>
                      <el-table-column prop="endpoint" label="端点" min-width="120" show-overflow-tooltip />
                      <el-table-column prop="description" label="描述" min-width="150" show-overflow-tooltip />
                    </el-table>
                  </el-card>
                </el-col>
              </el-row>
              <el-button @click="loadPerformanceData" :loading="metricsLoading">
                <el-icon><Refresh /></el-icon> 刷新性能数据
              </el-button>
            </el-tab-pane>
          </el-tabs>
        </div>
      </el-tab-pane>

      <!-- 登录安全 (仅超级管理员) -->
      <el-tab-pane label="登录安全" name="security" v-if="authStore.isSuperAdmin">
        <div class="tab-content">
          <!-- 安全子标签 -->
          <el-tabs v-model="securitySubTab" class="security-sub-tabs">
            <!-- 登录历史 -->
            <el-tab-pane label="登录历史" name="history">
              <el-row :gutter="20" class="mb-3">
                <el-col :span="12">
                  <el-input
                    v-model="loginHistoryUsernameFilter"
                    placeholder="按用户名过滤"
                    clearable
                    @change="loadLoginHistory"
                    style="width: 300px"
                  >
                    <template #prefix>
                      <el-icon><Search /></el-icon>
                    </template>
                  </el-input>
                </el-col>
                <el-col :span="12">
                  <el-button @click="loadLoginHistory" :loading="securityLoading">
                    <el-icon><Refresh /></el-icon> 刷新
                  </el-button>
                </el-col>
              </el-row>
              <el-table :data="allLoginHistory" stripe v-loading="securityLoading" style="width: 100%">
                <el-table-column prop="username" label="用户名" width="140" />
                <el-table-column prop="ip_address" label="IP地址" width="140" />
                <el-table-column label="状态" width="90">
                  <template #default="{ row }">
                    <el-tag :type="row.success ? 'success' : 'danger'" size="small">
                      {{ row.success ? '成功' : '失败' }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="failure_reason" label="失败原因" min-width="150" show-overflow-tooltip />
                <el-table-column prop="user_agent" label="用户代理" min-width="200" show-overflow-tooltip />
                <el-table-column prop="created_at" label="时间" min-width="160" />
              </el-table>
              <el-pagination
                v-model:current-page="loginHistoryPagination.page"
                v-model:page-size="loginHistoryPagination.limit"
                :total="loginHistoryPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => { loginHistoryPagination.page = p; loadLoginHistory() }"
                class="mt-3"
              />
            </el-tab-pane>

            <!-- 可疑登录 -->
            <el-tab-pane label="可疑登录" name="suspicious">
              <el-row :gutter="20" class="mb-3">
                <el-col :span="12">
                  <el-select v-model="suspiciousStatusFilter" placeholder="状态过滤" clearable @change="loadSuspiciousLogins">
                    <el-option label="全部" value="" />
                    <el-option label="待处理" value="pending" />
                    <el-option label="已审核" value="reviewed" />
                    <el-option label="已白名单" value="whitelisted" />
                    <el-option label="确认威胁" value="confirmed_threat" />
                  </el-select>
                </el-col>
                <el-col :span="12">
                  <el-button @click="loadSuspiciousLogins" :loading="securityLoading">
                    <el-icon><Refresh /></el-icon> 刷新
                  </el-button>
                </el-col>
              </el-row>
              <el-table :data="suspiciousLogins" stripe v-loading="securityLoading" style="width: 100%">
                <el-table-column prop="username" label="用户名" width="140" />
                <el-table-column prop="ip_address" label="IP地址" width="140" />
                <el-table-column prop="suspicion_reason" label="可疑原因" width="130">
                  <template #default="{ row }">
                    {{ ({
                      multiple_failures: '多次失败',
                      unknown_location: '未知位置',
                      impossible_travel: '不可能的行程',
                      bot_pattern: '机器人模式',
                      blacklisted_ip: '黑名单IP'
                    } as Record<string, string>)[row.suspicion_reason] || row.suspicion_reason }}
                  </template>
                </el-table-column>
                <el-table-column prop="risk_score" label="风险分数" width="90" align="center">
                  <template #default="{ row }">
                    <el-tag :type="row.risk_score >= 80 ? 'danger' : row.risk_score >= 50 ? 'warning' : 'info'" size="small">
                      {{ row.risk_score }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="status" label="状态" width="100">
                  <template #default="{ row }">
                    <el-tag :type="getSuspiciousStatusTagType(row.status)" size="small">
                      {{ getSuspiciousStatusLabel(row.status) }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="created_at" label="检测时间" min-width="160" />
                <el-table-column label="操作" width="180" fixed="right">
                  <template #default="{ row }">
                    <el-button size="small" @click="handleSuspiciousLoginAction(row.id, 'reviewed')" :disabled="row.status !== 'pending'">标记已审</el-button>
                    <el-button size="small" type="success" @click="handleSuspiciousLoginAction(row.id, 'whitelisted')" :disabled="row.status !== 'pending'">白名单</el-button>
                    <el-button size="small" type="danger" @click="handleSuspiciousLoginAction(row.id, 'confirmed_threat')" :disabled="row.status !== 'pending'">确认威胁</el-button>
                  </template>
                </el-table-column>
              </el-table>
              <el-pagination
                v-model:current-page="suspiciousPagination.page"
                v-model:page-size="suspiciousPagination.limit"
                :total="suspiciousPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => { suspiciousPagination.page = p; loadSuspiciousLogins() }"
                class="mt-3"
              />
            </el-tab-pane>

            <!-- IP黑名单 -->
            <el-tab-pane label="IP黑名单" name="blacklist">
              <el-row :gutter="20" class="mb-3">
                <el-col :span="18">
                  <el-button type="danger" @click="showAddIpBlacklistDialog">
                    <el-icon><Plus /></el-icon> 添加IP黑名单
                  </el-button>
                </el-col>
                <el-col :span="6">
                  <el-button @click="loadIpBlacklist" :loading="securityLoading">
                    <el-icon><Refresh /></el-icon> 刷新
                  </el-button>
                </el-col>
              </el-row>
              <el-table :data="ipBlacklist" stripe v-loading="securityLoading" style="width: 100%">
                <el-table-column prop="ip_address" label="IP地址" width="150" />
                <el-table-column prop="reason" label="原因" min-width="200" show-overflow-tooltip />
                <el-table-column prop="threat_level" label="威胁等级" width="100">
                  <template #default="{ row }">
                    <el-tag :type="getThreatLevelTagType(row.threat_level)" size="small">
                      {{ getThreatLevelLabel(row.threat_level) }}
                    </el-tag>
                  </template>
                </el-table-column>
                <el-table-column prop="attempt_count" label="尝试次数" width="90" align="center" />
                <el-table-column prop="created_by" label="创建者" width="120" />
                <el-table-column prop="expires_at" label="过期时间" min-width="160">
                  <template #default="{ row }">
                    {{ row.expires_at || '永久' }}
                  </template>
                </el-table-column>
                <el-table-column label="操作" width="100" fixed="right">
                  <template #default="{ row }">
                    <el-button size="small" type="danger" @click="handleRemoveIpBlacklist(row.id)">移除</el-button>
                  </template>
                </el-table-column>
              </el-table>
              <el-pagination
                v-model:current-page="ipBlacklistPagination.page"
                v-model:page-size="ipBlacklistPagination.limit"
                :total="ipBlacklistPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => { ipBlacklistPagination.page = p; loadIpBlacklist() }"
                class="mt-3"
              />
            </el-tab-pane>

            <!-- 账户锁定 -->
            <el-tab-pane label="账户锁定" name="lockouts">
              <el-row :gutter="20" class="mb-3">
                <el-col :span="18">
                  <el-button type="primary" @click="showLockUserDialog">
                    <el-icon><Lock /></el-icon> 锁定用户
                  </el-button>
                </el-col>
                <el-col :span="6">
                  <el-button @click="loadAccountLockouts" :loading="securityLoading">
                    <el-icon><Refresh /></el-icon> 刷新
                  </el-button>
                </el-col>
              </el-row>
              <el-table :data="accountLockouts" stripe v-loading="securityLoading" style="width: 100%">
                <el-table-column prop="username" label="用户名" width="140" />
                <el-table-column prop="lockout_reason" label="锁定原因" min-width="180" show-overflow-tooltip />
                <el-table-column prop="failed_attempts" label="失败次数" width="90" align="center" />
                <el-table-column prop="ip_address" label="IP地址" width="140" />
                <el-table-column prop="locked_until" label="锁定到期" min-width="160" />
                <el-table-column prop="created_at" label="创建时间" min-width="160" />
                <el-table-column label="操作" width="100" fixed="right">
                  <template #default="{ row }">
                    <el-button size="small" type="primary" @click="handleUnlockUserAccount(row.user_id)">解锁</el-button>
                  </template>
                </el-table-column>
              </el-table>
              <el-pagination
                v-model:current-page="accountLockoutsPagination.page"
                v-model:page-size="accountLockoutsPagination.limit"
                :total="accountLockoutsPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => { accountLockoutsPagination.page = p; loadAccountLockouts() }"
                class="mt-3"
              />
            </el-tab-pane>
          </el-tabs>
        </div>
      </el-tab-pane>

      <!-- 全局配置 (仅超级管理员) -->
      <el-tab-pane label="全局配置" name="config" v-if="authStore.isSuperAdmin">
        <div class="tab-content">
          <!-- 配置子标签 -->
          <el-tabs v-model="configSubTab" class="config-sub-tabs">
            <!-- 配置管理 -->
            <el-tab-pane label="配置管理" name="management">
              <el-row :gutter="20" class="mb-3">
                <el-col :span="12">
                  <el-select v-model="configCategoryFilter" placeholder="选择分类" clearable @change="loadConfigs">
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
                  <el-button @click="loadConfigs" :loading="configLoading">
                    <el-icon><Refresh /></el-icon> 刷新
                  </el-button>
                  <el-button type="primary" @click="showEditConfigDialog" :disabled="selectedConfig === null">
                    <el-icon><Edit /></el-icon> 编辑选中
                  </el-button>
                </el-col>
              </el-row>
              <el-table :data="configs" stripe v-loading="configLoading" style="width: 100%"
                        @selection-change="handleConfigSelectionChange" highlight-current-row>
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
                    v-model="configHistoryKeyFilter"
                    placeholder="按配置键过滤"
                    clearable
                    @change="loadConfigHistory"
                    style="width: 300px"
                  >
                    <template #prefix>
                      <el-icon><Search /></el-icon>
                    </template>
                  </el-input>
                </el-col>
                <el-col :span="6">
                  <el-button @click="loadConfigHistory" :loading="configLoading">
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
                v-model:current-page="configHistoryPagination.page"
                v-model:page-size="configHistoryPagination.limit"
                :total="configHistoryPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => { configHistoryPagination.page = p; loadConfigHistory() }"
                class="mt-3"
              />
            </el-tab-pane>
          </el-tabs>
        </div>
      </el-tab-pane>

      <!-- 数据备份 (仅超级管理员) -->
      <el-tab-pane label="数据备份" name="backup" v-if="authStore.isSuperAdmin">
        <div class="tab-content">
          <!-- 备份子标签 -->
          <el-tabs v-model="backupSubTab" class="backup-sub-tabs">
            <!-- 备份任务 -->
            <el-tab-pane label="备份任务" name="jobs">
              <el-row :gutter="20" class="mb-3">
                <el-col :span="18">
                  <el-button type="primary" @click="showCreateBackupJobDialog">
                    <el-icon><Plus /></el-icon> 创建备份任务
                  </el-button>
                </el-col>
                <el-col :span="6">
                  <el-button @click="loadBackupJobs" :loading="backupLoading">
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
                    <el-button size="small" @click="triggerBackupJob(row.id)">立即执行</el-button>
                    <el-button size="small" type="danger" @click="handleDeleteBackupJob(row.id)">删除</el-button>
                  </template>
                </el-table-column>
              </el-table>
            </el-tab-pane>

            <!-- 备份记录 -->
            <el-tab-pane label="备份记录" name="records">
              <el-row :gutter="20" class="mb-3">
                <el-col :span="18">
                  <el-select v-model="backupRecordsJobFilter" placeholder="选择任务" clearable @change="loadBackupRecords">
                    <el-option label="全部" :value="0" />
                    <el-option v-for="job in backupJobs" :key="job.id" :label="job.name" :value="job.id" />
                  </el-select>
                </el-col>
                <el-col :span="6">
                  <el-button @click="loadBackupRecords" :loading="backupLoading">
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
                    <el-button v-if="row.status === 'success'" size="small" type="primary" @click="downloadBackup(row.id)">下载</el-button>
                    <el-button size="small" type="danger" @click="handleDeleteBackupFile(row.id)" :disabled="row.status !== 'success'">删除</el-button>
                  </template>
                </el-table-column>
              </el-table>
              <el-pagination
                v-model:current-page="backupRecordsPagination.page"
                v-model:page-size="backupRecordsPagination.limit"
                :total="backupRecordsPagination.total"
                layout="total, prev, pager, next"
                @current-change="(p: number) => { backupRecordsPagination.page = p; loadBackupRecords() }"
                class="mt-3"
              />
            </el-tab-pane>
          </el-tabs>
        </div>
      </el-tab-pane>

      <!-- 权限管理 -->
      <el-tab-pane label="权限管理" name="permissions" v-if="authStore.isSuperAdmin">
        <div class="tab-content">
          <el-tabs v-model="permissionsSubTab" class="sub-tabs">
            <!-- 权限矩阵 -->
            <el-tab-pane label="权限矩阵" name="matrix">
              <div v-loading="permissionsLoading">
                <el-button type="primary" @click="loadPermissionMatrix" class="mb-3">刷新</el-button>
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
                <el-button type="primary" @click="showCreateRoleDialog" class="mb-3">创建角色</el-button>
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
                      <el-button size="small" @click="editRole(row)">编辑</el-button>
                      <el-button size="small" type="danger" @click="handleDeleteRole(row)" :disabled="row.isSystem">删除</el-button>
                    </template>
                  </el-table-column>
                </el-table>
              </div>
            </el-tab-pane>
            <!-- 用户角色 -->
            <el-tab-pane label="用户角色" name="user-roles">
              <div v-loading="userRolesLoading">
                <el-input v-model="userRoleSearchUserId" placeholder="输入用户ID" style="width: 200px" class="mr-2 mb-3" />
                <el-button type="primary" @click="loadUserRoles">查询</el-button>
                <el-button type="primary" @click="showAssignRoleDialog" :disabled="!userRoleSearchUserId">分配角色</el-button>
                <el-table :data="userRoles" stripe class="mt-3">
                  <el-table-column prop="userId" label="用户ID" width="80" />
                  <el-table-column prop="username" label="用户名" width="120" />
                  <el-table-column prop="roleName" label="角色" width="100" />
                  <el-table-column prop="roleLevel" label="级别" width="80" align="center" />
                  <el-table-column prop="assignedAt" label="分配时间" width="160" />
                  <el-table-column prop="expiresAt" label="过期时间" width="160" />
                  <el-table-column label="操作" width="100">
                    <template #default="{ row }">
                      <el-button size="small" type="danger" @click="handleRemoveUserRole(row)">移除</el-button>
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
          <el-tabs v-model="notificationsSubTab" class="sub-tabs">
            <!-- 系统通知 -->
            <el-tab-pane label="系统通知" name="system">
              <div v-loading="notificationsLoading">
                <el-button type="primary" @click="showSendNotificationDialog" class="mb-3">发送通知</el-button>
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
                  v-model:current-page="notificationsPagination.page"
                  v-model:page-size="notificationsPagination.limit"
                  :total="notificationsPagination.total"
                  layout="total, prev, pager, next"
                  @current-change="(p: number) => { notificationsPagination.page = p; loadSystemNotifications() }"
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
          <el-tabs v-model="cleanupSubTab" class="sub-tabs">
            <!-- 清理任务 -->
            <el-tab-pane label="清理任务" name="tasks">
              <div v-loading="cleanupTasksLoading">
                <el-button type="primary" @click="showCreateCleanupTaskDialog" class="mb-3">创建任务</el-button>
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
                      <el-switch v-model="row.isEnabled" @change="toggleCleanupTask(row)" />
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
                      <el-button size="small" @click="triggerCleanupTask(row)">执行</el-button>
                      <el-button size="small" type="danger" @click="handleDeleteCleanupTask(row)" :disabled="row.isSystem">删除</el-button>
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
                  v-model:current-page="cleanupHistoryPagination.page"
                  v-model:page-size="cleanupHistoryPagination.limit"
                  :total="cleanupHistoryPagination.total"
                  layout="total, prev, pager, next"
                  @current-change="(p: number) => { cleanupHistoryPagination.page = p; loadCleanupHistory() }"
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
          <el-tabs v-model="moderationSubTab" class="sub-tabs">
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
                      <el-button size="small" type="success" @click="handleApprovePaper(row)" :disabled="row.status !== 'pending'">通过</el-button>
                      <el-button size="small" type="danger" @click="showRejectPaperDialog(row)" :disabled="row.status !== 'pending'">拒绝</el-button>
                    </template>
                  </el-table-column>
                </el-table>
                <el-pagination
                  v-model:current-page="pendingPapersPagination.page"
                  v-model:page-size="pendingPapersPagination.limit"
                  :total="pendingPapersPagination.total"
                  layout="total, prev, pager, next"
                  @current-change="(p: number) => { pendingPapersPagination.page = p; loadPendingPapers() }"
                  class="mt-3"
                />
              </div>
            </el-tab-pane>
            <!-- 用户举报 -->
            <el-tab-pane label="用户举报" name="reports">
              <div v-loading="userReportsLoading">
                <el-select v-model="userReportsStatusFilter" placeholder="状态筛选" class="mb-3" style="width: 150px" clearable>
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
                      <el-button size="small" type="primary" @click="showResolveReportDialog(row)" :disabled="row.status !== 'pending'">处理</el-button>
                    </template>
                  </el-table-column>
                </el-table>
                <el-pagination
                  v-model:current-page="userReportsPagination.page"
                  v-model:page-size="userReportsPagination.limit"
                  :total="userReportsPagination.total"
                  layout="total, prev, pager, next"
                  @current-change="(p: number) => { userReportsPagination.page = p; loadUserReports() }"
                  class="mt-3"
                />
              </div>
            </el-tab-pane>
            <!-- 敏感词 -->
            <el-tab-pane label="敏感词" name="sensitive-words">
              <div v-loading="sensitiveWordsLoading">
                <el-button type="primary" @click="showCreateSensitiveWordDialog" class="mb-3">添加敏感词</el-button>
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
                      <el-button size="small" type="danger" @click="handleDeleteSensitiveWord(row)">删除</el-button>
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
          <el-tabs v-model="apiKeysSubTab" class="sub-tabs">
            <!-- 密钥列表 -->
            <el-tab-pane label="密钥列表" name="list">
              <div v-loading="apiKeysLoading">
                <el-button type="primary" @click="showCreateApiKeyDialog" class="mb-3">创建密钥</el-button>
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
                      <el-switch v-model="row.isActive" @change="toggleApiKey(row)" />
                    </template>
                  </el-table-column>
                  <el-table-column label="操作" width="150">
                    <template #default="{ row }">
                      <el-button size="small" @click="showApiKeyUsage(row)">使用</el-button>
                      <el-button size="small" type="warning" @click="handleRegenerateApiKey(row)">重新生成</el-button>
                      <el-button size="small" type="danger" @click="handleDeleteApiKey(row)">删除</el-button>
                    </template>
                  </el-table-column>
                </el-table>
                <el-pagination
                  v-model:current-page="apiKeysPagination.page"
                  v-model:page-size="apiKeysPagination.limit"
                  :total="apiKeysPagination.total"
                  layout="total, prev, pager, next"
                  @current-change="(p: number) => { apiKeysPagination.page = p; loadApiKeys() }"
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
    </el-tabs>

    <!-- 编辑配置对话框 -->
    <el-dialog v-model="editConfigDialogVisible" title="编辑配置" width="500px">
      <el-form :model="editConfigForm" label-width="100px">
        <el-form-item label="配置键">
          <el-input v-model="editConfigForm.key" disabled />
        </el-form-item>
        <el-form-item label="当前值">
          <el-input v-model="editConfigForm.value" type="textarea" :rows="3" />
        </el-form-item>
        <el-form-item label="原因">
          <el-input v-model="editConfigForm.reason" placeholder="修改原因（可选）" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="editConfigDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleUpdateConfig" :loading="configLoading">保存</el-button>
      </template>
    </el-dialog>

    <!-- 创建备份任务对话框 -->
    <el-dialog v-model="createBackupJobDialogVisible" title="创建备份任务" width="500px">
      <el-form :model="createBackupJobForm" label-width="120px">
        <el-form-item label="任务名称" required>
          <el-input v-model="createBackupJobForm.name" placeholder="例如: Daily Full Backup" />
        </el-form-item>
        <el-form-item label="备份类型">
          <el-select v-model="createBackupJobForm.jobType">
            <el-option label="完整备份" value="full" />
            <el-option label="增量备份" value="incremental" />
            <el-option label="仅数据库" value="database_only" />
            <el-option label="仅文件" value="files_only" />
          </el-select>
        </el-form-item>
        <el-form-item label="计划 (Cron)">
          <el-input v-model="createBackupJobForm.scheduleCron" placeholder="例如: 0 2 * * * (每天凌晨2点)" />
          <div class="text-xs text-gray-500 mt-1">
            格式: 分 时 日 月 周，留空表示手动执行
          </div>
        </el-form-item>
        <el-form-item label="备份路径">
          <el-input v-model="createBackupJobForm.backupPath" placeholder="/backups" />
        </el-form-item>
        <el-form-item label="保留天数">
          <el-input-number v-model="createBackupJobForm.retentionDays" :min="1" :max="365" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="createBackupJobDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleCreateBackupJob" :loading="backupLoading">创建</el-button>
      </template>
    </el-dialog>

    <!-- 添加IP黑名单对话框 -->
    <el-dialog v-model="addIpBlacklistDialogVisible" title="添加IP黑名单" width="500px">
      <el-form :model="addIpBlacklistForm" label-width="100px">
        <el-form-item label="IP地址" required>
          <el-input v-model="addIpBlacklistForm.ipAddress" placeholder="192.168.1.1 或 192.168.1.0/24" />
        </el-form-item>
        <el-form-item label="原因" required>
          <el-input v-model="addIpBlacklistForm.reason" placeholder="例如: 暴力破解攻击" />
        </el-form-item>
        <el-form-item label="威胁等级">
          <el-select v-model="addIpBlacklistForm.threatLevel">
            <el-option label="低" value="low" />
            <el-option label="中" value="medium" />
            <el-option label="高" value="high" />
            <el-option label="严重" value="critical" />
          </el-select>
        </el-form-item>
        <el-form-item label="过期时间">
          <el-date-picker
            v-model="addIpBlacklistForm.expiresAt"
            type="datetime"
            placeholder="留空表示永久"
            format="YYYY-MM-DD HH:mm:ss"
            value-format="YYYY-MM-DD HH:mm:ss"
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="addIpBlacklistDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleAddIpBlacklist" :loading="securityLoading">添加</el-button>
      </template>
    </el-dialog>

    <!-- 锁定用户对话框 -->
    <el-dialog v-model="lockUserDialogVisible" title="锁定用户账户" width="500px">
      <el-form :model="lockUserForm" label-width="100px">
        <el-form-item label="用户ID" required>
          <el-input-number v-model="lockUserForm.userId" :min="1" />
        </el-form-item>
        <el-form-item label="锁定时长">
          <el-input-number v-model="lockUserForm.lockMinutes" :min="5" :max="1440" />
          <span class="ml-2">分钟</span>
        </el-form-item>
        <el-form-item label="锁定原因">
          <el-input v-model="lockUserForm.reason" placeholder="例如: 可疑登录活动" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="lockUserDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleLockUserAccountConfirm" :loading="securityLoading">锁定</el-button>
      </template>
    </el-dialog>

    <!-- 创建用户对话框 -->
    <el-dialog v-model="createDialogVisible" title="新增用户" width="500px">
      <el-form :model="createForm" label-width="100px" :rules="createRules" ref="createFormRef">
        <el-form-item label="用户名" prop="username">
          <el-input v-model="createForm.username" placeholder="请输入用户名" />
        </el-form-item>
        <el-form-item label="邮箱" prop="email">
          <el-input v-model="createForm.email" placeholder="请输入邮箱" />
        </el-form-item>
        <el-form-item label="姓名">
          <el-input v-model="createForm.fullName" placeholder="请输入姓名" />
        </el-form-item>
        <el-form-item label="角色">
          <el-select v-model="createForm.role" placeholder="选择角色">
            <el-option label="普通用户" value="user" />
            <el-option label="高级用户" value="premium" />
            <el-option label="管理员" value="admin" />
            <el-option label="超级管理员" value="superadmin" />
          </el-select>
        </el-form-item>
        <el-form-item label="初始密码">
          <el-input v-model="createForm.password" type="password" placeholder="默认: 123456" show-password />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="createDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleConfirmCreate" :loading="saving">创建</el-button>
      </template>
    </el-dialog>

    <!-- 编辑用户对话框 -->
    <el-dialog v-model="editDialogVisible" title="编辑用户" width="500px">
      <el-form :model="editForm" label-width="100px">
        <el-form-item label="用户名">
          <el-input v-model="editForm.username" disabled />
        </el-form-item>
        <el-form-item label="邮箱">
          <el-input v-model="editForm.email" />
        </el-form-item>
        <el-form-item label="姓名">
          <el-input v-model="editForm.fullName" />
        </el-form-item>
        <el-form-item label="角色">
          <el-select v-model="editForm.role" :disabled="!canChangeRole">
            <el-option label="普通用户" value="user" />
            <el-option label="高级用户" value="premium" />
            <el-option label="管理员" value="admin" />
            <el-option label="超级管理员" value="superadmin" :disabled="editForm.role === 'superadmin' && editForm.id === authStore.user?.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="状态">
          <el-switch v-model="editForm.isActive" active-text="启用" inactive-text="停用" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="editDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleSaveUser" :loading="saving">保存</el-button>
      </template>
    </el-dialog>

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

    <!-- 用户详情抽屉 -->
    <el-drawer v-model="userDetailVisible" :title="detailUser?.username || '用户详情'" size="900px">
      <el-tabs v-model="userHistoryTab">
        <el-tab-pane label="基本信息" name="info">
          <el-descriptions :column="1" border v-if="detailUser">
            <el-descriptions-item label="ID">{{ detailUser.id }}</el-descriptions-item>
            <el-descriptions-item label="用户名">{{ detailUser.username }}</el-descriptions-item>
            <el-descriptions-item label="邮箱">{{ detailUser.email }}</el-descriptions-item>
            <el-descriptions-item label="姓名">{{ detailUser.fullName }}</el-descriptions-item>
            <el-descriptions-item label="角色">
              <el-tag :type="getRoleBadgeType(detailUser.role)">{{ getRoleLabel(detailUser.role) }}</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="状态">
              <el-tag :type="detailUser.isActive ? 'success' : 'danger'">{{ detailUser.isActive ? '启用' : '停用' }}</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="活跃度">
              <el-tag :type="getActivityTagType(detailUser.activityStatus)">{{ getActivityLabel(detailUser.activityStatus) }}</el-tag>
            </el-descriptions-item>
            <el-descriptions-item label="登录次数">{{ detailUser.loginCount }}</el-descriptions-item>
            <el-descriptions-item label="注册时间">{{ formatDateTime(detailUser.createdAt) }}</el-descriptions-item>
            <el-descriptions-item label="最后登录">{{ detailUser.lastLoginAt ? formatDateTime(detailUser.lastLoginAt) : '未登录' }}</el-descriptions-item>
          </el-descriptions>
        </el-tab-pane>
        <el-tab-pane label="登录历史" name="history">
          <el-table :data="loginHistory" stripe size="small" style="width: 100%">
            <el-table-column prop="loginTime" label="时间" />
            <el-table-column prop="ipAddress" label="IP" />
            <el-table-column prop="success" label="状态" width="80">
              <template #default="{ row }">
                <el-tag :type="row.success ? 'success' : 'danger'" size="small">
                  {{ row.success ? '成功' : '失败' }}
                </el-tag>
              </template>
            </el-table-column>
          </el-table>
        </el-tab-pane>
        <el-tab-pane label="在线会话" name="sessions">
          <el-table :data="userSessions" stripe size="small" style="width: 100%">
            <el-table-column prop="id" label="会话ID" />
            <el-table-column prop="ip" label="IP" />
            <el-table-column label="操作" width="100">
              <template #default="{ row }">
                <el-button type="danger" size="small" @click="handleKickSession(row.id)">踢出</el-button>
              </template>
            </el-table-column>
          </el-table>
          <el-empty v-if="!userSessions.length" description="暂无在线会话" />
        </el-tab-pane>
      </el-tabs>
    </el-drawer>

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
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores'
import adminApi, { type ModuleInfo } from '@/api/modules/admin'
import type { FrontendAdminUser as AdminUser, FrontendAuditLog as AuditLog, UserRole } from '@/api/adapters/adminAdapter'
import type { UploadFile } from 'element-plus'
import {
  Setting,
  User,
  Search,
  Refresh,
  CircleCheck,
  Star,
  Memo,
  Upload,
  Plus,
  Download
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Line as LineChart } from 'vue-chartjs'
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler
} from 'chart.js'

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend, Filler)

const router = useRouter()
const authStore = useAuthStore()

// 检查权限
if (!authStore.isAdminOrSuper) {
  ElMessage.error('您没有访问管理控制台的权限')
  router.push('/dashboard')
}

// 状态
const loading = ref(false)
const activeTab = ref('overview')
const searchQuery = ref('')
const roleFilter = ref('')

// Dashboard
const dashboardData = ref<any>(null)
const dashboardLoading = ref(false)

// 统计数据
const stats = ref({
  totalUsers: 0,
  activeUsers: 0,
  premiumUsers: 0,
  adminUsers: 0,
  totalPapers: 0,
  totalSearches: 0,
  recentlyActiveUsers: 0,
  enabledModules: 0,
  totalModules: 0
})

// 用户数据
const users = ref<AdminUser[]>([])
const pagination = ref({
  page: 1,
  limit: 20,
  total: 0
})

// 模块数据
const modules = ref<ModuleInfo[]>([])

// 模块上传
const uploadDialogVisible = ref(false)
const uploadRef = ref()
const uploading = ref(false)
const uploadForm = ref({
  file: null as File | null,
  moduleName: ''
})

// 审计日志
const auditLogs = ref<AuditLog[]>([])
const auditPagination = ref({
  page: 1,
  limit: 20,
  total: 0
})

// 创建用户
const createDialogVisible = ref(false)
const createFormRef = ref()
const createForm = ref({
  username: '',
  email: '',
  fullName: '',
  role: 'user' as UserRole,
  password: ''
})
const createRules = {
  username: [{ required: true, message: '请输入用户名', trigger: 'blur' }],
  email: [
    { required: true, message: '请输入邮箱', trigger: 'blur' },
    { type: 'email', message: '请输入正确的邮箱格式', trigger: 'blur' }
  ]
}

// 编辑用户
const editDialogVisible = ref(false)
const editForm = ref<AdminUser>({
  id: 0,
  username: '',
  email: '',
  fullName: '',
  role: 'user',
  isActive: true,
  createdAt: '',
  lastLoginAt: '',
  loginCount: 0,
  activityStatus: 'inactive'
})
const saving = ref(false)

// User detail drawer
const userDetailVisible = ref(false)
const detailUser = ref<AdminUser | null>(null)
const userHistoryTab = ref('info')
const loginHistory = ref<any[]>([])
const userSessions = ref<any[]>([])
const historyPagination = ref({ page: 1, limit: 10, total: 0 })

// Announcements
const announcements = ref<any[]>([])
const announcementPagination = ref({ page: 1, limit: 20, total: 0 })
const announcementDialogVisible = ref(false)
const announcementForm = ref({
  id: 0,
  title: '',
  content: '',
  type: 'info' as string,
  targetRole: 'all' as string,
  expiresAt: ''
})
const isEditingAnnouncement = ref(false)

// System Monitoring
const monitoringSubTab = ref('resources')
const metricsLoading = ref(false)
const systemMetrics = ref<any>({
  cpu_percent: 0,
  memory_used_mb: 0,
  memory_total_mb: 0,
  memory_percent: 0,
  disk_used_gb: 0,
  disk_total_gb: 0,
  disk_percent: 0,
  network_rx_mbps: 0,
  network_tx_mbps: 0,
  uptime_seconds: 0,
  active_connections: 0
})
const serviceHealthList = ref<any[]>([])
const systemLogs = ref<any[]>([])
const logLevelFilter = ref('')
const logModuleFilter = ref('')
const logPagination = ref({ page: 1, limit: 50, total: 0 })
const slowQueries = ref<any[]>([])
const bottlenecks = ref<any[]>([])

// Login Security
const securitySubTab = ref('history')
const securityLoading = ref(false)
const loginHistoryUsernameFilter = ref('')
const allLoginHistory = ref<any[]>([])
const loginHistoryPagination = ref({ page: 1, limit: 20, total: 0 })
const suspiciousStatusFilter = ref('')
const suspiciousLogins = ref<any[]>([])
const suspiciousPagination = ref({ page: 1, limit: 20, total: 0 })
const ipBlacklist = ref<any[]>([])
const ipBlacklistPagination = ref({ page: 1, limit: 20, total: 0 })
const addIpBlacklistDialogVisible = ref(false)
const addIpBlacklistForm = ref({
  ipAddress: '',
  reason: '',
  threatLevel: 'medium',
  expiresAt: ''
})
const accountLockouts = ref<any[]>([])
const accountLockoutsPagination = ref({ page: 1, limit: 20, total: 0 })
const lockUserDialogVisible = ref(false)
const lockUserForm = ref({
  userId: 0,
  lockMinutes: 30,
  reason: ''
})

// Global Configuration
const configSubTab = ref('management')
const configLoading = ref(false)
const configCategoryFilter = ref('')
const configs = ref<any[]>([])
const selectedConfig = ref<any>(null)
const configHistory = ref<any[]>([])
const configHistoryKeyFilter = ref('')
const configHistoryPagination = ref({ page: 1, limit: 20, total: 0 })
const editConfigDialogVisible = ref(false)
const editConfigForm = ref({
  key: '',
  value: '',
  reason: ''
})

// Data Backup
const backupSubTab = ref('jobs')
const backupLoading = ref(false)
const backupJobs = ref<any[]>([])
const backupRecords = ref<any[]>([])
const backupRecordsPagination = ref({ page: 1, limit: 20, total: 0 })
const backupRecordsJobFilter = ref(0)
const createBackupJobDialogVisible = ref(false)
const createBackupJobForm = ref({
  name: '',
  jobType: 'full',
  scheduleCron: '',
  backupPath: '/backups',
  retentionDays: 30
})

// RBAC权限管理
const permissionsSubTab = ref('matrix')
const permissionsLoading = ref(false)
const permissionMatrix = ref<any[]>([])
const roles = ref<any[]>([])
const rolesLoading = ref(false)
const userRolesLoading = ref(false)
const userRoleSearchUserId = ref('')
const userRoles = ref<any[]>([])

// 通知管理
const notificationsSubTab = ref('system')
const notificationsLoading = ref(false)
const systemNotifications = ref<any[]>([])
const notificationsPagination = ref({ page: 1, limit: 20, total: 0 })
const notificationTemplatesLoading = ref(false)
const notificationTemplates = ref<any[]>([])

// 数据清理
const cleanupSubTab = ref('tasks')
const cleanupTasksLoading = ref(false)
const cleanupTasks = ref<any[]>([])
const cleanupHistoryLoading = ref(false)
const cleanupHistory = ref<any[]>([])
const cleanupHistoryPagination = ref({ page: 1, limit: 20, total: 0 })
const storageStatsLoading = ref(false)
const storageStats = ref<any[]>([])

// 内容审核
const moderationSubTab = ref('pending')
const pendingPapersLoading = ref(false)
const pendingPapers = ref<any[]>([])
const pendingPapersPagination = ref({ page: 1, limit: 20, total: 0 })
const userReportsLoading = ref(false)
const userReports = ref<any[]>([])
const userReportsPagination = ref({ page: 1, limit: 20, total: 0 })
const userReportsStatusFilter = ref('')
const sensitiveWordsLoading = ref(false)
const sensitiveWords = ref<any[]>([])

// API密钥管理
const apiKeysSubTab = ref('list')
const apiKeysLoading = ref(false)
const apiKeys = ref<any[]>([])
const apiKeysPagination = ref({ page: 1, limit: 20, total: 0 })
const apiKeyUsageLoading = ref(false)
const apiKeyUsage = ref<any[]>([])
const apiKeyStats = ref<any>(null)

// 计算属性
const canChangeRole = computed(() => {
  return authStore.isSuperAdmin
})

const userTrendChartData = computed(() => {
  if (!dashboardData.value?.userTrend?.length) return null
  return {
    labels: dashboardData.value.userTrend.map((t: any) => t.date),
    datasets: [{
      label: '新增用户',
      data: dashboardData.value.userTrend.map((t: any) => t.count),
      borderColor: '#409eff',
      backgroundColor: 'rgba(64,158,255,0.1)',
      fill: true,
      tension: 0.4
    }]
  }
})

const chartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  plugins: {
    legend: { display: true, position: 'top' as const }
  },
  scales: {
    y: { beginAtZero: true, ticks: { stepSize: 1 } }
  }
}

// 方法
async function loadDashboard() {
  dashboardLoading.value = true
  try {
    const data = await adminApi.getDashboard()
    dashboardData.value = data
    stats.value = data.stats
  } catch (error: any) {
    console.error('Failed to load dashboard:', error)
  } finally {
    dashboardLoading.value = false
  }
}

async function loadUsers() {
  loading.value = true
  try {
    const response = await adminApi.getUsers({
      page: pagination.value.page,
      limit: pagination.value.limit,
      search: searchQuery.value,
      role: roleFilter.value as UserRole
    })
    console.log('[loadUsers] API response:', response)
    console.log('[loadUsers] response.items:', response.items)
    users.value = response.items || []
    pagination.value.total = response.total || 0
    console.log('[loadUsers] users.value:', users.value.length, 'users')
  } catch (error: any) {
    console.error('[loadUsers] Error:', error)
    ElMessage.error('加载用户列表失败: ' + (error.message || '未知错误'))
    users.value = []
  } finally {
    loading.value = false
  }
}

async function loadModules() {
  loading.value = true
  try {
    const result = await adminApi.getModules()
    console.log('[loadModules] API returned:', result)
    console.log('[loadModules] result type:', Array.isArray(result) ? 'Array' : typeof result)
    modules.value = result || []
    console.log('[loadModules] modules.value:', modules.value)
  } catch (error: any) {
    console.error('[loadModules] API call failed:', error)
    ElMessage.error('加载模块列表失败: ' + (error.message || '未知错误'))
    modules.value = []
  } finally {
    loading.value = false
  }
}

async function loadAuditLogs() {
  loading.value = true
  try {
    const response = await adminApi.getAuditLogs({
      page: auditPagination.value.page,
      limit: auditPagination.value.limit
    })
    auditLogs.value = response.items
    auditPagination.value.total = response.total
  } catch (error: any) {
    ElMessage.error('加载审计日志失败: ' + (error.message || '未知错误'))
  } finally {
    loading.value = false
  }
}

function handleSearch() {
  pagination.value.page = 1
  loadUsers()
}

function handlePageChange(page: number) {
  pagination.value.page = page
  loadUsers()
}

function handleSizeChange(size: number) {
  pagination.value.limit = size
  pagination.value.page = 1
  loadUsers()
}

function handleAuditPageChange(page: number) {
  auditPagination.value.page = page
  loadAuditLogs()
}

function handleAuditSizeChange(size: number) {
  auditPagination.value.limit = size
  auditPagination.value.page = 1
  loadAuditLogs()
}

function handleEditUser(user: AdminUser) {
  editForm.value = { ...user }
  editDialogVisible.value = true
}

function handleCreateUser() {
  createForm.value = {
    username: '',
    email: '',
    fullName: '',
    role: 'user',
    password: ''
  }
  createDialogVisible.value = true
}

async function handleConfirmCreate() {
  if (!createFormRef.value) return

  try {
    await createFormRef.value.validate()
  } catch {
    return
  }

  saving.value = true
  try {
    await adminApi.createUser(createForm.value)
    ElMessage.success('用户创建成功')
    createDialogVisible.value = false
    loadUsers()
    loadDashboard()
  } catch (error: any) {
    ElMessage.error('创建用户失败: ' + (error.message || '未知错误'))
  } finally {
    saving.value = false
  }
}

async function handleSaveUser() {
  saving.value = true
  try {
    await adminApi.updateUser(editForm.value.id, {
      fullName: editForm.value.fullName,
      role: editForm.value.role,
      isActive: editForm.value.isActive
    })
    ElMessage.success('用户更新成功')
    editDialogVisible.value = false
    loadUsers()
    loadDashboard()
  } catch (error: any) {
    ElMessage.error('更新用户失败: ' + (error.message || '未知错误'))
  } finally {
    saving.value = false
  }
}

async function handleToggleUserStatus(user: AdminUser) {
  const action = user.isActive ? '停用' : '启用'
  try {
    await ElMessageBox.confirm(
      `确定要${action}用户 "${user.username}" 吗？`,
      '确认操作',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }
    )

    if (user.isActive) {
      await adminApi.deactivateUser(user.id)
    } else {
      await adminApi.activateUser(user.id)
    }

    ElMessage.success(`用户${action}成功`)
    loadUsers()
    loadDashboard()
  } catch (error: any) {
    if (error !== 'cancel') {
      ElMessage.error(`${action}用户失败: ` + (error.message || '未知错误'))
    }
  }
}

// User detail
async function openUserDetail(user: AdminUser) {
  detailUser.value = user
  userDetailVisible.value = true
  userHistoryTab.value = 'info'
  loadUserHistory(user.id)
  loadUserSessions(user.id)
}

async function loadUserHistory(userId: number) {
  try {
    const response = await adminApi.getUserHistory(userId, {
      page: historyPagination.value.page,
      limit: historyPagination.value.limit
    })
    loginHistory.value = response.items
    historyPagination.value.total = response.total
  } catch (error: any) {
    console.error('Failed to load user history:', error)
  }
}

async function loadUserSessions(userId: number) {
  try {
    const sessions = await adminApi.getUserSessions(userId)
    userSessions.value = sessions
  } catch (error: any) {
    console.error('Failed to load sessions:', error)
  }
}

async function handleKickSession(sessionId: string) {
  if (!detailUser.value) return
  try {
    await adminApi.kickUserSession(detailUser.value.id, sessionId)
    ElMessage.success('已踢出会话')
    loadUserSessions(detailUser.value.id)
  } catch (error: any) {
    ElMessage.error('踢出失败: ' + (error.message || ''))
  }
}

// Announcements
async function loadAnnouncements() {
  loading.value = true
  try {
    const response = await adminApi.getAnnouncements({
      page: announcementPagination.value.page,
      limit: announcementPagination.value.limit
    })
    announcements.value = response.items
    announcementPagination.value.total = response.total
  } catch (error: any) {
    ElMessage.error('加载公告失败: ' + (error.message || ''))
  } finally {
    loading.value = false
  }
}

function handleCreateAnnouncement() {
  isEditingAnnouncement.value = false
  announcementForm.value = {
    id: 0,
    title: '',
    content: '',
    type: 'info',
    targetRole: 'all',
    expiresAt: ''
  }
  announcementDialogVisible.value = true
}

function handleEditAnnouncement(announcement: any) {
  isEditingAnnouncement.value = true
  announcementForm.value = { ...announcement }
  announcementDialogVisible.value = true
}

async function handleSaveAnnouncement() {
  try {
    if (isEditingAnnouncement.value) {
      await adminApi.updateAnnouncement(announcementForm.value.id, announcementForm.value)
    } else {
      await adminApi.createAnnouncement(announcementForm.value)
    }
    ElMessage.success(isEditingAnnouncement.value ? '公告已更新' : '公告已创建')
    announcementDialogVisible.value = false
    loadAnnouncements()
  } catch (error: any) {
    ElMessage.error('操作失败: ' + (error.message || ''))
  }
}

async function handleDeleteAnnouncement(id: number) {
  try {
    await ElMessageBox.confirm('确定要删除此公告吗？', '确认', { type: 'warning' })
    await adminApi.deleteAnnouncement(id)
    ElMessage.success('公告已删除')
    loadAnnouncements()
  } catch (error: any) {
    if (error !== 'cancel') ElMessage.error('删除失败: ' + (error.message || ''))
  }
}

async function handleToggleAnnouncement(id: number) {
  try {
    await adminApi.toggleAnnouncement(id)
    ElMessage.success('状态已切换')
    loadAnnouncements()
  } catch (error: any) {
    ElMessage.error('操作失败: ' + (error.message || ''))
  }
}

// Export
async function handleExportUsers() {
  try {
    const blob = await adminApi.exportUsersCsv({
      search: searchQuery.value,
      role: roleFilter.value
    })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `users_export_${new Date().toISOString().slice(0,10)}.csv`
    a.click()
    URL.revokeObjectURL(url)
    ElMessage.success('导出成功')
  } catch (error: any) {
    ElMessage.error('导出失败: ' + (error.message || ''))
  }
}

function getRoleLabel(role: UserRole): string {
  return adminApi.getRoleLabel(role, 'zh')
}

function getRoleBadgeType(role: UserRole): 'success' | 'warning' | 'info' | 'primary' | 'danger' {
  const types: Record<string, 'success' | 'warning' | 'info' | 'primary' | 'danger'> = {
    user: 'info',
    premium: 'warning',
    admin: 'danger',
    superadmin: 'danger'
  }
  return types[role] || 'info'
}

function canManageRole(currentRole: UserRole, targetRole: UserRole): boolean {
  const roleHierarchy: Record<UserRole, number> = {
    user: 1,
    premium: 2,
    admin: 3,
    superadmin: 4
  }
  return roleHierarchy[currentRole] > roleHierarchy[targetRole]
}

function getActivityTagType(status: string): 'success' | 'warning' | 'info' | 'primary' | 'danger' {
  const types: Record<string, 'success' | 'warning' | 'info' | 'primary' | 'danger'> = {
    active: 'success',
    idle: 'warning',
    inactive: 'info'
  }
  return types[status] || 'info'
}

function getActivityLabel(status: string): string {
  const labels: Record<string, string> = {
    active: '活跃',
    idle: '闲置',
    inactive: '不活跃'
  }
  return labels[status] || '不活跃'
}

function formatDateTime(timestamp: number | string): string {
  const num = typeof timestamp === 'string' ? parseInt(timestamp) : timestamp
  if (!num || num === 0) return ''
  const date = new Date(num * 1000)
  return date.toLocaleString('zh-CN')
}

// 模块管理方法
function canUninstallModule(moduleName: string): boolean {
  const coreModules = ['AuthApiModule', 'AdminApiModule', 'UserApiModule', 'DatabaseModule']
  return !coreModules.includes(moduleName)
}

async function handleToggleModule(module: ModuleInfo, enable: boolean) {
  try {
    if (enable) {
      await adminApi.enableModule({ moduleName: module.name })
      ElMessage.success(`模块 ${module.name} 已启用`)
    } else {
      await adminApi.disableModule({ moduleName: module.name })
      ElMessage.success(`模块 ${module.name} 已禁用`)
    }
    await loadModules()
    await loadDashboard()
  } catch (error: any) {
    ElMessage.error('操作失败: ' + (error.message || '未知错误'))
  }
}

async function handleReloadModule(module: ModuleInfo) {
  try {
    await adminApi.reloadModule(module.name)
    ElMessage.success(`模块 ${module.name} 重载成功`)
    await loadModules()
  } catch (error: any) {
    ElMessage.error('重载失败: ' + (error.message || '未知错误'))
  }
}

async function handleUninstallModule(module: ModuleInfo) {
  try {
    await ElMessageBox.confirm(
      `确定要卸载模块 "${module.name}" 吗？此操作不可恢复。`,
      '确认卸载',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }
    )

    await adminApi.uninstallModule(module.name)
    ElMessage.success(`模块 ${module.name} 已卸载`)
    await loadModules()
    await loadDashboard()
  } catch (error: any) {
    if (error !== 'cancel') {
      ElMessage.error('卸载失败: ' + (error.message || '未知错误'))
    }
  }
}

async function handleScanModules() {
  try {
    loading.value = true
    const scannedModules = await adminApi.scanModules()
    ElMessage.success(`扫描到 ${scannedModules.length} 个模块`)
  } catch (error: any) {
    ElMessage.error('扫描失败: ' + (error.message || '未知错误'))
  } finally {
    loading.value = false
  }
}

function handleUploadModule() {
  uploadForm.value = {
    file: null,
    moduleName: ''
  }
  uploadDialogVisible.value = true
}

function handleFileChange(file: UploadFile) {
  uploadForm.value.file = file.raw as File
  if (file.name) {
    const nameWithoutExt = file.name.replace(/\.(dll|so|dylib)$/i, '')
    uploadForm.value.moduleName = nameWithoutExt
  }
}

async function handleConfirmUpload() {
  if (!uploadForm.value.file) {
    ElMessage.warning('请选择文件')
    return
  }

  if (!uploadForm.value.moduleName) {
    ElMessage.warning('请输入模块名称')
    return
  }

  uploading.value = true
  try {
    const fileReader = new FileReader()
    fileReader.onload = async (e) => {
      const base64 = (e.target?.result as string).split(',')[1]

      const uploadResult = await adminApi.uploadModule({
        fileData: base64,
        filename: uploadForm.value.file!.name
      })

      await adminApi.installModule({
        moduleName: uploadForm.value.moduleName,
        modulePath: uploadResult.path
      })

      ElMessage.success('模块上传并安装成功')
      uploadDialogVisible.value = false
      await loadModules()
      await loadDashboard()
    }
    fileReader.readAsDataURL(uploadForm.value.file)
  } catch (error: any) {
    ElMessage.error('上传失败: ' + (error.message || '未知错误'))
  } finally {
    uploading.value = false
  }
}

// System Monitoring Functions
async function loadSystemMetrics() {
  metricsLoading.value = true
  try {
    const response = await adminApi.getSystemMetrics()
    if (response.success) {
      systemMetrics.value = response.data
    }
  } catch (error: any) {
    ElMessage.error('获取系统指标失败: ' + (error.message || '未知错误'))
  } finally {
    metricsLoading.value = false
  }
}

async function loadServiceHealth() {
  metricsLoading.value = true
  try {
    const response = await adminApi.getServiceHealth()
    if (response.success) {
      serviceHealthList.value = response.services || []
    }
  } catch (error: any) {
    ElMessage.error('获取服务健康状态失败: ' + (error.message || '未知错误'))
  } finally {
    metricsLoading.value = false
  }
}

async function loadSystemLogs() {
  metricsLoading.value = true
  try {
    const params: any = {
      page: logPagination.value.page,
      pageSize: logPagination.value.limit
    }
    if (logLevelFilter.value) params.level = logLevelFilter.value
    if (logModuleFilter.value) params.module = logModuleFilter.value

    const response = await adminApi.getSystemLogs(params)
    if (response.success) {
      systemLogs.value = response.logs || []
      logPagination.value.total = response.total || 0
    }
  } catch (error: any) {
    ElMessage.error('获取系统日志失败: ' + (error.message || '未知错误'))
  } finally {
    metricsLoading.value = false
  }
}

async function handleCleanLogs() {
  try {
    await ElMessageBox.confirm(
      '确定要清理旧日志吗？此操作不可恢复。',
      '确认清理',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning'
      }
    )

    const thirtyDaysAgo = new Date()
    thirtyDaysAgo.setDate(thirtyDaysAgo.getDate() - 30)
    const dateStr = thirtyDaysAgo.toISOString().split('T')[0]

    const response = await adminApi.cleanLogs({ date: dateStr })
    if (response.success) {
      ElMessage.success('日志清理成功')
      loadSystemLogs()
    }
  } catch (error: any) {
    if (error !== 'cancel') {
      ElMessage.error('清理失败: ' + (error.message || '未知错误'))
    }
  }
}

async function loadPerformanceData() {
  metricsLoading.value = true
  try {
    const [slowQueryRes, bottleneckRes] = await Promise.all([
      adminApi.getSlowQueries({ limit: 10 }),
      adminApi.getPerformanceBottlenecks()
    ])

    if (slowQueryRes.success) {
      slowQueries.value = slowQueryRes.data.queries || []
    }

    if (bottleneckRes.success) {
      bottlenecks.value = bottleneckRes.data.bottlenecks || []
    }
  } catch (error: any) {
    ElMessage.error('获取性能数据失败: ' + (error.message || '未知错误'))
  } finally {
    metricsLoading.value = false
  }
}

function getProgressColor(percent: number): string {
  if (percent >= 90) return '#f56c6c'
  if (percent >= 70) return '#e6a23c'
  return '#67c23a'
}

function formatBytes(bytes: number): string {
  if (bytes < 1024) return bytes + ' B'
  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB'
  if (bytes < 1024 * 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(1) + ' MB'
  return (bytes / (1024 * 1024 * 1024)).toFixed(1) + ' GB'
}

function formatUptime(seconds: number): string {
  if (seconds < 60) return seconds + ' 秒'
  if (seconds < 3600) return Math.floor(seconds / 60) + ' 分钟'
  if (seconds < 86400) return Math.floor(seconds / 3600) + ' 小时'
  const days = Math.floor(seconds / 86400)
  const hours = Math.floor((seconds % 86400) / 3600)
  return `${days} 天 ${hours} 小时`
}

function getLogLevelTagType(level: string): string {
  const types: Record<string, string> = {
    debug: 'info',
    info: 'success',
    warning: 'warning',
    error: 'danger',
    critical: 'danger'
  }
  return types[level] || 'info'
}

// Login Security Functions
async function loadLoginHistory() {
  securityLoading.value = true
  try {
    const response = await adminApi.getLoginHistory({
      page: loginHistoryPagination.value.page,
      limit: loginHistoryPagination.value.limit,
      username: loginHistoryUsernameFilter.value
    })
    if (response.success) {
      allLoginHistory.value = response.attempts || []
      loginHistoryPagination.value.total = response.total || 0
    }
  } catch (error: any) {
    ElMessage.error('获取登录历史失败: ' + (error.message || '未知错误'))
  } finally {
    securityLoading.value = false
  }
}

async function loadSuspiciousLogins() {
  securityLoading.value = true
  try {
    const response = await adminApi.getSuspiciousLogins({
      page: suspiciousPagination.value.page,
      limit: suspiciousPagination.value.limit,
      status: suspiciousStatusFilter.value
    })
    if (response.success) {
      suspiciousLogins.value = response.suspicious || []
      suspiciousPagination.value.total = response.total || 0
    }
  } catch (error: any) {
    ElMessage.error('获取可疑登录失败: ' + (error.message || '未知错误'))
  } finally {
    securityLoading.value = false
  }
}

async function loadIpBlacklist() {
  securityLoading.value = true
  try {
    const response = await adminApi.getIpBlacklist({
      page: ipBlacklistPagination.value.page,
      limit: ipBlacklistPagination.value.limit
    })
    if (response.success) {
      ipBlacklist.value = response.blacklist || []
      ipBlacklistPagination.value.total = response.total || 0
    }
  } catch (error: any) {
    ElMessage.error('获取IP黑名单失败: ' + (error.message || '未知错误'))
  } finally {
    securityLoading.value = false
  }
}

function showAddIpBlacklistDialog() {
  addIpBlacklistForm.value = {
    ipAddress: '',
    reason: '',
    threatLevel: 'medium',
    expiresAt: ''
  }
  addIpBlacklistDialogVisible.value = true
}

async function handleAddIpBlacklist() {
  if (!addIpBlacklistForm.value.ipAddress || !addIpBlacklistForm.value.reason) {
    ElMessage.warning('请填写IP地址和原因')
    return
  }

  securityLoading.value = true
  try {
    const response = await adminApi.addIpBlacklist(addIpBlacklistForm.value)
    if (response.message) {
      ElMessage.success('IP黑名单添加成功')
      addIpBlacklistDialogVisible.value = false
      loadIpBlacklist()
    }
  } catch (error: any) {
    ElMessage.error('添加IP黑名单失败: ' + (error.message || '未知错误'))
  } finally {
    securityLoading.value = false
  }
}

async function handleRemoveIpBlacklist(id: number) {
  try {
    await ElMessageBox.confirm('确定要移除此IP黑名单吗？', '确认移除', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    })

    securityLoading.value = true
    const response = await adminApi.removeIpBlacklist(id)
    if (response.message) {
      ElMessage.success('IP黑名单移除成功')
      loadIpBlacklist()
    }
  } catch (error: any) {
    if (error !== 'cancel') {
      ElMessage.error('移除IP黑名单失败: ' + (error.message || '未知错误'))
    }
  } finally {
    securityLoading.value = false
  }
}

async function loadAccountLockouts() {
  securityLoading.value = true
  try {
    const response = await adminApi.getAccountLockouts({
      page: accountLockoutsPagination.value.page,
      limit: accountLockoutsPagination.value.limit
    })
    if (response.success) {
      accountLockouts.value = response.lockouts || []
      accountLockoutsPagination.value.total = response.total || 0
    }
  } catch (error: any) {
    ElMessage.error('获取账户锁定失败: ' + (error.message || '未知错误'))
  } finally {
    securityLoading.value = false
  }
}

function showLockUserDialog() {
  lockUserForm.value = {
    userId: 0,
    lockMinutes: 30,
    reason: ''
  }
  lockUserDialogVisible.value = true
}

async function handleLockUserAccountConfirm() {
  if (lockUserForm.value.userId === 0) {
    ElMessage.warning('请输入用户ID')
    return
  }

  securityLoading.value = true
  try {
    const response = await adminApi.lockUserAccount(lockUserForm.value)
    if (response.message) {
      ElMessage.success('用户账户锁定成功')
      lockUserDialogVisible.value = false
      loadAccountLockouts()
    }
  } catch (error: any) {
    ElMessage.error('锁定用户账户失败: ' + (error.message || '未知错误'))
  } finally {
    securityLoading.value = false
  }
}

async function handleUnlockUserAccount(userId: number) {
  try {
    await ElMessageBox.confirm('确定要解锁此用户账户吗？', '确认解锁', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    })

    securityLoading.value = true
    const response = await adminApi.unlockUserAccount(userId)
    if (response.message) {
      ElMessage.success('用户账户解锁成功')
      loadAccountLockouts()
    }
  } catch (error: any) {
    if (error !== 'cancel') {
      ElMessage.error('解锁用户账户失败: ' + (error.message || '未知错误'))
    }
  } finally {
    securityLoading.value = false
  }
}

async function handleSuspiciousLoginAction(id: number, action: string) {
  securityLoading.value = true
  try {
    const response = await adminApi.handleSuspiciousLogin(id, action)
    if (response.message) {
      ElMessage.success('操作成功')
      loadSuspiciousLogins()
    }
  } catch (error: any) {
    ElMessage.error('操作失败: ' + (error.message || '未知错误'))
  } finally {
    securityLoading.value = false
  }
}

function getSuspiciousStatusLabel(status: string): string {
  const labels: Record<string, string> = {
    pending: '待处理',
    reviewed: '已审核',
    whitelisted: '已白名单',
    confirmed_threat: '确认威胁'
  }
  return labels[status] || status
}

function getSuspiciousStatusTagType(status: string): string {
  const types: Record<string, string> = {
    pending: 'warning',
    reviewed: 'info',
    whitelisted: 'success',
    confirmed_threat: 'danger'
  }
  return types[status] || 'info'
}

function getThreatLevelLabel(level: string): string {
  const labels: Record<string, string> = {
    low: '低',
    medium: '中',
    high: '高',
    critical: '严重'
  }
  return labels[level] || level
}

function getThreatLevelTagType(level: string): string {
  const types: Record<string, string> = {
    low: 'info',
    medium: 'warning',
    high: 'danger',
    critical: 'danger'
  }
  return types[level] || 'info'
}

// Watch tab changes to load data on demand
watch(activeTab, (tab) => {
  if (tab === 'announcements' && authStore.isSuperAdmin) {
    loadAnnouncements()
  } else if (tab === 'monitoring' && authStore.isSuperAdmin) {
    loadSystemMetrics()
    loadServiceHealth()
  } else if (tab === 'security' && authStore.isSuperAdmin) {
    loadLoginHistory()
  }
})

watch(monitoringSubTab, (tab) => {
  if (tab === 'logs' && authStore.isSuperAdmin) {
    loadSystemLogs()
  } else if (tab === 'performance' && authStore.isSuperAdmin) {
    loadPerformanceData()
  }
})

watch(securitySubTab, (tab) => {
  if (tab === 'history' && authStore.isSuperAdmin) {
    loadLoginHistory()
  } else if (tab === 'suspicious' && authStore.isSuperAdmin) {
    loadSuspiciousLogins()
  } else if (tab === 'blacklist' && authStore.isSuperAdmin) {
    loadIpBlacklist()
  } else if (tab === 'lockouts' && authStore.isSuperAdmin) {
    loadAccountLockouts()
  }
})

// Global Configuration Functions
async function loadConfigs() {
  configLoading.value = true
  try {
    const response = await adminApi.getConfigs({ category: configCategoryFilter.value })
    if (response.success) {
      configs.value = response.configs || []
    }
  } catch (error: any) {
    ElMessage.error('获取配置失败: ' + (error.message || '未知错误'))
  } finally {
    configLoading.value = false
  }
}

function handleConfigSelectionChange(selection: any[]) {
  selectedConfig.value = selection.length > 0 ? selection[0] : null
}

function showEditConfigDialog() {
  if (!selectedConfig.value) {
    ElMessage.warning('请先选择要编辑的配置')
    return
  }
  editConfigForm.value = {
    key: selectedConfig.value.key,
    value: selectedConfig.value.value,
    reason: ''
  }
  editConfigDialogVisible.value = true
}

async function handleUpdateConfig() {
  configLoading.value = true
  try {
    const response = await adminApi.updateConfig(editConfigForm.value)
    if (response.message) {
      ElMessage.success('配置更新成功')
      editConfigDialogVisible.value = false
      loadConfigs()
    }
  } catch (error: any) {
    ElMessage.error('更新配置失败: ' + (error.message || '未知错误'))
  } finally {
    configLoading.value = false
  }
}

async function loadConfigHistory() {
  configLoading.value = true
  try {
    const response = await adminApi.getConfigHistory({
      page: configHistoryPagination.value.page,
      limit: configHistoryPagination.value.limit,
      key: configHistoryKeyFilter.value
    })
    if (response.success) {
      configHistory.value = response.history || []
      configHistoryPagination.value.total = response.total || 0
    }
  } catch (error: any) {
    ElMessage.error('获取配置历史失败: ' + (error.message || '未知错误'))
  } finally {
    configLoading.value = false
  }
}

function getConfigChangeTypeTag(type: string): string {
  const types: Record<string, string> = {
    create: 'success',
    update: 'warning',
    delete: 'danger'
  }
  return types[type] || 'info'
}

// Data Backup Functions
async function loadBackupJobs() {
  backupLoading.value = true
  try {
    const response = await adminApi.getBackupJobs()
    if (response.success) {
      backupJobs.value = response.jobs || []
    }
  } catch (error: any) {
    ElMessage.error('获取备份任务失败: ' + (error.message || '未知错误'))
  } finally {
    backupLoading.value = false
  }
}

async function loadBackupRecords() {
  backupLoading.value = true
  try {
    const response = await adminApi.getBackupRecords({
      page: backupRecordsPagination.value.page,
      limit: backupRecordsPagination.value.limit,
      jobId: backupRecordsJobFilter.value
    })
    if (response.success) {
      backupRecords.value = response.records || []
      backupRecordsPagination.value.total = response.total || 0
    }
  } catch (error: any) {
    ElMessage.error('获取备份记录失败: ' + (error.message || '未知错误'))
  } finally {
    backupLoading.value = false
  }
}

function showCreateBackupJobDialog() {
  createBackupJobForm.value = {
    name: '',
    jobType: 'full',
    scheduleCron: '',
    backupPath: '/backups',
    retentionDays: 30
  }
  createBackupJobDialogVisible.value = true
}

async function handleCreateBackupJob() {
  if (!createBackupJobForm.value.name) {
    ElMessage.warning('请输入任务名称')
    return
  }

  backupLoading.value = true
  try {
    const response = await adminApi.createBackupJob(createBackupJobForm.value)
    if (response.message) {
      ElMessage.success('备份任务创建成功')
      createBackupJobDialogVisible.value = false
      loadBackupJobs()
    }
  } catch (error: any) {
    ElMessage.error('创建备份任务失败: ' + (error.message || '未知错误'))
  } finally {
    backupLoading.value = false
  }
}

async function triggerBackupJob(jobId: number) {
  backupLoading.value = true
  try {
    const response = await adminApi.triggerBackup(jobId)
    if (response.success) {
      ElMessage.success('备份任务已触发')
      loadBackupRecords()
    }
  } catch (error: any) {
    ElMessage.error('触发备份失败: ' + (error.message || '未知错误'))
  } finally {
    backupLoading.value = false
  }
}

async function handleDeleteBackupJob(id: number) {
  try {
    await ElMessageBox.confirm('确定要删除此备份任务吗？', '确认删除', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    })

    backupLoading.value = true
    const response = await adminApi.deleteBackupJob(id)
    if (response.message) {
      ElMessage.success('备份任务删除成功')
      loadBackupJobs()
    }
  } catch (error: any) {
    if (error !== 'cancel') {
      ElMessage.error('删除备份任务失败: ' + (error.message || '未知错误'))
    }
  } finally {
    backupLoading.value = false
  }
}

async function handleDeleteBackupFile(id: number) {
  try {
    await ElMessageBox.confirm('确定要删除此备份文件吗？此操作不可恢复。', '确认删除', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    })

    backupLoading.value = true
    const response = await adminApi.deleteBackupFile(id)
    if (response.message) {
      ElMessage.success('备份文件删除成功')
      loadBackupRecords()
    }
  } catch (error: any) {
    if (error !== 'cancel') {
      ElMessage.error('删除备份文件失败: ' + (error.message || '未知错误'))
    }
  } finally {
    backupLoading.value = false
  }
}

function downloadBackup(id: number) {
  // TODO: 实现备份文件下载
  ElMessage.info('备份文件下载功能待实现')
}

function getBackupJobTypeLabel(type: string): string {
  const labels: Record<string, string> = {
    full: '完整',
    incremental: '增量',
    database_only: '仅数据库',
    files_only: '仅文件'
  }
  return labels[type] || type
}

function getBackupStatusTagType(status: string): string {
  const types: Record<string, string> = {
    success: 'success',
    failed: 'danger',
    in_progress: 'warning',
    pending: 'info',
    deleted: 'info'
  }
  return types[status] || 'info'
}

function formatFileSize(bytes: number): string {
  if (bytes === 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]
}

	// ============================================================================
	// RBAC权限管理方法
	// ============================================================================

	async function loadPermissionMatrix() {
	  permissionsLoading.value = true
	  try {
	    const data = await adminApi.getPermissionMatrix()
	    permissionMatrix.value = data
	  } catch (error) {
	    console.error('Failed to load permission matrix:', error)
	  } finally {
	    permissionsLoading.value = false
	  }
	}

	async function loadRoles() {
	  rolesLoading.value = true
	  try {
	    const data = await adminApi.getRoles()
	    roles.value = data
	  } catch (error) {
	    console.error('Failed to load roles:', error)
	  } finally {
	    rolesLoading.value = false
	  }
	}

	async function handleDeleteRole(role: any) {
	  try {
	    await ElMessageBox.confirm('确定要删除此角色吗？', '确认删除')
	    await adminApi.deleteRole(role.id)
	    ElMessage.success('角色删除成功')
	    loadRoles()
	  } catch (error) {
	    if (error !== 'cancel') {
	      ElMessage.error('删除失败')
	    }
	  }
	}

	async function loadUserRoles() {
	  if (!userRoleSearchUserId.value) return
	  userRolesLoading.value = true
	  try {
	    const data = await adminApi.getUserRoles(parseInt(userRoleSearchUserId.value))
	    userRoles.value = data
	  } catch (error) {
	    console.error('Failed to load user roles:', error)
	  } finally {
	    userRolesLoading.value = false
	  }
	}

	async function handleRemoveUserRole(userRole: any) {
	  try {
	    await ElMessageBox.confirm('确定要移除此角色吗？', '确认移除')
	    await adminApi.removeUserRole(userRole.userId, userRole.roleId)
	    ElMessage.success('角色移除成功')
	    loadUserRoles()
	  } catch (error) {
	    if (error !== 'cancel') {
	      ElMessage.error('移除失败')
	    }
	  }
	}

	// ============================================================================
	// 通知管理方法
	// ============================================================================

	async function loadSystemNotifications() {
	  notificationsLoading.value = true
	  try {
	    const data = await adminApi.getSystemNotifications({
	      page: notificationsPagination.value.page,
	      limit: notificationsPagination.value.limit
	    })
	    systemNotifications.value = data.items
	    notificationsPagination.value.total = data.total
	  } catch (error) {
	    console.error('Failed to load notifications:', error)
	  } finally {
	    notificationsLoading.value = false
	  }
	}

	async function loadNotificationTemplates() {
	  notificationTemplatesLoading.value = true
	  try {
	    const data = await adminApi.getNotificationTemplates()
	    notificationTemplates.value = data
	  } catch (error) {
	    console.error('Failed to load notification templates:', error)
	  } finally {
	    notificationTemplatesLoading.value = false
	  }
	}

	function getNotificationStatusTagType(status: string): string {
	  const typeMap: Record<string, string> = {
	    pending: 'info',
	    sending: 'warning',
	    sent: 'success',
	    failed: 'danger'
	  }
	  return typeMap[status] || 'info'
	}

	function getNotificationStatusLabel(status: string): string {
	  const labelMap: Record<string, string> = {
	    pending: '待发送',
	    sending: '发送中',
	    sent: '已发送',
	    failed: '失败'
	  }
	  return labelMap[status] || status
	}

	// ============================================================================
	// 数据清理方法
	// ============================================================================

	async function loadCleanupTasks() {
	  cleanupTasksLoading.value = true
	  try {
	    const data = await adminApi.getCleanupTasks()
	    cleanupTasks.value = data
	  } catch (error) {
	    console.error('Failed to load cleanup tasks:', error)
	  } finally {
	    cleanupTasksLoading.value = false
	  }
	}

	async function loadCleanupHistory() {
	  cleanupHistoryLoading.value = true
	  try {
	    const data = await adminApi.getCleanupHistory({
	      page: cleanupHistoryPagination.value.page,
	      limit: cleanupHistoryPagination.value.limit
	    })
	    cleanupHistory.value = data.items
	    cleanupHistoryPagination.value.total = data.total
	  } catch (error) {
	    console.error('Failed to load cleanup history:', error)
	  } finally {
	    cleanupHistoryLoading.value = false
	  }
	}

	async function loadStorageStats() {
	  storageStatsLoading.value = true
	  try {
	    const data = await adminApi.getStorageStats()
	    storageStats.value = data
	  } catch (error) {
	    console.error('Failed to load storage stats:', error)
	  } finally {
	    storageStatsLoading.value = false
	  }
	}

	async function triggerCleanupTask(task: any) {
	  try {
	    await ElMessageBox.confirm(`确定要执行清理任务"${task.displayName}"吗？`, '确认执行')
	    await adminApi.triggerCleanup(task.id)
	    ElMessage.success('清理任务已触发')
	    loadCleanupHistory()
	  } catch (error) {
	    if (error !== 'cancel') {
	      ElMessage.error('触发失败')
	    }
	  }
	}

	async function handleDeleteCleanupTask(task: any) {
	  try {
	    await ElMessageBox.confirm('确定要删除此清理任务吗？', '确认删除')
	    await adminApi.deleteCleanupTask(task.id)
	    ElMessage.success('删除成功')
	    loadCleanupTasks()
	  } catch (error) {
	    if (error !== 'cancel') {
	      ElMessage.error('删除失败')
	    }
	  }
	}

	function getCleanupTaskTypeLabel(type: string): string {
	  const labelMap: Record<string, string> = {
	    logs: '日志',
	    sessions: '会话',
	    temp_files: '临时文件',
	    cache: '缓存',
	    expired_data: '过期数据',
	    custom_sql: '自定义SQL'
	  }
	  return labelMap[type] || type
	}

	function getCleanupStatusTagType(status: string): string {
	  const typeMap: Record<string, string> = {
	    running: 'warning',
	    success: 'success',
	    failed: 'danger',
	    cancelled: 'info'
	  }
	  return typeMap[status] || 'info'
	}

	function getCleanupStatusLabel(status: string): string {
	  const labelMap: Record<string, string> = {
	    running: '运行中',
	    success: '成功',
	    failed: '失败',
	    cancelled: '已取消'
	  }
	  return labelMap[status] || status
	}

	// ============================================================================
	// 内容审核方法
	// ============================================================================

	async function loadPendingPapers() {
	  pendingPapersLoading.value = true
	  try {
	    const data = await adminApi.getPendingPapers({
	      page: pendingPapersPagination.value.page,
	      limit: pendingPapersPagination.value.limit
	    })
	    pendingPapers.value = data.items
	    pendingPapersPagination.value.total = data.total
	  } catch (error) {
	    console.error('Failed to load pending papers:', error)
	  } finally {
	    pendingPapersLoading.value = false
	  }
	}

	async function handleApprovePaper(paper: any) {
	  try {
	    await ElMessageBox.confirm('确定要通过此论文审核吗？', '确认通过')
	    await adminApi.approvePaper(paper.paperId)
	    ElMessage.success('审核通过')
	    loadPendingPapers()
	  } catch (error) {
	    if (error !== 'cancel') {
	      ElMessage.error('操作失败')
	    }
	  }
	}

	async function showRejectPaperDialog(paper: any) {
	  try {
	    const reason = await ElMessageBox.prompt('请输入拒绝原因', '拒绝论文')
	    await adminApi.rejectPaper(paper.paperId, { reason: reason.value || '' })
	    ElMessage.success('论文已拒绝')
	    loadPendingPapers()
	  } catch (error) {
	    // User cancelled
	  }
	}

	async function loadUserReports() {
	  userReportsLoading.value = true
	  try {
	    const data = await adminApi.getUserReports({
	      page: userReportsPagination.value.page,
	      limit: userReportsPagination.value.limit,
	      status: userReportsStatusFilter.value
	    })
	    userReports.value = data.items
	    userReportsPagination.value.total = data.total
	  } catch (error) {
	    console.error('Failed to load user reports:', error)
	  } finally {
	    userReportsLoading.value = false
	  }
	}

	async function showResolveReportDialog(report: any) {
	  try {
	    const resolution = await ElMessageBox.prompt('请输入处理说明', '处理举报')
	    await adminApi.resolveReport(report.id, { resolution: resolution.value || '' })
	    ElMessage.success('举报已处理')
	    loadUserReports()
	  } catch (error) {
	    // User cancelled
	  }
	}

	async function loadSensitiveWords() {
	  sensitiveWordsLoading.value = true
	  try {
	    const data = await adminApi.getSensitiveWords()
	    sensitiveWords.value = data
	  } catch (error) {
	    console.error('Failed to load sensitive words:', error)
	  } finally {
	    sensitiveWordsLoading.value = false
	  }
	}

	async function handleDeleteSensitiveWord(word: any) {
	  try {
	    await ElMessageBox.confirm('确定要删除此敏感词吗？', '确认删除')
	    await adminApi.deleteSensitiveWord(word.id)
	    ElMessage.success('删除成功')
	    loadSensitiveWords()
	  } catch (error) {
	    if (error !== 'cancel') {
	      ElMessage.error('删除失败')
	    }
	  }
	}

	function getModerationStatusTagType(status: string): string {
	  const typeMap: Record<string, string> = {
	    pending: 'warning',
	    approved: 'success',
	    rejected: 'danger',
	    flagged: 'info'
	  }
	  return typeMap[status] || 'info'
	}

	function getModerationStatusLabel(status: string): string {
	  const labelMap: Record<string, string> = {
	    pending: '待审核',
	    approved: '已通过',
	    rejected: '已拒绝',
	    flagged: '已标记'
	  }
	  return labelMap[status] || status
	}

	function getReportTargetTypeLabel(type: string): string {
	  const labelMap: Record<string, string> = {
	    paper: '论文',
	    user: '用户',
	    comment: '评论'
	  }
	  return labelMap[type] || type
	}

	function getReportPriorityTagType(priority: string): string {
	  const typeMap: Record<string, string> = {
	    low: 'info',
	    medium: 'warning',
	    high: 'danger',
	    urgent: 'danger'
	  }
	  return typeMap[priority] || 'info'
	}

	function getReportPriorityLabel(priority: string): string {
	  const labelMap: Record<string, string> = {
	    low: '低',
	    medium: '中',
	    high: '高',
	    urgent: '紧急'
	  }
	  return labelMap[priority] || priority
	}

	function getSensitiveWordCategoryLabel(category: string): string {
	  const labelMap: Record<string, string> = {
	    politics: '政治',
	    violence: '暴力',
	    adult: '成人',
	    spam: '垃圾',
	    other: '其他'
	  }
	  return labelMap[category] || category
	}

	function getSensitiveWordSeverityTagType(severity: string): string {
	  const typeMap: Record<string, string> = {
	    low: 'info',
	    medium: 'warning',
	    high: 'danger'
	  }
	  return typeMap[severity] || 'info'
	}

	function getSensitiveWordSeverityLabel(severity: string): string {
	  const labelMap: Record<string, string> = {
	    low: '低',
	    medium: '中',
	    high: '高'
	  }
	  return labelMap[severity] || severity
	}

	// ============================================================================
	// API密钥管理方法
	// ============================================================================

	async function loadApiKeys() {
	  apiKeysLoading.value = true
	  try {
	    const data = await adminApi.getApiKeys({
	      page: apiKeysPagination.value.page,
	      limit: apiKeysPagination.value.limit
	    })
	    apiKeys.value = data.items
	    apiKeysPagination.value.total = data.total
	  } catch (error) {
	    console.error('Failed to load API keys:', error)
	  } finally {
	    apiKeysLoading.value = false
	  }
	}

	async function loadApiKeyUsage(keyId?: number) {
	  apiKeyUsageLoading.value = true
	  try {
	    const data = await adminApi.getApiKeyUsage({ limit: 50, keyId })
	    apiKeyUsage.value = data.items
	    const stats = await adminApi.getApiKeyStats({ keyId })
	    apiKeyStats.value = stats
	  } catch (error) {
	    console.error('Failed to load API key usage:', error)
	  } finally {
	    apiKeyUsageLoading.value = false
	  }
	}

	async function handleDeleteApiKey(key: any) {
	  try {
	    await ElMessageBox.confirm('确定要删除此API密钥吗？此操作不可恢复。', '确认删除')
	    await adminApi.deleteApiKey(key.id)
	    ElMessage.success('密钥删除成功')
	    loadApiKeys()
	  } catch (error) {
	    if (error !== 'cancel') {
	      ElMessage.error('删除失败')
	    }
	  }
	}

	async function handleRegenerateApiKey(key: any) {
	  try {
	    await ElMessageBox.confirm('重新生成密钥将使旧密钥失效，确定继续吗？', '确认重新生成')
	    const data = await adminApi.regenerateApiKey(key.id)
	    ElMessageBox.alert(data.apiKey, '新密钥', {
	      confirmButtonText: '复制',
	      showClose: false
	    })
	    ElMessage.success('密钥重新生成成功')
	    loadApiKeys()
	  } catch (error) {
	    if (error !== 'cancel') {
	      ElMessage.error('重新生成失败')
	    }
	  }
	}

	function showApiKeyUsage(key: any) {
	  apiKeysSubTab.value = 'usage'
	  loadApiKeyUsage(key.id)
	}

	function toggleApiKey(key: any) {
	  // Toggle implementation
	  loadApiKeys()
	}

	function toggleCleanupTask(task: any) {
	  // Toggle implementation
	  loadCleanupTasks()
	}

	// ============================================================================
	// Dialog显示函数（简化版 - 实际应用中需要实现完整对话框）
	// ============================================================================

	function showCreateRoleDialog() {
	  ElMessage.info('创建角色对话框功能开发中')
	}

	function showAssignRoleDialog() {
	  ElMessage.info('分配角色对话框功能开发中')
	}

	function editRole(role: any) {
	  ElMessage.info('编辑角色功能开发中')
	}

	function showSendNotificationDialog() {
	  ElMessage.info('发送通知对话框功能开发中')
	}

	function showCreateCleanupTaskDialog() {
	  ElMessage.info('创建清理任务对话框功能开发中')
	}

	function showCreateSensitiveWordDialog() {
	  ElMessage.info('添加敏感词对话框功能开发中')
	}

	function showCreateApiKeyDialog() {
	  ElMessage.info('创建API密钥对话框功能开发中')
	}

// 更新watch statements以处理新标签页
watch(activeTab, (tab) => {
  if (tab === 'announcements' && authStore.isSuperAdmin) {
    loadAnnouncements()
  } else if (tab === 'monitoring' && authStore.isSuperAdmin) {
    loadSystemMetrics()
    loadServiceHealth()
  } else if (tab === 'security' && authStore.isSuperAdmin) {
    loadLoginHistory()
  } else if (tab === 'config' && authStore.isSuperAdmin) {
    loadConfigs()
  } else if (tab === 'backup' && authStore.isSuperAdmin) {
    loadBackupJobs()
  } else if (tab === 'permissions' && authStore.isSuperAdmin) {
    loadPermissionMatrix()
    loadRoles()
  } else if (tab === 'notifications' && authStore.isSuperAdmin) {
    loadSystemNotifications()
    loadNotificationTemplates()
  } else if (tab === 'cleanup' && authStore.isSuperAdmin) {
    loadCleanupTasks()
    loadStorageStats()
  } else if (tab === 'moderation' && authStore.isSuperAdmin) {
    loadPendingPapers()
    loadSensitiveWords()
  } else if (tab === 'api-keys' && authStore.isSuperAdmin) {
    loadApiKeys()
  }
})

watch(notificationsSubTab, (tab) => {
  if (tab === 'templates' && authStore.isSuperAdmin) {
    loadNotificationTemplates()
  }
})

watch(cleanupSubTab, (tab) => {
  if (tab === 'history' && authStore.isSuperAdmin) {
    loadCleanupHistory()
  } else if (tab === 'storage' && authStore.isSuperAdmin) {
    loadStorageStats()
  }
})

watch(moderationSubTab, (tab) => {
  if (tab === 'reports' && authStore.isSuperAdmin) {
    loadUserReports()
  }
})

watch(userReportsStatusFilter, () => {
  loadUserReports()
})

watch(apiKeysSubTab, (tab) => {
  if (tab === 'usage' && authStore.isSuperAdmin) {
    // Load usage when tab changes
  }
})

watch(configSubTab, (tab) => {
  if (tab === 'history' && authStore.isSuperAdmin) {
    loadConfigHistory()
  }
})

watch(backupSubTab, (tab) => {
  if (tab === 'records' && authStore.isSuperAdmin) {
    loadBackupRecords()
  }
})

// 生命周期
onMounted(() => {
  loadDashboard()
  loadUsers()
  loadModules()
})
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

.mb-4 {
  margin-bottom: 20px;
}

.mt-3 {
  margin-top: 15px;
}
</style>
