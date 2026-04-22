import puppeteer from 'puppeteer';
import fs from 'fs';

class LaTeXEditorPerformanceTester {
  constructor() {
    this.metrics = {
      loadTimes: [],
      renderTimes: [],
      memoryUsage: [],
      inputDelays: [],
      bundleSizes: [],
      compilationTimes: []
    };
    this.browser = null;
    this.page = null;
  }

  async setup() {
    console.log('Setting up performance tests...');
    this.browser = await puppeteer.launch({
      headless: 'new',
      args: [
        '--no-sandbox',
        '--disable-setuid-sandbox',
        '--disable-dev-shm-usage',
        '--disable-accelerated-2d-canvas',
        '--disable-gpu',
        '--window-size=1920,1080'
      ]
    });

    this.page = await this.browser.newPage();

    // Enable performance metrics collection
    await this.page.evaluateOnNewDocument(() => {
      // Override console.log to capture performance logs
      const originalLog = console.log;
      console.log = function(...args) {
        if (args[0] && typeof args[0] === 'string' && args[0].includes('Rendering LaTeX')) {
          window.performanceLogs = window.performanceLogs || [];
          window.performanceLogs.push({
            type: 'latex_render',
            message: args[0],
            timestamp: performance.now()
          });
        }
        originalLog.apply(console, args);
      };
    });

    // Enable tracing for detailed performance data
    await this.page.tracing.start({
      path: 'trace.json',
      categories: ['devtools.timeline', 'v8', 'blink']
    });

    console.log('Setup complete');
  }

  async testPageLoad() {
    console.log('Testing page load performance...');

    // Navigate to LaTeX editor with performance monitoring
    const navigationPromise = this.page.goto('http://localhost:5173/latex-editor', {
      waitUntil: 'networkidle0',
      timeout: 30000
    });

    // Wait for navigation and measure performance
    const loadStart = performance.now();
    await navigationPromise;

    // Get detailed timing information
    const metrics = await this.page.metrics();
    const timing = await this.page.evaluate(() => {
      return JSON.parse(JSON.stringify(window.performance.timing));
    });

    const loadTime = performance.now() - loadStart;

    this.metrics.loadTimes.push({
      loadTime,
      domContentLoaded: timing.domContentLoadedEventEnd - timing.navigationStart,
      firstPaint: await this.page.evaluate(() => {
        const entries = performance.getEntriesByType('paint');
        const fp = entries.find(entry => entry.name === 'first-paint');
        return fp ? fp.startTime : null;
      }),
      firstContentfulPaint: await this.page.evaluate(() => {
        const entries = performance.getEntriesByType('paint');
        const fcp = entries.find(entry => entry.name === 'first-contentful-paint');
        return fcp ? fcp.startTime : null;
      }),
      memoryUsage: metrics.JSHeapUsedSize / 1024 / 1024, // MB
      timestamp: new Date().toISOString()
    });

    console.log(`Page load time: ${loadTime.toFixed(2)}ms`);
    return this.metrics.loadTimes[this.metrics.loadTimes.length - 1];
  }

  async testBundleAnalysis() {
    console.log('Analyzing bundle sizes...');

    // Get network requests to analyze bundle sizes
    const requests = await this.page.evaluate(() => {
      return performance.getEntriesByType('resource')
        .filter(entry => entry.initiatorType === 'script' || entry.name.includes('.js'))
        .map(entry => ({
          name: entry.name,
          size: entry.transferSize || entry.decodedBodySize || 0,
          duration: entry.duration
        }));
    });

    const totalBundleSize = requests.reduce((sum, req) => sum + req.size, 0);

    this.metrics.bundleSizes.push({
      totalSize: totalBundleSize / 1024, // KB
      individualBundles: requests.map(req => ({
        name: req.name.split('/').pop(),
        size: req.size / 1024, // KB
        duration: req.duration
      })),
      timestamp: new Date().toISOString()
    });

    console.log(`Total bundle size: ${(totalBundleSize / 1024).toFixed(2)}KB`);
    return this.metrics.bundleSizes[this.metrics.bundleSizes.length - 1];
  }

  async testEditorRendering(content, label) {
    console.log(`Testing editor rendering with ${label}...`);

    // Focus the editor and insert content
    await this.page.waitForSelector('.latex-textarea');

    const renderStart = await this.page.evaluate(() => {
      return performance.now();
    });

    // Set the content
    await this.page.evaluate((text) => {
      const textarea = document.querySelector('.latex-textarea');
      if (textarea) {
        textarea.value = text;
        // Trigger input event
        const event = new Event('input', { bubbles: true });
        textarea.dispatchEvent(event);
      }
    }, content);

    // Wait for rendering to complete (wait for next tick and any async operations)
    await this.page.waitForTimeout(100);

    const renderEnd = await this.page.evaluate(() => {
      return performance.now();
    });

    const renderTime = renderEnd - renderStart;

    // Get memory usage after rendering
    const metrics = await this.page.metrics();

    this.metrics.renderTimes.push({
      documentType: label,
      contentLength: content.length,
      lineCount: content.split('\n').length,
      renderTime,
      memoryUsed: metrics.JSHeapUsedSize / 1024 / 1024, // MB
      timestamp: new Date().toISOString()
    });

    console.log(`${label} render time: ${renderTime.toFixed(2)}ms`);
    return this.metrics.renderTimes[this.metrics.renderTimes.length - 1];
  }

  async testInputPerformance() {
    console.log('Testing input responsiveness...');

    const testContent = Array(100).fill('Sample text for input testing.').join('\n');
    await this.testEditorRendering(testContent, 'input-test');

    const inputDelays = [];

    // Test typing performance
    for (let i = 0; i < 10; i++) {
      const inputStart = await this.page.evaluate(() => performance.now());

      await this.page.type('.latex-textarea', 'x');

      const inputEnd = await this.page.evaluate(() => performance.now());
      inputDelays.push(inputEnd - inputStart);

      // Small delay between inputs
      await this.page.waitForTimeout(50);
    }

    this.metrics.inputDelays.push({
      avgDelay: inputDelays.reduce((a, b) => a + b, 0) / inputDelays.length,
      maxDelay: Math.max(...inputDelays),
      minDelay: Math.min(...inputDelays),
      sampleSize: inputDelays.length,
      timestamp: new Date().toISOString()
    });

    console.log(`Average input delay: ${this.metrics.inputDelays[this.metrics.inputDelays.length - 1].avgDelay.toFixed(2)}ms`);
    return this.metrics.inputDelays[this.metrics.inputDelays.length - 1];
  }

  async testMemoryUsage() {
    console.log('Testing memory usage over time...');

    const memorySnapshots = [];
    const testDuration = 60000; // 1 minute
    const snapshotInterval = 5000; // Every 5 seconds

    const startTime = Date.now();

    while (Date.now() - startTime < testDuration) {
      const metrics = await this.page.metrics();

      memorySnapshots.push({
        time: Date.now() - startTime,
        used: metrics.JSHeapUsedSize / 1024 / 1024, // MB
        total: metrics.JSHeapTotalSize / 1024 / 1024, // MB
        limit: metrics.JSHeapSizeLimit / 1024 / 1024 // MB
      });

      await this.page.waitForTimeout(snapshotInterval);
    }

    const memoryGrowth = memorySnapshots[memorySnapshots.length - 1].used - memorySnapshots[0].used;

    this.metrics.memoryUsage.push({
      duration: testDuration,
      initialMemory: memorySnapshots[0].used,
      peakMemory: Math.max(...memorySnapshots.map(s => s.used)),
      finalMemory: memorySnapshots[memorySnapshots.length - 1].used,
      memoryGrowth,
      snapshots: memorySnapshots,
      timestamp: new Date().toISOString()
    });

    console.log(`Memory growth over ${testDuration/1000}s: ${memoryGrowth.toFixed(2)}MB`);
    return this.metrics.memoryUsage[this.metrics.memoryUsage.length - 1];
  }

  async testPreviewPerformance(content, label) {
    console.log(`Testing preview performance with ${label}...`);

    // First set the editor content
    await this.testEditorRendering(content, `${label}-preview`);

    // Wait for preview to update
    await this.page.waitForTimeout(1000);

    // Click compile button if available
    try {
      await this.page.click('button:has-text("编译")');
      await this.page.waitForTimeout(2000); // Wait for compilation
    } catch (error) {
      console.log('No compile button found, using live preview');
    }

    // Measure preview rendering time
    const previewStart = await this.page.evaluate(() => performance.now());

    // Wait for any preview updates
    await this.page.waitForTimeout(500);

    const previewEnd = await this.page.evaluate(() => performance.now());
    const previewTime = previewEnd - previewStart;

    this.metrics.compilationTimes.push({
      documentType: label,
      contentLength: content.length,
      previewTime,
      timestamp: new Date().toISOString()
    });

    console.log(`${label} preview time: ${previewTime.toFixed(2)}ms`);
    return this.metrics.compilationTimes[this.metrics.compilationTimes.length - 1];
  }

  generateTestDocuments() {
    return {
      small: `\\documentclass{article}
\\begin{document}
\\section{Introduction}
This is a small document for testing.
\\end{document}`,

      medium: `\\documentclass{article}
\\usepackage{amsmath}
\\begin{document}
\\title{Medium Document}
\\author{Test}
\\maketitle
\\begin{abstract}
This is a medium-sized LaTeX document for performance testing.
\\end{abstract}
\\section{Introduction}
${Array(50).fill('This is a paragraph with some mathematical content: $E = mc^2$ and $\\alpha + \\beta = \\gamma$.').join('\n')}
\\section{Mathematics}
\\begin{align*}
${Array(25).fill('f(x) &= x^2 + 2x + 1 \\\\').join('\n')}
\\end{align*}
\\end{document}`,

      large: `\\documentclass{article}
\\usepackage{amsmath}
\\usepackage{amsfonts}
\\usepackage{amssymb}
\\usepackage{graphicx}
\\begin{document}
\\title{Large Document for Performance Testing}
\\author{Performance Tester}
\\date{\\today}
\\maketitle
\\begin{abstract}
This is a large LaTeX document designed to test performance with substantial content and complex mathematical expressions.
\\end{abstract}
${Array(200).fill('').map((_, i) =>
  `\\section{Section ${i + 1}}
This is section ${i + 1} content. ${Array(10).fill('Here is some text with mathematical expressions: $x^2 + y^2 = z^2$, $\\sin^2\\theta + \\cos^2\\theta = 1$, and $\\int_{-\\infty}^{\\infty} e^{-x^2} dx = \\sqrt{\\pi}$.').join(' ')}
\\begin{align*}
${Array(5).fill('f_' + (i + 1) + '(x) &= \\sum_{n=0}^{\\infty} \\frac{x^n}{n!} \\\\').join('\n')}
\\end{align*}
`).join('\n')
}\\end{document}`,

      mathHeavy: `\\documentclass{article}
\\usepackage{amsmath}
\\usepackage{amsfonts}
\\usepackage{amssymb}
\\begin{document}
\\title{Math Heavy Document}
\\author{Math Lover}
\\maketitle
${Array(100).fill('').map((_, i) =>
  `\\section{Complex Math Section ${i + 1}}
\\begin{align*}
${Array(10).fill('').map((_, j) =>
  `\\int_{\\Omega} \\nabla \\cdot \\mathbf{F} \\, dV &= \\oint_{\\partial \\Omega} \\mathbf{F} \\cdot d\\mathbf{S} \\\\
\\sum_{n=1}^{\\infty} \\frac{1}{n^s} &= \\prod_{p \\text{ prime}} \\left(1 - \\frac{1}{p^s}\\right)^{-1} \\\\
\\mathcal{L}\\{f(t)\\} &= \\int_0^{\\infty} e^{-st} f(t) \\, dt \\\\
\\det\\begin{pmatrix} a & b \\\\ c & d \\end{pmatrix} &= ad - bc
`).join('\n')}
\\end{align*}
\\begin{equation*}
${Array(15).fill('\\mathcal{H}\\,\\psi = E\\psi').join('\n')}
\\end{equation*}
`).join('\n')
}\\end{document}`
    };
  }

  async runFullTestSuite() {
    console.log('Starting comprehensive LaTeX editor performance test...');

    try {
      await this.setup();

      // Load the page
      await this.testPageLoad();

      // Analyze bundle sizes
      await this.testBundleAnalysis();

      // Generate test documents
      const documents = this.generateTestDocuments();

      // Test rendering with different document sizes
      await this.testEditorRendering(documents.small, 'small');
      await this.testEditorRendering(documents.medium, 'medium');
      await this.testEditorRendering(documents.large, 'large');
      await this.testEditorRendering(documents.mathHeavy, 'math-heavy');

      // Test input performance
      await this.testInputPerformance();

      // Test preview performance
      await this.testPreviewPerformance(documents.medium, 'medium-preview');
      await this.testPreviewPerformance(documents.mathHeavy, 'math-preview');

      // Test memory usage (shorter test for demo)
      console.log('Running memory test (30 seconds)...');
      await this.testMemoryUsage();

      // Generate report
      await this.generateReport();

    } catch (error) {
      console.error('Test failed:', error);
      throw error;
    } finally {
      if (this.browser) {
        await this.page.tracing.stop();
        await this.browser.close();
      }
    }
  }

  async generateReport() {
    console.log('Generating performance report...');

    const report = {
      timestamp: new Date().toISOString(),
      summary: {
        totalLoadTime: this.metrics.loadTimes.reduce((sum, m) => sum + m.loadTime, 0) / this.metrics.loadTimes.length,
        avgRenderTime: this.metrics.renderTimes.reduce((sum, m) => sum + m.renderTime, 0) / this.metrics.renderTimes.length,
        avgInputDelay: this.metrics.inputDelays.reduce((sum, m) => sum + m.avgDelay, 0) / this.metrics.inputDelays.length,
        peakMemory: Math.max(...this.metrics.memoryUsage.map(m => m.peakMemory)),
        totalBundleSize: this.metrics.bundleSizes[0]?.totalSize || 0
      },
      detailed: this.metrics
    };

    // Save report to file
    fs.writeFileSync('performance-report.json', JSON.stringify(report, null, 2));

    // Generate human-readable summary
    const summary = `
=== LaTeX Editor Performance Report ===
Generated: ${report.timestamp}

📊 SUMMARY METRICS:
• Average Load Time: ${report.summary.totalLoadTime.toFixed(2)}ms
• Average Render Time: ${report.summary.avgRenderTime.toFixed(2)}ms
• Average Input Delay: ${report.summary.avgInputDelay.toFixed(2)}ms
• Peak Memory Usage: ${report.summary.peakMemory.toFixed(2)}MB
• Total Bundle Size: ${report.summary.totalBundleSize.toFixed(2)}KB

🎯 PERFORMANCE RECOMMENDATIONS:
${this.generateRecommendations(report.summary)}

📈 DETAILED RESULTS:
${JSON.stringify(report.detailed, null, 2)}
`;

    fs.writeFileSync('performance-summary.txt', summary);
    console.log('Performance report saved to performance-summary.txt');

    return report;
  }

  generateRecommendations(summary) {
    const recommendations = [];

    if (summary.avgInputDelay > 100) {
      recommendations.push('• CRITICAL: Input delay exceeds 100ms - Implement virtual scrolling and debounced updates');
    }

    if (summary.avgRenderTime > 300) {
      recommendations.push('• CRITICAL: Render time too high - Move syntax highlighting to Web Workers');
    }

    if (summary.peakMemory > 200) {
      recommendations.push('• HIGH: Memory usage too high - Fix memory leaks in preview component');
    }

    if (summary.totalBundleSize > 1500) {
      recommendations.push('• MEDIUM: Bundle size too large - Implement code splitting and lazy loading');
    }

    if (summary.totalLoadTime > 3000) {
      recommendations.push('• MEDIUM: Load time too slow - Optimize critical rendering path');
    }

    if (recommendations.length === 0) {
      recommendations.push('• Performance looks good! Consider monitoring for regression.');
    }

    return recommendations.join('\n');
  }
}

// Run tests if this file is executed directly
if (import.meta.url === `file://${process.argv[1]}`) {
  const tester = new LaTeXEditorPerformanceTester();
  tester.runFullTestSuite()
    .then(() => {
      console.log('Performance testing completed successfully!');
      process.exit(0);
    })
    .catch((error) => {
      console.error('Performance testing failed:', error);
      process.exit(1);
    });
}

export default LaTeXEditorPerformanceTester;