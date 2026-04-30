/**
 * LaTeX宏管理器 (深度优化版)
 * 管理自定义命令、环境、宏包
 * 全新UI设计，现代化交互体验
 */

<template>
  <div class="latex-macro-manager">
    <!-- 头部工具栏 -->
    <div class="macro-header">
      <div class="header-title">
        <el-icon :size="20" color="var(--el-color-primary)"><Operation /></el-icon>
        <span>LaTeX 宏管理器</span>
      </div>

      <div class="header-actions">
        <el-input
          v-model="searchQuery"
          placeholder="搜索..."
          :prefix-icon="Search"
          clearable
          class="search-input"
        />
        <el-button v-if="activeTab === 'commands'" :icon="Plus" type="primary" @click="handleCreateNew">
          新建
        </el-button>
        <el-button v-else :icon="Plus" type="primary" @click="handleCreateNew">
          新建
        </el-button>
      </div>
    </div>

    <!-- 标签栏 -->
    <div class="tab-bar-wrapper">
      <div class="tab-bar">
        <button
          v-for="tab in tabs"
          :key="tab.key"
          :class="['tab-btn', { active: activeTab === tab.key }]"
          @click="activeTab = tab.key"
        >
          <el-icon><component :is="tab.icon" /></el-icon>
          <span>{{ tab.label }}</span>
          <el-badge v-if="tab.count > 0" :value="tab.count" :max="99" />
        </button>
      </div>
    </div>

    <!-- 内容区域 -->
    <div class="macro-content">
    <!-- 自定义命令内容 -->
    <div v-show="activeTab === 'commands'" class="commands-panel">
        <!-- 分组标签栏 -->
        <div v-if="macroGroups.length > 0 || favoriteCount > 0 || pinnedCount > 0" class="group-tabs">
          <el-radio-group v-model="groupFilter" size="small">
            <el-radio-button label="">全部</el-radio-button>
            <el-radio-button v-if="pinnedCount > 0" label="pinned">
              <el-icon><Star /></el-icon>
              置顶 ({{ pinnedCount }})
            </el-radio-button>
            <el-radio-button v-if="favoriteCount > 0" label="favorites">
              <el-icon><CollectionTag /></el-icon>
              收藏 ({{ favoriteCount }})
            </el-radio-button>
            <el-radio-button v-for="group in macroGroups" :key="group" :label="group">
              {{ group }}
            </el-radio-button>
          </el-radio-group>
        </div>

        <div v-if="filteredCommands.length > 0" class="macro-grid">
          <div
            v-for="cmd in filteredCommands"
            :key="cmd.id"
            class="macro-card"
            :class="{ 'is-favorite': cmd.isFavorite, 'is-pinned': cmd.isPinned }"
            @click="editMacro(cmd)"
          >
            <!-- 置顶标记 -->
            <div v-if="cmd.isPinned" class="pinned-badge">
              <el-icon><Star /></el-icon>
            </div>

            <!-- 卡片头部 -->
            <div class="card-header">
              <div class="macro-name">
                <code>\{{ cmd.name }}</code>
              </div>
              <div class="card-actions">
                <el-button
                  size="small"
                  type="primary"
                  @click.stop="insertMacro(cmd)"
                >
                  <el-icon><Plus /></el-icon>
                  插入
                </el-button>
                <el-button
                  :icon="cmd.isFavorite ? Star : CollectionTag"
                  size="small"
                  circle
                  @click.stop="toggleFavorite(cmd, $event)"
                  :class="{ 'is-active': cmd.isFavorite }"
                />
                <el-button
                  :icon="Delete"
                  size="small"
                  circle
                  type="danger"
                  @click.stop="deleteMacro(cmd)"
                />
              </div>
            </div>

            <!-- 卡片主体 -->
            <div class="card-body">
              <div v-if="cmd.description" class="card-description">
                {{ cmd.description }}
              </div>

              <div class="card-meta">
                <span v-if="cmd.params && cmd.params.length > 0" class="meta-item">
                  <el-icon><Tickets /></el-icon>
                  {{ cmd.params.length }}参数
                </span>
                <span v-if="cmd.usageCount" class="meta-item usage">
                  <el-icon><DataAnalysis /></el-icon>
                  {{ cmd.usageCount }}次
                </span>
                <span v-if="cmd.category" class="meta-item">
                  <el-tag size="small" type="info">{{ cmd.category }}</el-tag>
                </span>
              </div>

              <div v-if="cmd.usage" class="card-usage">
                <code>{{ cmd.usage }}</code>
              </div>
            </div>
          </div>
        </div>
        <div v-else class="empty-state">
          <div class="empty-illustration">
            <el-icon :size="60" color="var(--el-text-color-placeholder)">
              <EditPen />
            </el-icon>
          </div>
          <div class="empty-title">暂无自定义命令</div>
          <div class="empty-desc">创建自己的LaTeX命令提高写作效率</div>
          <el-button type="primary" @click="handleCreateNew">
            <el-icon><Plus /></el-icon>
            创建第一个命令
          </el-button>
        </div>
      </div>

      <!-- 环境列表 -->
      <div v-show="activeTab === 'environments'" class="environments-panel">
        <div v-if="filteredEnvironments.length > 0" class="macro-grid">
          <div
            v-for="env in filteredEnvironments"
            :key="env.id"
            class="macro-card env-card"
            @click="editEnvironment(env)"
          >
            <div class="card-badge env-badge">
              <el-icon><DocumentCopy /></el-icon>
            </div>
            <div class="card-header">
              <div class="macro-name">
                <code>\begin{{ env.name }}</code>
              </div>
              <el-tag size="small" type="success" effect="plain">环境</el-tag>
            </div>
            <div class="card-body">
              <div class="env-preview">
                <div class="env-line begin">\begin{{ env.name }}</div>
                <div class="env-content">...</div>
                <div class="env-line end">\end{{ env.name }}</div>
              </div>
              <div v-if="env.description" class="macro-description">
                {{ env.description }}
              </div>
            </div>
            <div class="card-footer" @click.stop>
              <el-button-group size="small">
                <el-tooltip content="插入到编辑器" placement="top">
                  <el-button @click="insertEnvironment(env)">
                    <el-icon><Plus /></el-icon>
                    插入
                  </el-button>
                </el-tooltip>
                <el-tooltip content="删除" placement="top">
                  <el-button type="danger" @click="deleteEnvironment(env)">
                    <el-icon><Delete /></el-icon>
                  </el-button>
                </el-tooltip>
              </el-button-group>
            </div>
          </div>
        </div>
        <div v-else class="empty-state">
          <div class="empty-illustration">
            <el-icon :size="60" color="var(--el-text-color-placeholder)">
              <DocumentCopy />
            </el-icon>
          </div>
          <div class="empty-title">暂无自定义环境</div>
          <div class="empty-desc">创建可重用的LaTeX环境</div>
          <el-button type="primary" @click="handleCreateNew">
            <el-icon><Plus /></el-icon>
            创建环境
          </el-button>
        </div>
      </div>

      <!-- 宏包列表 -->
      <div v-show="activeTab === 'packages'" class="packages-panel">
        <div class="package-grid">
          <div
            v-for="pkg in packages"
            :key="pkg.id"
            class="package-card"
            :class="{ 'is-enabled': pkg.enabled }"
          >
            <div class="package-header">
              <div class="package-icon" :class="{ 'enabled': pkg.enabled }">
                {{ pkg.icon }}
              </div>
              <el-switch
                v-model="pkg.enabled"
                @change="togglePackage(pkg)"
                size="large"
              />
            </div>
            <div class="package-info">
              <div class="package-name">{{ pkg.name }}</div>
              <div class="package-version">v{{ pkg.version }}</div>
              <div class="package-description">{{ pkg.description }}</div>
            </div>
            <div class="package-status">
              <el-tag v-if="pkg.enabled" type="success" effect="plain" size="small">
                <el-icon><Check /></el-icon>
                已启用
              </el-tag>
              <el-tag v-else type="info" effect="plain" size="small">
                未启用
              </el-tag>
            </div>
          </div>
        </div>
      </div>

      <!-- 模板库 -->
      <div v-show="activeTab === 'templates'" class="templates-panel">
        <div class="template-categories">
          <el-radio-group v-model="templateCategory" size="small">
            <el-radio-button label="all">全部</el-radio-button>
            <el-radio-button label="math">数学公式</el-radio-button>
            <el-radio-button label="text">文本格式</el-radio-button>
            <el-radio-button label="table">表格</el-radio-button>
            <el-radio-button label="graphic">图形</el-radio-button>
          </el-radio-group>
        </div>
        <div class="template-grid">
          <div
            v-for="tpl in filteredTemplates"
            :key="tpl.id"
            class="template-card"
            @click="useTemplate(tpl)"
          >
            <div class="template-preview" v-html="tpl.preview"></div>
            <div class="template-info">
              <div class="template-name">
                <code>{{ tpl.name }}</code>
              </div>
              <div class="template-description">{{ tpl.description }}</div>
            </div>
            <div class="template-actions">
              <el-button size="small" type="primary" @click.stop="useTemplate(tpl)">
                <el-icon><Plus /></el-icon>
                使用模板
              </el-button>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 编辑对话框 -->
    <el-dialog
      v-model="showEditDialog"
      :title="editingItem ? '编辑宏' : '新建宏'"
      width="700px"
      class="macro-edit-dialog"
      @close="resetForm"
    >
      <el-form ref="formRef" :model="formData" label-width="100px">
        <el-form-item label="宏名称" prop="name">
          <el-input
            v-model="formData.name"
            placeholder="例如: mycommand"
            size="large"
          >
            <template #prepend>\</template>
          </el-input>
        </el-form-item>

        <el-form-item label="描述">
          <el-input
            v-model="formData.description"
            type="textarea"
            :rows="2"
            placeholder="简要描述这个宏的用途..."
            maxlength="200"
            show-word-limit
          />
        </el-form-item>

        <el-form-item label="定义" prop="definition">
          <el-input
            v-model="formData.definition"
            type="textarea"
            :rows="6"
            placeholder="例如: \newcommand{\mycommand}[2]{#1 and #2}"
            class="code-editor"
          />
        </el-form-item>

        <el-form-item v-if="activeTab === 'commands'" label="用法示例">
          <el-input
            v-model="formData.usage"
            placeholder="\mycommand{arg1}{arg2}"
            size="large"
          >
            <template #prepend>\</template>
          </el-input>
        </el-form-item>

        <!-- 参数设置 -->
        <el-form-item label="参数">
          <div class="params-section">
            <div class="params-list">
              <div
                v-for="(param, index) in formData.params"
                :key="index"
                class="param-item"
              >
                <span class="param-number">{{ index + 1 }}</span>
                <el-input
                  v-model="param.name"
                  placeholder="参数名"
                  size="small"
                />
                <el-input
                  v-model="param.defaultValue"
                  placeholder="默认值"
                  size="small"
                />
                <el-select
                  v-model="param.type"
                  size="small"
                >
                  <el-option label="文本" value="text" />
                  <el-option label="数字" value="number" />
                  <el-option label="可选" value="optional" />
                </el-select>
                <el-button
                  size="small"
                  text
                  type="danger"
                  @click="removeParam(index)"
                >
                  <el-icon><Delete /></el-icon>
                </el-button>
              </div>
            </div>
            <el-button size="small" @click="addParam">
              <el-icon><Plus /></el-icon>
              添加参数
            </el-button>
          </div>
        </el-form-item>
      </el-form>

      <template #footer>
        <div class="dialog-footer">
          <el-button size="large" @click="showEditDialog = false">取消</el-button>
          <el-button size="large" type="primary" @click="saveMacro" :loading="saving">
            <el-icon><Check /></el-icon>
            保存
          </el-button>
        </div>
      </template>
    </el-dialog>

    <!-- 快速预览对话框 -->
    <el-dialog
      v-model="showQuickPreview"
      :title="`\\${previewingMacro?.name}`"
      width="700px"
      class="quick-preview-dialog"
    >
      <div v-if="previewingMacro" class="preview-content">
        <!-- 基本信息 -->
        <div class="preview-section">
          <div class="preview-label">描述</div>
          <div class="preview-value">{{ previewingMacro.description || '暂无描述' }}</div>
        </div>

        <!-- 用法示例 -->
        <div class="preview-section">
          <div class="preview-label">用法示例</div>
          <div class="code-block">
            <code>{{ previewingMacro.usage || `\\${previewingMacro.name}{...}` }}</code>
            <el-button
              size="small"
              text
              @click="navigator.clipboard.writeText(previewingMacro.usage || `\\${previewingMacro.name}`)"
            >
              <el-icon><DocumentCopy /></el-icon>
              复制
            </el-button>
          </div>
        </div>

        <!-- 完整定义 -->
        <div class="preview-section">
          <div class="preview-label">完整定义</div>
          <div class="code-block full">
            <pre>{{ previewingMacro.definition }}</pre>
            <el-button
              size="small"
              text
              @click="navigator.clipboard.writeText(previewingMacro.definition)"
            >
              <el-icon><DocumentCopy /></el-icon>
              复制
            </el-button>
          </div>
        </div>

        <!-- 参数信息 -->
        <div v-if="previewingMacro.params && previewingMacro.params.length > 0" class="preview-section">
          <div class="preview-label">参数列表</div>
          <div class="params-list">
            <div v-for="(param, idx) in previewingMacro.params" :key="idx" class="param-item">
              <span class="param-index">{{ idx + 1 }}</span>
              <span class="param-name">{{ param.name || `参数${idx + 1}` }}</span>
              <el-tag size="small" type="info">{{ param.type }}</el-tag>
              <span v-if="param.defaultValue" class="param-default">默认: {{ param.defaultValue }}</span>
            </div>
          </div>
        </div>

        <!-- 使用统计 -->
        <div class="preview-section stats">
          <div class="stat-item">
            <el-icon><DataAnalysis /></el-icon>
            <span>使用 {{ previewingMacro.usageCount || 0 }} 次</span>
          </div>
          <div v-if="previewingMacro.lastUsed" class="stat-item">
            <el-icon><Clock /></el-icon>
            <span>上次使用: {{ formatTimestamp(previewingMacro.lastUsed) }}</span>
          </div>
        </div>
      </div>

      <template #footer>
        <div class="dialog-footer">
          <el-button @click="showQuickPreview = false">关闭</el-button>
          <el-button type="primary" @click="insertMacro(previewingMacro!)">
            <el-icon><Plus /></el-icon>
            插入到编辑器
          </el-button>
          <el-button @click="editMacro(previewingMacro!); showQuickPreview = false">
            <el-icon><EditPen /></el-icon>
            编辑
          </el-button>
        </div>
      </template>
    </el-dialog>

    <!-- 模板库对话框 -->
    <el-dialog v-model="showTemplateLibrary" title="宏模板库" width="900px" class="template-library-dialog">
      <div class="template-library-content">
        <el-alert type="info" :closable="false" show-icon class="template-hint">
          点击模板即可直接使用，或基于模板创建自定义宏
        </el-alert>
        <div class="template-categories-filter">
          <el-radio-group v-model="templateCategory" size="small">
            <el-radio-button label="all">
              <el-icon><Grid /></el-icon>
              全部
            </el-radio-button>
            <el-radio-button label="math">
              <el-icon><Histogram /></el-icon>
              数学公式
            </el-radio-button>
            <el-radio-button label="text">
              <el-icon><Document /></el-icon>
              文本格式
            </el-radio-button>
            <el-radio-button label="table">
              <el-icon><Tickets /></el-icon>
              表格
            </el-radio-button>
            <el-radio-button label="graphic">
              <el-icon><Picture /></el-icon>
              图形
            </el-radio-button>
          </el-radio-group>
        </div>
        <div class="template-library-grid">
          <div
            v-for="tpl in filteredTemplates"
            :key="tpl.id"
            class="library-template-card"
            @click="useTemplate(tpl)"
          >
            <div class="template-card-preview" v-html="tpl.preview"></div>
            <div class="template-card-info">
              <code class="template-card-name">{{ tpl.name }}</code>
              <p class="template-card-desc">{{ tpl.description }}</p>
              <div class="template-card-code">
                <pre>{{ tpl.code }}</pre>
              </div>
            </div>
            <div class="template-card-actions">
              <el-button size="small" type="primary" @click.stop="useTemplate(tpl)">
                <el-icon><Plus /></el-icon>
                使用此模板
              </el-button>
            </div>
          </div>
        </div>
      </div>
    </el-dialog>

    <!-- 导入对话框 -->
    <el-dialog v-model="showImportDialog" title="导入宏" width="500px">
      <div class="import-content">
        <el-alert type="info" :closable="false" show-icon>
          支持 .tex 和 .txt 格式，将自动解析 \newcommand 和 \newenvironment 命令
        </el-alert>
        <div class="import-area">
          <el-upload
            drag
            accept=".tex,.txt"
            :auto-upload="false"
            :on-change="handleFileSelect"
            :show-file-list="false"
          >
            <el-icon :size="48"><UploadFilled /></el-icon>
            <div class="upload-text">拖拽文件到此处或点击上传</div>
            <div class="upload-hint">支持 .tex 和 .txt 格式</div>
          </el-upload>
        </div>
      </div>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import {
  Plus,
  Download,
  Upload,
  UploadFilled,
  Delete,
  DocumentCopy,
  EditPen,
  Search,
  Check,
  Operation,
  Box,
  FolderAdd as FolderAddIcon,
  MagicStick as MagicStickIcon,
  PriceTag as CollectionTag,
  DataAnalysis,
  Star,
  Clock,
  Grid,
  Histogram,
  Document,
  Tickets,
  Picture,
  Filter,
  ArrowDown
} from '@element-plus/icons-vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import type { FormInstance } from 'element-plus'

interface MacroCommand {
  id: string
  name: string
  type: 'command' | 'environment'
  definition: string
  description?: string
  usage?: string
  category?: string
  usageCount?: number
  lastUsed?: number
  isFavorite?: boolean
  isPinned?: boolean
  tags?: string[]
  group?: string
  params?: Array<{
    name: string
    defaultValue: string
    type: 'text' | 'number' | 'optional'
  }>
}

interface MacroEnvironment {
  id: string
  name: string
  begin: string
  end: string
  body: string
  description?: string
}

interface MacroPackage {
  id: string
  name: string
  version: string
  description: string
  enabled: boolean
  icon: string
  color: string
  options?: string[]
}

interface Props {
  documentId?: string
}

const props = defineProps<Props>()

// 状态
const activeTab = ref('commands')
const commands = ref<MacroCommand[]>([])
const environments = ref<MacroEnvironment[]>([])
const packages = ref<MacroPackage[]>([])
const searchQuery = ref('')
const categoryFilter = ref('')
const showEditDialog = ref(false)
const showImportDialog = ref(false)
const showTemplateLibrary = ref(false)
const showQuickPreview = ref(false)
const editingItem = ref<MacroCommand | MacroEnvironment | null>(null)
const previewingMacro = ref<MacroCommand | null>(null)
const saving = ref(false)
const formRef = ref<FormInstance>()
const templateCategory = ref('all')
const groupFilter = ref('')

// 表单数据
const formData = ref({
  name: '',
  description: '',
  definition: '',
  usage: '',
  params: [] as Array<{ name: string; defaultValue: string; type: string }>
})

// 计算属性
const enabledPackageCount = computed(() => packages.value.filter(p => p.enabled).length)

const tabs = computed(() => [
  { key: 'commands', label: '命令', icon: EditPen, count: commands.value.length },
  { key: 'environments', label: '环境', icon: DocumentCopy, count: environments.value.length },
  { key: 'packages', label: '宏包', icon: Box, count: enabledPackageCount.value },
  { key: 'templates', label: '模板', icon: CollectionTag, count: 0 }
])

const filteredCommands = computed(() => {
  let result = commands.value

  // 搜索过滤
  if (searchQuery.value) {
    const query = searchQuery.value.toLowerCase()
    result = result.filter(c =>
      c.name.toLowerCase().includes(query) ||
      c.description?.toLowerCase().includes(query) ||
      c.tags?.some(t => t.toLowerCase().includes(query))
    )
  }

  // 分类过滤
  if (categoryFilter.value) {
    result = result.filter(c => c.category === categoryFilter.value)
  }

  // 分组过滤
  if (groupFilter.value) {
    if (groupFilter.value === 'favorites') {
      result = result.filter(c => c.isFavorite)
    } else if (groupFilter.value === 'pinned') {
      result = result.filter(c => c.isPinned)
    } else {
      result = result.filter(c => c.group === groupFilter.value)
    }
  }

  // 排序：置顶 > 收藏 > 使用次数
  result.sort((a, b) => {
    if (a.isPinned && !b.isPinned) return -1
    if (!a.isPinned && b.isPinned) return 1
    if (a.isFavorite && !b.isFavorite) return -1
    if (!a.isFavorite && b.isFavorite) return 1
    return (b.usageCount || 0) - (a.usageCount || 0)
  })

  return result
})

// 获取所有分组
const macroGroups = computed(() => {
  const groups = new Set<string>()
  commands.value.forEach(c => {
    if (c.group) groups.add(c.group)
  })
  return Array.from(groups)
})

// 收藏的宏数量
const favoriteCount = computed(() =>
  commands.value.filter(c => c.isFavorite).length
)

// 置顶的宏数量
const pinnedCount = computed(() =>
  commands.value.filter(c => c.isPinned).length
)

const filteredEnvironments = computed(() => {
  if (!searchQuery.value) return environments.value
  const query = searchQuery.value.toLowerCase()
  return environments.value.filter(e =>
    e.name.toLowerCase().includes(query) ||
    e.description?.toLowerCase().includes(query)
  )
})

const totalUsageCount = computed(() =>
  commands.value.reduce((sum, c) => sum + (c.usageCount || 0), 0)
)

const mostUsedMacro = computed(() => {
  if (commands.value.length === 0) return null
  return commands.value.reduce((max, cmd) =>
    (cmd.usageCount || 0) > (max.usageCount || 0) ? cmd : max
  )
})

const lastUpdateText = computed(() => {
  const allMacros = [...commands.value, ...environments.value]
  if (allMacros.length === 0) return '-'

  const timestamps = allMacros
    .map(m => m.id)
    .map(id => parseInt(id.split('-')[1]) || 0)

  const latest = Math.max(...timestamps)
  const diff = Date.now() - latest

  if (diff < 60000) return '刚刚'
  if (diff < 3600000) return `${Math.floor(diff / 60000)} 分钟前`
  if (diff < 86400000) return `${Math.floor(diff / 3600000)} 小时前`
  return `${Math.floor(diff / 86400000)} 天前`
})

// 模板数据
const templates = ref([
  {
    id: 'tpl-frac',
    name: '\\frac',
    category: 'math',
    description: '分数',
    preview: '<span style="font-size: 24px; display: flex; align-items: center; justify-content: center;"><sup style="font-size: 16px;">a</sup>&frasl;<sub style="font-size: 16px;">b</sub></span>',
    code: '\\frac{a}{b}',
    definition: '\\newcommand{\\frac}[2]{\\genfrac{}{}{}{#1}{#2}}'
  },
  {
    id: 'tpl-binom',
    name: '\\binom',
    category: 'math',
    description: '二项式系数',
    preview: '<span style="font-size: 24px; display: flex; align-items: center; justify-content: center;">(n k)</span>',
    code: '\\binom{n}{k}',
    definition: '\\newcommand{\\binom}[2]{\\genfrac(){}{}{0}{#1}{#2}}'
  },
  {
    id: 'tpl-textbf',
    name: '\\textbf',
    category: 'text',
    description: '粗体文本',
    preview: '<span style="font-size: 24px; font-weight: bold; display: flex; align-items: center; justify-content: center;">Bold</span>',
    code: '\\textbf{text}',
    definition: '\\newcommand{\\textbf}[1]{\\textbf{#1}}'
  },
  {
    id: 'tpl-textit',
    name: '\\textit',
    category: 'text',
    description: '斜体文本',
    preview: '<span style="font-size: 24px; font-style: italic; display: flex; align-items: center; justify-content: center;">Italic</span>',
    code: '\\textit{text}',
    definition: '\\newcommand{\\textit}[1]{\\textit{#1}}'
  },
  {
    id: 'tpl-title',
    name: '标题环境',
    category: 'graphic',
    description: '居中标题',
    preview: '<div style="display: flex; align-items: center; justify-content: center; font-size: 16px; font-weight: bold; padding: 20px; border: 1px solid #ccc;">标题</div>',
    code: '\\begin{center}\\Huge 标题\\end{center}',
    definition: '',
    isEnvironment: true
  },
  {
    id: 'tpl-theorem',
    name: '定理环境',
    category: 'text',
    description: '定理环境',
    preview: '<div style="display: flex; align-items: center; justify-content: center; font-size: 14px; font-style: italic; padding: 15px;">定理: 陈述</div>',
    code: '\\begin{theorem}[可选标题]\\n陈述\\n\\end{theorem}',
    definition: '',
    isEnvironment: true
  }
])

const filteredTemplates = computed(() => {
  if (templateCategory.value === 'all') return templates.value
  return templates.value.filter(t => t.category === templateCategory.value)
})

// 内置宏包
const builtInPackages: MacroPackage[] = [
  {
    id: 'amsmath',
    name: 'amsmath',
    version: '2.1',
    description: 'American Mathematical Society数学公式',
    enabled: true,
    icon: '∑',
    color: '#409eff'
  },
  {
    id: 'amssymb',
    name: 'amssymb',
    version: '2.1',
    description: 'AMS数学符号',
    enabled: true,
    icon: '∫',
    color: '#67c23a'
  },
  {
    id: 'graphicx',
    name: 'graphicx',
    version: '2.0',
    description: '图形插入',
    enabled: false,
    icon: '🖼️',
    color: '#e6a23c'
  },
  {
    id: 'hyperref',
    name: 'hyperref',
    version: '3.0',
    description: '超链接和交叉引用',
    enabled: false,
    icon: '🔗',
    color: '#909399'
  },
  {
    id: 'geometry',
    name: 'geometry',
    version: '0.12',
    description: '页面布局',
    enabled: false,
    icon: '📄',
    color: '#f56c6c'
  },
  {
    id: 'xcolor',
    name: 'xcolor',
    version: '2.12',
    description: '颜色支持',
    enabled: false,
    icon: '🎨',
    color: '#a855f7'
  },
  {
    id: 'listings',
    name: 'listings',
    version: '1.9',
    description: '代码抄录',
    enabled: false,
    icon: '💻',
    color: '#3b82f6'
  },
  {
    id: 'tikz',
    name: 'tikz',
    version: '3.1',
    description: '绘图工具',
    enabled: false,
    icon: '✏️',
    color: '#ec4899'
  }
]

// 工具函数
const truncateCode = (code: string, maxLength: number): string => {
  if (code.length <= maxLength) return code
  return code.substring(0, maxLength) + '...'
}

const formatTimestamp = (timestamp: number): string => {
  const diff = Date.now() - timestamp
  if (diff < 60000) return '刚刚'
  if (diff < 3600000) return `${Math.floor(diff / 60000)} 分钟前`
  if (diff < 86400000) return `${Math.floor(diff / 3600000)} 小时前`
  if (diff < 604800000) return `${Math.floor(diff / 86400000)} 天前`
  return new Date(timestamp).toLocaleDateString()
}

// 初始化
onMounted(() => {
  loadMacros()
  loadPackages()
})

// 加载自定义宏
const loadMacros = () => {
  const saved = localStorage.getItem('latex_macros')
  if (saved) {
    try {
      const data = JSON.parse(saved)
      commands.value = data.commands || []
      environments.value = data.environments || []
    } catch (error) {
      console.error('Failed to load macros:', error)
    }
  }
}

// 保存自定义宏
const saveMacros = () => {
  const data = {
    commands: commands.value.map(c => ({
      ...c,
      isFavorite: c.isFavorite || false,
      isPinned: c.isPinned || false,
      tags: c.tags || [],
      group: c.group || ''
    })),
    environments: environments.value
  }
  localStorage.setItem('latex_macros', JSON.stringify(data))
}

// 加载宏包状态
const loadPackages = () => {
  const saved = localStorage.getItem('latex_packages')
  if (saved) {
    try {
      const enabled = new Set(JSON.parse(saved))
      packages.value = builtInPackages.map(pkg => ({
        ...pkg,
        enabled: enabled.has(pkg.id)
      }))
    } catch (error) {
      console.error('Failed to load packages:', error)
      packages.value = [...builtInPackages]
    }
  } else {
    packages.value = [...builtInPackages]
  }
}

// 切换宏包
const togglePackage = (pkg: MacroPackage) => {
  savePackages()
  ElMessage.success(`${pkg.name} 已${pkg.enabled ? '启用' : '禁用'}`)
}

// 保存宏包状态
const savePackages = () => {
  const enabled = packages.value.filter(p => p.enabled).map(p => p.id)
  localStorage.setItem('latex_packages', JSON.stringify(enabled))
}

// 处理标签切换
const handleTabChange = () => {
  editingItem.value = null
  searchQuery.value = ''
}

// 新建
const handleCreateNew = () => {
  editingItem.value = null
  formData.value = {
    name: '',
    description: '',
    definition: '',
    usage: '',
    params: []
  }

  if (activeTab.value === 'commands') {
    formData.value.definition = '\\newcommand{\\commandname}[2]{definition}'
    formData.value.usage = '\\commandname{arg1}{arg2}'
  } else if (activeTab.value === 'environments') {
    formData.value.definition = '\\begin{environmentname}\n  ...\n\\end{environmentname}'
  }

  showEditDialog.value = true
}

// 编辑命令
const editMacro = (cmd: MacroCommand) => {
  editingItem.value = cmd
  formData.value = {
    name: cmd.name,
    description: cmd.description || '',
    definition: cmd.definition,
    usage: cmd.usage || '',
    params: cmd.params || []
  }
  showEditDialog.value = true
}

// 编辑环境
const editEnvironment = (env: MacroEnvironment) => {
  editingItem.value = env as any
  formData.value = {
    name: env.name,
    description: env.description || '',
    definition: `\\begin{${env.name}}\n${env.body}\n\\end{${env.name}}`,
    usage: '',
    params: []
  }
  showEditDialog.value = true
}

// 重置表单
const resetForm = () => {
  formData.value = {
    name: '',
    description: '',
    definition: '',
    usage: '',
    params: []
  }
}

// 保存
const saveMacro = () => {
  if (!formData.value.name) {
    ElMessage.warning('请输入宏名称')
    return
  }

  saving.value = true

  setTimeout(() => {
    if (activeTab.value === 'commands') {
      const newMacro: MacroCommand = {
        id: editingItem.value?.id || `cmd-${Date.now()}`,
        name: formData.value.name,
        type: 'command',
        definition: formData.value.definition,
        description: formData.value.description,
        usage: formData.value.usage,
        params: formData.value.params
      }

      if (editingItem.value) {
        const index = commands.value.findIndex(c => c.id === editingItem.value!.id)
        if (index >= 0) {
          commands.value[index] = newMacro
        }
      } else {
        commands.value.push(newMacro)
      }
    } else if (activeTab.value === 'environments') {
      const beginMatch = formData.value.definition.match(/\\begin\{([^}]+)\}/)
      const bodyMatch = formData.value.definition.match(/\\begin\{[^}]+\}([\s\S]+?)\\end\{[^}]+\}/)

      if (beginMatch && bodyMatch) {
        const newEnv: MacroEnvironment = {
          id: editingItem.value?.id || `env-${Date.now()}`,
          name: beginMatch[1],
          begin: beginMatch[0],
          end: `\\end{${beginMatch[1]}}`,
          body: bodyMatch[1].trim(),
          description: formData.value.description
        }

        if (editingItem.value) {
          const index = environments.value.findIndex(e => e.id === editingItem.value!.id)
          if (index >= 0) {
            environments.value[index] = newEnv
          }
        } else {
          environments.value.push(newEnv)
        }
      }
    }

    saveMacros()
    showEditDialog.value = false
    saving.value = false

    ElMessage.success(editingItem.value ? '更新成功' : '创建成功')
  }, 500)
}

// 删除
const deleteMacro = async (cmd: MacroCommand) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除命令 \\${cmd.name} 吗？`,
      '确认删除',
      {
        type: 'warning',
        confirmButtonText: '删除',
        cancelButtonText: '取消'
      }
    )
    commands.value = commands.value.filter(c => c.id !== cmd.id)
    saveMacros()
    ElMessage.success('删除成功')
  } catch {
    // 用户取消
  }
}

const deleteEnvironment = async (env: MacroEnvironment) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除环境 \\begin{${env.name}} 吗？`,
      '确认删除',
      {
        type: 'warning',
        confirmButtonText: '删除',
        cancelButtonText: '取消'
      }
    )
    environments.value = environments.value.filter(e => e.id !== env.id)
    saveMacros()
    ElMessage.success('删除成功')
  } catch {
    // 用户取消
  }
}

// 复制
const duplicateMacro = (cmd: MacroCommand) => {
  const newMacro: MacroCommand = {
    ...cmd,
    id: `cmd-${Date.now()}`,
    name: `${cmd.name}_copy`
  }
  commands.value.push(newMacro)
  saveMacros()
  ElMessage.success('复制成功')
}

// 切换收藏
const toggleFavorite = (cmd: MacroCommand, event: Event) => {
  event.stopPropagation()
  cmd.isFavorite = !cmd.isFavorite
  saveMacros()
  ElMessage.success(cmd.isFavorite ? '已收藏' : '已取消收藏')
}

// 切换置顶
const togglePin = (cmd: MacroCommand, event: Event) => {
  event.stopPropagation()
  cmd.isPinned = !cmd.isPinned
  saveMacros()
  ElMessage.success(cmd.isPinned ? '已置顶' : '已取消置顶')
}

// 快速预览
const showPreview = (cmd: MacroCommand, event: Event) => {
  event.stopPropagation()
  previewingMacro.value = cmd
  showQuickPreview.value = true
}

// 插入
const insertMacro = (cmd: MacroCommand) => {
  // 增加使用计数
  cmd.usageCount = (cmd.usageCount || 0) + 1
  cmd.lastUsed = Date.now()
  saveMacros()

  // TODO: 插入到编辑器
  ElMessage.success(`已插入命令: \\${cmd.name}`)
}

const insertEnvironment = (env: MacroEnvironment) => {
  // TODO: 插入到编辑器
  ElMessage.success(`已插入环境: \\begin{${env.name}}...\\end{${env.name}}`)
}

// 参数管理
const addParam = () => {
  formData.value.params.push({
    name: '',
    defaultValue: '',
    type: 'text'
  })
}

const removeParam = (index: number) => {
  formData.value.params.splice(index, 1)
}

// 使用模板
const useTemplate = (tpl: any) => {
  if (tpl.isEnvironment) {
    // 环境模板
    const envMatch = tpl.code.match(/\\begin\{([^}]+)\}/)
    if (envMatch) {
      const newEnv: MacroEnvironment = {
        id: `env-${Date.now()}`,
        name: envMatch[1],
        begin: envMatch[0],
        end: `\\end{${envMatch[1]}}`,
        body: tpl.code.match(/\\begin\{[^}]+\}([\s\S]+?)\\end\{[^}]+\}/)?.[1] || '',
        description: tpl.description
      }
      environments.value.push(newEnv)
      saveMacros()
      ElMessage.success(`已添加环境模板: ${tpl.name}`)
    }
  } else {
    // 命令模板
    const newCmd: MacroCommand = {
      id: `cmd-${Date.now()}`,
      name: tpl.name.replace(/\\/g, ''),
      type: 'command',
      definition: tpl.definition,
      description: tpl.description,
      usage: tpl.code,
      category: tpl.category
    }
    commands.value.push(newCmd)
    saveMacros()
    ElMessage.success(`已添加命令模板: ${tpl.name}`)
  }

  // 增加使用计数
  const existingIndex = commands.value.findIndex(c => c.name === tpl.name.replace(/\\/g, ''))
  if (existingIndex >= 0) {
    commands.value[existingIndex].usageCount = (commands.value[existingIndex].usageCount || 0) + 1
  }
}

// 导出
const handleExport = () => {
  let content = ''

  const enabledPkgs = packages.value.filter(p => p.enabled)
  if (enabledPkgs.length > 0) {
    content += '% ========================================\n'
    content += '% 宏包\n'
    content += '% ========================================\n\n'
    enabledPkgs.forEach(pkg => {
      content += `\\usepackage{${pkg.name}}\n`
    })
    content += '\n'
  }

  if (commands.value.length > 0) {
    content += '% ========================================\n'
    content += '% 自定义命令\n'
    content += '% ========================================\n\n'
    commands.value.forEach(cmd => {
      content += `${cmd.definition}\n`
      if (cmd.description) {
        content += `% ${cmd.description}\n`
      }
      content += '\n'
    })
  }

  if (environments.value.length > 0) {
    content += '% ========================================\n'
    content += '% 自定义环境\n'
    content += '% ========================================\n\n'
    environments.value.forEach(env => {
      content += `${env.begin}\n${env.body}\n${env.end}\n`
      if (env.description) {
        content += `% ${env.description}\n`
      }
      content += '\n'
    })
  }

  const blob = new Blob([content], { type: 'text/plain' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = 'macros.tex'
  link.click()
  URL.revokeObjectURL(url)

  ElMessage.success('导出成功')
}

// 导入
const handleImport = () => {
  showImportDialog.value = true
}

const handleFileSelect = (file: any) => {
  const reader = new FileReader()
  reader.onload = (e) => {
    const content = e.target?.result as string
    importMacros(content)
    showImportDialog.value = false
  }
  reader.readAsText(file.raw)
}

// 导入宏
const importMacros = (content: string) => {
  let importedCount = 0

  const newCmdRegex = /\\newcommand\{\\([^}]+)\}(?:\[(\d+)\])?\{([^}]+)\}/g
  let match

  while ((match = newCmdRegex.exec(content)) !== null) {
    const [, name, args, definition] = match
    const cmd: MacroCommand = {
      id: `cmd-${Date.now()}-${Math.random()}`,
      name,
      type: 'command',
      definition: match[0],
      usage: `\\${name}${args ? `{${args}}` : ''}{...}`
    }
    commands.value.push(cmd)
    importedCount++
  }

  const newEnvRegex = /\\newenvironment\{([^}]+)\}\{([^}]*)\}\{([^}]*)\}/g
  while ((match = newEnvRegex.exec(content)) !== null) {
    const [, name, begin, end] = match
    const env: MacroEnvironment = {
      id: `env-${Date.now()}-${Math.random()}`,
      name,
      begin: `\\begin{${name}}`,
      end: `\\end{${name}}`,
      body: begin,
      description: ''
    }
    environments.value.push(env)
    importedCount++
  }

  saveMacros()
  ElMessage.success(`成功导入 ${importedCount} 个宏`)
}
</script>

<style scoped lang="scss">
.latex-macro-manager {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: var(--el-bg-color-page);
}

// 头部
.macro-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 24px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color-light);
}

.header-title {
  display: flex;
  align-items: center;
  gap: 10px;
  font-size: 17px;
  font-weight: 600;
  color: var(--el-text-color-primary);
}

.header-actions {
  display: flex;
  gap: 12px;
  align-items: center;

  .search-input {
    width: 220px;
  }
}

// 标签栏
.tab-bar-wrapper {
  padding: 12px 24px 0;
  background: var(--el-bg-color);

  .tab-bar {
    display: flex;
    gap: 6px;

    .tab-btn {
      display: flex;
      align-items: center;
      gap: 6px;
      padding: 8px 16px;
      border: none;
      background: transparent;
      color: var(--el-text-color-secondary);
      font-size: 14px;
      font-weight: 500;
      cursor: pointer;
      border-radius: 8px;
      transition: all 0.2s;
      white-space: nowrap;
      flex-shrink: 0;

      &:hover {
        background: var(--el-fill-color-light);
        color: var(--el-text-color-primary);
      }

      &.active {
        background: var(--el-color-primary);
        color: white;
      }

      .el-icon {
        font-size: 16px;
      }
    }
  }
}

// 内容区域
.macro-content {
  flex: 1;
  overflow: hidden;
  display: flex;
  flex-direction: column;
}

// 分组标签栏
.group-tabs {
  padding: 16px 20px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color-lighter);

  :deep(.el-radio-group) {
    flex-wrap: wrap;
    gap: 10px;
  }

  :deep(.el-radio-button__inner) {
    border-radius: 18px;
    padding: 8px 18px;
    font-size: 14px;
    font-weight: 500;
  }
}

// 模板面板
.templates-panel {
  flex: 1;
  overflow-y: auto;
  padding: 20px;

  .template-categories {
    margin-bottom: 20px;
  }

  .template-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
    gap: 16px;

    .template-card {
      background: var(--el-bg-color);
      border-radius: 12px;
      border: 1px solid var(--el-border-color-lighter);
      padding: 16px;
      cursor: pointer;
      transition: all 0.2s;

      &:hover {
        border-color: var(--el-color-primary);
        box-shadow: 0 4px 12px rgba(64, 158, 255, 0.15);
      }

      .template-preview {
        height: 80px;
        display: flex;
        align-items: center;
        justify-content: center;
        background: var(--el-fill-color-light);
        border-radius: 8px;
        margin-bottom: 12px;
        padding: 12px;
      }

      .template-info {
        .template-name {
          code {
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 14px;
            color: var(--el-color-primary);
            font-weight: 600;
          }
        }

        .template-description {
          font-size: 12px;
          color: var(--el-text-color-secondary);
          margin: 8px 0;
        }
      }

      .template-actions {
        display: flex;
        justify-content: flex-end;
      }
    }
  }
}

.commands-panel,
.environments-panel {
  flex: 1;
  overflow-y: auto;
  background: var(--el-fill-color-extra-light);

  // 自定义滚动条
  &::-webkit-scrollbar {
    width: 8px;
  }

  &::-webkit-scrollbar-track {
    background: transparent;
  }

  &::-webkit-scrollbar-thumb {
    background: var(--el-border-color-darker);
    border-radius: 4px;

    &:hover {
      background: var(--el-border-color-dark);
    }
  }
}

.macro-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(700px, 1fr));
  gap: 28px;
  padding: 28px;
}

.macro-card {
  position: relative;
  background: var(--el-bg-color);
  border-radius: 16px;
  border: 1px solid var(--el-border-color-light);
  padding: 0;
  cursor: pointer;
  overflow: hidden;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  min-height: 280px;

  &:hover {
    border-color: var(--el-color-primary-light-3);
    box-shadow:
      0 8px 24px rgba(0, 0, 0, 0.12),
      0 4px 12px rgba(64, 158, 255, 0.15);
    transform: translateY(-4px);
  }

  &.is-pinned {
    border-color: var(--el-color-warning);
    border-width: 2px;
  }

  &.is-favorite {
    .card-actions .el-button:last-child {
      color: var(--el-color-warning);
      background: var(--el-color-warning-light-9);
    }
  }

  // 置顶标记
  .pinned-badge {
    position: absolute;
    top: 12px;
    left: 12px;
    padding: 6px 12px;
    background: var(--el-color-warning);
    color: white;
    font-size: 12px;
    font-weight: 600;
    border-radius: 16px;
    display: flex;
    align-items: center;
    gap: 6px;
    z-index: 2;
    box-shadow: 0 2px 8px rgba(230, 162, 60, 0.3);

    .el-icon {
      font-size: 14px;
    }
  }

  // 卡片头部
  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    gap: 16px;
    padding: 20px;

    .macro-name {
      flex: 1;
      min-width: 0;

      code {
        font-family: 'Consolas', 'Monaco', 'JetBrains Mono', monospace;
        font-weight: 700;
        font-size: 18px;
        color: var(--el-color-primary);
        background: var(--el-color-primary-light-9);
        padding: 10px 16px;
        border-radius: 10px;
        display: inline-block;
        letter-spacing: 0.3px;
      }
    }

    .card-actions {
      display: flex;
      align-items: center;
      gap: 8px;
      flex-shrink: 0;

      .el-button {
        padding: 8px;
        transition: all 0.2s;

        &:hover {
          transform: translateY(-1px);
        }

        &.is-active {
          color: var(--el-color-warning);
          background: var(--el-color-warning-light-9);
        }
      }
    }
  }

  // 卡片主体
  .card-body {
    padding: 0 20px 20px;
  }

  .card-description {
    font-size: 14px;
    color: var(--el-text-color-regular);
    line-height: 1.7;
    margin-bottom: 16px;
    min-height: 48px;
  }

  .card-meta {
    display: flex;
    align-items: center;
    gap: 16px;
    flex-wrap: wrap;
    margin-bottom: 14px;
    padding: 12px 16px;
    background: var(--el-fill-color-light);
    border-radius: 12px;

    .meta-item {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      font-size: 13px;
      color: var(--el-text-color-secondary);
      font-weight: 500;

      .el-icon {
        font-size: 14px;
      }

      &.usage {
        color: var(--el-color-success);
        font-weight: 600;
      }
    }
  }

  .card-usage {
    code {
      display: block;
      background: var(--el-fill-color);
      padding: 12px 16px;
      border-radius: 10px;
      font-family: 'Consolas', 'Monaco', 'JetBrains Mono', monospace;
      font-size: 13px;
      color: var(--el-text-color-primary);
      overflow-x: auto;
      white-space: nowrap;
      line-height: 1.6;
    }
  }
}

.env-card {
  .env-preview {
    background: linear-gradient(135deg, var(--el-fill-color-light) 0%, var(--el-fill-color) 100%);
    padding: 14px;
    border-radius: 10px;
    margin-bottom: 12px;
    font-family: 'Consolas', 'Monaco', 'JetBrains Mono', monospace;
    font-size: 12px;
    border: 1px solid var(--el-border-color-lighter);

    .env-line {
      padding: 4px 0;

      &.begin {
        color: var(--el-color-success);
        font-weight: 600;
      }

      &.end {
        color: var(--el-color-danger);
        font-weight: 600;
      }
    }

    .env-content {
      color: var(--el-text-color-secondary);
      padding: 6px 0;
      text-align: center;
      font-style: italic;
    }
  }
}

// 宏包面板
.packages-panel {
  flex: 1;
  overflow-y: auto;
  padding: 24px;
  background: var(--el-fill-color-extra-light);
}

.package-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 24px;
}

.package-card {
  background: var(--el-bg-color);
  border-radius: 16px;
  border: 1px solid var(--el-border-color-light);
  padding: 24px;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow:
    0 1px 3px rgba(0, 0, 0, 0.05),
    0 1px 2px rgba(0, 0, 0, 0.08);

  &:hover {
    border-color: var(--el-color-primary-light-3);
    box-shadow:
      0 10px 30px rgba(0, 0, 0, 0.1),
      0 4px 12px rgba(64, 158, 255, 0.15);
    transform: translateY(-4px);
  }

  &.is-enabled {
    border-color: var(--el-color-success);
    background: linear-gradient(135deg, rgba(103, 194, 58, 0.05) 0%, var(--el-bg-color) 100%);
    box-shadow:
      0 1px 3px rgba(103, 194, 58, 0.1),
      0 1px 2px rgba(103, 194, 58, 0.15);
  }

  .package-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
  }

  .package-icon {
    width: 64px;
    height: 64px;
    border-radius: 16px;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 32px;
    background: linear-gradient(135deg, var(--el-fill-color-light) 0%, var(--el-fill-color) 100%);
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);

    &.enabled {
      background: linear-gradient(135deg, var(--el-color-success) 0%, var(--el-color-success-light-3) 100%);
      color: white;
      box-shadow:
        0 4px 16px rgba(103, 194, 58, 0.4),
        0 2px 8px rgba(103, 194, 58, 0.3);
      transform: scale(1.05);
    }
  }

  .package-info {
    .package-name {
      font-weight: 700;
      font-size: 16px;
      color: var(--el-text-color-primary);
      margin-bottom: 6px;
      letter-spacing: 0.3px;
    }

    .package-version {
      font-size: 12px;
      color: var(--el-text-color-placeholder);
      margin-bottom: 10px;
      font-weight: 500;
    }

    .package-description {
      font-size: 13px;
      color: var(--el-text-color-secondary);
      line-height: 1.6;
    }
  }

  .package-status {
    margin-top: 16px;
    display: flex;
    justify-content: flex-end;
  }
}

// 空状态
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 80px 40px;
  text-align: center;
  min-height: 400px;

  .empty-illustration {
    margin-bottom: 24px;
    opacity: 0.5;
    animation: float 3s ease-in-out infinite;
  }

  @keyframes float {
    0%, 100% {
      transform: translateY(0px);
    }
    50% {
      transform: translateY(-10px);
    }
  }

  .empty-title {
    font-size: 18px;
    font-weight: 600;
    color: var(--el-text-color-primary);
    margin-bottom: 12px;
  }

  .empty-desc {
    font-size: 14px;
    color: var(--el-text-color-secondary);
    margin-bottom: 28px;
    max-width: 320px;
    line-height: 1.6;
  }

  .el-button {
    padding: 12px 28px;
    font-size: 14px;
    font-weight: 500;
    border-radius: 24px;
  }
}

// 对话框样式
.macro-edit-dialog {
  :deep(.el-dialog__body) {
    padding: 24px;
  }

  .code-editor {
    :deep(.el-textarea__inner) {
      font-family: 'Consolas', 'Monaco', monospace;
      font-size: 13px;
      line-height: 1.6;
    }
  }

  .params-section {
    width: 100%;

    .params-list {
      display: flex;
      flex-direction: column;
      gap: 10px;
      margin-bottom: 12px;
    }

    .param-item {
      display: flex;
      align-items: center;
      gap: 10px;
      padding: 10px;
      background: var(--el-fill-color-light);
      border-radius: 8px;

      .param-number {
        width: 24px;
        height: 24px;
        border-radius: 50%;
        background: var(--el-color-primary);
        color: white;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 11px;
        font-weight: 600;
      }

      .el-input {
        flex: 1;
      }
    }
  }
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
}

// 模板库对话框
.template-library-dialog {
  :deep(.el-dialog__body) {
    padding: 0;
    height: 70vh;
  }
}

.template-library-content {
  display: flex;
  flex-direction: column;
  height: 100%;

  .template-hint {
    margin-bottom: 16px;
  }

  .template-categories-filter {
    padding: 16px 20px;
    border-bottom: 1px solid var(--el-border-color-lighter);
  }

  .template-library-grid {
    flex: 1;
    overflow-y: auto;
    padding: 20px;
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
    gap: 20px;
  }
}

.library-template-card {
  background: var(--el-bg-color);
  border-radius: 12px;
  border: 1px solid var(--el-border-color-lighter);
  padding: 16px;
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    border-color: var(--el-color-primary);
    box-shadow: 0 6px 16px rgba(64, 158, 255, 0.15);
    transform: translateY(-2px);
  }

  .template-card-preview {
    height: 100px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: var(--el-fill-color-light);
    border-radius: 8px;
    margin-bottom: 12px;
    padding: 16px;
  }

  .template-card-info {
    margin-bottom: 12px;

    .template-card-name {
      code {
        font-family: 'Consolas', 'Monaco', monospace;
        font-size: 14px;
        color: var(--el-color-primary);
        font-weight: 600;
      }
    }

    .template-card-desc {
      font-size: 12px;
      color: var(--el-text-color-secondary);
      margin: 6px 0;
    }

    .template-card-code {
      background: var(--el-fill-color-light);
      padding: 8px;
      border-radius: 6px;

      pre {
        margin: 0;
        font-family: 'Consolas', 'Monaco', monospace;
        font-size: 11px;
        overflow: hidden;
        text-overflow: ellipsis;
        white-space: nowrap;
      }
    }
  }

  .template-card-actions {
    display: flex;
    justify-content: flex-end;
  }
}

// 导入对话框
.import-content {
  .import-area {
    margin-top: 20px;

    .upload-text {
      font-size: 14px;
      color: var(--el-text-color-primary);
      margin-top: 12px;
    }

    .upload-hint {
      font-size: 12px;
      color: var(--el-text-color-secondary);
      margin-top: 4px;
    }
  }
}

// 快速预览对话框
.quick-preview-dialog {
  .preview-content {
    .preview-section {
      margin-bottom: 20px;

      .preview-label {
        font-size: 12px;
        color: var(--el-text-color-secondary);
        margin-bottom: 8px;
        font-weight: 500;
      }

      .preview-value {
        font-size: 14px;
        color: var(--el-text-color-primary);
        line-height: 1.6;
      }

      .code-block {
        background: var(--el-fill-color-light);
        border-radius: 8px;
        padding: 12px;
        position: relative;
        display: flex;
        align-items: flex-start;
        justify-content: space-between;
        gap: 12px;

        code, pre {
          margin: 0;
          font-family: 'Consolas', 'Monaco', monospace;
          font-size: 13px;
          line-height: 1.6;
          white-space: pre-wrap;
          word-break: break-all;
          flex: 1;
        }

        &.full {
          pre {
            font-size: 12px;
          }
        }
      }

      &.stats {
        display: flex;
        gap: 20px;
        padding: 12px;
        background: var(--el-fill-color-light);
        border-radius: 8px;

        .stat-item {
          display: flex;
          align-items: center;
          gap: 6px;
          font-size: 13px;
          color: var(--el-text-color-regular);

          .el-icon {
            color: var(--el-color-primary);
          }
        }
      }

      .params-list {
        display: flex;
        flex-direction: column;
        gap: 8px;

        .param-item {
          display: flex;
          align-items: center;
          gap: 10px;
          padding: 10px;
          background: var(--el-fill-color);
          border-radius: 6px;

          .param-index {
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: var(--el-color-primary);
            color: white;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 11px;
            font-weight: 600;
          }

          .param-name {
            flex: 1;
            font-weight: 500;
          }

          .param-default {
            font-size: 12px;
            color: var(--el-text-color-secondary);
          }
        }
      }
    }
  }
}

// 响应式
@media (max-width: 768px) {
  .macro-header {
    flex-direction: column;
    align-items: stretch;
  }

  .header-actions {
    justify-content: space-between;

    .search-input {
      flex: 1;
    }
  }

  .macro-grid {
    grid-template-columns: 1fr;
  }

  .package-grid {
    grid-template-columns: 1fr;
  }
}
</style>
