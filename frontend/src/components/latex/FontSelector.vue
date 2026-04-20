<template>
  <div class="font-selector">
    <div class="font-header">
      <h4>编辑器字体</h4>
      <el-button size="small" @click="resetToDefault" :icon="RefreshLeft">重置</el-button>
    </div>

    <div class="font-list">
      <div
        v-for="font in availableFonts"
        :key="font.id"
        class="font-item"
        :class="{ active: currentFont === font.id }"
        @click="selectFont(font)"
      >
        <div class="font-info">
          <div class="font-name" :style="{ fontFamily: font.family }">{{ font.name }}</div>
          <div class="font-preview" :style="{ fontFamily: font.family }">
            {{ font.preview }}
          </div>
          <div class="font-meta">
            <el-tag size="small" v-if="font.webfont">Web Font</el-tag>
            <el-tag size="small" type="info" v-if="font.ligatures">Ligatures</el-tag>
          </div>
        </div>
        <el-icon v-if="currentFont === font.id" class="check-icon"><Check /></el-icon>
      </div>
    </div>

    <div class="font-size-control">
      <div class="control-label">字体大小: {{ currentFontSize }}px</div>
      <el-slider
        v-model="currentFontSize"
        :min="10"
        :max="24"
        :step="1"
        show-input
        :show-input-controls="false"
        @change="(val: number | number[]) => updateFontSize(Array.isArray(val) ? val[0] : val)"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { RefreshLeft, Check } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import { useLatexEditorStore } from '@/architecture/stores/latexEditor'

interface FontOption {
  id: string
  name: string
  family: string
  preview: string
  webfont?: boolean
  ligatures?: boolean
  googleFont?: string
}

const emit = defineEmits<{
  fontChange: [font: string]
  sizeChange: [size: number]
}>()

const latexStore = useLatexEditorStore()

// 预定义字体选项
const availableFonts: FontOption[] = [
  {
    id: 'fira-code',
    name: 'Fira Code',
    family: '"Fira Code", "Fira Code VF", Consolas, Monaco, "Courier New", monospace',
    preview: 'function hello() { return "Hello World"; }',
    webfont: true,
    ligatures: true,
    googleFont: 'Fira+Code:wght@400;500;600&display=swap'
  },
  {
    id: 'jetbrains-mono',
    name: 'JetBrains Mono',
    family: '"JetBrains Mono", "Fira Code", Consolas, Monaco, monospace',
    preview: 'function hello() { return "Hello World"; }',
    webfont: true,
    ligatures: true,
    googleFont: 'JetBrains+Mono:wght@400;500;600&display=swap'
  },
  {
    id: 'source-code-pro',
    name: 'Source Code Pro',
    family: '"Source Code Pro", "Fira Code", Consolas, Monaco, monospace',
    preview: 'function hello() { return "Hello World"; }',
    webfont: true,
    ligatures: false,
    googleFont: 'Source+Code+Pro:wght@400;500;600&display=swap'
  },
  {
    id: 'ibm-plex-mono',
    name: 'IBM Plex Mono',
    family: '"IBM Plex Mono", "Fira Code", Consolas, Monaco, monospace',
    preview: 'function hello() { return "Hello World"; }',
    webfont: true,
    ligatures: true,
    googleFont: 'IBM+Plex+Mono:wght@400;500;600&display=swap'
  },
  {
    id: 'ubuntu-mono',
    name: 'Ubuntu Mono',
    family: '"Ubuntu Mono", "Fira Code", Consolas, Monaco, monospace',
    preview: 'function hello() { return "Hello World"; }',
    webfont: true,
    ligatures: false,
    googleFont: 'Ubuntu+Mono:wght@400;700&display=swap'
  },
  {
    id: 'consolas',
    name: 'Consolas',
    family: 'Consolas, "Fira Code", Monaco, "Courier New", monospace',
    preview: 'function hello() { return "Hello World"; }',
    webfont: false
  },
  {
    id: 'monaco',
    name: 'Monaco',
    family: 'Monaco, "Fira Code", Consolas, "Courier New", monospace',
    preview: 'function hello() { return "Hello World"; }',
    webfont: false
  },
  {
    id: 'courier',
    name: 'Courier New',
    family: '"Courier New", Consolas, Monaco, monospace',
    preview: 'function hello() { return "Hello World"; }',
    webfont: false
  }
]

const currentFont = ref<string>('fira-code')
const currentFontSize = ref<number>(14)
const loadedFonts = ref<Set<string>>(new Set())

onMounted(() => {
  // 从 store 加载当前设置
  currentFontSize.value = latexStore.editorSettings.fontSize

  // 解析当前字体，匹配到预设选项
  const currentFamily = latexStore.editorSettings.fontFamily
  const matchedFont = availableFonts.find(f => currentFamily.includes(f.name.split(' ')[0]))

  if (matchedFont) {
    currentFont.value = matchedFont.id
  } else {
    currentFont.value = 'fira-code'
  }
})

function selectFont(font: FontOption) {
  currentFont.value = font.id

  // 加载 Google Fonts
  if (font.googleFont && !loadedFonts.value.has(font.id)) {
    loadGoogleFont(font.googleFont)
    loadedFonts.value.add(font.id)
  }

  // 更新 store
  latexStore.updateEditorSettings({ fontFamily: font.family })

  // 保存到 localStorage
  localStorage.setItem('latex_editor_font', font.id)

  emit('fontChange', font.family)

  ElMessage.success(`字体已切换为 ${font.name}`)
}

function updateFontSize(size: number) {
  latexStore.updateEditorSettings({ fontSize: size })
  localStorage.setItem('latex_editor_font_size', String(size))
  emit('sizeChange', size)
}

function resetToDefault() {
  selectFont(availableFonts[0])
  updateFontSize(14)
  ElMessage.info('已重置为默认字体')
}

function loadGoogleFont(fontUrl: string) {
  const linkId = `google-font-${fontUrl.replace(/\W/g, '-')}`

  if (document.getElementById(linkId)) {
    return
  }

  const link = document.createElement('link')
  link.id = linkId
  link.href = `https://fonts.googleapis.com/css2?${fontUrl}`
  link.rel = 'stylesheet'
  document.head.appendChild(link)
}

// 从 localStorage 恢复设置
function restoreSettings() {
  const savedFont = localStorage.getItem('latex_editor_font')
  const savedSize = localStorage.getItem('latex_editor_font_size')

  if (savedFont) {
    const font = availableFonts.find(f => f.id === savedFont)
    if (font) {
      selectFont(font)
    }
  }

  if (savedSize) {
    const size = parseInt(savedSize, 10)
    if (size >= 10 && size <= 24) {
      updateFontSize(size)
      currentFontSize.value = size
    }
  }
}

restoreSettings()
</script>

<style scoped lang="scss">
.font-selector {
  padding: 16px;

  .font-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 16px;

    h4 {
      margin: 0;
      font-size: 16px;
      font-weight: 600;
    }
  }

  .font-list {
    display: flex;
    flex-direction: column;
    gap: 12px;
    margin-bottom: 20px;
    max-height: 400px;
    overflow-y: auto;

    .font-item {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 12px;
      border: 1px solid var(--el-border-color-lighter);
      border-radius: 8px;
      cursor: pointer;
      transition: all 0.2s;

      &:hover {
        border-color: var(--el-color-primary);
        background: var(--el-fill-color-light);
      }

      &.active {
        border-color: var(--el-color-primary);
        background: var(--el-color-primary-light-9);

        .check-icon {
          color: var(--el-color-primary);
        }
      }

      .font-info {
        flex: 1;
        min-width: 0;

        .font-name {
          font-size: 14px;
          font-weight: 600;
          margin-bottom: 4px;
        }

        .font-preview {
          font-size: 13px;
          color: var(--el-text-color-regular);
          margin-bottom: 8px;
          line-height: 1.6;
        }

        .font-meta {
          display: flex;
          gap: 6px;
        }
      }

      .check-icon {
        font-size: 20px;
        margin-left: 12px;
      }
    }
  }

  .font-size-control {
    padding-top: 16px;
    border-top: 1px solid var(--el-border-color-lighter);

    .control-label {
      font-size: 13px;
      color: var(--el-text-color-secondary);
      margin-bottom: 8px;
    }
  }
}
</style>
