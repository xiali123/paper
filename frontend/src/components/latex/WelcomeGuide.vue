<template>
  <el-dialog
    v-model="visible"
    title="欢迎使用 LaTeX 编辑器"
    width="800px"
    :close-on-click-modal="false"
    :show-close="currentStep > 0"
    class="welcome-guide-dialog"
  >
    <!-- 步骤指示器 -->
    <div class="guide-steps" v-if="currentStep > 0">
      <div
        v-for="(_, index) in steps"
        :key="index"
        class="step-item"
        :class="{
          active: index === currentStep - 1,
          completed: index < currentStep - 1
        }"
      >
        <div class="step-number">{{ index + 1 }}</div>
        <div class="step-line" v-if="index < steps.length - 1"></div>
      </div>
    </div>

    <!-- 步骤内容 -->
    <div class="guide-content">
      <!-- 欢迎 -->
      <div v-if="currentStep === 0" class="welcome-screen">
        <div class="welcome-illustration">
          <div class="latex-logo">𝐋</div>
          <h1>LaTeX 编辑器</h1>
          <p>专业的在线 LaTeX 编辑和协作平台</p>
        </div>

        <div class="welcome-features">
          <div class="feature-item">
            <el-icon :size="32"><Edit /></el-icon>
            <h3>智能编辑</h3>
            <p>语法高亮、自动补全、实时预览</p>
          </div>
          <div class="feature-item">
            <el-icon :size="32"><View /></el-icon>
            <h3>即时预览</h3>
            <p>PDF预览、同步滚动、错误提示</p>
          </div>
          <div class="feature-item">
            <el-icon :size="32"><User /></el-icon>
            <h3>团队协作</h3>
            <p>多人编辑、版本控制、评论批注</p>
          </div>
        </div>

        <div class="welcome-actions">
          <el-button type="primary" size="large" @click="startGuide">
            <el-icon><Guide /></el-icon>
            开始使用向导
          </el-button>
          <el-button size="large" @click="skipGuide">
            跳过向导
          </el-button>
        </div>
      </div>

      <!-- 步骤详情 -->
      <div v-else class="step-detail">
        <div class="step-header">
          <h2>{{ currentStepData.title }}</h2>
          <p>{{ currentStepData.description }}</p>
        </div>

        <div class="step-body">
          <!-- 第一步：创建文档 -->
          <div v-if="currentStep === 1" class="step-content">
            <div class="content-section">
              <h3>选择创建方式</h3>
              <div class="create-options">
                <div class="create-option" @click="createNew('blank')">
                  <div class="option-icon">📄</div>
                  <div class="option-info">
                    <h4>空白文档</h4>
                    <p>从空白LaTeX文档开始</p>
                  </div>
                </div>
                <div class="create-option" @click="showTemplates = true">
                  <div class="option-icon">📋</div>
                  <div class="option-info">
                    <h4>使用模板</h4>
                    <p>从预设模板快速开始</p>
                  </div>
                </div>
                <div class="create-option" @click="createNew('project')">
                  <div class="option-icon">📁</div>
                  <div class="option-info">
                    <h4>创建项目</h4>
                    <p>管理多个相关文件</p>
                  </div>
                </div>
              </div>
            </div>

            <div class="content-section">
              <h3>快捷键提示</h3>
              <div class="shortcut-list">
                <div class="shortcut-item">
                  <kbd>Ctrl</kbd> + <kbd>S</kbd>
                  <span>保存文档</span>
                </div>
                <div class="shortcut-item">
                  <kbd>Ctrl</kbd> + <kbd>Enter</kbd>
                  <span>编译文档</span>
                </div>
                <div class="shortcut-item">
                  <kbd>Ctrl</kbd> + <kbd>/</kbd>
                  <span>切换注释</span>
                </div>
                <div class="shortcut-item">
                  <kbd>F1</kbd>
                  <span>查看所有快捷键</span>
                </div>
              </div>
            </div>
          </div>

          <!-- 第二步：编辑器界面 -->
          <div v-if="currentStep === 2" class="step-content">
            <div class="editor-tour">
              <div class="tour-item" @mouseenter="highlightArea('toolbar')">
                <div class="tour-icon">🔧</div>
                <div class="tour-info">
                  <h4>工具栏</h4>
                  <p>常用操作按钮：保存、编译、导出、撤销/重做</p>
                </div>
              </div>
              <div class="tour-item" @mouseenter="highlightArea('sidebar')">
                <div class="tour-icon">📑</div>
                <div class="tour-info">
                  <h4>侧边栏</h4>
                  <p>文档大纲、文件树、符号面板</p>
                </div>
              </div>
              <div class="tour-item" @mouseenter="highlightArea('editor')">
                <div class="tour-icon">✏️</div>
                <div class="tour-info">
                  <h4>编辑器</h4>
                  <p>LaTeX代码编辑区，支持语法高亮和自动补全</p>
                </div>
              </div>
              <div class="tour-item" @mouseenter="highlightArea('preview')">
                <div class="tour-icon">👁️</div>
                <div class="tour-info">
                  <h4>预览区</h4>
                  <p>实时预览编译结果，支持同步滚动</p>
                </div>
              </div>
            </div>

            <div class="tips-section">
              <h4>💡 编辑技巧</h4>
              <ul>
                <li>输入 <code>\</code> 触发命令自动补全</li>
                <li>使用 <code>Ctrl + Space</code> 查看所有可用命令</li>
                <li>按住 <code>Ctrl</code> 滚动鼠标可快速缩放预览</li>
                <li>双击PDF可跳转到对应源码位置</li>
              </ul>
            </div>
          </div>

          <!-- 第三步：功能介绍 -->
          <div v-if="currentStep === 3" class="step-content">
            <div class="features-showcase">
              <div class="feature-showcase-item">
                <div class="showcase-icon">🎨</div>
                <h4>丰富的模板库</h4>
                <p>学术论文、简历、报告、书籍、演示文稿等多种模板</p>
                <el-button text type="primary">浏览模板 →</el-button>
              </div>

              <div class="feature-showcase-item">
                <div class="showcase-icon">📊</div>
                <h4>文档统计</h4>
                <p>实时查看字数、公式数、阅读时间等统计信息</p>
                <el-button text type="primary">查看统计 →</el-button>
              </div>

              <div class="feature-showcase-item">
                <div class="showcase-icon">🤖</div>
                <h4>AI 辅助</h4>
                <p>公式识别、语法检查、智能补全（即将推出）</p>
                <el-tag type="warning" size="small">即将推出</el-tag>
              </div>

              <div class="feature-showcase-item">
                <div class="showcase-icon">👥</div>
                <h4>团队协作</h4>
                <p>多人同时编辑、评论批注、版本历史</p>
                <el-button text type="primary">了解更多 →</el-button>
              </div>
            </div>
          </div>

          <!-- 第四步：完成 -->
          <div v-if="currentStep === 4" class="step-content">
            <div class="completion-screen">
              <div class="completion-icon">🎉</div>
              <h2>准备就绪！</h2>
              <p>您已经了解了 LaTeX 编辑器的基本功能</p>

              <div class="completion-links">
                <div class="link-item">
                  <el-icon><Document /></el-icon>
                  <div>
                    <h4>查看文档</h4>
                    <p>阅读详细使用指南</p>
                  </div>
                  <el-button circle icon="ArrowRight" />
                </div>
                <div class="link-item">
                  <el-icon><VideoPlay /></el-icon>
                  <div>
                    <h4>观看教程</h4>
                    <p>视频教程和演示</p>
                  </div>
                  <el-button circle icon="ArrowRight" />
                </div>
                <div class="link-item">
                  <el-icon><ChatDotSquare /></el-icon>
                  <div>
                    <h4>获取帮助</h4>
                    <p>FAQ和社区支持</p>
                  </div>
                  <el-button circle icon="ArrowRight" />
                </div>
              </div>

              <div class="completion-actions">
                <el-button type="primary" size="large" @click="startEditing">
                  <el-icon><Edit /></el-icon>
                  开始编辑
                </el-button>
                <div class="completion-hint">
                  此向导仅在首次使用时显示
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 底部操作栏 -->
    <template #footer>
      <div class="guide-footer">
        <div class="footer-progress">
          <span>步骤 {{ currentStep }} / {{ steps.length }}</span>
          <el-progress
            :percentage="((currentStep - 1) / (steps.length - 1)) * 100"
            :show-text="false"
            :stroke-width="3"
          />
        </div>

        <div class="footer-actions">
          <el-button
            v-if="currentStep > 1"
            @click="previousStep"
            :disabled="currentStep === 1"
          >
            上一步
          </el-button>
          <el-button
            v-if="currentStep < steps.length"
            type="primary"
            @click="nextStep"
          >
            {{ currentStep === 0 ? '开始' : '下一步' }}
          </el-button>
          <el-button
            v-else
            type="primary"
            @click="finishGuide"
          >
            完成
          </el-button>
        </div>
      </div>
    </template>

    <!-- 模板选择抽屉 -->
    <el-drawer
      v-model="showTemplates"
      title="选择模板"
      direction="rtl"
      size="500px"
    >
      <TemplateManager @insert="handleTemplateSelect" />
    </el-drawer>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import {
  Edit,
  View,
  User,
  Guide,
  Document,
  VideoPlay,
  ChatDotSquare,
  ArrowRight
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import TemplateManager from './TemplateManager.vue'

interface Step {
  title: string
  description: string
}

const emit = defineEmits<{
  'create-document': [type: string]
  'start-editing': []
  'close': []
}>()

// 统一的localStorage键
const WELCOME_SEEN_KEY = 'latex-welcome-seen'

const visible = ref(false) // 默认不显示，等待检查
const currentStep = ref(0)
const showTemplates = ref(false)

// 组件挂载时检查是否已看过欢迎
onMounted(() => {
  checkWelcomeSeen()
})

function checkWelcomeSeen() {
  try {
    const welcomeSeen = localStorage.getItem(WELCOME_SEEN_KEY)
    if (welcomeSeen !== 'true') {
      // 未看过，显示欢迎界面
      visible.value = true
    }
  } catch (e) {
    // 如果localStorage不可用，显示欢迎界面
    console.warn('localStorage not available, showing welcome guide')
    visible.value = true
  }
}

const steps: Step[] = [
  {
    title: '创建您的第一个文档',
    description: '选择创建方式，快速上手LaTeX编辑'
  },
  {
    title: '熟悉编辑器界面',
    description: '了解编辑器的各个区域和功能'
  },
  {
    title: '探索更多功能',
    description: '发现LaTeX编辑器的高级功能'
  },
  {
    title: '开始创作',
    description: '一切准备就绪，开始您的LaTeX创作之旅'
  }
]

const currentStepData = computed(() => {
  if (currentStep.value === 0) return null
  return steps[currentStep.value - 1]
})

function startGuide() {
  currentStep.value = 1
}

function skipGuide() {
  finishGuide()
}

function nextStep() {
  if (currentStep.value === 0) {
    currentStep.value = 1
  } else if (currentStep.value < steps.length) {
    currentStep.value++
  }
}

function previousStep() {
  if (currentStep.value > 1) {
    currentStep.value--
  }
}

function createNew(type: string) {
  emit('create-document', type)
  visible.value = false
}

function handleTemplateSelect(template: any) {
  emit('create-document', 'template')
  visible.value = false
}

function highlightArea(area: string) {
  ElMessage.info(`高亮显示: ${area}`)
}

function startEditing() {
  emit('start-editing')
  visible.value = false
}

function finishGuide() {
  // 总是标记为已看过，无论用户是否勾选"不再显示"
  try {
    localStorage.setItem(WELCOME_SEEN_KEY, 'true')
  } catch (e) {
    console.warn('Failed to save guide preference')
  }
  emit('close')
  visible.value = false
}

// open方法保留用于手动打开（如从菜单触发）
const open = () => {
  visible.value = true
  currentStep.value = 0
}

defineExpose({ open })
</script>

<style scoped lang="scss">
.welcome-guide-dialog {
  :deep(.el-dialog__body) {
    padding: 0;
  }
}

.guide-steps {
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 24px 0;
  gap: 8px;
}

.step-item {
  display: flex;
  align-items: center;
}

.step-number {
  width: 32px;
  height: 32px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--el-fill-color);
  border: 2px solid var(--el-border-color);
  color: var(--el-text-color-secondary);
  font-weight: 600;
  transition: all 0.3s;

  .step-item.active & {
    background: var(--el-color-primary);
    border-color: var(--el-color-primary);
    color: white;
  }

  .step-item.completed & {
    background: var(--el-color-success);
    border-color: var(--el-color-success);
    color: white;
  }
}

.step-line {
  width: 60px;
  height: 2px;
  background: var(--el-border-color);

  .step-item.completed ~ .step-item & {
    background: var(--el-color-success);
  }

  .step-item.active ~ .step-item & {
    background: var(--el-color-primary);
  }
}

.guide-content {
  min-height: 400px;
  padding: 24px 32px;
}

// 欢迎屏幕
.welcome-screen {
  text-align: center;
}

.welcome-illustration {
  margin-bottom: 40px;
}

.latex-logo {
  font-size: 64px;
  margin-bottom: 16px;
}

.welcome-illustration h1 {
  font-size: 32px;
  font-weight: 600;
  color: var(--el-text-color-primary);
  margin: 0 0 8px 0;
}

.welcome-illustration p {
  font-size: 16px;
  color: var(--el-text-color-secondary);
  margin: 0;
}

.welcome-features {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 24px;
  margin-bottom: 40px;
}

.feature-item {
  padding: 20px;
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  transition: all 0.3s;

  &:hover {
    border-color: var(--el-color-primary);
    background: var(--el-color-primary-light-9);
  }

  .el-icon {
    color: var(--el-color-primary);
    margin-bottom: 12px;
  }

  h3 {
    font-size: 16px;
    font-weight: 600;
    margin: 0 0 8px 0;
    color: var(--el-text-color-primary);
  }

  p {
    font-size: 13px;
    color: var(--el-text-color-secondary);
    margin: 0;
  }
}

.welcome-actions {
  display: flex;
  gap: 12px;
  justify-content: center;
}

// 步骤详情
.step-detail {
  .step-header {
    text-align: center;
    margin-bottom: 32px;

    h2 {
      font-size: 24px;
      font-weight: 600;
      color: var(--el-text-color-primary);
      margin: 0 0 8px 0;
    }

    p {
      font-size: 14px;
      color: var(--el-text-color-secondary);
      margin: 0;
    }
  }
}

.step-content {
  max-width: 600px;
  margin: 0 auto;
}

.content-section {
  margin-bottom: 32px;

  h3 {
    font-size: 16px;
    font-weight: 600;
    color: var(--el-text-color-primary);
    margin: 0 0 16px 0;
  }
}

// 创建选项
.create-options {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16px;
}

.create-option {
  padding: 20px;
  border: 2px solid var(--el-border-color);
  border-radius: 8px;
  text-align: center;
  cursor: pointer;
  transition: all 0.3s;

  &:hover {
    border-color: var(--el-color-primary);
    background: var(--el-color-primary-light-9);
    transform: translateY(-2px);
  }

  .option-icon {
    font-size: 32px;
    margin-bottom: 8px;
  }

  h4 {
    font-size: 14px;
    font-weight: 600;
    color: var(--el-text-color-primary);
    margin: 0 0 4px 0;
  }

  p {
    font-size: 12px;
    color: var(--el-text-color-secondary);
    margin: 0;
  }
}

// 快捷键列表
.shortcut-list {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
}

.shortcut-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  background: var(--el-fill-color-light);
  border-radius: 4px;

  kbd {
    display: inline-block;
    padding: 2px 6px;
    font-family: 'Consolas', monospace;
    font-size: 11px;
    background: white;
    border: 1px solid var(--el-border-color);
    border-radius: 3px;
    box-shadow: 0 1px 2px rgba(0, 0, 0, 0.05);
  }

  span {
    flex: 1;
    font-size: 13px;
    color: var(--el-text-color-regular);
  }
}

// 编辑器导览
.editor-tour {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
  margin-bottom: 32px;
}

.tour-item {
  display: flex;
  gap: 12px;
  padding: 16px;
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.3s;

  &:hover {
    border-color: var(--el-color-primary);
    background: var(--el-fill-color-light);
  }

  .tour-icon {
    font-size: 24px;
  }

  h4 {
    font-size: 14px;
    font-weight: 600;
    margin: 0 0 4px 0;
  }

  p {
    font-size: 12px;
    color: var(--el-text-color-secondary);
    margin: 0;
  }
}

.tips-section {
  padding: 16px;
  background: var(--el-color-info-light-9);
  border-radius: 8px;
  border-left: 4px solid var(--el-color-info);

  h4 {
    font-size: 14px;
    font-weight: 600;
    margin: 0 0 12px 0;
  }

  ul {
    margin: 0;
    padding-left: 20px;

    li {
      font-size: 13px;
      color: var(--el-text-color-regular);
      margin-bottom: 8px;

      code {
        padding: 2px 6px;
        background: var(--el-fill-color);
        border-radius: 3px;
        font-family: 'Consolas', monospace;
        font-size: 12px;
      }
    }
  }
}

// 功能展示
.features-showcase {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}

.feature-showcase-item {
  padding: 20px;
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  text-align: center;

  .showcase-icon {
    font-size: 32px;
    margin-bottom: 8px;
  }

  h4 {
    font-size: 14px;
    font-weight: 600;
    margin: 0 0 8px 0;
  }

  p {
    font-size: 12px;
    color: var(--el-text-color-secondary);
    margin: 0 0 12px 0;
  }
}

// 完成屏幕
.completion-screen {
  text-align: center;
}

.completion-icon {
  font-size: 64px;
  margin-bottom: 16px;
}

.completion-screen h2 {
  font-size: 24px;
  font-weight: 600;
  color: var(--el-text-color-primary);
  margin: 0 0 8px 0;
}

.completion-screen p {
  font-size: 14px;
  color: var(--el-text-color-secondary);
  margin: 0 0 32px 0;
}

.completion-links {
  display: flex;
  flex-direction: column;
  gap: 16px;
  max-width: 400px;
  margin: 0 auto 32px;
}

.link-item {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 16px;
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  text-align: left;
  transition: all 0.3s;

  &:hover {
    border-color: var(--el-color-primary);
    background: var(--el-fill-color-light);
  }

  .el-icon {
    font-size: 24px;
    color: var(--el-color-primary);
  }

  h4 {
    font-size: 14px;
    font-weight: 600;
    margin: 0 0 4px 0;
  }

  p {
    font-size: 12px;
    color: var(--el-text-color-secondary);
    margin: 0;
  }
}

.completion-actions {
  display: flex;
  flex-direction: column;
  gap: 16px;
  align-items: center;
}

.completion-hint {
  font-size: 13px;
  color: var(--el-text-color-secondary);
}

// 底部操作栏
.guide-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 24px;
  border-top: 1px solid var(--el-border-color);
  background: var(--el-fill-color-light);
}

.footer-progress {
  display: flex;
  flex-direction: column;
  gap: 8px;
  flex: 1;

  span {
    font-size: 13px;
    color: var(--el-text-color-secondary);
  }

  :deep(.el-progress) {
    width: 200px;
  }
}

.footer-actions {
  display: flex;
  gap: 8px;
}
</style>
