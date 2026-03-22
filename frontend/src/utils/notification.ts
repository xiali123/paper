/**
 * Simple Notification Utility
 *
 * Provides toast-style notifications without external dependencies
 */

type NotificationType = 'success' | 'warning' | 'error' | 'info'

interface NotificationOptions {
  message: string
  type?: NotificationType
  duration?: number
}

class NotificationManager {
  private container: HTMLElement | null = null

  constructor() {
    this.createContainer()
  }

  private createContainer() {
    if (typeof document === 'undefined') return

    this.container = document.createElement('div')
    this.container.id = 'notification-container'
    this.container.style.cssText = `
      position: fixed;
      top: 20px;
      right: 20px;
      z-index: 9999;
      display: flex;
      flex-direction: column;
      gap: 10px;
    `
    document.body.appendChild(this.container)
  }

  show(options: NotificationOptions) {
    if (!this.container) {
      this.createContainer()
    }

    const notification = document.createElement('div')
    const type = options.type || 'info'
    const duration = options.duration || 3000

    const colors = {
      success: '#67c23a',
      warning: '#e6a23c',
      error: '#f56c6c',
      info: '#909399'
    }

    notification.style.cssText = `
      background: white;
      border-left: 4px solid ${colors[type]};
      padding: 12px 20px;
      border-radius: 4px;
      box-shadow: 0 2px 12px rgba(0, 0, 0, 0.1);
      min-width: 300px;
      max-width: 400px;
      display: flex;
      align-items: center;
      animation: slideIn 0.3s ease-out;
      font-size: 14px;
      color: #333;
    `

    notification.innerHTML = `
      <span style="flex: 1">${options.message}</span>
      <span style="cursor: pointer; margin-left: 10px; color: #999;">&times;</span>
    `

    // Add styles for animation
    if (!document.getElementById('notification-styles')) {
      const style = document.createElement('style')
      style.id = 'notification-styles'
      style.textContent = `
        @keyframes slideIn {
          from {
            transform: translateX(100%);
            opacity: 0;
          }
          to {
            transform: translateX(0);
            opacity: 1;
          }
        }
        @keyframes slideOut {
          from {
            transform: translateX(0);
            opacity: 1;
          }
          to {
            transform: translateX(100%);
            opacity: 0;
          }
        }
      `
      document.head.appendChild(style)
    }

    // Close button functionality
    const closeBtn = notification.querySelector('span:last-child')
    closeBtn?.addEventListener('click', () => {
      this.dismiss(notification)
    })

    this.container?.appendChild(notification)

    // Auto dismiss
    setTimeout(() => {
      this.dismiss(notification)
    }, duration)
  }

  private dismiss(notification: HTMLElement) {
    notification.style.animation = 'slideOut 0.3s ease-in'
    setTimeout(() => {
      notification.remove()
    }, 300)
  }

  success(message: string) {
    this.show({ message, type: 'success' })
  }

  warning(message: string) {
    this.show({ message, type: 'warning' })
  }

  error(message: string) {
    this.show({ message, type: 'error' })
  }

  info(message: string) {
    this.show({ message, type: 'info' })
  }
}

// Singleton instance
const notification = new NotificationManager()

// Export as ElMessage-compatible API
export const ElMessage = {
  success: (message: string) => notification.success(message),
  warning: (message: string) => notification.warning(message),
  error: (message: string) => notification.error(message),
  info: (message: string) => notification.info(message)
}

export default notification
