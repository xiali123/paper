#!/usr/bin/env node
/**
 * Mock Authentication API Server v2
 * 使用端口 8081 避免冲突
 */

const express = require('express')
const cors = require('cors')
const app = express()
const PORT = 8081  // 使用不同端口

app.use(cors())
app.use(express.json())

// 模拟用户数据库
const users = [{
  id: 1,
  username: 'S221000789',
  email: 'x2830540584@163.com',
  password: 'Xl1234567890*#',  // 模拟存储
  fullName: 'xiali',
  role: 'user'
}]

const sessions = []

// 生成简单的 JWT-like token
function generateToken() {
  return Buffer.from(JSON.stringify({
    id: Math.random().toString(36),
    exp: Date.now() + 15 * 60 * 1000
  })).toString('base64')
}

// 日志中间件
app.use((req, res, next) => {
  console.log(`[${new Date().toISOString()}] ${req.method} ${req.path}`)
  if (req.body && Object.keys(req.body).length > 0) {
    console.log('Body:', JSON.stringify(req.body, null, 2))
  }
  next()
})

// POST /api/auth/register
app.post('/api/auth/register', (req, res) => {
  const { username, email, password, fullName } = req.body

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

  const accessToken = generateToken()
  const refreshToken = generateToken()

  sessions.push({
    userId: user.id,
    refreshToken,
    createdAt: new Date().toISOString()
  })

  console.log('✅ User created:', user)

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
  const { email, password } = req.body

  const user = users.find(u => u.email === email)

  if (!user) {
    console.log('❌ Login failed: User not found')
    return res.status(401).json({
      success: false,
      error: 'Invalid email or password'
    })
  }

  // 简化验证（实际应该使用密码哈希）
  const accessToken = generateToken()
  const refreshToken = generateToken()

  sessions.push({
    userId: user.id,
    refreshToken,
    createdAt: new Date().toISOString()
  })

  console.log('✅ User logged in:', user.email)

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
  console.log('✅ User logged out')
  res.json({
    success: true,
    data: { message: 'Logged out successfully' }
  })
})

// POST /api/auth/refresh
app.post('/api/auth/refresh', (req, res) => {
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

  const accessToken = generateToken()
  const newRefreshToken = generateToken()

  session.refreshToken = newRefreshToken

  console.log('✅ Token refreshed for:', user.email)

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
  const authHeader = req.headers.authorization

  if (!authHeader) {
    return res.status(401).json({
      success: false,
      error: 'No authorization header'
    })
  }

  const user = users[0]

  res.json({
    success: true,
    data: user
  })
})

// GET /health
app.get('/health', (req, res) => {
  res.json({
    success: true,
    data: {
      status: 'ok',
      message: 'Server is running',
      users: users.length,
      sessions: sessions.length
    },
    timestamp: Date.now()
  })
})

// 启动服务器
app.listen(PORT, () => {
  console.log(`\n========================================`)
  console.log(`Mock Authentication API Server v2`)
  console.log(`========================================`)
  console.log(`Running on: http://127.0.0.1:${PORT}`)
  console.log(``)
  console.log(`Registered Users:`)
  users.forEach(u => {
    console.log(`  - ${u.email} (${u.username})`)
  })
  console.log(``)
  console.log(`Available endpoints:`)
  console.log(`  POST /api/auth/register`)
  console.log(`  POST /api/auth/login`)
  console.log(`  POST /api/auth/logout`)
  console.log(`  POST /api/auth/refresh`)
  console.log(`  GET  /api/auth/me`)
  console.log(`  GET  /health`)
  console.log(`========================================\n`)
})

module.exports = app
