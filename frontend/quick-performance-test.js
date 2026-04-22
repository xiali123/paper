import puppeteer from 'puppeteer';
import fs from 'fs';

async function quickPerformanceTest() {
  console.log('Running quick performance analysis...');

  const browser = await puppeteer.launch({ headless: 'new' });
  const page = await browser.newPage();

  // Collect performance metrics
  const metrics = {
    loadTime: 0,
    bundleSizes: [],
    memoryUsage: 0,
    timestamp: new Date().toISOString()
  };

  try {
    // Navigate and measure load time
    const loadStart = Date.now();
    await page.goto('http://localhost:5173/latex-editor', { waitUntil: 'networkidle0' });
    metrics.loadTime = Date.now() - loadStart;

    // Get bundle analysis
    const resources = await page.evaluate(() => {
      return performance.getEntriesByType('resource')
        .filter(entry => entry.name.includes('.js') || entry.name.includes('.css'))
        .map(entry => ({
          name: entry.name.split('/').pop(),
          size: (entry.transferSize || entry.decodedBodySize || 0) / 1024,
          duration: entry.duration
        }));
    });

    metrics.bundleSizes = resources;
    metrics.totalBundleSize = resources.reduce((sum, r) => sum + r.size, 0);

    // Get memory usage
    const browserMetrics = await page.metrics();
    metrics.memoryUsage = browserMetrics.JSHeapUsedSize / 1024 / 1024;

    // Get paint timings
    const paintMetrics = await page.evaluate(() => {
      const entries = performance.getEntriesByType('paint');
      return {
        firstPaint: entries.find(e => e.name === 'first-paint')?.startTime || 0,
        firstContentfulPaint: entries.find(e => e.name === 'first-contentful-paint')?.startTime || 0
      };
    });

    metrics.paintMetrics = paintMetrics;

    console.log(`Load time: ${metrics.loadTime}ms`);
    console.log(`Total bundle size: ${metrics.totalBundleSize.toFixed(2)}KB`);
    console.log(`Memory usage: ${metrics.memoryUsage.toFixed(2)}MB`);
    console.log(`First Paint: ${paintMetrics.firstPaint.toFixed(2)}ms`);
    console.log(`First Contentful Paint: ${paintMetrics.firstContentfulPaint.toFixed(2)}ms`);

    // Generate quick report
    const report = `
=== QUICK PERFORMANCE ANALYSIS ===
Timestamp: ${metrics.timestamp}

📊 CORE METRICS:
• Page Load Time: ${metrics.loadTime}ms
• Total Bundle Size: ${metrics.totalBundleSize.toFixed(2)}KB (${(metrics.totalBundleSize/1024).toFixed(2)}MB)
• Memory Usage: ${metrics.memoryUsage.toFixed(2)}MB
• First Paint: ${paintMetrics.firstPaint.toFixed(2)}ms
• First Contentful Paint: ${paintMetrics.firstContentfulPaint.toFixed(2)}ms

📦 BUNDLE BREAKDOWN:
${resources.map(r => `• ${r.name}: ${r.size.toFixed(2)}KB (${r.duration.toFixed(2)}ms)`).join('\n')}

🎯 CRITICAL ISSUES:
${generateQuickRecommendations(metrics)}
`;

    fs.writeFileSync('quick-performance-report.txt', report);
    console.log('Quick report saved to quick-performance-report.txt');

  } catch (error) {
    console.error('Performance test error:', error);
  } finally {
    await browser.close();
  }

  return metrics;
}

function generateQuickRecommendations(metrics) {
  const issues = [];

  if (metrics.loadTime > 3000) {
    issues.push('• CRITICAL: Load time >3s - Poor user experience');
  }

  if (metrics.totalBundleSize > 3000) {
    issues.push('• CRITICAL: Bundle size >3MB - Very slow loading');
  }

  if (metrics.memoryUsage > 100) {
    issues.push('• HIGH: Memory usage >100MB - Potential memory leaks');
  }

  if (metrics.paintMetrics.firstContentfulPaint > 2500) {
    issues.push('• HIGH: FCP >2.5s - Content loads too slowly');
  }

  return issues.length > 0 ? issues.join('\n') : '• No critical issues found';
}

// Run the test
quickPerformanceTest().then(() => {
  console.log('Quick performance test completed!');
});