/**
 * PaperCrawler API 端点测试脚本
 *
 * 测试所有论文管理相关的 API 端点
 */

const http = require('http');

const API_BASE = 'http://localhost:8082';

function testEndpoint(method, path, data = null) {
  return new Promise((resolve, reject) => {
    const url = new URL(path, API_BASE);
    const options = {
      method: method,
      headers: {
        'Content-Type': 'application/json'
      }
    };

    const req = http.request(url, options, (res) => {
      let body = '';
      res.on('data', (chunk) => body += chunk);
      res.on('end', () => {
        try {
          const json = JSON.parse(body);
          resolve({
            status: res.statusCode,
            data: json
          });
        } catch (e) {
          resolve({
            status: res.statusCode,
            data: body
          });
        }
      });
    });

    req.on('error', reject);

    if (data) {
      req.write(JSON.stringify(data));
    }

    req.end();
  });
}

async function runTests() {
  console.log('🧪 PaperCrawler API 端点测试\n');
  console.log('📍 API 地址:', API_BASE);
  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n');

  const tests = [
    {
      name: '1. 健康检查',
      method: 'GET',
      path: '/api/health'
    },
    {
      name: '2. 获取论文列表',
      method: 'GET',
      path: '/api/papers?page=1&pageSize=20'
    },
    {
      name: '3. 获取论文统计',
      method: 'GET',
      path: '/api/papers/stats'
    },
    {
      name: '4. 搜索论文',
      method: 'GET',
      path: '/api/papers/search?q=attention'
    },
    {
      name: '5. 获取单个论文详情',
      method: 'GET',
      path: '/api/papers/1'
    }
  ];

  let passed = 0;
  let failed = 0;

  for (const test of tests) {
    try {
      console.log(`🔍 ${test.name}`);
      console.log(`   ${test.method} ${test.path}`);

      const result = await testEndpoint(test.method, test.path);

      if (result.status === 200 && result.data.success) {
        console.log(`   ✅ PASS (Status: ${result.status})`);
        if (result.data.data) {
          if (result.data.data.papers) {
            console.log(`   📊 返回 ${result.data.data.papers.length} 篇论文`);
          } else if (result.data.data.totalPapers !== undefined) {
            console.log(`   📊 总计: ${result.data.data.totalPapers} 篇`);
          }
        }
        passed++;
      } else {
        console.log(`   ❌ FAIL (Status: ${result.status})`);
        console.log(`   响应:`, JSON.stringify(result.data).substring(0, 100));
        failed++;
      }
    } catch (error) {
      console.log(`   ❌ ERROR: ${error.message}`);
      failed++;
    }
    console.log();
  }

  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━');
  console.log(`\n📊 测试结果:`);
  console.log(`   ✅ 通过: ${passed}/${tests.length}`);
  console.log(`   ❌ 失败: ${failed}/${tests.length}`);
  console.log();

  if (failed === 0) {
    console.log('🎉 所有测试通过！API 正常工作。\n');
    console.log('💡 现在可以访问前端应用测试完整功能:');
    console.log('   http://localhost:5173/papers\n');
  } else {
    console.log('❌ 部分测试失败。请检查:\n');
    console.log('1. Mock API 服务器是否正在运行?');
    console.log('   node complete-mock-api.js\n');
    console.log('2. 端口 8082 是否被占用?\n');
    console.log('3. 是否需要重启 Mock API 服务器?\n');
  }

  console.log('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n');
}

// 运行测试
runTests().catch(console.error);
