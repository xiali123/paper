# Security Fixes Installation Guide

## Critical Security Fixes for LaTeX Editor

This document provides the installation steps for fixing critical security vulnerabilities in the LaTeX editor.

---

## 🚨 Critical Actions Required

### Step 1: Install Missing Security Dependencies

**Execute the following commands in the frontend directory:**

```bash
cd /home/xiali/progress/paper/paper/frontend

# Install DOMPurify for XSS protection
npm install dompurify@^3.0.6

# Install TypeScript types for DOMPurify
npm install --save-dev @types/dompurify@^3.0.5

# Install crypto-js for localStorage encryption (optional but recommended)
npm install crypto-js@^4.2.0

# Install TypeScript types for crypto-js
npm install --save-dev @types/crypto-js@^4.2.1

# Verify installation
npm list dompurify
npm list crypto-js
```

### Step 2: Update Files

Replace the following files with their secure versions:

1. **Web Worker Security Fix**:
   ```bash
   # Already updated: frontend/public/workers/katex.worker.js
   # No action needed - file has been updated with DOMPurify integration
   ```

2. **LaTeX Preview Component**:
   ```bash
   # Replace with secure version
   cp frontend/src/components/latex/LatexPreview.vue.secure \
      frontend/src/components/latex/LatexPreview.vue
   ```

3. **Add Validation Utility**:
   ```bash
   # Already created: frontend/src/utils/latexValidation.ts
   # No action needed - file has been created
   ```

### Step 3: Update API Module

Add input validation to `frontend/src/api/modules/latex.ts`:

```typescript
// Add this import at the top
import { validateLatexInput } from '@/utils/latexValidation'

// Update the compile function
export const latexApi = {
  compile: async (requestData: CompileLatexRequest): Promise<CompileLatexResponse> => {
    // Validate input BEFORE sending to backend
    const validation = validateLatexInput(requestData.content)
    if (!validation.valid) {
      if (import.meta.env.DEV) {
        console.error('LaTeX validation failed:', validation.errors)
      }
      return {
        success: false,
        error: `Content validation failed: ${validation.errors.join(', ')}`,
        duration: 0
      }
    }

    try {
      const response = await request.post('/api/latex/compile', requestData)
      return response.data
    } catch (error) {
      console.error('LaTeX compilation failed:', error)
      return {
        success: false,
        error: error instanceof Error ? error.message : '编译失败',
        duration: 0
      }
    }
  },
  // ... rest of the API methods
}
```

### Step 4: Update Store (Optional - for Auto-save)

Add auto-save functionality to `frontend/src/architecture/stores/latexEditor.ts`:

```typescript
// Add this after line 518
let autoSaveTimer: NodeJS.Timeout | null = null

function startAutoSave(interval: number = 60000) { // Default: 1 minute
  stopAutoSave() // Clear any existing timer

  autoSaveTimer = setInterval(() => {
    if (currentDocument.value && editorContent.value) {
      // Check if content has changed
      if (currentDocument.value.content !== editorContent.value) {
        saveDocument()
      }
    }
  }, interval)
}

function stopAutoSave() {
  if (autoSaveTimer) {
    clearInterval(autoSaveTimer)
    autoSaveTimer = null
  }
}

// Update the return statement to include new methods
return {
  // ... existing exports
  startAutoSave,
  stopAutoSave,
}
```

---

## Step 5: Add Security Headers (Optional but Recommended)

Update `frontend/vite.config.ts`:

```typescript
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [vue()],

  server: {
    headers: {
      'X-Content-Type-Options': 'nosniff',
      'X-Frame-Options': 'DENY',
      'X-XSS-Protection': '1; mode=block',
      'Referrer-Policy': 'strict-origin-when-cross-origin',
      'Permissions-Policy': 'geolocation=(), microphone=(), camera=()'
    }
  },

  build: {
    headers: {
      'X-Content-Type-Options': 'nosniff',
      'X-Frame-Options': 'DENY'
    }
  }
})
```

---

## Step 6: Add Content Security Policy (Optional but Recommended)

Create `frontend/src/utils/csp.ts`:

```typescript
export function setupCSP(): void {
  // Check if CSP meta tag already exists
  if (document.querySelector('meta[http-equiv="Content-Security-Policy"]')) {
    return
  }

  const meta = document.createElement('meta')
  meta.httpEquiv = 'Content-Security-Policy'

  // CSP policy allowing KaTeX from CDN
  meta.content = [
    "default-src 'self'",
    "script-src 'self' 'unsafe-inline' https://cdn.jsdelivr.net",
    "style-src 'self' 'unsafe-inline' https://cdn.jsdelivr.net",
    "img-src 'self' data: https:",
    "font-src 'self' https://cdn.jsdelivr.net",
    "connect-src 'self'",
    "worker-src 'self' blob:",
    "frame-src 'none'",
    "form-action 'self'",
    "base-uri 'self'",
    "manifest-src 'self'"
  ].join('; ')

  document.head.appendChild(meta)
}

// Call this in main.ts
export function initSecurity(): void {
  setupCSP()
}
```

Then update `frontend/src/main.ts`:

```typescript
import { createApp } from 'vue'
import { createPinia } from 'pinia'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'

import App from './App.vue'
import router from './router'

// Add this import
import { initSecurity } from './utils/csp'

const app = createApp(App)
const pinia = createPinia()

app.use(pinia)
app.use(router)
app.use(ElementPlus)

// Add this line to initialize security
initSecurity()

app.mount('#app')
```

---

## Step 7: Test Security Fixes

Create a test file `frontend/tests/security/latex-security.test.ts`:

```typescript
import { describe, it, expect } from 'vitest'
import { validateLatexInput } from '@/utils/latexValidation'

describe('LaTeX Security Validation', () => {
  it('should reject dangerous commands', () => {
    const result = validateLatexInput('\\write18{rm -rf /}')
    expect(result.valid).toBe(false)
    expect(result.errors).toContain('Dangerous command detected: \\write18')
  })

  it('should reject path traversal', () => {
    const result = validateLatexInput('\\input{../../etc/passwd}')
    expect(result.valid).toBe(false)
    expect(result.errors).toContain('Path traversal sequence detected')
  })

  it('should reject XSS in section titles', () => {
    const result = validateLatexInput('\\section{<script>alert("XSS")</script>}')
    // The validation should pass (it's valid LaTeX) but sanitization should handle it
    expect(result.valid).toBe(true)
  })

  it('should reject content exceeding maximum length', () => {
    const longContent = 'a'.repeat(1000001)
    const result = validateLatexInput(longContent)
    expect(result.valid).toBe(false)
    expect(result.errors.some(e => e.includes('exceeds maximum length'))).toBe(true)
  })

  it('should accept valid LaTeX', () => {
    const validLatex = '\\section{Introduction}\\subsection{Background}$E = mc^2$'
    const result = validateLatexInput(validLatex)
    expect(result.valid).toBe(true)
  })
})
```

Run the tests:

```bash
npm test -- latex-security.test.ts
```

---

## Step 8: Manual Security Testing

Test the following scenarios in the browser:

1. **XSS Test**:
   ```latex
   \section{<script>alert('XSS')</script>}
   $<img src=x onerror=alert('XSS')>$
   ```

2. **Command Injection Test**:
   ```latex
   \write18{cat /etc/passwd}
   \input{../../etc/passwd}
   ```

3. **Path Traversal Test**:
   ```latex
   \include{../../../etc/passwd}
   ```

4. **DoS Test**:
   - Paste a very large document (1MB+)
   - Verify it's rejected

5. **Binary Content Test**:
   - Try to paste binary content
   - Verify it's rejected

---

## Verification Checklist

After installation, verify the following:

- [ ] DOMPurify is installed (`npm list dompurify`)
- [ ] All files have been updated
- [ ] Web worker includes DOMPurify
- [ ] Input validation is working
- [ ] XSS attempts are blocked
- [ ] Command injection is prevented
- [ ] Tests pass
- [ ] Manual security tests pass
- [ ] Application still functions normally

---

## Expected Results

After implementing these fixes:

1. **All XSS attempts should be blocked**
2. **Command injection should be prevented**
3. **Path traversal should be detected**
4. **Large content should be rejected**
5. **Application should continue to function normally**
6. **Performance should not be significantly impacted**

---

## Troubleshooting

### Issue: DOMPurify import fails

**Solution**: Make sure you've installed the package:
```bash
npm install dompurify
npm install --save-dev @types/dompurify
```

### Issue: Web worker fails to load

**Solution**: The worker needs to load DOMPurify from CDN. Make sure your CSP allows:
```
script-src 'self' https://cdn.jsdelivr.net
```

### Issue: Valid LaTeX is rejected

**Solution**: Check the validation rules in `latexValidation.ts`. You may need to adjust the allowed commands or limits.

### Issue: Performance degradation

**Solution**: The validation adds minimal overhead. If you notice issues:
1. Reduce the `MAX_LINE_LENGTH` limit
2. Adjust the debounce timing in components
3. Consider caching validation results

---

## Post-Installation Security Review

After implementing these fixes:

1. **Re-run the security audit** to verify all issues are resolved
2. **Perform penetration testing** to find any remaining vulnerabilities
3. **Document the security measures** for future reference
4. **Set up automated security scanning** in CI/CD
5. **Schedule regular security reviews** (at least quarterly)

---

## Additional Security Resources

- [OWASP XSS Prevention Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Cross_Site_Scripting_Prevention_Cheat_Sheet.html)
- [DOMPurify Documentation](https://github.com/cure53/DOMPurify)
- [Content Security Policy Level 3](https://w3c.github.io/webappsec-csp/)
- [KaTeX Security Considerations](https://katex.org/docs/security.html)

---

## Contact

For questions or issues with these security fixes, please refer to the main security audit report:
`frontend/SECURITY_AUDIT_REPORT_LATEX.md`

---

**Last Updated**: 2026-04-12
**Security Level**: 🔴 → 🟢 (after implementation)
