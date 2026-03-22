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
const users = [{
  id: 1,
  username: 'S221000789',
  email: 'x2830540584@163.com',
  password: 'Xl1234567890*#',
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

  const user = users.find(u => u.email === email)

  if (!user) {
    console.log('❌ Login failed: User not found')
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
      'GET /health'
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
    console.log(`  - ${u.email} (${u.username})`)
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
  console.log(`  ❤️  Health:`)
  console.log(`     GET    /health`)
  console.log(``)
  console.log(`Test credentials:`)
  console.log(`  Email: x2830540584@163.com`)
  console.log(`  Password: Xl1234567890*#`)
  console.log(`========================================\n`)
})

module.exports = app
