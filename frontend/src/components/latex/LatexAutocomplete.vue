<template>
  <Teleport to="body">
    <div
      v-if="isVisible"
      class="latex-autocomplete"
      :style="{
        left: position.x + 'px',
        top: position.y + 'px'
      }"
    >
      <div class="autocomplete-header">
        <span class="autocomplete-query">{{ query }}</span>
        <span class="autocomplete-count">{{ filteredOptions.length }} 项</span>
      </div>

      <div class="autocomplete-options">
        <div
          v-for="(option, index) in filteredOptions"
          :key="option.label"
          class="autocomplete-option"
          :class="{ 'is-selected': index === selectedIndex }"
          @click="$emit('select', option)"
          @mouseenter="$emit('hover', index)"
        >
          <div class="option-label">
            <span class="option-type" :class="'type-' + option.type">
              {{ getTypeLabel(option.type) }}
            </span>
            <span class="option-label-text">{{ option.label }}</span>
          </div>
          <div class="option-description">{{ option.description }}</div>
        </div>

        <div v-if="filteredOptions.length === 0" class="autocomplete-empty">
          无匹配结果
        </div>
      </div>

      <div class="autocomplete-footer">
        <span class="hint">↑↓ 导航</span>
        <span class="hint">Enter 选择</span>
        <span class="hint">Esc 关闭</span>
      </div>
    </div>
  </Teleport>
</template>

<script setup lang="ts">
import type { AutocompleteOption } from '@/composables/useLatexAutocomplete'

interface Props {
  isVisible: boolean
  position: { x: number; y: number }
  query: string
  selectedIndex: number
  filteredOptions: AutocompleteOption[]
}

defineProps<Props>()

defineEmits<{
  select: [option: AutocompleteOption]
  hover: [index: number]
}>()

function getTypeLabel(type: string): string {
  const labels: Record<string, string> = {
    command: 'cmd',
    environment: 'env',
    symbol: 'sym',
    snippet: 'snip'
  }
  return labels[type] || type
}
</script>

<style scoped lang="scss">
.latex-autocomplete {
  position: fixed;
  z-index: 9999;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color);
  border-radius: 8px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.15);
  min-width: 280px;
  max-width: 400px;
  max-height: 350px;
  overflow: hidden;
  font-size: 13px;
}

.autocomplete-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  border-bottom: 1px solid var(--el-border-color-lighter);
  background: var(--el-bg-color-page);

  .autocomplete-query {
    font-weight: 600;
    color: var(--el-color-primary);
    font-family: 'Fira Code', monospace;
  }

  .autocomplete-count {
    font-size: 11px;
    color: var(--el-text-color-secondary);
  }
}

.autocomplete-options {
  max-height: 250px;
  overflow-y: auto;
}

.autocomplete-option {
  padding: 8px 12px;
  cursor: pointer;
  transition: background-color 0.15s;
  border-bottom: 1px solid var(--el-border-color-lighter);

  &:last-child {
    border-bottom: none;
  }

  &:hover,
  &.is-selected {
    background: var(--el-color-primary-light-9);
  }

  &.is-selected {
    border-left: 3px solid var(--el-color-primary);
  }

  .option-label {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 2px;

    .option-type {
      font-size: 9px;
      padding: 2px 4px;
      border-radius: 3px;
      font-weight: 600;
      text-transform: uppercase;

      &.type-command {
        background: var(--el-color-info-light-9);
        color: var(--el-color-info);
      }

      &.type-environment {
        background: var(--el-color-success-light-9);
        color: var(--el-color-success);
      }

      &.type-symbol {
        background: var(--el-color-warning-light-9);
        color: var(--el-color-warning);
      }

      &.type-snippet {
        background: var(--el-color-primary-light-9);
        color: var(--el-color-primary);
      }
    }

    .option-label-text {
      font-family: 'Fira Code', monospace;
      font-weight: 600;
      color: var(--el-text-color-primary);
    }
  }

  .option-description {
    font-size: 11px;
    color: var(--el-text-color-secondary);
    padding-left: 28px;
  }
}

.autocomplete-empty {
  padding: 20px;
  text-align: center;
  color: var(--el-text-color-secondary);
}

.autocomplete-footer {
  display: flex;
  justify-content: center;
  gap: 16px;
  padding: 6px 12px;
  border-top: 1px solid var(--el-border-color-lighter);
  background: var(--el-bg-color-page);

  .hint {
    font-size: 10px;
    color: var(--el-text-color-secondary);
  }
}

// 滚动条样式
.autocomplete-options::-webkit-scrollbar {
  width: 6px;
}

.autocomplete-options::-webkit-scrollbar-track {
  background: transparent;
}

.autocomplete-options::-webkit-scrollbar-thumb {
  background: var(--el-border-color);
  border-radius: 3px;

  &:hover {
    background: var(--el-border-color-darker);
  }
}
</style>
