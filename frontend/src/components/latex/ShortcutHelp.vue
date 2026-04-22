<template>
  <el-dialog
    v-model="visible"
    title="键盘快捷键"
    width="700px"
    :close-on-click-modal="false"
    class="shortcut-help-dialog"
  >
    <div class="shortcut-help-content">
      <div class="shortcut-section" v-for="section in shortcuts" :key="section.category">
        <h3 class="section-title">{{ section.category }}</h3>
        <div class="shortcut-list">
          <div
            class="shortcut-item"
            v-for="item in section.items"
            :key="item.description"
          >
            <div class="shortcut-description">{{ item.description }}</div>
            <div class="shortcut-keys">
              <kbd
                v-for="(key, index) in item.keys"
                :key="index"
                class="shortcut-key"
              >
                {{ key }}
              </kbd>
            </div>
          </div>
        </div>
      </div>
    </div>
    <template #footer>
      <el-button @click="visible = false" type="primary">关闭</el-button>
    </template>
  </el-dialog>
</template>

<script setup lang="ts">
import { ref } from 'vue'

interface ShortcutItem {
  description: string
  keys: string[]
}

interface ShortcutSection {
  category: string
  items: ShortcutItem[]
}

const visible = ref(false)

const shortcuts: ShortcutSection[] = [
  {
    category: '文件操作',
    items: [
      { description: '保存文档', keys: ['Ctrl', 'S'] },
      { description: '编译文档', keys: ['Ctrl', 'Enter'] },
      { description: '新建文档', keys: ['Ctrl', 'Alt', 'N'] },
      { description: '导出PDF', keys: ['Ctrl', 'E'] },
    ]
  },
  {
    category: '编辑操作',
    items: [
      { description: '撤销', keys: ['Ctrl', 'Z'] },
      { description: '重做', keys: ['Ctrl', 'Shift', 'Z'] },
      { description: '查找', keys: ['Ctrl', 'F'] },
      { description: '替换', keys: ['Ctrl', 'H'] },
      { description: '注释/取消注释', keys: ['Ctrl', '/'] },
      { description: '删除行', keys: ['Ctrl', 'D'] },
      { description: '复制行', keys: ['Ctrl', 'Shift', 'D'] },
    ]
  },
  {
    category: '光标移动',
    items: [
      { description: '移动到行首', keys: ['Home'] },
      { description: '移动到行尾', keys: ['End'] },
      { description: '移动到文档开头', keys: ['Ctrl', 'Home'] },
      { description: '移动到文档结尾', keys: ['Ctrl', 'End'] },
      { description: '跳转到行', keys: ['Ctrl', 'G'] },
    ]
  },
  {
    category: '选择操作',
    items: [
      { description: '全选', keys: ['Ctrl', 'A'] },
      { description: '选择当前行', keys: ['Ctrl', 'L'] },
      { description: '选择到行首', keys: ['Shift', 'Home'] },
      { description: '选择到行尾', keys: ['Shift', 'End'] },
    ]
  },
  {
    category: '视图操作',
    items: [
      { description: '切换预览面板', keys: ['Ctrl', 'Shift', 'P'] },
      { description: '切换大纲面板', keys: ['Ctrl', 'Shift', 'O'] },
      { description: '切换侧边栏', keys: ['Ctrl', 'B'] },
      { description: '放大', keys: ['Ctrl', '+'] },
      { description: '缩小', keys: ['Ctrl', '-'] },
      { description: '重置缩放', keys: ['Ctrl', '0'] },
    ]
  },
  {
    category: 'LaTeX特定',
    items: [
      { description: '插入命令', keys: ['Ctrl', 'Space'] },
      { description: '插入环境', keys: ['Ctrl', 'E'] },
      { description: '格式化代码', keys: ['Shift', 'Alt', 'F'] },
      { description: '插入图片', keys: ['Ctrl', 'Alt', 'I'] },
      { description: '插入表格', keys: ['Ctrl', 'Alt', 'T'] },
      { description: '插入引用', keys: ['Ctrl', 'Alt', 'R'] },
    ]
  },
  {
    category: '工具面板',
    items: [
      { description: '打开符号面板', keys: ['Ctrl', 'Alt', 'S'] },
      { description: '打开代码片段', keys: ['Ctrl', 'Alt', 'C'] },
      { description: '打开表格生成器', keys: ['Ctrl', 'Alt', 'G'] },
      { description: '打开模板管理', keys: ['Ctrl', 'Alt', 'M'] },
      { description: '打开快捷键帮助', keys: ['F1'] },
      { description: '打开插入面板', keys: ['Ctrl', 'Alt', 'X'] },
    ]
  }
]

const open = () => {
  visible.value = true
}

defineExpose({ open })
</script>

<style scoped lang="scss">
.shortcut-help-dialog {
  :deep(.el-dialog__body) {
    padding: 0 20px;
    max-height: 60vh;
    overflow-y: auto;
  }
}

.shortcut-help-content {
  padding: 10px 0;
}

.shortcut-section {
  margin-bottom: 24px;

  &:last-child {
    margin-bottom: 0;
  }
}

.section-title {
  font-size: 14px;
  font-weight: 600;
  color: var(--el-text-color-primary);
  margin: 0 0 12px 0;
  padding-bottom: 8px;
  border-bottom: 2px solid var(--el-border-color-light);
}

.shortcut-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.shortcut-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  border-radius: 6px;
  background-color: var(--el-fill-color-light);
  transition: background-color 0.2s;

  &:hover {
    background-color: var(--el-fill-color);
  }
}

.shortcut-description {
  font-size: 13px;
  color: var(--el-text-color-regular);
}

.shortcut-keys {
  display: flex;
  gap: 4px;
  align-items: center;
}

.shortcut-key {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 28px;
  height: 28px;
  padding: 0 8px;
  font-size: 12px;
  font-weight: 500;
  font-family: 'Consolas', 'Monaco', monospace;
  color: var(--el-text-color-primary);
  background: linear-gradient(180deg, #ffffff 0%, #f5f7fa 100%);
  border: 1px solid var(--el-border-color);
  border-radius: 4px;
  box-shadow:
    0 1px 2px rgba(0, 0, 0, 0.05),
    inset 0 1px 0 rgba(255, 255, 255, 0.8);

  &:not(:last-child)::after {
    content: '+';
    margin-left: 4px;
    color: var(--el-text-color-secondary);
    font-size: 11px;
  }
}

// 暗色模式适配
@media (prefers-color-scheme: dark) {
  .shortcut-key {
    background: linear-gradient(180deg, #2a2a2a 0%, #1f1f1f 100%);
    border-color: var(--el-border-color-darker);
  }
}
</style>
