<template>
  <div class="research-plan-panel">
    <div class="panel-header">
      <h2>🎯 研究规划助手</h2>
      <el-button type="primary" :loading="generating" @click="generatePlan">
        生成研究规划
      </el-button>
    </div>

    <!-- 参数配置 -->
    <div class="config-section">
      <el-form :model="config" label-position="top">
        <el-form-item label="研究领域">
          <el-input
            v-model="config.researchField"
            placeholder="例如：深度学习在自然语言处理中的应用"
          />
        </el-form-item>

        <el-form-item label="研究兴趣">
          <el-select
            v-model="config.interests"
            multiple
            filterable
            allow-create
            placeholder="选择或输入您的研究兴趣"
            style="width: 100%"
          >
            <el-option
              v-for="interest in commonInterests"
              :key="interest"
              :label="interest"
              :value="interest"
            />
          </el-select>
        </el-form-item>

        <el-form-item label="目标受众">
          <el-radio-group v-model="config.targetAudience">
            <el-radio label="academic">学术界</el-radio>
            <el-radio label="industry">工业界</el-radio>
            <el-radio label="both">学术界与工业界</el-radio>
          </el-radio-group>
        </el-form-item>

        <el-form-item label="时间框架">
          <el-select v-model="config.timeframe" placeholder="选择研究周期">
            <el-option label="3个月" value="3m" />
            <el-option label="6个月" value="6m" />
            <el-option label="1年" value="1y" />
            <el-option label="2年" value="2y" />
          </el-select>
        </el-form-item>
      </el-form>
    </div>

    <!-- 生成结果 -->
    <div v-if="planResult" class="plan-result">
      <div class="result-header">
        <h3>{{ planResult.title }}</h3>
        <el-tag type="success">{{ planResult.researchField }}</el-tag>
      </div>

      <el-tabs v-model="activeTab" type="card">
        <el-tab-pane label="📋 研究目标" name="objectives">
          <div class="tab-content">
            <ul>
              <li v-for="(obj, index) in planResult.objectives" :key="index">
                <el-icon color="#67c23a"><Check /></el-icon>
                <span>{{ obj }}</span>
              </li>
            </ul>
          </div>
        </el-tab-pane>

        <el-tab-pane label="🔬 研究方法" name="methodology">
          <div class="tab-content">
            <p>{{ planResult.methodology }}</p>
          </div>
        </el-tab-pane>

        <el-tab-pane label="🎯 预期成果" name="outcomes">
          <div class="tab-content">
            <ul>
              <li v-for="(outcome, index) in planResult.expectedOutcomes" :key="index">
                <el-icon color="#409eff"><CircleCheck /></el-icon>
                <span>{{ outcome }}</span>
              </li>
            </ul>
          </div>
        </el-tab-pane>

        <el-tab-pane label="📅 时间线" name="timeline">
          <div class="tab-content">
            <el-timeline>
              <el-timeline-item
                v-for="(milestone, index) in planResult.timeline"
                :key="index"
                :timestamp="milestone.deadline"
                placement="top"
              >
                <el-card>
                  <h4>{{ milestone.title }}</h4>
                  <p>{{ milestone.description }}</p>
                </el-card>
              </el-timeline-item>
            </el-timeline>
          </div>
        </el-tab-pane>

        <el-tab-pane label="🛠️ 资源需求" name="resources">
          <div class="tab-content">
            <ul>
              <li v-for="(resource, index) in planResult.resources" :key="index">
                <el-icon color="#e6a23c"><Tools /></el-icon>
                <span>{{ resource }}</span>
              </li>
            </ul>
          </div>
        </el-tab-pane>

        <el-tab-pane label="⚠️ 潜在挑战" name="challenges">
          <div class="tab-content">
            <el-alert
              v-for="(challenge, index) in planResult.potentialChallenges"
              :key="index"
              :title="challenge"
              type="warning"
              :closable="false"
              show-icon
            />
          </div>
        </el-tab-pane>
      </el-tabs>

      <div class="result-actions">
        <el-button type="primary" @click="exportPlan">
          📥 导出研究规划
        </el-button>
        <el-button @click="savePlan">
          💾 保存到我的规划
        </el-button>
        <el-button @click="sharePlan">
          🔗 分享给导师
        </el-button>
      </div>
    </div>

    <!-- 加载状态 -->
    <div v-if="generating" class="loading-state">
      <el-icon class="is-loading"><Loading /></el-icon>
      <p>AI正在制定研究规划，请稍候...</p>
      <el-progress :percentage="progress" :status="progressStatus" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { ElMessage } from 'element-plus'
import { Loading, Check, CircleCheck, Tools } from '@element-plus/icons-vue'
import { aiCopilotApi } from '@/api/modules/aiCopilot'

const config = ref({
  researchField: '',
  interests: [] as string[],
  targetAudience: 'academic',
  timeframe: '6m'
})

const commonInterests = [
  '深度学习',
  '机器学习',
  '自然语言处理',
  '计算机视觉',
  '强化学习',
  '知识图谱',
  '数据挖掘',
  '推荐系统'
]

const planResult = ref<any>(null)
const generating = ref(false)
const progress = ref(0)
const progressStatus = ref<'success' | 'exception' | ''>('')
const activeTab = ref('objectives')

const generatePlan = async () => {
  if (!config.value.researchField) {
    ElMessage.warning('请输入研究领域')
    return
  }

  if (config.value.interests.length === 0) {
    ElMessage.warning('请选择至少一个研究兴趣')
    return
  }

  generating.value = true
  progress.value = 0

  // 模拟进度
  const progressInterval = setInterval(() => {
    if (progress.value < 90) {
      progress.value += 10
    }
  }, 500)

  try {
    const result = await aiCopilotApi.generateResearchPlan({
      researchField: config.value.researchField,
      interests: config.value.interests
    })

    clearInterval(progressInterval)
    progress.value = 100
    progressStatus.value = 'success'

    planResult.value = result
    ElMessage.success('研究规划生成成功')
  } catch (error) {
    clearInterval(progressInterval)
    progressStatus.value = 'exception'
    ElMessage.error('生成失败，请稍后重试')
  } finally {
    setTimeout(() => {
      generating.value = false
      progress.value = 0
      progressStatus.value = ''
    }, 2000)
  }
}

const exportPlan = () => {
  ElMessage.info('导出功能开发中...')
}

const savePlan = () => {
  ElMessage.success('研究规划已保存')
}

const sharePlan = () => {
  ElMessage.info('分享功能开发中...')
}
</script>

<style scoped>
.research-plan-panel {
  padding: 24px;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 32px;
}

.panel-header h2 {
  font-size: 24px;
  font-weight: 700;
  margin: 0;
}

.config-section {
  margin-bottom: 32px;
  padding: 24px;
  background: #f5f7fa;
  border-radius: 12px;
}

.plan-result {
  background: white;
  border-radius: 12px;
  padding: 24px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.result-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
}

.result-header h3 {
  font-size: 20px;
  font-weight: 700;
  margin: 0;
}

.tab-content {
  padding: 20px 0;
}

.tab-content ul {
  margin: 0;
  padding-left: 0;
  list-style: none;
}

.tab-content li {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 16px;
  font-size: 15px;
  line-height: 1.6;
  color: #606266;
}

.tab-content p {
  line-height: 1.8;
  color: #606266;
  margin: 0;
}

.result-actions {
  display: flex;
  gap: 12px;
  margin-top: 24px;
  padding-top: 16px;
  border-top: 1px solid #e4e7ed;
}

.loading-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 60px 20px;
  color: #909399;
}

.loading-state .el-icon {
  font-size: 48px;
  margin-bottom: 16px;
}

.loading-state p {
  margin: 0 0 24px 0;
  font-size: 16px;
}

.loading-state .el-progress {
  width: 300px;
}
</style>
