# PaperCrawler UI Design System

## Overview

The PaperCrawler Design System is a comprehensive, unified approach to UI design that ensures consistency, accessibility, and visual harmony across all pages and components.

## Design Principles

1. **Consistency First**: All components use the same spacing, typography, and color scales
2. **Accessibility**: WCAG AA compliant with proper contrast ratios and focus states
3. **Responsive**: Mobile-first design that works seamlessly across all devices
4. **Performance**: Optimized CSS with efficient animations and transitions
5. **Maintainability**: Clear naming conventions and systematic organization

## Design Tokens

### Spacing System (4px base unit)

All spacing follows a consistent scale based on powers of 2:

```css
--space-0: 0;           /* 0px */
--space-1: 0.25rem;     /* 4px */
--space-2: 0.5rem;      /* 8px */
--space-3: 0.75rem;     /* 12px */
--space-4: 1rem;        /* 16px */
--space-5: 1.25rem;     /* 20px */
--space-6: 1.5rem;      /* 24px */
--space-8: 2rem;        /* 32px */
--space-10: 2.5rem;     /* 40px */
--space-12: 3rem;       /* 48px */
--space-16: 4rem;       /* 64px */
--space-20: 5rem;       /* 80px */
```

**Usage Guidelines**:
- Use `--space-2` to `--space-4` for tight spacing (buttons, badges)
- Use `--space-4` to `--space-6` for default spacing (cards, sections)
- Use `--space-8` to `--space-12` for generous spacing (page margins, major sections)

### Typography System

Font sizes follow a major third scale for visual harmony:

```css
--font-size-xs: 0.75rem;      /* 12px - Captions, labels */
--font-size-sm: 0.875rem;     /* 14px - Body text, buttons */
--font-size-base: 1rem;       /* 16px - Default text */
--font-size-md: 1.125rem;     /* 18px - Large body */
--font-size-lg: 1.25rem;      /* 20px - Subheadings */
--font-size-xl: 1.5rem;       /* 24px - Headings */
--font-size-2xl: 1.875rem;    /* 30px - Large headings */
--font-size-3xl: 2.25rem;     /* 36px - Hero text */
```

**Font Weights**:
```css
--font-weight-normal: 400;    /* Regular text */
--font-weight-medium: 500;    /* Emphasized text */
--font-weight-semibold: 600;  /* Headings, buttons */
--font-weight-bold: 700;      /* Strong emphasis */
```

**Usage Guidelines**:
- Use `--font-size-xs` for meta information, timestamps, badges
- Use `--font-size-sm` for body text, buttons, labels
- Use `--font-size-base` for default content
- Use `--font-size-lg` to `--font-size-2xl` for headings
- Use `--font-size-3xl` for hero sections

### Border Radius System

Consistent corner radii for all components:

```css
--radius-sm: 0.25rem;    /* 4px - Small elements */
--radius-md: 0.5rem;     /* 8px - Buttons, inputs, cards */
--radius-lg: 0.75rem;    /* 12px - Large cards */
--radius-xl: 1rem;       /* 16px - Modals, panels */
--radius-2xl: 1.5rem;    /* 24px - Hero sections */
--radius-full: 9999px;   /* Pill shape, badges */
```

**Usage Guidelines**:
- Use `--radius-sm` for tags, small badges
- Use `--radius-md` for buttons, inputs, default cards
- Use `--radius-lg` for featured cards, sections
- Use `--radius-2xl` for hero sections, major containers
- Use `--radius-full` for pills, round buttons

### Container Widths

Consistent max-widths for layout:

```css
--container-8xl: 90rem;   /* 1440px - Standard container */
--container-9xl: 100rem;  /* 1600px - Wide container */
```

**Usage Guidelines**:
- Use `--container-8xl` for standard pages (Search, Stats)
- Use `--container-9xl` for content-heavy pages (Home)
- Always add responsive padding: `var(--space-4)` on mobile, `var(--space-6)` on tablet, `var(--space-8)` on desktop

### Color System

#### Primary Colors
```css
--color-primary: #667eea;           /* Main brand color */
--color-primary-dark: #764ba2;      /* Dark accent */
--color-primary-light: #8b9fe8;     /* Light accent */
```

#### Semantic Colors
```css
--color-success: #10b981;           /* Success states */
--color-warning: #f59e0b;           /* Warnings */
--color-error: #ef4444;             /* Errors */
--color-info: #3b82f6;              /* Information */
```

#### Badge Colors
```css
/* CCF Level Badges */
--badge-a-bg: #fecaca;
--badge-a-text: #991b1b;
--badge-b-bg: #fed7aa;
--badge-b-text: #9a3412;
--badge-c-bg: #d1d5db;
--badge-c-text: #374151;
```

## Component Standards

### Buttons

```css
/* Primary Button */
.btn {
  padding: var(--space-3) var(--space-5);
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-semibold);
  border-radius: var(--radius-md);
  gap: var(--space-2);
}
```

**Sizes**:
- Small: `padding: var(--space-2) var(--space-3)`
- Default: `padding: var(--space-3) var(--space-5)`
- Large: `padding: var(--space-4) var(--space-6)`

### Cards

```css
.card {
  background: var(--card-bg);
  border-radius: var(--radius-lg);
  padding: var(--space-5);
  box-shadow: var(--shadow-sm);
}
```

**Variants**:
- Default: `padding: var(--space-4) var(--space-5)`
- Compact: `padding: var(--space-3) var(--space-4)`
- Spacious: `padding: var(--space-6) var(--space-8)`

### Inputs

```css
.input {
  padding: var(--space-3) var(--space-4);
  font-size: var(--font-size-sm);
  border: 2px solid var(--color-border-primary);
  border-radius: var(--radius-md);
}
```

### Badges

```css
.badge {
  padding: var(--space-1) var(--space-3);
  font-size: var(--font-size-xs);
  font-weight: var(--font-weight-semibold);
  border-radius: var(--radius-full);
}
```

## Layout Guidelines

### Page Structure

```html
<div class="page-container">
  <!-- Max-width: var(--container-8xl) or var(--container-9xl) -->
  <!-- Padding: var(--space-3) var(--space-4) -->

  <header class="page-header">
    <!-- Padding: var(--space-5) var(--space-4) -->
  </header>

  <main class="page-content">
    <!-- Cards with gap: var(--space-4) -->
  </main>

  <footer class="page-footer">
    <!-- Padding: var(--space-5) -->
  </footer>
</div>
```

### Spacing Rules

1. **Vertical spacing between sections**: `var(--space-4)` to `var(--space-6)`
2. **Horizontal gaps in flex/grid**: `var(--space-3)` to `var(--space-4)`
3. **Card padding**: `var(--space-4)` to `var(--space-5)`
4. **Button gaps**: `var(--space-2)` to `var(--space-3)`

## Responsive Design

### Breakpoints

```css
--breakpoint-sm: 640px;
--breakpoint-md: 768px;
--breakpoint-lg: 1024px;
--breakpoint-xl: 1280px;
--breakpoint-2xl: 1536px;
```

### Mobile-First Approach

1. Start with mobile styles (default)
2. Use `@media (min-width: 768px)` for tablet
3. Use `@media (min-width: 1024px)` for desktop

### Responsive Padding

```css
.container {
  padding-left: var(--space-4);
  padding-right: var(--space-4);
}

@media (min-width: 768px) {
  .container {
    padding-left: var(--space-6);
    padding-right: var(--space-6);
  }
}

@media (min-width: 1024px) {
  .container {
    padding-left: var(--space-8);
    padding-right: var(--space-8);
  }
}
```

## Accessibility Standards

### Color Contrast

- Normal text: 4.5:1 ratio minimum
- Large text (18px+): 3:1 ratio minimum
- Interactive elements: 3:1 ratio minimum

### Focus States

All interactive elements must have visible focus states:

```css
:focus-visible {
  outline: 2px solid var(--color-primary);
  outline-offset: 2px;
}
```

### Touch Targets

- Minimum size: 44px × 44px
- Preferred size: 48px × 48px

### Screen Readers

- Use semantic HTML elements
- Provide ARIA labels where necessary
- Include `.sr-only` text for screen reader only content

## Animation & Transitions

### Durations

```css
--duration-instant: 100ms;   /* Hover effects */
--duration-fast: 150ms;      /* Button interactions */
--duration-normal: 300ms;    /* Default transitions */
--duration-slow: 500ms;      /* Complex animations */
```

### Transition Properties

Always specify transition properties explicitly:

```css
/* Good */
transition: all var(--transition-normal);

/* Better (more performant) */
transition: transform var(--transition-normal),
            box-shadow var(--transition-normal);
```

### Reduced Motion

Respect user preferences:

```css
@media (prefers-reduced-motion: reduce) {
  * {
    animation-duration: 0.01ms !important;
    transition-duration: 0.01ms !important;
  }
}
```

## Best Practices

### DO's

✅ Use design tokens for all spacing, typography, and colors
✅ Follow mobile-first responsive design
✅ Ensure WCAG AA accessibility compliance
✅ Use semantic HTML elements
✅ Test with keyboard navigation
✅ Provide loading and error states
✅ Maintain consistent visual hierarchy

### DON'Ts

❌ Use arbitrary pixel values
❌ Hardcode colors or spacing
❌ Skip accessibility testing
❌ Use divs for everything (use semantic HTML)
❌ Create inconsistent spacing
❌ Ignore focus states
❌ Forget responsive design

## Migration Checklist

When updating existing components:

1. Replace all pixel values with design tokens
2. Standardize container max-widths
3. Update font sizes to use typography scale
4. Replace inconsistent border-radius values
5. Ensure proper spacing between elements
6. Add missing focus states
7. Test responsive behavior
8. Verify accessibility compliance

## File Structure

```
frontend/src/
├── assets/
│   ├── design-system.css    # Design tokens and base styles
│   └── theme.css            # Color themes (light/dark)
├── components/
│   ├── common/              # Reusable components
│   └── ...
├── views/
│   ├── Home.vue             # Uses --container-9xl
│   ├── Search.vue           # Uses --container-8xl
│   └── Stats.vue            # Uses --container-8xl
└── App.vue                  # Root layout
```

## Browser Support

- Chrome/Edge: Last 2 versions
- Firefox: Last 2 versions
- Safari: Last 2 versions
- Mobile browsers: iOS Safari 12+, Chrome Android

## Resources

- [WCAG 2.1 Guidelines](https://www.w3.org/WAI/WCAG21/quickref/)
- [Material Design Spacing](https://material.io/design/layout/understanding-layout.html)
- [Tailwind CSS Spacing Scale](https://tailwindcss.com/docs/customizing-spacing)
- [Type Scale Calculator](https://type-scale.com/)

## Changelog

### Version 1.0.0 (2025-03-22)
- Initial design system implementation
- Unified spacing, typography, and color systems
- Standardized container widths and padding
- Accessibility improvements
- Responsive design enhancements

---

**Maintained by**: UI Designer Agent
**Last Updated**: March 22, 2025
**Version**: 1.0.0
