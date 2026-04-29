<template>
  <el-dialog
    :model-value="show"
    @update:model-value="$emit('update:show', $event)"
    title="欢迎使用 LaTeX 编辑器"
    width="700px"
    :close-on-click-modal="false"
    :show-close="currentStep > 0"
  >
    <div class="welcome-guide">
      <!-- 进度指示 -->
      <div class="step-indicator">
        <div
          v-for="(step, index) in steps"
          :key="index"
          class="step-dot"
          :class="{ active: currentStep === index, completed: index < currentStep }"
        >
          <el-icon v-if="index < currentStep"><CircleCheck /></el-icon>
          <span v-else>{{ index + 1 }}</span>
        </div>
        <div class="step-line"></div>
      </div>

      <!-- 步骤内容 -->
      <div class="step-content">
        <Transition name="fade" mode="out-in">
          <div :key="currentStep">
            <!-- 步骤1: 欢迎 -->
            <div v-if="currentStep === 0" class="step-card">
              <div class="welcome-icon">
                <el-icon :size="80"><EditPen /></el-icon>
              </div>
              <h2>欢迎使用 LaTeX 编辑器</h2>
              <p class="welcome-text">
                这是一个功能强大的在线 LaTeX 编辑器，支持实时预览、模板系统、代码片段、AI 辅助等功能。
              </p>
              <div class="features-grid">
                <div class="feature-item">
                  <el-icon><View /></el-icon>
                  <span>实时预览</span>
                </div>
                <div class="feature-item">
                  <el-icon><Collection /></el-icon>
                  <span>模板库</span>
                </div>
                <div class="feature-item">
                  <el-icon><Grid /></el-icon>
                  <span>代码片段</span>
                </div>
                <div class="feature-item">
                  <el-icon><MagicStick /></el-icon>
                  <span>AI 辅助</span>
                </div>
              </div>
            </div>

            <!-- 步骤2: 基础操作 -->
            <div v-else-if="currentStep === 1" class="step-card">
              <h3>基础操作</h3>
              <div class="instruction-list">
                <div class="instruction-item">
                  <div class="instruction-icon">
                    <kbd>Ctrl</kbd> + <kbd>S</kbd>
                  </div>
                  <div class="instruction-text">
                    <h4>保存文档</h4>
                    <p>手动保存当前编辑的文档</p>
                  </div>
                </div>
                <div class="instruction-item">
                  <div class="instruction-icon">
                    <kbd>Ctrl</kbd> + <kbd>Z</kbd>
                  </div>
                  <div class="instruction-text">
                    <h4>撤销</h4>
                    <p>撤销上一步操作</p>
                  </div>
                </div>
                <div class="instruction-item">
                  <div class="instruction-icon">
                    <kbd>Ctrl</kbd> + <kbd>F</kbd>
                  </div>
                  <div class="instruction-text">
                    <h4>查找替换</h4>
                    <p>快速查找和替换文本</p>
                  </div>
                </div>
                <div class="instruction-item">
                  <div class="instruction-icon">
                    <kbd>Ctrl</kbd> + <kbd>B</kbd>
                  </div>
                  <div class="instruction-text">
                    <h4>加粗</h4>
                    <p>快速插入加粗命令 \\textbf{}</p>
                  </div>
                </div>
              </div>
              <el-alert type="info" :closable="false">
                更多快捷键请点击编辑器右上角的 <el-icon><QuestionFilled /></el-icon> 按钮
              </el-alert>
            </div>

            <!-- 步骤3: 模板系统 -->
            <div v-else-if="currentStep === 2" class="step-card">
              <h3>模板系统</h3>
              <p class="step-description">
                使用预设模板快速创建文档，或保存自己的文档作为模板。
              </p>
              <div class="template-preview">
                <div
                  v-for="template in templates"
                  :key="template.id"
                  class="template-card"
                  :class="{ selected: selectedTemplate === template.id }"
                  @click="selectedTemplate = template.id"
                >
                  <div class="template-icon">
                    <el-icon>
                      <Document v-if="template.id === 'article'" />
                      <Reading v-else-if="template.id === 'report'" />
                      <Notebook v-else />
                    </el-icon>
                  </div>
                  <div class="template-info">
                    <div class="template-name">{{ template.name }}</div>
                    <div class="template-desc">{{ template.description }}</div>
                  </div>
                  <el-icon v-if="selectedTemplate === template.id" class="template-check">
                    <CircleCheck />
                  </el-icon>
                </div>
              </div>
              <div class="template-tip">
                <el-icon><InfoFilled /></el-icon>
                <span>点击左侧面板的"模板"选项卡可查看所有可用模板</span>
              </div>
            </div>

            <!-- 步骤4: 代码片段 -->
            <div v-else-if="currentStep === 3" class="step-card">
              <h3>代码片段</h3>
              <p class="step-description">
                常用代码片段库，快速插入公式、表格、图片等元素。
              </p>
              <div class="snippet-categories">
                <div
                  v-for="category in snippetCategories"
                  :key="category"
                  class="snippet-category"
                >
                  <div class="category-icon">
                    <el-icon>
                      <Operation v-if="category === '公式'" />
                      <Grid v-else-if="category === '表格'" />
                      <Picture v-else-if="category === '图片'" />
                      <Files v-else />
                    </el-icon>
                  </div>
                  <span>{{ category }}</span>
                </div>
              </div>
              <div class="snippet-example">
                <div class="example-label">示例片段</div>
                <el-input
                  type="textarea"
                  :model-value="exampleSnippet"
                  :rows="6"
                  readonly
                />
              </div>
            </div>

            <!-- 步骤5: AI 辅助 -->
            <div v-else-if="currentStep === 4" class="step-card">
              <h3>AI 辅助功能</h3>
              <p class="step-description">
                利用 AI 提升写作效率，包括智能补全、语法检查、润色优化等功能。
              </p>
              <div class="ai-features">
                <div class="ai-feature-card">
                  <div class="ai-icon" style="background: #409eff">
                    <el-icon><EditPen /></el-icon>
                  </div>
                  <h4>智能补全</h4>
                  <p>AI 续写文本内容</p>
                </div>
                <div class="ai-feature-card">
                  <div class="ai-icon" style="background: #67c23a">
                    <el-icon><CircleCheck /></el-icon>
                  </div>
                  <h4>语法检查</h4>
                  <p>检测语法和拼写错误</p>
                </div>
                <div class="ai-feature-card">
                  <div class="ai-icon" style="background: #e6a23c">
                    <el-icon><MagicStick /></el-icon>
                  </div>
                  <h4>润色优化</h4>
                  <p>多种润色风格</p>
                </div>
                <div class="ai-feature-card">
                  <div class="ai-icon" style="background: #f56c6c">
                    <el-icon><ChatDotSquare /></el-icon>
                  </div>
                  <h4>翻译助手</h4>
                  <p>中英互译</p>
                </div>
              </div>
            </div>

            <!-- 步骤6: 协作功能 -->
            <div v-else-if="currentStep === 5" class="step-card">
              <h3>协作功能</h3>
              <p class="step-description">
                与团队成员协作编辑文档，实时同步和评论交流。
              </p>
              <div class="collab-features">
                <div class="collab-item">
                  <div class="collab-icon">
                    <el-icon><UserFilled /></el-icon>
                  </div>
                  <div class="collab-info">
                    <h4>实时协作</h4>
                    <p>多人同时编辑，实时显示光标位置</p>
                  </div>
                </div>
                <div class="collab-item">
                  <div class="collab-icon">
                    <el-icon><ChatDotSquare /></el-icon>
                  </div>
                  <div class="collab-info">
                    <h4>评论批注</h4>
                    <p>添加评论和回复，标记问题</p>
                  </div>
                </div>
                <div class="collab-item">
                  <div class="collab-icon">
                    <el-icon><Message /></el-icon>
                  </div>
                  <div class="collab-info">
                    <h4>团队聊天</h4>
                    <p>内置聊天功能，实时沟通</p>
                  </div>
                </div>
              </div>
            </div>

            <!-- 步骤7: 第三方集成 -->
            <div v-else-if="currentStep === 6" class="step-card">
              <h3>第三方集成</h3>
              <p class="step-description">
                与常用工具集成，提升工作流程效率。
              </p>
              <div class="integration-list">
                <div class="integration-item">
                  <div class="integration-logo overleaf">
                    <span>Overleaf</span>
                  </div>
                  <div class="integration-info">
                    <h4>Overleaf 同步</h4>
                    <p>双向同步 Overleaf 项目</p>
                  </div>
                </div>
                <div class="integration-item">
                  <div class="integration-logo zotero">
                    <span>Zotero</span>
                  </div>
                  <div class="integration-info">
                    <h4>Zotero 引用</h4>
                    <p>从 Zotero 导入引用库</p>
                  </div>
                </div>
              </div>
            </div>

            <!-- 步骤8: 完成 -->
            <div v-else-if="currentStep === 7" class="step-card">
              <div class="welcome-icon">
                <el-icon :size="80" color="#67c23a"><CircleCheck /></el-icon>
              </div>
              <h2>设置完成！</h2>
              <p class="welcome-text">
                您已了解 LaTeX 编辑器的主要功能。现在可以开始创作了！
              </p>
              <div class="completion-options">
                <el-checkbox v-model="dontShowAgain">
                  不再显示此向导
                </el-checkbox>
              </div>
              <div class="quick-links">
                <el-button text @click="openHelp">
                  <el-icon><DocumentCopy /></el-icon>
                  查看完整文档
                </el-button>
                <el-button text @click="openShortcuts">
                  <el-icon><Key /></el-icon>
                  快捷键列表
                </el-button>
              </div>
            </div>
          </div>
        </Transition>
      </div>
    </div>

    <template #footer>
      <div class="dialog-footer">
        <el-button v-if="currentStep > 0" @click="previousStep">
          上一步
        </el-button>
        <el-button v-if="currentStep < steps.length - 1" type="primary" @click="nextStep">
          下一步
        </el-button>
        <el-button v-else type="primary" @click="completeGuide">
          开始使用
        </el-button>
      </div>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import {
  CircleCheck, EditPen, View, Collection, Grid, MagicStick, QuestionFilled,
  Document, Reading, Notebook, InfoFilled, Operation, Picture, Files,
  Edit, ChatDotSquare, Message, UserFilled, DocumentCopy, Key
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface Props {
  show: boolean
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'update:show': [value: boolean]
  'create-from-template': [templateId: string]
}>()

// 状态
const currentStep = ref(0)
const dontShowAgain = ref(false)
const selectedTemplate = ref('article')

const steps = [
  '欢迎',
  '基础操作',
  '模板系统',
  '代码片段',
  'AI 辅助',
  '协作功能',
  '第三方集成',
  '完成'
]

const templates = [
  {
    id: 'article',
    name: '学术论文',
    description: '标准论文模板，适合期刊投稿'
  },
  {
    id: 'report',
    name: '技术报告',
    description: '简洁的报告格式'
  },
  {
    id: 'thesis',
    name: '学位论文',
    description: '完整的学位论文结构'
  }
]

const snippetCategories = ['公式', '表格', '图片', '引用']

const exampleSnippet = `\\begin{equation}
  E = mc^2
  \\label{eq:einstein}
\\end{equation}`

// 方法
function nextStep() {
  if (currentStep.value < steps.length - 1) {
    currentStep.value++
  }
}

function previousStep() {
  if (currentStep.value > 0) {
    currentStep.value--
  }
}

function completeGuide() {
  if (dontShowAgain.value) {
    localStorage.setItem('latex-welcome-guide-dismissed', 'true')
  }

  if (selectedTemplate.value) {
    emit('create-from-template', selectedTemplate.value)
  }

  emit('update:show', false)
  ElMessage.success('欢迎使用 LaTeX 编辑器！')
}

function handleClose() {
  if (currentStep.value === 0) {
    return false // 阻止关闭第一步
  }
  emit('update:show', false)
}

function openHelp() {
  window.open('https://www.overleaf.com/learn', '_blank')
}

function openShortcuts() {
  emit('update:show', false)
  // 触发快捷键对话框
  setTimeout(() => {
    // 这里需要父组件处理
  }, 100)
}

// 监听对话框打开
watch(() => props.show, (show) => {
  if (show) {
    currentStep.value = 0
    const dismissed = localStorage.getItem('latex-welcome-guide-dismissed')
    if (dismissed === 'true') {
      dontShowAgain.value = true
    }
  }
})
</script>

<style scoped lang="scss">
.welcome-guide {
  padding: 20px 0;
}

.step-indicator {
  display: flex;
  justify-content: center;
  align-items: center;
  position: relative;
  margin-bottom: 32px;
  padding: 0 20px;
}

.step-dot {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  background: var(--el-fill-color-light);
  border: 2px solid var(--el-border-color);
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: 500;
  z-index: 1;
  transition: all 0.3s;

  &.active {
    background: var(--el-color-primary);
    border-color: var(--el-color-primary);
    color: #fff;
    transform: scale(1.1);
  }

  &.completed {
    background: var(--el-color-success);
    border-color: var(--el-color-success);
    color: #fff;
  }
}

.step-line {
  position: absolute;
  top: 50%;
  left: 50px;
  right: 50px;
  height: 2px;
  background: var(--el-border-color);
  transform: translateY(-50%);
  z-index: 0;
}

.step-content {
  min-height: 350px;
}

.step-card {
  text-align: center;
}

.welcome-icon {
  margin-bottom: 20px;
  color: var(--el-color-primary);
}

h2 {
  margin: 0 0 12px 0;
  font-size: 24px;
}

h3 {
  margin: 0 0 16px 0;
  font-size: 20px;
}

.welcome-text {
  color: var(--el-text-color-secondary);
  margin-bottom: 24px;
  line-height: 1.6;
}

.step-description {
  color: var(--el-text-color-secondary);
  margin-bottom: 24px;
}

.features-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
  margin-top: 24px;
}

.feature-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  padding: 16px;
  background: var(--el-fill-color-light);
  border-radius: 8px;

  .el-icon {
    font-size: 32px;
    color: var(--el-color-primary);
  }
}

.instruction-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
  text-align: left;
}

.instruction-item {
  display: flex;
  gap: 16px;
  align-items: center;
}

.instruction-icon {
  flex-shrink: 0;
  display: flex;
  gap: 4px;

  kbd {
    padding: 4px 8px;
    background: var(--el-fill-color-light);
    border: 1px solid var(--el-border-color);
    border-radius: 4px;
    font-family: monospace;
    font-size: 12px;
  }
}

.instruction-text {
  h4 {
    margin: 0 0 4px 0;
    font-size: 14px;
  }

  p {
    margin: 0;
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }
}

.template-preview {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin-bottom: 16px;
}

.template-card {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px;
  border: 2px solid var(--el-border-color);
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    border-color: var(--el-color-primary-light-5);
  }

  &.selected {
    border-color: var(--el-color-primary);
    background: var(--el-color-primary-light-9);
  }
}

.template-icon {
  width: 40px;
  height: 40px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--el-fill-color-light);
  border-radius: 8px;
  color: var(--el-color-primary);
}

.template-info {
  flex: 1;
  text-align: left;
}

.template-name {
  font-weight: 500;
  margin-bottom: 2px;
}

.template-desc {
  font-size: 12px;
  color: var(--el-text-color-secondary);
}

.template-check {
  color: var(--el-color-success);
  font-size: 20px;
}

.template-tip {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  background: var(--el-color-info-light-9);
  border-radius: 4px;
  font-size: 12px;
  color: var(--el-color-info);
  text-align: left;
}

.snippet-categories {
  display: flex;
  justify-content: center;
  gap: 16px;
  margin-bottom: 24px;
}

.snippet-category {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  padding: 16px;
  background: var(--el-fill-color-light);
  border-radius: 8px;
  min-width: 80px;

  .category-icon {
    width: 40px;
    height: 40px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: #fff;
    border-radius: 50%;
    color: var(--el-color-primary);
  }
}

.snippet-example {
  text-align: left;
}

.example-label {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin-bottom: 8px;
}

.ai-features {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}

.ai-feature-card {
  padding: 16px;
  background: var(--el-fill-color-light);
  border-radius: 8px;
  text-align: center;

  .ai-icon {
    width: 48px;
    height: 48px;
    margin: 0 auto 12px;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: 50%;
    color: #fff;
  }

  h4 {
    margin: 0 0 4px 0;
    font-size: 14px;
  }

  p {
    margin: 0;
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }
}

.collab-features {
  display: flex;
  flex-direction: column;
  gap: 12px;
  text-align: left;
}

.collab-item {
  display: flex;
  gap: 12px;
  padding: 12px;
  background: var(--el-fill-color-light);
  border-radius: 8px;
}

.collab-icon {
  width: 40px;
  height: 40px;
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--el-color-primary);
  color: #fff;
  border-radius: 8px;
}

.collab-info {
  h4 {
    margin: 0 0 4px 0;
    font-size: 14px;
  }

  p {
    margin: 0;
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }
}

.integration-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.integration-item {
  display: flex;
  gap: 12px;
  padding: 12px;
  background: var(--el-fill-color-light);
  border-radius: 8px;
}

.integration-logo {
  width: 80px;
  flex-shrink: 0;
  padding: 8px;
  border-radius: 6px;
  text-align: center;
  font-weight: 500;
  color: #fff;

  &.overleaf {
    background: linear-gradient(135deg, #4a90a4, #3d7a8a);
  }

  &.zotero {
    background: linear-gradient(135deg, #cc2936, #a31c29);
  }
}

.integration-info {
  h4 {
    margin: 0 0 4px 0;
    font-size: 14px;
  }

  p {
    margin: 0;
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }
}

.completion-options {
  margin: 24px 0;
}

.quick-links {
  display: flex;
  justify-content: center;
  gap: 16px;
  margin-top: 16px;
}

.dialog-footer {
  display: flex;
  justify-content: center;
  gap: 12px;
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity 0.3s;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}
</style>
