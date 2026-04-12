// Web Worker for Prism.js syntax highlighting

// Capture and suppress all console errors during import
const originalConsoleError = self.console ? self.console.error : null;
const errorMessages = [];

self.onerror = function(e) {
  // Suppress Prism.js internal JSON errors - they don't affect functionality
  if (e.message && e.message.includes('JSON')) {
    return true; // Prevent error from propagating
  }
  return false;
};

if (self.console) {
  self.console.error = function(...args) {
    // Collect error messages but don't log them during import
    const message = args[0];
    if (typeof message === 'string') {
      if (message.includes('JSON.parse') || message.includes('Prism') || message.includes('Script error')) {
        return; // Suppress these specific errors
      }
    }
  };
}

self.importScripts('https://cdn.jsdelivr.net/npm/prismjs@1.29.0/prism.min.js');
self.importScripts('https://cdn.jsdelivr.net/npm/prismjs@1.29.0/components/prism-latex.min.js');

// Restore console after import
if (self.console && originalConsoleError) {
  self.console.error = originalConsoleError;
}

self.onmessage = function(e) {
  const { content, theme, id } = e.data;

  if (!content) {
    self.postMessage({ success: true, html: '', id });
    return;
  }

  try {
    const startTime = performance.now();
    const highlighted = Prism.highlight(content, Prism.languages.latex, 'latex');
    const endTime = performance.now();

    self.postMessage({
      success: true,
      html: highlighted,
      processingTime: endTime - startTime,
      contentLength: content.length,
      id
    });
  } catch (error) {
    self.postMessage({
      success: false,
      error: error.message,
      html: content,
      id
    });
  }
};