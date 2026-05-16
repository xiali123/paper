import { ref, computed, onMounted, watch } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '@/stores'
import adminApi, { type ModuleInfo } from '@/api/modules/admin'
import type { FrontendAdminUser as AdminUser, FrontendAuditLog as AuditLog, UserRole } from '@/api/adapters/adminAdapter'
import type { UploadFile } from 'element-plus'
import { ElMessage, ElMessageBox } from 'element-plus'
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

export function useAdminDashboard() {
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
      if (import.meta.env.DEV) console.log('[loadUsers] API response:', response)
      if (import.meta.env.DEV) console.log('[loadUsers] response.items:', response.items)
      users.value = response.items || []
      pagination.value.total = response.total || 0
      if (import.meta.env.DEV) console.log('[loadUsers] users.value:', users.value.length, 'users')
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
      if (import.meta.env.DEV) console.log('[loadModules] API returned:', result)
      if (import.meta.env.DEV) console.log('[loadModules] result type:', Array.isArray(result) ? 'Array' : typeof result)
      modules.value = result || []
      if (import.meta.env.DEV) console.log('[loadModules] modules.value:', modules.value)
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

  // RBAC权限管理方法
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

  // 通知管理方法
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

  // 数据清理方法
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

  // 内容审核方法
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

  // API密钥管理方法
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
      const statsData = await adminApi.getApiKeyStats({ keyId })
      apiKeyStats.value = statsData
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

  // Dialog显示函数（简化版 - 实际应用中需要实现完整对话框）
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

  return {
    // Auth
    authStore,

    // State
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
    historyPagination,
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

    // Computed
    canChangeRole,
    userTrendChartData,
    chartOptions,

    // Methods
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
  }
}
