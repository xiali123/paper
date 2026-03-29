/**
 * Statistics Data Adapter
 *
 * Handles transformation between backend system metrics and frontend business statistics.
 *
 * @module api/adapters/statsAdapter
 */

// ============================================================================
// Backend Type Definitions (from C++ StatsApiModule)
// ============================================================================

/**
 * Backend system resources data
 */
interface BackendSystemResources {
  cpuUsagePercent: number
  memoryUsagePercent: number
  memoryTotal: number
  memoryUsed: number
  memoryAvailable: number
  diskUsagePercent: number
  diskTotal: number
  diskUsed: number
  diskAvailable: number
  networkRecvBytes: number
  networkSentBytes: number
  loadAverage1m: number
  loadAverage5m: number
  loadAverage15m: number
}

/**
 * Backend system information
 */
interface BackendSystemInfo {
  hostname: string
  osType: string
  osVersion: string
  osArchitecture: string
  cpuModel: string
  cpuCores: number
  cpuFrequency: number
  totalMemory: number
  kernelVersion: string
  cppVersion: string
}

/**
 * Backend module information
 */
interface BackendModuleInfo {
  name: string
  version: string
  type: string
  state: string
  loadedAt: string
}

/**
 * Backend performance metrics
 */
interface BackendPerformanceMetrics {
  requestCounts: Record<string, number>
  averageResponseTimes: Record<string, number>
  throughput: Record<string, number>
  errorCounts: Record<string, number>
  p50Latency: Record<string, number>
  p95Latency: Record<string, number>
  p99Latency: Record<string, number>
}

/**
 * Backend paper statistics
 */
interface BackendPaperStats {
  totalPapers: number
  readPapers: number
  unreadPapers: number
  favoritePapers: number
  papersByYear: Record<number, number>
  papersByJournal: Record<string, number>
  papersByAuthor: Record<string, number>
  papersByTag: Record<string, number>
}

// ============================================================================
// Frontend Type Definitions (from frontend/src/types)
// ============================================================================

/**
 * Frontend overview statistics
 */
export interface Statistics {
  totalPapers: number
  totalJournals: number
  topTierPapers: number
  papersLastYear: number
  mostActiveJournal: string
  averagePapersPerYear?: number
  latestUpdate?: string
  yearRange?: string
}

/**
 * Frontend journal statistics
 */
export interface JournalStats {
  journal: string
  count: number
  percentage: number
}

/**
 * Frontend year statistics
 */
export interface YearStats {
  year: number
  count: number
  percentage: number
}

/**
 * Frontend author statistics
 */
export interface AuthorStats {
  author: string
  count: number
  percentage: number
}

// ============================================================================
// Transformation Functions
// ============================================================================

/**
 * Transform backend paper statistics to frontend overview statistics
 */
export const transformOverviewStats = (backendStats: BackendPaperStats): Statistics => {
  const currentYear = new Date().getFullYear()
  const lastYearPapers = backendStats.papersByYear[currentYear - 1] || 0

  // Find most active journal
  let mostActiveJournal = ''
  let maxCount = 0
  for (const [journal, count] of Object.entries(backendStats.papersByJournal)) {
    if (count > maxCount) {
      maxCount = count
      mostActiveJournal = journal
    }
  }

  // Calculate year range
  const years = Object.keys(backendStats.papersByYear).map(Number).sort((a, b) => a - b)
  const yearRange = years.length > 0
    ? `${Math.min(...years)}-${Math.max(...years)}`
    : ''

  // Calculate average papers per year
  const totalYears = years.length || 1
  const averagePapersPerYear = Math.round(backendStats.totalPapers / totalYears)

  return {
    totalPapers: backendStats.totalPapers,
    totalJournals: Object.keys(backendStats.papersByJournal).length,
    topTierPapers: backendStats.favoritePapers, // Using favorite as proxy for top-tier
    papersLastYear: lastYearPapers,
    mostActiveJournal,
    averagePapersPerYear,
    latestUpdate: new Date().toISOString(),
    yearRange
  }
}

/**
 * Transform backend papersByJournal to frontend journal statistics
 */
export const transformJournalStats = (backendStats: BackendPaperStats): JournalStats[] => {
  const total = backendStats.totalPapers || 1

  return Object.entries(backendStats.papersByJournal)
    .map(([journal, count]) => ({
      journal,
      count,
      percentage: Math.round((count / total) * 100 * 100) / 100 // Round to 2 decimal places
    }))
    .sort((a, b) => b.count - a.count) // Sort by count descending
}

/**
 * Transform backend papersByYear to frontend year statistics
 */
export const transformYearStats = (backendStats: BackendPaperStats): YearStats[] => {
  const total = backendStats.totalPapers || 1

  return Object.entries(backendStats.papersByYear)
    .map(([year, count]) => ({
      year: parseInt(year, 10),
      count,
      percentage: Math.round((count / total) * 100 * 100) / 100
    }))
    .sort((a, b) => b.year - a.year) // Sort by year descending
}

/**
 * Transform backend papersByAuthor to frontend author statistics
 */
export const transformAuthorStats = (
  backendStats: BackendPaperStats,
  limit: number = 50
): AuthorStats[] => {
  const total = backendStats.totalPapers || 1

  return Object.entries(backendStats.papersByAuthor)
    .map(([author, count]) => ({
      author,
      count,
      percentage: Math.round((count / total) * 100 * 100) / 100
    }))
    .sort((a, b) => b.count - a.count) // Sort by count descending
    .slice(0, limit) // Limit to top N authors
}

/**
 * Transform system resources to health status
 */
export const transformSystemResources = (resources: BackendSystemResources) => {
  return {
    cpu: resources.cpuUsagePercent,
    memory: resources.memoryUsagePercent,
    disk: resources.diskUsagePercent,
    networkIn: bytesToKB(resources.networkRecvBytes),
    networkOut: bytesToKB(resources.networkSentBytes),
    loadAverage: {
      '1m': resources.loadAverage1m,
      '5m': resources.loadAverage5m,
      '15m': resources.loadAverage15m
    }
  }
}

/**
 * Transform system info to server information
 */
export const transformSystemInfo = (info: BackendSystemInfo) => {
  return {
    hostname: info.hostname,
    os: {
      type: info.osType,
      version: info.osVersion,
      architecture: info.osArchitecture,
      kernel: info.kernelVersion
    },
    cpu: {
      model: info.cpuModel,
      cores: info.cpuCores,
      frequency: info.cpuFrequency
    },
    memory: {
      total: bytesToGB(info.totalMemory)
    },
    runtime: {
      version: info.cppVersion
    }
  }
}

/**
 * Transform module info list to service status
 */
export const transformModuleStatus = (modules: BackendModuleInfo[]) => {
  return modules.map(module => ({
    name: module.name,
    version: module.version,
    type: module.type,
    status: module.state.toLowerCase(),
    loadedAt: module.loadedAt
  }))
}

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Convert bytes to KB
 */
const bytesToKB = (bytes: number): number => {
  return Math.round(bytes / 1024)
}

/**
 * Convert bytes to GB
 */
const bytesToGB = (bytes: number): number => {
  return Math.round(bytes / (1024 * 1024 * 1024) * 100) / 100
}

/**
 * Safe parse with fallback
 */
const safeParse = (value: string, fallback: any = 0): any => {
  try {
    return JSON.parse(value)
  } catch {
    return fallback
  }
}
