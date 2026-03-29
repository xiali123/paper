-- ============================================================================
-- PaperCrawler Database Schema (SQLite)
-- Migration: 001_init_schema_sqlite
-- Description: Initial schema for papers, journals, and authors
-- ============================================================================

-- ============================================================================
-- Papers Table
-- ============================================================================

CREATE TABLE IF NOT EXISTS papers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    authors TEXT NOT NULL,  -- JSON array of author names
    year INTEGER,
    publication TEXT,  -- Journal or conference name
    volume TEXT,
    issue TEXT,
    pages TEXT,
    doi TEXT UNIQUE,
    abstract TEXT,
    keywords TEXT,  -- JSON array of keywords
    citation_count INTEGER DEFAULT 0,
    pdf_path TEXT,
    pdf_available INTEGER DEFAULT 0,
    is_favorite INTEGER DEFAULT 0,
    is_read INTEGER DEFAULT 0,
    notes TEXT,
    tags TEXT,  -- JSON array of tags

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    imported_at INTEGER,
    last_accessed_at INTEGER
);

-- Indexes for papers
CREATE INDEX IF NOT EXISTS idx_papers_title ON papers(title);
CREATE INDEX IF NOT EXISTS idx_papers_authors ON papers(authors);
CREATE INDEX IF NOT EXISTS idx_papers_year ON papers(year);
CREATE INDEX IF NOT EXISTS idx_papers_publication ON papers(publication);
CREATE INDEX IF NOT EXISTS idx_papers_doi ON papers(doi);
CREATE INDEX IF NOT EXISTS idx_papers_is_favorite ON papers(is_favorite);
CREATE INDEX IF NOT EXISTS idx_papers_is_read ON papers(is_read);
CREATE INDEX IF NOT EXISTS idx_papers_created_at ON papers(created_at);

-- Full-text search index
CREATE VIRTUAL TABLE IF NOT EXISTS papers_fts USING fts5(
    title,
    authors,
    abstract,
    keywords,
    content=papers,
    content_rowid=id
);

-- ============================================================================
-- Journals Table
-- ============================================================================

CREATE TABLE IF NOT EXISTS journals (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT UNIQUE NOT NULL,
    publisher TEXT,
    issn TEXT,
    e_issn TEXT,
    url TEXT,
    description TEXT,
    impact_factor REAL,
    tier TEXT CHECK(tier IN ('Tier 1', 'Tier 2', 'Tier 3', 'Tier 4', 'Unknown')),
    subject_areas TEXT,  -- JSON array

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- Indexes for journals
CREATE INDEX IF NOT EXISTS idx_journals_name ON journals(name);
CREATE INDEX IF NOT EXISTS idx_journals_tier ON journals(tier);
CREATE INDEX IF NOT EXISTS idx_journals_impact_factor ON journals(impact_factor);

-- ============================================================================
-- Authors Table
-- ============================================================================

CREATE TABLE IF NOT EXISTS authors (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    email TEXT UNIQUE,
    affiliation TEXT,
    orcid TEXT UNIQUE,
    google_scholar_url TEXT,
    research_gate_url TEXT,

    -- Statistics
    paper_count INTEGER DEFAULT 0,
    citation_count INTEGER DEFAULT 0,
    h_index INTEGER,

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- Indexes for authors
CREATE INDEX IF NOT EXISTS idx_authors_name ON authors(name);
CREATE INDEX IF NOT EXISTS idx_authors_email ON authors(email);
CREATE INDEX IF NOT EXISTS idx_authors_orcid ON authors(orcid);
CREATE INDEX IF NOT EXISTS idx_authors_affiliation ON authors(affiliation);

-- ============================================================================
-- Paper-Author Junction Table
-- ============================================================================

CREATE TABLE IF NOT EXISTS paper_authors (
    paper_id INTEGER NOT NULL,
    author_id INTEGER NOT NULL,
    author_order INTEGER NOT NULL,  -- Order in author list
    is_corresponding INTEGER DEFAULT 0,

    PRIMARY KEY (paper_id, author_id),
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    FOREIGN KEY (author_id) REFERENCES authors(id) ON DELETE CASCADE
);

-- Indexes for paper_authors
CREATE INDEX IF NOT EXISTS idx_paper_authors_paper_id ON paper_authors(paper_id);
CREATE INDEX IF NOT EXISTS idx_paper_authors_author_id ON paper_authors(author_id);
CREATE INDEX IF NOT EXISTS idx_paper_authors_order ON paper_authors(author_order);

-- ============================================================================
-- Collections Table
-- ============================================================================

CREATE TABLE IF NOT EXISTS collections (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT,
    is_public INTEGER DEFAULT 0,
    paper_count INTEGER DEFAULT 0,

    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- Indexes for collections
CREATE INDEX IF NOT EXISTS idx_collections_name ON collections(name);
CREATE INDEX IF NOT EXISTS idx_collections_is_public ON collections(is_public);

-- ============================================================================
-- Collection Papers Junction Table
-- ============================================================================

CREATE TABLE IF NOT EXISTS collection_papers (
    collection_id INTEGER NOT NULL,
    paper_id INTEGER NOT NULL,
    notes TEXT,
    added_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    PRIMARY KEY (collection_id, paper_id),
    FOREIGN KEY (collection_id) REFERENCES collections(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE
);

-- Indexes for collection_papers
CREATE INDEX IF NOT EXISTS idx_collection_papers_collection_id ON collection_papers(collection_id);
CREATE INDEX IF NOT EXISTS idx_collection_papers_paper_id ON collection_papers(paper_id);

-- ============================================================================
-- Search History Table
-- ============================================================================

CREATE TABLE IF NOT EXISTS search_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    query TEXT NOT NULL,
    filters TEXT,  -- JSON object
    result_count INTEGER DEFAULT 0,
    search_duration_ms INTEGER,

    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- Indexes for search_history
CREATE INDEX IF NOT EXISTS idx_search_history_query ON search_history(query);
CREATE INDEX IF NOT EXISTS idx_search_history_created_at ON search_history(created_at);

-- ============================================================================
-- Triggers for Data Integrity
-- ============================================================================

-- Trigger: Update paper timestamp on update
CREATE TRIGGER IF NOT EXISTS update_paper_timestamp
AFTER UPDATE ON papers
FOR EACH ROW
BEGIN
    UPDATE papers SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

-- Trigger: Update journal timestamp on update
CREATE TRIGGER IF NOT EXISTS update_journal_timestamp
AFTER UPDATE ON journals
FOR EACH ROW
BEGIN
    UPDATE journals SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

-- Trigger: Update author timestamp on update
CREATE TRIGGER IF NOT EXISTS update_author_timestamp
AFTER UPDATE ON authors
FOR EACH ROW
BEGIN
    UPDATE authors SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

-- Trigger: Update collection timestamp on update
CREATE TRIGGER IF NOT EXISTS update_collection_timestamp
AFTER UPDATE ON collections
FOR EACH ROW
BEGIN
    UPDATE collections SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

-- Trigger: Update paper count when adding to collection
CREATE TRIGGER IF NOT EXISTS update_collection_paper_count
AFTER INSERT ON collection_papers
FOR EACH ROW
BEGIN
    UPDATE collections SET paper_count = paper_count + 1 WHERE id = NEW.collection_id;
END;

-- Trigger: Update paper count when removing from collection
CREATE TRIGGER IF NOT EXISTS update_collection_paper_count_delete
AFTER DELETE ON collection_papers
FOR EACH ROW
BEGIN
    UPDATE collections SET paper_count = paper_count - 1 WHERE id = OLD.collection_id;
END;

-- Trigger: Update author paper count
CREATE TRIGGER IF NOT EXISTS update_author_paper_count
AFTER INSERT ON paper_authors
FOR EACH ROW
BEGIN
    UPDATE authors SET paper_count = paper_count + 1 WHERE id = NEW.author_id;
END;

-- Trigger: Update FTS index on paper insert/update
CREATE TRIGGER IF NOT EXISTS update_papers_fts_insert
AFTER INSERT ON papers
BEGIN
    INSERT INTO papers_fts(rowid, title, authors, abstract, keywords)
    VALUES (NEW.id, NEW.title, NEW.authors, NEW.abstract, NEW.keywords);
END;

CREATE TRIGGER IF NOT EXISTS update_papers_fts_update
AFTER UPDATE ON papers
BEGIN
    UPDATE papers_fts
    SET title = NEW.title,
        authors = NEW.authors,
        abstract = NEW.abstract,
        keywords = NEW.keywords
    WHERE rowid = NEW.id;
END;

-- ============================================================================
-- Migration Tracking
-- ============================================================================

CREATE TABLE IF NOT EXISTS migrations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    version TEXT UNIQUE NOT NULL,
    description TEXT,
    executed_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- Record this migration
INSERT OR IGNORE INTO migrations (version, description) VALUES
('001_init_schema_sqlite', 'Initial schema for papers, journals, and authors');
