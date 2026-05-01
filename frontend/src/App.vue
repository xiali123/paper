<template>
  <el-config-provider :locale="locale">
    <ErrorBoundary fallback-title="应用加载出错">
      <router-view />
    </ErrorBoundary>
  </el-config-provider>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import zhCn from 'element-plus/dist/locale/zh-cn.mjs'
import ErrorBoundary from '@/components/common/ErrorBoundary.vue'

const locale = ref(zhCn)
</script>

<style>
#app {
  font-family: 'Helvetica Neue', Helvetica, 'PingFang SC', 'Hiragino Sans GB',
    'Microsoft YaHei', Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
}
</style>
