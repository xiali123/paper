#!/usr/bin/env python3
"""
PaperCrawler Load Testing Script

Uses Locust to simulate concurrent users and measure performance.

Install: pip install locust
Run: locust -f load_test.py --host=http://localhost:8080
"""

import json
import time
import random
from locust import HttpUser, task, between, events
from locust.stats import stats_printer

# Test data
QUERIES = [
    "machine learning",
    "deep learning",
    "neural networks",
    "computer vision",
    "natural language processing",
    "reinforcement learning",
    "transformer models",
    "graph neural networks"
]

TEMPLATE_IDS = ["arxiv", "pubmed", "ieee", "springer"]

class PaperCrawlerUser(HttpUser):
    """
    Simulates a realistic user of the PaperCrawler system.
    """

    # Wait time between tasks (in seconds)
    wait_time = between(1, 3)

    def on_start(self):
        """Called when a user starts."""
        # Login (if authentication is enabled)
        self.client.post("/api/auth/login", json={
            "email": "test@example.com",
            "password": "password123"
        })

    @task(5)
    def get_papers(self):
        """Get list of papers (high frequency)."""
        self.client.get("/api/papers")

    @task(3)
    def get_paper_by_id(self):
        """Get specific paper by ID."""
        paper_id = random.randint(1, 100)
        self.client.get(f"/api/papers/{paper_id}")

    @task(2)
    def search_papers(self):
        """Search for papers."""
        query = random.choice(QUERIES)
        self.client.get(f"/api/papers/search?q={query}")

    @task(1)
    def crawl_paper(self):
        """Crawl papers (low frequency, high latency)."""
        template_id = random.choice(TEMPLATE_IDS)
        query = random.choice(QUERIES)

        with self.client.post("/api/crawl", json={
            "template_id": template_id,
            "params": {"query": query}
        }, catch_response=True) as response:
            if response.status_code == 200:
                try:
                    data = response.json()
                    if data.get("success"):
                        papers = data.get("papers", [])
                        if len(papers) == 0:
                            response.mark_failure("No papers found")
                    else:
                        response.mark_failure("API returned success=false")
                except json.JSONDecodeError:
                    response.mark_failure("Invalid JSON response")

    @task(1)
    def create_paper(self):
        """Create a new paper."""
        self.client.post("/api/papers", json={
            "title": f"Test Paper {int(time.time())}",
            "authors": "Test Author",
            "year": 2024,
            "abstract": "This is a test abstract."
        })

class AdminUser(HttpUser):
    """
    Simulates admin users performing administrative tasks.
    """

    wait_time = between(2, 5)

    def on_start(self):
        """Login as admin."""
        self.client.post("/api/auth/login", json={
            "email": "admin@example.com",
            "password": "admin123"
        })

    @task(10)
    def get_stats(self):
        """Get system statistics."""
        self.client.get("/api/stats")

    @task(5)
    def get_users(self):
        """Get list of users."""
        self.client.get("/api/users")

    @task(3)
    def get_templates(self):
        """Get crawler templates."""
        self.client.get("/api/templates")

    @task(1)
    def create_template(self):
        """Create a new crawler template."""
        self.client.post("/api/templates", json={
            "template_id": f"test_template_{int(time.time())}",
            "name": "Test Template",
            "base_url": "https://example.com",
            "url_template": "/search?q={query}",
            "source_type": "API"
        })


# Custom event handlers for additional metrics
@events.request.add_listener
def on_request(request_type, name, response_time, response_length, **kwargs):
    """
    Custom request handler for additional metrics.
    """
    # Log slow requests
    if response_time > 5000:  # > 5 seconds
        print(f"⚠️  SLOW REQUEST: {name} took {response_time}ms")

    # Log failed requests
    if hasattr(kwargs['exception'], 'status_code'):
        status_code = kwargs['exception'].status_code
        if status_code >= 500:
            print(f"❌ SERVER ERROR: {name} returned {status_code}")
        elif status_code >= 400:
            print(f"⚠️  CLIENT ERROR: {name} returned {status_code}")


@events.test_stop.add_listener
def on_test_stop(environment, **kwargs):
    """
    Called when the test stops.
    """
    print("\n" + "=" * 80)
    print("LOAD TEST COMPLETED")
    print("=" * 80)
    print("\nPerformance Summary:")
    print("-" * 80)

    stats = environment.stats

    # Print overall statistics
    print(f"Total Requests: {stats.total.num_requests}")
    print(f"Success Rate: {stats.total.success_ratio * 100:.2f}%")
    print(f"Failure Rate: {(1 - stats.total.success_ratio) * 100:.2f}%")
    print(f"Response Times:")
    print(f"  - Median: {stats.total.median_response_time:.0f}ms")
    print(f"  - Average: {stats.total.avg_response_time:.0f}ms")
    print(f"  - Min: {stats.total.min_response_time:.0f}ms")
    print(f"  - Max: {stats.total.max_response_time:.0f}ms")
    print(f"  - P95: {stats.total.get_response_time_percentile(0.95):.0f}ms")
    print(f"  - P99: {stats.total.get_response_time_percentile(0.99):.0f}ms")
    print(f"Requests per Second: {stats.total.total_rps:.2f}")
    print()

    # Performance evaluation
    print("Performance Evaluation:")
    print("-" * 80)

    if stats.total.median_response_time < 200:
        print("✅ Median Response Time: EXCELLENT (< 200ms)")
    elif stats.total.median_response_time < 500:
        print("✅ Median Response Time: GOOD (< 500ms)")
    elif stats.total.median_response_time < 1000:
        print("⚠️  Median Response Time: ACCEPTABLE (< 1000ms)")
    else:
        print("❌ Median Response Time: POOR (>= 1000ms)")

    if stats.total.get_response_time_percentile(0.95) < 500:
        print("✅ P95 Response Time: EXCELLENT (< 500ms)")
    elif stats.total.get_response_time_percentile(0.95) < 1000:
        print("✅ P95 Response Time: GOOD (< 1000ms)")
    elif stats.total.get_response_time_percentile(0.95) < 2000:
        print("⚠️  P95 Response Time: ACCEPTABLE (< 2000ms)")
    else:
        print("❌ P95 Response Time: POOR (>= 2000ms)")

    if stats.total.success_ratio > 0.99:
        print("✅ Success Rate: EXCELLENT (> 99%)")
    elif stats.total.success_ratio > 0.95:
        print("✅ Success Rate: GOOD (> 95%)")
    elif stats.total.success_ratio > 0.90:
        print("⚠️  Success Rate: ACCEPTABLE (> 90%)")
    else:
        print("❌ Success Rate: POOR (<= 90%)")

    print()
    print("=" * 80)


# Performance targets
PERFORMANCE_TARGETS = {
    "GET /api/papers": {
        "median_ms": 200,
        "p95_ms": 500,
        "rps": 100
    },
    "GET /api/papers/{id}": {
        "median_ms": 100,
        "p95_ms": 300,
        "rps": 200
    },
    "POST /api/crawl": {
        "median_ms": 3000,
        "p95_ms": 5000,
        "rps": 10
    }
}


def check_performance_targets(environment):
    """
    Check if performance meets targets.
    """
    print("\nPerformance Targets:")
    print("-" * 80)

    stats = environment.stats

    for endpoint, targets in PERFORMANCE_TARGETS.items():
        # Find matching requests
        for entry in stats.entries:
            if endpoint in entry.name:
                median_ms = entry.median_response_time
                p95_ms = entry.get_response_time_percentile(0.95)

                median_target = targets["median_ms"]
                p95_target = targets["p95_ms"]

                median_status = "✅" if median_ms <= median_target else "❌"
                p95_status = "✅" if p95_ms <= p95_target else "❌"

                print(f"{endpoint}:")
                print(f"  Median: {median_ms:.0f}ms (target: {median_target}ms) {median_status}")
                print(f"  P95: {p95_ms:.0f}ms (target: {p95_target}ms) {p95_status}")
                print()


if __name__ == "__main__":
    # Run with default settings
    import subprocess
    print("Starting Locust load tester...")
    print("Open http://localhost:8089 in your browser to control the test")
    print()
    subprocess.run(["locust", "-f", __file__, "--host", "http://localhost:8080"])
