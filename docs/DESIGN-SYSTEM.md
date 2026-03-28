# PaperCrawler UI/UX Design System

## Overview

Comprehensive design system for PaperCrawler project ensuring visual consistency across web frontend and desktop application while maintaining modern, accessible, and performant user interfaces.

## Design Philosophy

- **Consistency First**: Unified visual language across web and desktop platforms
- **Accessibility**: WCAG AA compliance (4.5:1 contrast ratio minimum)
- **Performance**: Optimized animations and rendering
- **Modern Aesthetics**: Glass-morphism, gradient backgrounds, smooth transitions
- **User-Centric**: Intuitive navigation and clear visual hierarchy

---

## Color System

### Primary Colors

```css
/* Light Theme */
--color-primary: #667eea;        /* Main brand color */
--color-primary-dark: #764ba2;   /* Secondary brand color */
--color-primary-light: #8b9fe8;  /* Lighter variant */

/* Dark Theme */
--color-primary: #8b9fe8;
--color-primary-dark: #a7b9f0;
--color-primary-light: #667eea;
```

### Semantic Colors

```css
/* Status Colors - Light Theme */
--color-success: #10b981;  /* Green for success states */
--color-warning: #f59e0b;  /* Orange for warnings */
--color-error: #ef4444;    /* Red for errors */
--color-info: #3b82f6;     /* Blue for information */

/* Status Colors - Dark Theme */
--color-success: #34d399;
--color-warning: #fbbf24;
--color-error: #f87171;
--color-info: #60a5fa;
```

### CCF Level Colors

```css
/* CCF-A (Top Tier) - Light Theme */
--badge-a-bg: #fecaca;
--badge-a-text: #991b1b;

/* CCF-A - Dark Theme */
--badge-a-bg: #7f1d1d;
--badge-a-text: #fecaca;

/* CCF-B (Second Tier) - Light Theme */
--badge-b-bg: #fed7aa;
--badge-b-text: #9a3412;

/* CCF-B - Dark Theme */
--badge-b-bg: #7c2d12;
--badge-b-text: #fed7aa;

/* CCF-C (Third Tier) - Light Theme */
--badge-c-bg: #d1d5db;
--badge-c-text: #374151;

/* CCF-C - Dark Theme */
--badge-c-bg: #374151;
--badge-c-text: #d1d5db;
```

### Background Colors

```css
/* Light Theme */
--color-bg-primary: #ffffff;
--color-bg-secondary: #f9fafb;
--color-bg-tertiary: #f3f4f6;
--color-bg-overlay: rgba(255, 255, 255, 0.95);
--color-bg-gradient: linear-gradient(135deg, #667eea 0%, #764ba2 100%);

/* Dark Theme */
--color-bg-primary: #111827;
--color-bg-secondary: #1f2937;
--color-bg-tertiary: #374151;
--color-bg-overlay: rgba(31, 41, 55, 0.95);
--color-bg-gradient: linear-gradient(135deg, #1f2937 0%, #111827 100%);
```

### Text Colors

```css
/* Light Theme */
--color-text-primary: #1f2937;    /* Main text */
--color-text-secondary: #6b7280;  /* Secondary text */
--color-text-tertiary: #9ca3af;   /* Hints and placeholders */
--color-text-inverse: #ffffff;    /* Text on dark backgrounds */

/* Dark Theme */
--color-text-primary: #f9fafb;
--color-text-secondary: #d1d5db;
--color-text-tertiary: #9ca3af;
--color-text-inverse: #111827;
```

### Border Colors

```css
/* Light Theme */
--color-border-primary: #e5e7eb;
--color-border-secondary: #d1d5db;
--color-border-focus: #667eea;

/* Dark Theme */
--color-border-primary: #374151;
--color-border-secondary: #4b5563;
--color-border-focus: #8b9fe8;
```

---

## Typography System

### Font Families

```css
/* Primary Font - UI Elements */
--font-family-primary: 'Inter', 'Segoe UI', 'Microsoft YaHei UI', 'PingFang SC', system-ui, sans-serif;

/* Secondary Font - Code/Monospace */
--font-family-secondary: 'JetBrains Mono', 'Consolas', 'Monaco', monospace;
```

### Font Scale

```css
/* Type Scale */
--font-size-xs: 0.75rem;    /* 12px - Small text, captions */
--font-size-sm: 0.875rem;   /* 14px - Body text, labels */
--font-size-base: 1rem;     /* 16px - Default text */
--font-size-lg: 1.125rem;   /* 18px - Subheadings */
--font-size-xl: 1.25rem;    /* 20px - Important text */
--font-size-2xl: 1.5rem;    /* 24px - Headings */
--font-size-3xl: 1.875rem;  /* 30px - Large headings */
--font-size-4xl: 2.25rem;   /* 36px - Hero text */
```

### Font Weights

```css
--font-weight-normal: 400;    /* Regular body text */
--font-weight-medium: 500;    /* Emphasized text */
--font-weight-semibold: 600;  /* Headings, important text */
--font-weight-bold: 700;      /* Strong emphasis */
```

### Line Heights

```css
--line-height-tight: 1.25;    /* Headings */
--line-height-normal: 1.5;    /* Body text */
--line-height-relaxed: 1.75;  /* Long-form content */
```

---

## Spacing System

### 8-Point Grid System

```css
/* Base Unit: 4px */
--space-1: 0.25rem;   /* 4px */
--space-2: 0.5rem;    /* 8px */
--space-3: 0.75rem;   /* 12px */
--space-4: 1rem;      /* 16px */
--space-5: 1.25rem;   /* 20px */
--space-6: 1.5rem;    /* 24px */
--space-8: 2rem;      /* 32px */
--space-10: 2.5rem;   /* 40px */
--space-12: 3rem;     /* 48px */
--space-16: 4rem;     /* 64px */
--space-20: 5rem;     /* 80px */
```

### Component Spacing

```css
/* Card padding */
--card-padding-sm: var(--space-4);   /* 16px */
--card-padding-md: var(--space-6);   /* 24px */
--card-padding-lg: var(--space-8);   /* 32px */

/* Gap between elements */
--gap-xs: var(--space-2);   /* 8px */
--gap-sm: var(--space-3);   /* 12px */
--gap-md: var(--space-4);   /* 16px */
--gap-lg: var(--space-6);   /* 24px */
--gap-xl: var(--space-8);   /* 32px */
```

---

## Border Radius

```css
--radius-sm: 0.375rem;   /* 6px - Small elements */
--radius-md: 0.5rem;     /* 8px - Buttons, inputs */
--radius-lg: 0.75rem;    /* 12px - Cards */
--radius-xl: 1rem;       /* 16px - Large cards */
--radius-2xl: 1.5rem;    /* 24px - Hero elements */
--radius-full: 9999px;   /* Circular elements */
```

---

## Shadow System

```css
/* Light Theme Shadows */
--shadow-sm: 0 1px 2px 0 rgba(0, 0, 0, 0.05);
--shadow-md: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
--shadow-lg: 0 10px 15px -3px rgba(0, 0, 0, 0.1);
--shadow-xl: 0 20px 25px -5px rgba(0, 0, 0, 0.1);

/* Dark Theme Shadows */
--shadow-sm: 0 1px 2px 0 rgba(0, 0, 0, 0.3);
--shadow-md: 0 4px 6px -1px rgba(0, 0, 0, 0.4);
--shadow-lg: 0 10px 15px -3px rgba(0, 0, 0, 0.4);
--shadow-xl: 0 20px 25px -5px rgba(0, 0, 0, 0.5);
```

---

## Animation System

### Transition Durations

```css
--transition-fast: 150ms ease;     /* Micro-interactions */
--transition-normal: 300ms ease;   /* Standard transitions */
--transition-slow: 500ms ease;     /* Complex animations */
```

### Easing Functions

```css
--ease-in: cubic-bezier(0.4, 0, 1, 1);
--ease-out: cubic-bezier(0, 0, 0.2, 1);
--ease-in-out: cubic-bezier(0.4, 0, 0.2, 1);
```

### Key Animations

```css
/* Fade In */
@keyframes fadeIn {
  from { opacity: 0; }
  to { opacity: 1; }
}

/* Slide Up */
@keyframes slideUp {
  from { transform: translateY(20px); opacity: 0; }
  to { transform: translateY(0); opacity: 1; }
}

/* Scale In */
@keyframes scaleIn {
  from { transform: scale(0.95); opacity: 0; }
  to { transform: scale(1); opacity: 1; }
}

/* Pulse (for loading states) */
@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.5; }
}

/* Spin (for loading indicators) */
@keyframes spin {
  to { transform: rotate(360deg); }
}

/* Shimmer (for skeleton loading) */
@keyframes shimmer {
  0% { background-position: -1000px 0; }
  100% { background-position: 1000px 0; }
}
```

---

## Component Specifications

### Buttons

#### Primary Button
```css
.btn-primary {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  padding: 12px 24px;
  border-radius: 10px;
  border: none;
  font-weight: 600;
  font-size: 16px;
  cursor: pointer;
  transition: all var(--transition-normal);
}

.btn-primary:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: var(--shadow-lg);
}

.btn-primary:active:not(:disabled) {
  transform: translateY(0);
}

.btn-primary:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}
```

#### Secondary Button
```css
.btn-secondary {
  background: white;
  color: var(--color-primary);
  border: 2px solid var(--color-primary);
  padding: 12px 24px;
  border-radius: 10px;
  font-weight: 600;
  cursor: pointer;
  transition: all var(--transition-normal);
}

.btn-secondary:hover:not(:disabled) {
  background: var(--color-primary);
  color: white;
}
```

#### Icon Button
```css
.btn-icon {
  background: transparent;
  border: none;
  border-radius: 8px;
  padding: 8px;
  min-width: 36px;
  min-height: 36px;
  cursor: pointer;
  transition: background-color var(--transition-fast);
}

.btn-icon:hover {
  background-color: rgba(102, 126, 234, 0.1);
}
```

### Input Fields

```css
.input {
  background-color: white;
  border: 2px solid var(--color-border-primary);
  border-radius: 10px;
  padding: 12px 16px;
  font-size: 16px;
  color: var(--color-text-primary);
  transition: all var(--transition-fast);
}

.input:hover {
  border-color: var(--color-border-secondary);
}

.input:focus {
  outline: none;
  border-color: var(--color-border-focus);
  box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
}

.input:disabled {
  background-color: var(--color-bg-tertiary);
  color: var(--color-text-tertiary);
  cursor: not-allowed;
}

.input::placeholder {
  color: var(--color-text-tertiary);
}
```

### Cards

```css
.card {
  background: var(--card-bg);
  border: var(--card-border);
  border-radius: 15px;
  padding: var(--card-padding-md);
  box-shadow: var(--shadow-sm);
  transition: all var(--transition-normal);
}

.card:hover {
  box-shadow: var(--shadow-lg);
  transform: translateY(-2px);
}

.card-elevated {
  box-shadow: var(--shadow-md);
}

.card-elevated:hover {
  box-shadow: var(--shadow-xl);
  transform: translateY(-4px);
}
```

### Badges

```css
.badge {
  display: inline-flex;
  align-items: center;
  padding: 4px 12px;
  border-radius: 20px;
  font-size: 12px;
  font-weight: 600;
}

.badge-a {
  background-color: var(--badge-a-bg);
  color: var(--badge-a-text);
}

.badge-b {
  background-color: var(--badge-b-bg);
  color: var(--badge-b-text);
}

.badge-c {
  background-color: var(--badge-c-bg);
  color: var(--badge-c-text);
}
```

### Loading States

#### Spinner
```css
.spinner {
  width: 40px;
  height: 40px;
  border: 4px solid rgba(102, 126, 234, 0.2);
  border-top-color: var(--color-primary);
  border-radius: 50%;
  animation: spin 1s linear infinite;
}
```

#### Skeleton Loading
```css
.skeleton {
  background: linear-gradient(
    90deg,
    var(--color-bg-tertiary) 0%,
    var(--color-bg-secondary) 50%,
    var(--color-bg-tertiary) 100%
  );
  background-size: 1000px 100%;
  animation: shimmer 2s infinite;
  border-radius: 6px;
}
```

---

## Responsive Design

### Breakpoints

```css
/* Mobile First Approach */
--breakpoint-sm: 640px;   /* Small devices */
--breakpoint-md: 768px;   /* Medium devices */
--breakpoint-lg: 1024px;  /* Large devices */
--breakpoint-xl: 1280px;  /* Extra large devices */
```

### Container Widths

```css
.container {
  width: 100%;
  margin-left: auto;
  margin-right: auto;
  padding-left: var(--space-4);
  padding-right: var(--space-4);
}

@media (min-width: 640px) {
  .container { max-width: 640px; }
}

@media (min-width: 768px) {
  .container { max-width: 768px; }
}

@media (min-width: 1024px) {
  .container { max-width: 1024px; }
}

@media (min-width: 1280px) {
  .container { max-width: 1280px; }
}
```

---

## Accessibility Guidelines

### Color Contrast

- **Normal text** (< 18px): 4.5:1 minimum contrast ratio
- **Large text** (≥ 18px or ≥ 14px bold): 3:1 minimum contrast ratio
- **Interactive elements**: 3:1 minimum contrast ratio against adjacent colors
- **Non-text elements**: 3:1 minimum contrast ratio for icons and graphics

### Focus States

```css
.focusable:focus-visible {
  outline: 2px solid var(--color-border-focus);
  outline-offset: 2px;
}

.focusable:focus:not(:focus-visible) {
  outline: none;
}
```

### Touch Targets

- Minimum size: 44×44 pixels for touch devices
- Adequate spacing between interactive elements
- Clear visual feedback for touch interactions

### Screen Reader Support

- Semantic HTML structure
- Proper ARIA labels and roles
- Meaningful alt text for images
- Skip navigation links

---

## Dark Mode Implementation

### Theme Switching

```javascript
// JavaScript implementation for theme switching
function setTheme(theme) {
  document.documentElement.setAttribute('data-theme', theme);
  localStorage.setItem('theme', theme);
}

function getPreferredTheme() {
  const storedTheme = localStorage.getItem('theme');
  if (storedTheme) {
    return storedTheme;
  }
  return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
}

// Initialize theme
document.addEventListener('DOMContentLoaded', () => {
  setTheme(getPreferredTheme());
});
```

### Theme Transitions

```css
/* Smooth theme switching */
* {
  transition: background-color var(--transition-normal),
              border-color var(--transition-normal),
              color var(--transition-normal),
              box-shadow var(--transition-normal);
}

/* Prevent transition on initial load */
.no-transition *,
.no-transition *::before,
.no-transition *::after {
  transition: none !important;
}
```

---

## Performance Guidelines

### CSS Optimization

- Use CSS custom properties (variables) for theme switching
- Minimize layout thrashing with transform and opacity animations
- Use will-change property sparingly for known animations
- Implement CSS containment for independent components

### Asset Optimization

- Use modern image formats (WebP, AVIF)
- Implement responsive images with srcset
- Lazy load off-screen images
- Use SVG icons for scalability

### Animation Performance

- Prefer transform and opacity over position and size changes
- Use requestAnimationFrame for JavaScript animations
- Implement animation cleanup for component unmounting
- Reduce motion for users with motion sensitivity

```css
@media (prefers-reduced-motion: reduce) {
  *,
  *::before,
  *::after {
    animation-duration: 0.01ms !important;
    animation-iteration-count: 1 !important;
    transition-duration: 0.01ms !important;
  }
}
```

---

## Browser Support

- Modern browsers (Chrome, Firefox, Safari, Edge) last 2 versions
- Mobile browsers (iOS Safari, Chrome Mobile)
- Progressive enhancement for older browsers
- Graceful degradation for unsupported features

---

## Design Tokens Export

### CSS Variables

The complete design token system is available in:
- `frontend/src/assets/theme.css` - Web frontend theme system
- `desktop/resources/styles/modern.qss` - Desktop application QSS stylesheet

### Platform-Specific Implementations

- **Web**: CSS custom properties with JavaScript theme switching
- **Desktop (Qt)**: QSS stylesheets with C++ ThemeManager
- **Future**: Native mobile implementations following same tokens

---

## Implementation Checklist

### Phase 1: Foundation
- [x] Define color system
- [x] Create typography scale
- [x] Establish spacing system
- [x] Implement base component styles

### Phase 2: Components
- [ ] Build button variants
- [ ] Create form element styles
- [ ] Design card components
- [ ] Implement navigation patterns

### Phase 3: Patterns
- [ ] Loading states
- [ ] Error handling
- [ ] Empty states
- [ ] Success feedback

### Phase 4: Optimization
- [ ] Performance audit
- [ ] Accessibility testing
- [ ] Cross-browser validation
- [ ] Mobile responsiveness testing

---

**Version**: 1.0.0
**Last Updated**: 2025-01-22
**Maintained By**: UI Designer Agent
