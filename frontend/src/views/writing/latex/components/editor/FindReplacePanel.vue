<template>
  <transition name="el-zoom-in-top">
    <div class="find-replace-panel" v-if="show" v-click-outside="() => $emit('close')">
      <div class="find-replace-row">
        <el-input
          :model-value="findQuery"
          placeholder="查找..."
          size="small"
          clearable
          @input="$emit('update:findQuery', $event); $emit('find-input', $event)"
          @keydown.enter="$emit('find-next')"
          @keydown.shift.enter="$emit('find-previous')"
          ref="inputRef"
        >
          <template #prefix>
            <el-icon><Search /></el-icon>
          </template>
          <template #suffix>
            <span class="match-count" v-if="findQuery">{{ currentMatchIndex }}/{{ totalMatches }}</span>
          </template>
        </el-input>
        <el-button-group size="small">
          <el-button
            @click="$emit('find-previous')"
            :disabled="!findQuery || totalMatches === 0"
            title="上一个 (Enter)"
          >
            <el-icon><ArrowUp /></el-icon>
          </el-button>
          <el-button
            @click="$emit('find-next')"
            :disabled="!findQuery || totalMatches === 0"
            title="下一个 (Shift+Enter)"
          >
            <el-icon><ArrowDown /></el-icon>
          </el-button>
        </el-button-group>
      </div>

      <div class="find-replace-row" v-if="showReplace">
        <el-input
          :model-value="replaceQuery"
          placeholder="替换为..."
          size="small"
          clearable
          @input="$emit('update:replaceQuery', $event)"
          @keydown.enter="$emit('replace-current')"
        >
          <template #prefix>
            <el-icon><RefreshRight /></el-icon>
          </template>
        </el-input>
        <el-button-group size="small">
          <el-button
            @click="$emit('replace-current')"
            :disabled="!findQuery || totalMatches === 0"
            title="替换当前"
          >
            替换
          </el-button>
          <el-button
            @click="$emit('replace-all')"
            :disabled="!findQuery || totalMatches === 0"
            title="全部替换"
          >
            全部替换
          </el-button>
        </el-button-group>
      </div>

      <div class="find-replace-options">
        <el-checkbox
          :model-value="findOptions.caseSensitive"
          @change="handleCaseSensitiveChange"
          size="small"
        >
          区分大小写
        </el-checkbox>
        <el-checkbox
          :model-value="findOptions.wholeWord"
          @change="handleWholeWordChange"
          size="small"
        >
          全字匹配
        </el-checkbox>
        <el-checkbox
          :model-value="findOptions.useRegex"
          @change="handleUseRegexChange"
          size="small"
        >
          正则表达式
        </el-checkbox>
        <el-link
          @click="$emit('toggle-show-replace')"
          type="primary"
          style="margin-left: auto"
        >
          {{ showReplace ? '隐藏替换' : '显示替换' }}
        </el-link>
      </div>
    </div>
  </transition>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue'
import { Search, ArrowUp, ArrowDown, RefreshRight } from '@element-plus/icons-vue'

interface FindOptions {
  caseSensitive: boolean
  wholeWord: boolean
  useRegex: boolean
}

interface Props {
  show?: boolean
  findQuery: string
  replaceQuery: string
  currentMatchIndex: number
  totalMatches: number
  findOptions: FindOptions
  showReplace: boolean
}

const props = withDefaults(defineProps<Props>(), {
  show: false,
  findQuery: '',
  replaceQuery: '',
  currentMatchIndex: 0,
  totalMatches: 0,
  showReplace: false,
  findOptions: () => ({ caseSensitive: false, wholeWord: false, useRegex: false })
})

const emit = defineEmits<{
  'close': []
  'update:findQuery': [value: string]
  'update:replaceQuery': [value: string]
  'update:findOptions': [value: FindOptions]
  'find-input': [value: string]
  'find-next': []
  'find-previous': []
  'replace-current': []
  'replace-all': []
  'toggle-show-replace': []
}>()

const inputRef = ref<any>()

// Checkbox change handlers with proper typing
function handleCaseSensitiveChange(value: boolean | string | number) {
  emit('update:findOptions', { ...props.findOptions, caseSensitive: Boolean(value) })
}

function handleWholeWordChange(value: boolean | string | number) {
  emit('update:findOptions', { ...props.findOptions, wholeWord: Boolean(value) })
}

function handleUseRegexChange(value: boolean | string | number) {
  emit('update:findOptions', { ...props.findOptions, useRegex: Boolean(value) })
}

// Focus input when panel opens
watch(() => props.show, (show) => {
  if (show) {
    setTimeout(() => {
      inputRef.value?.focus()
    }, 100)
  }
})

defineExpose({
  inputRef
})
</script>

<style scoped lang="scss">
.find-replace-panel {
  position: absolute;
  top: 100%;
  left: 0;
  right: 0;
  z-index: 10;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color);
  border-radius: 4px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
  padding: 12px;
  margin-top: 4px;
}

.find-replace-row {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;

  &:last-child {
    margin-bottom: 0;
  }
}

.find-replace-options {
  display: flex;
  align-items: center;
  gap: 12px;
  font-size: 12px;
}

.match-count {
  color: var(--el-text-color-secondary);
  font-size: 12px;
}

.el-zoom-in-top-enter-active,
.el-zoom-in-top-leave-active {
  transition: all 0.2s ease;
}

.el-zoom-in-top-enter-from,
.el-zoom-in-top-leave-to {
  opacity: 0;
  transform: scaleY(0.8) translateX(-10px);
}
</style>
