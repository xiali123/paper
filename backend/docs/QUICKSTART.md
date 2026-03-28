# Quick Start Guide - PaperCrawler REST API

Get the PaperCrawler REST API server up and running in minutes.

## Prerequisites Check

Before starting, ensure you have:

- [ ] C++17 compatible compiler (GCC 7+, Clang 5+, or MSVC 2017+)
- [ ] CMake 3.15 or higher
- [ ] MySQL server installed and running
- [ ] Git (for cloning the repository)

## Installation Steps

### 1. Clone the Repository

```bash
git clone <repository-url>
cd PaperCrawler/backend
```

### 2. Configure Database

```bash
# Copy example configuration
cp config.example.json config/config.json

# Edit with your database credentials
nano config/config.json
```

Update the database section:
```json
{
  "database": {
    "host": "localhost",
    "port": 3306,
    "name": "papercrawler",
    "user": "your_username",
    "password": "your_password"
  }
}
```

### 3. Build the Server

**Linux/Mac:**
```bash
chmod +x start_server.sh
./start_server.sh
```

**Windows:**
```cmd
start_server.bat
```

**Or manually:**
```bash
mkdir build && cd build
cmake ..
cmake --build .
./PaperCrawlerServer
```

### 4. Verify the Server

Open a new terminal and test:

```bash
curl http://localhost:8080/health
```

Expected response:
```json
{
  "status": "healthy",
  "service": "PaperCrawler API",
  "version": "1.0.0",
  "database": "connected"
}
```

## First API Request

Let's search for papers:

```bash
curl "http://localhost:8080/api/search?q=deep+learning&limit=5"
```

## Testing the API

### Option 1: Automated Test Script

```bash
chmod +x test_api.sh
./test_api.sh
```

### Option 2: Postman

1. Import `postman_collection.json` into Postman
2. Click "Send" on any request

### Option 3: cURL Examples

```bash
# Health check
curl http://localhost:8080/health

# Search papers
curl "http://localhost:8080/api/search?q=machine+learning"

# Get paper details
curl http://localhost:8080/api/papers/1

# Export to CSV
curl -o papers.csv http://localhost:8080/api/export/csv

# Get statistics
curl http://localhost:8080/api/stats/overview
```

## Common Issues

### Issue: Port 8080 Already in Use

**Solution:** Change the port in `src/api_server.cpp` (line ~429):

```cpp
serverAddr.sin_port = htons(8081); // Use 8081 instead
```

### Issue: Database Connection Failed

**Solution:**
1. Verify MySQL is running:
   ```bash
   # Linux
   sudo systemctl status mysql

   # Mac
   brew services list

   # Windows
   # Check Services app
   ```

2. Test connection manually:
   ```bash
   mysql -h localhost -u your_username -p
   ```

3. Verify database exists:
   ```sql
   SHOW DATABASES;
   ```

### Issue: Build Errors

**Solution:** Install missing dependencies:

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libssl-dev
```

**Mac:**
```bash
brew install cmake openssl
```

**Windows:**
- Install Visual Studio 2017 or later
- Install CMake from cmake.org
- Install vcpkg for dependencies

## Next Steps

1. **Read the full API documentation:** `API_DOCUMENTATION.md`
2. **Explore examples:** Check the `examples/` directory
3. **Integrate with your application:** Use the provided client libraries
4. **Deploy to production:** See the deployment section in README.md

## Quick Reference

| Task | Command |
|------|---------|
| Start server | `./start_server.sh` or `./build/PaperCrawlerServer` |
| Stop server | Ctrl+C |
| Build | `make build` or `cmake --build build` |
| Clean | `make clean` |
| Test | `make test` or `./test_api.sh` |
| View logs | Check console output or `logs/api.log` |

## API Endpoints Cheat Sheet

```
GET  /health                          Health check
GET  /api/search                      Search papers
GET  /api/papers/{id}                 Get paper details
GET  /api/papers/recent               Get recent papers
POST /api/papers/batch                Batch get papers
GET  /api/stats/overview              Get statistics
GET  /api/export/csv                  Export CSV
GET  /api/export/json                 Export JSON
GET  /api/export/bibtex/{id}          Export BibTeX
```

## Support

- Documentation: `API_DOCUMENTATION.md`
- Examples: `examples/` directory
- Issues: GitHub Issues
- Community: Discord/Slack channel

## What's Next?

- [ ] Set up production database
- [ ] Configure reverse proxy (nginx/Apache)
- [ ] Enable SSL/TLS
- [ ] Set up monitoring and logging
- [ ] Deploy to production environment

---

**Need help?** See the full README.md or open an issue on GitHub.
