<template>
  <div class="ai-history-panel">
    <div class="panel-header">
      <h3>历史记录</h3>
      <el-button type="primary" :icon="Plus" @click="handleNew">
        新建任务
      </el-button>
    </div>

    <el-tabs v-model="activeTab" class="history-tabs">
      <el-tab-pane label="全部" name="all">
        <div class="history-list">
          <el-empty v-if="history.length === 0" description="暂无历史记录" />
          <div v-else class="history-items">
            <el-card
              v-for="item in history"
              :key="item.id"
              class="history-item"
              shadow="hover"
            >
              <div class="item-header">
                <el-tag :type="getTypeColor(item.type)" size="small">
                  {{ getTypeLabel(item.type) }}
                </el-tag>
                <span class="item-time">{{ formatTime(item.createdAt) }}</span>
              </div>
              <div class="item-content">
                <h4>{{ item.title }}</h4>
                <p>{{ item.description }}</p>
              </div>
              <div class="item-actions">
                <el-button size="small" :icon="View" @click="viewItem(item)">
                  查看详情
                </el-button>
                <el-button size="small" :icon="Download" @click="downloadItem(item)">
                  下载
                </el-button>
                <el-button size="small" type="danger" :icon="Delete" @click="deleteItem(item)">
                  删除
                </el-button>
              </div>
            </el-card>
          </div>
        </div>
      </el-tab-pane>

      <el-tab-pane label="AI审稿" name="review">
        <div class="history-list">
          <el-empty v-if="reviewHistory.length === 0" description="暂无审稿记录" />
          <div v-else class="history-items">
            <el-card
              v-for="item in reviewHistory"
              :key="item.id"
              class="history-item"
              shadow="hover"
            >
              <div class="item-header">
                <el-tag type="primary" size="small">AI审稿</el-tag>
                <span class="item-time">{{ formatTime(item.createdAt) }}</span>
              </div>
              <div class="item-content">
                <h4>{{ item.title }}</h4>
                <p>评分: {{ item.score }}/10</p>
              </div>
              <div class="item-actions">
                <el-button size="small" :icon="View" @click="viewItem(item)">
                  查看详情
                </el-button>
              </div>
            </el-card>
          </div>
        </div>
      </el-tab-pane>

      <el-tab-pane label="文献综述" name="literature">
        <div class="history-list">
          <el-empty v-if="literatureHistory.length === 0" description="暂无综述记录" />
          <div v-else class="history-items">
            <el-card
              v-for="item in literatureHistory"
              :key="item.id"
              class="history-item"
              shadow="hover"
            >
              <div class="item-header">
                <el-tag type="success" size="small">文献综述</el-tag>
                <span class="item-time">{{ formatTime(item.createdAt) }}</span>
              </div>
              <div class="item-content">
                <h4>{{ item.title }}</h4>
                <p>{{ item.paperCount }}篇论文</p>
              </div>
              <div class="item-actions">
                <el-button size="small" :icon="View" @click="viewItem(item)">
                  查看详情
                </el-button>
              </div>
            </el-card>
          </div>
        </div>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { Plus, View, Download, Delete } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'

const activeTab = ref('all')

// Mock data
const history = ref([
  {
    id: 1,
    type: 'review',
    title: '深度学习论文审稿',
    description: '对关于卷积神经网络的论文进行了详细评审',
    score: 8.5,
    createdAt: new Date(Date.now() - 3600000).toISOString()
  },
  {
    id: 2,
    type: 'literature',
    title: '自然语言处理综述',
    description: '生成了一份关于Transformer模型的综述',
    paperCount: 15,
    createdAt: new Date(Date.now() - 7200000).toISOString()
  }
])

const reviewHistory = computed(() =>
  history.value.filter(item => item.type === 'review')
)

const literatureHistory = computed(() =>
  history.value.filter(item => item.type === 'literature')
)

const getTypeColor = (type: string) => {
  const colors: Record<string, string> = {
    review: 'primary',
    literature: 'success',
    plan: 'warning',
    chat: 'info'
  }
  return colors[type] || 'info'
}

const getTypeLabel = (type: string) => {
  const labels: Record<string, string> = {
    review: 'AI审稿',
    literature: '文献综述',
    plan: '研究规划',
    chat: 'AI对话'
  }
  return labels[type] || type
}

const formatTime = (dateStr: string) => {
  const date = new Date(dateStr)
  const now = new Date()
  const diff = now.getTime() - date.getTime()
  const hours = Math.floor(diff / 3600000)

  if (hours < 1) {
    const minutes = Math.floor(diff / 60000)
    return `${minutes}分钟前`
  } else if (hours < 24) {
    return `${hours}小时前`
  } else {
    const days = Math.floor(hours / 24)
    return `${days}天前`
  }
}

const handleNew = () => {
  ElMessage.info('新建功能开发中...')
}

const viewItem = (item: any) => {
  ElMessage.info(`查看详情: ${item.title}`)
}

const downloadItem = (item: any) => {
  ElMessage.success(`开始下载: ${item.title}`)
}

const deleteItem = (item: any) => {
  const index = history.value.findIndex(h => h.id === item.id)
  if (index > -1) {
    history.value.splice(index, 1)
    ElMessage.success('删除成功')
  }
}
</script>

<style scoped lang="scss">
.ai-history-panel {
  width: 100%;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: $spacing-5;

  h3 {
    margin: 0;
    font-size: $font-size-xl;
    font-weight: $font-weight-semibold;
    color: $text-primary;
  }
}

.history-tabs {
  :deep(.el-tabs__header) {
    margin-bottom: $spacing-5;
  }
}

.history-list {
  min-height: 400px;
}

.history-items {
  display: flex;
  flex-direction: column;
  gap: $spacing-4;
}

.history-item {
  .item-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: $spacing-3;

    .item-time {
      font-size: $font-size-sm;
      color: $text-secondary;
    }
  }

  .item-content {
    margin-bottom: $spacing-4;

    h4 {
      margin: 0 0 $spacing-2 0;
      font-size: $font-size-base;
      font-weight: $font-weight-semibold;
      color: $text-primary;
    }

    p {
      margin: 0;
      font-size: $font-size-sm;
      color: $text-regular;
      line-height: 1.5;
    }
  }

  .item-actions {
    display: flex;
    gap: $spacing-2;
  }
}

@media (max-width: 768px) {
  .panel-header {
    flex-direction: column;
    align-items: flex-start;
    gap: $spacing-3;

    .el-button {
      width: 100%;
    }
  }

  .item-actions {
    flex-direction: column;

    .el-button {
      width: 100%;
    }
  }
}
</style>
