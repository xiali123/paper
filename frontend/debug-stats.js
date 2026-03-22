/**
 * Debug script for statistics page issues
 * Run this in the browser console when on the Stats page
 */

console.log('=== STATS PAGE DEBUG SCRIPT ===');

// Test 1: Check if Vue app is running
console.log('1. Checking Vue app...');
if (window.__VUE__) {
    console.log('✅ Vue is detected');
} else {
    console.log('❌ Vue not detected');
}

// Test 2: Test API call directly
console.log('\n2. Testing API call directly...');
fetch('/api/stats/overview')
    .then(response => {
        console.log('✅ Fetch response received:', response);
        return response.json();
    })
    .then(data => {
        console.log('✅ API Response data:', data);
        console.log('   - success:', data.success);
        console.log('   - has data:', !!data.data);
        console.log('   - data keys:', data.data ? Object.keys(data.data) : 'N/A');
        console.log('   - totalPapers:', data.data?.totalPapers);
        console.log('   - mostActiveJournal:', data.data?.mostActiveJournal);
    })
    .catch(error => {
        console.error('❌ API call failed:', error);
    });

// Test 3: Check Vue DevTools
console.log('\n3. Checking for Vue DevTools...');
if (window.__VUE_DEVTOOLS_GLOBAL_HOOK__) {
    console.log('✅ Vue DevTools detected');
} else {
    console.log('⚠️ Vue DevTools not detected (install for better debugging)');
}

// Test 4: Check component state (if possible)
console.log('\n4. Component state check...');
console.log('Instructions:');
console.log('1. Open Vue DevTools');
console.log('2. Select the Stats component');
console.log('3. Check the following:');
console.log('   - stats.loading should be: false');
console.log('   - stats.hasData should be: true');
console.log('   - stats.overview should be: an object with data');
console.log('   - stats.error should be: null');

// Test 5: Manually trigger state update
console.log('\n5. Manual state update test...');
console.log('You can manually test reactivity by running:');
console.log(`
// In browser console, find the component instance and try:
const app = document.querySelector('#app').__vueParentComponent;
// Then check the component state
`);

// Test 6: Check for console errors
console.log('\n6. Checking for errors...');
const originalError = console.error;
console.error = function(...args) {
    console.log('❌ Console.error called:', args);
    originalError.apply(console, args);
};

setTimeout(() => {
    console.error = originalError;
    console.log('\n=== DEBUG COMPLETE ===');
    console.log('Check the output above for any issues');
}, 5000);

console.log('Waiting 5 seconds to capture any errors...');
