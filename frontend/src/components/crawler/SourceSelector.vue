<template>
  <div class="source-selector">
    <el-select
      :model-value="modelValue"
      @update:model-value="$emit('update:modelValue', $event)"
      placeholder="选择数据源"
      :loading="loading"
      :disabled="disabled"
    >
      <el-option
        v-for="source in sources"
        :key="source.source"
        :label="source.name"
        :value="source.source"
        :disabled="!source.available"
      >
        <div class="source-option">
          <div class="source-info">
            <span class="source-name">{{ source.name }}</span>
            <span class="source-description">{{ source.description }}</span>
          </div>
          <div class="source-status">
            <el-tag
              v-if="!source.available"
              type="info"
              size="small"
            >
              不可用
            </el-tag>
            <el-tag
              v-else-if="connectionStatus[source.source]"
              type="success"
              size="small"
            >
              已连接
            </el-tag>
            <el-tag
              v-else
              type="warning"
              size="small"
            >
              未测试
            </el-tag>
          </div>
        </div>
      </el-option>
    </el-select>

    <el-button
      v-if="showTestButton"
      :icon="Connection"
      :loading="testing"
      @click="handleTestConnection"
      :disabled="!modelValue"
    >
      测试连接
    </el-button>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useCrawlerStore } from '@/stores/crawlerStore'
import { ElMessage } from 'element-plus'
import { Connection } from '@element-plus/icons-vue'
import type { CrawlerSource } from '@/api/modules/crawler'

defineProps<{
  modelValue: CrawlerSource | ''
  disabled?: boolean
  showTestButton?: boolean
}>()

defineEmits<{
  'update:modelValue': [value: CrawlerSource | '']
}>()

const crawlerStore = useCrawlerStore()

const loading = ref(false)
const testing = ref(false)
const sources = ref<Array<{
  source: CrawlerSource
  name: string
  description: string
  available: boolean
  requiresAuth: boolean
  maxLimit: number
}>>([])

const connectionStatus = ref<Record<CrawlerSource, boolean>>({})

const handleTestConnection = async () => {
  if (!sources.value) return

  const source = sources.value.find(s => s.source === props.modelValue)
  if (!source) return

  testing.value = true
  try {
    const result = await crawlerStore.testConnection(source.source)
    connectionStatus.value[source.source] = result.success

    if (result.success) {
      ElMessage.success(`${source.name} 连接成功 (延迟: ${result.latency}ms)`)
    } else {
      ElMessage.error(`${source.name} 连接失败: ${result.message}`)
    }
  } catch (error: any) {
    ElMessage.error('连接测试失败：' + error.message)
  } finally {
    testing.value = false
  }
}

onMounted(async () => {
  loading.value = true
  try {
    const response = await crawlerStore.fetchSupportedSources()
    sources.value = response.sources

    // Initialize connection status
    sources.value.forEach(source => {
      connectionStatus.value[source.source] = false
    })
  } catch (error: any) {
    ElMessage.error('加载数据源失败：' + error.message)
  } finally {
    loading.value = false
  }
})
</script>

<style scoped lang="scss">
.source-selector {
  display: flex;
  gap: 10px;
  width: 100%;

  .el-select {
    flex: 1;
  }
}

.source-option {
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
  height: 100%;

  .source-info {
    display: flex;
    flex-direction: column;
    gap: 4px;

    .source-name {
      font-weight: bold;
      color: #303133;
    }

    .source-description {
      font-size: 12px;
      color: #909399;
    }
  }

  .source-status {
    flex-shrink: 0;
  }
}
</style>
