/**
 * PaperCrawler API 连接测试脚本
 *
 * 使用方法：
 * node check-api-connection.js
 */

const http = require('http');

const API_URL = 'http://localhost:8082';
const TEST_ENDPOINTS = [
  '/api/health',
  '/api/papers',
  '/api/papers/stats'
];

console.log('🔍 检查 PaperCrawler Mock API 状态...\n');

function checkEndpoint(endpoint) {
  return new Promise((resolve) => {
    const url = new URL(endpoint, API_URL);

    http.get(url, (res) => {
      let data = '';

      res.on('data', (chunk) => {
        data += chunk;
      });

      res.on('end', () => {
        resolve({
          endpoint,
          status: res.statusCode,
          success: res.statusCode === 200,
          data: data.length > 100 ? data.substring(0, 100) + '...' : data
        });
      });
    }).on('error', (err) => {
      resolve({
        endpoint,
        status: 'ERROR',
        success: false,
        error: err.message
      });
    });
  });
}

async function runTests() {
  console.log(`📍 目标服务器: ${API_URL}\n`);

  for (const endpoint of TEST_ENDPOINTS) {
    const result = await checkEndpoint(endpoint);

    if (result.success) {
      console.log(`✅ ${endpoint}`);
      console.log(`   状态: ${result.status}`);
      console.log(`   数据: ${result.data}\n`);
    } else {
      console.log(`❌ ${endpoint}`);
      console.log(`   错误: ${result.error || 'Status ' + result.status}\n`);
    }
  }

  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n');

  console.log('📋 诊断结果:\n');

  const allSuccess = TEST_ENDPOINTS.every(async (endpoint) => {
    const result = await checkEndpoint(endpoint);
    return result.success;
  });

  if (await Promise.all(TEST_ENDPOINTS.map(checkEndpoint)).then(results => results.every(r => r.success))) {
    console.log('✅ Mock API 运行正常！');
    console.log('✅ 前端可以正常连接');
    console.log('\n🚀 可以开始测试论文管理功能了！');
    console.log('   访问: http://localhost:5173/papers\n');
  } else {
    console.log('❌ Mock API 未运行或连接失败\n');
    console.log('🔧 解决方案:\n');
    console.log('1. 启动 Mock API 服务器:');
    console.log('   node complete-mock-api.js\n');
    console.log('2. 或使用一键启动脚本:');
    console.log('   start-paper-test.bat\n');
  }

  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n');
}

runTests().catch(console.error);
