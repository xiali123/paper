#!/usr/bin/env python3
"""
PaperCrawler Performance Monitoring Script
Continuously monitors system performance and generates alerts
"""

import psutil
import requests
import json
import time
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Optional
import smtplib
from email.mime.text import MIMEText

# Configuration
BACKEND_URL = "http://localhost:8080"
MONITOR_INTERVAL = 30  # seconds
ALERT_THRESHOLDS = {
    "error_rate": 5.0,  # percentage
    "p95_latency": 500,  # milliseconds
    "p99_latency": 1000,  # milliseconds
    "cpu_utilization": 95.0,  # percentage
    "memory_utilization": 85.0,  # percentage
    "disk_utilization": 90.0,  # percentage
    "database_pool_exhaustion": 90.0,  # percentage
    "cache_hit_rate": 50.0,  # percentage (below this triggers alert)
}

# Logging setup
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('performance_monitor.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)


class PerformanceMonitor:
    """Comprehensive performance monitoring system"""

    def __init__(self, backend_url: str = BACKEND_URL):
        self.backend_url = backend_url
        self.metrics_history = []
        self.alert_count = 0
        self.last_alert_time = {}  # Track last alert time for each metric

    def collect_system_metrics(self) -> Dict:
        """Collect system-level performance metrics"""
        try:
            metrics = {
                "timestamp": datetime.now().isoformat(),
                "cpu": {
                    "utilization_percent": psutil.cpu_percent(interval=1),
                    "core_count": psutil.cpu_count(),
                    "frequency_mhz": psutil.cpu_freq().current if psutil.cpu_freq() else 0,
                },
                "memory": {
                    "total_gb": psutil.virtual_memory().total / (1024**3),
                    "available_gb": psutil.virtual_memory().available / (1024**3),
                    "used_gb": psutil.virtual_memory().used / (1024**3),
                    "utilization_percent": psutil.virtual_memory().percent,
                },
                "disk": {
                    "total_gb": psutil.disk_usage('/').total / (1024**3),
                    "used_gb": psutil.disk_usage('/').used / (1024**3),
                    "utilization_percent": psutil.disk_usage('/').percent,
                },
                "network": {
                    "bytes_sent": psutil.net_io_counters().bytes_sent,
                    "bytes_recv": psutil.net_io_counters().bytes_recv,
                    "packets_sent": psutil.net_io_counters().packets_sent,
                    "packets_recv": psutil.net_io_counters().packets_recv,
                }
            }
            return metrics
        except Exception as e:
            logger.error(f"Error collecting system metrics: {e}")
            return {}

    def collect_backend_metrics(self) -> Dict:
        """Collect backend application metrics"""
        try:
            metrics = {}

            # Health check
            try:
                health_response = requests.get(f"{self.backend_url}/health", timeout=5)
                metrics["health"] = {
                    "status": "healthy" if health_response.status_code == 200 else "unhealthy",
                    "response_time_ms": health_response.elapsed.total_seconds() * 1000,
                }
            except requests.exceptions.RequestException as e:
                metrics["health"] = {
                    "status": "unreachable",
                    "error": str(e),
                }
                logger.error(f"Health check failed: {e}")
                return metrics

            # Database pool stats
            try:
                db_stats = requests.get(f"{self.backend_url}/api/database/stats", timeout=5)
                if db_stats.status_code == 200:
                    metrics["database"] = db_stats.json()
            except requests.exceptions.RequestException as e:
                logger.warning(f"Failed to fetch database stats: {e}")

            # Cache stats
            try:
                cache_stats = requests.get(f"{self.backend_url}/api/cache/stats", timeout=5)
                if cache_stats.status_code == 200:
                    metrics["cache"] = cache_stats.json()
            except requests.exceptions.RequestException as e:
                logger.warning(f"Failed to fetch cache stats: {e}")

            # Thread pool stats
            try:
                pool_stats = requests.get(f"{self.backend_url}/api/pools/stats", timeout=5)
                if pool_stats.status_code == 200:
                    metrics["pools"] = pool_stats.json()
            except requests.exceptions.RequestException as e:
                logger.warning(f"Failed to fetch pool stats: {e}")

            return metrics

        except Exception as e:
            logger.error(f"Error collecting backend metrics: {e}")
            return {}

    def calculate_performance_metrics(self) -> Dict:
        """Calculate performance metrics from samples"""
        if len(self.metrics_history) < 2:
            return {}

        latest = self.metrics_history[-1]
        previous = self.metrics_history[-2]

        metrics = {
            "timestamp": latest["timestamp"],
            "throughput_requests_per_second": 0,
            "average_latency_ms": 0,
            "error_rate_percent": 0,
        }

        # Calculate network throughput
        if "network" in latest and "network" in previous:
            bytes_sent_delta = latest["network"]["bytes_sent"] - previous["network"]["bytes_sent"]
            bytes_recv_delta = latest["network"]["bytes_recv"] - previous["network"]["bytes_recv"]
            metrics["network_throughput_mbps"] = (bytes_sent_delta + bytes_recv_delta) / (1024**2)

        return metrics

    def check_thresholds(self, metrics: Dict) -> List[Dict]:
        """Check if metrics exceed alert thresholds"""
        alerts = []

        # Check CPU utilization
        if "cpu" in metrics:
            cpu_util = metrics["cpu"]["utilization_percent"]
            if cpu_util > ALERT_THRESHOLDS["cpu_utilization"]:
                alerts.append({
                    "metric": "cpu_utilization",
                    "current_value": cpu_util,
                    "threshold": ALERT_THRESHOLDS["cpu_utilization"],
                    "severity": "WARNING" if cpu_util < 98 else "CRITICAL",
                    "message": f"High CPU utilization: {cpu_util:.1f}%",
                })

        # Check memory utilization
        if "memory" in metrics:
            mem_util = metrics["memory"]["utilization_percent"]
            if mem_util > ALERT_THRESHOLDS["memory_utilization"]:
                alerts.append({
                    "metric": "memory_utilization",
                    "current_value": mem_util,
                    "threshold": ALERT_THRESHOLDS["memory_utilization"],
                    "severity": "WARNING" if mem_util < 95 else "CRITICAL",
                    "message": f"High memory utilization: {mem_util:.1f}%",
                })

        # Check database pool
        if "database" in metrics and "activeConnections" in metrics["database"]:
            db = metrics["database"]
            if "totalConnections" in db and db["totalConnections"] > 0:
                pool_util = (db["activeConnections"] / db["totalConnections"]) * 100
                if pool_util > ALERT_THRESHOLDS["database_pool_exhaustion"]:
                    alerts.append({
                        "metric": "database_pool_exhaustion",
                        "current_value": pool_util,
                        "threshold": ALERT_THRESHOLDS["database_pool_exhaustion"],
                        "severity": "WARNING",
                        "message": f"Database connection pool exhaustion: {pool_util:.1f}%",
                    })

        # Check cache hit rate
        if "cache" in metrics and "hitCount" in metrics["cache"]:
            cache = metrics["cache"]
            total_ops = cache.get("hitCount", 0) + cache.get("missCount", 0)
            if total_ops > 0:
                hit_rate = (cache["hitCount"] / total_ops) * 100
                if hit_rate < ALERT_THRESHOLDS["cache_hit_rate"]:
                    alerts.append({
                        "metric": "cache_hit_rate",
                        "current_value": hit_rate,
                        "threshold": ALERT_THRESHOLDS["cache_hit_rate"],
                        "severity": "WARNING",
                        "message": f"Low cache hit rate: {hit_rate:.1f}%",
                    })

        return alerts

    def send_alert(self, alert: Dict) -> None:
        """Send alert notification"""
        # Check if we've recently sent an alert for this metric (prevent spam)
        metric = alert["metric"]
        now = datetime.now()
        if metric in self.last_alert_time:
            time_since_last = now - self.last_alert_time[metric]
            if time_since_last < timedelta(minutes=15):  # Don't alert more than once per 15 minutes
                return

        self.last_alert_time[metric] = now
        self.alert_count += 1

        # Log alert
        severity = alert["severity"]
        logger.warning(f"ALERT [{severity}]: {alert['message']}")

        # In production, you would send email, Slack, or PagerDuty notifications here
        # Example: send_email_alert(alert)
        # Example: send_slack_alert(alert)

    def generate_report(self) -> Dict:
        """Generate comprehensive performance report"""
        if not self.metrics_history:
            return {}

        latest_metrics = self.metrics_history[-1]

        # Calculate averages over the monitoring period
        cpu_values = [m["cpu"]["utilization_percent"] for m in self.metrics_history if "cpu" in m]
        mem_values = [m["memory"]["utilization_percent"] for m in self.metrics_history if "memory" in m]

        report = {
            "monitoring_period": {
                "start": self.metrics_history[0]["timestamp"],
                "end": latest_metrics["timestamp"],
                "duration_minutes": len(self.metrics_history) * MONITOR_INTERVAL / 60,
                "samples_collected": len(self.metrics_history),
            },
            "current_status": {
                "cpu_utilization_percent": latest_metrics.get("cpu", {}).get("utilization_percent", 0),
                "memory_utilization_percent": latest_metrics.get("memory", {}).get("utilization_percent", 0),
                "health_status": latest_metrics.get("health", {}).get("status", "unknown"),
            },
            "averages": {
                "cpu_utilization_percent": sum(cpu_values) / len(cpu_values) if cpu_values else 0,
                "memory_utilization_percent": sum(mem_values) / len(mem_values) if mem_values else 0,
            },
            "alerts_triggered": self.alert_count,
        }

        return report

    def save_metrics(self, filename: str = "metrics_history.json") -> None:
        """Save metrics history to JSON file"""
        try:
            with open(filename, 'w') as f:
                json.dump(self.metrics_history, f, indent=2)
            logger.info(f"Metrics history saved to {filename}")
        except Exception as e:
            logger.error(f"Error saving metrics: {e}")

    def monitor_once(self) -> None:
        """Perform one monitoring cycle"""
        logger.info("Collecting metrics...")

        # Collect system metrics
        system_metrics = self.collect_system_metrics()

        # Collect backend metrics
        backend_metrics = self.collect_backend_metrics()

        # Merge metrics
        combined_metrics = {**system_metrics, **backend_metrics}
        self.metrics_history.append(combined_metrics)

        # Check thresholds and send alerts
        alerts = self.check_thresholds(combined_metrics)
        for alert in alerts:
            self.send_alert(alert)

        # Log current status
        if "cpu" in combined_metrics and "memory" in combined_metrics:
            logger.info(
                f"CPU: {combined_metrics['cpu']['utilization_percent']:.1f}%, "
                f"Memory: {combined_metrics['memory']['utilization_percent']:.1f}%, "
                f"Health: {combined_metrics.get('health', {}).get('status', 'unknown')}"
            )

    def monitor_continuous(self, duration_minutes: int = 60) -> None:
        """Continuously monitor for specified duration"""
        logger.info(f"Starting continuous monitoring for {duration_minutes} minutes...")

        end_time = datetime.now() + timedelta(minutes=duration_minutes)
        sample_count = 0

        try:
            while datetime.now() < end_time:
                self.monitor_once()
                sample_count += 1
                time.sleep(MONITOR_INTERVAL)

        except KeyboardInterrupt:
            logger.info("Monitoring interrupted by user")

        # Generate final report
        report = self.generate_report()
        logger.info(f"\nMonitoring Complete. Final Report:")
        logger.info(json.dumps(report, indent=2))

        # Save metrics
        self.save_metrics()

        logger.info(f"Total samples collected: {sample_count}")
        logger.info(f"Total alerts triggered: {self.alert_count}")


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(description="PaperCrawler Performance Monitor")
    parser.add_argument("--backend-url", default=BACKEND_URL, help="Backend URL to monitor")
    parser.add_argument("--duration", type=int, default=60, help="Monitoring duration in minutes")
    parser.add_argument("--interval", type=int, default=30, help="Monitoring interval in seconds")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose logging")

    args = parser.parse_args()

    if args.verbose:
        logger.setLevel(logging.DEBUG)

    # Create monitor
    monitor = PerformanceMonitor(backend_url=args.backend_url)

    # Start monitoring
    monitor.monitor_continuous(duration_minutes=args.duration)


if __name__ == "__main__":
    main()