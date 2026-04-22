<template>
  <el-dialog
    v-model="visible"
    title="AI 公式识别"
    width="600px"
    :close-on-click-modal="false"
    class="ai-formula-dialog"
  >
    <div class="formula-recognizer">
      <!-- 上传区域 -->
      <div class="upload-section">
        <el-upload
          ref="uploadRef"
          class="formula-upload"
          :show-file-list="false"
          :before-upload="beforeUpload"
          :http-request="handleUpload"
          accept="image/*"
          drag
        >
          <div class="upload-content" v-if="!previewImage">
            <el-icon class="upload-icon"><UploadFilled /></el-icon>
            <div class="upload-text">
              <div class="upload-title">拖拽图片到此处</div>
              <div class="upload-hint">或点击上传</div>
              <div class="upload-support">支持 JPG、PNG、WEBP 格式</div>
            </div>
          </div>
          <div class="upload-preview" v-else>
            <img :src="previewImage" alt="Preview" />
            <div class="preview-overlay" @click.stop="clearImage">
              <el-icon><Delete /></el-icon>
              <span>重新上传</span>
            </div>
          </div>
        </el-upload>
      </div>

      <!-- 识别结果 -->
      <div class="result-section" v-if="result">
        <div class="result-header">
          <h4>识别结果</h4>
          <div class="result-actions">
            <el-button
              size="small"
              @click="copyResult"
              :disabled="!result.latex"
            >
              <el-icon><DocumentCopy /></el-icon>
              复制
            </el-button>
            <el-button
              size="small"
              type="primary"
              @click="insertResult"
              :disabled="!result.latex"
            >
              <el-icon><Plus /></el-icon>
              插入
            </el-button>
          </div>
        </div>

        <div class="result-content">
          <!-- LaTeX 代码 -->
          <div class="latex-code">
            <div class="code-header">
              <span>LaTeX 代码</span>
              <el-tag v-if="result.confidence" size="small" type="success">
                置信度: {{ (result.confidence * 100).toFixed(1) }}%
              </el-tag>
            </div>
            <el-input
              v-model="result.latex"
              type="textarea"
              :rows="4"
              placeholder="识别的 LaTeX 公式代码"
              @input="onLatexChange"
            />
          </div>

          <!-- 预览 -->
          <div class="latex-preview">
            <div class="preview-header">预览</div>
            <div class="preview-content" ref="previewRef" v-html="renderedFormula"></div>
          </div>
        </div>

        <!-- 候选结果 -->
        <div class="candidates-section" v-if="result.candidates && result.candidates.length > 1">
          <div class="candidates-header">其他可能</div>
          <div class="candidates-list">
            <div
              v-for="(candidate, index) in result.candidates"
              :key="index"
              class="candidate-item"
              :class="{ active: candidate.latex === result.latex }"
              @click="selectCandidate(candidate)"
            >
              <div class="candidate-confidence">
                {{ (candidate.confidence * 100).toFixed(1) }}%
              </div>
              <div class="candidate-latex">{{ candidate.latex }}</div>
            </div>
          </div>
        </div>
      </div>

      <!-- 加载状态 -->
      <div class="loading-section" v-if="recognizing">
        <el-icon class="is-loading" :size="40"><Loading /></el-icon>
        <p>AI 正在识别公式...</p>
        <p class="loading-hint">这可能需要几秒钟</p>
      </div>

      <!-- 错误提示 -->
      <el-alert
        v-if="error"
        type="error"
        :title="error"
        :closable="false"
        show-icon
      />
    </div>

    <template #footer>
      <el-button @click="visible = false">取消</el-button>
      <el-button
        type="primary"
        @click="recognizeFormula"
        :disabled="!uploadedFile || recognizing"
        :loading="recognizing"
      >
        识别公式
      </el-button>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, computed, nextTick } from 'vue'
import {
  UploadFilled,
  Delete,
  DocumentCopy,
  Plus,
  Loading
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import axios from 'axios'

interface FormulaCandidate {
  latex: string
  confidence: number
}

interface RecognizeResult {
  latex: string
  confidence: number
  candidates?: FormulaCandidate[]
}

const emit = defineEmits<{
  insert: [latex: string]
}>()

const visible = ref(false)
const uploadedFile = ref<File | null>(null)
const previewImage = ref<string>('')
const recognizing = ref(false)
const error = ref<string>('')
const result = ref<RecognizeResult | null>(null)
const previewRef = ref<HTMLElement>()

// 上传前验证
const beforeUpload = (file: File) => {
  const isImage = file.type.startsWith('image/')
  const isLt5M = file.size / 1024 / 1024 < 5

  if (!isImage) {
    ElMessage.error('只能上传图片文件!')
    return false
  }
  if (!isLt5M) {
    ElMessage.error('图片大小不能超过 5MB!')
    return false
  }

  uploadedFile.value = file
  error.value = ''
  result.value = null

  // 生成预览
  const reader = new FileReader()
  reader.onload = (e) => {
    previewImage.value = e.target?.result as string
  }
  reader.readAsDataURL(file)

  return false // 阻止自动上传
}

// 处理上传（占位，实际在识别时处理）
const handleUpload = () => {
  // 占位函数
}

// 清除图片
const clearImage = () => {
  uploadedFile.value = null
  previewImage.value = ''
  result.value = null
  error.value = ''
}

// 识别公式
const recognizeFormula = async () => {
  if (!uploadedFile.value) {
    error.value = '请先上传图片'
    return
  }

  recognizing.value = true
  error.value = ''

  try {
    // 转换图片为 base64
    const base64Data = await new Promise<string>((resolve) => {
      const reader = new FileReader()
      reader.onload = (e) => {
        const data = e.target?.result as string
        resolve(data)
      }
      reader.readAsDataURL(uploadedFile.value!)
    })

    // 调用后端 API 进行识别
    // 注意：这里需要后端实现相应的 AI 公式识别接口
    const response = await axios.post('/api/latex/recognize-formula', {
      image: base64Data,
      format: 'latex'
    })

    if (response.data.success) {
      result.value = {
        latex: response.data.data.latex,
        confidence: response.data.data.confidence || 0.9,
        candidates: response.data.data.candidates || [
          {
            latex: response.data.data.latex,
            confidence: response.data.data.confidence || 0.9
          }
        ]
      }
      ElMessage.success('公式识别成功!')
    } else {
      throw new Error(response.data.message || '识别失败')
    }
  } catch (err: any) {
    error.value = err.response?.data?.message || err.message || '识别失败，请重试'
    ElMessage.error(error.value)

    // 如果 API 不可用，使用模拟数据（演示用）
    simulateRecognition()
  } finally {
    recognizing.value = false
  }
}

// 模拟识别结果（当 API 不可用时）
const simulateRecognition = () => {
  setTimeout(() => {
    result.value = {
      latex: '\\frac{d}{dx}\\left( \\int_{a}^{x} f(t) \\, dt \\right) = f(x)',
      confidence: 0.95,
      candidates: [
        {
          latex: '\\frac{d}{dx}\\left( \\int_{a}^{x} f(t) \\, dt \\right) = f(x)',
          confidence: 0.95
        },
        {
          latex: '\\frac{d}{dx} \\int_{a}^{x} f(t) \\, dt = f(x)',
          confidence: 0.85
        }
      ]
    }
    ElMessage.warning('使用模拟数据（API 不可用）')
  }, 1500)
}

// 渲染预览
const renderedFormula = computed(() => {
  if (!result.value?.latex) return ''

  // 使用 KaTeX 渲染（需要安装 katex）
  try {
    // 这里需要集成 KaTeX 或 MathJax
    // 简单演示：返回带格式的文本
    return `<span style="font-family: 'Times New Roman', serif; font-size: 18px;">${result.value.latex}</span>`
  } catch (err) {
    return result.value.latex
  }
})

// LaTeX 代码变化时更新预览
const onLatexChange = () => {
  nextTick(() => {
    // 重新渲染预览
  })
}

// 选择候选结果
const selectCandidate = (candidate: FormulaCandidate) => {
  if (result.value) {
    result.value.latex = candidate.latex
    result.value.confidence = candidate.confidence
  }
}

// 复制结果
const copyResult = async () => {
  if (!result.value?.latex) return

  try {
    await navigator.clipboard.writeText(result.value.latex)
    ElMessage.success('已复制到剪贴板')
  } catch (err) {
    ElMessage.error('复制失败')
  }
}

// 插入结果
const insertResult = () => {
  if (!result.value?.latex) return

  emit('insert', result.value.latex)
  ElMessage.success('已插入公式')
  visible.value = false
}

// 打开对话框
const open = () => {
  visible.value = true
  // 重置状态
  uploadedFile.value = null
  previewImage.value = ''
  result.value = null
  error.value = ''
}

defineExpose({ open })
</script>

<style scoped lang="scss">
.ai-formula-dialog {
  :deep(.el-dialog__body) {
    padding: 20px;
  }
}

.formula-recognizer {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

// 上传区域
.upload-section {
  .formula-upload {
    width: 100%;
  }

  :deep(.el-upload) {
    width: 100%;
  }

  :deep(.el-upload-dragger) {
    width: 100%;
    height: 200px;
    display: flex;
    align-items: center;
    justify-content: center;
    border: 2px dashed var(--el-border-color);
    border-radius: 8px;
    background: var(--el-fill-color-lighter);
    transition: all 0.3s;

    &:hover {
      border-color: var(--el-color-primary);
      background: var(--el-fill-color-light);
    }
  }
}

.upload-content {
  text-align: center;
}

.upload-icon {
  font-size: 48px;
  color: var(--el-text-color-secondary);
  margin-bottom: 16px;
}

.upload-text {
  .upload-title {
    font-size: 16px;
    font-weight: 500;
    color: var(--el-text-color-primary);
    margin-bottom: 4px;
  }

  .upload-hint {
    font-size: 14px;
    color: var(--el-text-color-regular);
    margin-bottom: 8px;
  }

  .upload-support {
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }
}

.upload-preview {
  position: relative;
  width: 100%;
  height: 100%;

  img {
    width: 100%;
    height: 100%;
    object-fit: contain;
  }

  .preview-overlay {
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 8px;
    background: rgba(0, 0, 0, 0.6);
    color: white;
    opacity: 0;
    transition: opacity 0.3s;
    cursor: pointer;

    &:hover {
      opacity: 1;
    }

    .el-icon {
      font-size: 32px;
    }
  }
}

// 结果区域
.result-section {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.result-header {
  display: flex;
  justify-content: space-between;
  align-items: center;

  h4 {
    margin: 0;
    font-size: 16px;
    font-weight: 600;
  }
}

.result-actions {
  display: flex;
  gap: 8px;
}

.result-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.latex-code {
  .code-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 8px;
    font-size: 13px;
    font-weight: 500;
    color: var(--el-text-color-secondary);
  }
}

.latex-preview {
  .preview-header {
    font-size: 13px;
    font-weight: 500;
    color: var(--el-text-color-secondary);
    margin-bottom: 8px;
  }

  .preview-content {
    min-height: 60px;
    padding: 16px;
    background: var(--el-fill-color-lighter);
    border-radius: 6px;
    display: flex;
    align-items: center;
    justify-content: center;
  }
}

// 候选结果
.candidates-section {
  margin-top: 8px;
}

.candidates-header {
  font-size: 13px;
  font-weight: 500;
  color: var(--el-text-color-secondary);
  margin-bottom: 8px;
}

.candidates-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.candidate-item {
  display: flex;
  gap: 12px;
  padding: 10px 12px;
  background: var(--el-fill-color-light);
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s;
  border: 2px solid transparent;

  &:hover {
    background: var(--el-fill-color);
  }

  &.active {
    border-color: var(--el-color-primary);
    background: var(--el-color-primary-light-9);
  }
}

.candidate-confidence {
  flex-shrink: 0;
  font-size: 12px;
  font-weight: 600;
  color: var(--el-color-success);
  padding: 2px 8px;
  background: var(--el-color-success-light-9);
  border-radius: 4px;
}

.candidate-latex {
  flex: 1;
  font-family: 'Consolas', 'Monaco', monospace;
  font-size: 13px;
  color: var(--el-text-color-primary);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

// 加载状态
.loading-section {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 40px 20px;
  gap: 12px;
  text-align: center;

  .el-icon {
    color: var(--el-color-primary);
  }

  p {
    margin: 0;
    color: var(--el-text-color-primary);
    font-size: 14px;
  }

  .loading-hint {
    color: var(--el-text-color-secondary);
    font-size: 12px;
  }
}
</style>
