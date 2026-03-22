#!/usr/bin/env python3
"""
Simple test server for PaperCrawler API
Returns mock data with correct camelCase format and proper response wrapper
"""

from http.server import HTTPServer, SimpleHTTPRequestHandler
import json
import urllib.parse
import time

class PaperCrawlerHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        # Parse path
        parsed_path = urllib.parse.urlparse(self.path)
        path = parsed_path.path

        # Enable CORS
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

        # Create success response wrapper
        def create_response(data):
            return {
                "success": True,
                "data": data,
                "timestamp": int(time.time())
            }

        # Health check
        if path == '/health' or path == '/api/health':
            response = create_response({
                "status": "ok",
                "message": "Server is running"
            })
            self.wfile.write(json.dumps(response).encode())
            return

        # Statistics overview
        if path == '/api/stats/overview':
            response = create_response({
                "totalPapers": 1250,
                "totalJournals": 85,
                "topTierPapers": 320,
                "papersLastYear": 180,
                "mostActiveJournal": "IEEE Transactions on Pattern Analysis and Machine Intelligence"
            })
            self.wfile.write(json.dumps(response).encode())
            return

        # Search endpoint
        if path.startswith('/api/search'):
            query = urllib.parse.parse_qs(parsed_path.query)
            keyword = query.get('q', [''])[0]
            limit = int(query.get('limit', [10])[0])
            offset = int(query.get('offset', [0])[0])

            # Generate diverse and realistic mock data
            papers = []
            levels = ['A', 'B', 'C']
            journals = [
                {"full": "IEEE Transactions on Pattern Analysis and Machine Intelligence", "short": "IEEE TPAMI"},
                {"full": "Journal of Machine Learning Research", "short": "JMLR"},
                {"full": "Neural Information Processing Systems", "short": "NeurIPS"},
                {"full": "Computer Vision and Pattern Recognition", "short": "CVPR"},
                {"full": "International Conference on Machine Learning", "short": "ICML"},
                {"full": "AAAI Conference on Artificial Intelligence", "short": "AAAI"},
                {"full": "ACM SIGKDD Conference on Knowledge Discovery", "short": "KDD"},
                {"full": "IEEE Transactions on Knowledge and Data Engineering", "short": "IEEE TKDE"},
                {"full": "ACM Conference on Computer Vision", "short": "ACM CV"},
                {"full": "European Conference on Computer Vision", "short": "ECCV"}
            ]

            # Diverse title patterns for variety
            title_patterns = [
                f"Deep {keyword} Methods for Real-World Applications",
                f"{keyword}-based Systems: A Comprehensive Survey",
                f"Advances in {keyword}: Theory and Practice",
                f"Scalable {keyword} Architectures for Big Data",
                f"{keyword} with Limited Supervision",
                f"Robust {keyword} in Adverse Conditions",
                f"Multi-modal {keyword}: Integrating Multiple Data Sources",
                f"Explainable {keyword} for Transparent Decision Making",
                f"{keyword} for Edge Computing: Efficiency and Accuracy",
                f"Transfer Learning in {keyword}: Cross-Domain Adaptation",
                f"Adversarial Attacks on {keyword} Systems and Defenses",
                f"{keyword} with Graph Neural Networks",
                f"Temporal {keyword}: Time-Series Analysis and Forecasting",
                f"{keyword} Optimization: Speed vs Accuracy Trade-offs",
                f"Federated {keyword}: Privacy-Preserving Distributed Learning",
                f"{keyword} in Healthcare: Medical Applications and Challenges",
                f"Attention Mechanisms for {keyword} Tasks",
                f"{keyword} with Self-Supervised Learning",
                f"Real-time {keyword} Systems for Production Environments",
                f"{keyword} Benchmark: Dataset and Evaluation Metrics"
            ]

            num_papers = min(limit, 20)
            for i in range(num_papers):
                level = levels[i % 3]
                journal = journals[i % len(journals)]
                title_idx = (offset + i) % len(title_patterns)
                papers.append({
                    "id": offset + i + 1,
                    "title": title_patterns[title_idx],
                    "journal": journal,
                    "year": 2020 + ((offset + i) % 5),
                    "level": level
                })

            response = create_response({
                "papers": papers,
                "total": 100,
                "keyword": keyword
            })
            self.wfile.write(json.dumps(response).encode())
            return

        # Default response
        error_response = {
            "success": False,
            "error": "Not found",
            "timestamp": int(time.time())
        }
        self.wfile.write(json.dumps(error_response).encode())

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

def run_server(port=8080):
    server_address = ('', port)
    httpd = HTTPServer(server_address, PaperCrawlerHandler)
    print(f"Server running on port {port}")
    print(f"Test endpoints:")
    print(f"   http://localhost:{port}/health")
    print(f"   http://localhost:{port}/api/stats/overview")
    print(f"   http://localhost:{port}/api/search?q=test")
    print(f"Response format: Wrapped with success/data/timestamp")
    httpd.serve_forever()

if __name__ == '__main__':
    run_server()
