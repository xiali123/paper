# Authentication Quick Start Guide

Quick reference for using the PaperCrawler authentication system.

## 🔐 Routes

| Route | Component | Description |
|-------|-----------|-------------|
| `/auth/login` | LoginView | User login page |
| `/auth/register` | RegisterView | User registration page |
| `/auth/forgot-password` | ForgotPasswordView | Password reset request |
| `/auth/reset-password` | ResetPasswordView | Password reset form |

## 🚀 Quick Usage

### Check Authentication Status

```vue
<script setup lang="ts">
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()

// Check if user is logged in
if (authStore.isAuthenticated) {
  console.log('User:', authStore.user.value)
  console.log('Display name:', authStore.displayName.value)
}
</script>
```

### Login User

```vue
<script setup lang="ts">
import { useAuthStore } from '@/stores/auth'
import { useRouter } from 'vue-router'

const authStore = useAuthStore()
const router = useRouter()

async function login(email: string, password: string) {
  const result = await authStore.login({ email, password })

  if (result.success) {
    router.push('/dashboard')
  } else {
    console.error('Login failed:', result.error)
  }
}
</script>
```

### Logout User

```vue
<script setup lang="ts">
import { useAuthStore } from '@/stores/auth'
import { useRouter } from 'vue-router'

const authStore = useAuthStore()
const router = useRouter()

async function logout() {
  await authStore.logout()
  router.push('/auth/login')
}
</script>
```

### Register User

```vue
<script setup lang="ts">
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()

async function register(username: string, email: string, password: string) {
  const result = await authStore.register({
    username,
    email,
    password
  })

  if (result.success) {
    console.log('Registration successful')
  } else {
    console.error('Registration failed:', result.error)
  }
}
</script>
```

### Get User Information

```vue
<script setup lang="ts">
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()

// User object
const user = authStore.user.value

// User properties
console.log('User ID:', user?.id)
console.log('Username:', user?.username)
console.log('Email:', user?.email)
console.log('Role:', user?.role)

// Computed properties
console.log('Is Admin:', authStore.isAdmin.value)
console.log('Is Premium:', authStore.isPremium.value)
console.log('Display Name:', authStore.displayName.value)
</script>
```

## 🛡️ Route Protection

### Protect Routes

```typescript
// router/index.ts
{
  path: '/dashboard',
  component: Dashboard,
  meta: { requiresAuth: true }  // Requires login
}
```

### Guest-Only Routes

```typescript
// router/index.ts
{
  path: '/auth/login',
  component: LoginView,
  meta: { guestOnly: true }  // Redirect if logged in
}
```

### Role-Based Access

```typescript
// router/index.ts
{
  path: '/admin',
  component: Admin,
  meta: {
    requiresAuth: true,
    requiresAdmin: true  // Requires admin role
  }
}
```

## 🎨 Component Usage

### Navigation

```vue
<template>
  <nav>
    <router-link v-if="!authStore.isAuthenticated" to="/auth/login">
      Login
    </router-link>

    <router-link v-if="!authStore.isAuthenticated" to="/auth/register">
      Register
    </router-link>

    <button v-if="authStore.isAuthenticated" @click="logout">
      Logout
    </button>
  </nav>
</template>

<script setup lang="ts">
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()

async function logout() {
  await authStore.logout()
}
</script>
```

### User Profile Display

```vue
<template>
  <div v-if="authStore.isAuthenticated">
    <img :src="authStore.avatarUrl.value" alt="Avatar" />
    <h2>{{ authStore.displayName.value }}</h2>
    <p>{{ authStore.user.value?.email }}</p>
    <span class="role">{{ authStore.user.value?.role }}</span>
  </div>
</template>

<script setup lang="ts">
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()
</script>
```

## 🔧 Password Reset Flow

### Request Password Reset

```vue
<script setup lang="ts">
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()

async function requestReset(email: string) {
  const result = await authStore.requestPasswordReset(email)

  if (result.success) {
    console.log('Reset email sent')
  } else {
    console.error('Failed to send reset email:', result.error)
  }
}
</script>
```

### Reset Password

```vue
<script setup lang="ts">
import { useRoute } from 'vue-router'
import { useAuthStore } from '@/stores/auth'

const route = useRoute()
const authStore = useAuthStore()

// Get token from URL
const token = route.query.token as string

async function resetPassword(newPassword: string) {
  const result = await authStore.resetPassword(token, newPassword)

  if (result.success) {
    console.log('Password reset successful')
  } else {
    console.error('Password reset failed:', result.error)
  }
}
</script>
```

## 🌍 Internationalization

### Use Translations

```vue
<template>
  <h1>{{ t('auth.login') }}</h1>
  <p>{{ t('auth.loginSubtitle') }}</p>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

const { t } = useI18n()
</script>
```

### Add New Translations

```json
// en-US.json
{
  "auth": {
    "yourNewKey": "Your translation"
  }
}

// zh-CN.json
{
  "auth": {
    "yourNewKey": "您的翻译"
  }
}
```

## 🎯 Common Patterns

### Conditional Rendering

```vue
<template>
  <!-- Show if authenticated -->
  <div v-if="authStore.isAuthenticated">
    Welcome back!
  </div>

  <!-- Show if not authenticated -->
  <div v-else>
    Please login
  </div>

  <!-- Show if admin -->
  <div v-if="authStore.isAdmin">
    Admin Panel
  </div>
</template>
```

### Loading States

```vue
<template>
  <button :disabled="authStore.loading">
    <span v-if="authStore.loading">Loading...</span>
    <span v-else>Submit</span>
  </button>
</template>
```

### Error Handling

```vue
<template>
  <div v-if="authStore.error" class="error">
    {{ authStore.error }}
  </div>
</template>
```

## 📱 Responsive Design

### Mobile-Friendly Navigation

```vue
<template>
  <nav class="mobile-nav">
    <router-link to="/auth/login" class="nav-link">
      Login
    </router-link>
  </nav>
</template>

<style scoped>
.mobile-nav {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

@media (min-width: 768px) {
  .mobile-nav {
    flex-direction: row;
  }
}
</style>
```

## 🧪 Testing

### Manual Testing Checklist

1. **Login Flow**
   - [ ] Login with valid credentials
   - [ ] Login with invalid credentials
   - [ ] Login with empty fields
   - [ ] Redirect after successful login

2. **Registration Flow**
   - [ ] Register with valid data
   - [ ] Register with existing email
   - [ ] Password strength indicator
   - [ ] Terms agreement requirement

3. **Password Reset**
   - [ ] Request reset email
   - [ ] Reset password with token
   - [ ] Invalid token handling

## 🐛 Debug Mode

Enable console logging:

```javascript
// Browser console
localStorage.setItem('debug', 'true')

// Disable
localStorage.removeItem('debug')
```

## 📚 Additional Resources

- [Full Documentation](./README.md)
- [API Documentation](../../api/modules/auth.ts)
- [Store Documentation](../../stores/auth.ts)
- [Router Configuration](../../router/index.ts)

---

**Last Updated**: 2026-04-04
**Version**: 1.0.0