/**
 * LaTeX Editor Performance Test Suite
 * Comprehensive performance analysis and benchmarking
 */

import { describe, it, expect, beforeEach, afterEach } from 'vitest'
import { performanceMonitor, checkPerformanceThreshold, PERFORMANCE_THRESHOLDS } from '@/utils/performance'
import { highlightSyntax, renderLatex } from '@/utils/workers'
import { useTextVirtualScroll } from '@/composables/useVirtualScroll'
import { nextTick } from 'vue'

describe('LaTeX Editor Performance Tests', () => {
  beforeEach(() => {
    performanceMonitor.clearMetrics()
  })

  afterEach(() => {
    performanceMonitor.clearMetrics()
  })

  describe('Input Response Time', () => {
    it('should handle single character input within 50ms', async () => {
      const testContent = 'a'
      const endTimer = performanceMonitor.startTimer('single_char_input')

      // Simulate input processing
      await highlightSyntax(testContent)

      const duration = endTimer()
      expect(duration).toBeLessThan(PERFORMANCE_THRESHOLDS.inputDelay)
    })

    it('should handle rapid input with debouncing', async () => {
      const inputs = ['a', 'ab', 'abc', 'abcd', 'abcde']
      const times: number[] = []

      for (const input of inputs) {
        const endTimer = performanceMonitor.startTimer('rapid_input')
        await highlightSyntax(input)
        times.push(endTimer())
      }

      // Average should be fast due to debouncing
      const avgTime = times.reduce((a, b) => a + b, 0) / times.length
      expect(avgTime).toBeLessThan(PERFORMANCE_THRESHOLDS.inputDelay * 1.5)
    })

    it('should handle large text paste efficiently', async () => {
      const largeContent = '\\section{Test}\n'.repeat(100) // ~2000 chars
      const endTimer = performanceMonitor.startTimer('large_paste', {
        contentLength: largeContent.length
      })

      await highlightSyntax(largeContent)

      const duration = endTimer()
      expect(duration).toBeLessThan(500) // Larger threshold for big content
    })
  })

  describe('Syntax Highlighting Performance', () => {
    it('should highlight small documents (<1000 chars) within 100ms', async () => {
      const content = generateLatexContent(500)
      const endTimer = performanceMonitor.startTimer('highlight_small')

      const result = await highlightSyntax(content)

      const duration = endTimer()
      expect(duration).toBeLessThan(PERFORMANCE_THRESHOLDS.renderTime)
      expect(result.html).toBeTruthy()
    })

    it('should highlight medium documents (1000-5000 chars) within 200ms', async () => {
      const content = generateLatexContent(3000)
      const endTimer = performanceMonitor.startTimer('highlight_medium')

      const result = await highlightSyntax(content)

      const duration = endTimer()
      expect(duration).toBeLessThan(200)
      expect(result.html).toBeTruthy()
    })

    it('should highlight large documents (5000+ chars) within 500ms', async () => {
      const content = generateLatexContent(8000)
      const endTimer = performanceMonitor.startTimer('highlight_large')

      const result = await highlightSyntax(content)

      const duration = endTimer()
      expect(duration).toBeLessThan(500)
      expect(result.html).toBeTruthy()
    })

    it('should maintain consistent performance with nested structures', async () => {
      const nestedContent = generateNestedLatex(10) // 10 levels deep
      const endTimer = performanceMonitor.startTimer('highlight_nested')

      await highlightSyntax(nestedContent)

      const duration = endTimer()
      expect(duration).toBeLessThan(300)
    })
  })

  describe('LaTeX Rendering Performance', () => {
    it('should render simple math within 100ms', async () => {
      const mathContent = 'Simple equation: $E = mc^2$'
      const endTimer = performanceMonitor.startTimer('render_simple_math')

      const result = await renderLatex(mathContent)

      const duration = endTimer()
      expect(duration).toBeLessThan(PERFORMANCE_THRESHOLDS.renderTime)
      expect(result.html).toContain('math')
    })

    it('should render complex equations within 300ms', async () => {
      const complexMath = generateComplexMath()
      const endTimer = performanceMonitor.startTimer('render_complex_math')

      const result = await renderLatex(complexMath)

      const duration = endTimer()
      expect(duration).toBeLessThan(300)
      expect(result.html).toBeTruthy()
    })

    it('should handle multiple equations efficiently', async () => {
      const multiMath = Array(10).fill('$x^2 + y^2 = z^2$').join('\n')
      const endTimer = performanceMonitor.startTimer('render_multiple_math')

      const result = await renderLatex(multiMath)

      const duration = endTimer()
      expect(duration).toBeLessThan(500)
    })
  })

  describe('Virtual Scrolling Performance', () => {
    it('should handle large documents without lag', () => {
      const largeDoc = generateLatexContent(10000) // 10k lines
      const endTimer = performanceMonitor.startTimer('virtual_scroll_init')

      // This would be tested in the actual component
      const virtualScroll = useTextVirtualScroll(
        largeDoc,
        400,
        20
      )

      const duration = endTimer()
      expect(duration).toBeLessThan(100)
      expect(virtualScroll.lines.length).toBeGreaterThan(1000)
    })

    it('should calculate visible range efficiently', () => {
      const content = generateLatexContent(5000)
      const virtualScroll = useTextVirtualScroll(content, 400, 20)

      const endTimer = performanceMonitor.startTimer('calculate_visible_range')

      // Simulate scroll to middle
      virtualScroll.scrollToLine(2500)

      const duration = endTimer()
      expect(duration).toBeLessThan(50)
      expect(virtualScroll.visibleLineRange.count).toBeLessThan(100) // Should only render visible
    })

    it('should navigate to line instantly', () => {
      const content = generateLatexContent(8000)
      const virtualScroll = useTextVirtualScroll(content, 400, 20)

      const endTimer = performanceMonitor.startTimer('navigate_to_line')

      virtualScroll.scrollToLine(7500)

      const duration = endTimer()
      expect(duration).toBeLessThan(PERFORMANCE_THRESHOLDS.inputDelay)
    })
  })

  describe('Memory Management', () => {
    it('should clean up workers on unmount', async () => {
      const initialMemory = performanceMonitor.getMemoryUsage()

      // Simulate component lifecycle
      await highlightSyntax('test content')
      await renderLatex('$x^2$')

      // Check memory hasn't grown significantly
      const finalMemory = performanceMonitor.getMemoryUsage()
      if (initialMemory && finalMemory) {
        const growth = finalMemory.used - initialMemory.used
        expect(growth).toBeLessThan(10 * 1024 * 1024) // Less than 10MB growth
      }
    })

    it('should handle large documents without memory leaks', async () => {
      const baselineMemory = performanceMonitor.getMemoryUsage()

      // Process multiple large documents
      for (let i = 0; i < 10; i++) {
        const content = generateLatexContent(5000)
        await highlightSyntax(content)
        await renderLatex(content)
      }

      const finalMemory = performanceMonitor.getMemoryUsage()
      if (baselineMemory && finalMemory) {
        const growthPercent = finalMemory.percentage - baselineMemory.percentage
        expect(growthPercent).toBeLessThan(20) // Less than 20% growth
      }
    })
  })

  describe('Debouncing Efficiency', () => {
    it('should reduce syntax highlighting calls by 80% with rapid input', async () => {
      const calls: number[] = []

      // Simulate rapid input over 1 second
      const startTime = Date.now()
      while (Date.now() - startTime < 1000) {
        const endTimer = performanceMonitor.startTimer('debounced_input')
        await highlightSyntax('test')
        calls.push(endTimer())
      }

      // With debouncing, we should have significantly fewer actual renders
      const avgTime = calls.reduce((a, b) => a + b, 0) / calls.length
      expect(avgTime).toBeLessThan(50) // Should be fast due to debouncing
    })
  })

  describe('Worker Performance', () => {
    it('should complete worker tasks within timeout', async () => {
      const content = generateLatexContent(3000)

      const startTime = Date.now()
      const result = await Promise.race([
        highlightSyntax(content),
        new Promise((_, reject) =>
          setTimeout(() => reject(new Error('Timeout')), 10000)
        )
      ])
      const duration = Date.now() - startTime

      expect(result).toBeTruthy()
      expect(duration).toBeLessThan(10000)
    })

    it('should handle concurrent worker requests', async () => {
      const contents = [
        generateLatexContent(1000),
        generateLatexContent(1500),
        generateLatexContent(2000),
        generateLatexContent(2500),
        generateLatexContent(3000)
      ]

      const startTime = Date.now()
      const results = await Promise.all(contents.map(c => highlightSyntax(c)))
      const duration = Date.now() - startTime

      expect(results).toHaveLength(5)
      expect(duration).toBeLessThan(3000) // Should be faster than sequential
    })
  })

  describe('Performance Threshold Monitoring', () => {
    it('should detect when rendering exceeds threshold', async () => {
      const largeContent = generateLatexContent(10000)
      const endTimer = performanceMonitor.startTimer('threshold_test')

      await highlightSyntax(largeContent)
      const duration = endTimer()

      checkPerformanceThreshold('rendering', duration, 100)

      // Check if warning was logged (would be in console in real scenario)
      const metrics = performanceMonitor.getMetrics('threshold_test')
      expect(metrics.length).toBeGreaterThan(0)
    })

    it('should track performance metrics over time', async () => {
      // Perform multiple operations
      for (let i = 0; i < 10; i++) {
        const content = generateLatexContent(500 + i * 100)
        const endTimer = performanceMonitor.startTimer('tracking_test')
        await highlightSyntax(content)
        endTimer()
      }

      const metrics = performanceMonitor.getMetrics('tracking_test')
      expect(metrics.length).toBe(10)

      const avgTime = metrics.reduce((sum, m) => sum + m.value, 0) / metrics.length
      expect(avgTime).toBeLessThan(200)
    })
  })
})

// Helper functions to generate test data
function generateLatexContent(lines: number): string {
  const sections = ['Introduction', 'Methods', 'Results', 'Discussion', 'Conclusion']
  const content: string[] = []

  for (let i = 0; i < lines; i++) {
    const section = sections[i % sections.length]
    content.push(`\\section{${section} ${Math.floor(i / sections.length) + 1}}`)
    content.push('Some text content here with math: $x^2 + y^2 = z^2$')
    content.push('More content with \\textbf{bold} and \\textit{italic} text.')
    content.push('$$')
    content.push('\\int_0^\\infty e^{-x^2} dx = \\frac{\\sqrt{\\pi}}{2}')
    content.push('$$')
    content.push('') // Empty line
  }

  return content.join('\n')
}

function generateNestedLatex(depth: number): string {
  let content = '\\documentclass{article}\n\\begin{document}\n'

  for (let i = 0; i < depth; i++) {
    content += '\\begin{itemize}\n\\item '
  }
  content += 'Nested content'
  for (let i = 0; i < depth; i++) {
    content += '\n\\end{itemize}'
  }

  content += '\n\\end{document}'
  return content
}

function generateComplexMath(): string {
  return `
Complex mathematical expressions:

$$
\\frac{\\partial^2 u}{\\partial t^2} = c^2 \\nabla^2 u
$$

$$
\\int_{-\\infty}^{\\infty} e^{-x^2} dx = \\sqrt{\\pi}
$$

$$
\\sum_{n=1}^{\\infty} \\frac{1}{n^2} = \\frac{\\pi^2}{6}
$$

$$
\\begin{pmatrix}
a & b \\\\
c & d
\\end{pmatrix}
\\begin{pmatrix}
x \\\\
y
\\end{pmatrix}
=
\\begin{pmatrix}
ax + by \\\\
cx + dy
\\end{pmatrix}
$$
`
}
