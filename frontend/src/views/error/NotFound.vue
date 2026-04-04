<template>
  <div class="not-found">
    <div class="content">
      <h1>404</h1>
      <p>抱歉，您访问的页面不存在</p>
      <el-button type="primary" @click="goHome">返回首页</el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router'

const router = useRouter()

function goHome() {
  router.push('/dashboard')
}
</script>

<style scoped lang="scss">
.not-found {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 100vh;
  background-color: #f5f7fa;

  .content {
    text-align: center;

    h1 {
      font-size: 120px;
      font-weight: bold;
      color: #409eff;
      margin: 0 0 20px;
    }

    p {
      font-size: 18px;
      color: #606266;
      margin-bottom: 30px;
    }
  }
}
</style>
