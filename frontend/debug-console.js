/**
 * 浏览器控制台调试脚本
 * 在浏览器控制台中直接运行此脚本来诊断API问题
 *
 * 使用方法：
 * 1. 打开 http://localhost:5173
 * 2. 按 F12 打开控制台
 * 3. 复制并粘贴此脚本内容
 * 4. 运行 testStatsAPI() 函数
 */

// 测试统计数据API
async function testStatsAPI() {
    console.log('=== 开始测试统计API ===\n');

    // 测试1: 直接axios调用
    console.log('📍 测试1: 直接调用 /api/stats/overview');
    try {
        const response = await fetch('/api/stats/overview');
        console.log('Response status:', response.status);
        console.log('Response headers:', Object.fromEntries(response.headers.entries()));

        const data = await response.json();
        console.log('✅ 原始响应:', data);

        if (data.success && data.data) {
            console.log('✅ 提取的数据:', data.data);
        } else {
            console.log('⚠️ 响应格式异常');
        }
    } catch (error) {
        console.error('❌ 请求失败:', error);
    }

    console.log('\n');

    // 测试2: 测试响应拦截器
    console.log('📍 测试2: 测试axios拦截器');
    try {
        // 导入request模块（如果在Vue组件中）
        const response = await fetch('/api/stats/overview');
        const raw = await response.json();

        console.log('原始响应:', raw);
        console.log('需要提取 data.data:', raw.data);
    } catch (error) {
        console.error('❌ 测试失败:', error);
    }

    console.log('\n');

    // 测试3: 测试Stats.vue组件状态
    console.log('📍 测试3: 检查Vue应用状态');
    if (window.__VUE__) {
        console.log('✅ Vue已安装');
        console.log('Vue版本:', window.__VUE__.version);

        // 尝试访问Vue实例
        if (window.__VUE_APP__) {
            console.log('Vue应用实例:', window.__VUE_APP__);
        }
    } else {
        console.log('⚠️ Vue未检测到');
    }

    console.log('\n=== 测试完成 ===');
}

// 测试搜索API
async function testSearchAPI() {
    console.log('=== 开始测试搜索API ===\n');

    console.log('📍 测试: 搜索 "AI" 相关论文');
    try {
        const response = await fetch('/api/search?q=AI&limit=5&offset=0');
        const data = await response.json();

        console.log('✅ 搜索结果:', data);
        console.log('论文数量:', data.data?.papers?.length || 0);

        if (data.data?.papers) {
            console.log('\n前5篇论文:');
            data.data.papers.forEach((paper, index) => {
                console.log(`${index + 1}. ${paper.title}`);
                console.log(`   期刊: ${paper.journal.full} (${paper.journal.short})`);
                console.log(`   年份: ${paper.year}, 级别: ${paper.level}\n`);
            });
        }
    } catch (error) {
        console.error('❌ 搜索失败:', error);
    }

    console.log('\n=== 测试完成 ===');
}

// 网络连接诊断
function diagnoseNetwork() {
    console.log('=== 网络连接诊断 ===\n');

    console.log('当前页面:', window.location.href);
    console.log('API baseURL:', '/api');
    console.log('预期完整URL:', window.location.origin + '/api/stats/overview');

    console.log('\n测试连接性:');

    // 测试后端直接连接
    fetch('http://localhost:8080/health')
        .then(response => response.json())
        .then(data => console.log('✅ 后端直连成功:', data))
        .catch(error => console.error('❌ 后端直连失败:', error));

    // 测试代理连接
    fetch('/health')
        .then(response => response.json())
        .then(data => console.log('✅ 代理连接成功:', data))
        .catch(error => console.error('❌ 代理连接失败:', error));
}

// Vue组件状态检查
function checkVueComponent() {
    console.log('=== Vue组件状态检查 ===\n');

    // 查找Vue DevTools
    if (window.__VUE_DEVTOOLS_GLOBAL_HOOK__) {
        console.log('✅ Vue DevTools已安装');
        const apps = window.__VUE_DEVTOOLS_GLOBAL_HOOK__.apps;
        console.log('已注册的Vue应用:', apps);
    } else {
        console.log('⚠️ Vue DevTools未检测到');
    }

    // 检查当前路由
    console.log('当前路由:', window.location.pathname);

    console.log('\n提示: 如果Vue DevTools已安装，可以检查组件状态：');
    console.log('1. 打开Vue DevTools标签');
    console.log('2. 选择Stats组件');
    console.log('3. 检查以下属性:');
    console.log('   - stats.loading (应该为 false)');
    console.log('   - stats.overview (应该包含数据)');
    console.log('   - stats.error (应该为 null)');
}

// 运行所有测试
async function runAllTests() {
    console.clear();
    console.log('🚀 开始全面诊断...\n');

    await testStatsAPI();
    console.log('\n---\n');

    await testSearchAPI();
    console.log('\n---\n');

    diagnoseNetwork();
    console.log('\n---\n');

    checkVueComponent();

    console.log('\n✅ 所有测试完成！');
}

// 导出函数到全局
window.testStatsAPI = testStatsAPI;
window.testSearchAPI = testSearchAPI;
window.diagnoseNetwork = diagnoseNetwork;
window.checkVueComponent = checkVueComponent;
window.runAllTests = runAllTests;

console.log('✅ 调试脚本已加载！');
console.log('📝 可用命令:');
console.log('   - runAllTests()      运行所有测试');
console.log('   - testStatsAPI()     测试统计API');
console.log('   - testSearchAPI()    测试搜索API');
console.log('   - diagnoseNetwork()  诊断网络连接');
console.log('   - checkVueComponent() 检查Vue组件状态');
console.log('\n💡 提示: 输入 runAllTests() 开始诊断\n');
