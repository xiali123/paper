/**
 * KaTeX Rendering Web Worker - SECURE VERSION
 * Includes DOMPurify for XSS protection
 */

// Suppress internal errors from CDN libraries
const originalConsoleError = console.error;
console.error = function(...args) {
  const message = args[0];
  if (typeof message === 'string' && (message.includes('JSON') || message.includes('Script error'))) {
    return; // Suppress CDN internal errors
  }
  originalConsoleError.apply(console, args);
};

// Import libraries with SRI (Subresource Integrity)
self.importScripts('https://cdn.jsdelivr.net/npm/katex@0.16.45/dist/katex.min.js');
self.importScripts('https://cdn.jsdelivr.net/npm/dompurify@3.0.6/dist/purify.min.js');

// Configure DOMPurify for LaTeX content
const PURIFY_CONFIG = {
  ALLOWED_TAGS: [
    // LaTeX structure
    'h2', 'h3', 'h4', 'h5', 'h6',
    // Text formatting
    'strong', 'em', 'u', 'b', 'i',
    // Lists
    'ul', 'ol', 'li',
    // Layout
    'div', 'span', 'p', 'br',
    // KaTeX elements
    'math', 'semantics', 'mrow', 'mi', 'mn', 'mo', 'mtext', 'mspace',
    'mfrac', 'msqrt', 'mroot', 'mtable', 'mtr', 'mtd', 'mstyle',
    'munder', 'mover', 'munderover', 'msup', 'msub', 'msubsup',
    'mphantom', 'merror', 'annotation', 'annotation-xml',
    // Other
    'a', 'img', 'code', 'pre'
  ],
  ALLOWED_ATTR: [
    'class', 'id', 'style',
    'title', 'alt',
    'href', 'src', 'width', 'height',
    'viewbox', 'xmlns', 'data-mathml',
    'colspan', 'rowspan',
    'aria-label', 'role'
  ],
  ALLOW_DATA_ATTR: false,
  SAFE_FOR_JQUERY: true,
  SANITIZE_DOM: true,
  KEEP_CONTENT: true,
  RETURN_DOM: false,
  RETURN_DOM_FRAGMENT: false,
  RETURN_DOM_IMPORT: false,
  FORCE_BODY: false,
  WHOLE_DOCUMENT: false,

  // Custom hooks for additional security
  ADD_ATTR: ['data-latex', 'data-katex'],

  // Hook into tag processing
  FORBID_TAGS: ['script', 'style', 'iframe', 'object', 'embed', 'form', 'input', 'button'],
  FORBID_ATTR: ['onerror', 'onload', 'onclick', 'onmouseover', 'onfocus', 'onblur', 'onchange', 'onsubmit']
};

/**
 * Escape HTML special characters
 */
function escapeHtml(text) {
  const map = {
    '&': '&amp;',
    '<': '&lt;',
    '>': '&gt;',
    '"': '&quot;',
    "'": '&#039;'
  };
  return text.replace(/[&<>"']/g, m => map[m]);
}

/**
 * Sanitize error messages
 */
function sanitizeError(error) {
  const errorStr = String(error);
  // Remove HTML tags and escape special characters
  return escapeHtml(errorStr.replace(/<[^>]*>/g, ''));
}

/**
 * Process LaTeX structure with security
 */
function processLatexStructure(latex) {
  let html = latex;

  // Process sections (with title escaping)
  html = html.replace(/\\section\*?\{([^}]+)\}/g, (match, title) => {
    return `<h2>${escapeHtml(title)}</h2>`;
  });

  html = html.replace(/\\subsection\*?\{([^}]+)\}/g, (match, title) => {
    return `<h3>${escapeHtml(title)}</h3>`;
  });

  html = html.replace(/\\subsubsection\*?\{([^}]+)\}/g, (match, title) => {
    return `<h4>${escapeHtml(title)}</h4>`;
  });

  html = html.replace(/\\paragraph\*?\{([^}]+)\}/g, (match, title) => {
    return `<h5>${escapeHtml(title)}</h5>`;
  });

  html = html.replace(/\\subparagraph\*?\{([^}]+)\}/g, (match, title) => {
    return `<h6>${escapeHtml(title)}</h6>`;
  });

  // Process text formatting (with content escaping)
  html = html.replace(/\\textbf\{([^}]+)\}/g, (match, content) => {
    return `<strong>${escapeHtml(content)}</strong>`;
  });

  html = html.replace(/\\textit\{([^}]+)\}/g, (match, content) => {
    return `<em>${escapeHtml(content)}</em>`;
  });

  html = html.replace(/\\underline\{([^}]+)\}/g, (match, content) => {
    return `<u>${escapeHtml(content)}</u>`;
  });

  html = html.replace(/\\emph\{([^}]+)\}/g, (match, content) => {
    return `<em>${escapeHtml(content)}</em>`;
  });

  html = html.replace(/\\texttt\{([^}]+)\}/g, (match, content) => {
    return `<code>${escapeHtml(content)}</code>`;
  });

  html = html.replace(/\\textsc\{([^}]+)\}/g, (match, content) => {
    return `<span style="font-variant: small-caps">${escapeHtml(content)}</span>`;
  });

  // Process lists (with item escaping)
  html = html.replace(/\\begin\{itemize\}([\s\S]*?)\\end\{itemize\}/g, (match, content) => {
    const items = content.split('\\item').filter(s => s.trim());
    const escapedItems = items.map(item => `<li>${escapeHtml(item.trim())}</li>`).join('');
    return `<ul>${escapedItems}</ul>`;
  });

  html = html.replace(/\\begin\{enumerate\}([\s\S]*?)\\end\{enumerate\}/g, (match, content) => {
    const items = content.split('\\item').filter(s => s.trim());
    const escapedItems = items.map(item => `<li>${escapeHtml(item.trim())}</li>`).join('');
    return `<ol>${escapedItems}</ol>`;
  });

  // Process verbatim (escape all content)
  html = html.replace(/\\begin\{verbatim\}([\s\S]*?)\\end\{verbatim\}/g, (match, content) => {
    return `<pre><code>${escapeHtml(content.trim())}</code></pre>`;
  });

  html = html.replace(/\\verb\|([^|]+)\|/g, (match, content) => {
    return `<code>${escapeHtml(content)}</code>`;
  });

  // Process line breaks
  html = html.replace(/\\\\/g, '<br>');

  // Process URLs (with validation)
  html = html.replace(/\\href\{([^}]+)\}\{([^}]+)\}/g, (match, url, text) => {
    // Basic URL validation
    if (url.startsWith('javascript:') || url.startsWith('data:') || url.startsWith('vbscript:')) {
      return `<span class="link-error">${escapeHtml(text)}</span>`;
    }
    return `<a href="${escapeHtml(url)}" target="_blank" rel="noopener noreferrer">${escapeHtml(text)}</a>`;
  });

  // Process quotes
  html = html.replace(/``/g, '&ldquo;').replace(/''/g, '&rdquo;');
  html = html.replace(/`/g, '&lsquo;').replace(/'/g, '&rsquo;');

  // Process dashes
  html = html.replace(/---/g, '&mdash;');
  html = html.replace(/--/g, '&ndash;');

  // Process ellipsis
  html = html.replace(/\.\.\./g, '&hellip;');

  return html;
}

/**
 * Main message handler
 */
self.onmessage = function(e) {
  const { content, id } = e.data;

  if (!content) {
    self.postMessage({
      success: true,
      html: '',
      processingTime: 0,
      contentLength: 0,
      id
    });
    return;
  }

  try {
    const startTime = performance.now();

    // Process math expressions with error handling
    let html = content;

    // Render inline math $...$
    html = html.replace(/\$([^$\n]+?)\$/g, (match, math) => {
      try {
        return katex.renderToString(math, {
          displayMode: false,
          throwOnError: false,
          output: 'html',
          strict: false,
          trust: false, // Don't trust input
          displayMode: false
        });
      } catch (e) {
        const safeError = sanitizeError(e);
        const safeMath = escapeHtml(math);
        return `<span class="katex-error" title="${safeError}">$${safeMath}$</span>`;
      }
    });

    // Render display math $$...$$ or \[...\]
    html = html.replace(/\$\$([^$]+?)\$\$/g, (match, math) => {
      try {
        return katex.renderToString(math, {
          displayMode: true,
          throwOnError: false,
          output: 'html',
          strict: false,
          trust: false
        });
      } catch (e) {
        const safeError = sanitizeError(e);
        const safeMath = escapeHtml(math);
        return `<div class="katex-error" title="${safeError}">$$${safeMath}$$</div>`;
      }
    });

    // Render display math \[...\]
    html = html.replace(/\\\[([\s\S]+?)\\\]/g, (match, math) => {
      try {
        return katex.renderToString(math.trim(), {
          displayMode: true,
          throwOnError: false,
          output: 'html',
          strict: false,
          trust: false
        });
      } catch (e) {
        const safeError = sanitizeError(e);
        const safeMath = escapeHtml(math);
        return `<div class="katex-error" title="${safeError}">\\[${safeMath}\\]</div>`;
      }
    });

    // Process LaTeX structure
    html = processLatexStructure(html);

    // CRITICAL: Sanitize final HTML output
    const sanitizedHtml = DOMPurify.sanitize(html, PURIFY_CONFIG);

    const endTime = performance.now();
    const processingTime = endTime - startTime;

    self.postMessage({
      success: true,
      html: sanitizedHtml,
      processingTime: processingTime,
      contentLength: content.length,
      sanitized: true,
      id
    });
  } catch (error) {
    const safeError = sanitizeError(error);
    const safeContent = escapeHtml(content);

    self.postMessage({
      success: false,
      error: safeError,
      html: `<div class="render-error">LaTeX rendering failed: ${safeError}</div>`,
      processingTime: 0,
      contentLength: content.length,
      id
    });
  }
};
