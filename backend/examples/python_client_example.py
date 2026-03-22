#!/usr/bin/env python3
"""
PaperCrawler API Client Example
Demonstrates how to interact with the PaperCrawler REST API
"""

import requests
import json
from typing import List, Dict, Optional

class PaperCrawlerClient:
    """Client for PaperCrawler REST API"""

    def __init__(self, base_url: str = "http://localhost:8080"):
        self.base_url = base_url.rstrip('/')
        self.session = requests.Session()

    def health_check(self) -> Dict:
        """Check API health status"""
        response = self.session.get(f"{self.base_url}/health")
        response.raise_for_status()
        return response.json()

    def search_papers(
        self,
        keyword: str = "",
        year: str = "",
        level: str = "",
        offset: int = 0,
        limit: int = 20
    ) -> Dict:
        """
        Search for papers

        Args:
            keyword: Search keyword
            year: Filter by year
            level: Filter by level (A/B/C)
            offset: Pagination offset
            limit: Results per page (max 100)
        """
        params = {
            "q": keyword,
            "year": year,
            "level": level,
            "offset": offset,
            "limit": limit
        }

        response = self.session.get(f"{self.base_url}/api/search", params=params)
        response.raise_for_status()
        return response.json()

    def get_paper(self, paper_id: int) -> Dict:
        """Get paper details by ID"""
        response = self.session.get(f"{self.base_url}/api/papers/{paper_id}")
        response.raise_for_status()
        return response.json()

    def get_recent_papers(self, limit: int = 20) -> Dict:
        """Get recent papers"""
        params = {"limit": limit}
        response = self.session.get(f"{self.base_url}/api/papers/recent", params=params)
        response.raise_for_status()
        return response.json()

    def batch_get_papers(self, paper_ids: List[int]) -> Dict:
        """Get multiple papers in one request"""
        data = {"ids": paper_ids}
        response = self.session.post(
            f"{self.base_url}/api/papers/batch",
            json=data,
            headers={"Content-Type": "application/json"}
        )
        response.raise_for_status()
        return response.json()

    def get_statistics(self) -> Dict:
        """Get overview statistics"""
        response = self.session.get(f"{self.base_url}/api/stats/overview")
        response.raise_for_status()
        return response.json()

    def export_csv(self, keyword: str = "", limit: int = 1000, filename: str = "papers.csv") -> str:
        """Export papers to CSV file"""
        params = {"q": keyword, "limit": limit}
        response = self.session.get(f"{self.base_url}/api/export/csv", params=params)
        response.raise_for_status()

        with open(filename, 'wb') as f:
            f.write(response.content)

        return filename

    def export_json(self, keyword: str = "", limit: int = 1000, filename: str = "papers.json") -> Dict:
        """Export papers to JSON file"""
        params = {"q": keyword, "limit": limit}
        response = self.session.get(f"{self.base_url}/api/export/json", params=params)
        response.raise_for_status()

        with open(filename, 'wb') as f:
            f.write(response.content)

        with open(filename, 'r') as f:
            return json.load(f)

    def export_bibtex(self, paper_id: int, filename: Optional[str] = None) -> str:
        """Export paper to BibTeX format"""
        if filename is None:
            filename = f"paper_{paper_id}.bib"

        response = self.session.get(f"{self.base_url}/api/export/bibtex/{paper_id}")
        response.raise_for_status()

        with open(filename, 'wb') as f:
            f.write(response.content)

        return filename


def main():
    """Example usage of the PaperCrawler client"""

    # Initialize client
    client = PaperCrawlerClient()

    print("=" * 50)
    print("PaperCrawler API Client Examples")
    print("=" * 50)
    print()

    # 1. Health Check
    print("1. Health Check")
    print("-" * 50)
    try:
        health = client.health_check()
        print(f"Status: {health['status']}")
        print(f"Database: {health['database']}")
        print(f"Version: {health['version']}")
        print("✓ API is healthy\n")
    except Exception as e:
        print(f"✗ Health check failed: {e}\n")
        return

    # 2. Search Papers
    print("2. Search Papers")
    print("-" * 50)
    try:
        results = client.search_papers(
            keyword="deep learning",
            year="2023",
            level="A",
            limit=5
        )
        data = results['data']
        print(f"Found {data['pagination']['total']} papers")
        print(f"Showing {len(data['papers'])} results:")
        for paper in data['papers']:
            print(f"  - {paper['title']} ({paper['year']})")
        print()
    except Exception as e:
        print(f"✗ Search failed: {e}\n")

    # 3. Get Paper Details
    print("3. Get Paper Details")
    print("-" * 50)
    try:
        paper = client.get_paper(1)
        data = paper['data']
        print(f"Title: {data['title']}")
        print(f"Authors: {data['author']}")
        print(f"Journal: {data['journal_short']} ({data['year']})")
        print(f"Level: {data['level']}")
        print()
    except Exception as e:
        print(f"✗ Get paper failed: {e}\n")

    # 4. Get Recent Papers
    print("4. Get Recent Papers")
    print("-" * 50)
    try:
        recent = client.get_recent_papers(limit=5)
        papers = recent['data']
        print(f"Recent papers (showing {len(papers)}):")
        for paper in papers:
            print(f"  - {paper['title']} ({paper['year']})")
        print()
    except Exception as e:
        print(f"✗ Get recent papers failed: {e}\n")

    # 5. Statistics
    print("5. Statistics Overview")
    print("-" * 50)
    try:
        stats = client.get_statistics()
        data = stats['data']
        print(f"Total Papers: {data['total_papers']}")
        print(f"Total Journals: {data['total_journals']}")
        print(f"Top-tier Papers: {data['top_tier_papers']}")
        print(f"Papers Last Year: {data['papers_last_year']}")
        print(f"Most Active Journal: {data['most_active_journal']}")
        print()
    except Exception as e:
        print(f"✗ Get statistics failed: {e}\n")

    # 6. Batch Get Papers
    print("6. Batch Get Papers")
    print("-" * 50)
    try:
        papers = client.batch_get_papers([1, 2, 3])
        data = papers['data']
        print(f"Retrieved {len(data)} papers:")
        for paper in data:
            print(f"  - ID {paper['id']}: {paper['title']}")
        print()
    except Exception as e:
        print(f"✗ Batch get failed: {e}\n")

    # 7. Export to CSV
    print("7. Export to CSV")
    print("-" * 50)
    try:
        filename = client.export_csv(
            keyword="machine learning",
            limit=10,
            filename="example_papers.csv"
        )
        print(f"✓ Exported to {filename}")
        print()
    except Exception as e:
        print(f"✗ Export CSV failed: {e}\n")

    # 8. Export to BibTeX
    print("8. Export to BibTeX")
    print("-" * 50)
    try:
        filename = client.export_bibtex(1, "example_paper.bib")
        print(f"✓ Exported to {filename}")

        # Show content
        with open(filename, 'r') as f:
            print(f"Content preview:\n{f.read()[:200]}...")
        print()
    except Exception as e:
        print(f"✗ Export BibTeX failed: {e}\n")

    print("=" * 50)
    print("Examples completed!")
    print("=" * 50)


if __name__ == "__main__":
    main()
