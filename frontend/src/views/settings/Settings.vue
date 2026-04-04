<template>
  <div class="settings">
    <div class="page-header">
      <h1 class="page-title">设置</h1>
    </div>

    <el-card>
      <el-tabs v-model="activeTab">
        <el-tab-pane label="基本设置" name="basic">
          <el-form :model="basicForm" label-width="120px">
            <el-form-item label="系统名称">
              <el-input v-model="basicForm.systemName" />
            </el-form-item>
            <el-form-item label="系统描述">
              <el-input v-model="basicForm.systemDesc" type="textarea" :rows="3" />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" @click="handleSaveBasic">保存</el-button>
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="个人设置" name="profile">
          <el-form :model="profileForm" label-width="120px">
            <el-form-item label="用户名">
              <el-input v-model="profileForm.username" disabled />
            </el-form-item>
            <el-form-item label="姓名">
              <el-input v-model="profileForm.name" />
            </el-form-item>
            <el-form-item label="邮箱">
              <el-input v-model="profileForm.email" />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" @click="handleSaveProfile">保存</el-button>
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="安全设置" name="security">
          <el-form :model="securityForm" label-width="120px">
            <el-form-item label="当前密码">
              <el-input v-model="securityForm.currentPassword" type="password" />
            </el-form-item>
            <el-form-item label="新密码">
              <el-input v-model="securityForm.newPassword" type="password" />
            </el-form-item>
            <el-form-item label="确认密码">
              <el-input v-model="securityForm.confirmPassword" type="password" />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" @click="handleSaveSecurity">修改密码</el-button>
            </el-form-item>
          </el-form>
        </el-tab-pane>
      </el-tabs>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import { ElMessage } from 'element-plus'

const activeTab = ref('basic')

const basicForm = reactive({
  systemName: 'PaperCrawler',
  systemDesc: '学术论文爬取管理系统',
})

const profileForm = reactive({
  username: 'admin',
  name: 'Admin',
  email: 'admin@example.com',
})

const securityForm = reactive({
  currentPassword: '',
  newPassword: '',
  confirmPassword: '',
})

function handleSaveBasic() {
  ElMessage.success('保存成功')
}

function handleSaveProfile() {
  ElMessage.success('保存成功')
}

function handleSaveSecurity() {
  if (securityForm.newPassword !== securityForm.confirmPassword) {
    ElMessage.error('两次输入的密码不一致')
    return
  }
  ElMessage.success('密码修改成功')
}
</script>

<style scoped lang="scss">
.settings {
  padding: 20px;
}
</style>
