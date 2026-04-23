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
              <div class="stat-value">{{ stats.activeUsers }}</div>
              <div class="stat-label">活跃用户</div>
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

    <!-- 选项卡内容 -->
    <el-tabs v-model="activeTab" class="admin-tabs">
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
            <el-col :span="6">
              <el-button type="primary" @click="loadUsers">
                <el-icon><Refresh /></el-icon>
                刷新
              </el-button>
            </el-col>
          </el-row>

          <!-- 用户列表 -->
          <el-table :data="users" stripe v-loading="loading" style="width: 100%">
            <el-table-column prop="id" label="ID" width="80" />
            <el-table-column prop="username" label="用户名" width="150" />
            <el-table-column prop="email" label="邮箱" width="200" />
            <el-table-column prop="fullName" label="姓名" width="150" />
            <el-table-column label="角色" width="120">
              <template #default="{ row }">
                <el-tag :type="getRoleBadgeType(row.role)" size="small">
                  {{ getRoleLabel(row.role) }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="状态" width="100">
              <template #default="{ row }">
                <el-tag :type="row.active ? 'success' : 'danger'" size="small">
                  {{ row.active ? '活跃' : '停用' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="操作" width="250">
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
                  v-if="row.active"
                  :type="authStore.canManageRole(authStore.user?.role || 'user', row.role) ? 'warning' : 'info'"
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

      <!-- 模块管理 (仅超级管理员) -->
      <el-tab-pane label="模块管理" name="modules" v-if="authStore.isSuperAdmin">
        <div class="tab-content">
          <el-alert
            title="模块管理"
            type="warning"
            description="启用或禁用系统模块。禁用模块将影响相关功能的可用性。"
            :closable="false"
            show-icon
            class="mb-3"
          />

          <el-table :data="modules" stripe v-loading="loading">
            <el-table-column prop="name" label="模块名称" width="200" />
            <el-table-column prop="version" label="版本" width="100" />
            <el-table-column prop="description" label="描述" />
            <el-table-column prop="type" label="类型" width="120">
              <template #default="{ row }">
                <el-tag size="small">{{ row.type }}</el-tag>
              </template>
            </el-table-column>
            <el-table-column label="状态" width="100">
              <template #default="{ row }">
                <el-switch
                  v-model="row.enabled"
                  @change="handleToggleModule(row)"
                  :loading="moduleToggleLoading"
                />
              </template>
            </el-table-column>
          </el-table>
        </div>
      </el-tab-pane>

      <!-- 审计日志 (仅超级管理员) -->
      <el-tab-pane label="审计日志" name="audit" v-if="authStore.isSuperAdmin">
        <div class="tab-content">
          <el-table :data="auditLogs" stripe v-loading="loading" style="width: 100%">
            <el-table-column prop="id" label="ID" width="80" />
            <el-table-column prop="action" label="操作" width="150" />
            <el-table-column prop="entityType" label="实体类型" width="120" />
            <el-table-column prop="actorUsername" label="操作者" width="150" />
            <el-table-column prop="details" label="详情" />
            <el-table-column prop="ipAddress" label="IP地址" width="150" />
            <el-table-column prop="createdAt" label="时间" width="180">
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
    </el-tabs>

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
            <el-option label="超级管理员" value="superadmin" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="editDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleSaveUser" :loading="saving">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores'
import { adminApi, type AdminUser, type ModuleInfo, type AuditLog, type UserRole } from '@/api/modules/admin'
import {
  Setting,
  User,
  Search,
  Refresh,
  CircleCheck,
  Star,
  Memo
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'

const router = useRouter()
const authStore = useAuthStore()

// 检查权限
if (!authStore.isAdminOrSuper) {
  ElMessage.error('您没有访问管理控制台的权限')
  router.push('/dashboard')
}

// 状态
const loading = ref(false)
const activeTab = ref('users')
const searchQuery = ref('')
const roleFilter = ref('')

// 统计数据
const stats = ref({
  totalUsers: 0,
  activeUsers: 0,
  premiumUsers: 0,
  adminUsers: 0,
  totalPapers: 0,
  totalSearches: 0,
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
const moduleToggleLoading = ref(false)

// 审计日志
const auditLogs = ref<AuditLog[]>([])
const auditPagination = ref({
  page: 1,
  limit: 20,
  total: 0
})

// 编辑用户
const editDialogVisible = ref(false)
const editForm = ref<AdminUser>({
  id: 0,
  username: '',
  email: '',
  fullName: '',
  avatar: '',
  role: 'user',
  active: true,
  createdAt: new Date(),
  lastLoginAt: new Date(),
  lastLoginIp: ''
})
const saving = ref(false)

// 计算属性
const canChangeRole = computed(() => {
  return authStore.isSuperAdmin
})

// 方法
async function loadStats() {
  try {
    const data = await adminApi.getStats()
    stats.value = data
  } catch (error: any) {
    ElMessage.error('加载统计数据失败: ' + (error.message || '未知错误'))
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
    users.value = response.items
    pagination.value.total = response.total
  } catch (error: any) {
    ElMessage.error('加载用户列表失败: ' + (error.message || '未知错误'))
  } finally {
    loading.value = false
  }
}

async function loadModules() {
  loading.value = true
  try {
    modules.value = await adminApi.getModules()
  } catch (error: any) {
    ElMessage.error('加载模块列表失败: ' + (error.message || '未知错误'))
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

async function handleSaveUser() {
  saving.value = true
  try {
    await adminApi.updateUser(editForm.value.id, {
      email: editForm.value.email,
      fullName: editForm.value.fullName,
      avatar: editForm.value.avatar,
      role: editForm.value.role
    })
    ElMessage.success('用户更新成功')
    editDialogVisible.value = false
    loadUsers()
    loadStats()
  } catch (error: any) {
    ElMessage.error('更新用户失败: ' + (error.message || '未知错误'))
  } finally {
    saving.value = false
  }
}

async function handleToggleUserStatus(user: AdminUser) {
  const action = user.active ? '停用' : '启用'
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

    if (user.active) {
      await adminApi.deactivateUser(user.id)
    } else {
      await adminApi.activateUser(user.id)
    }

    ElMessage.success(`用户${action}成功`)
    loadUsers()
    loadStats()
  } catch (error: any) {
    if (error !== 'cancel') {
      ElMessage.error(`${action}用户失败: ` + (error.message || '未知错误'))
    }
  }
}

async function handleToggleModule(module: ModuleInfo) {
  moduleToggleLoading.value = true
  try {
    if (module.enabled) {
      await adminApi.disableModule({ moduleName: module.name })
      ElMessage.success(`模块 ${module.name} 已禁用`)
    } else {
      await adminApi.enableModule({ moduleName: module.name })
      ElMessage.success(`模块 ${module.name} 已启用`)
    }
    loadModules()
    loadStats()
  } catch (error: any) {
    ElMessage.error('操作失败: ' + (error.message || '未知错误'))
    // 回滚状态
    module.enabled = !module.enabled
  } finally {
    moduleToggleLoading.value = false
  }
}

function getRoleLabel(role: UserRole): string {
  return adminApi.getRoleLabel(role, 'zh')
}

function getRoleBadgeType(role: UserRole): string {
  const types: Record<string, string> = {
    user: '',
    premium: 'warning',
    admin: 'danger',
    superadmin: 'danger'
  }
  return types[role] || ''
}

function formatDateTime(timestamp: number | string): string {
  const date = new Date(timestamp)
  return date.toLocaleString('zh-CN')
}

// 生命周期
onMounted(() => {
  loadStats()
  loadUsers()
})
</script>

<style scoped>
.admin-dashboard {
  padding: 20px;
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
}

.tab-content {
  min-height: 400px;
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
