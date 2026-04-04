<template>
  <footer class="app-footer">
    <div class="footer-container">
      <!-- Footer Content -->
      <div class="footer-content">
        <!-- Logo and Description -->
        <div class="footer-brand">
          <div class="brand-logo">
            <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
              <path d="M19 3H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm-5 14H7v-2h7v2zm3-4H7v-2h10v2zm0-4H7V7h10v2z"/>
            </svg>
            <span>PaperCrawler</span>
          </div>
          <p class="brand-description">
            {{ t('footer.description') }}
          </p>
        </div>

        <!-- Quick Links -->
        <div class="footer-links">
          <h4 class="links-title">{{ t('footer.quickLinks') }}</h4>
          <ul class="links-list">
            <li>
              <router-link to="/papers">{{ t('nav.papers') }}</router-link>
            </li>
            <li>
              <router-link to="/crawler">{{ t('nav.crawler') }}</router-link>
            </li>
            <li>
              <router-link to="/statistics">{{ t('nav.statistics') }}</router-link>
            </li>
            <li>
              <router-link to="/export">{{ t('nav.export') }}</router-link>
            </li>
          </ul>
        </div>

        <!-- Resources -->
        <div class="footer-links">
          <h4 class="links-title">{{ t('footer.resources') }}</h4>
          <ul class="links-list">
            <li>
              <a href="https://github.com/papercrawler" target="_blank" rel="noopener">
                {{ t('footer.documentation') }}
              </a>
            </li>
            <li>
              <a href="https://github.com/papercrawler" target="_blank" rel="noopener">
                GitHub
              </a>
            </li>
            <li>
              <a href="https://github.com/papercrawler/issues" target="_blank" rel="noopener">
                {{ t('footer.support') }}
              </a>
            </li>
            <li>
              <router-link to="/changelog">{{ t('footer.changelog') }}</router-link>
            </li>
          </ul>
        </div>

        <!-- Legal -->
        <div class="footer-links">
          <h4 class="links-title">{{ t('footer.legal') }}</h4>
          <ul class="links-list">
            <li>
              <router-link to="/privacy">{{ t('footer.privacy') }}</router-link>
            </li>
            <li>
              <router-link to="/terms">{{ t('footer.terms') }}</router-link>
            </li>
            <li>
              <router-link to="/license">{{ t('footer.license') }}</router-link>
            </li>
          </ul>
        </div>
      </div>

      <!-- Footer Bottom -->
      <div class="footer-bottom">
        <div class="copyright">
          &copy; {{ currentYear }} PaperCrawler. {{ t('footer.rights') }}
        </div>
        <div class="footer-meta">
          <span class="version">v{{ version }}</span>
          <el-divider direction="vertical" />
          <span class="build">{{ buildNumber }}</span>
        </div>
        <div class="social-links">
          <el-tooltip content="GitHub" placement="top">
            <a
              href="https://github.com/papercrawler"
              target="_blank"
              rel="noopener"
              class="social-link"
              aria-label="GitHub"
            >
              <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                <path d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z"/>
              </svg>
            </a>
          </el-tooltip>
          <el-tooltip content="Twitter" placement="top">
            <a
              href="https://twitter.com/papercrawler"
              target="_blank"
              rel="noopener"
              class="social-link"
              aria-label="Twitter"
            >
              <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                <path d="M23.953 4.57a10 10 0 01-2.825.775 4.958 4.958 0 002.163-2.723c-.951.555-2.005.959-3.127 1.184a4.92 4.92 0 00-8.384 4.482C7.69 8.095 4.067 6.13 1.64 3.162a4.822 4.822 0 00-.666 2.475c0 1.71.87 3.213 2.188 4.096a4.904 4.904 0 01-2.228-.616v.06a4.923 4.923 0 003.946 4.827 4.996 4.996 0 01-2.212.085 4.936 4.936 0 004.604 3.417 9.867 9.867 0 01-6.102 2.105c-.39 0-.779-.023-1.17-.067a13.995 13.995 0 007.557 2.209c9.053 0 13.998-7.496 13.998-13.985 0-.21 0-.42-.015-.63A9.935 9.935 0 0024 4.59z"/>
              </svg>
            </a>
          </el-tooltip>
          <el-tooltip content="Discord" placement="top">
            <a
              href="https://discord.gg/papercrawler"
              target="_blank"
              rel="noopener"
              class="social-link"
              aria-label="Discord"
            >
              <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                <path d="M20.317 4.37a19.791 19.791 0 00-4.885-1.515.074.074 0 00-.079.037c-.21.375-.444.864-.608 1.25a18.27 18.27 0 00-5.487 0 12.64 12.64 0 00-.617-1.25.077.077 0 00-.079-.037A19.736 19.736 0 003.677 4.37a.07.07 0 00-.032.027C.533 9.046-.32 13.58.099 18.057a.082.082 0 00.031.057 19.9 19.9 0 005.993 3.03.078.078 0 00.084-.028c.462-.63.874-1.295 1.226-1.994a.076.076 0 00-.041-.106 13.107 13.107 0 01-1.872-.892.077.077 0 01-.008-.128 10.2 10.2 0 00.372-.292.074.074 0 01.077-.01c3.928 1.793 8.18 1.793 12.062 0a.074.074 0 01.078.01c.12.098.246.198.373.292a.077.077 0 01-.006.127 12.299 12.299 0 01-1.873.892.077.077 0 00-.041.107c.36.698.772 1.362 1.225 1.993a.076.076 0 00.084.028 19.839 19.839 0 006.002-3.03.077.077 0 00.032-.054c.5-5.177-.838-9.674-3.549-13.66a.061.061 0 00-.031-.03zM8.02 15.33c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.956-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.956 2.418-2.157 2.418zm7.975 0c-1.183 0-2.157-1.085-2.157-2.419 0-1.333.955-2.419 2.157-2.419 1.21 0 2.176 1.096 2.157 2.42 0 1.333-.946 2.418-2.157 2.418z"/>
              </svg>
            </a>
          </el-tooltip>
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
const buildNumber = '2026.04.04'
const currentYear = computed(() => new Date().getFullYear())
</script>

<style scoped lang="scss">
.app-footer {
  background: var(--gray-50);
  border-top: 1px solid var(--gray-200);
  padding: var(--space-12) 0 var(--space-6);
  margin-top: auto;

  .dark & {
    background: var(--gray-900);
    border-top-color: var(--gray-800);
  }
}

.footer-container {
  max-width: var(--container-default);
  margin: 0 auto;
  padding: 0 var(--space-6);
}

.footer-content {
  display: grid;
  grid-template-columns: 2fr repeat(3, 1fr);
  gap: var(--space-8);
  margin-bottom: var(--space-8);

  @media (max-width: 1024px) {
    grid-template-columns: repeat(2, 1fr);
  }

  @media (max-width: 640px) {
    grid-template-columns: 1fr;
    gap: var(--space-6);
  }
}

.footer-brand {
  .brand-logo {
    display: flex;
    align-items: center;
    gap: var(--space-3);
    margin-bottom: var(--space-4);
    color: var(--primary-600);
    font-size: var(--font-lg);
    font-weight: var(--font-semibold);

    svg {
      flex-shrink: 0;
    }
  }

  .brand-description {
    color: var(--gray-600);
    font-size: var(--font-sm);
    line-height: var(--leading-relaxed);
    max-width: 300px;

    .dark & {
      color: var(--gray-400);
    }
  }
}

.footer-links {
  .links-title {
    font-size: var(--font-sm);
    font-weight: var(--font-semibold);
    color: var(--gray-900);
    margin-bottom: var(--space-4);
    text-transform: uppercase;
    letter-spacing: 0.05em;

    .dark & {
      color: var(--gray-100);
    }
  }

  .links-list {
    list-style: none;
    padding: 0;
    margin: 0;

    li {
      margin-bottom: var(--space-3);

      &:last-child {
        margin-bottom: 0;
      }
    }

    a {
      color: var(--gray-600);
      font-size: var(--font-sm);
      text-decoration: none;
      transition: color var(--duration-fast);

      &:hover {
        color: var(--primary-600);
      }

      .dark & {
        color: var(--gray-400);

        &:hover {
          color: var(--primary-400);
        }
      }
    }
  }
}

.footer-bottom {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding-top: var(--space-6);
  border-top: 1px solid var(--gray-200);
  flex-wrap: wrap;
  gap: var(--space-4);

  .dark & {
    border-top-color: var(--gray-800);
  }

  @media (max-width: 640px) {
    flex-direction: column;
    text-align: center;
  }
}

.copyright {
  color: var(--gray-600);
  font-size: var(--font-sm);

  .dark & {
    color: var(--gray-400);
  }
}

.footer-meta {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  color: var(--gray-500);
  font-size: var(--font-xs);

  .dark & {
    color: var(--gray-500);
  }

  @media (max-width: 640px) {
    order: 3;
  }
}

.social-links {
  display: flex;
  align-items: center;
  gap: var(--space-3);

  @media (max-width: 640px) {
    order: 2;
  }
}

.social-link {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 36px;
  height: 36px;
  color: var(--gray-600);
  background: var(--gray-100);
  border-radius: var(--radius-full);
  transition: all var(--duration-fast);

  &:hover {
    color: var(--primary-600);
    background: var(--primary-50);
    transform: translateY(-2px);
  }

  .dark & {
    color: var(--gray-400);
    background: var(--gray-800);

    &:hover {
      color: var(--primary-400);
      background: var(--primary-900);
    }
  }
}

@media (prefers-reduced-motion: reduce) {
  .social-link {
    transition: none;

    &:hover {
      transform: none;
    }
  }
}
</style>
