/**
 * Paper Adapter Tests
 *
 * Tests for frontend-backend Paper object transformations
 */

import {
  toFrontendPaper,
  transformPaperList,
  toBackendPaper,
  transformCreateRequest,
  transformUpdateRequest,
  transformQueryParams,
  type BackendPaper,
  type FrontendPaper
} from '../paperAdapter'

describe('Paper Adapter', () => {
  describe('toFrontendPaper', () => {
    it('should transform backend paper to frontend paper', () => {
      const backendPaper: BackendPaper = {
        id: 1,
        title: 'Test Paper',
        authors: 'John Doe',
        year: 2024,
        abstract: 'Test abstract',
        journal: 'Test Journal',
        volume: '1',
        issue: '1',
        pages: '1-10',
        doi: '10.1234/test',
        url: 'https://example.com',
        pdf_path: '/path/to/pdf.pdf',
        created_at: '2024-01-01T00:00:00Z',
        updated_at: '2024-01-01T00:00:00Z',
        tags: ['AI', 'ML'],
        keywords: ['deep learning', 'neural networks'],
        citation_count: 10,
        is_read: false,
        is_favorite: true,
        notes: 'Test notes'
      }

      const frontendPaper = toFrontendPaper(backendPaper)

      expect(frontendPaper).toEqual({
        id: 1,
        userId: 0,
        title: 'Test Paper',
        authors: 'John Doe',
        abstract: 'Test abstract',
        keywords: 'deep learning, neural networks',
        doi: '10.1234/test',
        publication: 'Test Journal',
        year: '2024',
        volume: '1',
        issue: '1',
        pages: '1-10',
        url: 'https://example.com',
        pdfPath: '/path/to/pdf.pdf',
        source: 'manual',
        category: '',
        tags: 'AI, ML',
        citationCount: 10,
        isRead: false,
        isBookmarked: true,
        readingProgress: 0,
        notes: 'Test notes',
        createdAt: '2024-01-01T00:00:00Z',
        updatedAt: '2024-01-01T00:00:00Z'
      })
    })

    it('should handle empty arrays and undefined fields', () => {
      const backendPaper: BackendPaper = {
        id: 1,
        title: 'Test Paper',
        authors: '',
        year: 0,
        abstract: '',
        journal: '',
        volume: '',
        issue: '',
        pages: '',
        doi: '',
        url: '',
        pdf_path: '',
        created_at: '2024-01-01T00:00:00Z',
        updated_at: '2024-01-01T00:00:00Z',
        tags: [],
        keywords: [],
        citation_count: 0,
        is_read: false,
        is_favorite: false,
        notes: ''
      }

      const frontendPaper = toFrontendPaper(backendPaper)

      expect(frontendPaper.tags).toBe('')
      expect(frontendPaper.keywords).toBe('')
      expect(frontendPaper.year).toBe('0')
    })
  })

  describe('transformPaperList', () => {
    it('should transform array of backend papers', () => {
      const backendPapers: BackendPaper[] = [
        {
          id: 1,
          title: 'Paper 1',
          authors: 'Author 1',
          year: 2024,
          abstract: 'Abstract 1',
          journal: 'Journal 1',
          volume: '1',
          issue: '1',
          pages: '1-10',
          doi: '10.1234/1',
          url: 'https://example.com/1',
          pdf_path: '/path/1.pdf',
          created_at: '2024-01-01T00:00:00Z',
          updated_at: '2024-01-01T00:00:00Z',
          tags: ['AI'],
          keywords: ['ml'],
          citation_count: 5,
          is_read: false,
          is_favorite: false,
          notes: ''
        },
        {
          id: 2,
          title: 'Paper 2',
          authors: 'Author 2',
          year: 2023,
          abstract: 'Abstract 2',
          journal: 'Journal 2',
          volume: '2',
          issue: '2',
          pages: '11-20',
          doi: '10.1234/2',
          url: 'https://example.com/2',
          pdf_path: '/path/2.pdf',
          created_at: '2024-01-01T00:00:00Z',
          updated_at: '2024-01-01T00:00:00Z',
          tags: ['ML'],
          keywords: ['ai'],
          citation_count: 3,
          is_read: true,
          is_favorite: true,
          notes: 'Notes'
        }
      ]

      const frontendPapers = transformPaperList(backendPapers)

      expect(frontendPapers).toHaveLength(2)
      expect(frontendPapers[0].title).toBe('Paper 1')
      expect(frontendPapers[1].title).toBe('Paper 2')
      expect(frontendPapers[0].publication).toBe('Journal 1')
      expect(frontendPapers[1].isBookmarked).toBe(true)
    })
  })

  describe('toBackendPaper', () => {
    it('should transform frontend paper to backend request', () => {
      const frontendPaper: Partial<FrontendPaper> = {
        title: 'Test Paper',
        authors: 'John Doe',
        year: '2024',
        publication: 'Test Journal',
        tags: 'AI, ML',
        keywords: 'deep learning, neural networks',
        isBookmarked: true,
        isRead: false
      }

      const backendRequest = toBackendPaper(frontendPaper)

      expect(backendRequest).toEqual({
        title: 'Test Paper',
        authors: 'John Doe',
        year: 2024,
        journal: 'Test Journal',
        tags: ['AI', 'ML'],
        keywords: ['deep learning', 'neural networks'],
        is_favorite: true,
        is_read: false
      })
    })

    it('should handle empty strings for tags and keywords', () => {
      const frontendPaper: Partial<FrontendPaper> = {
        title: 'Test Paper',
        tags: '',
        keywords: ''
      }

      const backendRequest = toBackendPaper(frontendPaper)

      expect(backendRequest.tags).toEqual([])
      expect(backendRequest.keywords).toEqual([])
    })

    it('should omit frontend-specific fields', () => {
      const frontendPaper: Partial<FrontendPaper> = {
        title: 'Test Paper',
        userId: 123,
        source: 'ieee',
        category: 'AI',
        readingProgress: 50
      }

      const backendRequest = toBackendPaper(frontendPaper)

      expect(backendRequest.title).toBe('Test Paper')
      expect(backendRequest).not.toHaveProperty('userId')
      expect(backendRequest).not.toHaveProperty('source')
      expect(backendRequest).not.toHaveProperty('category')
      expect(backendRequest).not.toHaveProperty('readingProgress')
    })
  })

  describe('transformCreateRequest', () => {
    it('should transform create request', () => {
      const createRequest = {
        title: 'New Paper',
        authors: 'Jane Doe',
        publication: 'Nature',
        year: '2024',
        tags: 'AI, ML'
      }

      const backendRequest = transformCreateRequest(createRequest)

      expect(backendRequest).toEqual({
        title: 'New Paper',
        authors: 'Jane Doe',
        journal: 'Nature',
        year: 2024,
        tags: ['AI', 'ML']
      })
    })

    it('should throw error for missing title', () => {
      expect(() => {
        transformCreateRequest({} as any)
      }).toThrow('Title is required')
    })

    it('should throw error for empty title', () => {
      expect(() => {
        transformCreateRequest({ title: '   ' })
      }).toThrow('Title is required')
    })
  })

  describe('transformUpdateRequest', () => {
    it('should transform update request', () => {
      const updateRequest = {
        title: 'Updated Title',
        isRead: true,
        citationCount: 15
      }

      const backendRequest = transformUpdateRequest(updateRequest)

      expect(backendRequest).toEqual({
        title: 'Updated Title',
        is_read: true,
        citation_count: 15
      })
    })

    it('should handle partial updates', () => {
      const updateRequest = {
        notes: 'New notes'
      }

      const backendRequest = transformUpdateRequest(updateRequest)

      expect(backendRequest).toEqual({
        notes: 'New notes'
      })
      expect(Object.keys(backendRequest)).toHaveLength(1)
    })
  })

  describe('transformQueryParams', () => {
    it('should transform query parameters', () => {
      const params = {
        keyword: 'machine learning',
        category: 'AI',
        isRead: false,
        isBookmarked: true,
        orderBy: 'createdAt',
        order: 'DESC' as const,
        page: 1,
        pageSize: 20
      }

      const backendParams = transformQueryParams(params)

      expect(backendParams).toEqual({
        query: 'machine learning',
        category: 'AI',
        is_read: false,
        is_favorite: true,
        sort_by: 'created_at',
        ascending: false,
        page: 1,
        limit: 20
      })
    })

    it('should handle tags as comma-separated string', () => {
      const params = {
        tags: 'AI, ML, NLP'
      }

      const backendParams = transformQueryParams(params)

      expect(backendParams.tags).toEqual(['AI', 'ML', 'NLP'])
    })

    it('should convert orderBy to backend format', () => {
      const params1 = { orderBy: 'citationCount' }
      const backendParams1 = transformQueryParams(params1)
      expect(backendParams1.sort_by).toBe('citation_count')

      const params2 = { orderBy: 'updatedAt' }
      const backendParams2 = transformQueryParams(params2)
      expect(backendParams2.sort_by).toBe('updated_at')
    })

    it('should filter out undefined values', () => {
      const params = {
        keyword: 'test',
        category: undefined,
        page: undefined
      }

      const backendParams = transformQueryParams(params)

      expect(backendParams).toHaveProperty('query')
      expect(backendParams).not.toHaveProperty('category')
      expect(backendParams).not.toHaveProperty('page')
    })
  })

  describe('Edge Cases', () => {
    it('should handle invalid year string', () => {
      const result = toBackendPaper({ year: 'invalid' })
      expect(result.year).toBe(0)
    })

    it('should handle whitespace in tags', () => {
      const result = toBackendPaper({ tags: 'AI ,  ML , NLP' })
      expect(result.tags).toEqual(['AI', 'ML', 'NLP'])
    })

    it('should handle Unix timestamp', () => {
      const backendPaper: BackendPaper = {
        id: 1,
        title: 'Test',
        authors: '',
        year: 2024,
        abstract: '',
        journal: '',
        volume: '',
        issue: '',
        pages: '',
        doi: '',
        url: '',
        pdf_path: '',
        created_at: '1609459200', // Unix timestamp
        updated_at: '1609459200',
        tags: [],
        keywords: [],
        citation_count: 0,
        is_read: false,
        is_favorite: false,
        notes: ''
      }

      const frontendPaper = toFrontendPaper(backendPaper)
      expect(frontendPaper.createdAt).toMatch(/^\d{4}-\d{2}-\d{2}T/)
    })

    it('should handle zero values correctly', () => {
      const backendPaper: BackendPaper = {
        id: 1,
        title: 'Test',
        authors: '',
        year: 0,
        abstract: '',
        journal: '',
        volume: '',
        issue: '',
        pages: '',
        doi: '',
        url: '',
        pdf_path: '',
        created_at: '2024-01-01T00:00:00Z',
        updated_at: '2024-01-01T00:00:00Z',
        tags: [],
        keywords: [],
        citation_count: 0,
        is_read: false,
        is_favorite: false,
        notes: ''
      }

      const frontendPaper = toFrontendPaper(backendPaper)
      expect(frontendPaper.citationCount).toBe(0)
      expect(frontendPaper.year).toBe('0')
      expect(frontendPaper.isRead).toBe(false)
    })
  })
})
