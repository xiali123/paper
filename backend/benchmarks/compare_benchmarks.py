#!/usr/bin/env python3
"""
Performance Benchmark Comparison Tool

Compares two benchmark results and calculates performance regression/improvement.

Usage:
    python compare_benchmarks.py baseline.json current.json
    python compare_benchmarks.py baseline.json current.json --threshold 5
"""

import json
import sys
import argparse
from typing import Dict, List, Tuple

def load_benchmark_results(filename: str) -> Dict:
    """Load benchmark results from JSON file."""
    with open(filename, 'r') as f:
        data = json.load(f)
    return data

def extract_benchmark_metrics(data: Dict) -> Dict[str, float]:
    """Extract benchmark names and their mean execution times."""
    benchmarks = {}

    for benchmark in data.get('benchmarks', []):
        name = benchmark.get('name', '')
        # Extract real_time (execution time in nanoseconds)
        if 'real_time' in benchmark:
            benchmarks[name] = benchmark['real_time']

    return benchmarks

def compare_benchmarks(baseline: Dict[str, float], current: Dict[str, float],
                      threshold: float = 5.0) -> Tuple[List[Dict], List[Dict], float]:
    """
    Compare baseline and current benchmark results.

    Returns:
        Tuple of (improvements, regressions, max_regression_percent)
    """
    improvements = []
    regressions = []
    max_regression = 0.0

    for name, baseline_time in baseline.items():
        if name not in current:
            continue

        current_time = current[name]

        if baseline_time == 0 or current_time == 0:
            continue

        # Calculate percentage change (negative = improvement)
        percent_change = ((current_time - baseline_time) / baseline_time) * 100

        result = {
            'name': name,
            'baseline': baseline_time,
            'current': current_time,
            'percent_change': percent_change,
            'baseline_ms': baseline_time / 1_000_000,  # Convert to milliseconds
            'current_ms': current_time / 1_000_000
        }

        if percent_change > threshold:
            regressions.append(result)
            max_regression = max(max_regression, percent_change)
        elif percent_change < -threshold:
            improvements.append(result)

    return improvements, regressions, max_regression

def print_results(improvements: List[Dict], regressions: List[Dict],
                 max_regression: float, threshold: float):
    """Print comparison results."""

    print("=" * 80)
    print("PERFORMANCE BENCHMARK COMPARISON")
    print("=" * 80)
    print()

    if improvements:
        print(f"✅ IMPROVEMENTS (>{threshold}% faster):")
        print("-" * 80)
        for imp in sorted(improvements, key=lambda x: x['percent_change']):
            print(f"  {imp['name']}")
            print(f"    Baseline: {imp['baseline_ms']:.3f} ms")
            print(f"    Current:  {imp['current_ms']:.3f} ms")
            print(f"    Change:   {imp['percent_change']:+.2f}%")
            print()
    else:
        print("✅ No significant improvements detected")
        print()

    if regressions:
        print(f"❌ REGRESSIONS (>{threshold}% slower):")
        print("-" * 80)
        for reg in sorted(regressions, key=lambda x: -x['percent_change']):
            print(f"  {reg['name']}")
            print(f"    Baseline: {reg['baseline_ms']:.3f} ms")
            print(f"    Current:  {reg['current_ms']:.3f} ms")
            print(f"    Change:   {reg['percent_change']:+.2f}%")
            print()
    else:
        print("❌ No significant regressions detected")
        print()

    print("=" * 80)
    print(f"MAXIMUM REGRESSION: {max_regression:.2f}%")
    print("=" * 80)
    print()

    if max_regression > threshold:
        print(f"⚠️  PERFORMANCE REGRESSION DETECTED (> {threshold}%)")
        print("   Please review and optimize before merging.")
        return 1
    else:
        print("✅ PERFORMANCE ACCEPTABLE (within threshold)")
        return 0

def generate_report(baseline_file: str, current_file: str,
                   improvements: List[Dict], regressions: List[Dict],
                   max_regression: float) -> str:
    """Generate detailed performance report."""

    report = []
    report.append("# Performance Benchmark Report")
    report.append("")
    report.append(f"**Baseline**: {baseline_file}")
    report.append(f"**Current**: {current_file}")
    report.append(f"**Maximum Regression**: {max_regression:.2f}%")
    report.append("")
    report.append("## Summary")
    report.append("")

    if improvements:
        report.append(f"### ✅ Improvements ({len(improvements)} benchmarks)")
        report.append("")
        for imp in sorted(improvements, key=lambda x: x['percent_change']):
            report.append(f"- **{imp['name']}**")
            report.append(f"  - Baseline: {imp['baseline_ms']:.3f} ms")
            report.append(f"  - Current: {imp['current_ms']:.3f} ms")
            report.append(f"  - Improvement: {imp['percent_change']:+.2f}%")
            report.append("")

    if regressions:
        report.append(f"### ❌ Regressions ({len(regressions)} benchmarks)")
        report.append("")
        for reg in sorted(regressions, key=lambda x: -x['percent_change']):
            report.append(f"- **{reg['name']}**")
            report.append(f"  - Baseline: {reg['baseline_ms']:.3f} ms")
            report.append(f"  - Current: {reg['current_ms']:.3f} ms")
            report.append(f"  - Regression: {reg['percent_change']:+.2f}%")
            report.append("")

    report.append("## Recommendations")
    report.append("")

    if regressions:
        report.append("### Priority Actions")
        report.append("")
        report.append("1. Review regressions > 5%")
        report.append("2. Profile affected code paths")
        report.append("3. Implement optimizations")
        report.append("4. Re-run benchmarks to validate fixes")
        report.append("")

    if not regressions and improvements:
        report.append("### Performance Gains")
        report.append("")
        report.append("Excellent! The following optimizations were successful:")
        for imp in improvements[:5]:
            report.append(f"- {imp['name']}: {imp['percent_change']:+.2f}%")
        report.append("")

    return "\n".join(report)

def main():
    parser = argparse.ArgumentParser(
        description='Compare performance benchmark results'
    )
    parser.add_argument(
        'baseline',
        help='Baseline benchmark JSON file'
    )
    parser.add_argument(
        'current',
        help='Current benchmark JSON file'
    )
    parser.add_argument(
        '--threshold',
        type=float,
        default=5.0,
        help='Regression threshold in percentage (default: 5.0)'
    )
    parser.add_argument(
        '--report',
        type=str,
        help='Generate markdown report file'
    )

    args = parser.parse_args()

    try:
        # Load benchmark results
        baseline_data = load_benchmark_results(args.baseline)
        current_data = load_benchmark_results(args.current)

        # Extract metrics
        baseline_metrics = extract_benchmark_metrics(baseline_data)
        current_metrics = extract_benchmark_metrics(current_data)

        # Compare benchmarks
        improvements, regressions, max_regression = compare_benchmarks(
            baseline_metrics, current_metrics, args.threshold
        )

        # Print results
        exit_code = print_results(improvements, regressions, max_regression, args.threshold)

        # Generate report if requested
        if args.report:
            report = generate_report(
                args.baseline, args.current,
                improvements, regressions, max_regression
            )
            with open(args.report, 'w') as f:
                f.write(report)
            print(f"Report generated: {args.report}")

        sys.exit(exit_code)

    except FileNotFoundError as e:
        print(f"Error: File not found - {e}")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON format - {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
