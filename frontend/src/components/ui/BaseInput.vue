<template>
  <div :class="inputGroupClasses">
    <label v-if="label" :for="inputId" class="input-label">
      {{ label }}
      <span v-if="required" class="input-required">*</span>
    </label>

    <div class="input-wrapper">
      <span v-if="prefixIcon" class="input-icon input-prefix-icon">
        <component :is="prefixIcon" />
      </span>

      <input
        :id="inputId"
        ref="inputRef"
        v-model="inputValue"
        :type="type"
        :placeholder="placeholder"
        :disabled="disabled"
        :readonly="readonly"
        :min="min"
        :max="max"
        :step="step"
        :autocomplete="autocomplete"
        :class="inputClasses"
        v-bind="$attrs"
        @focus="handleFocus"
        @blur="handleBlur"
        @input="handleInput"
        @change="handleChange"
      />

      <span v-if="suffixIcon || showClearButton" class="input-icon input-suffix-icon">
        <button
          v-if="showClearButton && !disabled && !readonly"
          type="button"
          class="input-clear-button"
          @click="handleClear"
          aria-label="Clear input"
        >
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
            <line x1="18" y1="6" x2="6" y2="18"/>
            <line x1="6" y1="6" x2="18" y2="18"/>
          </svg>
        </button>
        <component v-else-if="suffixIcon" :is="suffixIcon" />
      </span>
    </div>

    <div v-if="hint || errorMessage" class="input-footer">
      <p v-if="hint && !errorMessage" class="input-hint">{{ hint }}</p>
      <p v-if="errorMessage" class="input-error">{{ errorMessage }}</p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, nextTick } from 'vue'

interface Props {
  modelValue: string | number
  type?: string
  label?: string
  placeholder?: string
  disabled?: boolean
  readonly?: boolean
  required?: boolean
  error?: boolean
  errorMessage?: string
  hint?: string
  size?: 'sm' | 'md' | 'lg'
  prefixIcon?: any
  suffixIcon?: any
  clearable?: boolean
  min?: number | string
  max?: number | string
  step?: number | string
  autocomplete?: string
  id?: string
}

const props = withDefaults(defineProps<Props>(), {
  type: 'text',
  disabled: false,
  readonly: false,
  required: false,
  error: false,
  size: 'md',
  clearable: false,
  autocomplete: 'off'
})

const emit = defineEmits<{
  'update:modelValue': [value: string | number]
  focus: [event: FocusEvent]
  blur: [event: FocusEvent]
  input: [event: Event]
  change: [event: Event]
  clear: []
}>()

const inputRef = ref<HTMLInputElement>()
const isFocused = ref(false)

const inputId = computed(() => props.id || `input-${Math.random().toString(36).substr(2, 9)}`)

const inputValue = computed({
  get: () => props.modelValue,
  set: (value) => emit('update:modelValue', value)
})

const showClearButton = computed(() => props.clearable && inputValue.value && !props.disabled)

const inputGroupClasses = computed(() => [
  'input-group',
  `input-group--${props.size}`,
  {
    'input-group--disabled': props.disabled,
    'input-group--error': props.error || props.errorMessage,
    'input-group--focused': isFocused.value
  }
])

const inputClasses = computed(() => [
  'base-input',
  `base-input--${props.size}`,
  {
    'base-input--has-prefix': props.prefixIcon,
    'base-input--has-suffix': props.suffixIcon || showClearButton.value
  }
])

const handleFocus = (event: FocusEvent) => {
  isFocused.value = true
  emit('focus', event)
}

const handleBlur = (event: FocusEvent) => {
  isFocused.value = false
  emit('blur', event)
}

const handleInput = (event: Event) => {
  emit('input', event)
}

const handleChange = (event: Event) => {
  emit('change', event)
}

const handleClear = () => {
  inputValue.value = ''
  emit('clear')
  nextTick(() => {
    inputRef.value?.focus()
  })
}

const focus = () => {
  inputRef.value?.focus()
}

const blur = () => {
  inputRef.value?.blur()
}

defineExpose({
  focus,
  blur
})
</script>

<style scoped>
.input-group {
  display: flex;
  flex-direction: column;
  gap: var(--space-2);
}

.input-label {
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-medium);
  color: var(--color-text-primary);
  display: flex;
  align-items: center;
  gap: var(--space-1);
}

.input-required {
  color: var(--color-error);
  font-weight: var(--font-weight-bold);
}

.input-wrapper {
  position: relative;
  display: flex;
  align-items: center;
}

.base-input {
  width: 100%;
  font-family: var(--font-family-primary);
  font-size: var(--font-size-base);
  color: var(--color-text-primary);
  background-color: var(--input-bg);
  border: var(--input-border);
  border-radius: var(--radius-lg);
  outline: none;
  transition: all var(--transition-fast);
}

.base-input::placeholder {
  color: var(--color-text-tertiary);
}

.base-input:hover:not(:disabled) {
  border-color: var(--color-border-secondary);
}

.base-input:focus {
  border-color: var(--color-border-focus);
  box-shadow: var(--input-focus-shadow);
}

.base-input:disabled {
  background-color: var(--color-bg-tertiary);
  color: var(--color-text-tertiary);
  cursor: not-allowed;
}

/* Sizes */
.base-input--sm {
  padding: var(--space-2) var(--space-3);
  font-size: var(--font-size-sm);
  min-height: 32px;
}

.base-input--md {
  padding: var(--space-3) var(--space-4);
  font-size: var(--font-size-base);
  min-height: 40px;
}

.base-input--lg {
  padding: var(--space-4) var(--space-5);
  font-size: var(--font-size-lg);
  min-height: 48px;
}

/* Icons */
.base-input--has-prefix {
  padding-left: var(--space-10);
}

.base-input--has-suffix {
  padding-right: var(--space-10);
}

.input-icon {
  position: absolute;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--color-text-tertiary);
  pointer-events: none;
}

.input-prefix-icon {
  left: var(--space-3);
}

.input-suffix-icon {
  right: var(--space-3);
}

.input-clear-button {
  background: none;
  border: none;
  padding: var(--space-1);
  cursor: pointer;
  color: var(--color-text-tertiary);
  border-radius: var(--radius-sm);
  pointer-events: auto;
  display: flex;
  align-items: center;
  justify-content: center;
}

.input-clear-button:hover {
  color: var(--color-text-primary);
  background-color: var(--color-bg-tertiary);
}

.input-clear-button svg {
  width: 16px;
  height: 16px;
}

/* States */
.input-group--error .base-input {
  border-color: var(--color-error);
}

.input-group--error .base-input:focus {
  border-color: var(--color-error);
  box-shadow: 0 0 0 3px rgba(239, 68, 68, 0.1);
}

.input-group--focused .input-icon {
  color: var(--color-primary);
}

/* Footer */
.input-footer {
  display: flex;
  flex-direction: column;
  gap: var(--space-1);
  min-height: 20px;
}

.input-hint,
.input-error {
  font-size: var(--font-size-xs);
  margin: 0;
}

.input-hint {
  color: var(--color-text-secondary);
}

.input-error {
  color: var(--color-error);
}

/* Dark theme */
[data-theme='dark'] .base-input {
  background-color: var(--input-bg);
  color: var(--color-text-primary);
}

[data-theme='dark'] .base-input::placeholder {
  color: var(--color-text-tertiary);
}

/* Accessibility */
.base-input:focus-visible {
  outline: 2px solid var(--color-border-focus);
  outline-offset: 2px;
}

/* Reduced motion */
@media (prefers-reduced-motion: reduce) {
  .base-input {
    transition: border-color var(--transition-fast);
  }
}
</style>
