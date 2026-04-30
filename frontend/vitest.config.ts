/**
 * Vitest 配置文件
 * 支持单元测试、组件测试、集成测试
 */

import { defineConfig } from 'vitest/config'
import vue from '@vitejs/plugin-vue'
import { fileURLToPath } from 'node:url'

export default defineConfig({
  plugins: [vue()],
  test: {
    globals: true,
    environment: 'jsdom',
    setupFiles: ['./tests/setup.ts'],
    include: [
      'tests/**/*.{test,spec}.{js,mjs,ts,mts}',
      'src/**/*.{test,spec}.{js,mjs,ts,mts}'
    ],
    exclude: [
      'node_modules',
      'dist',
      '.idea',
      '.git',
      '.cache'
    ],
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json', 'html', 'lcov', 'text-summary'],
      exclude: [
        'node_modules/',
        'tests/',
        '**/*.d.ts',
        '**/*.config.*',
        '**/mockData',
        'src/types/',
        'dist/'
      ],
      // 覆盖率目标
      lines: 75,
      functions: 75,
      branches: 70,
      statements: 75,
      // 覆盖所有文件
      all: true
    },
    testTimeout: 10000,
    hookTimeout: 10000,
    isolate: true,
    pool: 'threads',
    poolOptions: {
      threads: {
        singleThread: false,
        minThreads: 1,
        maxThreads: 4
      }
    },
    reporters: ['verbose', 'json', 'html', 'junit'],
    outputFile: {
      json: './test-results/results.json',
      html: './test-results/index.html',
      junit: './test-results/junit.xml'
    },
    // 并行测试
    parallel: true,
    // 失败时重试
    retry: 2,
    // 只运行变更的测试
    watchIgnore: [
      '**/node_modules/**',
      '**/dist/**',
      '**/test-results/**'
    ]
  },
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    }
  },
  // 优化构建
  define: {
    __VUE_OPTIONS_API__: true,
    __VUE_PROD_DEVTOOLS_TOOLS__: false
  }
})
