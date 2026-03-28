# PaperCrawler Component Specifications

## Detailed Component Implementation Specifications

This document provides detailed specifications for key components in the PaperCrawler frontend application.

---

## 1. Layout Components

### 1.1 MainLayout.vue

**Purpose**: Primary application layout with header, sidebar, and main content area

**Props**: None

**Slots**:
- `header`: Custom header content
- `sidebar`: Custom sidebar content
- `default`: Main content area

**Features**:
- Responsive sidebar (collapses on mobile)
- Breadcrumb navigation
- User menu dropdown
- Notification bell
- Search quick access

**Example**:
```vue
<template>
  <el-container class="main-layout">
    <el-aside
      :width="collapsed ? '64px' : '240px'"
      class="sidebar"
    >
      <AppLogo :collapsed="collapsed" />
      <AppMenu :collapsed="collapsed" />
    </el-aside>

    <el-container>
      <el-header height="60px">
        <AppHeader>
          <template #actions>
            <NotificationBell />
            <UserDropdown />
          </template>
        </AppHeader>
      </el-header>

      <el-breadcrumb separator="/">
        <el-breadcrumb-item
          v-for="item in breadcrumbs"
          :key="item.path"
          :to="item.path"
        >
          {{ item.title }}
        </el-breadcrumb-item>
      </el-breadcrumb>

      <el-main>
        <router-view v-slot="{ Component }">
          <transition name="fade" mode="out-in">
            <component :is="Component" />
          </transition>
        </router-view>
      </el-main>
    </el-container>
  </el-container>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRoute } from 'vue-router'
import { useUIStore } from '@/stores/ui'

const route = useRoute()
const uiStore = useUIStore()

const collapsed = computed(() => uiStore.sidebarCollapsed)

const breadcrumbs = computed(() => {
  const matched = route.matched.filter(r => r.meta?.title)
  return matched.map(r => ({
    path: r.path,
    title: r.meta?.title as string
  }))
})
</script>

<style scoped>
.main-layout {
  height: 100vh;
}

.sidebar {
  transition: width 0.3s;
}

.fade-enter-active, .fade-leave-active {
  transition: opacity 0.2s;
}

.fade-enter-from, .fade-leave-to {
  opacity: 0;
}
</style>
```

### 1.2 AuthLayout.vue

**Purpose**: Layout for authentication pages (login, register, password reset)

**Props**: None

**Slots**:
- `default`: Authentication form content

**Features**:
- Centered card layout
- Background pattern/gradient
- Logo and branding
- Language switcher
- Footer with links

**Example**:
```vue
<template>
  <div class="auth-layout">
    <div class="auth-background">
      <div class="background-pattern" />
    </div>

    <div class="auth-container">
      <div class="auth-header">
        <AppLogo size="large" />
        <LanguageSwitcher />
      </div>

      <el-card class="auth-card">
        <router-view />
      </el-card>

      <div class="auth-footer">
        <el-link href="/terms">{{ $t('auth.terms') }}</el-link>
        <el-link href="/privacy">{{ $t('auth.privacy') }}</el-link>
      </div>
    </div>
  </div>
</template>

<style scoped>
.auth-layout {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
}

.auth-background {
  position: absolute;
  inset: 0;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  z-index: 0;
}

.background-pattern {
  opacity: 0.1;
  background-image: url('/pattern.svg');
  background-size: cover;
}

.auth-container {
  position: relative;
  z-index: 1;
  width: 100%;
  max-width: 440px;
  padding: 20px;
}

.auth-card {
  box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.1);
}

@media (max-width: 640px) {
  .auth-card {
    box-shadow: none;
  }
}
</style>
```

---

## 2. Paper Management Components

### 2.1 PaperCard.vue

**Purpose**: Display paper information in card format

**Props**:
```typescript
interface Props {
  paper: Paper                    // Paper data
  compact?: boolean               // Compact display mode
  showActions?: boolean           // Show action buttons
  selectable?: boolean            // Allow selection
  selected?: boolean              // Is selected
}
```

**Emits**:
```typescript
interface Emits {
  click: [paper: Paper]
  save: [paper: Paper]
  export: [paper: Paper]
  share: [paper: Paper]
  select: [paper: Paper, selected: boolean]
}
```

**Slots**:
- `thumbnail`: Custom thumbnail display
- `actions`: Custom action buttons
- `meta`: Additional metadata display

**Features**:
- Truncated abstract
- Author list with "et al" for many authors
- Citation count
- Publication year
- Favorite toggle
- Quick actions menu
- Selection checkbox

### 2.2 SearchFilters.vue

**Purpose**: Sidebar for paper search filters

**Props**:
```typescript
interface Props {
  modelValue: SearchFilters        // Current filters
  availableFilters: FilterConfig[]  // Available filter options
}
```

**Emits**:
```typescript
interface Emits {
  'update:modelValue': [filters: SearchFilters]
  'reset': []
}
```

**Features**:
- Keyword search
- Year range slider
- Field of study dropdown
- Publication type checkbox
- Citation count filter
- Date range picker
- Save preset filters
- Reset filters button

**Example**:
```vue
<template>
  <el-aside width="280px" class="search-filters">
    <el-form :model="localFilters" label-position="top">
      <!-- Keyword Search -->
      <el-form-item :label="$t('search.keywords')">
        <el-input
          v-model="localFilters.query"
          :placeholder="$t('search.keywordsPlaceholder')"
          clearable
          @input="handleUpdate"
        />
      </el-form-item>

      <!-- Year Range -->
      <el-form-item :label="$t('search.yearRange')">
        <el-slider
          v-model="localFilters.yearRange"
          range
          :min="1950"
          :max="2024"
          :marks="yearMarks"
          @change="handleUpdate"
        />
      </el-form-item>

      <!-- Field of Study -->
      <el-form-item :label="$t('search.field')">
        <el-select
          v-model="localFilters.field"
          multiple
          collapse-tags
          @change="handleUpdate"
        >
          <el-option
            v-for="field in availableFields"
            :key="field.value"
            :label="field.label"
            :value="field.value"
          />
        </el-select>
      </el-form-item>

      <!-- Publication Type -->
      <el-form-item :label="$t('search.type')">
        <el-checkbox-group v-model="localFilters.types" @change="handleUpdate">
          <el-checkbox label="journal">{{ $t('search.journal') }}</el-checkbox>
          <el-checkbox label="conference">{{ $t('search.conference') }}</el-checkbox>
          <el-checkbox label="preprint">{{ $t('search.preprint') }}</el-checkbox>
        </el-checkbox-group>
      </el-form-item>

      <!-- Actions -->
      <el-button type="primary" @click="applyFilters" block>
        {{ $t('search.apply') }}
      </el-button>
      <el-button @click="resetFilters" block>
        {{ $t('search.reset') }}
      </el-button>
    </el-form>
  </el-aside>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import type { SearchFilters } from '@/types'

interface Props {
  modelValue: SearchFilters
  availableFilters: any[]
}

const props = defineProps<Props>()
const emit = defineEmits<{
  'update:modelValue': [filters: SearchFilters]
  reset: []
}>()

const localFilters = ref({ ...props.modelValue })

const yearMarks = {
  1950: '1950',
  2000: '2000',
  2024: '2024'
}

function handleUpdate() {
  emit('update:modelValue', localFilters.value)
}

function resetFilters() {
  localFilters.value = {
    query: '',
    yearRange: [1950, 2024],
    field: [],
    types: []
  }
  emit('reset')
}

function applyFilters() {
  emit('update:modelValue', localFilters.value)
}
</script>
```

### 2.3 PaperPreview.vue

**Purpose**: PDF viewer component for paper preview

**Props**:
```typescript
interface Props {
  pdfUrl: string                    // PDF file URL
  paperId: string | number          // Paper ID for notes
  currentPage?: number              // Initial page
  enableNotes?: boolean             // Enable note-taking
}
```

**Emits**:
```typescript
interface Emits {
  'page-change': [page: number]
  'note-add': [note: Note]
  'error': [error: Error]
}
```

**Features**:
- PDF rendering with PDF.js
- Page navigation
- Zoom controls
- Fullscreen mode
- Note-taking panel
- Text selection and highlighting
- Download PDF button

---

## 3. AI Analysis Components

### 3.1 PDFUploader.vue

**Purpose**: Upload PDF for AI analysis

**Props**:
```typescript
interface Props {
  maxSize?: number                  // Max file size in MB (default: 10)
  acceptedFormats?: string[]        // Accepted file types
  multiple?: boolean                // Allow multiple files
}
```

**Emits**:
```typescript
interface Emits {
  upload: [files: File[]]
  'upload-progress': [progress: number]
  'upload-complete': [response: UploadResponse]
  'upload-error': [error: Error]
}
```

**Features**:
- Drag and drop zone
- File size validation
- File type validation
- Upload progress bar
- Preview uploaded files
- Remove files before upload
- Retry failed uploads

**Example**:
```vue
<template>
  <div
    class="pdf-uploader"
    :class="{ 'drag-over': isDragOver }"
    @dragover.prevent="isDragOver = true"
    @dragleave.prevent="isDragOver = false"
    @drop.prevent="handleDrop"
  >
    <el-upload
      ref="uploadRef"
      :auto-upload="false"
      :on-change="handleFileChange"
      :on-remove="handleFileRemove"
      :before-upload="beforeUpload"
      :file-list="fileList"
      drag
      :accept="acceptString"
      :multiple="multiple"
    >
      <div class="upload-content">
        <el-icon class="upload-icon"><UploadFilled /></el-icon>
        <div class="upload-text">
          {{ $t('ai.uploadText') }}
        </div>
        <div class="upload-hint">
          {{ $t('ai.uploadHint', { maxSize: maxSize + 'MB' }) }}
        </div>
      </div>
    </el-upload>

    <div v-if="uploadProgress > 0" class="upload-progress">
      <el-progress :percentage="uploadProgress" :status="uploadStatus" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { ElMessage } from 'element-plus'
import { UploadFilled } from '@element-plus/icons-vue'

interface Props {
  maxSize?: number
  acceptedFormats?: string[]
  multiple?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  maxSize: 10,
  acceptedFormats: () => ['application/pdf'],
  multiple: false
})

const emit = defineEmits<{
  upload: [files: File[]]
  'upload-progress': [progress: number]
}>()

const isDragOver = ref(false)
const fileList = ref<any[]>([])
const uploadProgress = ref(0)
const uploadStatus = ref<'success' | 'exception' | ''>('')

const acceptString = computed(() =>
  props.acceptedFormats.join(',')
)

function beforeUpload(file: File) {
  // Validate file size
  const isLtSize = file.size / 1024 / 1024 < props.maxSize
  if (!isLtSize) {
    ElMessage.error(`${$t('ai.fileTooLarge')} ${props.maxSize}MB`)
    return false
  }

  // Validate file type
  const isValidType = props.acceptedFormats.includes(file.type)
  if (!isValidType) {
    ElMessage.error($t('ai.invalidFileType'))
    return false
  }

  return true
}

function handleFileChange(file: any, files: any[]) {
  fileList.value = files
  if (beforeUpload(file.raw)) {
    startUpload([file.raw])
  }
}

function handleFileRemove() {
  fileList.value = []
}

function handleDrop(e: DragEvent) {
  isDragOver.value = false
  const files = Array.from(e.dataTransfer?.files || [])
  const validFiles = files.filter(file => beforeUpload(file))

  if (validFiles.length > 0) {
    startUpload(validFiles)
  }
}

async function startUpload(files: File[]) {
  emit('upload', files)

  // Simulate upload progress
  let progress = 0
  const interval = setInterval(() => {
    progress += 10
    uploadProgress.value = progress
    emit('upload-progress', progress)

    if (progress >= 100) {
      clearInterval(interval)
      uploadStatus.value = 'success'
    }
  }, 200)
}
</script>

<style scoped>
.pdf-uploader {
  border: 2px dashed var(--el-border-color);
  border-radius: 6px;
  padding: 40px;
  text-align: center;
  transition: border-color 0.3s;
}

.pdf-uploader.drag-over {
  border-color: var(--el-color-primary);
  background-color: var(--el-fill-color-light);
}

.upload-icon {
  font-size: 48px;
  color: var(--el-color-primary);
}

.upload-text {
  font-size: 16px;
  margin-top: 16px;
}

.upload-hint {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin-top: 8px;
}

.upload-progress {
  margin-top: 20px;
}
</style>
```

### 3.2 AIChat.vue

**Purpose**: Chat interface for AI paper analysis

**Props**:
```typescript
interface Props {
  paperId?: string | number       // Associated paper ID
  messages?: ChatMessage[]         // Initial messages
  disabled?: boolean               // Disable input
}
```

**Emits**:
```typescript
interface Emits {
  'send-message': [message: string]
  'clear-chat': []
}
```

**Features**:
- Chat message bubbles
- Auto-scroll to latest message
- Message timestamps
- Typing indicator
- Markdown rendering
- Code syntax highlighting
- Copy code button
- Clear chat confirmation
- Message input with character limit

---

## 4. Admin Components

### 4.1 UserTable.vue

**Purpose**: Display and manage users in admin dashboard

**Props**:
```typescript
interface Props {
  users: AdminUser[]               // User data
  loading?: boolean                // Loading state
  pagination?: Pagination          // Pagination info
}
```

**Emits**:
```typescript
interface Emits {
  'edit-user': [user: AdminUser]
  'delete-user': [user: AdminUser]
  'change-role': [user: AdminUser, role: UserRole]
  'activate': [user: AdminUser]
  'deactivate': [user: AdminUser]
  'page-change': [page: number]
}
```

**Features**:
- Sortable columns
- Filter by role
- Search by email/username
- Bulk actions
- Row actions menu
- Role badge display
- Status indicator
- Pagination
- Export to CSV

**Example**:
```vue
<template>
  <div class="user-table">
    <!-- Toolbar -->
    <div class="toolbar">
      <el-input
        v-model="searchQuery"
        :placeholder="$t('admin.searchUsers')"
        clearable
        @input="handleSearch"
      >
        <template #prefix>
          <el-icon><Search /></el-icon>
        </template>
      </el-input>

      <el-select
        v-model="roleFilter"
        :placeholder="$t('admin.filterByRole')"
        clearable
        @change="handleFilter"
      >
        <el-option
          v-for="role in roles"
          :key="role.value"
          :label="role.label"
          :value="role.value"
        />
      </el-select>

      <el-button @click="handleExport">
        {{ $t('admin.export') }}
      </el-button>
    </div>

    <!-- Table -->
    <el-table
      :data="filteredUsers"
      v-loading="loading"
      stripe
      @selection-change="handleSelectionChange"
    >
      <el-table-column type="selection" width="55" />

      <el-table-column
        prop="username"
        :label="$t('admin.username')"
        sortable
      />

      <el-table-column
        prop="email"
        :label="$t('admin.email')"
        sortable
      />

      <el-table-column
        prop="role"
        :label="$t('admin.role')"
        width="120"
      >
        <template #default="{ row }">
          <el-tag :type="getRoleTagType(row.role)">
            {{ getRoleLabel(row.role) }}
          </el-tag>
        </template>
      </el-table-column>

      <el-table-column
        prop="isActive"
        :label="$t('admin.status')"
        width="100"
      >
        <template #default="{ row }">
          <el-tag :type="row.isActive ? 'success' : 'danger'">
            {{ row.isActive ? $t('admin.active') : $t('admin.inactive') }}
          </el-tag>
        </template>
      </el-table-column>

      <el-table-column
        prop="createdAt"
        :label="$t('admin.joined')"
        width="120"
        sortable
      >
        <template #default="{ row }">
          {{ formatDate(row.createdAt) }}
        </template>
      </el-table-column>

      <el-table-column
        :label="$t('admin.actions')"
        width="200"
        fixed="right"
      >
        <template #default="{ row }">
          <el-button
            size="small"
            @click="$emit('edit-user', row)"
          >
            {{ $t('common.edit') }}
          </el-button>
          <el-dropdown @command="(cmd) => handleAction(cmd, row)">
            <el-button size="small">
              {{ $t('common.more') }}
              <el-icon class="el-icon--right"><ArrowDown /></el-icon>
            </el-button>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item
                  command="activate"
                  v-if="!row.isActive"
                >
                  {{ $t('admin.activate') }}
                </el-dropdown-item>
                <el-dropdown-item
                  command="deactivate"
                  v-else
                >
                  {{ $t('admin.deactivate') }}
                </el-dropdown-item>
                <el-dropdown-item command="role">
                  {{ $t('admin.changeRole') }}
                </el-dropdown-item>
                <el-dropdown-item
                  command="delete"
                  divided
                  style="color: var(--el-color-danger)"
                >
                  {{ $t('common.delete') }}
                </el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </template>
      </el-table-column>
    </el-table>

    <!-- Pagination -->
    <el-pagination
      v-model:current-page="pagination.page"
      v-model:page-size="pagination.pageSize"
      :total="pagination.total"
      :page-sizes="[10, 20, 50, 100]"
      layout="total, sizes, prev, pager, next, jumper"
      @current-change="$emit('page-change', $event)"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import type { AdminUser, UserRole } from '@/api/modules/admin'
import { getRoleLabel, canManageRole } from '@/api/modules/admin'

interface Props {
  users: AdminUser[]
  loading?: boolean
  pagination: {
    page: number
    pageSize: number
    total: number
  }
}

const props = defineProps<Props>()
const emit = defineEmits<{
  'edit-user': [user: AdminUser]
  'delete-user': [user: AdminUser]
  'change-role': [user: AdminUser, role: UserRole]
  'page-change': [page: number]
}>()

const searchQuery = ref('')
const roleFilter = ref<UserRole | ''>('')
const selectedUsers = ref<AdminUser[]>([])

const roles = [
  { value: 'user', label: 'User' },
  { value: 'premium', label: 'Premium' },
  { value: 'admin', label: 'Admin' },
  { value: 'superadmin', label: 'Superadmin' }
]

const filteredUsers = computed(() => {
  return props.users.filter(user => {
    const matchesSearch =
      user.username.toLowerCase().includes(searchQuery.value.toLowerCase()) ||
      user.email.toLowerCase().includes(searchQuery.value.toLowerCase())

    const matchesRole = !roleFilter.value || user.role === roleFilter.value

    return matchesSearch && matchesRole
  })
})

function getRoleTagType(role: UserRole) {
  const types = {
    user: '',
    premium: 'warning',
    admin: 'danger',
    superadmin: 'success'
  }
  return types[role]
}

function handleSearch() {
  // Search is handled via computed property
}

function handleFilter() {
  // Filter is handled via computed property
}

function handleAction(command: string, user: AdminUser) {
  switch (command) {
    case 'activate':
      // Handle activate
      break
    case 'deactivate':
      // Handle deactivate
      break
    case 'role':
      // Open role change dialog
      break
    case 'delete':
      emit('delete-user', user)
      break
  }
}

function handleSelectionChange(selection: AdminUser[]) {
  selectedUsers.value = selection
}

function formatDate(date: string) {
  return new Date(date).toLocaleDateString()
}

function handleExport() {
  // Export to CSV
  const csv = filteredUsers.value.map(user => ({
    Username: user.username,
    Email: user.email,
    Role: user.role,
    Status: user.isActive ? 'Active' : 'Inactive'
  }))

  const csvContent = 'data:text/csv;charset=utf-8,' +
    encodeURIComponent(JSON.stringify(csv))

  const link = document.createElement('a')
  link.setAttribute('href', csvContent)
  link.setAttribute('download', 'users.csv')
  link.click()
}
</script>
```

---

## 5. Premium/Payment Components

### 5.1 PricingCard.vue

**Purpose**: Display premium pricing plan

**Props**:
```typescript
interface Props {
  plan: PremiumPlan               // Plan data
  popular?: boolean               // Highlight as popular
  current?: boolean               // User's current plan
}
```

**Emits**:
```typescript
interface Emits {
  'select-plan': [plan: PremiumPlan]
}
```

**Features**:
- Plan name and description
- Price display
- Feature list with checkmarks
- Popular badge
- Current plan indicator
- CTA button
- Comparison highlights

---

## 6. Common Components

### 6.1 StatusBadge.vue

**Purpose**: Display status indicator badge

**Props**:
```typescript
interface Props {
  status: 'active' | 'inactive' | 'pending' | 'success' | 'error' | 'warning'
  text?: string                   // Custom text
  size?: 'small' | 'medium' | 'large'
}
```

**Features**:
- Color-coded status
- Icon indicator
- Customizable size
- Dot indicator option
- Pulsing animation for active status

**Example**:
```vue
<template>
  <span
    class="status-badge"
    :class="[
      `status-badge--${status}`,
      `status-badge--${size}`
    ]"
  >
    <span class="status-badge__dot" />
    <span v-if="text" class="status-badge__text">
      {{ text }}
    </span>
  </span>
</template>

<script setup lang="ts">
interface Props {
  status: 'active' | 'inactive' | 'pending' | 'success' | 'error' | 'warning'
  text?: string
  size?: 'small' | 'medium' | 'large'
}

withDefaults(defineProps<Props>(), {
  size: 'medium'
})
</script>

<style scoped>
.status-badge {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 4px 10px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 500;
}

.status-badge--small {
  padding: 2px 6px;
  font-size: 10px;
}

.status-badge--large {
  padding: 6px 14px;
  font-size: 14px;
}

.status-badge__dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background-color: currentColor;
}

.status-badge--active {
  background-color: #e6f7e6;
  color: #52c41a;
}

.status-badge--inactive {
  background-color: #f5f5f5;
  color: #8c8c8c;
}

.status-badge--pending {
  background-color: #fff7e6;
  color: #faad14;
}

.status-badge--success {
  background-color: #e6f7e6;
  color: #52c41a;
}

.status-badge--error {
  background-color: #fff1f0;
  color: #ff4d4f;
}

.status-badge--warning {
  background-color: #fff7e6;
  color: #faad14;
}

.status-badge--active .status-badge__dot {
  animation: pulse 2s infinite;
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.5;
  }
}
</style>
```

### 6.2 ConfirmDialog.vue

**Purpose**: Reusable confirmation dialog

**Props**:
```typescript
interface Props {
  modelValue: boolean              // Show/hide dialog
  title?: string                   // Dialog title
  message?: string                 // Dialog message
  confirmText?: string             // Confirm button text
  cancelText?: string              // Cancel button text
  type?: 'warning' | 'danger' | 'info'
}
```

**Emits**:
```typescript
interface Emits {
  'update:modelValue': [value: boolean]
  'confirm': []
  'cancel': []
}
```

**Usage**:
```vue
<script setup lang="ts">
import { ref } from 'vue'
import ConfirmDialog from '@/components/common/ConfirmDialog.vue'

const showConfirm = ref(false)

const handleDelete = () => {
  showConfirm.value = true
}

const confirmDelete = () => {
  // Perform delete action
  showConfirm.value = false
}
</script>

<template>
  <el-button @click="handleDelete">Delete</el-button>

  <ConfirmDialog
    v-model="showConfirm"
    title="Delete Item"
    message="Are you sure you want to delete this item? This action cannot be undone."
    type="danger"
    @confirm="confirmDelete"
  />
</template>
```

---

## Component Testing Guidelines

### Unit Testing Example
```typescript
// PaperCard.test.ts
import { mount } from '@vue/test-utils'
import { describe, it, expect } from 'vitest'
import PaperCard from '@/components/paper/PaperCard.vue'

describe('PaperCard', () => {
  const mockPaper = {
    id: 1,
    title: 'Test Paper',
    authors: ['Author 1', 'Author 2'],
    year: 2024,
    citations: 100,
    abstract: 'This is a test abstract'
  }

  it('renders paper information correctly', () => {
    const wrapper = mount(PaperCard, {
      props: { paper: mockPaper }
    })

    expect(wrapper.text()).toContain('Test Paper')
    expect(wrapper.text()).toContain('Author 1')
    expect(wrapper.text()).toContain('2024')
  })

  it('emits click event when clicked', async () => {
    const wrapper = mount(PaperCard, {
      props: { paper: mockPaper }
    })

    await wrapper.trigger('click')
    expect(wrapper.emitted('click')).toBeTruthy()
    expect(wrapper.emitted('click')![0]).toEqual([mockPaper])
  })

  it('toggles favorite correctly', async () => {
    const wrapper = mount(PaperCard, {
      props: { paper: mockPaper }
    })

    const favoriteButton = wrapper.find('[data-testid="favorite-button"]')
    await favoriteButton.trigger('click')

    expect(wrapper.emitted('save')).toBeTruthy()
  })
})
```

---

This document provides detailed specifications for the key components. Each component should be implemented following these specifications with proper TypeScript typing, accessibility considerations, and responsive design.