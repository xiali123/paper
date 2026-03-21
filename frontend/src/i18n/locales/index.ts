// Re-export all translation files
export { default as zhCN } from './zh-CN.json';
export { default as enUS } from './en-US.json';

// Type definitions for translation keys
export interface TranslationResources {
  app: {
    title: string;
    description: string;
    keywords: string;
  };
  nav: {
    home: string;
    search: string;
    stats: string;
    settings: string;
    about: string;
  };
  home: {
    welcome: string;
    subtitle: string;
    quickSearch: string;
    searchPlaceholder: string;
    searchButton: string;
    popularSearches: string;
    recentPapers: string;
  };
  search: {
    title: string;
    description: string;
    keyword: string;
    keywordPlaceholder: string;
    search: string;
    searching: string;
    filters: string;
    year: string;
    allYears: string;
    level: string;
    allLevels: string;
    results: string;
    found: string;
    papers: string;
    duration: string;
    seconds: string;
    noResults: string;
    tryOtherKeywords: string;
  };
  stats: {
    title: string;
    overview: string;
    totalPapers: string;
    totalJournals: string;
    topTierPapers: string;
    papersLastYear: string;
    mostActiveJournal: string;
    refresh: string;
    export: string;
    exportCSV: string;
    exportJSON: string;
  };
  paper: {
    title: string;
    journal: string;
    year: string;
    authors: string;
    level: string;
    abstract: string;
    doi: string;
    url: string;
    ccfLevel: string;
    viewDetails: string;
    cite: string;
    download: string;
  };
  level: {
    A: string;
    B: string;
    C: string;
    Unknown: string;
  };
  theme: {
    toggle: string;
    light: string;
    dark: string;
    auto: string;
  };
  backend: {
    status: string;
    connected: string;
    disconnected: string;
    checking: string;
  };
  footer: {
    copyright: string;
    poweredBy: string;
    version: string;
    docs: string;
    github: string;
  };
  common: {
    loading: string;
    error: string;
    success: string;
    cancel: string;
    confirm: string;
    save: string;
    delete: string;
    edit: string;
    close: string;
    back: string;
    next: string;
    previous: string;
    submit: string;
    reset: string;
  };
  error: {
    networkError: string;
    serverError: string;
    notFound: string;
    unauthorized: string;
    forbidden: string;
    unknownError: string;
  };
  settings: {
    title: string;
    language: string;
    theme: string;
    apiUrl: string;
    timeout: string;
    save: string;
    reset: string;
  };
}
