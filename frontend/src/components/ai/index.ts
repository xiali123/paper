/**
 * AI Co-Pilot Components
 *
 * This module exports all AI-related Vue 3 + TypeScript components for the PaperCrawler AI Research Co-Pilot.
 *
 * Components:
 * - AIReviewInterface: AI reviewer system interface with scoring, recommendations, and detailed feedback
 * - LiteratureReviewDisplay: Literature review generator with PRISMA-aligned systematic reviews
 * - ResearchPlanVisualization: Research plan generator with SMART goals, methodology, and timeline visualization
 *
 * @module PaperCrawler/Components/AI
 * @version 1.0.0
 */

export { default as AIReviewInterface } from './AIReviewInterface.vue'
export { default as LiteratureReviewDisplay } from './LiteratureReviewDisplay.vue'
export { default as ResearchPlanVisualization } from './ResearchPlanVisualization.vue'

// Component metadata for dynamic loading
export const AIComponents = {
  AIReviewInterface: {
    name: 'AIReviewInterface',
    displayName: 'AI审稿人系统',
    description: '模拟顶级期刊审稿流程，提供专业评审意见',
    icon: 'fas fa-robot',
    route: '/ai/review',
    version: '1.0.0'
  },
  LiteratureReviewDisplay: {
    name: 'LiteratureReviewDisplay',
    displayName: 'AI文献综述生成器',
    description: '自动生成系统性文献综述，符合PRISMA指南',
    icon: 'fas fa-book-open',
    route: '/ai/literature-review',
    version: '1.0.0'
  },
  ResearchPlanVisualization: {
    name: 'ResearchPlanVisualization',
    displayName: 'AI研究计划助手',
    description: '生成完整的研究项目计划，包含SMART目标和方法论设计',
    icon: 'fas fa-clipboard-list',
    route: '/ai/research-plan',
    version: '1.0.0'
  }
}

// Type exports for TypeScript usage
export type { default as AIReviewInterface } from './AIReviewInterface.vue'
export type { default as LiteratureReviewDisplay } from './LiteratureReviewDisplay.vue'
export type { default as ResearchPlanVisualization } from './ResearchPlanVisualization.vue'
