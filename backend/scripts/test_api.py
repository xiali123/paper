#!/usr/bin/env python3
"""
PaperCrawler API Test Script
Test database-backed API endpoints
"""

import requests
import json

BASE_URL = "http://localhost:8080"

def print_response(response, title):
    """Print formatted response"""
    print(f"\n{'='*60}")
    print(f"TEST: {title}")
    print(f"{'='*60}")
    print(f"Status Code: {response.status_code}")
    print(f"Response Time: {response.elapsed.total_seconds*1000:.0f}ms")
    try:
        print(f"Response Body:\n{json.dumps(response.json(), indent=2, ensure_ascii=False)}")
    except:
        print(f"Response Body:\n{response.text}")

def test_health_check():
    """Test health endpoint"""
    response = requests.get(f"{BASE_URL}/health")
    print_response(response, "Health Check")
    return response.status_code == 200

def test_get_papers():
    """Test getting all papers"""
    response = requests.get(f"{BASE_URL}/api/papers")
    print_response(response, "Get All Papers")
    return response.status_code == 200

def test_get_paper_by_id():
    """Test getting a specific paper"""
    response = requests.get(f"{BASE_URL}/api/papers/1")
    print_response(response, "Get Paper by ID (ID=1)")
    return response.status_code == 200

def test_search_papers():
    """Test paper search"""
    response = requests.get(f"{BASE_URL}/api/papers/search", params={"q": "attention"})
    print_response(response, "Search Papers (query='attention')")
    return response.status_code == 200

def test_get_journals():
    """Test getting journals"""
    response = requests.get(f"{BASE_URL}/api/journals")
    print_response(response, "Get Journals")
    return response.status_code == 200

def test_get_authors():
    """Test getting authors"""
    response = requests.get(f"{BASE_URL}/api/authors")
    print_response(response, "Get Authors")
    return response.status_code == 200

def test_get_collections():
    """Test getting collections"""
    response = requests.get(f"{BASE_URL}/api/collections")
    print_response(response, "Get Collections")
    return response.status_code == 200

def test_get_stats():
    """Test statistics endpoint"""
    response = requests.get(f"{BASE_URL}/api/stats")
    print_response(response, "Get Statistics")
    return response.status_code == 200

def main():
    print("\n" + "="*60)
    print("PaperCrawler API Test Suite")
    print("="*60)
    print(f"Base URL: {BASE_URL}")
    print("="*60)

    tests = [
        ("Health Check", test_health_check),
        ("Get All Papers", test_get_papers),
        ("Get Paper by ID", test_get_paper_by_id),
        ("Search Papers", test_search_papers),
        ("Get Journals", test_get_journals),
        ("Get Authors", test_get_authors),
        ("Get Collections", test_get_collections),
        ("Get Statistics", test_get_stats),
    ]

    results = []
    for name, test_func in tests:
        try:
            success = test_func()
            results.append((name, success))
        except Exception as e:
            print(f"\n[ERROR] {name}: {e}")
            results.append((name, False))

    # Summary
    print("\n" + "="*60)
    print("TEST SUMMARY")
    print("="*60)
    passed = sum(1 for _, success in results if success)
    total = len(results)

    for name, success in results:
        status = "[PASS]" if success else "[FAIL]"
        print(f"{status} {name}")

    print("-"*60)
    print(f"Results: {passed}/{total} tests passed")
    print("="*60 + "\n")

if __name__ == "__main__":
    main()
