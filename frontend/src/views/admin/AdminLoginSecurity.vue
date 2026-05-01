<template>
  <div class="tab-content">
    <!-- 安全子标签 -->
    <el-tabs :model-value="securitySubTab" @update:model-value="(val: string) => $emit('update:securitySubTab', val)" class="security-sub-tabs">
      <!-- 登录历史 -->
      <el-tab-pane label="登录历史" name="history">
        <el-row :gutter="20" class="mb-3">
          <el-col :span="12">
            <el-input
              :model-value="loginHistoryUsernameFilter"
              placeholder="按用户名过滤"
              clearable
              @change="$emit('load-login-history')"
              @update:model-value="(val: string) => $emit('update:loginHistoryUsernameFilter', val)"
              style="width: 300px"
            >
              <template #prefix>
                <el-icon><Search /></el-icon>
              </template>
            </el-input>
          </el-col>
          <el-col :span="12">
            <el-button @click="$emit('load-login-history')" :loading="securityLoading">
              <el-icon><Refresh /></el-icon> 刷新
            </el-button>
          </el-col>
        </el-row>
        <el-table :data="allLoginHistory" stripe v-loading="securityLoading" style="width: 100%">
          <el-table-column prop="username" label="用户名" width="140" />
          <el-table-column prop="ip_address" label="IP地址" width="140" />
          <el-table-column label="状态" width="90">
            <template #default="{ row }">
              <el-tag :type="row.success ? 'success' : 'danger'" size="small">
                {{ row.success ? '成功' : '失败' }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="failure_reason" label="失败原因" min-width="150" show-overflow-tooltip />
          <el-table-column prop="user_agent" label="用户代理" min-width="200" show-overflow-tooltip />
          <el-table-column prop="created_at" label="时间" min-width="160" />
        </el-table>
        <el-pagination
          :current-page="loginHistoryPagination.page"
          :page-size="loginHistoryPagination.limit"
          :total="loginHistoryPagination.total"
          layout="total, prev, pager, next"
          @current-change="(p: number) => $emit('login-history-page-change', p)"
          class="mt-3"
        />
      </el-tab-pane>

      <!-- 可疑登录 -->
      <el-tab-pane label="可疑登录" name="suspicious">
        <el-row :gutter="20" class="mb-3">
          <el-col :span="12">
            <el-select :model-value="suspiciousStatusFilter" placeholder="状态过滤" clearable @change="(val: string) => { $emit('update:suspiciousStatusFilter', val); $emit('load-suspicious-logins') }">
              <el-option label="全部" value="" />
              <el-option label="待处理" value="pending" />
              <el-option label="已审核" value="reviewed" />
              <el-option label="已白名单" value="whitelisted" />
              <el-option label="确认威胁" value="confirmed_threat" />
            </el-select>
          </el-col>
          <el-col :span="12">
            <el-button @click="$emit('load-suspicious-logins')" :loading="securityLoading">
              <el-icon><Refresh /></el-icon> 刷新
            </el-button>
          </el-col>
        </el-row>
        <el-table :data="suspiciousLogins" stripe v-loading="securityLoading" style="width: 100%">
          <el-table-column prop="username" label="用户名" width="140" />
          <el-table-column prop="ip_address" label="IP地址" width="140" />
          <el-table-column prop="suspicion_reason" label="可疑原因" width="130">
            <template #default="{ row }">
              {{ ({
                multiple_failures: '多次失败',
                unknown_location: '未知位置',
                impossible_travel: '不可能的行程',
                bot_pattern: '机器人模式',
                blacklisted_ip: '黑名单IP'
              } as Record<string, string>)[row.suspicion_reason] || row.suspicion_reason }}
            </template>
          </el-table-column>
          <el-table-column prop="risk_score" label="风险分数" width="90" align="center">
            <template #default="{ row }">
              <el-tag :type="row.risk_score >= 80 ? 'danger' : row.risk_score >= 50 ? 'warning' : 'info'" size="small">
                {{ row.risk_score }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="status" label="状态" width="100">
            <template #default="{ row }">
              <el-tag :type="getSuspiciousStatusTagType(row.status)" size="small">
                {{ getSuspiciousStatusLabel(row.status) }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="created_at" label="检测时间" min-width="160" />
          <el-table-column label="操作" width="180" fixed="right">
            <template #default="{ row }">
              <el-button size="small" @click="$emit('suspicious-login-action', row.id, 'reviewed')" :disabled="row.status !== 'pending'">标记已审</el-button>
              <el-button size="small" type="success" @click="$emit('suspicious-login-action', row.id, 'whitelisted')" :disabled="row.status !== 'pending'">白名单</el-button>
              <el-button size="small" type="danger" @click="$emit('suspicious-login-action', row.id, 'confirmed_threat')" :disabled="row.status !== 'pending'">确认威胁</el-button>
            </template>
          </el-table-column>
        </el-table>
        <el-pagination
          :current-page="suspiciousPagination.page"
          :page-size="suspiciousPagination.limit"
          :total="suspiciousPagination.total"
          layout="total, prev, pager, next"
          @current-change="(p: number) => $emit('suspicious-page-change', p)"
          class="mt-3"
        />
      </el-tab-pane>

      <!-- IP黑名单 -->
      <el-tab-pane label="IP黑名单" name="blacklist">
        <el-row :gutter="20" class="mb-3">
          <el-col :span="18">
            <el-button type="danger" @click="$emit('show-add-ip-blacklist-dialog')">
              <el-icon><Plus /></el-icon> 添加IP黑名单
            </el-button>
          </el-col>
          <el-col :span="6">
            <el-button @click="$emit('load-ip-blacklist')" :loading="securityLoading">
              <el-icon><Refresh /></el-icon> 刷新
            </el-button>
          </el-col>
        </el-row>
        <el-table :data="ipBlacklist" stripe v-loading="securityLoading" style="width: 100%">
          <el-table-column prop="ip_address" label="IP地址" width="150" />
          <el-table-column prop="reason" label="原因" min-width="200" show-overflow-tooltip />
          <el-table-column prop="threat_level" label="威胁等级" width="100">
            <template #default="{ row }">
              <el-tag :type="getThreatLevelTagType(row.threat_level)" size="small">
                {{ getThreatLevelLabel(row.threat_level) }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="attempt_count" label="尝试次数" width="90" align="center" />
          <el-table-column prop="created_by" label="创建者" width="120" />
          <el-table-column prop="expires_at" label="过期时间" min-width="160">
            <template #default="{ row }">
              {{ row.expires_at || '永久' }}
            </template>
          </el-table-column>
          <el-table-column label="操作" width="100" fixed="right">
            <template #default="{ row }">
              <el-button size="small" type="danger" @click="$emit('remove-ip-blacklist', row.id)">移除</el-button>
            </template>
          </el-table-column>
        </el-table>
        <el-pagination
          :current-page="ipBlacklistPagination.page"
          :page-size="ipBlacklistPagination.limit"
          :total="ipBlacklistPagination.total"
          layout="total, prev, pager, next"
          @current-change="(p: number) => $emit('ip-blacklist-page-change', p)"
          class="mt-3"
        />
      </el-tab-pane>

      <!-- 账户锁定 -->
      <el-tab-pane label="账户锁定" name="lockouts">
        <el-row :gutter="20" class="mb-3">
          <el-col :span="18">
            <el-button type="primary" @click="$emit('show-lock-user-dialog')">
              <el-icon><Lock /></el-icon> 锁定用户
            </el-button>
          </el-col>
          <el-col :span="6">
            <el-button @click="$emit('load-account-lockouts')" :loading="securityLoading">
              <el-icon><Refresh /></el-icon> 刷新
            </el-button>
          </el-col>
        </el-row>
        <el-table :data="accountLockouts" stripe v-loading="securityLoading" style="width: 100%">
          <el-table-column prop="username" label="用户名" width="140" />
          <el-table-column prop="lockout_reason" label="锁定原因" min-width="180" show-overflow-tooltip />
          <el-table-column prop="failed_attempts" label="失败次数" width="90" align="center" />
          <el-table-column prop="ip_address" label="IP地址" width="140" />
          <el-table-column prop="locked_until" label="锁定到期" min-width="160" />
          <el-table-column prop="created_at" label="创建时间" min-width="160" />
          <el-table-column label="操作" width="100" fixed="right">
            <template #default="{ row }">
              <el-button size="small" type="primary" @click="$emit('unlock-user-account', row.user_id)">解锁</el-button>
            </template>
          </el-table-column>
        </el-table>
        <el-pagination
          :current-page="accountLockoutsPagination.page"
          :page-size="accountLockoutsPagination.limit"
          :total="accountLockoutsPagination.total"
          layout="total, prev, pager, next"
          @current-change="(p: number) => $emit('account-lockouts-page-change', p)"
          class="mt-3"
        />
      </el-tab-pane>
    </el-tabs>

    <!-- 添加IP黑名单对话框 -->
    <el-dialog :model-value="addIpBlacklistDialogVisible" title="添加IP黑名单" width="500px" @update:model-value="(val: boolean) => $emit('update:addIpBlacklistDialogVisible', val)">
      <el-form :model="addIpBlacklistForm" label-width="100px">
        <el-form-item label="IP地址" required>
          <el-input :model-value="addIpBlacklistForm.ipAddress" @update:model-value="(val: string) => $emit('update:addIpBlacklistForm', 'ipAddress', val)" placeholder="192.168.1.1 或 192.168.1.0/24" />
        </el-form-item>
        <el-form-item label="原因" required>
          <el-input :model-value="addIpBlacklistForm.reason" @update:model-value="(val: string) => $emit('update:addIpBlacklistForm', 'reason', val)" placeholder="例如: 暴力破解攻击" />
        </el-form-item>
        <el-form-item label="威胁等级">
          <el-select :model-value="addIpBlacklistForm.threatLevel" @update:model-value="(val: string) => $emit('update:addIpBlacklistForm', 'threatLevel', val)">
            <el-option label="低" value="low" />
            <el-option label="中" value="medium" />
            <el-option label="高" value="high" />
            <el-option label="严重" value="critical" />
          </el-select>
        </el-form-item>
        <el-form-item label="过期时间">
          <el-date-picker
            :model-value="addIpBlacklistForm.expiresAt"
            @update:model-value="(val: string) => $emit('update:addIpBlacklistForm', 'expiresAt', val)"
            type="datetime"
            placeholder="留空表示永久"
            format="YYYY-MM-DD HH:mm:ss"
            value-format="YYYY-MM-DD HH:mm:ss"
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="$emit('update:addIpBlacklistDialogVisible', false)">取消</el-button>
        <el-button type="primary" @click="$emit('add-ip-blacklist')" :loading="securityLoading">添加</el-button>
      </template>
    </el-dialog>

    <!-- 锁定用户对话框 -->
    <el-dialog :model-value="lockUserDialogVisible" title="锁定用户账户" width="500px" @update:model-value="(val: boolean) => $emit('update:lockUserDialogVisible', val)">
      <el-form :model="lockUserForm" label-width="100px">
        <el-form-item label="用户ID" required>
          <el-input-number :model-value="lockUserForm.userId" @update:model-value="(val: number) => $emit('update:lockUserForm', 'userId', val)" :min="1" />
        </el-form-item>
        <el-form-item label="锁定时长">
          <el-input-number :model-value="lockUserForm.lockMinutes" @update:model-value="(val: number) => $emit('update:lockUserForm', 'lockMinutes', val)" :min="5" :max="1440" />
          <span class="ml-2">分钟</span>
        </el-form-item>
        <el-form-item label="锁定原因">
          <el-input :model-value="lockUserForm.reason" @update:model-value="(val: string) => $emit('update:lockUserForm', 'reason', val)" placeholder="例如: 可疑登录活动" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="$emit('update:lockUserDialogVisible', false)">取消</el-button>
        <el-button type="primary" @click="$emit('lock-user-account-confirm')" :loading="securityLoading">锁定</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { Search, Refresh, Plus, Lock } from '@element-plus/icons-vue'

defineProps<{
  securitySubTab: string
  securityLoading: boolean
  loginHistoryUsernameFilter: string
  allLoginHistory: any[]
  loginHistoryPagination: { page: number; limit: number; total: number }
  suspiciousStatusFilter: string
  suspiciousLogins: any[]
  suspiciousPagination: { page: number; limit: number; total: number }
  ipBlacklist: any[]
  ipBlacklistPagination: { page: number; limit: number; total: number }
  addIpBlacklistDialogVisible: boolean
  addIpBlacklistForm: { ipAddress: string; reason: string; threatLevel: string; expiresAt: string }
  accountLockouts: any[]
  accountLockoutsPagination: { page: number; limit: number; total: number }
  lockUserDialogVisible: boolean
  lockUserForm: { userId: number; lockMinutes: number; reason: string }
  getSuspiciousStatusLabel: (status: string) => string
  getSuspiciousStatusTagType: (status: string) => string
  getThreatLevelLabel: (level: string) => string
  getThreatLevelTagType: (level: string) => string
}>()

defineEmits<{
  (e: 'update:securitySubTab', value: string): void
  (e: 'update:loginHistoryUsernameFilter', value: string): void
  (e: 'update:suspiciousStatusFilter', value: string): void
  (e: 'update:addIpBlacklistDialogVisible', value: boolean): void
  (e: 'update:addIpBlacklistForm', field: string, value: any): void
  (e: 'update:lockUserDialogVisible', value: boolean): void
  (e: 'update:lockUserForm', field: string, value: any): void
  (e: 'load-login-history'): void
  (e: 'load-suspicious-logins'): void
  (e: 'load-ip-blacklist'): void
  (e: 'load-account-lockouts'): void
  (e: 'show-add-ip-blacklist-dialog'): void
  (e: 'add-ip-blacklist'): void
  (e: 'remove-ip-blacklist', id: number): void
  (e: 'show-lock-user-dialog'): void
  (e: 'lock-user-account-confirm'): void
  (e: 'unlock-user-account', userId: number): void
  (e: 'suspicious-login-action', id: number, action: string): void
  (e: 'login-history-page-change', page: number): void
  (e: 'suspicious-page-change', page: number): void
  (e: 'ip-blacklist-page-change', page: number): void
  (e: 'account-lockouts-page-change', page: number): void
}>()
</script>
