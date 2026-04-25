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
            <el-col :span="6" class="text-right">
              <el-space>
                <el-button type="primary" @click="handleCreateUser" v-if="authStore.isSuperAdmin">
                  <el-icon><Plus /></el-icon>
                  新增用户
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
            <el-table-column prop="username" label="用户名" width="130" />
            <el-table-column prop="email" label="邮箱" width="180" />
            <el-table-column prop="fullName" label="姓名" width="130" />
            <el-table-column label="角色" width="110">
              <template #default="{ row }">
                <el-tag :type="getRoleBadgeType(row.role)" size="small">
                  {{ getRoleLabel(row.role, 'zh') }}
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
            <el-table-column prop="loginCount" label="登录次数" width="90" />
            <el-table-column label="注册时间" width="160">
              <template #default="{ row }">
                {{ formatDateTime(row.createdAt) }}
              </template>
            </el-table-column>
            <el-table-column label="最后登录" width="160">
              <template #default="{ row }">
                {{ row.lastLoginAt ? formatDateTime(row.lastLoginAt) : '未登录' }}
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
            <el-table-column prop="name" label="模块名称" width="200" />
            <el-table-column prop="version" label="版本" width="100" />
            <el-table-column prop="description" label="描述" />
            <el-table-column label="类型" width="120">
              <template #default="{ row }">
                <el-tag :type="row.type === 'business' ? 'primary' : 'info'" size="small">
                  {{ row.type === 'business' ? '业务' : '功能' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="状态" width="100">
              <template #default="{ row }">
                <el-tag :type="row.enabled ? 'success' : 'warning'" size="small">
                  {{ row.enabled ? '已启用' : '已禁用' }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="操作" width="300">
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
    </el-tabs>

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
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores'
import adminApi, { type AdminUser, type ModuleInfo, type AuditLog, type UserRole } from '@/api/modules/admin'
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
  Plus
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
const moduleToggleLoading = ref(false)

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
  avatar: '',
  role: 'user',
  isActive: true,
  createdAt: new Date(),
  lastLoginAt: new Date(),
  lastLoginIp: '',
  loginCount: 0,
  activityStatus: 'inactive'
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
    console.log('📦 [loadUsers] API response:', response)
    console.log('📦 [loadUsers] response.items:', response.items)
    users.value = response.items || []
    pagination.value.total = response.total || 0
    console.log('📦 [loadUsers] users.value:', users.value.length, 'users')
  } catch (error: any) {
    console.error('❌ [loadUsers] Error:', error)
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
    console.log('📦 [loadModules] API返回:', result)
    console.log('📦 [loadModules] 结果类型:', Array.isArray(result) ? 'Array' : typeof result)
    modules.value = result || []
    console.log('📦 [loadModules] modules.value:', modules.value)
  } catch (error: any) {
    console.error('❌ [loadModules] API调用失败:', error)
    ElMessage.error('加载模块列表失败: ' + (error.message || '未知错误'))
    modules.value = []  // 确保失败时是空数组
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
    loadStats()
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
    loadStats()
  } catch (error: any) {
    if (error !== 'cancel') {
      ElMessage.error(`${action}用户失败: ` + (error.message || '未知错误'))
    }
  }
}

function getRoleLabel(role: UserRole): string {
  return adminApi.getRoleLabel(role, 'zh')
}

function getRoleBadgeType(role: UserRole): string {
  const types: Record<string, string> = {
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

function getActivityTagType(status: string): string {
  const types: Record<string, string> = {
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
  // 如果是字符串，先转换为数字
  const num = typeof timestamp === 'string' ? parseInt(timestamp) : timestamp
  // 如果是0或无效值，显示未登录或空
  if (!num || num === 0) return ''
  const date = new Date(num * 1000)  // Unix时间戳需要乘以1000转换为毫秒
  return date.toLocaleString('zh-CN')
}

// 模块管理方法
function canUninstallModule(moduleName: string): boolean {
  // 防止卸载核心模块
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
    await loadStats()
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
    await loadStats()
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
    // 可选：合并扫描结果或显示在单独的列表中
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
  // 自动从文件名推断模块名
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
    // 读取文件为Base64
    const fileReader = new FileReader()
    fileReader.onload = async (e) => {
      const base64 = (e.target?.result as string).split(',')[1]

      // 上传文件
      const uploadResult = await adminApi.uploadModule({
        fileData: base64,
        filename: uploadForm.value.file!.name
      })

      // 安装模块
      await adminApi.installModule({
        moduleName: uploadForm.value.moduleName,
        modulePath: uploadResult.path
      })

      ElMessage.success('模块上传并安装成功')
      uploadDialogVisible.value = false
      await loadModules()
      await loadStats()
    }
    fileReader.readAsDataURL(uploadForm.value.file)
  } catch (error: any) {
    ElMessage.error('上传失败: ' + (error.message || '未知错误'))
  } finally {
    uploading.value = false
  }
}

// 生命周期
onMounted(() => {
  loadStats()
  loadUsers()
  loadModules()
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
