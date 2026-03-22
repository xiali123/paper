#!/usr/bin/env node
/**
 * Complete Mock API Server
 * 包含认证 + 业务端点
 */

const express = require('express')
const cors = require('cors')
const app = express()
const PORT = 8082  // 使用不同端口避免冲突

app.use(cors())
app.use(express.json())

// 模拟用户数据库
const users = [
  {
    id: 1,
    username: 'S221000789',
    email: 'x2830540584@163.com',
    password: 'Xl1234567890*#',
    fullName: 'xiali',
    role: 'user',
    isActive: true,
    createdAt: '2024-01-15T10:30:00Z'
  },
  {
    id: 2,
    username: 'superadmin',
    email: 'superadmin@papercrawler.local',
    password: 'SuperAdmin123!',
    fullName: 'Super Administrator',
    role: 'superadmin',
    isActive: true,
    createdAt: '2024-01-01T00:00:00Z'
  },
  {
    id: 3,
    username: 'admin',
    email: 'admin@papercrawler.local',
    password: 'Admin123!',
    fullName: 'System Admin',
    role: 'admin',
    isActive: true,
    createdAt: '2024-01-10T08:00:00Z'
  },
  {
    id: 4,
    username: 'premium_user',
    email: 'premium@example.com',
    password: 'Premium123!',
    fullName: 'Premium User',
    role: 'premium',
    isActive: true,
    createdAt: '2024-02-01T12:00:00Z'
  }
]

const sessions = []

// 模拟审计日志
const auditLogs = [
  {
    id: 1,
    adminUserId: 2,
    adminUsername: 'superadmin',
    targetUserId: 4,
    action: 'role_changed',
    entityType: 'user',
    entityId: 4,
    oldValues: { role: 'user' },
    newValues: { role: 'premium' },
    status: 'success',
    createdAt: '2024-02-01T12:30:00Z'
  },
  {
    id: 2,
    adminUserId: 2,
    adminUsername: 'superadmin',
    targetUserId: 3,
    action: 'user_created',
    entityType: 'user',
    entityId: 3,
    oldValues: null,
    newValues: { role: 'admin' },
    status: 'success',
    createdAt: '2024-01-10T08:00:00Z'
  }
]

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

// ============================================================================
// 认证端点
// ============================================================================

// POST /api/auth/register
app.post('/api/auth/register', (req, res) => {
  const { username, email, password, fullName } = req.body

  if (users.find(u => u.email === email)) {
    return res.status(400).json({
      success: false,
      error: 'Email already registered'
    })
  }

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

  console.log('🔍 Login attempt:', { email, passwordLength: password?.length })

  const user = users.find(u => u.email === email)

  if (!user) {
    console.log('❌ Login failed: User not found for email:', email)
    return res.status(401).json({
      success: false,
      error: 'Invalid email or password'
    })
  }

  // 简单的密码验证（实际生产环境应该使用哈希比较）
  if (user.password !== password) {
    console.log('❌ Login failed: Wrong password for user:', user.email)
    return res.status(401).json({
      success: false,
      error: 'Invalid email or password'
    })
  }

  const accessToken = generateToken()
  const refreshToken = generateToken()

  sessions.push({
    userId: user.id,
    refreshToken,
    createdAt: new Date().toISOString()
  })

  console.log('✅ User logged in successfully:', user.email, 'Role:', user.role)

  // 返回用户时不包含密码
  const { password: _, ...userWithoutPassword } = user

  res.json({
    success: true,
    data: {
      user: userWithoutPassword,
      tokens: {
        accessToken,
        refreshToken,
        expiresIn: 900,
        expiresAt: Date.now() + 15 * 60 * 1000
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

// ============================================================================
// 业务端点 - Stats
// ============================================================================

// GET /api/stats/overview
app.get('/api/stats/overview', (req, res) => {
  console.log('✅ Stats overview requested')

  res.json({
    success: true,
    data: {
      totalPapers: 12500,
      totalJournals: 850,
      topTierPapers: 3200,
      papersLastYear: 1850,
      mostActiveJournal: {
        id: 1,
        name: 'IEEE Transactions on Pattern Analysis and Machine Intelligence',
        paperCount: 156
      },
      growthRate: 12.5
    }
  })
})

// ============================================================================
// 业务端点 - Search
// ============================================================================

// GET /api/search
app.get('/api/search', (req, res) => {
  const { query, page, offset, limit = 10, q } = req.query
  const limitNum = parseInt(limit)

  // 支持两种分页方式: page 或 offset
  let pageNum = 1
  if (offset !== undefined) {
    // 前端使用 offset 分页
    const offsetNum = parseInt(offset)
    pageNum = Math.floor(offsetNum / limitNum) + 1
  } else if (page !== undefined) {
    // 直接指定页码
    pageNum = parseInt(page)
  }

  // 支持两种查询参数: query 或 q
  const searchQuery = query || q || ''

  console.log('✅ Search requested:', { query: searchQuery, page: pageNum, offset, limit: limitNum })

  // Extended mock database with 25 papers for pagination testing
  const allPapers = [
    { id: 1, title: 'Deep Learning for Computer Vision: A Comprehensive Review', authors: ['Zhang Wei', 'Li Ming', 'Wang Fang'], abstract: 'This paper presents a comprehensive review of deep learning techniques in computer vision...', year: 2023, journal: 'IEEE Transactions on Pattern Analysis and Machine Intelligence', citations: 156, doi: '10.1109/TPAMI.2023.1234567' },
    { id: 2, title: 'Natural Language Processing with Transformers', authors: ['Chen Xi', 'Liu Yang'], abstract: 'We propose a novel transformer architecture for NLP tasks...', year: 2023, journal: 'ACL', citations: 89, doi: '10.1007/978-3-030-12345-6_1' },
    { id: 3, title: 'Graph Neural Networks for Molecular Property Prediction', authors: ['Wang Lei', 'Zhao Min'], abstract: 'We apply GNNs to predict molecular properties...', year: 2023, journal: 'NeurIPS', citations: 124, doi: '10.1007/978-3-030-12345-6_2' },
    { id: 4, title: 'Attention Mechanisms in Deep Learning', authors: ['Li Hua', 'Wu Qiang'], abstract: 'A survey of attention mechanisms across different domains...', year: 2022, journal: 'ICML', citations: 201, doi: '10.1007/978-3-030-12345-6_3' },
    { id: 5, title: 'Reinforcement Learning for Game AI', authors: ['Liu Xing', 'Sun Hao'], abstract: 'Deep RL achieves superhuman performance in games...', year: 2023, journal: 'AAAI', citations: 178, doi: '10.1007/978-3-030-12345-6_4' },
    { id: 6, title: 'Federated Learning: Privacy-Preserving ML', authors: ['Yang Fan', 'Zhou Jie'], abstract: 'We propose a federated learning framework with differential privacy...', year: 2023, journal: 'ICDE', citations: 95, doi: '10.1007/978-3-030-12345-6_5' },
    { id: 7, title: 'Self-Supervised Learning in Computer Vision', authors: ['Zhao Yu', 'Xu Ming'], abstract: 'Self-supervised pretraining reduces reliance on labeled data...', year: 2022, journal: 'CVPR', citations: 234, doi: '10.1007/978-3-030-12345-6_6' },
    { id: 8, title: 'Large Language Models: A Survey', authors: ['Wang Xi', 'Li Kai'], abstract: 'Comprehensive survey of LLMs architecture and applications...', year: 2023, journal: 'ACM Computing Surveys', citations: 312, doi: '10.1007/978-3-030-12345-6_7' },
    { id: 9, title: 'Neural Architecture Search with RL', authors: ['Chen Hao', 'Zhang Lin'], abstract: 'Automated neural architecture design using reinforcement learning...', year: 2022, journal: 'ICLR', citations: 167, doi: '10.1007/978-3-030-12345-6_8' },
    { id: 10, title: 'Multimodal Learning with Vision and Language', authors: ['Liu Yan', 'Wang Jun'], abstract: 'Joint representation learning for vision and language tasks...', year: 2023, journal: 'ECCV', citations: 143, doi: '10.1007/978-3-030-12345-6_9' },
    { id: 11, title: 'Adversarial Training for Robust Deep Learning', authors: ['Zhou Wei', 'Xu Yang'], abstract: 'Improving model robustness through adversarial examples...', year: 2023, journal: 'IJCAI', citations: 189, doi: '10.1007/978-3-030-12345-6_10' },
    { id: 12, title: 'Knowledge Graph Construction from Text', authors: ['Wang Ming', 'Li Qing'], abstract: 'Automated knowledge graph extraction from unstructured text...', year: 2022, journal: 'EMNLP', citations: 98, doi: '10.1007/978-3-030-12345-6_11' },
    { id: 13, title: 'Meta-Learning for Few-Shot Classification', authors: ['Chen Yu', 'Zhao Lin'], abstract: 'Learning to learn with gradient-based meta-learning...', year: 2023, journal: 'NeurIPS', citations: 221, doi: '10.1007/978-3-030-12345-6_12' },
    { id: 14, title: 'Diffusion Models for Image Generation', authors: ['Liu Kai', 'Wang Hua'], abstract: 'Denoising diffusion probabilistic models for high-quality images...', year: 2023, journal: 'ICCV', citations: 287, doi: '10.1007/978-3-030-12345-6_13' },
    { id: 15, title: 'Explainable AI: Methods and Applications', authors: ['Zhang Ming', 'Li Xu'], abstract: 'Interpretable machine learning for critical applications...', year: 2022, journal: 'Nature Machine Intelligence', citations: 176, doi: '10.1007/978-3-030-12345-6_14' },
    { id: 16, title: 'Continual Learning without Catastrophic Forgetting', authors: ['Wang Lei', 'Chen Xi'], abstract: 'Lifelong learning with elastic weight consolidation...', year: 2023, journal: 'ICML', citations: 134, doi: '10.1007/978-3-030-12345-6_15' },
    { id: 17, title: '3D Point Cloud Processing with Deep Learning', authors: ['Zhao Qing', 'Xu Ming'], abstract: 'Hierarchical neural networks for 3D point cloud understanding...', year: 2023, journal: 'CVPR', citations: 156, doi: '10.1007/978-3-030-12345-6_16' },
    { id: 18, title: 'Bayesian Deep Learning for Uncertainty Quantification', authors: ['Li Wei', 'Zhang Hao'], abstract: 'Probabilistic deep learning for reliable predictions...', year: 2022, journal: 'AISTATS', citations: 112, doi: '10.1007/978-3-030-12345-6_17' },
    { id: 19, title: 'Speech Recognition with End-to-End Models', authors: ['Wang Yu', 'Chen Lin'], abstract: 'Transformer-based speech recognition systems...', year: 2023, journal: 'INTERSPEECH', citations: 145, doi: '10.1007/978-3-030-12345-6_18' },
    { id: 20, title: 'Contrastive Learning for Visual Representations', authors: ['Liu Ming', 'Zhao Kai'], abstract: 'Self-supervised contrastive learning for image representation...', year: 2023, journal: 'ICLR', citations: 198, doi: '10.1007/978-3-030-12345-6_19' },
    { id: 21, title: 'Time Series Forecasting with Temporal Fusion Transformers', authors: ['Zhang Wei', 'Li Yang'], abstract: 'Attention-based architecture for multi-horizon forecasting...', year: 2022, journal: 'IJCNN', citations: 87, doi: '10.1007/978-3-030-12345-6_20' },
    { id: 22, title: 'Graph Representation Learning for Social Networks', authors: ['Wang Xi', 'Chen Qing'], abstract: 'Graph neural networks for social network analysis...', year: 2023, journal: 'KDD', citations: 123, doi: '10.1007/978-3-030-12345-6_21' },
    { id: 23, title: 'Efficient Transformers for Long Sequences', authors: ['Zhao Yu', 'Liu Ming'], abstract: 'Linear attention mechanisms for long-sequence modeling...', year: 2023, journal: 'NeurIPS', citations: 165, doi: '10.1007/978-3-030-12345-6_22' },
    { id: 24, title: 'Medical Image Segmentation with U-Net Variants', authors: ['Li Hua', 'Wang Jun'], abstract: 'Advanced U-Net architectures for medical image analysis...', year: 2022, journal: 'MICCAI', citations: 209, doi: '10.1007/978-3-030-12345-6_23' },
    { id: 25, title: 'Robotics with Imitation Learning', authors: ['Chen Hao', 'Zhou Wei'], abstract: 'Learning robot manipulation from human demonstrations...', year: 2023, journal: 'RSS', citations: 94, doi: '10.1007/978-3-030-12345-6_24' }
  ]

  // Filter by query if provided
  let filteredPapers = allPapers
  if (searchQuery && searchQuery.trim()) {
    const lowerQuery = searchQuery.toLowerCase()
    filteredPapers = allPapers.filter(p =>
      p.title.toLowerCase().includes(lowerQuery) ||
      p.abstract.toLowerCase().includes(lowerQuery) ||
      p.authors.some(a => a.toLowerCase().includes(lowerQuery))
    )
  }

  // Calculate pagination
  const total = filteredPapers.length
  const totalPages = Math.ceil(total / limitNum)
  const validPageNum = Math.max(1, Math.min(pageNum, totalPages || 1))
  const startIndex = (validPageNum - 1) * limitNum
  const endIndex = startIndex + limitNum
  const paginatedPapers = filteredPapers.slice(startIndex, endIndex)

  res.json({
    success: true,
    data: {
      papers: paginatedPapers,
      total: total,
      page: validPageNum,
      pageSize: limitNum,
      totalPages: totalPages,
      query: searchQuery,
      duration: Math.floor(Math.random() * 200) + 50
    }
  })
})

// ============================================================================
// 业务端点 - Papers
// ============================================================================

// GET /api/papers/:id
app.get('/api/papers/:id', (req, res) => {
  const { id } = req.params
  console.log('✅ Paper details requested:', id)

  const mockPaper = {
    id: parseInt(id),
    title: 'Deep Learning for Computer Vision: A Comprehensive Review',
    authors: ['Zhang Wei', 'Li Ming', 'Wang Fang'],
    affiliations: ['Tsinghua University', 'Peking University'],
    abstract: 'This paper presents a comprehensive review of deep learning techniques in computer vision, covering convolutional neural networks, recurrent neural networks, and transformer architectures...',
    keywords: ['deep learning', 'computer vision', 'CNN', 'transformer'],
    year: 2023,
    journal: 'IEEE Transactions on Pattern Analysis and Machine Intelligence',
    volume: '45',
    issue: '3',
    pages: '1234-1256',
    citations: 156,
    doi: '10.1109/TPAMI.2023.1234567',
    pdfUrl: 'https://example.com/papers/123.pdf',
    createdAt: '2023-01-15T10:30:00Z'
  }

  res.json({
    success: true,
    data: mockPaper
  })
})

// ============================================================================
// Health Check
// ============================================================================

// Health check handler
const healthHandler = (req, res) => {
  res.json({
    success: true,
    data: {
      status: 'ok',
      message: 'Server is running',
      timestamp: new Date().toISOString()
    }
  })
}

// Both /health and /api/health endpoints
app.get('/health', healthHandler)
app.get('/api/health', healthHandler)

// ============================================================================
// Admin API Endpoints
// ============================================================================

// Middleware to check admin role
const checkAdminRole = (req, res, next) => {
  const authHeader = req.headers.authorization

  if (!authHeader) {
    return res.status(401).json({
      success: false,
      error: 'Unauthorized'
    })
  }

  // Extract user from token (simplified for mock)
  // In real implementation, verify JWT token
  const userId = 2 // Default to superadmin for testing

  const user = users.find(u => u.id === userId)

  if (!user || (user.role !== 'admin' && user.role !== 'superadmin')) {
    return res.status(403).json({
      success: false,
      error: 'Forbidden: Admin access required'
    })
  }

  req.user = user
  next()
}

// Middleware to check superadmin role
const checkSuperAdminRole = (req, res, next) => {
  const authHeader = req.headers.authorization

  if (!authHeader) {
    return res.status(401).json({
      success: false,
      error: 'Unauthorized'
    })
  }

  const userId = 2 // Default to superadmin for testing
  const user = users.find(u => u.id === userId)

  if (!user || user.role !== 'superadmin') {
    return res.status(403).json({
      success: false,
      error: 'Forbidden: Superadmin access required'
    })
  }

  req.user = user
  next()
}

// GET /api/admin/stats - Admin dashboard statistics
app.get('/api/admin/stats', checkAdminRole, (req, res) => {
  const totalUsers = users.length
  const activeUsers = users.filter(u => u.isActive).length
  const adminUsers = users.filter(u => u.role === 'admin').length
  const premiumUsers = users.filter(u => u.role === 'premium').length
  const superadminUsers = users.filter(u => u.role === 'superadmin').length

  res.json({
    success: true,
    data: {
      totalUsers,
      activeUsers,
      adminUsers,
      premiumUsers,
      superadminUsers,
      regularUsers: totalUsers - adminUsers - premiumUsers - superadminUsers,
      totalPapers: 12500,
      totalSearches: 8900,
      recentRegistrations: 5
    }
  })
})

// GET /api/admin/users - List all users with filtering and pagination
app.get('/api/admin/users', checkAdminRole, (req, res) => {
  const { page = 1, limit = 10, search = '', role = '' } = req.query
  const pageNum = parseInt(page)
  const limitNum = parseInt(limit)

  let filteredUsers = users

  // Filter by role
  if (role) {
    filteredUsers = filteredUsers.filter(u => u.role === role)
  }

  // Filter by search
  if (search) {
    const lowerSearch = search.toLowerCase()
    filteredUsers = filteredUsers.filter(u =>
      u.username.toLowerCase().includes(lowerSearch) ||
      u.email.toLowerCase().includes(lowerSearch) ||
      (u.fullName && u.fullName.toLowerCase().includes(lowerSearch))
    )
  }

  const total = filteredUsers.length
  const totalPages = Math.ceil(total / limitNum)
  const startIndex = (pageNum - 1) * limitNum
  const endIndex = startIndex + limitNum
  const paginatedUsers = filteredUsers.slice(startIndex, endIndex)

  res.json({
    success: true,
    data: {
      users: paginatedUsers,
      pagination: {
        page: pageNum,
        limit: limitNum,
        total,
        totalPages
      }
    }
  })
})

// GET /api/admin/users/:id - Get user details
app.get('/api/admin/users/:id', checkAdminRole, (req, res) => {
  const { id } = req.params
  const user = users.find(u => u.id === parseInt(id))

  if (!user) {
    return res.status(404).json({
      success: false,
      error: 'User not found'
    })
  }

  res.json({
    success: true,
    data: user
  })
})

// PUT /api/admin/users/:id - Update user
app.put('/api/admin/users/:id', checkAdminRole, (req, res) => {
  const { id } = req.params
  const { fullName, affiliation, isActive, role } = req.body

  const userIndex = users.findIndex(u => u.id === parseInt(id))

  if (userIndex === -1) {
    return res.status(404).json({
      success: false,
      error: 'User not found'
    })
  }

  const targetUser = users[userIndex]
  const oldValues = { ...targetUser }

  // Permission checks
  if (req.user.role === 'admin' && targetUser.role === 'admin') {
    return res.status(403).json({
      success: false,
      error: 'Admin cannot modify other admin users'
    })
  }

  if (req.user.role === 'admin' && role === 'admin') {
    return res.status(403).json({
      success: false,
      error: 'Admin cannot promote users to admin role'
    })
  }

  if (role === 'superadmin' && req.user.role !== 'superadmin') {
    return res.status(403).json({
      success: false,
      error: 'Only superadmin can assign superadmin role'
    })
  }

  // Apply updates
  if (fullName !== undefined) targetUser.fullName = fullName
  if (affiliation !== undefined) targetUser.affiliation = affiliation
  if (isActive !== undefined) targetUser.isActive = isActive
  if (role !== undefined) targetUser.role = role

  const newValues = { ...targetUser }

  // Log the action
  const logEntry = {
    id: auditLogs.length + 1,
    adminUserId: req.user.id,
    adminUsername: req.user.username,
    targetUserId: targetUser.id,
    action: 'user_updated',
    entityType: 'user',
    entityId: targetUser.id,
    oldValues,
    newValues,
    status: 'success',
    createdAt: new Date().toISOString()
  }
  auditLogs.push(logEntry)

  res.json({
    success: true,
    data: targetUser
  })
})

// DELETE /api/admin/users/:id - Delete user (superadmin only)
app.delete('/api/admin/users/:id', checkSuperAdminRole, (req, res) => {
  const { id } = req.params
  const userId = parseInt(id)

  // Cannot delete self
  if (userId === req.user.id) {
    return res.status(400).json({
      success: false,
      error: 'Cannot delete your own account'
    })
  }

  const userIndex = users.findIndex(u => u.id === userId)

  if (userIndex === -1) {
    return res.status(404).json({
      success: false,
      error: 'User not found'
    })
  }

  const targetUser = users[userIndex]

  // Cannot delete another superadmin
  if (targetUser.role === 'superadmin') {
    return res.status(403).json({
      success: false,
      error: 'Cannot delete superadmin user'
    })
  }

  const oldValues = { ...targetUser }

  // Remove user
  users.splice(userIndex, 1)

  // Log the action
  const logEntry = {
    id: auditLogs.length + 1,
    adminUserId: req.user.id,
    adminUsername: req.user.username,
    targetUserId: userId,
    action: 'user_deleted',
    entityType: 'user',
    entityId: userId,
    oldValues,
    newValues: null,
    status: 'success',
    createdAt: new Date().toISOString()
  }
  auditLogs.push(logEntry)

  res.json({
    success: true,
    message: 'User deleted successfully'
  })
})

// GET /api/admin/audit-logs - Get audit logs (superadmin only)
app.get('/api/admin/audit-logs', checkSuperAdminRole, (req, res) => {
  const { page = 1, limit = 20, action = '', userId = '' } = req.query
  const pageNum = parseInt(page)
  const limitNum = parseInt(limit)

  let filteredLogs = auditLogs

  // Filter by action
  if (action) {
    filteredLogs = filteredLogs.filter(log => log.action === action)
  }

  // Filter by userId (either admin or target)
  if (userId) {
    const uid = parseInt(userId)
    filteredLogs = filteredLogs.filter(log =>
      log.adminUserId === uid || log.targetUserId === uid
    )
  }

  const total = filteredLogs.length
  const totalPages = Math.ceil(total / limitNum)
  const startIndex = (pageNum - 1) * limitNum
  const endIndex = startIndex + limitNum
  const paginatedLogs = filteredLogs.slice(startIndex, endIndex)

  res.json({
    success: true,
    data: {
      logs: paginatedLogs,
      pagination: {
        page: pageNum,
        limit: limitNum,
        total,
        totalPages
      }
    }
  })
})

// POST /api/admin/users/:id/activate - Activate user
app.post('/api/admin/users/:id/activate', checkAdminRole, (req, res) => {
  const { id } = req.params
  const userId = parseInt(id)

  const user = users.find(u => u.id === userId)

  if (!user) {
    return res.status(404).json({
      success: false,
      error: 'User not found'
    })
  }

  if (user.isActive) {
    return res.status(400).json({
      success: false,
      error: 'User is already active'
    })
  }

  // Permission check
  if (req.user.role === 'admin' && user.role === 'admin') {
    return res.status(403).json({
      success: false,
      error: 'Admin cannot activate other admin users'
    })
  }

  const oldValues = { isActive: user.isActive }
  user.isActive = true
  const newValues = { isActive: user.isActive }

  // Log the action
  const logEntry = {
    id: auditLogs.length + 1,
    adminUserId: req.user.id,
    adminUsername: req.user.username,
    targetUserId: user.id,
    action: 'user_activated',
    entityType: 'user',
    entityId: user.id,
    oldValues,
    newValues,
    status: 'success',
    createdAt: new Date().toISOString()
  }
  auditLogs.push(logEntry)

  res.json({
    success: true,
    data: user
  })
})

// POST /api/admin/users/:id/deactivate - Deactivate user
app.post('/api/admin/users/:id/deactivate', checkAdminRole, (req, res) => {
  const { id } = req.params
  const userId = parseInt(id)

  const user = users.find(u => u.id === userId)

  if (!user) {
    return res.status(404).json({
      success: false,
      error: 'User not found'
    })
  }

  if (!user.isActive) {
    return res.status(400).json({
      success: false,
      error: 'User is already inactive'
    })
  }

  // Cannot deactivate self
  if (userId === req.user.id) {
    return res.status(400).json({
      success: false,
      error: 'Cannot deactivate your own account'
    })
  }

  // Permission check
  if (req.user.role === 'admin' && user.role === 'admin') {
    return res.status(403).json({
      success: false,
      error: 'Admin cannot deactivate other admin users'
    })
  }

  if (user.role === 'superadmin') {
    return res.status(403).json({
      success: false,
      error: 'Cannot deactivate superadmin user'
    })
  }

  const oldValues = { isActive: user.isActive }
  user.isActive = false
  const newValues = { isActive: user.isActive }

  // Log the action
  const logEntry = {
    id: auditLogs.length + 1,
    adminUserId: req.user.id,
    adminUsername: req.user.username,
    targetUserId: user.id,
    action: 'user_deactivated',
    entityType: 'user',
    entityId: user.id,
    oldValues,
    newValues,
    status: 'success',
    createdAt: new Date().toISOString()
  }
  auditLogs.push(logEntry)

  res.json({
    success: true,
    data: user
  })
})

// ============================================================================
// 404 处理
// ============================================================================

app.use((req, res) => {
  console.log('⚠️  404 Not Found:', req.method, req.path)

  res.status(404).json({
    success: false,
    error: 'Endpoint not found',
    path: req.path,
    method: req.method,
    availableEndpoints: [
      'POST /api/auth/register',
      'POST /api/auth/login',
      'POST /api/auth/logout',
      'POST /api/auth/refresh',
      'GET /api/auth/me',
      'GET /api/stats/overview',
      'GET /api/search',
      'GET /api/papers/:id',
      'GET /health',
      'GET /api/admin/stats',
      'GET /api/admin/users',
      'GET /api/admin/users/:id',
      'PUT /api/admin/users/:id',
      'DELETE /api/admin/users/:id',
      'GET /api/admin/audit-logs',
      'POST /api/admin/users/:id/activate',
      'POST /api/admin/users/:id/deactivate'
    ]
  })
})

// 启动服务器
app.listen(PORT, () => {
  console.log(`\n========================================`)
  console.log(`PaperCrawler Complete Mock API Server`)
  console.log(`========================================`)
  console.log(`Running on: http://127.0.0.1:${PORT}`)
  console.log(``)
  console.log(`Registered Users:`)
  users.forEach(u => {
    console.log(`  - ${u.email} (${u.username}) [${u.role}]`)
  })
  console.log(``)
  console.log(`Available Endpoints:`)
  console.log(`  📝 Authentication:`)
  console.log(`     POST   /api/auth/register`)
  console.log(`     POST   /api/auth/login`)
  console.log(`     POST   /api/auth/logout`)
  console.log(`     POST   /api/auth/refresh`)
  console.log(`     GET    /api/auth/me`)
  console.log(``)
  console.log(`  📊 Statistics:`)
  console.log(`     GET    /api/stats/overview`)
  console.log(``)
  console.log(`  🔍 Search:`)
  console.log(`     GET    /api/search`)
  console.log(``)
  console.log(`  📄 Papers:`)
  console.log(`     GET    /api/papers/:id`)
  console.log(``)
  console.log(`  👨‍💼 Admin (Admin/Superadmin):`)
  console.log(`     GET    /api/admin/stats`)
  console.log(`     GET    /api/admin/users`)
  console.log(`     GET    /api/admin/users/:id`)
  console.log(`     PUT    /api/admin/users/:id`)
  console.log(`     POST   /api/admin/users/:id/activate`)
  console.log(`     POST   /api/admin/users/:id/deactivate`)
  console.log(``)
  console.log(`  🔐 Superadmin Only:`)
  console.log(`     DELETE /api/admin/users/:id`)
  console.log(`     GET    /api/admin/audit-logs`)
  console.log(``)
  console.log(`  ❤️  Health:`)
  console.log(`     GET    /health`)
  console.log(``)
  console.log(`Test credentials:`)
  console.log(`  User: x2830540584@163.com / Xl1234567890*#`)
  console.log(`  Admin: admin@papercrawler.local / Admin123!`)
  console.log(`  Superadmin: superadmin@papercrawler.local / SuperAdmin123!`)
  console.log(`========================================\n`)
})

module.exports = app
