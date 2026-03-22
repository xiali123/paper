/**
 * PaperCrawler API Client Example (Node.js)
 * Demonstrates how to interact with the PaperCrawler REST API
 */

const http = require('http');
const https = require('https');
const fs = require('fs');
const { URL } = require('url');

class PaperCrawlerClient {
    constructor(baseUrl = 'http://localhost:8080') {
        this.baseUrl = baseUrl.replace(/\/$/, '');
    }

    async request(method, path, data = null) {
        return new Promise((resolve, reject) => {
            const url = new URL(path, this.baseUrl);
            const options = {
                method: method,
                headers: {
                    'Content-Type': 'application/json',
                }
            };

            const protocol = url.protocol === 'https:' ? https : http;

            const req = protocol.request(url, options, (res) => {
                let body = '';

                res.on('data', (chunk) => {
                    body += chunk;
                });

                res.on('end', () => {
                    try {
                        const jsonData = JSON.parse(body);
                        if (res.statusCode >= 200 && res.statusCode < 300) {
                            resolve(jsonData);
                        } else {
                            reject(new Error(`HTTP ${res.statusCode}: ${jsonData.error || 'Unknown error'}`));
                        }
                    } catch (e) {
                        reject(new Error(`Failed to parse response: ${e.message}`));
                    }
                });
            });

            req.on('error', reject);

            if (data) {
                req.write(JSON.stringify(data));
            }

            req.end();
        });
    }

    async downloadFile(path, filename) {
        return new Promise((resolve, reject) => {
            const url = new URL(path, this.baseUrl);
            const file = fs.createWriteStream(filename);

            const protocol = url.protocol === 'https:' ? https : http;

            protocol.get(url, (response) => {
                response.pipe(file);

                file.on('finish', () => {
                    file.close();
                    resolve(filename);
                });
            }).on('error', (err) => {
                fs.unlink(filename, () => {});
                reject(err);
            });
        });
    }

    async healthCheck() {
        return this.request('GET', '/health');
    }

    async searchPapers(params = {}) {
        const { keyword = '', year = '', level = '', offset = 0, limit = 20 } = params;
        const query = new URLSearchParams({
            q: keyword,
            year,
            level,
            offset: offset.toString(),
            limit: limit.toString()
        });
        return this.request('GET', `/api/search?${query}`);
    }

    async getPaper(paperId) {
        return this.request('GET', `/api/papers/${paperId}`);
    }

    async getRecentPapers(limit = 20) {
        return this.request('GET', `/api/papers/recent?limit=${limit}`);
    }

    async batchGetPapers(paperIds) {
        return this.request('POST', '/api/papers/batch', { ids: paperIds });
    }

    async getStatistics() {
        return this.request('GET', '/api/stats/overview');
    }

    async exportCsv(params = {}) {
        const { keyword = '', limit = 1000, filename = 'papers.csv' } = params;
        const query = new URLSearchParams({ q: keyword, limit: limit.toString() });
        return this.downloadFile(`/api/export/csv?${query}`, filename);
    }

    async exportJson(params = {}) {
        const { keyword = '', limit = 1000, filename = 'papers.json' } = params;
        const query = new URLSearchParams({ q: keyword, limit: limit.toString() });
        return this.downloadFile(`/api/export/json?${query}`, filename);
    }

    async exportBibtex(paperId, filename = null) {
        if (!filename) {
            filename = `paper_${paperId}.bib`;
        }
        return this.downloadFile(`/api/export/bibtex/${paperId}`, filename);
    }
}

// Example usage
async function main() {
    const client = new PaperCrawlerClient();

    console.log('='.repeat(50));
    console.log('PaperCrawler API Client Examples');
    console.log('='.repeat(50));
    console.log();

    // 1. Health Check
    console.log('1. Health Check');
    console.log('-'.repeat(50));
    try {
        const health = await client.healthCheck();
        console.log(`Status: ${health.status}`);
        console.log(`Database: ${health.database}`);
        console.log(`Version: ${health.version}`);
        console.log('✓ API is healthy\n');
    } catch (error) {
        console.log(`✗ Health check failed: ${error.message}\n`);
        return;
    }

    // 2. Search Papers
    console.log('2. Search Papers');
    console.log('-'.repeat(50));
    try {
        const results = await client.searchPapers({
            keyword: 'deep learning',
            year: '2023',
            level: 'A',
            limit: 5
        });

        const data = results.data;
        console.log(`Found ${data.pagination.total} papers`);
        console.log(`Showing ${data.papers.length} results:`);
        data.papers.forEach(paper => {
            console.log(`  - ${paper.title} (${paper.year})`);
        });
        console.log();
    } catch (error) {
        console.log(`✗ Search failed: ${error.message}\n`);
    }

    // 3. Get Paper Details
    console.log('3. Get Paper Details');
    console.log('-'.repeat(50));
    try {
        const paper = await client.getPaper(1);
        const data = paper.data;
        console.log(`Title: ${data.title}`);
        console.log(`Authors: ${data.author}`);
        console.log(`Journal: ${data.journal_short} (${data.year})`);
        console.log(`Level: ${data.level}`);
        console.log();
    } catch (error) {
        console.log(`✗ Get paper failed: ${error.message}\n`);
    }

    // 4. Get Recent Papers
    console.log('4. Get Recent Papers');
    console.log('-'.repeat(50));
    try {
        const recent = await client.getRecentPapers(5);
        const papers = recent.data;
        console.log(`Recent papers (showing ${papers.length}):`);
        papers.forEach(paper => {
            console.log(`  - ${paper.title} (${paper.year})`);
        });
        console.log();
    } catch (error) {
        console.log(`✗ Get recent papers failed: ${error.message}\n`);
    }

    // 5. Statistics
    console.log('5. Statistics Overview');
    console.log('-'.repeat(50));
    try {
        const stats = await client.getStatistics();
        const data = stats.data;
        console.log(`Total Papers: ${data.total_papers}`);
        console.log(`Total Journals: ${data.total_journals}`);
        console.log(`Top-tier Papers: ${data.top_tier_papers}`);
        console.log(`Papers Last Year: ${data.papers_last_year}`);
        console.log(`Most Active Journal: ${data.most_active_journal}`);
        console.log();
    } catch (error) {
        console.log(`✗ Get statistics failed: ${error.message}\n`);
    }

    // 6. Batch Get Papers
    console.log('6. Batch Get Papers');
    console.log('-'.repeat(50));
    try {
        const papers = await client.batchGetPapers([1, 2, 3]);
        const data = papers.data;
        console.log(`Retrieved ${data.length} papers:`);
        data.forEach(paper => {
            console.log(`  - ID ${paper.id}: ${paper.title}`);
        });
        console.log();
    } catch (error) {
        console.log(`✗ Batch get failed: ${error.message}\n`);
    }

    // 7. Export to CSV
    console.log('7. Export to CSV');
    console.log('-'.repeat(50));
    try {
        const filename = await client.exportCsv({
            keyword: 'machine learning',
            limit: 10,
            filename: 'example_papers.csv'
        });
        console.log(`✓ Exported to ${filename}`);
        console.log();
    } catch (error) {
        console.log(`✗ Export CSV failed: ${error.message}\n`);
    }

    // 8. Export to BibTeX
    console.log('8. Export to BibTeX');
    console.log('-'.repeat(50));
    try {
        const filename = await client.exportBibtex(1, 'example_paper.bib');
        console.log(`✓ Exported to ${filename}`);

        const content = fs.readFileSync(filename, 'utf8');
        console.log(`Content preview:\n${content.substring(0, 200)}...`);
        console.log();
    } catch (error) {
        console.log(`✗ Export BibTeX failed: ${error.message}\n`);
    }

    console.log('='.repeat(50));
    console.log('Examples completed!');
    console.log('='.repeat(50));
}

// Run examples
main().catch(console.error);
