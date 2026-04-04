export function isEmail(email: string): boolean {
  const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/
  return emailRegex.test(email)
}

export function isPhone(phone: string): boolean {
  const phoneRegex = /^1[3-9]\d{9}$/
  return phoneRegex.test(phone)
}

export function isUrl(url: string): boolean {
  try {
    new URL(url)
    return true
  } catch {
    return false
  }
}

export function isValidDate(date: string): boolean {
  return !isNaN(Date.parse(date))
}

export function isInRange(value: number, min: number, max: number): boolean {
  return value >= min && value <= max
}

export function isValidLength(value: string, min: number, max: number): boolean {
  return value.length >= min && value.length <= max
}

export function isStrongPassword(password: string): boolean {
  const passwordRegex = /^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[@$!%*?&])[A-Za-z\d@$!%*?&]{8,}$/
  return passwordRegex.test(password)
}

export function isValidUsername(username: string): boolean {
  const usernameRegex = /^[a-zA-Z0-9_]{3,20}$/
  return usernameRegex.test(username)
}

export function isDOI(doi: string): boolean {
  const doiRegex = /^10\.\d{4,9}\/[-._;()/:A-Z0-9]+$/i
  return doiRegex.test(doi)
}

export function required(value: any): boolean {
  if (value === null || value === undefined) return false
  if (typeof value === 'string') return value.trim().length > 0
  if (Array.isArray(value)) return value.length > 0
  return true
}
