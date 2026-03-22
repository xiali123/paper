#!/usr/bin/env node
/**
 * Mock Authentication API Server
 * 用于前端测试，模拟后端认证端点
 *
 * 使用方法：
 * 1. 确保 Node.js 已安装
 * 2. 安装依赖：npm install express cors
 * 3. 运行：node mock-auth-api.js
 */

const express = require('express')
const cors = require('cors')
const app = express()
const PORT = 8080

app.use(cors())
app.use(express.json())

// 模拟用户数据库
const users = []
const sessions = []

// 生成简单的 JWT-like token
function generateToken() {
  return Buffer.from(JSON.stringify({
    id: Math.random().toString(36),
    exp: Date.now() + 15 * 60 * 1000 // 15分钟
  })).toString('base64')
}

// POST /api/auth/register
app.post('/api/auth/register', (req, res) => {
  console.log('[Mock API] Register request:', req.body)

  const { username, email, password, fullName } = req.body

  // 验证
  if (!username || !email || !password) {
    return res.status(400).json({
      success: false,
      error: 'Missing required fields'
    })
  }

  // 检查邮箱是否已存在
  if (users.find(u => u.email === email)) {
    return res.status(400).json({
      success: false,
      error: 'Email already registered'
    })
  }

  // 创建用户
  const user = {
    id: users.length + 1,
    username,
    email,
    fullName: fullName || '',
    role: 'user',
    createdAt: new Date().toISOString()
  }

  users.push(user)

  console.log('[Mock API] User created:', user)

  // 生成 token
  const accessToken = generateToken()
  const refreshToken = generateToken()

  sessions.push({
    userId: user.id,
    refreshToken,
    createdAt: new Date().toISOString()
  })

  res.json({
    success: true,
    data: {
      user,
      tokens: {
        accessToken,
        refreshToken,
        expiresIn: 900
      }
    }
  })
})

// POST /api/auth/login
app.post('/api/auth/login', (req, res) => {
  console.log('[Mock API] Login request:', req.body)

  const { email, password } = req.body

  // 查找用户
  const user = users.find(u => u.email === email)

  if (!user) {
    return res.status(401).json({
      success: false,
      error: 'Invalid email or password'
    })
  }

  // 生成 token
  const accessToken = generateToken()
  const refreshToken = generateToken()

  sessions.push({
    userId: user.id,
    refreshToken,
    createdAt: new Date().toISOString()
  })

  console.log('[Mock API] User logged in:', user)

  res.json({
    success: true,
    data: {
      user,
      tokens: {
        accessToken,
        refreshToken,
        expiresIn: 900
      }
    }
  })
})

// POST /api/auth/logout
app.post('/api/auth/logout', (req, res) => {
  console.log('[Mock API] Logout request')

  res.json({
    success: true,
    data: { message: 'Logged out successfully' }
  })
})

// POST /api/auth/refresh
app.post('/api/auth/refresh', (req, res) => {
  console.log('[Mock API] Token refresh request')

  const { refreshToken } = req.body

  const session = sessions.find(s => s.refreshToken === refreshToken)

  if (!session) {
    return res.status(401).json({
      success: false,
      error: 'Invalid refresh token'
    })
  }

  const user = users.find(u => u.id === session.userId)

  if (!user) {
    return res.status(401).json({
      success: false,
      error: 'User not found'
    })
  }

  // 生成新 token
  const accessToken = generateToken()
  const newRefreshToken = generateToken()

  // 更新 session
  session.refreshToken = newRefreshToken

  res.json({
    success: true,
    data: {
      tokens: {
        accessToken,
        refreshToken: newRefreshToken,
        expiresIn: 900
      }
    }
  })
})

// GET /api/auth/me
app.get('/api/auth/me', (req, res) => {
  console.log('[Mock API] Get current user')

  // 简单验证：从 header 获取 token
  const authHeader = req.headers.authorization

  if (!authHeader) {
    return res.status(401).json({
      success: false,
      error: 'No authorization header'
    })
  }

  // 返回模拟用户
  const user = users[0] || {
    id: 1,
    username: 'testuser',
    email: 'test@example.com',
    fullName: 'Test User',
    role: 'user'
  }

  res.json({
    success: true,
    data: user
  })
})

// GET /health
app.get('/health', (req, res) => {
  res.json({
    status: 'healthy',
    database: 'connected (mock)',
    authentication: 'enabled (mock)'
  })
})

// 启动服务器
app.listen(PORT, () => {
  console.log(`\n========================================`)
  console.log(`Mock Authentication API Server`)
  console.log(`========================================`)
  console.log(`Running on: http://127.0.0.1:${PORT}`)
  console.log(``)
  console.log(`Available endpoints:`)
  console.log(`  POST /api/auth/register`)
  console.log(`  POST /api/auth/login`)
  console.log(`  POST /api/auth/logout`)
  console.log(`  POST /api/auth/refresh`)
  console.log(`  GET  /api/auth/me`)
  console.log(`  GET  /health`)
  console.log(``)
  console.log(`Test credentials:`)
  console.log(`  Email: test@example.com`)
  console.log(`  Password: any password`)
  console.log(`========================================\n`)
})

module.exports = app
