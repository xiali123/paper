<template>
  <div class="ai-chat-panel">
    <div class="chat-container">
      <!-- 消息列表 -->
      <div class="messages-wrapper">
        <div
          v-for="message in messages"
          :key="message.id"
          :class="['message', message.role]"
        >
          <div class="message-avatar">
            <span v-if="message.role === 'user'">👤</span>
            <span v-else>🤖</span>
          </div>
          <div class="message-content">
            <div class="message-text">{{ message.content }}</div>
            <div class="message-time">{{ formatTime(message.timestamp) }}</div>
          </div>
        </div>

        <!-- 打字效果 -->
        <div v-if="isTyping" class="message assistant">
          <div class="message-avatar">
            <span>🤖</span>
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

      <!-- 输入框 -->
      <div class="input-wrapper">
        <el-input
          v-model="inputMessage"
          type="textarea"
          :rows="3"
          placeholder="向AI助手提问..."
          @keydown.enter.exact="sendMessage"
        />
        <el-button
          type="primary"
          :loading="loading"
          :disabled="!inputMessage.trim()"
          @click="sendMessage"
        >
          发送
        </el-button>
      </div>
    </div>

    <!-- 快捷问题 -->
    <div class="quick-questions">
      <h4>快捷提问</h4>
      <div class="question-list">
        <el-button
          v-for="(question, index) in quickQuestions"
          :key="index"
          text
          @click="askQuestion(question)"
        >
          {{ question }}
        </el-button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { ElMessage } from 'element-plus'
import { aiCopilotApi } from '@/api/modules/aiCopilot'
import type { AIChatMessage } from '@/types/ai'

const messages = ref<AIChatMessage[]>([
  {
    id: '1',
    role: 'assistant',
    content: '您好！我是AI研究助手，可以帮您解答学术问题、提供研究建议、分析论文内容。请问有什么可以帮助您的？',
    timestamp: new Date().toISOString()
  }
])

const inputMessage = ref('')
const loading = ref(false)
const isTyping = ref(false)

const quickQuestions = [
  '如何撰写一篇高质量的学术论文？',
  '这篇论文的创新点是什么？',
  '推荐一些相关的研究方向',
  '如何提高论文的引用率？'
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

const formatTime = (timestamp: string) => {
  const date = new Date(timestamp)
  const now = new Date()
  const diff = now.getTime() - date.getTime()

  if (diff < 60000) return '刚刚'
  if (diff < 3600000) return `${Math.floor(diff / 60000)}分钟前`
  if (diff < 86400000) return `${Math.floor(diff / 3600000)}小时前`
  return date.toLocaleDateString()
}
</script>

<style scoped>
.ai-chat-panel {
  display: flex;
  gap: 24px;
  height: 100%;
}

.chat-container {
  flex: 1;
  display: flex;
  flex-direction: column;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  overflow: hidden;
}

.messages-wrapper {
  flex: 1;
  overflow-y: auto;
  padding: 24px;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.message {
  display: flex;
  gap: 12px;
  max-width: 80%;
}

.message.user {
  align-self: flex-end;
  flex-direction: row-reverse;
}

.message-avatar {
  width: 40px;
  height: 40px;
  border-radius: 50%;
  background: #f0f0f0;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 20px;
  flex-shrink: 0;
}

.message-content {
  flex: 1;
}

.message.user .message-content {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
}

.message-text {
  padding: 12px 16px;
  background: #f5f7fa;
  border-radius: 12px;
  line-height: 1.6;
  color: #303133;
}

.message.user .message-text {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.message-time {
  font-size: 12px;
  color: #909399;
  margin-top: 4px;
}

.typing-indicator {
  display: flex;
  gap: 4px;
  padding: 12px 16px;
  background: #f5f7fa;
  border-radius: 12px;
  width: fit-content;
}

.typing-indicator span {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #909399;
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

.input-wrapper {
  display: flex;
  gap: 12px;
  padding: 16px 24px;
  background: #f5f7fa;
  border-top: 1px solid #e4e7ed;
}

.input-wrapper .el-button {
  align-self: flex-end;
}

.quick-questions {
  width: 300px;
  background: white;
  border-radius: 12px;
  padding: 24px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.quick-questions h4 {
  font-size: 16px;
  font-weight: 600;
  margin: 0 0 16px 0;
}

.question-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.question-list .el-button {
  text-align: left;
  white-space: normal;
  padding: 12px;
  border-radius: 8px;
  transition: all 0.3s ease;
}

.question-list .el-button:hover {
  background: #f0f0f0;
}
</style>
