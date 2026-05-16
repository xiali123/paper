<template>
  <div class="collaboration-panel">
    <div class="panel-header">
      <h3>协作功能</h3>
      <el-tag v-if="session" size="small" :type="isActive ? 'success' : 'info'">
        {{ isActive ? '协作中' : '未连接' }}
      </el-tag>
    </div>

    <!-- 用户列表 -->
    <div class="user-section">
      <div class="section-header">
        <h4>在线用户 ({{ activeUsers.length }})</h4>
        <el-button size="small" @click="showInviteForm = !showInviteForm">
          <el-icon><Plus /></el-icon>
          邀请
        </el-button>
      </div>

      <div class="user-list">
        <div
          v-for="user in activeUsers"
          :key="user.id"
          class="user-item"
          :class="{ 'is-current-user': user.id === currentUserId }"
        >
          <div class="user-avatar" :style="{ backgroundColor: user.color }">
            {{ user.name.charAt(0).toUpperCase() }}
          </div>
          <div class="user-info">
            <div class="user-name">
              {{ user.name }}
              <el-tag v-if="user.id === currentUserId" size="small" type="info">
                我
              </el-tag>
            </div>
            <div class="user-status">
              <el-icon :class="user.isOnline ? 'online' : 'offline'">
                <Check />
              </el-icon>
              {{ user.isOnline ? '在线' : '离线' }}
              <span v-if="user.cursor" class="cursor-position">
                第 {{ user.cursor.line }} 行
              </span>
            </div>
          </div>
          <div class="user-actions">
            <el-dropdown v-if="user.id !== currentUserId" size="small">
              <el-button size="small" text>
                <el-icon><MoreFilled /></el-icon>
              </el-button>
              <template #dropdown>
                <el-dropdown-menu>
                  <el-dropdown-item @click="sendMessage(user)">
                    发送消息
                  </el-dropdown-item>
                  <el-dropdown-item @click="followUser(user)" divided>
                    跟随光标
                  </el-dropdown-item>
                  <el-dropdown-item
                    v-if="canManageUsers"
                    @click="removeUser(user)"
                    style="color: var(--el-color-danger)"
                  >
                    移除用户
                  </el-dropdown-item>
                </el-dropdown-menu>
              </template>
            </el-dropdown>
          </div>
        </div>
      </div>

      <!-- 邀请表单 -->
      <div v-if="showInviteForm" class="invite-section">
        <el-form @submit.prevent="handleInvite" size="small">
          <el-form-item>
            <el-input
              v-model="inviteEmail"
              placeholder="输入用户邮箱"
              type="email"
            >
              <template #append>
                <el-button @click="handleInvite" :loading="inviting">
                  邀请
                </el-button>
              </template>
            </el-input>
          </el-form-item>
        </el-form>
      </div>
    </div>

    <!-- 权限设置 -->
    <div class="permissions-section" v-if="canManagePermissions">
      <div class="section-header">
        <h4>权限设置</h4>
      </div>
      <div class="permissions-list">
        <el-checkbox v-model="permissions.canEdit" size="small">
          允许编辑
        </el-checkbox>
        <el-checkbox v-model="permissions.canComment" size="small">
          允许评论
        </el-checkbox>
        <el-checkbox v-model="permissions.canShare" size="small">
          允许分享
        </el-checkbox>
      </div>
    </div>

    <!-- 会话信息 -->
    <div class="session-section">
      <div class="section-header">
        <h4>会话信息</h4>
      </div>
      <div class="session-info">
        <div class="info-item">
          <span class="label">会话ID:</span>
          <span class="value">{{ session?.id || '未连接' }}</span>
        </div>
        <div class="info-item">
          <span class="label">创建时间:</span>
          <span class="value">{{ sessionCreatedTime }}</span>
        </div>
        <div class="info-item">
          <span class="label">最后活动:</span>
          <span class="value">{{ lastActivityTime }}</span>
        </div>
      </div>
    </div>

    <!-- 操作按钮 -->
    <div class="panel-actions">
      <el-button-group>
        <el-button
          v-if="!isActive"
          type="primary"
          size="small"
          @click="startCollaboration"
        >
          开始协作
        </el-button>
        <el-button
          v-else
          type="danger"
          size="small"
          @click="leaveCollaboration"
        >
          离开协作
        </el-button>
      </el-button-group>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Plus, Check, MoreFilled } from '@element-plus/icons-vue'

interface CollaborationUser {
  id: string
  name: string
  color: string
  cursor?: {
    line: number
    column: number
  }
  selection?: {
    startLine: number
    startColumn: number
    endLine: number
    endColumn: number
  }
  isOnline: boolean
  lastSeen: number
}

interface CollaborationSession {
  id: string
  documentId: string
  users: Map<string, CollaborationUser>
  activeUsers: string[]
  lastActivity: number
  permissions: {
    canEdit: boolean
    canComment: boolean
    canShare: boolean
  }
  createdAt: number
}

interface Props {
  session: CollaborationSession | null
  users: CollaborationUser[]
}

interface Emits {
  invite: [email: string]
  leave: []
}

const props = defineProps<Props>()
const emit = defineEmits<Emits>()

const showInviteForm = ref(false)
const inviteEmail = ref('')
const inviting = ref(false)
const currentUserId = ref('user-current') // 模拟当前用户ID

// 计算属性
const isActive = computed(() => {
  return props.session !== null && props.users.length > 0
})

const activeUsers = computed(() => {
  return props.users.filter(user => user.isOnline)
})

const canManageUsers = computed(() => {
  return props.session?.permissions.canEdit || false
})

const canManagePermissions = computed(() => {
  return props.session?.permissions.canEdit || false
})

const permissions = computed({
  get: () => props.session?.permissions || {
    canEdit: true,
    canComment: true,
    canShare: true
  },
  set: (value) => {
    // 这里应该更新会话权限
    if (import.meta.env.DEV) console.log('Update permissions:', value)
  }
})

const sessionCreatedTime = computed(() => {
  if (!props.session?.createdAt) return '未知'
  return new Date(props.session.createdAt).toLocaleString()
})

const lastActivityTime = computed(() => {
  if (!props.session?.lastActivity) return '未知'
  return new Date(props.session.lastActivity).toLocaleString()
})

// 方法
function handleInvite() {
  if (!inviteEmail.value) return

  inviting.value = true
  emit('invite', inviteEmail.value)

  // 模拟邀请过程
  setTimeout(() => {
    inviting.value = false
    inviteEmail.value = ''
    showInviteForm.value = false
  }, 1000)
}

function startCollaboration() {
  // 这里应该连接到协作服务
  if (import.meta.env.DEV) console.log('Starting collaboration...')
}

function leaveCollaboration() {
  emit('leave')
}

function sendMessage(user: CollaborationUser) {
  if (import.meta.env.DEV) console.log('Send message to:', user.name)
}

function followUser(user: CollaborationUser) {
  if (import.meta.env.DEV) console.log('Follow user:', user.name)
}

function removeUser(user: CollaborationUser) {
  if (import.meta.env.DEV) console.log('Remove user:', user.name)
}
</script>

<style scoped lang="scss">
.collaboration-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: var(--el-bg-color);
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);

  h3 {
    margin: 0;
    font-size: 16px;
    font-weight: 600;
    color: var(--el-text-color-primary);
  }
}

.user-section {
  flex: 1;
  padding: 16px;

  .section-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 12px;

    h4 {
      margin: 0;
      font-size: 14px;
      font-weight: 600;
      color: var(--el-text-color-primary);
    }
  }
}

.user-list {
  .user-item {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 8px 0;
    border-bottom: 1px solid var(--el-border-color-lighter);

    &:last-child {
      border-bottom: none;
    }

    &.is-current-user {
      background: var(--el-color-primary-light-9);
      margin: 0 -8px;
      padding: 8px;
      border-radius: 4px;
    }
  }

  .user-avatar {
    width: 32px;
    height: 32px;
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    color: white;
    font-weight: 600;
    font-size: 14px;
    flex-shrink: 0;
  }

  .user-info {
    flex: 1;
    min-width: 0;

    .user-name {
      display: flex;
      align-items: center;
      gap: 8px;
      font-weight: 500;
      color: var(--el-text-color-primary);
      margin-bottom: 2px;
    }

    .user-status {
      display: flex;
      align-items: center;
      gap: 4px;
      font-size: 12px;
      color: var(--el-text-color-secondary);

      .online {
        color: var(--el-color-success);
      }

      .offline {
        color: var(--el-color-info);
      }

      .cursor-position {
        margin-left: 8px;
        background: var(--el-bg-color-overlay);
        padding: 1px 4px;
        border-radius: 2px;
      }
    }
  }

  .user-actions {
    flex-shrink: 0;
  }
}

.invite-section {
  margin-top: 12px;
  padding-top: 12px;
  border-top: 1px solid var(--el-border-color-lighter);
}

.permissions-section {
  padding: 16px;
  border-top: 1px solid var(--el-border-color-lighter);

  .section-header {
    margin-bottom: 12px;

    h4 {
      margin: 0;
      font-size: 14px;
      font-weight: 600;
      color: var(--el-text-color-primary);
    }
  }

  .permissions-list {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }
}

.session-section {
  padding: 16px;
  border-top: 1px solid var(--el-border-color-lighter);

  .section-header {
    margin-bottom: 12px;

    h4 {
      margin: 0;
      font-size: 14px;
      font-weight: 600;
      color: var(--el-text-color-primary);
    }
  }

  .session-info {
    .info-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 4px 0;

      .label {
        font-size: 12px;
        color: var(--el-text-color-secondary);
      }

      .value {
        font-size: 12px;
        color: var(--el-text-color-primary);
        font-family: monospace;
      }
    }
  }
}

.panel-actions {
  padding: 16px;
  border-top: 1px solid var(--el-border-color-lighter);
  display: flex;
  justify-content: center;
}
</style>