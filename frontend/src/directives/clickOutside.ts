/**
 * v-click-outside directive
 * Calls handler when clicked outside of element
 */
import type { Directive, DirectiveBinding } from 'vue'

const clickOutsideDirective: Directive = {
  mounted(el: HTMLElement, binding: DirectiveBinding) {
    el._clickOutside = (event: MouseEvent) => {
      if (!(el === event.target || el.contains(event.target as Node))) {
        binding.value(event)
      }
    }
    document.addEventListener('click', el._clickOutside as EventListener)
  },
  unmounted(el: HTMLElement) {
    document.removeEventListener('click', el._clickOutside as EventListener)
    delete el._clickOutside
  }
}

declare global {
  interface HTMLElement {
    _clickOutside?: (event: MouseEvent) => void
  }
}

export default clickOutsideDirective
