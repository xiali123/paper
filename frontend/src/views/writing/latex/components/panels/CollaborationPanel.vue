<template>
  <PanelDrawer
    :show="show"
    title="协作编辑"
    direction="rtl"
    size="350px"
    @update:show="$emit('update:show', $event)"
  >
    <div class="collaboration-panel">
      <!-- 在线用户 -->
      <div class="online-users">
        <div class="section-header">
          <h4>在线用户</h4>
          <el-badge :value="onlineUsers.length" :max="99" />
        </div>
        <div class="users-list">
          <div
            v-for="user in onlineUsers"
            :key="user.id"
            class="user-item"
            :class="{ 'is-current-user': user.isCurrentUser }"
          >
            <div class="user-avatar-wrapper">
              <el-avatar :size="36" :src="user.avatar">
                {{ user.name[0] }}
              </el-avatar>
              <el-badge
                v-if="user.cursor"
                :is-dot="true"
                :color="user.cursorColor"
                class="user-status"
              />
            </div>
            <div class="user-info">
              <div class="user-name">{{ user.name }}</div>
              <div class="user-status-text">
                <el-icon v-if="user.isTyping" class="typing-icon"><Edit /></el-icon>
                {{ user.statusText }}
              </div>
            </div>
            <div class="user-cursor-info" v-if="user.cursor">
              <span>第 {{ user.cursor.line }} 行</span>
            </div>
          </div>
        </div>
      </div>

      <!-- 协作活动 -->
      <div class="activity-feed">
        <div class="section-header">
          <h4>活动动态</h4>
          <el-button text size="small" @click="clearActivity">
            <el-icon><Delete /></el-icon>
            清除
          </el-button>
        </div>
        <div class="activity-list">
          <div
            v-for="activity in recentActivities"
            :key="activity.id"
            class="activity-item"
          >
            <div class="activity-icon" :class="`type-${activity.type}`">
              <el-icon v-if="activity.type === 'edit'"><Edit /></el-icon>
              <el-icon v-else-if="activity.type === 'comment'"><ChatDotSquare /></el-icon>
              <el-icon v-else-if="activity.type === 'join'"><UserFilled /></el-icon>
              <el-icon v-else-if="activity.type === 'leave'"><User /></el-icon>
            </div>
            <div class="activity-content">
              <div class="activity-header">
                <span class="activity-user">{{ activity.userName }}</span>
                <span class="activity-time">{{ formatActivityTime(activity.timestamp) }}</span>
              </div>
              <div class="activity-message">{{ activity.message }}</div>
              <div v-if="activity.diff" class="activity-diff" @click="showActivityDiff(activity)">
                <code>{{ activity.diff }}</code>
              </div>
            </div>
          </div>
          <el-empty v-if="recentActivities.length === 0" description="暂无活动" :image-size="60" />
        </div>
      </div>

      <!-- 聊天 -->
      <div class="collaboration-chat">
        <div class="section-header">
          <h4>团队聊天</h4>
          <el-badge :value="unreadCount" :hidden="unreadCount === 0" />
        </div>
        <div class="chat-messages" ref="messagesContainer">
          <div
            v-for="message in chatMessages"
            :key="message.id"
            class="message-item"
            :class="{ 'is-own': message.isOwn }"
          >
            <el-avatar :size="28" :src="message.avatar">
              {{ message.userName[0] }}
            </el-avatar>
            <div class="message-content">
              <div class="message-header">
                <span class="message-sender">{{ message.userName }}</span>
                <span class="message-time">{{ formatMessageTime(message.timestamp) }}</span>
              </div>
              <div class="message-text">{{ message.text }}</div>
            </div>
          </div>
        </div>
        <div class="chat-input">
          <el-input
            v-model="chatInput"
            placeholder="输入消息..."
            @keyup.enter="sendMessage"
          >
            <template #append>
              <el-button @click="sendMessage" :disabled="!chatInput.trim()">
                <el-icon><Position /></el-icon>
              </el-button>
            </template>
          </el-input>
        </div>
      </div>
    </div>
  </PanelDrawer>
</template>

<script setup lang="ts">
import { ref, computed, watch, nextTick } from 'vue'
import {
  Edit, ChatDotSquare, UserFilled, User, Delete, Position
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import PanelDrawer from './PanelDrawer.vue'

interface OnlineUser {
  id: string | number
  name: string
  avatar?: string
  cursor?: {
    line: number
    column: number
  }
  cursorColor?: string
  isTyping: boolean
  statusText: string
  isCurrentUser?: boolean
}

interface Activity {
  id: string | number
  type: 'edit' | 'comment' | 'join' | 'leave'
  userName: string
  userAvatar?: string
  message: string
  timestamp: number
  diff?: string
  line?: number
}

interface ChatMessage {
  id: string | number
  userName: string
  avatar?: string
  text: string
  timestamp: number
  isOwn: boolean
}

interface Props {
  show: boolean
  projectId?: string | number
  documentId?: string
  currentUser?: string
}

const props = withDefaults(defineProps<Props>(), {
  currentUser: 'User'
})

const emit = defineEmits<{
  'update:show': [value: boolean]
  'send-message': [message: string]
  'navigate-activity': [activity: Activity]
}>()

// 状态
const onlineUsers = ref<OnlineUser[]>([
  {
    id: 1,
    name: '当前用户',
    avatar: '',
    cursor: { line: 25, column: 10 },
    cursorColor: '#67C23A',
    isTyping: false,
    statusText: '编辑中',
    isCurrentUser: true
  },
  {
    id: 2,
    name: '协作者 A',
    avatar: '',
    cursor: { line: 42, column: 15 },
    cursorColor: '#409EFF',
    isTyping: true,
    statusText: '正在输入...'
  },
  {
    id: 3,
    name: '协作者 B',
    avatar: '',
    isTyping: false,
    statusText: '在线'
  }
])

const recentActivities = ref<Activity[]>([
  {
    id: 1,
    type: 'edit',
    userName: '协作者 A',
    message: '修改了第 25 行',
    timestamp: Date.now() - 30000,
    diff: '- 示例文本',
    line: 25
  },
  {
    id: 2,
    type: 'comment',
    userName: '协作者 B',
    message: '添加了评论: "这里需要更多说明"',
    timestamp: Date.now() - 60000
  },
  {
    id: 3,
    type: 'join',
    userName: '协作者 C',
    message: '加入了协作编辑',
    timestamp: Date.now() - 120000
  }
])

const chatMessages = ref<ChatMessage[]>([
  {
    id: 1,
    userName: '协作者 A',
    text: '大家好！我正在修改引言部分',
    timestamp: Date.now() - 180000,
    isOwn: false
  },
  {
    id: 2,
    userName: '当前用户',
    text: '收到，我会注意检查',
    timestamp: Date.now() - 120000,
    isOwn: true
  }
])

const chatInput = ref('')
const messagesContainer = ref<HTMLElement>()
const unreadCount = ref(0)

// 计算属性
const activitiesByType = computed(() => {
  const grouped: Record<string, Activity[]> = {
    edit: [],
    comment: [],
    join: [],
    leave: []
  }
  recentActivities.value.forEach(activity => {
    if (grouped[activity.type]) {
      grouped[activity.type].push(activity)
    }
  })
  return grouped
})

// 方法
function formatActivityTime(timestamp: number): string {
  const now = Date.now()
  const diff = now - timestamp

  if (diff < 60000) return '刚刚'
  if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`
  if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`
  return new Date(timestamp).toLocaleDateString()
}

function formatMessageTime(timestamp: number): string {
  return new Date(timestamp).toLocaleTimeString('zh-CN', {
    hour: '2-digit',
    minute: '2-digit'
  })
}

function sendMessage() {
  if (!chatInput.value.trim()) {
    return
  }

  const message: ChatMessage = {
    id: Date.now(),
    userName: props.currentUser,
    text: chatInput.value,
    timestamp: Date.now(),
    isOwn: true
  }

  chatMessages.value.push(message)
  emit('send-message', chatInput.value)

  chatInput.value = ''

  // 滚动到底部
  nextTick(() => {
    if (messagesContainer.value) {
      messagesContainer.value.scrollTop = messagesContainer.value.scrollHeight
    }
  })
}

function showActivityDiff(activity: Activity) {
  if (activity.line) {
    emit('navigate-activity', activity)
  }
}

function clearActivity() {
  recentActivities.value = []
  ElMessage.success('活动记录已清除')
}

// WebSocket连接模拟
function connectWebSocket() {
  // 实际应该连接到WebSocket服务器
  // ws.onmessage = (event) => {
  //   const data = JSON.parse(event.data)
  //   handleRealtimeUpdate(data)
  // }
}

function handleRealtimeUpdate(data: any) {
  switch (data.type) {
    case 'user_join':
      onlineUsers.value.push(data.user)
      addActivity({
        type: 'join',
        userName: data.user.name,
        message: '加入了协作编辑'
      })
      break
    case 'user_leave':
      onlineUsers.value = onlineUsers.value.filter(u => u.id !== data.user.id)
      break
    case 'cursor_update':
      updateCursorPosition(data.user.id, data.cursor)
      break
    case 'typing_start':
      setUserTyping(data.user.id, true)
      break
    case 'typing_stop':
      setUserTyping(data.user.id, false)
      break
    case 'content_change':
      handleContentChange(data)
      break
    case 'new_message':
      chatMessages.value.push(data.message)
      break
  }
}

function updateCursorPosition(userId: string | number, cursor: { line: number; column: number }) {
  const user = onlineUsers.value.find(u => u.id === userId)
  if (user) {
    user.cursor = cursor
  }
}

function setUserTyping(userId: string | number, typing: boolean) {
  const user = onlineUsers.value.find(u => u.id === userId)
  if (user) {
    user.isTyping = typing
    user.statusText = typing ? '正在输入...' : '在线'
  }
}

function handleContentChange(data: any) {
  addActivity({
    type: 'edit',
    userName: data.userName,
    message: `修改了第 ${data.line} 行`,
    diff: data.diff,
    line: data.line
  })
}

function addActivity(activity: Partial<Activity>) {
  const newActivity: Activity = {
    id: Date.now(),
    type: 'edit',
    userName: '',
    message: '',
    timestamp: Date.now(),
    ...activity
  }

  recentActivities.value.unshift(newActivity)

  // 限制活动记录数量
  if (recentActivities.value.length > 50) {
    recentActivities.value = recentActivities.value.slice(0, 50)
  }
}

defineExpose({
  connectWebSocket,
  handleRealtimeUpdate
})
</script>

<style scoped lang="scss">
.collaboration-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;

  h4 {
    margin: 0;
    font-size: 14px;
    font-weight: 500;
  }
}

.online-users {
  padding-bottom: 16px;
  border-bottom: 1px solid var(--el-border-color);
}

.users-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.user-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px;
  border-radius: 8px;
  transition: background 0.2s;

  &:hover {
    background: var(--el-fill-color-light);
  }

  &.is-current-user {
    background: var(--el-color-primary-light-9);
  }
}

.user-avatar-wrapper {
  position: relative;
}

.user-status {
  position: absolute;
  bottom: -2px;
  right: -2px;
}

.user-info {
  flex: 1;
  min-width: 0;
}

.user-name {
  font-size: 13px;
  font-weight: 500;
}

.user-status-text {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 12px;
  color: var(--el-text-color-secondary);

  .typing-icon {
    font-size: 12px;
    animation: pulse 1s infinite;
  }
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.5; }
}

.user-cursor-info {
  font-size: 11px;
  color: var(--el-text-color-secondary);
}

.activity-feed {
  flex: 1;
  overflow-y: auto;
  padding: 16px 0;
}

.activity-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.activity-item {
  display: flex;
  gap: 10px;
  padding: 8px;
  border-radius: 6px;
  transition: background 0.2s;

  &:hover {
    background: var(--el-fill-color-light);
  }
}

.activity-icon {
  flex-shrink: 0;
  width: 28px;
  height: 28px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  color: #fff;

  &.type-edit {
    background: var(--el-color-primary);
  }

  &.type-comment {
    background: var(--el-color-warning);
  }

  &.type-join {
    background: var(--el-color-success);
  }

  &.type-leave {
    background: var(--el-color-info);
  }
}

.activity-content {
  flex: 1;
  min-width: 0;
}

.activity-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 4px;
}

.activity-user {
  font-size: 13px;
  font-weight: 500;
}

.activity-time {
  font-size: 11px;
  color: var(--el-text-color-secondary);
}

.activity-message {
  font-size: 13px;
  color: var(--el-text-color-regular);
  line-height: 1.5;
}

.activity-diff {
  margin-top: 4px;
  padding: 4px;
  background: var(--el-fill-color-light);
  border-radius: 4px;
  font-size: 12px;
  cursor: pointer;

  code {
    background: var(--el-bg-color);
    padding: 2px 4px;
    border-radius: 3px;
  }

  &:hover {
    background: var(--el-fill-color);
  }
}

.collaboration-chat {
  padding-top: 16px;
  border-top: 1px solid var(--el-border-color);
  display: flex;
  flex-direction: column;
  height: 300px;
}

.chat-messages {
  flex: 1;
  overflow-y: auto;
  padding: 8px 0;
  margin-bottom: 12px;
}

.message-item {
  display: flex;
  gap: 8px;
  margin-bottom: 12px;

  &.is-own {
    flex-direction: row-reverse;
  }
}

.message-content {
  max-width: 75%;
}

.message-header {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 4px;
}

.message-sender {
  font-size: 12px;
  font-weight: 500;
}

.message-time {
  font-size: 11px;
  color: var(--el-text-color-secondary);
}

.message-text {
  padding: 8px 12px;
  background: var(--el-fill-color-light);
  border-radius: 8px;
  font-size: 13px;
  line-height: 1.5;
  word-break: break-word;
}

.is-own .message-content {
  .message-text {
    background: var(--el-color-primary);
    color: #fff;
  }
}

.chat-input {
  flex-shrink: 0;
}
</style>
