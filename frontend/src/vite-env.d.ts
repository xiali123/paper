/// <reference types="vite/client" />

declare interface Window {
  _errorAlertShown?: boolean
}

interface ImportMetaEnv {
  readonly VITE_API_BASE_URL: string
  readonly VITE_APP_ENV: string
  readonly VITE_ENABLE_REALTIME_SEARCH: string
  readonly VITE_ENABLE_HEALTH_CHECK: string
}

interface ImportMeta {
  readonly env: ImportMetaEnv
}
