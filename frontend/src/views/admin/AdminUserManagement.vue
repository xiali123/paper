<template>
  <div class="tab-content">
    <!-- 搜索和过滤 -->
    <el-row :gutter="20" class="mb-3">
      <el-col :span="12">
        <el-input
          :model-value="searchQuery"
          placeholder="搜索用户名或邮箱"
          clearable
          @input="(val: string) => $emit('update:searchQuery', val)"
          @change="onSearch"
        >
          <template #prefix>
            <el-icon><Search /></el-icon>
          </template>
        </el-input>
      </el-col>
      <el-col :span="6">
        <el-select :model-value="roleFilter" placeholder="按角色筛选" clearable @change="(val: string) => { $emit('update:roleFilter', val); onRoleFilterChange() }">
          <el-option label="所有用户" value="" />
          <el-option label="普通用户" value="user" />
          <el-option label="高级用户" value="premium" />
          <el-option label="管理员" value="admin" />
          <el-option label="超级管理员" value="superadmin" />
        </el-select>
      </el-col>
      <el-col :span="6" class="text-right">
        <el-space>
          <el-button type="primary" @click="$emit('create-user')" v-if="authStore.isSuperAdmin">
            <el-icon><Plus /></el-icon>
            新增用户
          </el-button>
          <el-button @click="$emit('export-users')">
            <el-icon><Download /></el-icon>
            导出CSV
          </el-button>
          <el-button @click="$emit('load-users')">
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
          <el-link type="primary" @click="$emit('open-user-detail', row)">{{ row.username }}</el-link>
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
            @click="$emit('edit-user', row)"
          >
            编辑
          </el-button>
          <el-button
            v-if="row.isActive"
            :type="canManageRole(authStore.user?.role || 'user', row.role) ? 'warning' : 'info'"
            size="small"
            @click="$emit('toggle-user-status', row)"
          >
            停用
          </el-button>
          <el-button
            v-else
            type="success"
            size="small"
            @click="$emit('toggle-user-status', row)"
          >
            启用
          </el-button>
        </template>
      </el-table-column>
    </el-table>

    <!-- 分页 -->
    <el-pagination
      :current-page="pagination.page"
      :page-size="pagination.limit"
      :total="pagination.total"
      :page-sizes="[10, 20, 50, 100]"
      layout="total, sizes, prev, pager, next, jumper"
      @size-change="(size: number) => $emit('size-change', size)"
      @current-change="(page: number) => $emit('page-change', page)"
      class="mt-3"
    />

    <!-- 创建用户对话框 -->
    <el-dialog :model-value="createDialogVisible" title="新增用户" width="500px" @update:model-value="(val: boolean) => $emit('update:createDialogVisible', val)">
      <el-form :model="createForm" label-width="100px" :rules="createRules" :ref="(el: any) => $emit('update:createFormRef', el)">
        <el-form-item label="用户名" prop="username">
          <el-input :model-value="createForm.username" @update:model-value="(val: string) => updateCreateForm('username', val)" placeholder="请输入用户名" />
        </el-form-item>
        <el-form-item label="邮箱" prop="email">
          <el-input :model-value="createForm.email" @update:model-value="(val: string) => updateCreateForm('email', val)" placeholder="请输入邮箱" />
        </el-form-item>
        <el-form-item label="姓名">
          <el-input :model-value="createForm.fullName" @update:model-value="(val: string) => updateCreateForm('fullName', val)" placeholder="请输入姓名" />
        </el-form-item>
        <el-form-item label="角色">
          <el-select :model-value="createForm.role" @update:model-value="(val: string) => updateCreateForm('role', val)" placeholder="选择角色">
            <el-option label="普通用户" value="user" />
            <el-option label="高级用户" value="premium" />
            <el-option label="管理员" value="admin" />
            <el-option label="超级管理员" value="superadmin" />
          </el-select>
        </el-form-item>
        <el-form-item label="初始密码">
          <el-input :model-value="createForm.password" @update:model-value="(val: string) => updateCreateForm('password', val)" type="password" placeholder="默认: 123456" show-password />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="$emit('update:createDialogVisible', false)">取消</el-button>
        <el-button type="primary" @click="$emit('confirm-create')" :loading="saving">创建</el-button>
      </template>
    </el-dialog>

    <!-- 编辑用户对话框 -->
    <el-dialog :model-value="editDialogVisible" title="编辑用户" width="500px" @update:model-value="(val: boolean) => $emit('update:editDialogVisible', val)">
      <el-form :model="editForm" label-width="100px">
        <el-form-item label="用户名">
          <el-input :model-value="editForm.username" disabled />
        </el-form-item>
        <el-form-item label="邮箱">
          <el-input :model-value="editForm.email" @update:model-value="(val: string) => updateEditForm('email', val)" />
        </el-form-item>
        <el-form-item label="姓名">
          <el-input :model-value="editForm.fullName" @update:model-value="(val: string) => updateEditForm('fullName', val)" />
        </el-form-item>
        <el-form-item label="角色">
          <el-select :model-value="editForm.role" @update:model-value="(val: string) => updateEditForm('role', val)" :disabled="!canChangeRole">
            <el-option label="普通用户" value="user" />
            <el-option label="高级用户" value="premium" />
            <el-option label="管理员" value="admin" />
            <el-option label="超级管理员" value="superadmin" :disabled="editForm.role === 'superadmin' && editForm.id === authStore.user?.id" />
          </el-select>
        </el-form-item>
        <el-form-item label="状态">
          <el-switch :model-value="editForm.isActive" @update:model-value="(val: boolean) => updateEditForm('isActive', val)" active-text="启用" inactive-text="停用" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="$emit('update:editDialogVisible', false)">取消</el-button>
        <el-button type="primary" @click="$emit('save-user')" :loading="saving">保存</el-button>
      </template>
    </el-dialog>

    <!-- 用户详情抽屉 -->
    <el-drawer :model-value="userDetailVisible" @update:model-value="(val: boolean) => $emit('update:userDetailVisible', val)" :title="detailUser?.username || '用户详情'" size="900px">
      <el-tabs :model-value="userHistoryTab" @update:model-value="(val: string) => $emit('update:userHistoryTab', val)">
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
                <el-button type="danger" size="small" @click="$emit('kick-session', row.id)">踢出</el-button>
              </template>
            </el-table-column>
          </el-table>
          <el-empty v-if="!userSessions.length" description="暂无在线会话" />
        </el-tab-pane>
      </el-tabs>
    </el-drawer>
  </div>
</template>

<script setup lang="ts">
import { Search, Refresh, Plus, Download } from '@element-plus/icons-vue'

const props = defineProps<{
  authStore: any
  loading: boolean
  users: any[]
  pagination: { page: number; limit: number; total: number }
  searchQuery: string
  roleFilter: string
  createDialogVisible: boolean
  createForm: any
  createRules: any
  editDialogVisible: boolean
  editForm: any
  saving: boolean
  canChangeRole: boolean
  userDetailVisible: boolean
  detailUser: any
  userHistoryTab: string
  loginHistory: any[]
  userSessions: any[]
  getRoleLabel: (role: any) => string
  getRoleBadgeType: (role: any) => string
  getActivityTagType: (status: string) => string
  getActivityLabel: (status: string) => string
  formatDateTime: (ts: any) => string
  canManageRole: (current: any, target: any) => boolean
}>()

const emit = defineEmits<{
  (e: 'search'): void
  (e: 'load-users'): void
  (e: 'page-change', page: number): void
  (e: 'size-change', size: number): void
  (e: 'create-user'): void
  (e: 'confirm-create'): void
  (e: 'edit-user', user: any): void
  (e: 'save-user'): void
  (e: 'toggle-user-status', user: any): void
  (e: 'export-users'): void
  (e: 'open-user-detail', user: any): void
  (e: 'kick-session', sessionId: string): void
  (e: 'update:searchQuery', value: string): void
  (e: 'update:roleFilter', value: string): void
  (e: 'update:createDialogVisible', value: boolean): void
  (e: 'update:editDialogVisible', value: boolean): void
  (e: 'update:userDetailVisible', value: boolean): void
  (e: 'update:userHistoryTab', value: string): void
  (e: 'update:createFormRef', el: any): void
  (e: 'update:createForm', field: string, value: any): void
  (e: 'update:editForm', field: string, value: any): void
}>()

function onSearch() {
  emit('search')
}

function onRoleFilterChange() {
  emit('load-users')
}

function updateCreateForm(field: string, value: any) {
  emit('update:createForm', field, value)
}

function updateEditForm(field: string, value: any) {
  emit('update:editForm', field, value)
}
</script>
