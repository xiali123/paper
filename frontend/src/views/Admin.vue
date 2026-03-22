<template>
  <div class="admin-page">
    <div class="admin-container">
      <!-- Header -->
      <div class="admin-header">
        <h1>{{ t('admin.dashboard') }}</h1>
        <div v-if="authStore.isSuperAdmin" class="superadmin-badge">
          👑 {{ t('roles.superadmin') }}
        </div>
      </div>

      <!-- Authorization Check -->
      <div v-if="!authStore.isAdminOrSuper" class="not-authorized">
        <div class="error-icon">🔒</div>
        <h2>{{ t('admin.accessDenied') }}</h2>
        <p>{{ t('admin.adminRequired') }}</p>
        <router-link to="/" class="btn-primary">{{ t('common.back') }}</router-link>
      </div>

      <!-- Admin Content -->
      <div v-else class="admin-content">
        <!-- Statistics Cards -->
        <div class="stats-grid">
          <div class="stat-card">
            <div class="stat-icon">👥</div>
            <div class="stat-content">
              <h3>{{ t('admin.totalUsers') }}</h3>
              <p class="stat-value">{{ stats.totalUsers }}</p>
            </div>
          </div>
          <div class="stat-card">
            <div class="stat-icon">✅</div>
            <div class="stat-content">
              <h3>{{ t('admin.activeUsers') }}</h3>
              <p class="stat-value">{{ stats.activeUsers }}</p>
            </div>
          </div>
          <div class="stat-card">
            <div class="stat-icon">🔧</div>
            <div class="stat-content">
              <h3>{{ t('admin.adminUsers') }}</h3>
              <p class="stat-value">{{ stats.adminUsers }}</p>
            </div>
          </div>
          <div class="stat-card" v-if="authStore.isSuperAdmin">
            <div class="stat-icon">👑</div>
            <div class="stat-content">
              <h3>{{ t('admin.superadminUsers') }}</h3>
              <p class="stat-value">{{ stats.superadminUsers }}</p>
            </div>
          </div>
          <div class="stat-card">
            <div class="stat-icon">⭐</div>
            <div class="stat-content">
              <h3>{{ t('admin.premiumUsers') }}</h3>
              <p class="stat-value">{{ stats.premiumUsers }}</p>
            </div>
          </div>
          <div class="stat-card">
            <div class="stat-icon">📄</div>
            <div class="stat-content">
              <h3>{{ t('admin.totalPapers') }}</h3>
              <p class="stat-value">{{ stats.totalPapers }}</p>
            </div>
          </div>
        </div>

        <!-- User Management Section -->
        <div class="admin-section">
          <div class="section-header">
            <h2>{{ t('admin.userManagement') }}</h2>
            <div class="section-controls">
              <input
                v-model="searchQuery"
                type="text"
                class="search-input"
                :placeholder="t('admin.searchPlaceholder')"
                @input="handleSearch"
              />
              <select v-model="roleFilter" class="role-filter" @change="handleSearch">
                <option value="">{{ t('admin.allRoles') }}</option>
                <option value="user">{{ t('roles.user') }}</option>
                <option value="premium">{{ t('roles.premium') }}</option>
                <option value="admin">{{ t('roles.admin') }}</option>
                <option value="superadmin" v-if="authStore.isSuperAdmin">{{ t('roles.superadmin') }}</option>
              </select>
              <button class="btn-refresh" @click="loadUsers">
                🔄 {{ t('stats.refresh') }}
              </button>
            </div>
          </div>

          <!-- Users Table -->
          <div class="table-container">
            <table class="data-table">
              <thead>
                <tr>
                  <th>ID</th>
                  <th>{{ t('auth.username') }}</th>
                  <th>{{ t('auth.email') }}</th>
                  <th>{{ t('admin.role') }}</th>
                  <th>{{ t('admin.status') }}</th>
                  <th>{{ t('admin.lastLogin') }}</th>
                  <th>{{ t('admin.joined') }}</th>
                  <th>{{ t('admin.actions') }}</th>
                </tr>
              </thead>
              <tbody>
                <tr v-for="user in paginatedUsers" :key="user.id">
                  <td>{{ user.id }}</td>
                  <td>
                    <div class="user-cell">
                      <span class="user-avatar">{{ getUserInitial(user) }}</span>
                      <span class="user-name">{{ user.username }}</span>
                    </div>
                  </td>
                  <td>{{ user.email }}</td>
                  <td>
                    <span :class="['role-badge', getRoleBadgeClass(user.role)]">
                      {{ getRoleLabel(user.role) }}
                    </span>
                  </td>
                  <td>
                    <span :class="['status-badge', user.isActive ? 'active' : 'inactive']">
                      {{ user.isActive ? t('common.active') : t('common.inactive') }}
                    </span>
                  </td>
                  <td>{{ formatDate(user.lastLoginAt) }}</td>
                  <td>{{ formatDate(user.createdAt) }}</td>
                  <td>
                    <div class="action-buttons">
                      <button class="btn-action btn-edit" @click="openEditModal(user)" :title="t('admin.editUser')">
                        ✏️
                      </button>
                      <button
                        v-if="!user.isActive && canActivateUser(user)"
                        class="btn-action btn-activate"
                        @click="confirmActivateUser(user)"
                        :title="t('admin.activate')"
                      >
                        ✅
                      </button>
                      <button
                        v-if="user.isActive && canDeactivateUser(user)"
                        class="btn-action btn-deactivate"
                        @click="confirmDeactivateUser(user)"
                        :title="t('admin.deactivate')"
                      >
                        ⏸️
                      </button>
                      <button
                        v-if="authStore.isSuperAdmin && canDeleteUser(user)"
                        class="btn-action btn-delete"
                        @click="confirmDeleteUser(user)"
                        :title="t('admin.delete')"
                      >
                        🗑️
                      </button>
                    </div>
                  </td>
                </tr>
              </tbody>
            </table>

            <!-- Pagination -->
            <div class="pagination" v-if="totalPages > 1">
              <button
                class="pagination-btn"
                :disabled="currentPage === 1"
                @click="currentPage--"
              >
                ‹
              </button>
              <span class="pagination-info">
                {{ t('search.page') }} {{ currentPage }} / {{ totalPages }}
              </span>
              <button
                class="pagination-btn"
                :disabled="currentPage === totalPages"
                @click="currentPage++"
              >
                ›
              </button>
            </div>
          </div>
        </div>

        <!-- Audit Logs Section (Superadmin Only) -->
        <div v-if="authStore.isSuperAdmin" class="admin-section">
          <div class="section-header">
            <h2>{{ t('admin.auditLogs') }}</h2>
            <button class="btn-refresh" @click="loadAuditLogs">
              🔄 {{ t('stats.refresh') }}
            </button>
          </div>

          <div class="audit-logs-container">
            <div v-if="auditLogs.length === 0" class="no-logs">
              {{ t('audit.noLogsFound') }}
            </div>
            <div v-else>
              <div v-for="log in auditLogs.slice(0, 10)" :key="log.id" class="log-entry">
                <div class="log-header">
                  <span class="log-action">{{ formatAuditAction(log.action) }}</span>
                  <span class="log-time">{{ formatDate(log.createdAt) }}</span>
                </div>
                <div class="log-details">
                  <span class="log-admin">{{ t('audit.admin') }}: {{ log.adminUsername }}</span>
                  <span v-if="log.targetUserId" class="log-target">
                    → {{ t('audit.target') }}: #{{ log.targetUserId }}
                  </span>
                </div>
                <div v-if="log.changes" class="log-changes">
                  {{ formatChanges(log.changes) }}
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Edit User Modal -->
    <div v-if="showEditModal" class="modal-overlay" @click="closeEditModal">
      <div class="modal-content" @click.stop>
        <div class="modal-header">
          <h3>{{ t('admin.editUser') }}</h3>
          <button class="modal-close" @click="closeEditModal">×</button>
        </div>
        <div class="modal-body">
          <div class="form-group">
            <label for="edit-username">{{ t('auth.username') }}</label>
            <input id="edit-username" v-model="editingUser.username" type="text" disabled class="form-input disabled" />
          </div>
          <div class="form-group">
            <label for="edit-email">{{ t('auth.email') }}</label>
            <input id="edit-email" v-model="editingUser.email" type="email" disabled class="form-input disabled" />
          </div>
          <div class="form-group">
            <label for="edit-fullname">{{ t('auth.fullName') }}</label>
            <input id="edit-fullname" v-model="editingUser.fullName" type="text" class="form-input" />
          </div>
          <div class="form-group">
            <label for="edit-affiliation">{{ t('user.affiliation') }}</label>
            <input id="edit-affiliation" v-model="editingUser.affiliation" type="text" class="form-input" />
          </div>
          <div class="form-group">
            <label for="edit-role">{{ t('admin.role') }}</label>
            <select id="edit-role" v-model="editingUser.role" class="form-select" :disabled="!canChangeRole(editingUser)">
              <option value="user">{{ t('roles.user') }}</option>
              <option value="premium">{{ t('roles.premium') }}</option>
              <option value="admin" v-if="authStore.isSuperAdmin">{{ t('roles.admin') }}</option>
              <option value="superadmin" v-if="authStore.isSuperAdmin && editingUser.id !== authStore.user?.id">{{ t('roles.superadmin') }}</option>
            </select>
          </div>
          <div class="form-group">
            <label for="edit-status">{{ t('admin.status') }}</label>
            <select id="edit-status" v-model="editingUser.isActive" class="form-select" :disabled="!canChangeStatus(editingUser)">
              <option :value="true">{{ t('common.active') }}</option>
              <option :value="false">{{ t('common.inactive') }}</option>
            </select>
          </div>
        </div>
        <div class="modal-footer">
          <button class="btn-secondary" @click="closeEditModal">{{ t('common.cancel') }}</button>
          <button class="btn-primary" @click="saveUser" :disabled="saving">
            {{ saving ? t('common.saving') : t('common.save') }}
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'
import { useAuthStore } from '@/stores/auth'
import adminApi, { type AdminUser, type UserRole, getRoleLabel, getRoleBadgeClass, formatAuditAction } from '@/api/modules/admin'
import { ElMessage } from '@/utils/notification'

const { t } = useI18n()
const authStore = useAuthStore()

// State
const stats = ref({
  totalUsers: 0,
  activeUsers: 0,
  adminUsers: 0,
  premiumUsers: 0,
  superadminUsers: 0,
  regularUsers: 0,
  totalPapers: 0,
  totalSearches: 0,
  recentRegistrations: 0
})

const users = ref<AdminUser[]>([])
const auditLogs = ref<any[]>([])
const loading = ref(false)
const saving = ref(false)

// Pagination & Filtering
const currentPage = ref(1)
const pageSize = ref(10)
const searchQuery = ref('')
const roleFilter = ref('')
const totalPages = computed(() => Math.ceil(users.value.length / pageSize.value))

// Edit Modal
const showEditModal = ref(false)
const editingUser = ref<AdminUser>({
  id: 0,
  username: '',
  email: '',
  fullName: '',
  role: 'user',
  isActive: true,
  createdAt: ''
})

// Computed
const paginatedUsers = computed(() => {
  const start = (currentPage.value - 1) * pageSize.value
  const end = start + pageSize.value
  return users.value.slice(start, end)
})

// Load Data
onMounted(async () => {
  await Promise.all([loadStats(), loadUsers()])
  if (authStore.isSuperAdmin) {
    await loadAuditLogs()
  }
})

async function loadStats() {
  try {
    loading.value = true
    stats.value = await adminApi.getStats()
  } catch (error: any) {
    console.error('Failed to load stats:', error)
  } finally {
    loading.value = false
  }
}

async function loadUsers() {
  try {
    loading.value = true
    const response = await adminApi.getUsers({
      page: currentPage.value,
      limit: 100, // Load more for client-side filtering
      search: searchQuery.value,
      role: roleFilter.value as UserRole
    })
    users.value = response.items
  } catch (error: any) {
    console.error('Failed to load users:', error)
    ElMessage.error(t('admin.errorOccurred'))
  } finally {
    loading.value = false
  }
}

async function loadAuditLogs() {
  try {
    loading.value = true
    const response = await adminApi.getAuditLogs({
      page: 1,
      limit: 50
    })
    auditLogs.value = response.items
  } catch (error: any) {
    console.error('Failed to load audit logs:', error)
  } finally {
    loading.value = false
  }
}

function handleSearch() {
  currentPage.value = 1
  loadUsers()
}

// User Actions
function getUserInitial(user: AdminUser): string {
  return (user.fullName || user.username || '').charAt(0).toUpperCase()
}

function formatDate(dateStr?: string): string {
  if (!dateStr) return '-'
  return new Date(dateStr).toLocaleDateString()
}

function formatChanges(changes: any): string {
  if (!changes) return ''
  return JSON.stringify(changes)
}

function canChangeRole(user: AdminUser): boolean {
  if (!authStore.isSuperAdmin) return false
  if (user.id === authStore.user?.id) return false
  return true
}

function canChangeStatus(user: AdminUser): boolean {
  if (user.id === authStore.user?.id) return false
  if (user.role === 'superadmin') return false
  if (user.role === 'admin' && !authStore.isSuperAdmin) return false
  return true
}

function canActivateUser(user: AdminUser): boolean {
  if (user.id === authStore.user?.id) return false
  if (user.role === 'superadmin') return false
  if (user.role === 'admin' && !authStore.isSuperAdmin) return false
  return true
}

function canDeactivateUser(user: AdminUser): boolean {
  return canChangeStatus(user) && user.isActive
}

function canDeleteUser(user: AdminUser): boolean {
  if (user.id === authStore.user?.id) return false
  if (user.role === 'superadmin') return false
  return true
}

// Edit Modal
function openEditModal(user: AdminUser) {
  editingUser.value = { ...user }
  showEditModal.value = true
}

function closeEditModal() {
  showEditModal.value = false
  editingUser.value = {
    id: 0,
    username: '',
    email: '',
    fullName: '',
    role: 'user',
    isActive: true,
    createdAt: ''
  }
}

async function saveUser() {
  try {
    saving.value = true
    await adminApi.updateUser(editingUser.value.id, {
      fullName: editingUser.value.fullName,
      affiliation: editingUser.value.affiliation,
      role: editingUser.value.role,
      isActive: editingUser.value.isActive
    })

    ElMessage.success(t('admin.userUpdated'))
    closeEditModal()
    await loadUsers()
    await loadStats()
  } catch (error: any) {
    console.error('Failed to save user:', error)
    ElMessage.error(error.message || t('admin.errorOccurred'))
  } finally {
    saving.value = false
  }
}

// Activate/Deactivate
async function confirmActivateUser(user: AdminUser) {
  if (!confirm(t('admin.confirmActivate'))) return

  try {
    await adminApi.activateUser(user.id)
    ElMessage.success(t('admin.userActivated'))
    await loadUsers()
    await loadStats()
  } catch (error: any) {
    console.error('Failed to activate user:', error)
    ElMessage.error(error.message || t('admin.errorOccurred'))
  }
}

async function confirmDeactivateUser(user: AdminUser) {
  if (!confirm(t('admin.confirmDeactivate'))) return

  try {
    await adminApi.deactivateUser(user.id)
    ElMessage.success(t('admin.userDeactivated'))
    await loadUsers()
    await loadStats()
  } catch (error: any) {
    console.error('Failed to deactivate user:', error)
    ElMessage.error(error.message || t('admin.errorOccurred'))
  }
}

// Delete
async function confirmDeleteUser(user: AdminUser) {
  if (!confirm(t('admin.confirmDelete'))) return

  try {
    await adminApi.deleteUser(user.id)
    ElMessage.success(t('admin.userDeleted'))
    await loadUsers()
    await loadStats()
  } catch (error: any) {
    console.error('Failed to delete user:', error)
    ElMessage.error(error.message || t('admin.errorOccurred'))
  }
}
</script>

<style scoped>
.admin-page {
  min-height: 100vh;
  padding: 2rem;
  background: var(--color-background);
}

.admin-container {
  max-width: 1400px;
  margin: 0 auto;
}

.admin-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 2rem;
}

.admin-header h1 {
  margin: 0;
  font-size: 2rem;
  color: var(--color-text-primary);
}

.superadmin-badge {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  padding: 0.5rem 1rem;
  border-radius: 20px;
  font-size: 0.875rem;
  font-weight: 500;
}

.not-authorized {
  text-align: center;
  padding: 4rem 2rem;
}

.error-icon {
  font-size: 4rem;
  margin-bottom: 1rem;
}

.not-authorized h2 {
  margin-bottom: 1rem;
  color: var(--color-error);
}

.not-authorized p {
  color: var(--color-text-secondary);
  margin-bottom: 2rem;
}

.btn-primary {
  display: inline-block;
  padding: 0.75rem 1.5rem;
  background: var(--color-primary);
  color: white;
  border: none;
  border-radius: 8px;
  cursor: pointer;
  font-weight: 500;
  transition: all 0.3s ease;
}

.btn-primary:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
}

.btn-primary:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.btn-secondary {
  padding: 0.75rem 1.5rem;
  background: var(--color-surface);
  color: var(--color-text-primary);
  border: 1px solid var(--color-border);
  border-radius: 8px;
  cursor: pointer;
  font-weight: 500;
  transition: all 0.3s ease;
}

.btn-secondary:hover {
  background: var(--color-background);
}

/* Stats Grid */
.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 1.5rem;
  margin-bottom: 2rem;
}

.stat-card {
  background: var(--color-surface);
  padding: 1.5rem;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  display: flex;
  align-items: center;
  gap: 1rem;
  transition: transform 0.3s ease;
}

.stat-card:hover {
  transform: translateY(-4px);
}

.stat-icon {
  font-size: 2.5rem;
  opacity: 0.8;
}

.stat-content h3 {
  font-size: 0.875rem;
  color: var(--color-text-secondary);
  margin: 0 0 0.5rem 0;
}

.stat-value {
  font-size: 2rem;
  font-weight: bold;
  color: var(--color-primary);
  margin: 0;
}

/* Admin Sections */
.admin-section {
  background: var(--color-surface);
  padding: 2rem;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  margin-bottom: 2rem;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 1.5rem;
  flex-wrap: wrap;
  gap: 1rem;
}

.section-header h2 {
  margin: 0;
  font-size: 1.5rem;
  color: var(--color-text-primary);
}

.section-controls {
  display: flex;
  gap: 1rem;
  align-items: center;
}

.search-input {
  padding: 0.625rem 1rem;
  border: 1px solid var(--color-border);
  border-radius: 8px;
  font-size: 0.875rem;
  min-width: 250px;
  background: var(--color-background);
  color: var(--color-text-primary);
}

.role-filter {
  padding: 0.625rem 1rem;
  border: 1px solid var(--color-border);
  border-radius: 8px;
  font-size: 0.875rem;
  background: var(--color-background);
  color: var(--color-text-primary);
  cursor: pointer;
}

.btn-refresh {
  padding: 0.625rem 1rem;
  background: var(--color-primary);
  color: white;
  border: none;
  border-radius: 8px;
  cursor: pointer;
  font-size: 0.875rem;
  transition: all 0.3s ease;
}

.btn-refresh:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
}

/* Table */
.table-container {
  overflow-x: auto;
}

.data-table {
  width: 100%;
  border-collapse: collapse;
}

.data-table th,
.data-table td {
  padding: 1rem;
  text-align: left;
  border-bottom: 1px solid var(--color-border);
}

.data-table th {
  font-weight: 600;
  color: var(--color-text-secondary);
  font-size: 0.875rem;
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.data-table tbody tr:hover {
  background: var(--color-background);
}

.user-cell {
  display: flex;
  align-items: center;
  gap: 0.75rem;
}

.user-avatar {
  width: 32px;
  height: 32px;
  border-radius: 50%;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 0.875rem;
  font-weight: 600;
}

.user-name {
  font-weight: 500;
}

.role-badge {
  padding: 0.375rem 0.75rem;
  border-radius: 20px;
  font-size: 0.75rem;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.role-badge.user {
  background: #e8f5e9;
  color: #2e7d32;
}

.role-badge.premium {
  background: #fff8e1;
  color: #f57f17;
}

.role-badge.admin {
  background: #ffebee;
  color: #c62828;
}

.role-badge.superadmin {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.status-badge {
  padding: 0.375rem 0.75rem;
  border-radius: 20px;
  font-size: 0.75rem;
  font-weight: 600;
}

.status-badge.active {
  background: #e8f5e9;
  color: #2e7d32;
}

.status-badge.inactive {
  background: #ffebee;
  color: #c62828;
}

.action-buttons {
  display: flex;
  gap: 0.5rem;
}

.btn-action {
  padding: 0.5rem;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  font-size: 1rem;
  transition: all 0.2s ease;
  background: var(--color-background);
}

.btn-action:hover {
  transform: scale(1.1);
}

.btn-edit:hover {
  background: #e3f2fd;
}

.btn-activate:hover {
  background: #e8f5e9;
}

.btn-deactivate:hover {
  background: #fff8e1;
}

.btn-delete:hover {
  background: #ffebee;
}

/* Pagination */
.pagination {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 1rem;
  margin-top: 1.5rem;
}

.pagination-btn {
  padding: 0.5rem 1rem;
  border: 1px solid var(--color-border);
  background: var(--color-surface);
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s ease;
}

.pagination-btn:hover:not(:disabled) {
  background: var(--color-primary);
  color: white;
}

.pagination-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.pagination-info {
  color: var(--color-text-primary);
  font-size: 0.875rem;
}

/* Audit Logs */
.audit-logs-container {
  max-height: 500px;
  overflow-y: auto;
  background: var(--color-background);
  border-radius: 8px;
  padding: 1rem;
}

.no-logs {
  text-align: center;
  color: var(--color-text-primary);
  padding: 2rem;
}

.log-entry {
  background: var(--color-surface);
  padding: 1rem;
  border-radius: 8px;
  margin-bottom: 0.75rem;
  border-left: 4px solid var(--color-primary);
}

.log-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 0.5rem;
}

.log-action {
  font-weight: 600;
  color: var(--color-primary);
}

.log-time {
  color: var(--color-text-secondary);
  font-size: 0.75rem;
}

.log-details {
  color: var(--color-text-primary);
  font-size: 0.875rem;
  margin-bottom: 0.5rem;
}

.log-changes {
  font-family: monospace;
  font-size: 0.75rem;
  background: var(--color-background);
  padding: 0.5rem;
  border-radius: 4px;
  color: var(--color-text-primary);
}

/* Modal */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  animation: fadeIn 0.2s ease;
}

@keyframes fadeIn {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

.modal-content {
  background: var(--color-surface);
  border-radius: 12px;
  width: 90%;
  max-width: 500px;
  max-height: 90vh;
  overflow-y: auto;
  animation: slideUp 0.3s ease;
}

@keyframes slideUp {
  from {
    transform: translateY(20px);
    opacity: 0;
  }
  to {
    transform: translateY(0);
    opacity: 1;
  }
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 1.5rem;
  border-bottom: 1px solid var(--color-border);
}

.modal-header h3 {
  margin: 0;
  font-size: 1.25rem;
}

.modal-close {
  background: none;
  border: none;
  font-size: 1.5rem;
  cursor: pointer;
  color: var(--color-text-secondary);
  padding: 0;
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  transition: all 0.2s ease;
}

.modal-close:hover {
  background: var(--color-background);
}

.modal-body {
  padding: 1.5rem;
}

.form-group {
  margin-bottom: 1rem;
}

.form-group label {
  display: block;
  margin-bottom: 0.5rem;
  font-weight: 500;
  color: var(--color-text-primary);
  font-size: 0.875rem;
}

.form-input,
.form-select {
  width: 100%;
  padding: 0.75rem;
  border: 1px solid var(--color-border);
  border-radius: 8px;
  font-size: 0.875rem;
  background: var(--color-background);
  color: var(--color-text-primary);
  transition: border-color 0.2s ease;
}

.form-input:focus,
.form-select:focus {
  outline: none;
  border-color: var(--color-primary);
}

.form-input.disabled {
  opacity: 0.6;
  cursor: not-allowed;
  background: var(--color-background);
}

.modal-footer {
  display: flex;
  justify-content: flex-end;
  gap: 1rem;
  padding: 1.5rem;
  border-top: 1px solid var(--color-border);
}

/* Responsive */
@media (max-width: 768px) {
  .admin-page {
    padding: 1rem;
  }

  .stats-grid {
    grid-template-columns: 1fr;
  }

  .section-header {
    flex-direction: column;
    align-items: stretch;
  }

  .section-controls {
    flex-direction: column;
    width: 100%;
  }

  .search-input,
  .role-filter {
    width: 100%;
  }

  .data-table {
    font-size: 0.75rem;
  }

  .data-table th,
  .data-table td {
    padding: 0.75rem 0.5rem;
  }
}
</style>
