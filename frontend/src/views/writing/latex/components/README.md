# LaTeX Editor 组件库

从 `LatexEditorView.vue` 提取的可复用组件，总计 **22个组件**，**~2000行代码**。

## 目录结构

```
components/
├── dialogs/        # 对话框组件 (11个)
├── editor/         # 编辑器组件 (2个)
├── panels/         # 面板组件 (8个)
├── preview/        # 预览组件 (1个)
└── index.ts        # 统一导出
```

## 组件分类

### 对话框组件 (dialogs/)

| 组件 | 描述 | Props |
|------|------|-------|
| **BaseDialog** | 基础对话框容器 | show, title, width, closeOnClickModal |
| **QuickInsertDialog** | 快速插入对话框 | - |
| **AiRecognizerDialog** | AI公式识别对话框 | - |
| **ReviewModeDialog** | 审阅模式对话框 | - |
| **ExportDialogWrapper** | 导出对话框包装器 | - |
| **WelcomeGuideDialog** | 欢迎引导对话框 | - |
| **ShortcutHelpDialog** | 快捷键帮助对话框 | - |
| **SettingsDialog** | 编辑器设置对话框 | theme, fontSize, tabSize, wordWrap... |
| **StatsDialog** | 文档统计对话框 | stats, documentId |
| **ImageManagerDialog** | 图片管理器 | show, projectId, images, documentContent |
| **BibTeXManagerDialog** | BibTeX文献管理 | show, projectId, entries, documentContent |

### 编辑器组件 (editor/)

| 组件 | 描述 | Props | Emits |
|------|------|-------|-------|
| **EditorToolbar** | 编辑器工具栏 | showOutline, canUndo, canRedo, compilationStatus | toggle-left-panel, undo, redo, insert-command... |
| **FindReplacePanel** | 查找替换面板 | show, findQuery, replaceQuery, findOptions | update:findQuery, find-next, replace-current... |

### 面板组件 (panels/)

| 组件 | 描述 | Props | Emits |
|------|------|-------|-------|
| **PanelDrawer** | 统一的drawer容器 | show, title, direction, size | update:show |
| **LeftPanel** | 左侧面板(大纲/项目树) | mode, panelTitle, content, isProjectMode... | close, navigate, file-select... |
| **SymbolPanel** | LaTeX符号面板 | show | update:show, insert |
| **SnippetsPanel** | 代码片段面板 | show | update:show, insert |
| **TemplatesPanel** | 模板管理面板 | show | update:show, insert |
| **TablePanel** | 表格生成器面板 | show | update:show, insert |
| **SpellCheckPanel** | 拼写检查面板 | show, content | update:show, replace, goto |
| **FontPanel** | 字体选择面板 | show | update:show, select |

### 预览组件 (preview/)

| 组件 | 描述 | Props | Emits |
|------|------|-------|-------|
| **PreviewPanel** | 预览面板(HTML/PDF) | show, layoutMode, previewMode, content... | update:previewMode, zoom-in, compile... |

## 使用方式

### 导入组件

```typescript
// 从index.ts导入所有组件
import {
  EditorToolbar,
  FindReplacePanel,
  PreviewPanel,
  LeftPanel,
  SymbolPanel,
  QuickInsertDialog,
  // ... 更多组件
} from './latex/components'
```

### 直接导入单个组件

```typescript
import EditorToolbar from './latex/components/editor/EditorToolbar.vue'
```

## 组件通信模式

### Props (父 → 子)

所有组件使用TypeScript接口定义Props：

```typescript
interface Props {
  show: boolean
  title: string
  // ... 更多props
}
```

### Emits (子 → 父)

使用defineEmits定义事件：

```typescript
const emit = defineEmits<{
  'update:show': [value: boolean]
  'insert': [content: string]
}>()
```

### Refs (父访问子)

使用defineExpose暴露方法：

```typescript
defineExpose({
  open: () => dialogRef.value?.open()
})
```

## 设计原则

1. **单一职责**: 每个组件只负责一个功能
2. **Props down, Events up**: 单向数据流
3. **TypeScript优先**: 完整的类型定义
4. **可复用性**: 组件可在其他地方复用
5. **统一API**: PanelDrawer/BaseDialog提供一致的容器样式

## 扩展指南

### 添加新面板组件

1. 在 `panels/` 目录创建组件
2. 使用 `PanelDrawer` 作为容器
3. 定义Props和Emits接口
4. 在 `index.ts` 中导出

### 添加新对话框组件

1. 在 `dialogs/` 目录创建组件
2. 使用 `BaseDialog` 或创建wrapper
3. 暴露 `open()` 方法通过 `defineExpose`
4. 在 `index.ts` 中导出
