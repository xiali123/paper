<template>
  <el-drawer
    :model-value="show"
    @update:model-value="$emit('update:show', $event)"
    title="主题定制"
    direction="rtl"
    size="450px"
  >
    <div class="theme-customizer">
      <!-- 预设主题 -->
      <div class="section">
        <h4>预设主题</h4>
        <div class="theme-presets">
          <div
            v-for="preset in themePresets"
            :key="preset.id"
            class="theme-preset-card"
            :class="{ active: currentTheme.preset === preset.id }"
            @click="applyPreset(preset)"
          >
            <div class="preset-preview">
              <div class="preview-header" :style="{ background: preset.colors.header }"></div>
              <div class="preview-body">
                <div class="preview-sidebar" :style="{ background: preset.colors.sidebar }"></div>
                <div class="preview-content">
                  <div class="preview-line primary" :style="{ background: preset.colors.primary }"></div>
                  <div class="preview-line secondary" :style="{ background: preset.colors.secondary }"></div>
                  <div class="preview-line text" :style="{ background: preset.colors.text }"></div>
                </div>
              </div>
            </div>
            <div class="preset-name">{{ preset.name }}</div>
          </div>
        </div>
      </div>

      <!-- 编辑器主题 -->
      <div class="section">
        <h4>编辑器主题</h4>
        <el-form label-width="100px" size="small">
          <el-form-item label="代码主题">
            <el-select v-model="currentTheme.editor" @change="updateTheme">
              <el-option label="Monokai" value="monokai" />
              <el-option label="GitHub Light" value="github-light" />
              <el-option label="GitHub Dark" value="github-dark" />
              <el-option label="Dracula" value="dracula" />
              <el-option label="Nord" value="nord" />
              <el-option label="Solarized Light" value="solarized-light" />
              <el-option label="Solarized Dark" value="solarized-dark" />
            </el-select>
          </el-form-item>
          <el-form-item label="字体大小">
            <el-slider
              v-model="currentTheme.fontSize"
              :min="12"
              :max="24"
              :step="1"
              show-input
              @change="updateTheme"
            />
          </el-form-item>
          <el-form-item label="行高">
            <el-slider
              v-model="currentTheme.lineHeight"
              :min="1.0"
              :max="2.0"
              :step="0.1"
              show-input
              @change="updateTheme"
            />
          </el-form-item>
          <el-form-item label="字体系列">
            <el-select v-model="currentTheme.fontFamily" @change="updateTheme">
              <el-option label="Fira Code" value="'Fira Code', monospace" />
              <el-option label="JetBrains Mono" value="'JetBrains Mono', monospace" />
              <el-option label="Source Code Pro" value="'Source Code Pro', monospace" />
              <el-option label="Consolas" value="'Consolas', monospace" />
              <el-option label="Monaco" value="'Monaco', monospace" />
            </el-select>
          </el-form-item>
        </el-form>
      </div>

      <!-- 预览主题 -->
      <div class="section">
        <h4>预览主题</h4>
        <el-form label-width="100px" size="small">
          <el-form-item label="预览模式">
            <el-radio-group v-model="currentTheme.previewMode" @change="updateTheme">
              <el-radio value="light">浅色</el-radio>
              <el-radio value="dark">深色</el-radio>
              <el-radio value="auto">自动</el-radio>
            </el-radio-group>
          </el-form-item>
          <el-form-item label="PDF 缩放">
            <el-slider
              v-model="currentTheme.pdfZoom"
              :min="50"
              :max="200"
              :step="10"
              show-input
              @change="updateTheme"
            />
          </el-form-item>
        </el-form>
      </div>

      <!-- 自定义颜色 -->
      <div class="section">
        <div class="section-header">
          <h4>自定义颜色</h4>
          <el-switch
            v-model="currentTheme.customColors"
            @change="updateTheme"
          />
        </div>
        <div v-if="currentTheme.customColors" class="color-customization">
          <div class="color-item">
            <label>主色调</label>
            <el-color-picker v-model="currentTheme.colors.primary" @change="updateTheme" />
          </div>
          <div class="color-item">
            <label>强调色</label>
            <el-color-picker v-model="currentTheme.colors.accent" @change="updateTheme" />
          </div>
          <div class="color-item">
            <label>背景色</label>
            <el-color-picker v-model="currentTheme.colors.background" @change="updateTheme" />
          </div>
          <div class="color-item">
            <label>文字色</label>
            <el-color-picker v-model="currentTheme.colors.text" @change="updateTheme" />
          </div>
          <div class="color-item">
            <label>边框色</label>
            <el-color-picker v-model="currentTheme.colors.border" @change="updateTheme" />
          </div>
        </div>
      </div>

      <!-- 布局选项 -->
      <div class="section">
        <h4>布局选项</h4>
        <el-form label-width="100px" size="small">
          <el-form-item label="面板位置">
            <el-radio-group v-model="currentTheme.panelPosition" @change="updateTheme">
              <el-radio value="left">左侧</el-radio>
              <el-radio value="right">右侧</el-radio>
            </el-radio-group>
          </el-form-item>
          <el-form-item label="预览位置">
            <el-radio-group v-model="currentTheme.previewPosition" @change="updateTheme">
              <el-radio value="bottom">底部</el-radio>
              <el-radio value="right">右侧</el-radio>
            </el-radio-group>
          </el-form-item>
          <el-form-item label="工具栏">
            <el-checkbox v-model="currentTheme.compactToolbar" @change="updateTheme">
              紧凑模式
            </el-checkbox>
          </el-form-item>
          <el-form-item>
            <el-checkbox v-model="currentTheme.showMinimap" @change="updateTheme">
              显示代码地图
            </el-checkbox>
          </el-form-item>
          <el-form-item>
            <el-checkbox v-model="currentTheme.showLineNumbers" @change="updateTheme">
              显示行号
            </el-checkbox>
          </el-form-item>
        </el-form>
      </div>

      <!-- 操作按钮 -->
      <div class="actions">
        <el-button @click="resetToDefault">
          <el-icon><RefreshLeft /></el-icon>
          重置默认
        </el-button>
        <el-button type="primary" @click="saveTheme">
          <el-icon><Check /></el-icon>
          保存主题
        </el-button>
      </div>
    </div>
  </el-drawer>
</template>

<script setup lang="ts">
import { ref, reactive, watch } from 'vue'
import { RefreshLeft, Check } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

interface ThemeColors {
  primary: string
  accent: string
  background: string
  text: string
  border: string
}

interface ThemeConfig {
  preset: string
  editor: string
  fontSize: number
  lineHeight: number
  fontFamily: string
  previewMode: 'light' | 'dark' | 'auto'
  pdfZoom: number
  customColors: boolean
  colors: ThemeColors
  panelPosition: 'left' | 'right'
  previewPosition: 'bottom' | 'right'
  compactToolbar: boolean
  showMinimap: boolean
  showLineNumbers: boolean
}

interface ThemePreset {
  id: string
  name: string
  colors: {
    header: string
    sidebar: string
    primary: string
    secondary: string
    text: string
  }
}

interface Props {
  show: boolean
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'update:show': [value: boolean]
  'theme-change': [theme: ThemeConfig]
}>()

// 预设主题
const themePresets: ThemePreset[] = [
  {
    id: 'default',
    name: '默认',
    colors: {
      header: '#409eff',
      sidebar: '#f5f7fa',
      primary: '#409eff',
      secondary: '#67c23a',
      text: '#606266'
    }
  },
  {
    id: 'dark',
    name: '深色',
    colors: {
      header: '#1d1d1d',
      sidebar: '#252525',
      primary: '#409eff',
      secondary: '#67c23a',
      text: '#e0e0e0'
    }
  },
  {
    id: 'forest',
    name: '森林',
    colors: {
      header: '#2d5a27',
      sidebar: '#e8f5e9',
      primary: '#4caf50',
      secondary: '#8bc34a',
      text: '#2e7d32'
    }
  },
  {
    id: 'ocean',
    name: '海洋',
    colors: {
      header: '#0277bd',
      sidebar: '#e1f5fe',
      primary: '#03a9f4',
      secondary: '#00bcd4',
      text: '#01579b'
    }
  },
  {
    id: 'sunset',
    name: '日落',
    colors: {
      header: '#e65100',
      sidebar: '#fff3e0',
      primary: '#ff9800',
      secondary: '#ff5722',
      text: '#e65100'
    }
  },
  {
    id: 'lavender',
    name: '薰衣草',
    colors: {
      header: '#5e35b1',
      sidebar: '#f3e5f5',
      primary: '#7e57c2',
      secondary: '#9575cd',
      text: '#4527a0'
    }
  }
]

// 当前主题配置
const currentTheme = reactive<ThemeConfig>({
  preset: 'default',
  editor: 'monokai',
  fontSize: 14,
  lineHeight: 1.5,
  fontFamily: "'Fira Code', monospace",
  previewMode: 'auto',
  pdfZoom: 100,
  customColors: false,
  colors: {
    primary: '#409eff',
    accent: '#67c23a',
    background: '#ffffff',
    text: '#606266',
    border: '#dcdfe6'
  },
  panelPosition: 'left',
  previewPosition: 'bottom',
  compactToolbar: false,
  showMinimap: true,
  showLineNumbers: true
})

// 方法
function applyPreset(preset: ThemePreset) {
  currentTheme.preset = preset.id

  if (!currentTheme.customColors) {
    currentTheme.colors.primary = preset.colors.primary
    currentTheme.colors.accent = preset.colors.secondary
  }

  updateTheme()
}

function updateTheme() {
  emit('theme-change', { ...currentTheme })
}

function resetToDefault() {
  currentTheme.preset = 'default'
  currentTheme.editor = 'monokai'
  currentTheme.fontSize = 14
  currentTheme.lineHeight = 1.5
  currentTheme.fontFamily = "'Fira Code', monospace"
  currentTheme.previewMode = 'auto'
  currentTheme.pdfZoom = 100
  currentTheme.customColors = false
  currentTheme.colors = {
    primary: '#409eff',
    accent: '#67c23a',
    background: '#ffffff',
    text: '#606266',
    border: '#dcdfe6'
  }
  currentTheme.panelPosition = 'left'
  currentTheme.previewPosition = 'bottom'
  currentTheme.compactToolbar = false
  currentTheme.showMinimap = true
  currentTheme.showLineNumbers = true

  updateTheme()
  ElMessage.success('已重置为默认主题')
}

function saveTheme() {
  // 保存到 localStorage
  localStorage.setItem('latex-editor-theme', JSON.stringify(currentTheme))
  ElMessage.success('主题已保存')
}

function loadTheme() {
  const saved = localStorage.getItem('latex-editor-theme')
  if (saved) {
    try {
      const theme = JSON.parse(saved)
      Object.assign(currentTheme, theme)
    } catch (e) {
      console.error('Failed to load theme', e)
    }
  }
}

// 监听对话框打开
watch(() => props.show, (show) => {
  if (show) {
    loadTheme()
  }
})
</script>

<style scoped lang="scss">
.theme-customizer {
  display: flex;
  flex-direction: column;
  gap: 24px;
  padding: 16px;
}

.section {
  h4 {
    margin: 0 0 12px 0;
    font-size: 14px;
    font-weight: 500;
  }
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;

  h4 {
    margin: 0;
  }
}

.theme-presets {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px;
}

.theme-preset-card {
  cursor: pointer;
  border: 2px solid var(--el-border-color);
  border-radius: 8px;
  padding: 8px;
  transition: all 0.2s;

  &:hover {
    border-color: var(--el-color-primary-light-5);
    transform: translateY(-2px);
  }

  &.active {
    border-color: var(--el-color-primary);
    box-shadow: 0 0 0 2px var(--el-color-primary-light-8);
  }
}

.preset-preview {
  width: 100%;
  aspect-ratio: 4/3;
  border-radius: 4px;
  overflow: hidden;
  margin-bottom: 8px;
}

.preview-header {
  height: 25%;
  width: 100%;
}

.preview-body {
  height: 75%;
  width: 100%;
  display: flex;
}

.preview-sidebar {
  width: 30%;
  height: 100%;
}

.preview-content {
  flex: 1;
  padding: 6px;
  display: flex;
  flex-direction: column;
  gap: 4px;
  background: #f5f5f5;
}

.preview-line {
  height: 6px;
  border-radius: 2px;

  &.primary {
    width: 80%;
  }

  &.secondary {
    width: 60%;
  }

  &.text {
    width: 90%;
  }
}

.preset-name {
  text-align: center;
  font-size: 12px;
  font-weight: 500;
}

.color-customization {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.color-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  background: var(--el-fill-color-light);
  border-radius: 6px;

  label {
    font-size: 13px;
    color: var(--el-text-color-regular);
  }
}

.actions {
  display: flex;
  gap: 8px;
  padding-top: 16px;
  border-top: 1px solid var(--el-border-color);
}
</style>
