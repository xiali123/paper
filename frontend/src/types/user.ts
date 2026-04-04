export interface LoginRequest {
  username: string
  password: string
  remember?: boolean
}

export interface UserInfo {
  id: number
  username: string
  name: string
  email: string
  role: string
  permissions: string[]
  createdAt: string
  updatedAt: string
}

export interface LoginResponse {
  token: string
  refreshToken: string
  user: UserInfo
  permissions: string[]
}

export interface User {
  id: number
  username: string
  name: string
  email: string
  role: string
}
