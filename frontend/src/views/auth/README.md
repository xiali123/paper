# PaperCrawler Authentication System

Complete authentication system for PaperCrawler with modern Vue 3 components, comprehensive validation, and internationalization support.

## 📁 Structure

```
src/views/auth/
├── LoginView.vue           # Login page with social login buttons
├── RegisterView.vue        # Registration with password strength indicator
├── ForgotPasswordView.vue  # Forgot password with email submission
├── ResetPasswordView.vue   # Password reset with validation
└── README.md              # This file
```

## 🎯 Features

### LoginView.vue
- **Email & Password Authentication**
  - Real-time form validation
  - Password visibility toggle
  - Remember me option
  - Loading states with spinner

- **User Experience**
  - Auto-focus on email field
  - Keyboard support (Enter to submit)
  - Responsive design for mobile
  - Dark mode support

- **Social Login (Optional)**
  - Google OAuth button (prepared)
  - GitHub OAuth button (prepared)
  - Easy to extend with other providers

### RegisterView.vue
- **Comprehensive Registration**
  - Username, email, password fields
  - Real-time password strength indicator
  - Password requirements checklist
  - Terms and conditions agreement

- **Password Strength Meter**
  - Visual strength bar (weak/medium/strong)
  - Real-time feedback as user types
  - Requirements checklist with visual indicators
  - Color-coded strength levels

- **Validation**
  - Client-side validation for all fields
  - Server-side error handling
  - Clear error messages
  - Inline validation feedback

### ForgotPasswordView.vue
- **Email-Based Password Reset**
  - Simple email input form
  - Success state with confirmation
  - Resend email functionality
  - Clear instructions for users

- **User Experience**
  - Success state with animated checkmark
  - Countdown for resend (optional)
  - Back to login link
  - Email confirmation display

### ResetPasswordView.vue
- **Secure Password Reset**
  - Token-based reset flow
  - New password with confirmation
  - Password requirements display
  - Strength indicator

- **Visual Feedback**
  - Real-time password strength
  - Requirements checklist
  - Success confirmation
  - Auto-redirect to login

## 🔧 Usage

### Basic Integration

The authentication views are automatically integrated with the Vue Router. Simply navigate to the routes:

```typescript
// Navigate to login
router.push('/auth/login')

// Navigate to register
router.push('/auth/register')

// Navigate to forgot password
router.push('/auth/forgot-password')

// Navigate to reset password (with token)
router.push({
  path: '/auth/reset-password',
  query: { token: 'reset-token-from-email' }
})
```

### Route Protection

Routes are automatically protected using the auth store:

```typescript
// Protected routes require authentication
{
  path: '/dashboard',
  component: Dashboard,
  meta: { requiresAuth: true }
}

// Guest-only routes redirect if authenticated
{
  path: '/auth/login',
  component: LoginView,
  meta: { guestOnly: true }
}
```

### Using in Components

```vue
<script setup lang="ts">
import { useAuthStore } from '@/stores/auth'
import { useRouter } from 'vue-router'

const authStore = useAuthStore()
const router = useRouter()

// Check authentication status
if (authStore.isAuthenticated) {
  console.log('User is logged in')
  console.log('User:', authStore.user)
}

// Logout
async function logout() {
  await authStore.logout()
  router.push('/auth/login')
}
</script>
```

## 🎨 Styling

### Design System
- **Modern gradient backgrounds** with animated patterns
- **Card-based layout** with rounded corners and shadows
- **Responsive design** that works on all devices
- **Dark mode support** with proper color schemes
- **Accessibility features** (ARIA labels, keyboard navigation)

### Color Scheme
- **Primary**: Gradient from #667eea to #764ba2
- **Success**: #10b981 (green)
- **Error**: #ef4444 (red)
- **Warning**: #f59e0b (amber)
- **Neutral**: #6b7280 (gray)

### Typography
- **Headings**: 28px, weight 700
- **Body**: 15px, weight 400
- **Small**: 13px, weight 400
- **Font**: System font stack

## 🌍 Internationalization

All text is internationalized using Vue I18n. Translations are available in:

- **English** (`en-US.json`)
- **Chinese** (`zh-CN.json`)

### Adding New Translations

```json
// src/i18n/locales/en-US.json
{
  "auth": {
    "yourKey": "Your translation"
  }
}

// Use in components
{{ t('auth.yourKey') }}
```

## 🔐 Security Features

### Client-Side Security
- **Input validation** on all fields
- **Password strength requirements**
- **XSS protection** through Vue's templating
- **CSRF protection** via API headers

### Server-Side Integration
- **JWT token authentication**
- **Refresh token mechanism**
- **Secure token storage**
- **Automatic token refresh**

### Password Requirements
- Minimum 8 characters
- At least one uppercase letter
- At least one lowercase letter
- At least one number
- Special characters encouraged

## 📱 Responsive Design

### Breakpoints
- **Mobile**: < 480px
- **Tablet**: 480px - 768px
- **Desktop**: > 768px

### Mobile Optimizations
- Full-width cards on mobile
- Stacked social buttons
- Optimized touch targets
- Reduced padding and margins

## 🚀 Performance

### Optimizations
- **Lazy loading** of auth components
- **Code splitting** by route
- **Optimized re-renders** with Vue 3 composition API
- **Minimal bundle size** impact

### Load Times
- Initial page load: < 1s
- Route transitions: < 100ms
- Form validation: Real-time

## 🧪 Testing

### Manual Testing Checklist

#### LoginView
- [ ] Email validation works
- [ ] Password validation works
- [ ] Show/hide password toggle
- [ ] Remember me checkbox
- [ ] Login success flow
- [ ] Login error handling
- [ ] Redirect after login

#### RegisterView
- [ ] Username validation
- [ ] Email validation
- [ ] Password strength indicator
- [ ] Password requirements checklist
- [ ] Confirm password matching
- [ ] Terms agreement checkbox
- [ ] Registration success flow
- [ ] Registration error handling

#### ForgotPasswordView
- [ ] Email validation
- [ ] Send reset link flow
- [ ] Success state display
- [ ] Resend email functionality
- [ ] Back to login link

#### ResetPasswordView
- [ ] Password validation
- [ ] Confirm password matching
- [ ] Password strength indicator
- [ ] Reset success flow
- [ ] Reset error handling
- [ ] Redirect to login

## 🐛 Troubleshooting

### Common Issues

#### "Login failed" error
- Check email format
- Verify password is correct
- Check API endpoint configuration
- Review browser console for errors

#### "Registration failed" error
- Ensure all fields are filled correctly
- Check password meets requirements
- Verify email isn't already registered
- Check network connection

#### Password reset not working
- Verify reset token is valid
- Check token expiration
- Ensure email was received
- Check API endpoint configuration

### Debug Mode

Enable debug logging in browser console:

```javascript
localStorage.setItem('debug', 'true')
```

## 📚 API Integration

The auth views integrate with the existing API:

```typescript
import { authApi } from '@/api/modules/auth'

// Login
const response = await authApi.login({
  email: 'user@example.com',
  password: 'password123'
})

// Register
const response = await authApi.register({
  username: 'johndoe',
  email: 'john@example.com',
  password: 'SecurePass123!'
})

// Password reset
await authApi.requestPasswordReset('user@example.com')
await authApi.resetPassword({
  token: 'reset-token',
  newPassword: 'NewSecurePass456!'
})
```

## 🔄 Migration from Old Auth

If migrating from the old authentication system:

1. **Update route references**:
   - `/login` → `/auth/login`
   - `/register` → `/auth/register`
   - `/forgot-password` → `/auth/forgot-password`

2. **Update imports**:
   ```typescript
   // Old
   import Login from '@/views/Login.vue'

   // New
   import LoginView from '@/views/auth/LoginView.vue'
   ```

3. **Update store usage**:
   ```typescript
   // Old (if using different store)
   import { useUserStore } from '@/stores/user'

   // New
   import { useAuthStore } from '@/stores/auth'
   ```

## 🎓 Best Practices

1. **Always validate input** on both client and server
2. **Use HTTPS** in production
3. **Implement rate limiting** for login attempts
4. **Log out users** on token expiration
5. **Clear sensitive data** on logout
6. **Use secure password** requirements
7. **Implement 2FA** for enhanced security
8. **Keep sessions** manageable with refresh tokens

## 📄 License

This authentication system is part of PaperCrawler and follows the same license.

## 🤝 Contributing

To contribute to the authentication system:

1. Follow the existing code style
2. Add tests for new features
3. Update documentation
4. Ensure internationalization
5. Test on multiple devices
6. Verify accessibility

---

**Last Updated**: 2026-04-04
**Version**: 1.0.0
**Author**: PaperCrawler Team