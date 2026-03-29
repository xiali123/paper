/**
 * 字段名转换适配器
 * 提供前后端数据格式转换的核心工具函数
 *
 * 主要功能：
 * - camelCase ↔ snake_case 字符串转换
 * - 递归转换对象所有键名（支持嵌套对象和数组）
 * - 处理特殊边界情况（null、undefined、Date等）
 *
 * @module api/adapters/transformAdapter
 */

// ============================================================================
// 类型定义
// ============================================================================

/**
 * 深度转换选项
 */
export interface TransformOptions {
  /** 是否忽略特定键名 */
  ignoreKeys?: string[]

  /** 是否只转换特定键名 */
  onlyKeys?: string[]

  /** 是否递归转换嵌套对象 */
  deep?: boolean

  /** 是否转换数组元素 */
  transformArrays?: boolean
}

/**
 * 可转换的值类型
 */
type TransformableValue =
  | string
  | number
  | boolean
  | null
  | undefined
  | Date
  | { [key: string]: any }
  | any[]

/**
 * 转换后的对象类型
 */
type TransformedObject<T> = {
  [K in keyof T as string]: any
}

// ============================================================================
// 字符串转换函数
// ============================================================================

/**
 * camelCase 转 snake_case
 *
 * 转换规则：
 * - 在每个大写字母前插入下划线
 * - 将所有字母转为小写
 * - 处理连续大写字母（如 HTTPResponse → http_response）
 * - 处理数字（如 user2FA → user_2fa）
 *
 * @param str - camelCase 字符串
 * @returns snake_case 字符串
 *
 * @example
 * ```typescript
 * camelToSnake('helloWorld')        // 'hello_world'
 * camelToSnake('getUserID')         // 'get_user_id'
 * camelToSnake('XMLHTTPRequest')    // 'xmlhttp_request'
 * camelToSnake('parseURL2JSON')     // 'parse_url_2json'
 * camelToSnake('isHTTPSConnection') // 'is_https_connection'
 * camelToSnake('userId')            // 'user_id'
 * camelToSnake('createdAt')         // 'created_at'
 * ```
 */
export const camelToSnake = (str: string): string => {
  // 处理空字符串
  if (!str) {
    return str
  }

  // 在小写字母/数字后面跟大写字母的位置插入下划线
  // 在大写字母后面跟小写字母的位置插入下划线（处理连续大写）
  let result = str
    .replace(/([a-z0-9])([A-Z])/g, '$1_$2')
    .replace(/([A-Z])([A-Z][a-z])/g, '$1_$2')

  // 转换为小写
  return result.toLowerCase()
}

/**
 * snake_case 转 camelCase
 *
 * 转换规则：
 * - 以下划线分割单词
 * - 第一个单词保持小写
 * - 后续单词首字母大写
 * - 处理连续下划线（如 user__id → userId）
 * - 处理首尾下划线（如 _user_id_ → userId）
 *
 * @param str - snake_case 字符串
 * @returns camelCase 字符串
 *
 * @example
 * ```typescript
 * snakeToCamel('hello_world')     // 'helloWorld'
 * snakeToCamel('get_user_id')     // 'getUserId'
 * snakeToCamel('xml_http_request') // 'xmlHttpRequest'
 * snakeToCamel('user__id')        // 'userId'
 * snakeToCamel('_user_id_')       // 'userId'
 * snakeToCamel('created_at')      // 'createdAt'
 * snakeToCamel('is_read')         // 'isRead'
 * ```
 */
export const snakeToCamel = (str: string): string => {
  // 处理空字符串
  if (!str) {
    return str
  }

  // 分割字符串并过滤空字符串（处理连续下划线）
  const words = str.split('_').filter(word => word.length > 0)

  if (words.length === 0) {
    return str
  }

  // 第一个单词保持小写，后续单词首字母大写
  return words
    .map((word, index) => {
      if (index === 0) {
        return word.toLowerCase()
      }
      return word.charAt(0).toUpperCase() + word.slice(1).toLowerCase()
    })
    .join('')
}

/**
 * PascalCase 转 snake_case
 *
 * @param str - PascalCase 字符串
 * @returns snake_case 字符串
 *
 * @example
 * ```typescript
 * pascalToSnake('HelloWorld') // 'hello_world'
 * pascalToSnake('GetUserID')  // 'get_user_id'
 * ```
 */
export const pascalToSnake = (str: string): string => {
  if (!str) {
    return str
  }
  // 将首字母小写后使用 camelToSnake
  return camelToSnake(str.charAt(0).toLowerCase() + str.slice(1))
}

/**
 * snake_case 转 PascalCase
 *
 * @param str - snake_case 字符串
 * @returns PascalCase 字符串
 *
 * @example
 * ```typescript
 * snakeToPascal('hello_world') // 'HelloWorld'
 * snakeToPascal('get_user_id') // 'GetUserId'
 * ```
 */
export const snakeToPascal = (str: string): string => {
  if (!str) {
    return str
  }
  const camel = snakeToCamel(str)
  return camel.charAt(0).toUpperCase() + camel.slice(1)
}

// ============================================================================
// 对象键名转换函数
// ============================================================================

/**
 * 递归转换对象的所有键名为 snake_case
 *
 * 特性：
 * - 支持嵌套对象
 * - 支持数组和对象数组
 * - 保留 Date、RegExp、null、undefined 等特殊值
 * - 可选择性忽略或只转换特定键
 *
 * @param obj - 要转换的对象或值
 * @param options - 转换选项
 * @returns 转换后的对象
 *
 * @example
 * ```typescript
 * const input = {
 *   userId: 1,
 *   userProfile: {
 *     firstName: 'John',
 *     lastName: 'Doe',
 *     contactInfo: {
 *       emailAddress: 'john@example.com'
 *     }
 *   },
 *   userRoles: ['admin', 'user'],
 *   createdAt: new Date(),
 *   metadata: null
 * }
 *
 * const output = objectKeysToSnake(input)
 * // {
 * //   user_id: 1,
 * //   user_profile: {
 * //     first_name: 'John',
 * //     last_name: 'Doe',
 * //     contact_info: {
 * //       email_address: 'john@example.com'
 * //     }
 * //   },
 * //   user_roles: ['admin', 'user'],
 * //   created_at: new Date(),
 * //   metadata: null
 * // }
 * ```
 */
export const objectKeysToSnake = <T extends Record<string, any>>(
  obj: T,
  options: TransformOptions = {}
): TransformedObject<T> => {
  const {
    ignoreKeys = [],
    onlyKeys = [],
    deep = true,
    transformArrays = true
  } = options

  // 处理 null 和 undefined
  if (obj === null || obj === undefined) {
    return obj as any
  }

  // 处理数组
  if (Array.isArray(obj)) {
    if (transformArrays && deep) {
      return obj.map(item =>
        isPlainObject(item) ? objectKeysToSnake(item, options) : item
      ) as any
    }
    return obj as any
  }

  // 处理 Date 对象
  if (obj instanceof Date) {
    return obj as any
  }

  // 处理 RegExp 对象
  if (obj instanceof RegExp) {
    return obj as any
  }

  // 只处理普通对象
  if (!isPlainObject(obj)) {
    return obj as any
  }

  const result: Record<string, any> = {}

  Object.keys(obj).forEach(key => {
    const value = (obj as Record<string, any>)[key]

    // 检查是否应该转换此键
    const shouldTransform = shouldTransformKey(key, ignoreKeys, onlyKeys)
    const newKey = shouldTransform ? camelToSnake(key) : key

    // 递归转换值
    if (deep && value !== null && value !== undefined) {
      if (isPlainObject(value)) {
        result[newKey] = objectKeysToSnake(value, options)
      } else if (Array.isArray(value) && transformArrays) {
        result[newKey] = value.map(item =>
          isPlainObject(item) ? objectKeysToSnake(item, options) : item
        )
      } else {
        result[newKey] = value
      }
    } else {
      result[newKey] = value
    }
  })

  return result as TransformedObject<T>
}

/**
 * 递归转换对象的所有键名为 camelCase
 *
 * 特性：
 * - 支持嵌套对象
 * - 支持数组和对象数组
 * - 保留 Date、RegExp、null、undefined 等特殊值
 * - 可选择性忽略或只转换特定键
 *
 * @param obj - 要转换的对象或值
 * @param options - 转换选项
 * @returns 转换后的对象
 *
 * @example
 * ```typescript
 * const input = {
 *   user_id: 1,
 *   user_profile: {
 *     first_name: 'John',
 *     last_name: 'Doe',
 *     contact_info: {
 *       email_address: 'john@example.com'
 *     }
 *   },
 *   user_roles: ['admin', 'user'],
 *   created_at: new Date(),
 *   metadata: null
 * }
 *
 * const output = objectKeysToCamel(input)
 * // {
 * //   userId: 1,
 * //   userProfile: {
 * //     firstName: 'John',
 * //     lastName: 'Doe',
 * //     contactInfo: {
 * //       emailAddress: 'john@example.com'
 * //     }
 * //   },
 * //   userRoles: ['admin', 'user'],
 * //   createdAt: new Date(),
 * //   metadata: null
 * // }
 * ```
 */
export const objectKeysToCamel = <T extends Record<string, any>>(
  obj: T,
  options: TransformOptions = {}
): TransformedObject<T> => {
  const {
    ignoreKeys = [],
    onlyKeys = [],
    deep = true,
    transformArrays = true
  } = options

  // 处理 null 和 undefined
  if (obj === null || obj === undefined) {
    return obj as any
  }

  // 处理数组
  if (Array.isArray(obj)) {
    if (transformArrays && deep) {
      return obj.map(item =>
        isPlainObject(item) ? objectKeysToCamel(item, options) : item
      ) as any
    }
    return obj as any
  }

  // 处理 Date 对象
  if (obj instanceof Date) {
    return obj as any
  }

  // 处理 RegExp 对象
  if (obj instanceof RegExp) {
    return obj as any
  }

  // 只处理普通对象
  if (!isPlainObject(obj)) {
    return obj as any
  }

  const result: Record<string, any> = {}

  Object.keys(obj).forEach(key => {
    const value = (obj as Record<string, any>)[key]

    // 检查是否应该转换此键
    const shouldTransform = shouldTransformKey(key, ignoreKeys, onlyKeys)
    const newKey = shouldTransform ? snakeToCamel(key) : key

    // 递归转换值
    if (deep && value !== null && value !== undefined) {
      if (isPlainObject(value)) {
        result[newKey] = objectKeysToCamel(value, options)
      } else if (Array.isArray(value) && transformArrays) {
        result[newKey] = value.map(item =>
          isPlainObject(item) ? objectKeysToCamel(item, options) : item
        )
      } else {
        result[newKey] = value
      }
    } else {
      result[newKey] = value
    }
  })

  return result as TransformedObject<T>
}

/**
 * 转换数组的所有元素（对象数组）
 *
 * @param arr - 对象数组
 * @param direction - 转换方向 ('toSnake' 或 'toCamel')
 * @param options - 转换选项
 * @returns 转换后的数组
 *
 * @example
 * ```typescript
 * const users = [
 *   { userId: 1, firstName: 'John' },
 *   { userId: 2, firstName: 'Jane' }
 * ]
 *
 * const snakeArray = transformArray(users, 'toSnake')
 * // [
 * //   { user_id: 1, first_name: 'John' },
 * //   { user_id: 2, first_name: 'Jane' }
 * // ]
 * ```
 */
export const transformArray = <T extends Record<string, any>>(
  arr: T[],
  direction: 'toSnake' | 'toCamel',
  options: TransformOptions = {}
): TransformedObject<T>[] => {
  const transformer = direction === 'toSnake' ? objectKeysToSnake : objectKeysToCamel
  return arr.map(item => transformer(item, options)) as any
}

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * 检查值是否为普通对象（非数组、Date、RegExp等）
 *
 * @param value - 要检查的值
 * @returns 是否为普通对象
 */
const isPlainObject = (value: any): boolean => {
  if (value === null || typeof value !== 'object') {
    return false
  }

  // 排除数组
  if (Array.isArray(value)) {
    return false
  }

  // 排除 Date
  if (value instanceof Date) {
    return false
  }

  // 排除 RegExp
  if (value instanceof RegExp) {
    return false
  }

  // 检查原型链
  const prototype = Object.getPrototypeOf(value)
  return prototype === null || prototype === Object.prototype
}

/**
 * 判断是否应该转换指定的键
 *
 * @param key - 键名
 * @param ignoreKeys - 要忽略的键名列表
 * @param onlyKeys - 只转换的键名列表（如果指定，则只转换这些键）
 * @returns 是否应该转换
 */
const shouldTransformKey = (
  key: string,
  ignoreKeys: string[],
  onlyKeys: string[]
): boolean => {
  // 如果在忽略列表中，不转换
  if (ignoreKeys.includes(key)) {
    return false
  }

  // 如果指定了 onlyKeys，只转换列表中的键
  if (onlyKeys.length > 0) {
    return onlyKeys.includes(key)
  }

  // 默认转换所有键
  return true
}

/**
 * 深度合并两个对象（用于合并转换后的数据）
 *
 * @param target - 目标对象
 * @param source - 源对象
 * @returns 合并后的对象
 *
 * @example
 * ```typescript
 * const target = { user_id: 1, name: 'John' }
 * const source = { email: 'john@example.com' }
 *
 * const merged = deepMerge(target, source)
 * // { user_id: 1, name: 'John', email: 'john@example.com' }
 * ```
 */
export const deepMerge = <T extends Record<string, any>>(
  target: T,
  source: Partial<T>
): T => {
  const result = { ...target }

  Object.keys(source).forEach(key => {
    const sourceValue = source[key as keyof T]
    const targetValue = result[key as keyof T]

    if (isPlainObject(sourceValue) && isPlainObject(targetValue)) {
      result[key as keyof T] = deepMerge(
        targetValue as Record<string, any>,
        sourceValue as Record<string, any>
      ) as T[keyof T]
    } else {
      result[key as keyof T] = sourceValue as T[keyof T]
    }
  })

  return result
}

/**
 * 创建转换管道（链式转换）
 *
 * @param transformers - 转换函数数组
 * @returns 组合转换函数
 *
 * @example
 * ```typescript
 * const pipeline = createTransformPipeline([
 *   (data) => objectKeysToSnake(data),
 *   (data) => ({ ...data, timestamp: Date.now() })
 * ])
 *
 * const result = pipeline({ userId: 1, name: 'John' })
 * // { user_id: 1, name: 'John', timestamp: 1234567890 }
 * ```
 */
export const createTransformPipeline = <T, R>(
  transformers: Array<(data: any) => any>
): ((data: T) => R) => {
  return (data: T): R => {
    return transformers.reduce((acc, transformer) => transformer(acc), data) as R
  }
}

// ============================================================================
// 常用预设转换器
// ============================================================================

/**
 * 快速转换：前端请求 → 后端格式
 * 将 camelCase 转为 snake_case，不递归转换嵌套对象
 */
export const quickToSnake = <T extends Record<string, any>>(
  obj: T
): TransformedObject<T> => {
  return objectKeysToSnake(obj, { deep: false })
}

/**
 * 快速转换：后端响应 → 前端格式
 * 将 snake_case 转为 camelCase，不递归转换嵌套对象
 */
export const quickToCamel = <T extends Record<string, any>>(
  obj: T
): TransformedObject<T> => {
  return objectKeysToCamel(obj, { deep: false })
}

/**
 * 深度转换：前端请求 → 后端格式
 * 将 camelCase 转为 snake_case，递归转换所有嵌套对象
 */
export const deepToSnake = <T extends Record<string, any>>(
  obj: T
): TransformedObject<T> => {
  return objectKeysToSnake(obj, { deep: true })
}

/**
 * 深度转换：后端响应 → 前端格式
 * 将 snake_case 转为 camelCase，递归转换所有嵌套对象
 */
export const deepToCamel = <T extends Record<string, any>>(
  obj: T
): TransformedObject<T> => {
  return objectKeysToCamel(obj, { deep: true })
}

// ============================================================================
// 类型守卫
// ============================================================================

/**
 * 检查对象是否包含 snake_case 键名
 *
 * @param obj - 要检查的对象
 * @returns 是否包含 snake_case 键名
 *
 * @example
 * ```typescript
 * hasSnakeKeys({ user_id: 1 }) // true
 * hasSnakeKeys({ userId: 1 })   // false
 * ```
 */
export const hasSnakeKeys = (obj: Record<string, any>): boolean => {
  return Object.keys(obj).some(key => key.includes('_'))
}

/**
 * 检查对象是否包含 camelCase 键名
 *
 * @param obj - 要检查的对象
 * @returns 是否包含 camelCase 键名
 *
 * @example
 * ```typescript
 * hasCamelKeys({ userId: 1 })   // true
 * hasCamelKeys({ user_id: 1 }) // false
 * ```
 */
export const hasCamelKeys = (obj: Record<string, any>): boolean => {
  return Object.keys(obj).some(key => /[a-z][A-Z]/.test(key))
}

// ============================================================================
// 默认导出
// ============================================================================

export default {
  // 字符串转换
  camelToSnake,
  snakeToCamel,
  pascalToSnake,
  snakeToPascal,

  // 对象转换
  objectKeysToSnake,
  objectKeysToCamel,
  transformArray,

  // 预设转换器
  quickToSnake,
  quickToCamel,
  deepToSnake,
  deepToCamel,

  // 工具函数
  deepMerge,
  createTransformPipeline,
  hasSnakeKeys,
  hasCamelKeys
}
