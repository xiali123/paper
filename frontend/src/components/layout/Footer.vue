<template>
  <footer class="app-footer">
    <div class="footer-container">
      <!-- Footer Content -->
      <div class="footer-content">
        <!-- Logo and Links -->
        <div class="footer-brand">
          <div class="brand-logo">
            <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
              <path d="M19 3H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm-5 14H7v-2h7v2zm3-4H7v-2h10v2zm0-4H7V7h10v2z"/>
            </svg>
            <span>PaperCrawler</span>
          </div>
        </div>

        <!-- Quick Links -->
        <div class="footer-links">
          <router-link to="/papers">{{ t('nav.papers') }}</router-link>
          <el-divider direction="vertical" />
          <router-link to="/crawler">{{ t('nav.crawler') }}</router-link>
          <el-divider direction="vertical" />
          <router-link to="/search">{{ t('nav.search') }}</router-link>
          <el-divider direction="vertical" />
          <router-link to="/statistics">{{ t('nav.statistics') }}</router-link>
          <el-divider direction="vertical" />
          <router-link to="/export">{{ t('nav.export') }}</router-link>
        </div>

        <!-- External Links -->
        <div class="footer-links">
          <a href="https://github.com/papercrawler" target="_blank" rel="noopener">GitHub</a>
          <el-divider direction="vertical" />
          <router-link to="/changelog">{{ t('footer.changelog') }}</router-link>
          <el-divider direction="vertical" />
          <a href="https://github.com/papercrawler/issues" target="_blank" rel="noopener">{{ t('footer.support') }}</a>
        </div>

        <!-- Copyright -->
        <div class="footer-copyright">
          &copy; {{ currentYear }} PaperCrawler. {{ t('footer.rights') }}
          <el-divider direction="vertical" />
          <span class="version">v{{ version }}</span>
        </div>
      </div>
    </div>
  </footer>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useI18n } from 'vue-i18n'

const { t } = useI18n()

// Version info
const version = '1.0.0'
const currentYear = computed(() => new Date().getFullYear())
</script>

<style scoped lang="scss">
.app-footer {
  background: #f8fafc;
  border-top: 1px solid #e5e7eb;
  padding: 8px 0;
  margin-top: auto;

  .dark & {
    background: #1f2937;
    border-top-color: #374151;
  }
}

.footer-container {
  max-width: 1920px;
  margin: 0 auto;
  padding: 0 12px;
}

.footer-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;

  @media (max-width: 1024px) {
    flex-direction: column;
    align-items: flex-start;
    gap: 8px;
  }
}

.footer-brand {
  .brand-logo {
    display: flex;
    align-items: center;
    gap: 6px;
    color: #4f46e5;
    font-size: 14px;
    font-weight: 700;

    svg {
      flex-shrink: 0;
      width: 16px;
      height: 16px;
    }
  }
}

.footer-links {
  display: flex;
  align-items: center;
  gap: 4px 12px;
  flex-wrap: wrap;

  a {
    color: #6b7280;
    font-size: 12px;
    text-decoration: none;
    transition: color 0.2s;
    white-space: nowrap;

    &:hover {
      color: #4f46e5;
    }

    .dark & {
      color: #9ca3af;

      &:hover {
        color: #818cf8;
      }
    }
  }

  .el-divider {
    margin: 0;
    height: 12px;
    border-color: #d1d5db;

    .dark & {
      border-color: #4b5563;
    }
  }
}

.footer-copyright {
  display: flex;
  align-items: center;
  gap: 8px;
  color: #6b7280;
  font-size: 11px;
  white-space: nowrap;

  .dark & {
    color: #9ca3af;
  }

  .el-divider {
    margin: 0;
    height: 12px;
    border-color: #d1d5db;

    .dark & {
      border-color: #4b5563;
    }
  }

  .version {
    color: #9ca3af;
    font-weight: 500;
  }
}
</style>
