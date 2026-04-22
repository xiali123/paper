/**
 * LaTeX Input Validation Utilities
 * Provides security validation for LaTeX content to prevent:
 * - XSS attacks
 * - Command injection
 * - Path traversal
 * - DoS attacks
 */

const DANGEROUS_COMMANDS = [
  '\\write18',      // Execute shell commands
  '\\input',        // File inclusion
  '\\include',      // File inclusion
  '\\openout',      // Write to files
  '\\def',          // Redefine commands
  '\\let',          // Redefine commands
  '\\newcommand',   // Define new commands
  '\\renewcommand', // Redefine commands
  '\\newenvironment', // Define environments
  '\\renewenvironment', // Redefine environments
  '\\read',         // Read from files
  '\\write',        // Write to files
  '\\shell',        // Shell escape
  '\\immediate',    // Immediate execution
  '\\expandafter'   // Command expansion
] as const;

const MAX_CONTENT_LENGTH = 1000000; // 1MB limit
const MAX_LINE_LENGTH = 1000; // Per-line limit
const MAX_NESTING_DEPTH = 100; // Maximum nesting depth

export interface ValidationResult {
  valid: boolean;
  errors: string[];
  warnings: string[];
}

/**
 * Validate LaTeX input for security issues
 */
export function validateLatexInput(content: string): ValidationResult {
  const errors: string[] = [];
  const warnings: string[] = [];

  // Check content length
  if (content.length > MAX_CONTENT_LENGTH) {
    errors.push(`Content exceeds maximum length of ${MAX_CONTENT_LENGTH} characters`);
  }

  // Check for empty content (allow but warn)
  if (!content || content.trim().length === 0) {
    warnings.push('Content is empty');
  }

  // Split into lines for validation
  const lines = content.split('\n');

  // Check each line
  lines.forEach((line, index) => {
    const lineNum = index + 1;

    // Check line length
    if (line.length > MAX_LINE_LENGTH) {
      errors.push(`Line ${lineNum} exceeds maximum length of ${MAX_LINE_LENGTH} characters`);
    }

    // Check for dangerous commands
    DANGEROUS_COMMANDS.forEach(cmd => {
      if (line.includes(cmd)) {
        errors.push(`Line ${lineNum}: Dangerous command detected: ${cmd}`);
      }
    });

    // Check for path traversal
    if (line.includes('../') || line.includes('..\\')) {
      errors.push(`Line ${lineNum}: Path traversal sequence detected`);
    }

    // Check for binary content
    if (/[\x00-\x08\x0E-\x1F]/.test(line)) {
      errors.push(`Line ${lineNum}: Binary content detected`);
    }

    // Check for shell escape patterns
    if (/\\[a-zA-Z]*shell|\\[a-zA-Z]*escape|\\[a-zA-Z]*exec/i.test(line)) {
      errors.push(`Line ${lineNum}: Shell escape pattern detected`);
    }

    // Check for external file references
    if (/\\(input|include|verbatiminput)\s*\{[^}]*\.(tex|txt|bib|sty)/i.test(line)) {
      warnings.push(`Line ${lineNum}: External file reference detected - ensure files are validated`);
    }

    // Check for URL schemes (can be dangerous)
    if (/\\href\s*\{[^}]*\}/i.test(line)) {
      const urlMatch = line.match(/\\href\s*\{([^}]+)\}/i);
      if (urlMatch) {
        const url = urlMatch[1];
        if (url.startsWith('javascript:') || url.startsWith('data:')) {
          errors.push(`Line ${lineNum}: Dangerous URL scheme detected: ${url}`);
        }
      }
    }
  });

  // Check nesting depth
  const depth = checkNestingDepth(content);
  if (depth > MAX_NESTING_DEPTH) {
    errors.push(`Maximum nesting depth (${MAX_NESTING_DEPTH}) exceeded: ${depth}`);
  }

  // Check for balanced braces
  const braceCheck = checkBalancedBraces(content);
  if (!braceCheck.balanced) {
    errors.push(`Unbalanced braces detected: ${braceCheck.message}`);
  }

  // Check for math delimiter balance
  const mathCheck = checkBalancedMathDelimiters(content);
  if (!mathCheck.balanced) {
    warnings.push(`Unbalanced math delimiters detected: ${mathCheck.message}`);
  }

  return {
    valid: errors.length === 0,
    errors,
    warnings
  };
}

/**
 * Check nesting depth to prevent DoS
 */
function checkNestingDepth(content: string): number {
  let maxDepth = 0;
  let currentDepth = 0;

  for (let i = 0; i < content.length; i++) {
    const char = content[i];

    if (char === '{') {
      currentDepth++;
      maxDepth = Math.max(maxDepth, currentDepth);
    } else if (char === '}') {
      currentDepth--;
    }
  }

  return maxDepth;
}

/**
 * Check for balanced braces
 */
function checkBalancedBraces(content: string): {
  balanced: boolean;
  message: string;
} {
  let depth = 0;
  let maxDepth = 0;

  for (let i = 0; i < content.length; i++) {
    const char = content[i];

    if (char === '{') {
      depth++;
      maxDepth = Math.max(maxDepth, depth);
    } else if (char === '}') {
      depth--;
      if (depth < 0) {
        return {
          balanced: false,
          message: 'More closing braces than opening braces'
        };
      }
    }
  }

  if (depth > 0) {
    return {
      balanced: false,
      message: `Unclosed braces (depth: ${depth})`
    };
  }

  return {
    balanced: true,
    message: ''
  };
}

/**
 * Check for balanced math delimiters
 */
function checkBalancedMathDelimiters(content: string): {
  balanced: boolean;
  message: string;
} {
  let inlineDepth = 0;
  let displayDepth = 0;

  // Count inline math delimiters $
  for (let i = 0; i < content.length; i++) {
    if (content[i] === '$') {
      // Check if this is part of $$
      if (content[i + 1] === '$') {
        displayDepth++;
        i++; // Skip next character
      } else {
        inlineDepth++;
      }
    }
  }

  const issues: string[] = [];

  if (inlineDepth % 2 !== 0) {
    issues.push('Unmatched inline math delimiter $');
  }

  if (displayDepth % 2 !== 0) {
    issues.push('Unmatched display math delimiter $$');
  }

  return {
    balanced: issues.length === 0,
    message: issues.join('; ')
  };
}

/**
 * Sanitize LaTeX content by removing dangerous patterns
 * This is a last resort - always validate first!
 */
export function sanitizeLatexContent(content: string): string {
  let sanitized = content;

  // Remove dangerous commands
  DANGEROUS_COMMANDS.forEach(cmd => {
    const regex = new RegExp(cmd, 'gi');
    sanitized = sanitized.replace(regex, `%% ${cmd} REMOVED FOR SECURITY %%`);
  });

  // Remove path traversal
  sanitized = sanitized.replace(/\.\.\/|\.\\+/g, '');

  // Remove binary characters
  sanitized = sanitized.replace(/[\x00-\x08\x0E-\x1F]/g, '');

  return sanitized;
}

/**
 * Validate file path for LaTeX includes
 */
export function validateFilePath(filePath: string): {
  valid: boolean;
  error?: string;
} {
  // Check for path traversal
  if (filePath.includes('..') || filePath.includes('~')) {
    return {
      valid: false,
      error: 'Path traversal detected'
    };
  }

  // Check for absolute paths
  if (filePath.startsWith('/') || filePath.startsWith('\\')) {
    return {
      valid: false,
      error: 'Absolute paths not allowed'
    };
  }

  // Check for dangerous file extensions
  const dangerousExts = ['.exe', '.sh', '.bat', '.cmd', '.ps1', '.vbs'];
  const ext = filePath.toLowerCase().split('.').pop();
  if (ext && dangerousExts.includes(`.${ext}`)) {
    return {
      valid: false,
      error: 'Dangerous file extension detected'
    };
  }

  return {
    valid: true
  };
}

/**
 * Validate URL for href commands
 */
export function validateUrl(url: string): {
  valid: boolean;
  error?: string;
} {
  // Check for dangerous URL schemes
  const dangerousSchemes = ['javascript:', 'data:', 'vbscript:', 'file:'];
  const lowerUrl = url.toLowerCase();

  for (const scheme of dangerousSchemes) {
    if (lowerUrl.startsWith(scheme)) {
      return {
        valid: false,
        error: `Dangerous URL scheme detected: ${scheme}`
      };
    }
  }

  // Only allow http, https, mailto
  const allowedSchemes = ['http://', 'https://', 'mailto:'];
  const hasAllowedScheme = allowedSchemes.some(scheme => lowerUrl.startsWith(scheme));

  if (!hasAllowedScheme && !url.startsWith('#') && !url.startsWith('/')) {
    return {
      valid: false,
      error: 'Only http, https, mailto, and relative URLs are allowed'
    };
  }

  return {
    valid: true
  };
}
