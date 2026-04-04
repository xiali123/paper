/**
 * Simple CORS proxy server for development
 * Proxies requests from frontend to backend without modifying JSON payloads
 */

const http = require('http')
const url = require('url')

const BACKEND_PORT = 8080
const PROXY_PORT = 3008
const BACKEND_HOST = 'localhost'

const proxy = http.createServer((req, res) => {
  const options = {
    hostname: BACKEND_HOST,
    port: BACKEND_PORT,
    path: req.url,
    method: req.method,
    headers: {
      ...req.headers,
      host: `${BACKEND_HOST}:${BACKEND_PORT}`
    }
  }

  console.log(`[PROXY] ${req.method} ${req.url} -> ${BACKEND_HOST}:${BACKEND_PORT}${req.url}`)

  // Log request body for debugging
  let requestBody = []
  req.on('data', (chunk) => {
    requestBody.push(chunk)
  })

  req.on('end', () => {
    const bodyBuffer = Buffer.concat(requestBody)
    if (bodyBuffer.length > 0) {
      console.log(`[PROXY] Request body (${bodyBuffer.length} bytes):`, bodyBuffer.toString())
      options.headers['Content-Length'] = bodyBuffer.length
    }

    // Forward request to backend
    const proxyReq = http.request(options, (proxyRes) => {
      // Add CORS headers to response
      res.setHeader('Access-Control-Allow-Origin', '*')
      res.setHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
      res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization')

      // Handle OPTIONS preflight
      if (req.method === 'OPTIONS') {
        res.writeHead(200)
        res.end()
        return
      }

      // Copy status code and headers from backend response
      res.writeHead(proxyRes.statusCode, proxyRes.headers)

      // Pipe response body without modification
      proxyRes.pipe(res)

      console.log(`[PROXY] Response: ${proxyRes.statusCode}`)
    })

    proxyReq.on('error', (err) => {
      console.error(`[PROXY] Error: ${err.message}`)
      if (res.headersSent) {
        res.end()
      } else {
        res.writeHead(500, { 'Content-Type': 'application/json' })
        res.end(JSON.stringify({ error: 'Proxy error', message: err.message }))
      }
    })

    // Write request body if exists
    if (bodyBuffer.length > 0) {
      proxyReq.write(bodyBuffer)
    }

    proxyReq.end()
  })
})

proxy.listen(PROXY_PORT, () => {
  console.log(`[PROXY] Server running on port ${PROXY_PORT}`)
  console.log(`[PROXY] Forwarding to ${BACKEND_HOST}:${BACKEND_PORT}`)
  console.log(`[PROXY] Frontend should use: http://localhost:${PROXY_PORT}`)
})
