/**
 * 命令面板
 * VS Code风格命令面板，支持模糊搜索、快捷键、最近使用
 */

<template>
  <teleport to="body">
    <transition name="fade">
      <div v-if="show" class="command-palette-overlay" @click.self="close">
        <div class="command-palette" :class="{ 'is-full-screen': fullScreen }">
          <!-- 搜索框 -->
          <div class="palette-search">
            <el-icon class="search-icon"><Search /></el-icon>
            <input
              ref="searchInputRef"
              v-model="searchQuery"
              type="text"
              placeholder="输入命令或搜索..."
              class="search-input"
              @keydown="handleKeydown"
            />
            <div v-if="searchQuery" class="search-clear" @click="clearSearch">
              <el-icon><Close /></el-icon>
            </div>
          </div>

          <!-- 快捷提示 -->
          <div v-if="!searchQuery" class="palette-hint">
            <span class="hint-item">
              <kbd>↑↓</kbd> 导航
            </span>
            <span class="hint-item">
              <kbd>Enter</kbd> 执行
            </span>
            <span class="hint-item">
              <kbd>Esc</kbd> 关闭
            </span>
            <span class="hint-item">
              <kbd>Tab</kbd> 切换分类
            </span>
          </div>

          <!-- 分类标签 -->
          <div v-if="!searchQuery && categories.length > 1" class="palette-categories">
            <div
              v-for="category in categories"
              :key="category.id"
              class="category-item"
              :class="{ 'is-active': activeCategory === category.id }"
              @click="activeCategory = category.id"
            >
              <el-icon v-if="category.icon">
                <component :is="category.icon" />
              </el-icon>
              <span>{{ category.label }}</span>
              <span class="category-count">{{ category.count }}</span>
            </div>
          </div>

          <!-- 命令列表 -->
          <div class="palette-commands">
            <!-- 最近使用 -->
            <div v-if="!searchQuery && recentCommands.length > 0" class="command-group">
              <div class="group-header">
                <el-icon><Clock /></el-icon>
                最近使用
              </div>
              <div
                v-for="cmd in recentCommands"
                :key="cmd.id"
                class="command-item"
                :class="{ 'is-selected': selectedIndex === recentCommands.indexOf(cmd) }"
                @click="executeCommand(cmd)"
                @mouseenter="selectedIndex = recentCommands.indexOf(cmd)"
              >
                <div class="command-left">
                  <el-icon class="command-icon" :color="cmd.color">
                    <component :is="cmd.icon" />
                  </el-icon>
                  <div class="command-info">
                    <div class="command-title">{{ cmd.title }}</div>
                    <div class="command-desc">{{ cmd.description }}</div>
                  </div>
                </div>
                <div v-if="cmd.shortcut" class="command-shortcut">
                  <kbd v-for="key in cmd.shortcut" :key="key">{{ key }}</kbd>
                </div>
              </div>
            </div>

            <!-- 搜索结果或分类命令 -->
            <div v-if="searchQuery" class="command-group">
              <div class="group-header">
                <el-icon><Search /></el-icon>
                搜索结果 ({{ filteredCommands.length }})
              </div>
              <div
                v-for="cmd in filteredCommands"
                :key="cmd.id"
                class="command-item"
                :class="{ 'is-selected': selectedIndex === filteredCommands.indexOf(cmd) }"
                @click="executeCommand(cmd)"
                @mouseenter="selectedIndex = filteredCommands.indexOf(cmd)"
              >
                <div class="command-left">
                  <el-icon class="command-icon" :color="cmd.color">
                    <component :is="cmd.icon" />
                  </el-icon>
                  <div class="command-info">
                    <div class="command-title" v-html="highlightMatch(cmd.title)"></div>
                    <div class="command-desc">{{ cmd.description }}</div>
                  </div>
                </div>
                <div v-if="cmd.shortcut" class="command-shortcut">
                  <kbd v-for="key in cmd.shortcut" :key="key">{{ key }}</kbd>
                </div>
              </div>
              <el-empty v-if="filteredCommands.length === 0" description="没有找到匹配的命令" :image-size="60" />
            </div>

            <!-- 分类命令 -->
            <div v-if="!searchQuery" class="command-group">
              <div
                v-for="category in categories"
                :key="category.id"
                v-show="activeCategory === 'all' || activeCategory === category.id"
              >
                <div class="group-header">
                  <el-icon v-if="category.icon">
                    <component :is="category.icon" />
                  </el-icon>
                  {{ category.label }}
                </div>
                <div
                  v-for="cmd in commandsByCategory[category.id]"
                  :key="cmd.id"
                  class="command-item"
                  :class="{ 'is-selected': selectedIndex === getGlobalIndex(cmd, category.id) }"
                  @click="executeCommand(cmd)"
                  @mouseenter="selectedIndex = getGlobalIndex(cmd, category.id)"
                >
                  <div class="command-left">
                    <el-icon class="command-icon" :color="cmd.color">
                      <component :is="cmd.icon" />
                    </el-icon>
                    <div class="command-info">
                      <div class="command-title">{{ cmd.title }}</div>
                      <div class="command-desc">{{ cmd.description }}</div>
                    </div>
                  </div>
                  <div v-if="cmd.shortcut" class="command-shortcut">
                    <kbd v-for="key in cmd.shortcut" :key="key">{{ key }}</kbd>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <!-- 底部状态栏 -->
          <div v-if="selectedCommand" class="palette-footer">
            <div class="footer-info">
              <el-icon><InfoFilled /></el-icon>
              {{ selectedCommand.description }}
            </div>
          </div>
        </div>
      </div>
    </transition>
  </teleport>
</template>

<script setup lang="ts">
import { ref, computed, watch, nextTick } from 'vue'
import {
  Search,
  Close,
  Clock,
  InfoFilled,
  Document,
  FolderOpened,
  Edit,
  View,
  Tools,
  Setting,
  Share,
  Download,
  Upload,
  Picture,
  Grid,
  MagicStick,
  DocumentCopy,
  Delete,
  Refresh
} from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import { useLocalStorage } from '@vueuse/core'

interface Command {
  id: string
  title: string
  description: string
  category: string
  icon: any
  color?: string
  shortcut?: string[]
  action: () => void | Promise<void>
  keywords?: string[]
}

interface CommandCategory {
  id: string
  label: string
  icon?: any
  count: number
}

const props = defineProps<{
  modelValue: boolean
  fullScreen?: boolean
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  'command-executed': [command: Command]
}>()

// 状态
const searchQuery = ref('')
const selectedIndex = ref(0)
const activeCategory = ref<string>('all')
const searchInputRef = ref<HTMLInputElement>()

// 最近使用的命令
const recentCommands = useLocalStorage<Command[]>('command-palette-recent', [], {
  serializer: {
    read: (v) => {
      try {
        const parsed = JSON.parse(v as string)
        return parsed.map((cmd: any) => ({
          ...cmd,
          icon: getIconComponent(cmd.iconName)
        }))
      } catch {
        return []
      }
    },
    write: (v) => JSON.stringify(v.map(cmd => ({
      ...cmd,
      iconName: cmd.icon?.name || cmd.icon
    })))
  }
})

// 命令定义
const commands = ref<Command[]>([
  // 文件操作
  {
    id: 'file.new',
    title: '新建文档',
    description: '创建新的LaTeX文档',
    category: 'file',
    icon: Document,
    color: '#409eff',
    shortcut: ['Ctrl', 'N'],
    action: () => ElMessage.info('创建新文档'),
    keywords: ['new', 'create', '文档', 'create file']
  },
  {
    id: 'file.open',
    title: '打开文件',
    description: '打开本地LaTeX文件',
    category: 'file',
    icon: FolderOpened,
    color: '#409eff',
    shortcut: ['Ctrl', 'O'],
    action: () => ElMessage.info('打开文件对话框'),
    keywords: ['open', 'load', '打开', 'file']
  },
  {
    id: 'file.save',
    title: '保存',
    description: '保存当前文档',
    category: 'file',
    icon: Download,
    color: '#409eff',
    shortcut: ['Ctrl', 'S'],
    action: () => ElMessage.success('文档已保存'),
    keywords: ['save', '保存']
  },
  {
    id: 'file.saveAs',
    title: '另存为',
    description: '将文档保存为其他文件',
    category: 'file',
    icon: Upload,
    color: '#409eff',
    action: () => ElMessage.info('另存为'),
    keywords: ['save as', 'export', '另存']
  },
  {
    id: 'file.export',
    title: '导出PDF',
    description: '编译并导出为PDF',
    category: 'file',
    icon: Download,
    color: '#409eff',
    action: () => ElMessage.success('正在导出PDF...'),
    keywords: ['export', 'pdf', 'compile', '导出', '编译']
  },

  // 编辑操作
  {
    id: 'edit.format',
    title: '格式化文档',
    description: '格式化LaTeX代码',
    category: 'edit',
    icon: Edit,
    color: '#67c23a',
    shortcut: ['Shift', 'Alt', 'F'],
    action: () => ElMessage.success('格式化完成'),
    keywords: ['format', 'beautify', '格式化', '美化']
  },
  {
    id: 'edit.find',
    title: '查找',
    description: '在文档中查找文本',
    category: 'edit',
    icon: Search,
    color: '#67c23a',
    shortcut: ['Ctrl', 'F'],
    action: () => ElMessage.info('查找'),
    keywords: ['find', 'search', '查找', '搜索']
  },
  {
    id: 'edit.replace',
    title: '替换',
    description: '查找并替换文本',
    category: 'edit',
    icon: Edit,
    color: '#67c23a',
    shortcut: ['Ctrl', 'H'],
    action: () => ElMessage.info('替换'),
    keywords: ['replace', '替换']
  },
  {
    id: 'edit.undo',
    title: '撤销',
    description: '撤销上一步操作',
    category: 'edit',
    icon: Refresh,
    color: '#67c23a',
    shortcut: ['Ctrl', 'Z'],
    action: () => ElMessage.info('撤销'),
    keywords: ['undo', '撤销']
  },
  {
    id: 'edit.redo',
    title: '重做',
    description: '重做上一步操作',
    category: 'edit',
    icon: Refresh,
    color: '#67c23a',
    shortcut: ['Ctrl', 'Shift', 'Z'],
    action: () => ElMessage.info('重做'),
    keywords: ['redo', '重做']
  },

  // 视图操作
  {
    id: 'view.preview',
    title: '预览PDF',
    description: '切换PDF预览面板',
    category: 'view',
    icon: View,
    color: '#e6a23c',
    shortcut: ['Ctrl', 'Shift', 'P'],
    action: () => ElMessage.info('切换预览'),
    keywords: ['preview', 'pdf', '预览']
  },
  {
    id: 'view.outline',
    title: '大纲',
    description: '显示/隐藏文档大纲',
    category: 'view',
    icon: Grid,
    color: '#e6a23c',
    action: () => ElMessage.info('切换大纲'),
    keywords: ['outline', 'toc', '大纲', '目录']
  },
  {
    id: 'view.fullscreen',
    title: '全屏模式',
    description: '切换全屏编辑',
    category: 'view',
    icon: View,
    color: '#e6a23c',
    shortcut: ['F11'],
    action: () => {
      if (document.fullscreenElement) {
        document.exitFullscreen()
      } else {
        document.documentElement.requestFullscreen()
      }
    },
    keywords: ['fullscreen', '全屏']
  },
  {
    id: 'view.zen',
    title: '禅模式',
    description: '无干扰编辑模式',
    category: 'view',
    icon: View,
    color: '#e6a23c',
    action: () => ElMessage.info('进入禅模式'),
    keywords: ['zen', 'focus', 'distraction-free', '禅', '专注']
  },

  // 工具操作
  {
    id: 'tools.snippets',
    title: '代码片段',
    description: '插入LaTeX代码片段',
    category: 'tools',
    icon: DocumentCopy,
    color: '#909399',
    action: () => ElMessage.info('打开代码片段'),
    keywords: ['snippet', 'template', '片段', '模板']
  },
  {
    id: 'tools.images',
    title: '图片管理',
    description: '管理文档中的图片',
    category: 'tools',
    icon: Picture,
    color: '#909399',
    action: () => ElMessage.info('打开图片管理器'),
    keywords: ['image', 'picture', 'figure', '图片', '图']
  },
  {
    id: 'tools.bibtex',
    title: 'BibTeX管理',
    description: '管理文献引用',
    category: 'tools',
    icon: Document,
    color: '#909399',
    action: () => ElMessage.info('打开BibTeX管理器'),
    keywords: ['bibtex', 'reference', 'citation', '文献', '引用']
  },
  {
    id: 'tools.macro',
    title: '宏管理器',
    description: '管理LaTeX宏和环境',
    category: 'tools',
    icon: MagicStick,
    color: '#909399',
    action: () => ElMessage.info('打开宏管理器'),
    keywords: ['macro', 'environment', '宏', '环境']
  },
  {
    id: 'tools.compiler',
    title: '编译设置',
    description: '配置LaTeX编译器',
    category: 'tools',
    icon: Tools,
    color: '#909399',
    action: () => ElMessage.info('打开编译设置'),
    keywords: ['compiler', 'compile', 'build', '编译', '构建']
  },

  // 设置操作
  {
    id: 'settings.preferences',
    title: '偏好设置',
    description: '打开设置页面',
    category: 'settings',
    icon: Setting,
    color: '#f56c6c',
    action: () => ElMessage.info('打开设置'),
    keywords: ['settings', 'preferences', 'options', '设置', '选项']
  },
  {
    id: 'settings.theme',
    title: '切换主题',
    description: '切换深色/浅色主题',
    category: 'settings',
    icon: View,
    color: '#f56c6c',
    action: () => ElMessage.success('主题已切换'),
    keywords: ['theme', 'dark', 'light', '主题', '深色', '浅色']
  },
  {
    id: 'settings.keyboard',
    title: '键盘快捷键',
    description: '查看和编辑快捷键',
    category: 'settings',
    icon: Edit,
    color: '#f56c6c',
    action: () => ElMessage.info('打开快捷键设置'),
    keywords: ['keyboard', 'shortcut', 'hotkey', '键盘', '快捷键']
  },

  // 协作操作
  {
    id: 'collab.share',
    title: '分享文档',
    description: '生成分享链接',
    category: 'collab',
    icon: Share,
    color: '#a855f7',
    action: () => ElMessage.success('分享链接已复制'),
    keywords: ['share', 'collaborate', '分享', '协作']
  },
  {
    id: 'collab.comments',
    title: '查看评论',
    description: '打开评论面板',
    category: 'collab',
    icon: Edit,
    color: '#a855f7',
    action: () => ElMessage.info('打开评论面板'),
    keywords: ['comment', 'review', '评论', '审阅']
  },
  {
    id: 'collab.history',
    title: '版本历史',
    description: '查看文档版本历史',
    category: 'collab',
    icon: Clock,
    color: '#a855f7',
    action: () => ElMessage.info('打开版本历史'),
    keywords: ['history', 'version', 'git', '历史', '版本']
  }
])

// 分类定义
const categories = computed<CommandCategory[]>(() => [
  { id: 'all', label: '全部', count: commands.value.length },
  { id: 'file', label: '文件', icon: Document, count: commands.value.filter(c => c.category === 'file').length },
  { id: 'edit', label: '编辑', icon: Edit, count: commands.value.filter(c => c.category === 'edit').length },
  { id: 'view', label: '视图', icon: View, count: commands.value.filter(c => c.category === 'view').length },
  { id: 'tools', label: '工具', icon: Tools, count: commands.value.filter(c => c.category === 'tools').length },
  { id: 'settings', label: '设置', icon: Setting, count: commands.value.filter(c => c.category === 'settings').length },
  { id: 'collab', label: '协作', icon: Share, count: commands.value.filter(c => c.category === 'collab').length }
])

// 按分类分组的命令
const commandsByCategory = computed(() => {
  const grouped: Record<string, Command[]> = {}
  commands.value.forEach(cmd => {
    if (!grouped[cmd.category]) {
      grouped[cmd.category] = []
    }
    grouped[cmd.category].push(cmd)
  })
  return grouped
})

// 模糊搜索过滤
const filteredCommands = computed(() => {
  if (!searchQuery.value.trim()) return []

  const query = searchQuery.value.toLowerCase()

  return commands.value.filter(cmd => {
    // 标题匹配
    if (cmd.title.toLowerCase().includes(query)) return true

    // 描述匹配
    if (cmd.description.toLowerCase().includes(query)) return true

    // 关键词匹配
    if (cmd.keywords?.some(kw => kw.toLowerCase().includes(query))) return true

    return false
  }).sort((a, b) => {
    // 优先显示标题匹配的
    const aTitleMatch = a.title.toLowerCase().startsWith(query)
    const bTitleMatch = b.title.toLowerCase().startsWith(query)
    if (aTitleMatch && !bTitleMatch) return -1
    if (!aTitleMatch && bTitleMatch) return 1

    // 优先显示最近使用的
    const aRecent = recentCommands.value.findIndex(c => c.id === a.id)
    const bRecent = recentCommands.value.findIndex(c => c.id === b.id)
    if (aRecent >= 0 && bRecent < 0) return -1
    if (aRecent < 0 && bRecent >= 0) return 1
    if (aRecent >= 0 && bRecent >= 0) return aRecent - bRecent

    return 0
  })
})

// 当前选中的命令
const selectedCommand = computed(() => {
  const displayList = searchQuery.value ? filteredCommands.value : [...recentCommands.value, ...commands.value]
  return displayList[selectedIndex.value] || null
})

// 是否显示
const show = computed({
  get: () => props.modelValue,
  set: (value) => emit('update:modelValue', value)
})

// 高亮匹配文本
const highlightMatch = (text: string) => {
  if (!searchQuery.value) return text

  const regex = new RegExp(`(${searchQuery.value})`, 'gi')
  return text.replace(regex, '<mark>$1</mark>')
}

// 获取全局索引
const getGlobalIndex = (cmd: Command, category: string) => {
  const recentCount = recentCommands.value.length
  const categoryCommands = commandsByCategory.value[category] || []
  const cmdIndex = categoryCommands.findIndex(c => c.id === cmd.id)

  // 计算在所有可见命令中的位置
  let index = recentCount

  for (const [catId, cmds] of Object.entries(commandsByCategory.value)) {
    if (activeCategory.value !== 'all' && activeCategory.value !== catId) continue

    if (catId === category) {
      return index + cmdIndex
    }

    index += cmds.length
  }

  return index
}

// 获取图标组件
const getIconComponent = (iconName: string) => {
  const icons: Record<string, any> = {
    Document, FolderOpened, Edit, View, Tools, Setting, Share,
    Download, Upload, Picture, Grid, MagicStick, DocumentCopy,
    Delete, Refresh, Search, Clock, Close, InfoFilled
  }
  return icons[iconName] || Document
}

// 执行命令
const executeCommand = async (cmd: Command) => {
  await cmd.action()

  // 添加到最近使用
  const recentIndex = recentCommands.value.findIndex(c => c.id === cmd.id)
  if (recentIndex >= 0) {
    recentCommands.value.splice(recentIndex, 1)
  }
  recentCommands.value.unshift(cmd)

  // 最多保留10个
  if (recentCommands.value.length > 10) {
    recentCommands.value = recentCommands.value.slice(0, 10)
  }

  emit('command-executed', cmd)
  close()
}

// 键盘导航
const handleKeydown = (event: KeyboardEvent) => {
  const displayList = searchQuery.value ? filteredCommands.value : [...recentCommands.value, ...commands.value]

  switch (event.key) {
    case 'ArrowDown':
      event.preventDefault()
      selectedIndex.value = Math.min(selectedIndex.value + 1, displayList.length - 1)
      break
    case 'ArrowUp':
      event.preventDefault()
      selectedIndex.value = Math.max(selectedIndex.value - 1, 0)
      break
    case 'Enter':
      event.preventDefault()
      if (selectedCommand.value) {
        executeCommand(selectedCommand.value)
      }
      break
    case 'Tab':
      event.preventDefault()
      const currentIndex = categories.value.findIndex(c => c.id === activeCategory.value)
      const nextIndex = (currentIndex + 1) % categories.value.length
      activeCategory.value = categories.value[nextIndex].id
      selectedIndex.value = 0
      break
    case 'Escape':
      close()
      break
  }
}

// 清空搜索
const clearSearch = () => {
  searchQuery.value = ''
  selectedIndex.value = 0
}

// 关闭面板
const close = () => {
  show.value = false
  searchQuery.value = ''
  selectedIndex.value = 0
  activeCategory.value = 'all'
}

// 监听显示状态
watch(() => props.modelValue, async (visible) => {
  if (visible) {
    await nextTick()
    searchInputRef.value?.focus()
  }
})
</script>

<style scoped lang="scss">
.command-palette-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  backdrop-filter: blur(4px);
  z-index: 9999;
  display: flex;
  align-items: flex-start;
  justify-content: center;
  padding-top: 15vh;
}

.command-palette {
  width: 600px;
  max-width: 90vw;
  max-height: 70vh;
  background: var(--el-bg-color);
  border-radius: 12px;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
  overflow: hidden;
  display: flex;
  flex-direction: column;

  &.is-full-screen {
    width: 90vw;
    max-height: 80vh;
  }
}

.palette-search {
  display: flex;
  align-items: center;
  padding: 16px 20px;
  border-bottom: 1px solid var(--el-border-color-lighter);
  gap: 12px;

  .search-icon {
    font-size: 20px;
    color: var(--el-text-color-secondary);
  }

  .search-input {
    flex: 1;
    border: none;
    outline: none;
    font-size: 16px;
    background: transparent;
    color: var(--el-text-color-primary);

    &::placeholder {
      color: var(--el-text-color-placeholder);
    }
  }

  .search-clear {
    cursor: pointer;
    color: var(--el-text-color-secondary);
    transition: color 0.2s;

    &:hover {
      color: var(--el-text-color-primary);
    }
  }
}

.palette-hint {
  display: flex;
  gap: 16px;
  padding: 8px 20px;
  background: var(--el-fill-color-light);
  font-size: 12px;
  color: var(--el-text-color-secondary);

  .hint-item {
    display: flex;
    align-items: center;
    gap: 4px;

    kbd {
      padding: 2px 6px;
      background: var(--el-bg-color);
      border: 1px solid var(--el-border-color);
      border-radius: 4px;
      font-family: monospace;
      font-size: 11px;
    }
  }
}

.palette-categories {
  display: flex;
  padding: 8px 12px;
  gap: 4px;
  border-bottom: 1px solid var(--el-border-color-lighter);
  overflow-x: auto;

  &::-webkit-scrollbar {
    height: 4px;
  }
}

.category-item {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 12px;
  border-radius: 6px;
  cursor: pointer;
  white-space: nowrap;
  transition: all 0.2s;
  font-size: 13px;
  color: var(--el-text-color-regular);

  &:hover {
    background: var(--el-fill-color-light);
  }

  &.is-active {
    background: var(--el-color-primary);
    color: white;
  }

  .category-count {
    font-size: 11px;
    opacity: 0.7;
  }
}

.palette-commands {
  flex: 1;
  overflow-y: auto;
  padding: 8px;

  &::-webkit-scrollbar {
    width: 8px;
  }

  &::-webkit-scrollbar-thumb {
    background: var(--el-border-color-darker);
    border-radius: 4px;
  }
}

.command-group {
  margin-bottom: 8px;

  &:last-child {
    margin-bottom: 0;
  }
}

.group-header {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 12px;
  font-size: 12px;
  font-weight: 600;
  color: var(--el-text-color-secondary);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.command-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 10px 12px;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s;

  &:hover,
  &.is-selected {
    background: var(--el-fill-color-light);
  }

  &.is-selected {
    background: var(--el-color-primary-light-9);
  }
}

.command-left {
  display: flex;
  align-items: center;
  gap: 12px;
  flex: 1;
  min-width: 0;
}

.command-icon {
  flex-shrink: 0;
  font-size: 18px;
}

.command-info {
  flex: 1;
  min-width: 0;
}

.command-title {
  font-size: 14px;
  font-weight: 500;
  color: var(--el-text-color-primary);
  margin-bottom: 2px;

  :deep(mark) {
    background: var(--el-color-warning);
    color: inherit;
    padding: 0 2px;
    border-radius: 2px;
  }
}

.command-desc {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.command-shortcut {
  display: flex;
  gap: 4px;

  kbd {
    padding: 2px 6px;
    background: var(--el-fill-color);
    border: 1px solid var(--el-border-color);
    border-radius: 4px;
    font-family: monospace;
    font-size: 11px;
    color: var(--el-text-color-secondary);
  }
}

.palette-footer {
  padding: 12px 20px;
  border-top: 1px solid var(--el-border-color-lighter);
  background: var(--el-fill-color-light);
}

.footer-info {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 13px;
  color: var(--el-text-color-secondary);
}

.fade-enter-active, .fade-leave-active {
  transition: opacity 0.2s;
}

.fade-enter-from, .fade-leave-to {
  opacity: 0;
}

.fade-enter-active .command-palette {
  animation: slideDown 0.2s ease-out;
}

@keyframes slideDown {
  from {
    opacity: 0;
    transform: translateY(-20px) scale(0.95);
  }
  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}
</style>
