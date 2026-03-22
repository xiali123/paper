# Design Tokens Quick Reference

## Quick Lookup Guide for PaperCrawler Design System

### Spacing (4px base unit)

```css
--space-1: 4px    /* Tight spacing */
--space-2: 8px    /* Small gaps */
--space-3: 12px   /* Compact spacing */
--space-4: 16px   /* Default spacing */
--space-5: 20px   /* Comfortable spacing */
--space-6: 24px   /* Generous spacing */
--space-8: 32px   /* Large spacing */
--space-10: 40px  /* Extra large spacing */
--space-12: 48px  /* Huge spacing */
```

**Common Usage**:
- Button padding: `--space-3 --space-5`
- Card padding: `--space-4 --space-5`
- Section gaps: `--space-4 --space-6`
- Page margins: `--space-3 --space-4`

---

### Typography

```css
--font-size-xs: 12px     /* Meta text, badges */
--font-size-sm: 14px     /* Body text, buttons */
--font-size-base: 16px   /* Default text */
--font-size-lg: 20px     /* Subheadings */
--font-size-xl: 24px     /* Headings */
--font-size-2xl: 30px    /* Large headings */
--font-size-3xl: 36px    /* Hero text */
```

**Font Weights**:
```css
--font-weight-normal: 400
--font-weight-medium: 500
--font-weight-semibold: 600
--font-weight-bold: 700
```

---

### Border Radius

```css
--radius-sm: 4px      /* Small elements */
--radius-md: 8px      /* Buttons, inputs */
--radius-lg: 12px     /* Cards */
--radius-xl: 16px     /* Panels */
--radius-2xl: 24px    /* Large containers */
--radius-full: 9999px /* Pills, badges */
```

---

### Container Widths

```css
--container-8xl: 1440px  /* Standard pages */
--container-9xl: 1600px  /* Wide pages */
```

---

### Transitions

```css
--duration-instant: 100ms  /* Hover */
--duration-fast: 150ms     /* Button */
--duration-normal: 300ms   /* Default */
--duration-slow: 500ms     /* Complex */
```

---

## Component Templates

### Button
```css
.my-button {
  padding: var(--space-3) var(--space-5);
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-semibold);
  border-radius: var(--radius-md);
  gap: var(--space-2);
}
```

### Card
```css
.my-card {
  padding: var(--space-4) var(--space-5);
  border-radius: var(--radius-lg);
  box-shadow: var(--shadow-sm);
}
```

### Input
```css
.my-input {
  padding: var(--space-3) var(--space-4);
  font-size: var(--font-size-sm);
  border-radius: var(--radius-md);
}
```

### Badge
```css
.my-badge {
  padding: var(--space-1) var(--space-3);
  font-size: var(--font-size-xs);
  border-radius: var(--radius-full);
}
```

---

## Responsive Padding

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

---

## Quick Decision Tree

### What spacing to use?

1. **Is it a tight gap?** → `--space-1` to `--space-2`
2. **Is it default spacing?** → `--space-3` to `--space-4`
3. **Is it generous spacing?** → `--space-6` to `--space-8`
4. **Is it extra large?** → `--space-10` to `--space-12`

### What font size to use?

1. **Is it meta/caption?** → `--font-size-xs`
2. **Is it body text?** → `--font-size-sm`
3. **Is it a heading?** → `--font-size-lg` to `--font-size-2xl`
4. **Is it hero text?** → `--font-size-3xl`

### What border radius to use?

1. **Is it small?** → `--radius-sm`
2. **Is it standard?** → `--radius-md`
3. **Is it a card?** → `--radius-lg`
4. **Is it a pill/badge?** → `--radius-full`

---

## Common Patterns

### Page Layout
```html
<div class="page-container">
  <!-- max-width: var(--container-8xl) -->
  <!-- padding: var(--space-3) var(--space-4) -->
</div>
```

### Section Spacing
```css
.section {
  padding: var(--space-4) 0;
  gap: var(--space-4);
}
```

### Card Grid
```css
.card-grid {
  display: grid;
  gap: var(--space-4);
}

.card-grid .card {
  padding: var(--space-4) var(--space-5);
}
```

### Flex Gaps
```css
.flex-row {
  display: flex;
  gap: var(--space-3); /* or --space-4 */
}
```

---

## Migration Checklist

When updating components:

- [ ] Replace `padding: 16px` with `padding: var(--space-4)`
- [ ] Replace `font-size: 14px` with `font-size: var(--font-size-sm)`
- [ ] Replace `border-radius: 8px` with `border-radius: var(--radius-md)`
- [ ] Replace `gap: 10px` with `gap: var(--space-3)`
- [ ] Replace `max-width: 1400px` with `max-width: var(--container-8xl)`

---

## Accessibility Notes

### Minimum Touch Targets
```css
/* Minimum: 44px × 44px */
.touch-target {
  min-width: var(--space-11); /* 44px */
  min-height: var(--space-11);
}
```

### Focus Styles
```css
:focus-visible {
  outline: 2px solid var(--color-primary);
  outline-offset: 2px;
}
```

### Color Contrast
- Normal text: 4.5:1 minimum
- Large text: 3:1 minimum

---

## Browser Support

- Chrome/Edge: Last 2 versions ✅
- Firefox: Last 2 versions ✅
- Safari: Last 2 versions ✅
- Mobile: iOS 12+, Chrome Android ✅

---

## Need Help?

- Full documentation: `DESIGN-SYSTEM.md`
- Optimization summary: `UI-OPTIMIZATION-SUMMARY.md`
- Design tokens: `src/assets/design-system.css`

---

**Quick Tip**: Always use design tokens instead of hardcoded values for consistency!

**Version**: 1.0.0
**Last Updated**: March 22, 2025
