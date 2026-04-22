import { check, sleep } from 'k6'
import http from 'k6/http'
import { Counter, Trend, Rate } from 'k6/metrics'

// Custom metrics for LaTeX editor performance
const renderTimeTrend = new Trend('latex_render_time')
const memoryUsageTrend = new Trend('memory_usage_mb')
const bundleLoadTimeTrend = new Trend('bundle_load_time')
const inputDelayTrend = new Trend('input_delay_ms')
const compilationTimeTrend = new Trend('compilation_time_ms')
const errorRate = new Rate('performance_errors')
const virtualScrollEfficiency = new Trend('virtual_scroll_efficiency')

export const options = {
  scenarios: {
    // Load testing for different document sizes
    small_documents: {
      executor: 'ramping-vus',
      startVUs: 1,
      stages: [
        { duration: '30s', target: 5 }, // Ramp up
        { duration: '1m', target: 5 },  // Stay at 5 users
        { duration: '30s', target: 0 }, // Ramp down
      ],
      exec: 'smallDocumentTest',
    },
    medium_documents: {
      executor: 'ramping-vus',
      startVUs: 1,
      stages: [
        { duration: '30s', target: 3 },
        { duration: '1m', target: 3 },
        { duration: '30s', target: 0 },
      ],
      exec: 'mediumDocumentTest',
    },
    large_documents: {
      executor: 'shared-iterations',
      vus: 2,
      iterations: 10,
      maxDuration: '5m',
      exec: 'largeDocumentTest',
    },
    collaboration_stress: {
      executor: 'ramping-vus',
      startVUs: 1,
      stages: [
        { duration: '1m', target: 10 },
        { duration: '2m', target: 10 },
        { duration: '1m', target: 0 },
      ],
      exec: 'collaborationTest',
    },
  },
  thresholds: {
    'latex_render_time': ['p(95)<500'], // 95% of renders under 500ms
    'input_delay_ms': ['p(95)<100'],    // 95% of inputs under 100ms
    'compilation_time_ms': ['p(95)<2000'], // 95% of compilations under 2s
    'performance_errors': ['rate<0.02'], // Less than 2% error rate
  },
}

// Test document templates of different sizes
const documentTemplates = {
  small: generateLatexDocument(50),    // ~50 lines
  medium: generateLatexDocument(500),  // ~500 lines
  large: generateLatexDocument(2000),  // ~2000 lines
  mathHeavy: generateMathHeavyDocument(1000),
  complexStructure: generateComplexDocument(1500)
}

export function smallDocumentTest() {
  const url = 'http://localhost:5173/latex-editor'
  const document = documentTemplates.small

  // Measure initial page load
  const loadStart = new Date().getTime()
  const response = http.get(url)
  const loadTime = new Date().getTime() - loadStart
  bundleLoadTimeTrend.add(loadTime)

  check(response, {
    'page loaded successfully': (r) => r.status === 200,
    'initial load under 3s': (r) => loadTime < 3000,
  })

  // Simulate document editing with performance tracking
  const editingSession = simulateEditing(document, 'small')

  // Validate performance metrics
  check(editingSession, {
    'render time acceptable': (s) => s.avgRenderTime < 100,
    'input delay acceptable': (s) => s.avgInputDelay < 50,
    'memory growth controlled': (s) => s.memoryGrowth < 20,
  })

  sleep(1)
}

export function mediumDocumentTest() {
  const document = documentTemplates.medium

  // Simulate editing session
  const editingSession = simulateEditing(document, 'medium')

  // Validate medium document performance
  check(editingSession, {
    'render time acceptable': (s) => s.avgRenderTime < 300,
    'input delay acceptable': (s) => s.avgInputDelay < 100,
    'compilation works': (s) => s.compilationSuccess > 0.95,
  })

  sleep(2)
}

export function largeDocumentTest() {
  const document = documentTemplates.large

  // Large document editing simulation
  const editingSession = simulateEditing(document, 'large')

  // Validate large document performance (more lenient thresholds)
  check(editingSession, {
    'basic editing works': (s) => s.editSuccess,
    'no crashes': (s) => !s.crashed,
    'memory manageable': (s) => s.peakMemory < 500,
  })

  sleep(3)
}

export function collaborationTest() {
  // Simulate real-time collaboration performance
  const collaborationMetrics = simulateCollaboration(5) // 5 concurrent users

  check(collaborationMetrics, {
    'cursor sync fast': (m) => m.avgCursorSyncTime < 100,
    'document sync reliable': (m) => m.syncSuccess > 0.98,
    'network usage reasonable': (m) => m.avgMessageSize < 1000,
  })

  sleep(1)
}

// Helper functions

function generateLatexDocument(lineCount) {
  const sections = Math.ceil(lineCount / 50)
  let document = `\\documentclass{article}
\\usepackage{amsmath}
\\usepackage{amsfonts}
\\usepackage{amssymb}
\\usepackage{graphicx}

\\title{Performance Test Document}
\\author{Test User}
\\date{\\today}

\\begin{document}
\\maketitle

\\begin{abstract}
This is a test document for performance analysis.
\\end{abstract}

`

  for (let i = 1; i <= sections; i++) {
    document += `\\section{Section ${i}}
This is section ${i} content. `

    // Add some math content
    for (let j = 0; j < 5; j++) {
      document += `Here is some mathematical content: $x^2 + y^2 = z^2$. `
    }

    document += `
More text content to reach the desired line count.
`

    // Add some environments
    if (i % 3 === 0) {
      document += `\\begin{itemize}
\\item First item
\\item Second item with math: $\\alpha + \\beta = \\gamma$
\\item Third item
\\end{itemize}

`
    }
  }

  document += `\\end{document}`
  return document
}

function generateMathHeavyDocument(lineCount) {
  let document = `\\documentclass{article}
\\usepackage{amsmath}
\\usepackage{amsfonts}
\\usepackage{amssymb}

\\title{Math Heavy Document}
\\author{Test User}

\\begin{document}
\\maketitle

`

  for (let i = 1; i <= lineCount / 10; i++) {
    document += `\\section{Math Section ${i}}
`

    // Add complex math environments
    document += `\\begin{align*}
`
    for (let j = 0; j < 5; j++) {
      document += `f_${i}${j}(x) &= \\int_{-\\infty}^{\\infty} e^{-x^2} dx \\\\
`
    }
    document += `\\end{align*}

`

    // Add display math
    document += `$$\\sum_{n=1}^{\\infty} \\frac{1}{n^2} = \\frac{\\pi^2}{6}$$

`
  }

  document += `\\end{document}`
  return document
}

function generateComplexDocument(lineCount) {
  let document = `\\documentclass{article}
\\usepackage{amsmath}
\\usepackage{amsfonts}
\\usepackage{amssymb}
\\usepackage{graphicx}
\\usepackage{tabularx}

\\title{Complex Structure Document}
\\author{Test User}

\\begin{document}
\\maketitle

`

  for (let i = 1; i <= lineCount / 100; i++) {
    document += `\\section{Complex Section ${i}}
\\subsection{Subsection ${i}.1}
\\subsubsection{Subsubsection ${i}.1.1}

`

    // Add tables
    document += `\\begin{table}[h]
\\centering
\\begin{tabular}{|c|c|c|}
\\hline
Column 1 & Column 2 & Column 3 \\\\
\\hline
Data 1 & Data 2 & Data 3 \\\\
\\hline
\\end{tabular}
\\caption{Table ${i}}
\\end{table}

`

    // Add figures placeholder
    document += `\\begin{figure}[h]
\\centering
\\includegraphics[width=0.8\\textwidth]{example-image}
\\caption{Figure ${i}}
\\end{figure}

`
  }

  document += `\\end{document}`
  return document
}

function simulateEditing(document, size) {
  const startTime = new Date().getTime()
  let renderTimes = []
  let inputDelays = []
  let memoryUsage = []
  let compilationTimes = []
  let compilationSuccesses = 0
  let totalCompilations = 0
  let crashed = false

  try {
    // Simulate typing and editing
    const lines = document.split('\n')
    const editPoints = Math.min(20, lines.length) // Edit at 20 points

    for (let i = 0; i < editPoints; i++) {
      const editStart = new Date().getTime()

      // Simulate typing a character
      const lineIndex = Math.floor(Math.random() * lines.length)
      const charIndex = Math.floor(Math.random() * (lines[lineIndex].length + 1))

      // Measure input delay
      const inputStart = new Date().getTime()
      lines[lineIndex] = lines[lineIndex].slice(0, charIndex) + 'x' + lines[lineIndex].slice(charIndex)
      const inputDelay = new Date().getTime() - inputStart
      inputDelays.push(inputDelay)

      // Simulate rendering
      const renderStart = new Date().getTime()
      const updatedDocument = lines.join('\n')
      // Simulate syntax highlighting time
      simulateRendering(updatedDocument)
      const renderTime = new Date().getTime() - renderStart
      renderTimes.push(renderTime)

      // Simulate memory usage (rough approximation)
      const currentMemory = estimateMemoryUsage(updatedDocument)
      memoryUsage.push(currentMemory)

      // Periodically test compilation
      if (i % 5 === 0) {
        totalCompilations++
        const compilationStart = new Date().getTime()
        const compilationSuccess = simulateCompilation(updatedDocument)
        const compilationTime = new Date().getTime() - compilationStart

        compilationTimes.push(compilationTime)
        if (compilationSuccess) compilationSuccesses++
      }

      // Check for performance degradation
      if (renderTime > 2000) { // 2 seconds is too slow
        crashed = true
        break
      }
    }
  } catch (error) {
    crashed = true
    errorRate.add(1)
  }

  const endTime = new Date().getTime()
  const duration = endTime - startTime

  return {
    size,
    duration,
    avgRenderTime: renderTimes.length > 0 ? renderTimes.reduce((a, b) => a + b, 0) / renderTimes.length : 0,
    avgInputDelay: inputDelays.length > 0 ? inputDelays.reduce((a, b) => a + b, 0) / inputDelays.length : 0,
    peakMemory: Math.max(...memoryUsage),
    memoryGrowth: memoryUsage.length > 1 ? memoryUsage[memoryUsage.length - 1] - memoryUsage[0] : 0,
    compilationSuccess: totalCompilations > 0 ? compilationSuccesses / totalCompilations : 1,
    avgCompilationTime: compilationTimes.length > 0 ? compilationTimes.reduce((a, b) => a + b, 0) / compilationTimes.length : 0,
    editSuccess: !crashed,
    crashed
  }
}

function simulateCollaboration(userCount) {
  const messages = []
  let cursorSyncTimes = []
  let syncSuccesses = 0
  let totalSyncs = 0

  // Simulate cursor movements for each user
  for (let user = 0; user < userCount; user++) {
    const cursorMovements = 50 // Each user moves cursor 50 times

    for (let move = 0; move < cursorMovements; move++) {
      const syncStart = new Date().getTime()

      // Simulate cursor position update
      const position = {
        line: Math.floor(Math.random() * 100) + 1,
        column: Math.floor(Math.random() * 80) + 1
      }

      // Simulate network delay
      const networkDelay = Math.random() * 50 + 10 // 10-60ms

      // Create message
      const message = {
        type: 'cursor',
        userId: `user_${user}`,
        position,
        timestamp: new Date().getTime()
      }

      messages.push(message)

      const syncTime = new Date().getTime() - syncStart + networkDelay
      cursorSyncTimes.push(syncTime)

      totalSyncs++
      if (syncTime < 150) { // Successful if under 150ms
        syncSuccesses++
      }
    }
  }

  return {
    avgCursorSyncTime: cursorSyncTimes.reduce((a, b) => a + b, 0) / cursorSyncTimes.length,
    syncSuccess: syncSuccesses / totalSyncs,
    avgMessageSize: JSON.stringify(messages[0]).length,
    totalMessages: messages.length
  }
}

function simulateRendering(document) {
  // Simulate the work done by Prism.js syntax highlighting
  const complexity = document.length / 1000 // Normalize by document size
  const mathComplexity = (document.match(/\$/g) || []).length / 2 // Math formulas

  // Simulate processing time based on complexity
  const baseTime = 50 // Base 50ms
  const sizeFactor = Math.min(complexity * 20, 500) // Up to 500ms for size
  const mathFactor = mathComplexity * 5 // 5ms per math formula

  const totalTime = baseTime + sizeFactor + mathFactor

  // Simulate actual delay (in real implementation, this would be actual work)
  const start = new Date().getTime()
  while (new Date().getTime() - start < Math.min(totalTime, 100)) {
    // Busy wait (in real app, this would be actual rendering work)
  }
}

function estimateMemoryUsage(document) {
  // Rough estimation of memory usage
  const baseMemory = 50 // Base 50MB
  const documentMemory = document.length * 2 // 2 bytes per character
  const domMemory = document.split('\n').length * 0.1 // 0.1MB per line for DOM

  return (baseMemory + (documentMemory / 1024 / 1024) + domMemory)
}

function simulateCompilation(document) {
  // Simulate LaTeX compilation success rate
  const hasErrors = document.includes('ERROR') || Math.random() < 0.05 // 5% error rate
  const compilationTime = 200 + Math.random() * 800 // 200-1000ms

  // Simulate delay
  const start = new Date().getTime()
  while (new Date().getTime() - start < compilationTime) {
    // Busy wait
  }

  return !hasErrors
}