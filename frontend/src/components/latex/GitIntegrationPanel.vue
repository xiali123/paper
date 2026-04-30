/**
 * Git集成面板 (深度优化版)
 * 支持版本历史、分支管理、提交、差异对比、冲突解决
 * 全新UI设计，现代化交互体验
 */

<template>
  <div class="git-integration-panel" v-loading="fetching && !fetchingSilent">
    <!-- Git头部 -->
    <div class="git-header">
      <div class="header-left">
        <el-icon class="git-icon" color="var(--el-color-primary)"><Connection /></el-icon>
        <span class="current-branch">
          <el-tag size="small" :type="isProtected ? 'danger' : 'primary'" effect="plain">
            {{ currentBranch }}
          </el-tag>
        </span>
        <el-tag v-if="hasChanges" size="small" type="warning" effect="plain">
          {{ totalChanges }} 个变更
        </el-tag>
      </div>
      <div class="header-actions">
        <el-button-group class="action-group">
          <el-tooltip content="拉取远程更新" placement="bottom">
            <el-button size="small" @click="fetchLatest" :loading="fetching">
              <el-icon><Refresh /></el-icon>
              <span class="btn-text">拉取</span>
            </el-button>
          </el-tooltip>
          <el-tooltip :content="stagedFiles.length > 0 ? `提交 ${stagedFiles.length} 个已暂存文件` : '暂存文件后提交'" placement="bottom">
            <el-button size="small" type="primary" @click="showCommitDialog = true">
              <el-icon><Upload /></el-icon>
              <span class="btn-text">提交</span>
            </el-button>
          </el-tooltip>
        </el-button-group>
        <el-dropdown trigger="click" @command="handleBranchAction">
          <el-button size="small">
            更多
            <el-icon class="el-icon--right"><ArrowDown /></el-icon>
          </el-button>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item command="new-branch" divided>
                <el-icon><Plus /></el-icon>
                <span>新建分支</span>
              </el-dropdown-item>
              <el-dropdown-item command="merge">
                <el-icon><Share /></el-icon>
                <span>合并分支</span>
              </el-dropdown-item>
              <el-dropdown-item command="switch">
                <el-icon><Sort /></el-icon>
                <span>切换分支</span>
              </el-dropdown-item>
              <el-dropdown-item command="stash" divided>
                <el-icon><FolderOpened /></el-icon>
                <span>暂存工作区</span>
              </el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
    </div>

    <!-- 状态标签页 -->
    <el-tabs v-model="activeTab" class="git-tabs">
      <!-- 变更标签 -->
      <el-tab-pane>
        <template #label>
          <span class="tab-label">
            <el-icon><Document /></el-icon>
            变更
            <el-badge v-if="totalChanges > 0" :value="totalChanges" :max="99" type="danger" />
          </span>
        </template>
        <div class="changes-content">
          <!-- 暂存区 -->
          <div v-if="stagedFiles.length > 0" class="file-section">
            <div class="section-header staged">
              <div class="section-title">
                <el-checkbox
                  v-model="allStagedChecked"
                  :indeterminate="stagedIndeterminate"
                  @change="toggleAllStaged"
                >
                  <span class="section-text">
                    <el-icon><Upload /></el-icon>
                    已暂存 ({{ stagedFiles.length }})
                  </span>
                </el-checkbox>
              </div>
              <div class="section-actions">
                <el-button size="small" text type="danger" @click="unstageAll">
                  <el-icon><Download /></el-icon>
                  全部取消
                </el-button>
              </div>
            </div>
            <div class="file-list">
              <div
                v-for="file in stagedFiles"
                :key="file.path"
                class="file-item"
                :class="[
                  `status-${file.status}`,
                  { 'is-selected': file.checked }
                ]"
                @click="file.checked = !file.checked"
              >
                <el-checkbox v-model="file.checked" @click.stop @change="updateStagedIndeterminate" />
                <div class="file-icon-wrapper">
                  <el-icon class="file-icon" :class="`icon-${file.status}`">
                    <Document v-if="file.status === 'modified'" />
                    <Plus v-else-if="file.status === 'added'" />
                    <Delete v-else />
                  </el-icon>
                </div>
                <div class="file-info">
                  <span class="file-name" :title="file.path">{{ truncatePath(file.path) }}</span>
                  <span class="file-full-path" v-show="showFullPath === file.path">{{ file.path }}</span>
                </div>
                <div class="file-status">
                  <el-tag v-if="file.status === 'added'" size="small" type="success" effect="plain">新增</el-tag>
                  <el-tag v-else-if="file.status === 'modified'" size="small" type="warning" effect="plain">修改</el-tag>
                  <el-tag v-else size="small" type="danger" effect="plain">删除</el-tag>
                </div>
                <div class="file-actions" @click.stop>
                  <el-button-group size="small">
                    <el-tooltip content="查看差异" placement="top">
                      <el-button @click="showDiff(file)">
                        <el-icon><View /></el-icon>
                      </el-button>
                    </el-tooltip>
                    <el-tooltip content="取消暂存" placement="top">
                      <el-button type="danger" @click="unstageFile(file)">
                        <el-icon><Download /></el-icon>
                      </el-button>
                    </el-tooltip>
                  </el-button-group>
                </div>
              </div>
            </div>
          </div>

          <!-- 未暂存 -->
          <div v-if="unstagedFiles.length > 0" class="file-section">
            <div class="section-header unstaged">
              <div class="section-title">
                <el-checkbox
                  v-model="allUnstagedChecked"
                  :indeterminate="unstagedIndeterminate"
                  @change="toggleAllUnstaged"
                >
                  <span class="section-text">
                    <el-icon><Edit /></el-icon>
                    未暂存 ({{ unstagedFiles.length }})
                  </span>
                </el-checkbox>
              </div>
              <div class="section-actions">
                <el-button size="small" text type="primary" @click="stageAll">
                  <el-icon><Upload /></el-icon>
                  全部暂存
                </el-button>
              </div>
            </div>
            <div class="file-list">
              <div
                v-for="file in unstagedFiles"
                :key="file.path"
                class="file-item"
                :class="[
                  `status-${file.status}`,
                  { 'is-selected': file.checked }
                ]"
                @click="file.checked = !file.checked"
              >
                <el-checkbox v-model="file.checked" @click.stop @change="updateUnstagedIndeterminate" />
                <div class="file-icon-wrapper">
                  <el-icon class="file-icon" :class="`icon-${file.status}`">
                    <Document v-if="file.status === 'modified'" />
                    <Plus v-else-if="file.status === 'added'" />
                    <Delete v-else />
                  </el-icon>
                </div>
                <div class="file-info">
                  <span class="file-name" :title="file.path">{{ truncatePath(file.path) }}</span>
                  <span class="file-full-path" v-show="showFullPath === file.path">{{ file.path }}</span>
                </div>
                <div class="file-status">
                  <el-tag v-if="file.status === 'added'" size="small" type="success" effect="plain">新增</el-tag>
                  <el-tag v-else-if="file.status === 'modified'" size="small" type="warning" effect="plain">修改</el-tag>
                  <el-tag v-else size="small" type="danger" effect="plain">删除</el-tag>
                </div>
                <div class="file-actions" @click.stop>
                  <el-button-group size="small">
                    <el-tooltip content="查看差异" placement="top">
                      <el-button @click="showDiff(file)">
                        <el-icon><View /></el-icon>
                      </el-button>
                    </el-tooltip>
                    <el-tooltip content="暂存" placement="top">
                      <el-button type="primary" @click="stageFile(file)">
                        <el-icon><Upload /></el-icon>
                      </el-button>
                    </el-tooltip>
                    <el-tooltip content="放弃修改" placement="top">
                      <el-button type="danger" @click="discardFile(file)">
                        <el-icon><Delete /></el-icon>
                      </el-button>
                    </el-tooltip>
                  </el-button-group>
                </div>
              </div>
            </div>
          </div>

          <!-- 空状态 -->
          <div v-if="stagedFiles.length === 0 && unstagedFiles.length === 0" class="empty-state">
            <div class="empty-illustration">
              <el-icon :size="80" color="var(--el-color-success)">
                <SuccessFilled />
              </el-icon>
            </div>
            <div class="empty-title">工作区干净</div>
            <div class="empty-desc">没有检测到任何变更</div>
            <el-button type="primary" plain @click="fetchLatest">
              <el-icon><Refresh /></el-icon>
              检查更新
            </el-button>
          </div>
        </div>
      </el-tab-pane>

      <!-- 历史标签 -->
      <el-tab-pane>
        <template #label>
          <span class="tab-label">
            <el-icon><View /></el-icon>
            历史
          </span>
        </template>
        <div class="history-content">
          <div class="history-filters">
            <el-input
              v-model="historySearch"
              size="large"
              placeholder="搜索提交消息、作者或哈希..."
              :prefix-icon="Search"
              clearable
              class="search-input"
            />
            <el-select v-model="historyBranch" size="large" class="branch-select" placeholder="筛选分支">
              <el-option label="所有分支" value="all" />
              <el-option
                v-for="branch in branches"
                :key="branch.name"
                :label="branch.name"
                :value="branch.name"
              />
            </el-select>
          </div>
          <div class="commit-list">
            <div
              v-for="commit in filteredCommits"
              :key="commit.hash"
              class="commit-item"
              @click="showCommitDetail(commit)"
            >
              <div class="commit-avatar">
                <el-avatar :size="40" :src="commit.authorAvatar">
                  {{ commit.author.charAt(0) }}
                </el-avatar>
              </div>
              <div class="commit-body">
                <div class="commit-header-row">
                  <span class="commit-author">{{ commit.author }}</span>
                  <span class="commit-time">{{ formatTime(commit.timestamp) }}</span>
                </div>
                <div class="commit-message">{{ commit.message }}</div>
                <div class="commit-footer">
                  <div class="commit-hash">
                    <el-icon><CopyDocument /></el-icon>
                    {{ commit.hash.substring(0, 8) }}
                  </div>
                  <div class="commit-tags">
                    <el-tag size="small" :type="getCommitTagType(commit)" effect="plain">
                      {{ commit.branch }}
                    </el-tag>
                    <el-tag v-if="commit.additions" size="small" type="success" effect="plain">
                      +{{ commit.additions }}
                    </el-tag>
                    <el-tag v-if="commit.deletions" size="small" type="danger" effect="plain">
                      -{{ commit.deletions }}
                    </el-tag>
                  </div>
                </div>
              </div>
            </div>
          </div>
          <div v-if="filteredCommits.length === 0" class="empty-state">
            <div class="empty-illustration">
              <el-icon :size="60" color="var(--el-text-color-placeholder)">
                <Search />
              </el-icon>
            </div>
            <div class="empty-title">没有找到匹配的提交</div>
            <div class="empty-desc">尝试调整搜索条件</div>
          </div>
        </div>
      </el-tab-pane>

      <!-- 分支标签 -->
      <el-tab-pane>
        <template #label>
          <span class="tab-label">
            <el-icon><Connection /></el-icon>
            分支
            <span class="branch-count">({{ branches.length }})</span>
          </span>
        </template>
        <div class="branches-content">
          <div class="branch-list">
            <div
              v-for="branch in branches"
              :key="branch.name"
              class="branch-card"
              :class="{ 'is-current': branch.name === currentBranch, 'is-protected': branch.protected }"
            >
              <div class="branch-icon">
                <el-icon :size="24">
                  <Connection />
                </el-icon>
              </div>
              <div class="branch-info">
                <div class="branch-name">{{ branch.name }}</div>
                <div class="branch-commit">{{ truncateText(branch.lastCommit, 35) }}</div>
              </div>
              <div class="branch-badges">
                <el-tag v-if="branch.protected" size="small" type="danger" effect="plain">
                  <el-icon><Lock /></el-icon>
                  受保护
                </el-tag>
                <el-tag v-if="branch.name === currentBranch" size="small" type="primary" effect="plain">
                  当前
                </el-tag>
              </div>
              <div class="branch-actions">
                <el-dropdown trigger="click" @command="(cmd) => handleBranchItemAction(cmd, branch)">
                  <el-button size="small" text>
                    <el-icon><MoreFilled /></el-icon>
                  </el-button>
                  <template #dropdown>
                    <el-dropdown-menu>
                      <el-dropdown-item command="checkout" :disabled="branch.name === currentBranch">
                        <el-icon><Sort /></el-icon>
                        切换到此分支
                      </el-dropdown-item>
                      <el-dropdown-item command="merge" :disabled="branch.name === currentBranch">
                        <el-icon><Share /></el-icon>
                        合并到当前分支
                      </el-dropdown-item>
                      <el-dropdown-item command="delete" :disabled="branch.protected || branch.name === currentBranch" divided>
                        <el-icon><Delete /></el-icon>
                        删除分支
                      </el-dropdown-item>
                    </el-dropdown-menu>
                  </template>
                </el-dropdown>
              </div>
            </div>
          </div>
          <div class="branch-footer">
            <el-button type="primary" class="new-branch-btn" @click="showNewBranchDialog = true">
              <el-icon><Plus /></el-icon>
              新建分支
            </el-button>
          </div>
        </div>
      </el-tab-pane>

      <!-- 冲突标签 -->
      <el-tab-pane>
        <template #label>
          <span class="tab-label">
            <el-icon><Warning /></el-icon>
            冲突
            <el-badge v-if="conflicts.length > 0" :value="conflicts.length" type="danger" />
          </span>
        </template>
        <div class="conflicts-content">
          <div v-if="conflicts.length > 0">
            <el-alert type="warning" :closable="false" show-icon class="conflict-alert">
              检测到 <strong>{{ conflicts.length }}</strong> 个合并冲突需要解决
            </el-alert>
            <div class="conflict-list">
              <div
                v-for="conflict in conflicts"
                :key="conflict.path"
                class="conflict-card"
              >
                <div class="conflict-icon">
                  <el-icon :size="32" color="var(--el-color-warning)">
                    <Warning />
                  </el-icon>
                </div>
                <div class="conflict-info">
                  <div class="conflict-path">{{ conflict.path }}</div>
                  <div class="conflict-branches">
                    <el-tag size="small" type="primary" effect="plain">{{ conflict.ours }}</el-tag>
                    <el-icon class="vs-icon"><Share /></el-icon>
                    <el-tag size="small" type="warning" effect="plain">{{ conflict.theirs }}</el-tag>
                  </div>
                </div>
                <div class="conflict-actions">
                  <el-button size="small" @click="resolveConflict(conflict, 'ours')">
                    <el-icon><Check /></el-icon>
                    使用当前
                  </el-button>
                  <el-button size="small" @click="resolveConflict(conflict, 'theirs')">
                    <el-icon><Check /></el-icon>
                    使用传入
                  </el-button>
                  <el-button size="small" type="primary" @click="openConflictEditor(conflict)">
                    <el-icon><Edit /></el-icon>
                    手动解决
                  </el-button>
                </div>
              </div>
            </div>
          </div>
          <div v-else class="empty-state">
            <div class="empty-illustration">
              <el-icon :size="80" color="var(--el-color-success)">
                <SuccessFilled />
              </el-icon>
            </div>
            <div class="empty-title">没有冲突</div>
            <div class="empty-desc">所有合并冲突已解决</div>
          </div>
        </div>
      </el-tab-pane>
    </el-tabs>

    <!-- 提交对话框 (全新设计) -->
    <el-dialog
      v-model="showCommitDialog"
      title=""
      width="700px"
      class="commit-dialog"
      @close="resetCommitForm"
      :close-on-click-modal="false"
    >
      <div class="commit-dialog-content">
        <!-- 头部 -->
        <div class="commit-dialog-header">
          <el-icon :size="32" color="var(--el-color-primary)"><Upload /></el-icon>
          <div>
            <div class="commit-dialog-title">提交变更</div>
            <div class="commit-dialog-subtitle">
              {{ stagedFiles.length }} 个文件已暂存，准备提交
            </div>
          </div>
        </div>

        <!-- 提交类型选择 -->
        <div class="commit-type-section">
          <div class="section-label">选择提交类型</div>
          <div class="commit-type-grid">
            <div
              v-for="type in commitTypes"
              :key="type.value"
              class="commit-type-card"
              :class="{ 'is-active': commitForm.type === type.value }"
              @click="commitForm.type = type.value"
            >
              <div class="type-icon" :style="{ background: type.color }">
                <el-icon :size="20">
                  <component :is="type.icon" />
                </el-icon>
              </div>
              <div class="type-info">
                <div class="type-name">{{ type.label }}</div>
                <div class="type-desc">{{ type.desc }}</div>
              </div>
              <div v-if="commitForm.type === type.value" class="type-check">
                <el-icon><Check /></el-icon>
              </div>
            </div>
          </div>
        </div>

        <!-- 提交内容 -->
        <div class="commit-form-section">
          <div class="form-row">
            <div class="form-item form-item-scope">
              <label>影响范围</label>
              <el-input
                v-model="commitForm.scope"
                placeholder="例如: latex-editor (可选)"
                size="large"
              />
            </div>
            <div class="form-item form-item-issue">
              <label>关联 Issue</label>
              <el-input
                v-model="commitForm.issue"
                placeholder="#123"
                size="large"
              />
            </div>
          </div>

          <div class="form-item">
            <label>提交消息</label>
            <el-input
              v-model="commitForm.message"
              type="textarea"
              :rows="4"
              placeholder="简要描述本次变更的内容..."
              maxlength="500"
              show-word-limit
              size="large"
              @input="updateCommitPreview"
            />
          </div>
        </div>

        <!-- 预览 -->
        <div class="commit-preview-section">
          <div class="preview-header">
            <el-icon><View /></el-icon>
            预览
          </div>
          <div class="preview-content">
            <pre>{{ commitPreview }}</pre>
          </div>
        </div>

        <!-- 附加选项 -->
        <div class="commit-options">
          <el-checkbox v-model="commitForm.signed">
            <el-icon><Lock /></el-icon>
            签名提交 (GPG)
          </el-checkbox>
          <el-checkbox v-model="commitForm.amend">
            <el-icon><Edit /></el-icon>
            修改上次提交
          </el-checkbox>
        </div>
      </div>

      <template #footer>
        <div class="commit-dialog-footer">
          <el-button size="large" @click="showCommitDialog = false">取消</el-button>
          <el-button
            size="large"
            type="primary"
            :disabled="!commitForm.message.trim()"
            @click="submitCommit"
          >
            <el-icon><Upload /></el-icon>
            提交变更
          </el-button>
        </div>
      </template>
    </el-dialog>

    <!-- 新建分支对话框 (全新设计) -->
    <el-dialog
      v-model="showNewBranchDialog"
      title=""
      width="550px"
      class="branch-dialog"
      @close="resetBranchForm"
    >
      <div class="branch-dialog-content">
        <!-- 头部 -->
        <div class="branch-dialog-header">
          <el-icon :size="32" color="var(--el-color-success)"><Connection /></el-icon>
          <div>
            <div class="branch-dialog-title">新建分支</div>
            <div class="branch-dialog-subtitle">创建新分支开始开发</div>
          </div>
        </div>

        <!-- 分支命名 -->
        <div class="branch-name-section">
          <div class="section-label">分支名称</div>
          <div class="branch-namer">
            <el-select v-model="branchPrefix" size="large" class="branch-prefix">
              <el-option label="feature/" value="feature/" />
              <el-option label="bugfix/" value="bugfix/" />
              <el-option label="hotfix/" value="hotfix/" />
              <el-option label="release/" value="release/" />
              <el-option label="无前缀" value="" />
            </el-select>
            <el-input
              v-model="newBranchForm.name"
              placeholder="branch-name"
              size="large"
              class="branch-input"
              @input="validateBranchName"
            />
          </div>
          <div v-if="newBranchForm.name || branchPrefix" class="branch-preview">
            <span class="preview-label">完整分支名:</span>
            <code class="preview-name">{{ branchPrefix }}{{ newBranchForm.name }}</code>
          </div>
          <div v-if="branchNameError" class="branch-error">
            <el-icon><Warning /></el-icon>
            {{ branchNameError }}
          </div>
          <div v-else-if="newBranchForm.name && !branchNameError" class="branch-success">
            <el-icon><Check /></el-icon>
            分支名可用
          </div>
        </div>

        <!-- 基于分支 -->
        <div class="branch-base-section">
          <div class="section-label">基于分支</div>
          <el-select v-model="newBranchForm.from" size="large" class="branch-base-select">
            <el-option
              v-for="branch in branches"
              :key="branch.name"
              :label="branch.name"
              :value="branch.name"
            >
              <div class="branch-option">
                <span class="option-name">{{ branch.name }}</span>
                <span class="option-commit">{{ truncateText(branch.lastCommit, 30) }}</span>
              </div>
            </el-option>
          </el-select>
        </div>

        <!-- 命名建议 -->
        <div class="branch-tips">
          <div class="tip-title">
            <el-icon><InfoFilled /></el-icon>
            分支命名建议
          </div>
          <div class="tip-list">
            <div class="tip-item">
              <el-tag size="small" type="success">feature/</el-tag>
              新功能开发
            </div>
            <div class="tip-item">
              <el-tag size="small" type="warning">bugfix/</el-tag>
              Bug 修复
            </div>
            <div class="tip-item">
              <el-tag size="small" type="danger">hotfix/</el-tag>
              紧急修复
            </div>
            <div class="tip-item">
              <el-tag size="small" type="info">release/</el-tag>
              发布准备
            </div>
          </div>
        </div>
      </div>

      <template #footer>
        <div class="branch-dialog-footer">
          <el-button size="large" @click="showNewBranchDialog = false">取消</el-button>
          <el-button
            size="large"
            type="primary"
            :disabled="!newBranchForm.name.trim() || !!branchNameError"
            @click="createBranch"
          >
            <el-icon><Plus /></el-icon>
            创建分支
          </el-button>
        </div>
      </template>
    </el-dialog>

    <!-- 差异查看器对话框 (全新设计) -->
    <el-dialog
      v-model="showDiffDialog"
      title=""
      width="92%"
      top="2vh"
      class="diff-dialog"
      append-to-body
    >
      <div v-if="currentDiff" class="diff-viewer-compact">
        <!-- 工具栏 -->
        <div class="diff-toolbar">
          <div class="diff-file-info">
            <el-icon :size="18"><Document /></el-icon>
            <span class="diff-path">{{ currentDiff.path }}</span>
          </div>
          <div class="diff-stats">
            <el-tag type="success" effect="plain" size="small">
              <el-icon><Plus /></el-icon>
              {{ currentDiff.additions }}
            </el-tag>
            <el-tag type="danger" effect="plain" size="small">
              <el-icon><Delete /></el-icon>
              {{ currentDiff.deletions }}
            </el-tag>
            <el-button text size="small" @click="copyDiff">
              <el-icon><CopyDocument /></el-icon>
              复制
            </el-button>
          </div>
        </div>

        <!-- 差异内容 -->
        <div class="diff-body">
          <div class="diff-code">
            <div
              v-for="(line, index) in currentDiff.lines"
              :key="index"
              class="diff-row"
              :class="`diff-${line.type}`"
            >
              <span class="diff-line-num">{{ line.number }}</span>
              <span class="diff-prefix">{{ getLinePrefix(line.type) }}</span>
              <span class="diff-code-line">{{ line.content }}</span>
            </div>
          </div>
        </div>
      </div>
    </el-dialog>

    <!-- 冲突编辑器对话框 (全新设计) -->
    <el-dialog
      v-model="showConflictDialog"
      title=""
      width="96%"
      top="1vh"
      class="conflict-dialog"
      append-to-body
    >
      <div v-if="currentConflict" class="conflict-resolver">
        <!-- 头部 -->
        <div class="conflict-resolver-header">
          <el-icon :size="28" color="var(--el-color-warning)"><Warning /></el-icon>
          <div class="header-info">
            <div class="header-title">解决合并冲突</div>
            <div class="header-file">{{ currentConflict.path }}</div>
          </div>
          <div class="header-actions">
            <el-button-group>
              <el-button @click="useAllOurs" size="small">
                <el-icon><Check /></el-icon>
                使用当前
              </el-button>
              <el-button @click="useAllTheirs" size="small">
                <el-icon><Check /></el-icon>
                使用传入
              </el-button>
              <el-button type="primary" @click="editManually" size="small">
                <el-icon><Edit /></el-icon>
                手动编辑
              </el-button>
            </el-button-group>
          </div>
        </div>

        <!-- 三面板对比 -->
        <div class="conflict-panels" :class="{ 'edit-mode': isEditingManually }">
          <!-- 当前版本 -->
          <div class="conflict-panel ours-panel" :class="{ 'hidden': isEditingManually }">
            <div class="panel-header">
              <el-icon><Connection /></el-icon>
              当前版本
              <el-tag size="small" type="primary" effect="plain">{{ currentConflict.ours }}</el-tag>
            </div>
            <div class="panel-body">
              <pre class="panel-code">{{ currentConflict.oursContent }}</pre>
            </div>
          </div>

          <!-- 传入版本 -->
          <div class="conflict-panel theirs-panel" :class="{ 'hidden': isEditingManually }">
            <div class="panel-header">
              <el-icon><Share /></el-icon>
              传入版本
              <el-tag size="small" type="warning" effect="plain">{{ currentConflict.theirs }}</el-tag>
            </div>
            <div class="panel-body">
              <pre class="panel-code">{{ currentConflict.theirsContent }}</pre>
            </div>
          </div>

          <!-- 合并结果/编辑区 -->
          <div class="conflict-panel merge-panel">
            <div class="panel-header merge-header">
              <el-icon><EditPen /></el-icon>
              {{ isEditingManually ? '手动编辑' : '合并结果' }}
            </div>
            <div class="panel-body">
              <textarea
                v-if="isEditingManually"
                v-model="mergedContent"
                class="merge-editor"
                spellcheck="false"
                placeholder="在此编辑合并后的内容..."
              />
              <pre v-else class="panel-code">{{ mergedContent }}</pre>
            </div>
          </div>
        </div>
      </div>

      <template #footer>
        <div class="conflict-dialog-footer">
          <el-button size="large" @click="showConflictDialog = false">取消</el-button>
          <el-button size="large" type="success" :disabled="!mergedContent" @click="acceptMerge">
            <el-icon><Check /></el-icon>
            应用合并结果
          </el-button>
        </div>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import {
  Connection,
  Refresh,
  Upload,
  Download,
  More,
  MoreFilled,
  Plus,
  Share,
  Sort,
  Document,
  Delete,
  View,
  Search,
  Warning,
  ArrowDown,
  Lock,
  Check,
  Edit,
  EditPen,
  CopyDocument,
  SuccessFilled,
  FolderOpened,
  InfoFilled,
  Star,
  Tools,
  Setting
} from '@element-plus/icons-vue'

// 图标别名
const SetUp = Setting
const DocumentCopy = CopyDocument
import { ElMessage, ElMessageBox } from 'element-plus'
import type { FormInstance, FormRules } from 'element-plus'

interface GitFile {
  path: string
  status: 'added' | 'modified' | 'deleted'
  checked: boolean
}

interface Commit {
  hash: string
  message: string
  author: string
  authorAvatar?: string
  timestamp: number
  branch: string
  additions: number
  deletions: number
}

interface Branch {
  name: string
  protected: boolean
  lastCommit: string
}

interface Conflict {
  path: string
  ours: string
  theirs: string
  oursContent: string
  theirsContent: string
  baseContent: string
}

interface Diff {
  path: string
  status: string
  additions: number
  deletions: number
  lines: Array<{
    type: 'added' | 'removed' | 'context'
    number: number
    content: string
  }>
}

// 提交类型配置
const commitTypes = [
  { value: 'feat', label: 'feat', desc: '新功能', color: 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)', icon: Star },
  { value: 'fix', label: 'fix', desc: 'Bug 修复', color: 'linear-gradient(135deg, #f093fb 0%, #f5576c 100%)', icon: Tools },
  { value: 'docs', label: 'docs', desc: '文档变更', color: 'linear-gradient(135deg, #4facfe 0%, #00f2fe 100%)', icon: DocumentCopy },
  { value: 'style', label: 'style', desc: '代码格式', color: 'linear-gradient(135deg, #43e97b 0%, #38f9d7 100%)', icon: SetUp },
  { value: 'refactor', label: 'refactor', desc: '重构', color: 'linear-gradient(135deg, #fa709a 0%, #fee140 100%)', icon: Edit },
  { value: 'perf', label: 'perf', desc: '性能优化', color: 'linear-gradient(135deg, #30cfd0 0%, #330867 100%)', icon: Sort },
  { value: 'test', label: 'test', desc: '测试', color: 'linear-gradient(135deg, #a8edea 0%, #fed6e3 100%)', icon: Check },
  { value: 'chore', label: 'chore', desc: '构建/工具', color: 'linear-gradient(135deg, #ff9a9e 0%, #fecfef 100%)', icon: Tools }
]

// 状态
const activeTab = ref('changes')
const currentBranch = ref('main')
const isProtected = ref(false)
const fetching = ref(false)
const fetchingSilent = ref(false)
const showFullPath = ref('')

// 文件变更
const stagedFiles = ref<GitFile[]>([])
const unstagedFiles = ref<GitFile[]>([])
const allStagedChecked = ref(false)
const allUnstagedChecked = ref(false)
const stagedIndeterminate = ref(false)
const unstagedIndeterminate = ref(false)

// 计算属性
const hasChanges = computed(() => stagedFiles.value.length > 0 || unstagedFiles.value.length > 0)
const totalChanges = computed(() => stagedFiles.value.length + unstagedFiles.value.length)

// 提交
const showCommitDialog = ref(false)
const commitForm = ref({
  message: '',
  type: 'feat',
  scope: '',
  issue: '',
  signed: false,
  amend: false
})
const commitPreview = ref('')

// 分支
const showNewBranchDialog = ref(false)
const branchFormRef = ref<FormInstance>()
const branchPrefix = ref('feature/')
const branchNameError = ref('')
const newBranchForm = ref({
  name: '',
  from: 'main'
})
const branchRules: FormRules = {
  name: [
    { required: true, message: '请输入分支名称', trigger: 'blur' }
  ]
}

const branches = ref<Branch[]>([
  { name: 'main', protected: true, lastCommit: 'Update README documentation' },
  { name: 'develop', protected: false, lastCommit: 'Add new feature: dark mode' },
  { name: 'feature/latex-editor', protected: false, lastCommit: 'WIP: Improve editor performance' }
])

// 历史
const historySearch = ref('')
const historyBranch = ref('all')
const commits = ref<Commit[]>([])
const filteredCommits = computed(() => {
  let result = commits.value

  if (historySearch.value) {
    const search = historySearch.value.toLowerCase()
    result = result.filter(c =>
      c.message.toLowerCase().includes(search) ||
      c.author.toLowerCase().includes(search) ||
      c.hash.toLowerCase().includes(search)
    )
  }

  if (historyBranch.value !== 'all') {
    result = result.filter(c => c.branch === historyBranch.value)
  }

  return result
})

// 冲突
const conflicts = ref<Conflict[]>([])
const showConflictDialog = ref(false)
const currentConflict = ref<Conflict | null>(null)
const mergedContent = ref('')
const isEditingManually = ref(false)

// 差异
const showDiffDialog = ref(false)
const currentDiff = ref<Diff | null>(null)

// 初始化模拟数据
commits.value = [
  {
    hash: 'a1b2c3d4e5f6',
    message: 'feat(latex-editor): 添加LaTeX编辑器自动保存功能',
    author: '张三',
    timestamp: Date.now() - 3600000,
    branch: 'main',
    additions: 150,
    deletions: 20
  },
  {
    hash: 'b2c3d4e5f6g7',
    message: 'fix(preview): 修复PDF预览中文显示问题',
    author: '李四',
    timestamp: Date.now() - 7200000,
    branch: 'develop',
    additions: 45,
    deletions: 10
  },
  {
    hash: 'c3d4e5f6g7h8',
    message: 'docs: 更新README文档',
    author: '王五',
    timestamp: Date.now() - 86400000,
    branch: 'main',
    additions: 80,
    deletions: 30
  },
  {
    hash: 'd4e5f6g7h8i9',
    message: 'refactor(auth): 重构认证模块',
    author: '赵六',
    timestamp: Date.now() - 172800000,
    branch: 'feature/auth-refactor',
    additions: 200,
    deletions: 150
  }
]

stagedFiles.value = [
  { path: 'src/components/latex/LatexEditor.vue', status: 'modified', checked: true },
  { path: 'src/utils/bibtexParser.ts', status: 'added', checked: true },
  { path: 'src/styles/latex.scss', status: 'modified', checked: false }
]

unstagedFiles.value = [
  { path: 'README.md', status: 'modified', checked: false },
  { path: 'src/assets/logo.png', status: 'deleted', checked: false },
  { path: 'frontend/package.json', status: 'modified', checked: true }
]

// 工具函数
const truncatePath = (path: string, maxLength = 35): string => {
  if (path.length <= maxLength) return path
  const parts = path.split('/')
  if (parts.length <= 2) return path.substring(0, maxLength) + '...'
  return '.../' + parts.slice(-2).join('/')
}

const truncateText = (text: string, maxLength: number): string => {
  if (text.length <= maxLength) return text
  return text.substring(0, maxLength) + '...'
}

const getLinePrefix = (type: string): string => {
  switch (type) {
    case 'added': return '+ '
    case 'removed': return '- '
    default: return '  '
  }
}

// 提交预览
const updateCommitPreview = () => {
  let preview = commitForm.value.type
  if (commitForm.value.scope) {
    preview += `(${commitForm.value.scope})`
  }
  preview += `: ${commitForm.value.message || '提交消息'}`
  if (commitForm.value.issue) {
    preview += `\n\nCloses ${commitForm.value.issue}`
  }
  commitPreview.value = preview
}

// 监听提交表单变化
watch([
  () => commitForm.value.type,
  () => commitForm.value.scope,
  () => commitForm.value.issue
], updateCommitPreview)

// 分支名称验证
const validateBranchName = () => {
  const name = newBranchForm.value.name.trim()
  if (!name) {
    branchNameError.value = ''
    return
  }

  const fullName = branchPrefix.value + name
  if (branches.value.some(b => b.name === fullName)) {
    branchNameError.value = '分支已存在'
    return
  }

  const invalidChars = /[^\w\-]/
  if (invalidChars.test(name)) {
    branchNameError.value = '只能包含字母、数字、连字符和下划线'
    return
  }

  if (name.startsWith('-') || name.endsWith('-')) {
    branchNameError.value = '不能以连字符开头或结尾'
    return
  }

  branchNameError.value = ''
}

// 文件操作
const stageFile = (file: GitFile) => {
  const index = unstagedFiles.value.findIndex(f => f.path === file.path)
  if (index >= 0) {
    unstagedFiles.value.splice(index, 1)
    stagedFiles.value.push({ ...file, checked: true })
  }
  updateUnstagedIndeterminate()
  updateStagedIndeterminate()
  ElMessage.success(`已暂存: ${truncatePath(file.path, 30)}`)
}

const unstageFile = (file: GitFile) => {
  const index = stagedFiles.value.findIndex(f => f.path === file.path)
  if (index >= 0) {
    stagedFiles.value.splice(index, 1)
    unstagedFiles.value.push({ ...file, checked: false })
  }
  updateStagedIndeterminate()
  updateUnstagedIndeterminate()
  ElMessage.success(`已取消暂存: ${truncatePath(file.path, 30)}`)
}

const stageAll = () => {
  const count = unstagedFiles.value.length
  unstagedFiles.value.forEach(file => {
    stagedFiles.value.push({ ...file, checked: true })
  })
  unstagedFiles.value = []
  allUnstagedChecked.value = false
  unstagedIndeterminate.value = false
  ElMessage.success(`已暂存 ${count} 个文件`)
}

const unstageAll = () => {
  const count = stagedFiles.value.length
  stagedFiles.value.forEach(file => {
    unstagedFiles.value.push({ ...file, checked: false })
  })
  stagedFiles.value = []
  allStagedChecked.value = false
  stagedIndeterminate.value = false
  ElMessage.success(`已取消暂存 ${count} 个文件`)
}

const discardFile = async (file: GitFile) => {
  try {
    await ElMessageBox.confirm(
      `确定要放弃对 "${file.path}" 的所有修改吗？此操作不可恢复。`,
      '确认放弃修改',
      {
        type: 'warning',
        confirmButtonText: '确定放弃',
        cancelButtonText: '取消',
        distinguishCancelAndClose: true
      }
    )

    const index = unstagedFiles.value.findIndex(f => f.path === file.path)
    if (index >= 0) {
      unstagedFiles.value.splice(index, 1)
    }

    ElMessage.success('已放弃修改')
  } catch {
    // 用户取消
  }
}

const toggleAllStaged = () => {
  stagedFiles.value.forEach(file => {
    file.checked = allStagedChecked.value
  })
  stagedIndeterminate.value = false
}

const toggleAllUnstaged = () => {
  unstagedFiles.value.forEach(file => {
    file.checked = allUnstagedChecked.value
  })
  unstagedIndeterminate.value = false
}

const updateStagedIndeterminate = () => {
  const checkedCount = stagedFiles.value.filter(f => f.checked).length
  allStagedChecked.value = checkedCount === stagedFiles.value.length && checkedCount > 0
  stagedIndeterminate.value = checkedCount > 0 && checkedCount < stagedFiles.value.length
}

const updateUnstagedIndeterminate = () => {
  const checkedCount = unstagedFiles.value.filter(f => f.checked).length
  allUnstagedChecked.value = checkedCount === unstagedFiles.value.length && checkedCount > 0
  unstagedIndeterminate.value = checkedCount > 0 && checkedCount < unstagedFiles.value.length
}

// Git操作
const fetchLatest = async () => {
  fetching.value = true
  try {
    await new Promise(resolve => setTimeout(resolve, 1000))
    ElMessage.success('已拉取最新代码')
  } finally {
    fetching.value = false
  }
}

const resetCommitForm = () => {
  commitForm.value = {
    message: '',
    type: 'feat',
    scope: '',
    issue: '',
    signed: false,
    amend: false
  }
  commitPreview.value = ''
}

const resetBranchForm = () => {
  newBranchForm.value = { name: '', from: 'main' }
  branchPrefix.value = 'feature/'
  branchNameError.value = ''
}

const submitCommit = () => {
  const message = commitForm.value.message.trim()
  if (!message) return

  if (stagedFiles.value.length === 0) {
    ElMessage.warning('没有已暂存的文件，请先暂存要提交的文件')
    return
  }

  let fullMessage = `${commitForm.value.type}`
  if (commitForm.value.scope) {
    fullMessage += `(${commitForm.value.scope})`
  }
  fullMessage += `: ${message}`
  if (commitForm.value.issue) {
    fullMessage += `\n\nCloses ${commitForm.value.issue}`
  }

  commits.value.unshift({
    hash: Math.random().toString(36).substring(2, 15),
    message: fullMessage,
    author: '当前用户',
    timestamp: Date.now(),
    branch: currentBranch.value,
    additions: stagedFiles.value.reduce((sum, f) => sum + (f.status === 'added' ? 50 : 20), 0),
    deletions: stagedFiles.value.reduce((sum, f) => sum + (f.status === 'deleted' ? 50 : 10), 0)
  })

  stagedFiles.value = []
  showCommitDialog.value = false
  resetCommitForm()

  ElMessage.success({
    message: `提交成功！\n${fullMessage.split('\n')[0]}`,
    duration: 3000
  })
}

const createBranch = () => {
  const name = newBranchForm.value.name.trim()
  if (!name || branchNameError.value) return

  const fullName = branchPrefix.value + name

  branches.value.push({
    name: fullName,
    protected: false,
    lastCommit: 'Branch created'
  })

  showNewBranchDialog.value = false
  resetBranchForm()

  ElMessage.success(`分支 "${fullName}" 已创建`)
}

const handleBranchAction = (command: string) => {
  switch (command) {
    case 'new-branch':
      showNewBranchDialog.value = true
      break
    case 'merge':
      ElMessage.info('请在分支列表中选择要合并的分支')
      activeTab.value = 'branches'
      break
    case 'switch':
      ElMessage.info('请在分支列表中选择要切换的分支')
      activeTab.value = 'branches'
      break
    case 'stash':
      ElMessage.info('工作区已暂存')
      break
  }
}

const handleBranchItemAction = (command: string, branch: Branch) => {
  switch (command) {
    case 'checkout':
      if (hasChanges.value) {
        ElMessageBox.confirm(
          '切换分支前，当前分支有未提交的变更。建议先提交或暂存这些变更。',
          '切换分支确认',
          {
            type: 'warning',
            confirmButtonText: '强制切换',
            cancelButtonText: '取消',
            distinguishCancelAndClose: true
          }
        ).then(() => {
          currentBranch.value = branch.name
          isProtected.value = branch.protected
          ElMessage.success(`已切换到分支 "${branch.name}"`)
        }).catch(() => {})
      } else {
        currentBranch.value = branch.name
        isProtected.value = branch.protected
        ElMessage.success(`已切换到分支 "${branch.name}"`)
      }
      break
    case 'merge':
      ElMessage.info(`将 "${branch.name}" 合并到当前分支`)
      break
    case 'delete':
      ElMessageBox.confirm(
        `确定要删除分支 "${branch.name}" 吗？此操作不可恢复。`,
        '确认删除分支',
        {
          type: 'warning',
          confirmButtonText: '删除',
          cancelButtonText: '取消'
        }
      ).then(() => {
        const index = branches.value.findIndex(b => b.name === branch.name)
        if (index >= 0) {
          branches.value.splice(index, 1)
        }
        ElMessage.success('分支已删除')
      }).catch(() => {})
      break
  }
}

// 差异查看
const showDiff = (file: GitFile) => {
  currentDiff.value = {
    path: file.path,
    status: file.status,
    additions: 15,
    deletions: 8,
    lines: [
      { type: 'context', number: 1, content: '\\documentclass{article}' },
      { type: 'context', number: 2, content: '\\usepackage{amsmath}' },
      { type: 'removed', number: 3, content: '\\title{Old Title}' },
      { type: 'added', number: 3, content: '\\title{New Title}' },
      { type: 'context', number: 4, content: '\\begin{document}' },
      { type: 'added', number: 5, content: '% New comment added' },
      { type: 'context', number: 5, content: '\\maketitle' }
    ]
  }
  showDiffDialog.value = true
}

const copyDiff = () => {
  if (!currentDiff.value) return
  const content = currentDiff.value.lines.map(l => `${l.type === 'added' ? '+' : l.type === 'removed' ? '-' : ' '}${l.content}`).join('\n')
  navigator.clipboard.writeText(content)
  ElMessage.success('差异内容已复制到剪贴板')
}

// 冲突解决
const openConflictEditor = (conflict: Conflict) => {
  currentConflict.value = conflict
  mergedContent.value = conflict.oursContent
  isEditingManually.value = false
  showConflictDialog.value = true
}

const useAllOurs = () => {
  if (!currentConflict.value) return
  mergedContent.value = currentConflict.value.oursContent
  ElMessage.info('已选择当前版本')
}

const useAllTheirs = () => {
  if (!currentConflict.value) return
  mergedContent.value = currentConflict.value.theirsContent
  ElMessage.info('已选择传入版本')
}

const editManually = () => {
  isEditingManually.value = true
}

const resolveConflict = (conflict: Conflict, side: 'ours' | 'theirs') => {
  const index = conflicts.value.findIndex(c => c.path === conflict.path)
  if (index >= 0) {
    conflicts.value.splice(index, 1)
  }

  ElMessage.success(`已选择${side === 'ours' ? '当前' : '传入'}版本`)
}

const acceptMerge = () => {
  if (!currentConflict.value) return

  const index = conflicts.value.findIndex(c => c.path === currentConflict.value!.path)
  if (index >= 0) {
    conflicts.value.splice(index, 1)
  }

  showConflictDialog.value = false
  isEditingManually.value = false
  ElMessage.success('冲突已解决，合并结果已应用')
}

// 提交详情
const showCommitDetail = (commit: Commit) => {
  ElMessageBox.alert(`
    <div style="text-align: left; line-height: 1.8;">
      <div style="margin-bottom: 12px;">
        <strong style="color: var(--el-color-primary);">提交哈希:</strong><br>
        <code style="background: var(--el-fill-color-light); padding: 4px 8px; border-radius: 4px;">${commit.hash}</code>
      </div>
      <div style="margin-bottom: 12px;">
        <strong>作者:</strong> ${commit.author}<br>
        <strong>时间:</strong> ${formatTime(commit.timestamp)}
      </div>
      <div style="margin-bottom: 12px;">
        <strong>消息:</strong><br>
        <span style="color: var(--el-text-color-regular);">${commit.message}</span>
      </div>
      <div>
        <strong>变更:</strong>
        <el-tag size="small" type="success">+${commit.additions}</el-tag>
        <el-tag size="small" type="danger">-${commit.deletions}</el-tag>
      </div>
    </div>
  `, '提交详情', {
    dangerouslyUseHTMLString: true,
    confirmButtonText: '关闭',
    customClass: 'commit-detail-dialog'
  })
}

// 时间格式化
const formatTime = (timestamp: number): string => {
  const date = new Date(timestamp)
  const now = new Date()
  const diff = now.getTime() - date.getTime()
  const minutes = Math.floor(diff / 60000)

  if (minutes < 1) return '刚刚'
  if (minutes < 60) return `${minutes}分钟前`
  if (minutes < 1440) return `${Math.floor(minutes / 60)}小时前`
  if (minutes < 43200) return `${Math.floor(minutes / 1440)}天前`
  return date.toLocaleDateString('zh-CN', { year: 'numeric', month: 'short', day: 'numeric' })
}

const getCommitTagType = (commit: Commit) => {
  if (commit.branch === 'main') return 'danger'
  if (commit.branch === 'develop') return 'warning'
  return 'info'
}

// 初始化预览
updateCommitPreview()
</script>

<style scoped lang="scss">
// 基础样式
.git-integration-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: var(--el-bg-color-page);
}

// 头部
.git-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 14px 18px;
  background: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color-light);

  .header-left {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .git-icon {
    font-size: 22px;
  }

  .action-group {
    @media (max-width: 768px) {
      .btn-text {
        display: none;
      }
    }
  }
}

// 标签页
.git-tabs {
  flex: 1;
  overflow: hidden;

  :deep(.el-tabs__header) {
    margin: 0;
    padding: 0 16px;
    background: var(--el-bg-color);
    border-bottom: 1px solid var(--el-border-color-light);
  }

  :deep(.el-tabs__content) {
    height: calc(100% - 48px);
    overflow: hidden;
  }

  :deep(.el-tab-pane) {
    height: 100%;
    overflow-y: auto;
  }

  .tab-label {
    display: flex;
    align-items: center;
    gap: 6px;

    .branch-count {
      opacity: 0.7;
    }
  }
}

// 变更内容
.changes-content {
  padding: 16px;
}

.file-section {
  margin-bottom: 20px;

  .section-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 16px;
    border-radius: 10px 10px 0 0;
    font-size: 13px;
    font-weight: 500;
    border: 1px solid var(--el-border-color-lighter);
    border-bottom: none;

    &.staged {
      background: linear-gradient(135deg, var(--el-color-success-light-9) 0%, var(--el-color-success-light-8) 100%);
      border-color: var(--el-color-success-light-6);
      color: var(--el-color-success-dark-2);
    }

    &.unstaged {
      background: var(--el-fill-color-light);
      color: var(--el-text-color-regular);
    }

    .section-text {
      display: flex;
      align-items: center;
      gap: 6px;
    }
  }
}

.file-list {
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 0 0 10px 10px;
  overflow: hidden;
  background: var(--el-bg-color);
}

.file-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px 16px;
  border-bottom: 1px solid var(--el-border-color-lighter);
  transition: all 0.2s;
  cursor: pointer;

  &:last-child {
    border-bottom: none;
  }

  &:hover {
    background: var(--el-fill-color-light);
  }

  &.is-selected {
    background: var(--el-color-primary-light-9);
    border-left: 3px solid var(--el-color-primary);
    padding-left: 13px;
  }

  .file-icon-wrapper {
    display: flex;
    align-items: center;
    justify-content: center;
    width: 32px;
    height: 32px;
    border-radius: 6px;
    background: var(--el-fill-color-light);

    .file-icon {
      font-size: 18px;

      &.icon-added { color: var(--el-color-success); }
      &.icon-modified { color: var(--el-color-warning); }
      &.icon-deleted { color: var(--el-color-danger); }
    }
  }

  .file-info {
    flex: 1;
    min-width: 0;

    .file-name {
      display: block;
      font-size: 13px;
      font-family: monospace;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }

    .file-full-path {
      display: block;
      font-size: 11px;
      color: var(--el-text-color-secondary);
      font-family: monospace;
      margin-top: 2px;
    }
  }
}

// 空状态
.empty-state {
  padding: 60px 20px;
  text-align: center;

  .empty-illustration {
    margin-bottom: 16px;
  }

  .empty-title {
    font-size: 16px;
    font-weight: 500;
    color: var(--el-text-color-primary);
    margin-bottom: 8px;
  }

  .empty-desc {
    font-size: 13px;
    color: var(--el-text-color-secondary);
    margin-bottom: 20px;
  }
}

// 历史内容
.history-content {
  padding: 16px;
}

.history-filters {
  display: flex;
  gap: 12px;
  margin-bottom: 20px;

  .search-input {
    flex: 1;
  }

  .branch-select {
    width: 160px;
  }
}

.commit-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.commit-item {
  display: flex;
  gap: 14px;
  padding: 16px;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 12px;
  cursor: pointer;
  transition: all 0.2s;

  &:hover {
    border-color: var(--el-color-primary);
    box-shadow: 0 6px 16px rgba(64, 158, 255, 0.12);
    transform: translateY(-2px);
  }

  .commit-avatar {
    flex-shrink: 0;
  }

  .commit-body {
    flex: 1;
    min-width: 0;
  }

  .commit-header-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 8px;
  }

  .commit-author {
    font-weight: 500;
    font-size: 14px;
    color: var(--el-text-color-primary);
  }

  .commit-time {
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }

  .commit-message {
    font-size: 13px;
    line-height: 1.6;
    color: var(--el-text-color-regular);
    margin-bottom: 10px;
  }

  .commit-footer {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .commit-hash {
    display: flex;
    align-items: center;
    gap: 4px;
    font-size: 12px;
    color: var(--el-text-color-secondary);
    font-family: monospace;
  }

  .commit-tags {
    display: flex;
    gap: 4px;
  }
}

// 分支内容
.branches-content {
  padding: 16px;
}

.branch-list {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 14px;
  margin-bottom: 16px;
}

.branch-card {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 16px;
  background: var(--el-bg-color);
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 12px;
  transition: all 0.2s;
  position: relative;

  &:hover {
    border-color: var(--el-color-primary);
    box-shadow: 0 4px 12px rgba(64, 158, 255, 0.15);
  }

  &.is-current {
    border-color: var(--el-color-primary);
    background: linear-gradient(135deg, var(--el-color-primary-light-9) 0%, var(--el-bg-color) 100%);
    box-shadow: 0 0 0 1px var(--el-color-primary-light-5);
  }

  &.is-protected {
    border-color: var(--el-color-danger-light-5);
  }

  .branch-icon {
    flex-shrink: 0;
    color: var(--el-color-success);
  }

  .branch-info {
    flex: 1;
    min-width: 0;
  }

  .branch-name {
    font-weight: 500;
    font-size: 14px;
    font-family: monospace;
    margin-bottom: 4px;
  }

  .branch-commit {
    font-size: 11px;
    color: var(--el-text-color-secondary);
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .branch-badges {
    display: flex;
    gap: 6px;
    flex-shrink: 0;
  }
}

.branch-footer {
  display: flex;
  justify-content: center;

  .new-branch-btn {
    min-width: 200px;
  }
}

// 冲突内容
.conflicts-content {
  padding: 16px;
}

.conflict-alert {
  margin-bottom: 20px;
}

.conflict-list {
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.conflict-card {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 16px;
  background: var(--el-bg-color);
  border: 1px solid var(--el-color-warning-light-5);
  border-radius: 12px;

  .conflict-icon {
    flex-shrink: 0;
  }

  .conflict-info {
    flex: 1;
  }

  .conflict-path {
    font-weight: 500;
    font-size: 14px;
    font-family: monospace;
    margin-bottom: 8px;
  }

  .conflict-branches {
    display: flex;
    align-items: center;
    gap: 8px;

    .vs-icon {
      font-size: 14px;
      color: var(--el-text-color-secondary);
    }
  }

  .conflict-actions {
    display: flex;
    gap: 8px;
  }
}

// ============ 对话框样式 ============

// 提交对话框
.commit-dialog {
  :deep(.el-dialog__header) {
    display: none;
  }

  :deep(.el-dialog__body) {
    padding: 0;
  }

  :deep(.el-dialog__footer) {
    padding: 16px 24px;
    border-top: 1px solid var(--el-border-color-lighter);
  }
}

.commit-dialog-content {
  padding: 24px;
}

.commit-dialog-header {
  display: flex;
  align-items: center;
  gap: 14px;
  margin-bottom: 24px;

  .commit-dialog-title {
    font-size: 20px;
    font-weight: 600;
    color: var(--el-text-color-primary);
  }

  .commit-dialog-subtitle {
    font-size: 13px;
    color: var(--el-text-color-secondary);
    margin-top: 2px;
  }
}

.commit-type-section {
  margin-bottom: 24px;

  .section-label {
    font-size: 13px;
    font-weight: 500;
    color: var(--el-text-color-regular);
    margin-bottom: 12px;
  }
}

.commit-type-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 10px;
}

.commit-type-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 14px 10px;
  background: var(--el-fill-color-light);
  border: 2px solid transparent;
  border-radius: 10px;
  cursor: pointer;
  transition: all 0.2s;
  position: relative;

  &:hover {
    background: var(--el-fill-color);
    transform: translateY(-2px);
  }

  &.is-active {
    border-color: var(--el-color-primary);
    background: var(--el-color-primary-light-9);
  }

  .type-icon {
    width: 36px;
    height: 36px;
    border-radius: 8px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: white;
    margin-bottom: 8px;
  }

  .type-info {
    text-align: center;
  }

  .type-name {
    font-size: 12px;
    font-weight: 600;
    color: var(--el-text-color-primary);
  }

  .type-desc {
    font-size: 10px;
    color: var(--el-text-color-secondary);
    margin-top: 2px;
  }

  .type-check {
    position: absolute;
    top: 6px;
    right: 6px;
    color: var(--el-color-primary);
  }
}

.commit-form-section {
  margin-bottom: 20px;

  .form-row {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
    margin-bottom: 12px;
  }

  .form-item {
    label {
      display: block;
      font-size: 13px;
      font-weight: 500;
      color: var(--el-text-color-regular);
      margin-bottom: 6px;
    }

    :deep(.el-input__wrapper),
    :deep(.el-textarea__inner) {
      border-radius: 8px;
    }
  }
}

.commit-preview-section {
  margin-bottom: 16px;

  .preview-header {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 13px;
    font-weight: 500;
    color: var(--el-text-color-regular);
    margin-bottom: 8px;
  }

  .preview-content {
    background: var(--el-fill-color-light);
    border: 1px solid var(--el-border-color);
    border-radius: 8px;
    overflow: hidden;

    pre {
      margin: 0;
      padding: 12px 14px;
      font-size: 12px;
      font-family: 'Consolas', 'Monaco', monospace;
      line-height: 1.6;
      white-space: pre-wrap;
      word-break: break-all;
      color: var(--el-text-color-regular);
    }
  }
}

.commit-options {
  display: flex;
  gap: 20px;

  :deep(.el-checkbox) {
    display: flex;
    align-items: center;
    gap: 6px;
  }
}

.commit-dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}

// 分支对话框
.branch-dialog {
  :deep(.el-dialog__header) {
    display: none;
  }
}

.branch-dialog-content {
  padding: 24px;
}

.branch-dialog-header {
  display: flex;
  align-items: center;
  gap: 14px;
  margin-bottom: 24px;

  .branch-dialog-title {
    font-size: 20px;
    font-weight: 600;
    color: var(--el-text-color-primary);
  }

  .branch-dialog-subtitle {
    font-size: 13px;
    color: var(--el-text-color-secondary);
    margin-top: 2px;
  }
}

.branch-name-section {
  margin-bottom: 20px;

  .section-label {
    font-size: 13px;
    font-weight: 500;
    color: var(--el-text-color-regular);
    margin-bottom: 10px;
  }
}

.branch-namer {
  display: flex;
  gap: 0;

  .branch-prefix {
    width: 120px;
    flex-shrink: 0;

    :deep(.el-input__wrapper) {
      border-radius: 8px 0 0 8px;
    }
  }

  .branch-input {
    flex: 1;

    :deep(.el-input__wrapper) {
      border-radius: 0 8px 8px 0;
      border-left: none;
    }
  }
}

.branch-preview {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 10px;
  font-size: 12px;

  .preview-label {
    color: var(--el-text-color-secondary);
  }

  .preview-name {
    background: var(--el-fill-color-light);
    padding: 4px 8px;
    border-radius: 4px;
    font-family: monospace;
    color: var(--el-color-primary);
  }
}

.branch-error {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-top: 8px;
  font-size: 12px;
  color: var(--el-color-danger);
}

.branch-success {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-top: 8px;
  font-size: 12px;
  color: var(--el-color-success);
}

.branch-base-section {
  margin-bottom: 20px;

  .section-label {
    font-size: 13px;
    font-weight: 500;
    color: var(--el-text-color-regular);
    margin-bottom: 10px;
  }

  .branch-base-select {
    width: 100%;
  }
}

.branch-option {
  display: flex;
  flex-direction: column;
  gap: 2px;

  .option-name {
    font-weight: 500;
  }

  .option-commit {
    font-size: 11px;
    color: var(--el-text-color-secondary);
  }
}

.branch-tips {
  background: var(--el-fill-color-light);
  border-radius: 10px;
  padding: 14px;

  .tip-title {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 13px;
    font-weight: 500;
    color: var(--el-text-color-regular);
    margin-bottom: 12px;
  }

  .tip-list {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 8px;
  }

  .tip-item {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 12px;
    color: var(--el-text-color-secondary);
  }
}

.branch-dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}

// 差异对话框
.diff-dialog {
  :deep(.el-dialog__header) {
    display: none;
  }

  :deep(.el-dialog__body) {
    padding: 0;
  }
}

.diff-viewer-compact {
  display: flex;
  flex-direction: column;
  height: 75vh;

  .diff-toolbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 14px 18px;
    background: var(--el-fill-color-light);
    border-bottom: 1px solid var(--el-border-color);

    .diff-file-info {
      display: flex;
      align-items: center;
      gap: 8px;

      .diff-path {
        font-weight: 500;
        font-size: 13px;
        font-family: monospace;
      }
    }

    .diff-stats {
      display: flex;
      gap: 8px;
      align-items: center;
    }
  }

  .diff-body {
    flex: 1;
    overflow: auto;
    background: var(--el-bg-color);
  }
}

.diff-code {
  font-family: 'Consolas', 'Monaco', monospace;
  font-size: 13px;
  line-height: 24px;
}

.diff-row {
  display: flex;
  border-bottom: 1px solid var(--el-border-color-lighter);

  .diff-line-num {
    width: 50px;
    padding-right: 16px;
    text-align: right;
    color: var(--el-text-color-secondary);
    user-select: none;
    background: var(--el-fill-color-light);
    border-right: 1px solid var(--el-border-color-lighter);
    flex-shrink: 0;
  }

  .diff-prefix {
    width: 20px;
    text-align: center;
    color: var(--el-text-color-secondary);
    user-select: none;
    flex-shrink: 0;
  }

  .diff-code-line {
    flex: 1;
    padding-left: 12px;
    white-space: pre;
  }

  &.diff-added {
    background: var(--el-color-success-light-9);
    .diff-line-num { color: var(--el-color-success-dark-2); }
    .diff-prefix { color: var(--el-color-success); }
  }

  &.diff-removed {
    background: var(--el-color-danger-light-9);
    .diff-line-num { color: var(--el-color-danger-dark-2); }
    .diff-prefix { color: var(--el-color-danger); }
  }
}

// 冲突对话框
.conflict-dialog {
  :deep(.el-dialog__header) {
    display: none;
  }

  :deep(.el-dialog__body) {
    padding: 0;
  }
}

.conflict-resolver {
  display: flex;
  flex-direction: column;
  height: 80vh;

  .conflict-resolver-header {
    display: flex;
    align-items: center;
    gap: 14px;
    padding: 16px 20px;
    background: var(--el-fill-color-light);
    border-bottom: 1px solid var(--el-border-color);

    .header-info {
      flex: 1;
    }

    .header-title {
      font-size: 16px;
      font-weight: 600;
      color: var(--el-text-color-primary);
    }

    .header-file {
      font-size: 12px;
      color: var(--el-text-color-secondary);
      font-family: monospace;
      margin-top: 2px;
    }
  }

  .conflict-panels {
    flex: 1;
    display: grid;
    grid-template-columns: 1fr 1fr 1fr;
    overflow: hidden;

    &.edit-mode {
      grid-template-columns: 0 0 1fr;
    }

    .conflict-panel.hidden {
      display: none;
    }
  }

  .conflict-panel {
    display: flex;
    flex-direction: column;
    border-right: 1px solid var(--el-border-color);
    overflow: hidden;

    &:last-child {
      border-right: none;
    }

    .panel-header {
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      padding: 12px 14px;
      background: var(--el-fill-color);
      border-bottom: 1px solid var(--el-border-color);
      font-size: 13px;
      font-weight: 500;
    }

    .merge-header {
      background: var(--el-color-success-light-9);
      color: var(--el-color-success-dark-2);
    }

    .panel-body {
      flex: 1;
      overflow: auto;
      background: var(--el-bg-color);
    }

    .panel-code {
      margin: 0;
      padding: 14px;
      font-family: 'Consolas', 'Monaco', monospace;
      font-size: 12px;
      line-height: 1.6;
      white-space: pre-wrap;
      word-break: break-all;
    }

    .merge-editor {
      width: 100%;
      height: 100%;
      border: none;
      resize: none;
      padding: 14px;
      font-family: 'Consolas', 'Monaco', monospace;
      font-size: 12px;
      line-height: 1.6;
      background: transparent;
      outline: none;
    }
  }
}

.conflict-dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  padding: 16px 24px;
  border-top: 1px solid var(--el-border-color-lighter);
}

// 响应式
@media (max-width: 768px) {
  .commit-type-grid {
    grid-template-columns: repeat(2, 1fr);
  }

  .branch-list {
    grid-template-columns: 1fr;
  }

  .tip-list {
    grid-template-columns: 1fr;
  }
}
</style>
