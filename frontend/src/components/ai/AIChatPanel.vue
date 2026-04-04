<template>
  <div class="ai-chat-panel">
    <!-- 聊天主区域 -->
    <div class="chat-container">
      <!-- 欢迎头部 -->
      <div class="chat-header">
        <div class="header-info">
          <div class="ai-avatar">
            <el-icon :size="28"><ChatDotRound /></el-icon>
          </div>
          <div class="header-text">
            <h3 class="ai-title">AI 研究助手</h3>
            <p class="ai-status">在线 · 随时为您服务</p>
          </div>
        </div>
        <div class="header-actions">
          <el-button text @click="handleClearHistory">
            <el-icon><Delete /></el-icon>
            清空对话
          </el-button>
        </div>
      </div>

      <!-- 消息列表 -->
      <div class="messages-wrapper">
        <div
          v-for="message in messages"
          :key="message.id"
          :class="['message', message.role]"
        >
          <div class="message-avatar" :class="message.role">
            <el-icon v-if="message.role === 'user'"><User /></el-icon>
            <el-icon v-else><ChatDotRound /></el-icon>
          </div>
          <div class="message-content">
            <div class="message-text" v-html="formatMessage(message.content)"></div>
            <div class="message-time">{{ formatTime(message.timestamp) }}</div>
          </div>
        </div>

        <!-- AI打字效果 -->
        <div v-if="isTyping" class="message assistant">
          <div class="message-avatar assistant">
            <el-icon><ChatDotRound /></el-icon>
          </div>
          <div class="message-content">
            <div class="typing-indicator">
              <span></span>
              <span></span>
              <span></span>
            </div>
          </div>
        </div>
      </div>

      <!-- 输入区域 -->
      <div class="input-wrapper">
        <div class="input-container">
          <el-input
            v-model="inputMessage"
            type="textarea"
            :rows="2"
            placeholder="向AI助手提问... (Enter发送，Shift+Enter换行)"
            @keydown.enter.exact="handleEnterKey"
            @keydown.enter.shift.prevent="handleNewLine"
            class="message-input"
          />
          <el-button
            type="primary"
            :loading="loading"
            :disabled="!inputMessage.trim()"
            @click="sendMessage"
            class="send-button"
            :icon="Promotion"
            circle
          />
        </div>
      </div>
    </div>

    <!-- 右侧面板 -->
    <div class="side-panel">
      <!-- 快捷问题卡片 -->
      <el-card class="quick-questions-card" shadow="hover">
        <template #header>
          <div class="card-header">
            <el-icon class="header-icon"><Lightning /></el-icon>
            <span>快捷提问</span>
          </div>
        </template>
        <div class="question-list">
          <el-button
            v-for="(question, index) in quickQuestions"
            :key="index"
            @click="askQuestion(question)"
            class="question-item"
            effect="plain"
          >
            <span class="question-icon">{{ index + 1 }}</span>
            <span class="question-text">{{ question }}</span>
          </el-button>
        </div>
      </el-card>

      <!-- 功能介绍卡片 -->
      <el-card class="features-card" shadow="hover">
        <template #header>
          <div class="card-header">
            <el-icon class="header-icon"><Star /></el-icon>
            <span>功能特点</span>
          </div>
        </template>
        <div class="features-list">
          <div class="feature-item">
            <el-icon class="feature-icon" color="#409EFF"><Document /></el-icon>
            <div class="feature-text">
              <h4>论文分析</h4>
              <p>深度分析论文内容和创新点</p>
            </div>
          </div>
          <div class="feature-item">
            <el-icon class="feature-icon" color="#67C23A"><Reading /></el-icon>
            <div class="feature-text">
              <h4>文献综述</h4>
              <p>自动生成文献综述报告</p>
            </div>
          </div>
          <div class="feature-item">
            <el-icon class="feature-icon" color="#E6A23C"><Notebook /></el-icon>
            <div class="feature-text">
              <h4>研究规划</h4>
              <p>制定个性化研究计划</p>
            </div>
          </div>
        </div>
      </el-card>

      <!-- 使用提示 -->
      <el-card class="tips-card" shadow="hover">
        <template #header>
          <div class="card-header">
            <el-icon class="header-icon"><InfoFilled /></el-icon>
            <span>使用提示</span>
          </div>
        </template>
        <div class="tips-content">
          <ul>
            <li>可以询问学术写作建议</li>
            <li>提供论文创新点分析</li>
            <li>获取研究方法指导</li>
            <li>了解相关研究方向</li>
          </ul>
        </div>
      </el-card>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  ChatDotRound,
  User,
  Delete,
  Lightning,
  Star,
  InfoFilled,
  Document,
  Reading,
  Notebook,
  Promotion
} from '@element-plus/icons-vue'
import { aiCopilotApi } from '@/api/modules/aiCopilot'
import type { AIChatMessage } from '@/types/ai'

const messages = ref<AIChatMessage[]>([
  {
    id: '1',
    role: 'assistant',
    content: '您好！我是AI研究助手，可以帮您：<br>• 📝 解答学术写作问题<br>• 🔍 分析论文创新点<br>• 💡 提供研究建议<br>• 📚 生成文献综述<br><br>请问有什么可以帮助您的？',
    timestamp: new Date().toISOString()
  }
])

const inputMessage = ref('')
const loading = ref(false)
const isTyping = ref(false)

const quickQuestions = [
  '如何撰写一篇高质量的学术论文？',
  '怎样提升论文的创新性和学术价值？',
  '推荐一些当前热门的研究方向',
  '如何有效提升论文的引用率？'
]

const sendMessage = async () => {
  if (!inputMessage.value.trim() || loading.value) return

  const userMessage: AIChatMessage = {
    id: Date.now().toString(),
    role: 'user',
    content: inputMessage.value,
    timestamp: new Date().toISOString()
  }

  messages.value.push(userMessage)
  const question = inputMessage.value
  inputMessage.value = ''
  loading.value = true
  isTyping.value = true

  try {
    const response = await aiCopilotApi.chat({
      message: question
    })

    isTyping.value = false
    const assistantMessage: AIChatMessage = {
      id: (Date.now() + 1).toString(),
      role: 'assistant',
      content: response.content,
      timestamp: response.timestamp
    }
    messages.value.push(assistantMessage)
  } catch (error) {
    isTyping.value = false
    ElMessage.error('发送失败，请稍后重试')
  } finally {
    loading.value = false
  }
}

const askQuestion = (question: string) => {
  inputMessage.value = question
  sendMessage()
}

const handleEnterKey = (event: Event | KeyboardEvent) => {
  // 检查是否是键盘事件且没有按Shift键
  if ('key' in event && !event.shiftKey) {
    sendMessage()
  }
}

const handleNewLine = () => {
  // Shift+Enter 会自动换行，不需要额外处理
}

const handleClearHistory = () => {
  ElMessageBox.confirm(
    '确定要清空所有对话记录吗？',
    '清空对话',
    {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning',
      confirmButtonClass: 'el-button--danger'
    }
  ).then(() => {
    messages.value = [
      {
        id: '1',
        role: 'assistant',
        content: '您好！我是AI研究助手，可以帮您：<br>• 📝 解答学术写作问题<br>• 🔍 分析论文创新点<br>• 💡 提供研究建议<br>• 📚 生成文献综述<br><br>请问有什么可以帮助您的？',
        timestamp: new Date().toISOString()
      }
    ]
    ElMessage.success('对话记录已清空')
  }).catch(() => {})
}

const formatTime = (timestamp: string) => {
  const date = new Date(timestamp)
  const now = new Date()
  const diff = now.getTime() - date.getTime()

  if (diff < 60000) return '刚刚'
  if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`
  if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`
  return date.toLocaleDateString()
}

const formatMessage = (content: string) => {
  // 将换行符转换为<br>标签
  return content.replace(/\n/g, '<br>')
}
</script>

<style scoped lang="scss">
.ai-chat-panel {
  display: flex;
  gap: $spacing-6;
  height: calc(100vh - 120px);
  min-height: 600px;
}

// 聊天主容器
.chat-container {
  flex: 1;
  display: flex;
  flex-direction: column;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border-radius: $border-radius-2xl;
  box-shadow: $shadow-2xl;
  border: 2px solid $border-light;
  overflow: hidden;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    border-color: $gray-700;
  }
}

// 聊天头部
.chat-header {
  padding: $spacing-6 $spacing-8;
  background: linear-gradient(135deg, rgba($primary-50, 0.8) 0%, rgba($primary-100, 0.4) 100%);
  border-bottom: 2px solid rgba($primary-200, 0.5);
  display: flex;
  justify-content: space-between;
  align-items: center;

  .dark & {
    background: linear-gradient(135deg, rgba($gray-700, 0.6) 0%, rgba($gray-800, 0.4) 100%);
    border-bottom-color: rgba($gray-600, 0.5);
  }
}

.header-info {
  display: flex;
  align-items: center;
  gap: $spacing-4;
}

.ai-avatar {
  width: 48px;
  height: 48px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  border-radius: $border-radius-full;
  color: #ffffff;
  box-shadow: $shadow-md;
}

.header-text {
  .ai-title {
    margin: 0 0 $spacing-1 0;
    font-size: $font-size-lg;
    font-weight: $font-weight-bold;
    color: $text-primary;

    .dark & {
      color: $gray-100;
    }
  }

  .ai-status {
    margin: 0;
    font-size: $font-size-sm;
    color: $success-color;
    font-weight: $font-weight-medium;

    &::before {
      content: '●';
      margin-right: $spacing-2;
    }
  }
}

.header-actions {
  .el-button {
    color: $text-secondary;
    font-weight: $font-weight-medium;

    &:hover {
      color: $danger-color;
    }
  }
}

// 消息区域
.messages-wrapper {
  flex: 1;
  overflow-y: auto;
  padding: $spacing-8;
  display: flex;
  flex-direction: column;
  gap: $spacing-6;

  // 自定义滚动条
  &::-webkit-scrollbar {
    width: 8px;
  }

  &::-webkit-scrollbar-track {
    background: transparent;
  }

  &::-webkit-scrollbar-thumb {
    background: rgba($gray-400, 0.3);
    border-radius: $border-radius-full;

    &:hover {
      background: rgba($gray-400, 0.5);
    }
  }
}

.message {
  display: flex;
  gap: $spacing-4;
  max-width: 85%;
  animation: messageSlideIn 0.3s ease-out;
}

@keyframes messageSlideIn {
  from {
    opacity: 0;
    transform: translateY(10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.message.user {
  align-self: flex-end;
  flex-direction: row-reverse;
}

.message-avatar {
  width: 40px;
  height: 40px;
  border-radius: $border-radius-full;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  box-shadow: $shadow-sm;

  &.user {
    background: linear-gradient(135deg, $gray-400 0%, $gray-500 100%);
    color: #ffffff;

    .dark & {
      background: linear-gradient(135deg, $gray-600 0%, $gray-700 100%);
    }
  }

  &.assistant {
    background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
    color: #ffffff;
  }
}

.message-content {
  flex: 1;
  display: flex;
  flex-direction: column;
}

.message.user .message-content {
  align-items: flex-end;
}

.message-text {
  padding: $spacing-4 $spacing-5;
  background: $gray-50;
  border-radius: $border-radius-lg;
  line-height: 1.6;
  color: $text-primary;
  font-size: $font-size-base;
  box-shadow: $shadow-sm;
  word-wrap: break-word;

  .dark & {
    background: $gray-700;
    color: $gray-100;
  }
}

.message.user .message-text {
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  color: #ffffff;
  box-shadow: $shadow-md;
}

.message-time {
  font-size: $font-size-xs;
  color: $text-secondary;
  margin-top: $spacing-2;
  padding: 0 $spacing-1;
}

// 打字效果
.typing-indicator {
  display: flex;
  gap: $spacing-2;
  padding: $spacing-4 $spacing-5;
  background: $gray-50;
  border-radius: $border-radius-lg;
  width: fit-content;

  .dark & {
    background: $gray-700;
  }
}

.typing-indicator span {
  width: 8px;
  height: 8px;
  border-radius: $border-radius-full;
  background: $primary-500;
  animation: typing 1.4s infinite;
}

.typing-indicator span:nth-child(2) {
  animation-delay: 0.2s;
}

.typing-indicator span:nth-child(3) {
  animation-delay: 0.4s;
}

@keyframes typing {
  0%, 60%, 100% {
    transform: translateY(0);
  }
  30% {
    transform: translateY(-10px);
  }
}

// 输入区域
.input-wrapper {
  padding: $spacing-6 $spacing-8;
  background: linear-gradient(180deg, rgba($primary-50, 0.6) 0%, rgba($gray-50, 0.4) 100%);
  border-top: 2px solid rgba($primary-200, 0.4);

  .dark & {
    background: linear-gradient(180deg, rgba($gray-700, 0.6) 0%, rgba($gray-800, 0.4) 100%);
    border-top-color: rgba($gray-600, 0.5);
  }
}

.input-container {
  display: flex;
  gap: $spacing-4;
  align-items: flex-end;
}

.message-input {
  flex: 1;

  .el-textarea__inner {
    border-radius: $border-radius-lg;
    border: 2px solid $border-light;
    box-shadow: $shadow-sm;
    transition: all $duration-fast;

    &:focus {
      border-color: $primary-500;
      box-shadow: 0 0 0 4px rgba($primary-500, 0.1);
    }

    .dark & {
      background: $gray-700;
      border-color: $gray-600;

      &:focus {
        border-color: $primary-400;
      }
    }
  }
}

.send-button {
  width: 48px;
  height: 48px;
  flex-shrink: 0;
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  border: none;

  &:hover:not(:disabled) {
    transform: scale(1.05);
    box-shadow: $shadow-lg;
  }

  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }
}

// 右侧面板
.side-panel {
  width: 320px;
  display: flex;
  flex-direction: column;
  gap: $spacing-6;
}

// 快捷问题卡片
.quick-questions-card,
.features-card,
.tips-card {
  border-radius: $border-radius-xl;
  border: 2px solid $border-light;
  transition: all $duration-fast;

  &:hover {
    box-shadow: $shadow-lg;
    transform: translateY(-2px);
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }
}

.card-header {
  display: flex;
  align-items: center;
  gap: $spacing-3;
  font-size: $font-size-base;
  font-weight: $font-weight-bold;
  color: $text-primary;

  .dark & {
    color: $gray-100;
  }
}

.header-icon {
  color: $primary-500;
  font-size: $font-size-xl;
}

// 快捷问题
.question-list {
  display: flex;
  flex-direction: column;
  gap: $spacing-3;
}

.question-item {
  width: 100%;
  display: flex;
  align-items: center;
  gap: $spacing-3;
  padding: $spacing-4;
  border: 2px solid $border-light;
  border-radius: $border-radius-lg;
  transition: all $duration-fast;
  text-align: left;
  height: auto;
  white-space: normal;
  font-size: $font-size-sm;
  color: $text-primary;

  &:hover {
    background: $primary-50;
    border-color: $primary-300;
    transform: translateX(4px);
  }

  .dark & {
    background: $gray-700;
    border-color: $gray-600;
    color: $gray-100;

    &:hover {
      background: rgba($primary-900, 0.2);
      border-color: $primary-500;
    }
  }
}

.question-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 24px;
  height: 24px;
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  color: #ffffff;
  border-radius: $border-radius-full;
  font-size: $font-size-xs;
  font-weight: $font-weight-bold;
  flex-shrink: 0;
}

.question-text {
  flex: 1;
  line-height: 1.4;
}

// 功能特点
.features-list {
  display: flex;
  flex-direction: column;
  gap: $spacing-5;
}

.feature-item {
  display: flex;
  gap: $spacing-4;
  align-items: flex-start;
}

.feature-icon {
  font-size: $font-size-2xl;
  flex-shrink: 0;
  margin-top: $spacing-1;
}

.feature-text {
  h4 {
    margin: 0 0 $spacing-1 0;
    font-size: $font-size-base;
    font-weight: $font-weight-semibold;
    color: $text-primary;

    .dark & {
      color: $gray-100;
    }
  }

  p {
    margin: 0;
    font-size: $font-size-sm;
    color: $text-secondary;
    line-height: 1.5;
  }
}

// 使用提示
.tips-content {
  ul {
    margin: 0;
    padding-left: $spacing-5;

    li {
      font-size: $font-size-sm;
      color: $text-regular;
      line-height: 1.6;
      margin-bottom: $spacing-2;
      position: relative;

      &::marker {
        color: $primary-500;
      }
    }
  }
}

// 响应式设计
@media (max-width: 1200px) {
  .ai-chat-panel {
    flex-direction: column;
  }

  .side-panel {
    width: 100%;
    flex-direction: row;
    flex-wrap: wrap;
  }

  .quick-questions-card,
  .features-card,
  .tips-card {
    flex: 1;
    min-width: 280px;
  }
}

@media (max-width: 768px) {
  .ai-chat-panel {
    height: auto;
    min-height: auto;
  }

  .chat-container {
    min-height: 500px;
  }

  .side-panel {
    flex-direction: column;
  }

  .message {
    max-width: 95%;
  }
}
</style>
