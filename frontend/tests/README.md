# 单元测试指南

## 测试框架

- **Vitest** - 快速的单元测试框架
- **Vue Test Utils** - Vue组件测试工具
- **jsdom** - DOM环境模拟
- **@testing-library/vue** - 用户行为测试

## 测试命令

```bash
# 运行所有测试
npm run test

# 运行单元测试
npm run test:unit

# 监听模式（开发时使用）
npm run test:watch

# 生成覆盖率报告
npm run test:coverage

# 启动测试UI
npm run test:ui

# 运行特定测试文件
npm run test apiCache
```

## 测试目录结构

```
tests/
├── setup.ts                    # 测试环境设置
├── unit/                       # 单元测试
│   ├── apiCache.test.ts
│   ├── CollaborativeCursorTracker.test.ts
│   ├── CompilationMonitor.test.ts
│   ├── serviceWorker.test.ts
│   └── cloudStorageSync.test.ts
├── integration/                # 集成测试
├── e2e/                        # 端到端测试
└── performance/                # 性能测试
```

## 覆盖率目标

- **语句覆盖率**: 75%
- **分支覆盖率**: 70%
- **函数覆盖率**: 75%
- **行覆盖率**: 75%

## 编写测试的最佳实践

### 1. 测试文件命名

```
src/utils/apiCache.ts → tests/unit/apiCache.test.ts
src/components/Foo.vue → tests/unit/Foo.test.ts
```

### 2. 测试结构

```typescript
describe('ComponentName', () => {
  describe('Feature Group', () => {
    it('should do something when condition is met', () => {
      // Arrange
      const input = prepareInput()

      // Act
      const result = functionUnderTest(input)

      // Assert
      expect(result).toBe(expectedOutput)
    })
  })
})
```

### 3. 组件测试示例

```typescript
import { mount } from '@vue/test-utils'
import MyComponent from '@/components/MyComponent.vue'

describe('MyComponent', () => {
  it('should render correctly', () => {
    const wrapper = mount(MyComponent, {
      props: { title: 'Test' }
    })

    expect(wrapper.find('h1').text()).toBe('Test')
  })

  it('should emit event on button click', async () => {
    const wrapper = mount(MyComponent)

    await wrapper.find('button').trigger('click')

    expect(wrapper.emitted('click')).toBeTruthy()
  })
})
```

### 4. 工具函数测试示例

```typescript
import { formatSize } from '@/utils/format'

describe('formatSize', () => {
  it('should format bytes correctly', () => {
    expect(formatSize(0)).toBe('0 B')
    expect(formatSize(1024)).toBe('1 KB')
    expect(formatSize(1048576)).toBe('1 MB')
  })
})
```

### 5. 异步测试示例

```typescript
it('should fetch data async', async () => {
  const wrapper = mount(Component)

  await wrapper.vm.fetchData()

  expect(wrapper.vm.data).toEqual(expectedData)
})
```

## Mock策略

### Mock外部依赖

```typescript
// Mock fetch
global.fetch = vi.fn(() =>
  Promise.resolve({
    ok: true,
    json: () => Promise.resolve({ data: 'test' })
  })
)

// Mock localStorage
const localStorageMock = {
  getItem: vi.fn(),
  setItem: vi.fn(),
  clear: vi.fn()
}
global.localStorage = localStorageMock
```

### Mock Vue组件

```typescript
const wrapper = mount(Component, {
  global: {
    stubs: {
      'child-component': true
    }
  }
})
```

## CI/CD集成

测试在CI流水线中自动运行：

```yaml
# .github/workflows/test.yml
- name: Run tests
  run: npm run test:coverage

- name: Upload coverage
  uses: codecov/codecov-action@v3
  with:
    files: ./coverage/lcov.info
```

## 常见问题

### Q: 测试超时？
A: 增加测试超时时间：
```typescript
it('slow test', async () => {
  // ...
}, { timeout: 30000 })
```

### Q: 组件无法渲染？
A: 检查是否正确配置了全局mock，参考 `tests/setup.ts`

### Q: 覆盖率不正确？
A: 确保vitest配置中排除了不需要测试的文件

## 参考资源

- [Vitest文档](https://vitest.dev/)
- [Vue Test Utils](https://test-utils.vuejs.org/)
- [Testing Library](https://testing-library.com/docs/vue-testing-library/intro/)
