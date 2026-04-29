<template>
  <el-drawer
    v-model="visible"
    title="编辑器设置"
    direction="rtl"
    size="500px"
    :close-on-click-modal="false"
    class="settings-drawer"
  >
    <div class="settings-content">
      <!-- 编辑器设置 -->
      <div class="settings-section">
        <h3 class="section-title">
          <el-icon><Edit /></el-icon>
          编辑器
        </h3>

        <div class="setting-item">
          <label>字体大小</label>
          <el-slider
            v-model="settings.fontSize"
            :min="10"
            :max="24"
            :step="1"
            :marks="{ 10: '10', 14: '14', 18: '18', 24: '24' }"
            show-stops
          />
          <span class="value-label">{{ settings.fontSize }}px</span>
        </div>

        <div class="setting-item">
          <label>字体</label>
          <el-select v-model="settings.fontFamily" placeholder="选择字体">
            <el-option
              v-for="font in availableFonts"
              :key="font.value"
              :label="font.label"
              :value="font.value"
            >
              <span :style="{ fontFamily: font.value }">{{ font.label }}</span>
            </el-option>
          </el-select>
        </div>

        <div class="setting-item">
          <label>行高</label>
          <el-slider
            v-model="settings.lineHeight"
            :min="1.0"
            :max="2.5"
            :step="0.1"
            :marks="{ 1.0: '1.0', 1.6: '1.6', 2.0: '2.0' }"
            show-stops
          />
          <span class="value-label">{{ settings.lineHeight }}</span>
        </div>

        <div class="setting-item">
          <label>Tab 宽度</label>
          <el-input-number
            v-model="settings.tabSize"
            :min="2"
            :max="8"
            :step="1"
            size="small"
          />
          <span class="value-label">空格</span>
        </div>

        <div class="setting-item switch-item">
          <label>显示行号</label>
          <el-switch
            v-model="settings.showLineNumbers"
            active-text="开"
            inactive-text="关"
          />
        </div>

        <div class="setting-item switch-item">
          <label>自动换行</label>
          <el-switch
            v-model="settings.wordWrap"
            active-text="开"
            inactive-text="关"
          />
        </div>

        <div class="setting-item switch-item">
          <label>括号匹配</label>
          <el-switch
            v-model="settings.bracketMatching"
            active-text="开"
            inactive-text="关"
          />
        </div>
      </div>

      <el-divider />

      <!-- 主题设置 -->
      <div class="settings-section">
        <h3 class="section-title">
          <el-icon><Brush /></el-icon>
          主题
        </h3>

        <div class="setting-item">
          <label>编辑器主题</label>
          <el-select v-model="settings.theme" placeholder="选择主题">
            <el-option label="浅色" value="light" />
            <el-option label="深色" value="dark" />
            <el-option label="跟随系统" value="auto" />
          </el-select>
        </div>

        <div class="setting-item">
          <label>语法高亮</label>
          <el-select v-model="settings.highlightTheme" placeholder="选择主题">
            <el-option label="Monokai" value="monokai" />
            <el-option label="GitHub Dark" value="github-dark" />
            <el-option label="Dracula" value="dracula" />
            <el-option label="Solarized" value="solarized" />
          </el-select>
        </div>

        <div class="setting-item switch-item">
          <label>代码折叠</label>
          <el-switch
            v-model="settings.codeFolding"
            active-text="开"
            inactive-text="关"
          />
        </div>
      </div>

      <el-divider />

      <!-- 预览设置 -->
      <div class="settings-section">
        <h3 class="section-title">
          <el-icon><View /></el-icon>
          预览
        </h3>

        <div class="setting-item">
          <label>默认缩放</label>
          <el-slider
            v-model="settings.defaultZoom"
            :min="0.5"
            :max="2"
            :step="0.1"
            :marks="{ 0.5: '50%', 1: '100%', 1.5: '150%', 2: '200%' }"
            show-stops
          />
          <span class="value-label">{{ Math.round(settings.defaultZoom * 100) }}%</span>
        </div>

        <div class="setting-item switch-item">
          <label>同步滚动</label>
          <el-switch
            v-model="settings.syncScroll"
            active-text="开"
            inactive-text="关"
          />
        </div>

        <div class="setting-item">
          <label>更新频率</label>
          <el-select v-model="settings.updateFrequency">
            <el-option label="实时" value="realtime" />
            <el-option label="防抖 300ms" value="debounce-300" />
            <el-option label="防抖 500ms" value="debounce-500" />
            <el-option label="手动" value="manual" />
          </el-select>
        </div>

        <div class="setting-item switch-item">
          <label>显示渲染时间</label>
          <el-switch
            v-model="settings.showRenderTime"
            active-text="开"
            inactive-text="关"
          />
        </div>
      </div>

      <el-divider />

      <!-- 自动保存设置 -->
      <div class="settings-section">
        <h3 class="section-title">
          <el-icon><Clock /></el-icon>
          自动保存
        </h3>

        <div class="setting-item switch-item">
          <label>启用自动保存</label>
          <el-switch
            v-model="settings.autoSave"
            active-text="开"
            inactive-text="关"
          />
        </div>

        <div class="setting-item" v-if="settings.autoSave">
          <label>保存间隔</label>
          <el-select v-model="settings.autoSaveInterval">
            <el-option label="30 秒" :value="30000" />
            <el-option label="1 分钟" :value="60000" />
            <el-option label="2 分钟" :value="120000" />
            <el-option label="5 分钟" :value="300000" />
          </el-select>
        </div>

        <div class="setting-item switch-item" v-if="settings.autoSave">
          <label>保存本地备份</label>
          <el-switch
            v-model="settings.localBackup"
            active-text="开"
            inactive-text="关"
          />
        </div>
      </div>

      <el-divider />

      <!-- 快捷键设置 -->
      <div class="settings-section">
        <h3 class="section-title">
          <el-icon><Key /></el-icon>
          快捷键
        </h3>

        <div class="setting-item">
          <el-button @click="openShortcutHelp" text>
            <el-icon><QuestionFilled /></el-icon>
            查看所有快捷键
          </el-button>
          <el-button @click="customizeShortcuts" text>
            <el-icon><Edit /></el-icon>
            自定义快捷键
          </el-button>
        </div>
      </div>

      <el-divider />

      <!-- 高级设置 -->
      <div class="settings-section">
        <h3 class="section-title">
          <el-icon><Tools /></el-icon>
          高级
        </h3>

        <div class="setting-item">
          <label>编译器</label>
          <el-select v-model="settings.compiler">
            <el-option label="pdflatex" value="pdflatex" />
            <el-option label="xelatex" value="xelatex" />
            <el-option label="lualatex" value="lualatex" />
          </el-select>
        </div>

        <div class="setting-item">
          <label>编译选项</label>
          <el-checkbox-group v-model="settings.compileOptions">
            <el-checkbox label="交互模式" value="-interaction=nonstopmode" />
            <el-checkbox label="Halt on error" value="-halt-on-error" />
            <el-checkbox label="Shell escape" value="-shell-escape" />
          </el-checkbox-group>
        </div>

        <div class="setting-item">
          <label>最大编译时间（秒）</label>
          <el-input-number
            v-model="settings.compileTimeout"
            :min="10"
            :max="300"
            :step="10"
          />
        </div>

        <div class="setting-item">
          <el-button @click="clearCache" type="warning" text>
            <el-icon><Delete /></el-icon>
            清除缓存
          </el-button>
          <el-button @click="resetSettings" text>
            <el-icon><RefreshLeft /></el-icon>
            重置设置
          </el-button>
        </div>
      </div>
    </div>

    <!-- 底部操作栏 -->
    <template #footer>
      <div class="settings-footer">
        <el-button @click="resetSettings">
          重置为默认
        </el-button>
        <el-button type="primary" @click="applySettings">
          应用设置
        </el-button>
      </div>
    </template>
  </el-drawer>

  <!-- 快捷键帮助对话框 -->
  <ShortcutHelp ref="shortcutHelpRef" />
</template>

<script setup lang="ts">
import { ref, reactive, watch, computed } from 'vue'
import {
  Edit,
  View,
  Clock,
  Key,
  Tools,
  QuestionFilled,
  Brush,
  Delete,
  RefreshLeft,
  DocumentChecked
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import ShortcutHelp from './ShortcutHelp.vue'

interface Settings {
  fontSize: number
  fontFamily: string
  lineHeight: number
  tabSize: number
  showLineNumbers: boolean
  wordWrap: boolean
  bracketMatching: boolean
  theme: 'light' | 'dark' | 'auto'
  highlightTheme: string
  codeFolding: boolean
  defaultZoom: number
  syncScroll: boolean
  updateFrequency: string
  showRenderTime: boolean
  autoSave: boolean
  autoSaveInterval: number
  localBackup: boolean
  compiler: string
  compileOptions: string[]
  compileTimeout: number
}

const DEFAULT_SETTINGS: Settings = {
  fontSize: 14,
  fontFamily: "'Fira Code', 'Consolas', 'Monaco', monospace",
  lineHeight: 1.6,
  tabSize: 2,
  showLineNumbers: true,
  wordWrap: false,
  bracketMatching: true,
  theme: 'auto',
  highlightTheme: 'github-dark',
  codeFolding: true,
  defaultZoom: 1.0,
  syncScroll: true,
  updateFrequency: 'debounce-300',
  showRenderTime: false,
  autoSave: true,
  autoSaveInterval: 30000,
  localBackup: true,
  compiler: 'xelatex',
  compileOptions: ['-shell-escape'],
  compileTimeout: 60
}

const emit = defineEmits<{
  'update-settings': [settings: Settings]
  'clear-cache': []
  'update:visible': [value: boolean]
}>()

const props = defineProps<{
  visible?: boolean
}>()

const visible = computed({
  get: () => props.visible ?? false,
  set: (value: boolean) => emit('update:visible', value)
})
const shortcutHelpRef = ref<InstanceType<typeof ShortcutHelp> | null>(null)

const settings = reactive<Settings>({ ...DEFAULT_SETTINGS })

// 可用字体
const availableFonts = [
  { label: 'Fira Code', value: "'Fira Code', 'Consolas', 'Monaco', monospace" },
  { label: 'JetBrains Mono', value: "'JetBrains Mono', 'Consolas', monospace" },
  { label: 'Source Code Pro', value: "'Source Code Pro', 'Consolas', monospace" },
  { label: 'Consolas', value: "Consolas, 'Monaco', monospace" },
  { label: 'Monaco', value: "'Monaco', 'Consolas', monospace" },
  { label: 'Courier New', value: "'Courier New', Consolas, monospace" }
]

// 监听设置变化
watch(settings, () => {
  // 设置变化时自动保存到 localStorage
  try {
    localStorage.setItem('latex-editor-settings', JSON.stringify(settings))
  } catch (e) {
    console.warn('Failed to save settings to localStorage')
  }
}, { deep: true })

// 加载保存的设置
function loadSettings() {
  try {
    const saved = localStorage.getItem('latex-editor-settings')
    if (saved) {
      const parsed = JSON.parse(saved)
      Object.assign(settings, parsed)
    }
  } catch (e) {
    console.warn('Failed to load settings from localStorage')
  }
}

// 应用设置
function applySettings() {
  emit('update-settings', { ...settings })
  ElMessage.success('设置已应用')
  visible.value = false
}

// 重置设置
function resetSettings() {
  ElMessageBox.confirm(
    '确定要重置所有设置为默认值吗？',
    '重置设置',
    {
      type: 'warning',
      confirmButtonText: '重置',
      cancelButtonText: '取消'
    }
  ).then(() => {
    Object.assign(settings, DEFAULT_SETTINGS)
    ElMessage.success('设置已重置')
  }).catch(() => {
    // 用户取消
  })
}

// 清除缓存
function clearCache() {
  ElMessageBox.confirm(
    '清除缓存将删除所有本地保存的数据，包括自动保存的备份。',
    '清除缓存',
    {
      type: 'warning',
      confirmButtonText: '清除',
      cancelButtonText: '取消'
    }
  ).then(() => {
    emit('clear-cache')
    ElMessage.success('缓存已清除')
  }).catch(() => {
    // 用户取消
  })
}

// 打开快捷键帮助
function openShortcutHelp() {
  shortcutHelpRef.value?.open()
}

// 自定义快捷键
function customizeShortcuts() {
  ElMessage.info('快捷键自定义功能开发中...')
}

const open = () => {
  loadSettings()
  visible.value = true
}

defineExpose({ open })
</script>

<style scoped lang="scss">
.settings-drawer {
  :deep(.el-drawer__body) {
    padding: 0;
    display: flex;
    flex-direction: column;
  }
}

.settings-content {
  flex: 1;
  overflow-y: auto;
  padding: 20px;
}

.settings-section {
  margin-bottom: 24px;

  &:last-child {
    margin-bottom: 0;
  }
}

.section-title {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 16px;
  font-weight: 600;
  color: var(--el-text-color-primary);
  margin: 0 0 16px 0;
  padding-bottom: 8px;
  border-bottom: 1px solid var(--el-border-color-light);

  .el-icon {
    color: var(--el-color-primary);
  }
}

.setting-item {
  display: flex;
  align-items: center;
  margin-bottom: 16px;

  &:last-child {
    margin-bottom: 0;
  }

  label {
    flex: 1;
    font-size: 13px;
    color: var(--el-text-color-regular);
    margin-bottom: 8px;
  }

  .value-label {
    font-size: 12px;
    color: var(--el-text-color-secondary);
    min-width: 50px;
    text-align: right;
  }

  .el-select,
  .el-input-number,
  .el-slider {
    flex: 2;
  }
}

.switch-item {
  justify-content: space-between;
}

// 自定义滑块标记
:deep(.el-slider__marks-text) {
  font-size: 11px;
  color: var(--el-text-color-secondary);
}

// 设置底部
.settings-footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
  padding: 16px 20px;
  border-top: 1px solid var(--el-border-color);
  background: var(--el-fill-color-light);
}

// 复选框组样式
:deep(.el-checkbox-group) {
  display: flex;
  flex-direction: column;
  gap: 8px;

  .el-checkbox {
    margin-right: 0;
  }
}
</style>
