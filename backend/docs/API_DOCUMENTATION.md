# PaperCrawler REST API Documentation

## Base URL
```
http://localhost:8080
```

## Overview
PaperCrawler API provides access to academic paper database with search, filtering, and export capabilities.

## Authentication
Currently, the API does not require authentication. (Future versions may include API keys)

## Response Format
All successful responses follow this structure:
```json
{
  "success": true,
  "data": { ... },
  "timestamp": 1710987654
}
```

Error responses:
```json
{
  "success": false,
  "error": "ERROR_CODE",
  "message": "Human-readable error message",
  "timestamp": 1710987654
}
```

---

## Endpoints

### 1. Health Check
Check API and database connectivity status.

**Endpoint:** `GET /health`

**Response:**
```json
{
  "status": "healthy",
  "service": "PaperCrawler API",
  "version": "1.0.0",
  "database": "connected",
  "uptime": 1710987654,
  "timestamp": 1710987654
}
```

**cURL Example:**
```bash
curl http://localhost:8080/health
```

---

### 2. Search Papers
Search for papers by keyword with optional filters.

**Endpoint:** `GET /api/search`

**Query Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| q | string | No | - | Search keyword (paper type field) |
| year | string | No | - | Filter by year |
| level | string | No | - | Filter by level (A/B/C) |
| offset | integer | No | 0 | Pagination offset |
| limit | integer | No | 20 | Results per page (max 100) |

**Response:**
```json
{
  "success": true,
  "data": {
    "papers": [
      {
        "id": 1,
        "kid": 0,
        "type": "computer vision",
        "title": "Deep Learning for Image Recognition",
        "journal_full": "Conference on Computer Vision and Pattern Recognition",
        "journal_short": "CVPR",
        "year": "2023",
        "author": "John Doe, Jane Smith",
        "journal_url": "https://cvf.com",
        "doi_url": "https://doi.org/10.1234/example",
        "info": "Published in proceedings",
        "qkid": 123,
        "level": "A"
      }
    ],
    "pagination": {
      "offset": 0,
      "limit": 20,
      "total": 150
    },
    "query": {
      "keyword": "deep learning",
      "year": "",
      "level": "A"
    },
    "duration_ms": 45.32
  },
  "timestamp": 1710987654
}
```

**cURL Examples:**
```bash
# Basic search
curl "http://localhost:8080/api/search?q=deep+learning"

# Search with filters
curl "http://localhost:8080/api/search?q=computer+vision&year=2023&level=A&offset=0&limit=10"

# Pagination
curl "http://localhost:8080/api/search?q=machine+learning&offset=20&limit=20"
```

---

### 3. Get Paper Details
Retrieve detailed information about a specific paper.

**Endpoint:** `GET /api/papers/{id}`

**Path Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| id | integer | Yes | Paper ID |

**Response:**
```json
{
  "success": true,
  "data": {
    "id": 1,
    "kid": 0,
    "type": "computer vision",
    "title": "Deep Learning for Image Recognition",
    "journal_full": "Conference on Computer Vision and Pattern Recognition",
    "journal_short": "CVPR",
    "year": "2023",
    "author": "John Doe, Jane Smith",
    "journal_url": "https://cvf.com",
    "doi_url": "https://doi.org/10.1234/example",
    "info": "Published in proceedings",
    "qkid": 123,
    "level": "A"
  },
  "timestamp": 1710987654
}
```

**cURL Example:**
```bash
curl http://localhost:8080/api/papers/123
```

---

### 4. Get Recent Papers
Retrieve the most recent papers sorted by year.

**Endpoint:** `GET /api/papers/recent`

**Query Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| limit | integer | No | 20 | Number of results (max 100) |

**Response:**
```json
{
  "success": true,
  "data": [
    {
      "id": 456,
      "title": "Latest Research in AI",
      "year": "2024",
      ...
    }
  ],
  "timestamp": 1710987654
}
```

**cURL Example:**
```bash
curl "http://localhost:8080/api/papers/recent?limit=50"
```

---

### 5. Batch Get Papers
Retrieve multiple papers in a single request.

**Endpoint:** `POST /api/papers/batch`

**Request Body:**
```json
{
  "ids": [1, 2, 3, 45, 123]
}
```

**Response:**
```json
{
  "success": true,
  "data": [
    { "id": 1, "title": "...", ... },
    { "id": 2, "title": "...", ... },
    { "id": 3, "title": "...", ... }
  ],
  "timestamp": 1710987654
}
```

**cURL Example:**
```bash
curl -X POST http://localhost:8080/api/papers/batch \
  -H "Content-Type: application/json" \
  -d '{"ids":[1,2,3,45,123]}'
```

---

### 6. Statistics Overview
Get overall statistics about the database.

**Endpoint:** `GET /api/stats/overview`

**Response:**
```json
{
  "success": true,
  "data": {
    "total_papers": 15234,
    "total_journals": 345,
    "top_tier_papers": 5421,
    "papers_last_year": 2341,
    "most_active_journal": "CVPR"
  },
  "timestamp": 1710987654
}
```

**cURL Example:**
```bash
curl http://localhost:8080/api/stats/overview
```

---

### 7. Export to CSV
Export papers to CSV format.

**Endpoint:** `GET /api/export/csv`

**Query Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| q | string | No | - | Search keyword |
| limit | integer | No | 1000 | Maximum papers to export |

**Response:** CSV file download

**cURL Examples:**
```bash
# Export all papers
curl -o papers.csv http://localhost:8080/api/export/csv

# Export filtered papers
curl -o ml_papers.csv "http://localhost:8080/api/export/csv?q=machine+learning&limit=500"
```

---

### 8. Export to JSON
Export papers to JSON format.

**Endpoint:** `GET /api/export/json`

**Query Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| q | string | No | - | Search keyword |
| limit | integer | No | 1000 | Maximum papers to export |

**Response:** JSON file download

**cURL Examples:**
```bash
# Export all papers
curl -o papers.json http://localhost:8080/api/export/json

# Export filtered papers
curl -o cv_papers.json "http://localhost:8080/api/export/json?q=computer+vision&limit=200"
```

---

### 9. Export to BibTeX
Export a single paper to BibTeX format.

**Endpoint:** `GET /api/export/bibtex/{id}`

**Path Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| id | integer | Yes | Paper ID |

**Response:** BibTeX file (.bib)

**cURL Example:**
```bash
curl -o paper_123.bib http://localhost:8080/api/export/bibtex/123
```

**Example BibTeX Output:**
```bibtex
@article{doe2023deep,
  title={Deep Learning for Image Recognition},
  author={Doe, John and Smith, Jane},
  journal={Conference on Computer Vision and Pattern Recognition},
  year={2023},
  doi={10.1234/example}
}
```

---

## Error Codes

| Code | Description |
|------|-------------|
| 200 | Success |
| 400 | Bad Request (invalid parameters) |
| 404 | Not Found |
| 500 | Internal Server Error |
| API_NOT_INITIALIZED | Database connection not available |
| DATABASE_ERROR | Database operation failed |
| INVALID_ID | Invalid paper ID format |
| PAPER_NOT_FOUND | Paper with specified ID not found |
| INTERNAL_ERROR | Unexpected server error |

---

## Rate Limiting
Currently, there are no rate limits. (Future versions may implement rate limiting)

## CORS
The API supports CORS for all origins. All responses include:
```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization
```

## Logging
All requests are logged with:
- Timestamp
- HTTP method
- Request path
- Response status code
- Response time (milliseconds)

Example log output:
```
[Thu Mar 21 10:30:45 2026] GET /api/search?q=deep+learning → 200 (45ms)
```

---

## Examples

### Python
```python
import requests

# Search papers
response = requests.get('http://localhost:8080/api/search', params={
    'q': 'deep learning',
    'year': '2023',
    'level': 'A',
    'limit': 10
})
data = response.json()
print(f"Found {data['data']['pagination']['total']} papers")

# Get paper details
paper_id = 123
response = requests.get(f'http://localhost:8080/api/papers/{paper_id}')
paper = response.json()['data']
print(f"Title: {paper['title']}")

# Export to CSV
response = requests.get('http://localhost:8080/api/export/csv', params={
    'q': 'computer vision',
    'limit': 100
})
with open('papers.csv', 'wb') as f:
    f.write(response.content)
```

### JavaScript (Fetch)
```javascript
// Search papers
async function searchPapers(keyword) {
  const response = await fetch(
    `http://localhost:8080/api/search?q=${encodeURIComponent(keyword)}&limit=10`
  );
  const data = await response.json();
  console.log('Papers:', data.data.papers);
  return data;
}

// Get paper details
async function getPaperDetails(paperId) {
  const response = await fetch(`http://localhost:8080/api/papers/${paperId}`);
  const data = await response.json();
  return data.data;
}

// Export to JSON
async function exportPapers(keyword) {
  const response = await fetch(
    `http://localhost:8080/api/export/json?q=${encodeURIComponent(keyword)}`
  );
  const blob = await response.blob();
  const url = window.URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'papers.json';
  a.click();
}
```

---

## Building and Running

### Build
```bash
cd backend
mkdir build && cd build
cmake ..
make
```

### Run
```bash
./PaperCrawlerServer
```

### Docker
```bash
docker build -t papercrawler-api .
docker run -p 8080:8080 papercrawler-api
```

---

## Support
For issues or questions, please visit the project repository.
