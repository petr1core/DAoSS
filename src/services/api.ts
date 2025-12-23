import { getToken, removeToken } from '../utils/auth';

const API_BASE_URL = 'http://localhost:5143/api';

interface ApiError {
  message: string;
}

interface LoginResponse {
  token: string;
}

interface RegisterResponse {
  token: string;
}

interface UserInfo {
  sub: string;
  name: string;
  email: string;
}

interface ValidateResponse {
  isValid: boolean;
}

interface ParserRequest {
  code: string;
  language: string;
}

interface ParserResponse {
  success: boolean;
  representation?: any;
  representationType?: string;
  error?: string;
  lexerErrors?: any[];
  parserErrors?: any[];
}

interface ValidationResponse {
  valid: boolean;
  lexerErrors?: any[];
  parserErrors?: any[];
}

interface SimpleValidationResponse {
  valid: boolean;
  hasErrors: boolean;
  lexerErrorsCount: number;
  parserErrorsCount: number;
}

// Project types
export interface Project {
  id: string;
  name: string;
  description?: string;
  visibility: string;
  createdAt: string;
  ownerId: string;
  defaultLanguageId?: string;
  requiredReviewersRules?: string;
}

export interface CreateProjectDto {
  name: string;
  description: string; // Всегда передаем, даже если пустая строка
  ownerId: string;
  visibility?: string;
  defaultLanguageId?: string;
  requiredReviewersRules?: string;
}

export interface UpdateProjectDto {
  name?: string;
  description: string; // Всегда передаем, даже если пустая строка
  ownerId: string;
  visibility?: string;
  defaultLanguageId?: string;
  requiredReviewersRules?: string;
}

// Invitation types
export interface Invitation {
  id: string;
  projectId: string;
  role: 'admin' | 'reviewer';
  status: string;
  expiresAt: string;
  createdAt: string;
}

export interface SendInvitationDto {
  invitedUserId: string;
  role: 'admin' | 'reviewer';
}

// Member types
export interface ProjectMember {
  userId: string;
  role: 'owner' | 'admin' | 'reviewer';
  name?: string;
  email?: string;
}

export interface CreateMemberDto {
  userId: string;
  role: 'admin' | 'reviewer';
}

export interface UpdateRoleDto {
  role: 'admin' | 'reviewer';
}

// Review types
export interface Review {
  id: string;
  projectId: string;
  status: string;
  createdAt: string;
  updatedAt?: string;
}

export interface CreateReviewDto {
  targetType: string;
  targetId: string;
}

export interface UpdateReviewDto {
  status?: 'approved' | 'changes_requested';
}

export interface ReviewItem {
  id: string;
  reviewId: string;
  kind: 'comment' | 'issue';
  anchorType: 'code' | 'diagram';
  anchorRef: string;
  body: string;
  status: 'open' | 'resolved';
  createdAt?: string;
  updatedAt?: string;
}

export interface CreateReviewItemDto {
  kind: 'comment' | 'issue';
  anchorType: 'code' | 'diagram';
  anchorRef: string;
  body: string;
}

export interface UpdateReviewItemDto {
  body?: string;
  status?: 'open' | 'resolved';
}

export interface Comment {
  id: string;
  reviewItemId: string;
  body: string;
  createdAt: string;
  updatedAt?: string;
  authorId?: string;
  authorName?: string;
}

export interface CreateCommentDto {
  body: string;
}

export interface UpdateCommentDto {
  body: string;
}

// Source File types
export interface SourceFile {
  id: string;
  projectId: string;
  path: string;
  languageId: string;
  latestVersionId?: string;
  createdAt: string;
}

export interface SourceFileVersion {
  id: string;
  sourceFileId: string;
  versionIndex: number;
  content: string;
  authorId: string;
  authorName: string;
  message: string;
  createdAt: string;
  isVerified: boolean;
}

export interface CreateSourceFileDto {
  path: string;
  content: string;
  message?: string;
}

export interface CreateSourceFileVersionDto {
  content: string;
  message?: string;
}

// User types
export interface User {
  id: string;
  login: string;
  email: string;
  name: string;
}

// Language types
export interface Language {
  id: string;
  code: string;
  name: string;
  versionHint?: string;
  fileExtensions?: string;
}

async function handleResponse<T>(response: Response): Promise<T> {
  if (!response.ok) {
    let errorMessage = 'Произошла ошибка';

    try {
      const errorData: ApiError = await response.json();
      errorMessage = errorData.message || errorMessage;
    } catch {
      if (response.status === 401) {
        errorMessage = 'Неверный логин или пароль';
      } else if (response.status === 500) {
        errorMessage = 'Ошибка сервера, попробуйте позже';
      } else if (response.status === 0 || response.status >= 500) {
        errorMessage = 'Не удалось подключиться к серверу';
      } else {
        errorMessage = `Ошибка ${response.status}`;
      }
    }

    throw new Error(errorMessage);
  }

  // Если ответ 204 No Content, возвращаем пустой объект
  if (response.status === 204) {
    return {} as T;
  }

  // Проверяем, есть ли контент
  const contentType = response.headers.get('content-type');
  if (!contentType || !contentType.includes('application/json')) {
    return {} as T;
  }

  return response.json();
}

async function fetchWithAuth<T>(url: string, options: RequestInit = {}): Promise<T> {
  const token = getToken();
  
  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    ...(options.headers as Record<string, string>),
  };

  if (token) {
    headers['Authorization'] = `Bearer ${token}`;
  }

  try {
    // Добавляем таймаут для fetch запроса
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 5000); // 5 секунд таймаут

    const response = await fetch(`${API_BASE_URL}${url}`, {
      ...options,
      headers,
      signal: controller.signal,
    });

    clearTimeout(timeoutId);

    // Если токен невалидный, удаляем его
    if (response.status === 401) {
      removeToken();
      // Вызываем событие для обновления состояния авторизации
      window.dispatchEvent(new CustomEvent('auth:logout'));
    }

    return handleResponse<T>(response);
  } catch (error) {
    if (error instanceof TypeError && error.message.includes('fetch')) {
      throw new Error('Не удалось подключиться к серверу');
    }
    if (error instanceof Error && error.name === 'AbortError') {
      throw new Error('Не удалось подключиться к серверу (таймаут)');
    }
    throw error;
  }
}

async function fetchWithoutAuth<T>(url: string, options: RequestInit = {}): Promise<T> {
  const headers: HeadersInit = {
    'Content-Type': 'application/json',
    ...options.headers,
  };

  try {
    // Добавляем таймаут для fetch запроса
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 5000); // 5 секунд таймаут

    const response = await fetch(`${API_BASE_URL}${url}`, {
      ...options,
      headers,
      signal: controller.signal,
    });

    clearTimeout(timeoutId);

    return handleResponse<T>(response);
  } catch (error) {
    if (error instanceof TypeError && error.message.includes('fetch')) {
      throw new Error('Не удалось подключиться к серверу');
    }
    if (error instanceof Error && error.name === 'AbortError') {
      throw new Error('Не удалось подключиться к серверу (таймаут)');
    }
    throw error;
  }
}

export const api = {
  async login(login: string, password: string): Promise<string> {
    const response = await fetchWithoutAuth<LoginResponse>('/auth/login', {
      method: 'POST',
      body: JSON.stringify({ Login: login, Password: password }),
    });
    return response.token;
  },

  async register(
    email: string,
    password: string,
    name?: string,
    login?: string
  ): Promise<string> {
    const response = await fetchWithoutAuth<RegisterResponse>('/auth/register', {
      method: 'POST',
      body: JSON.stringify({
        Email: email,
        Password: password,
        Name: name,
        Login: login,
      }),
    });
    return response.token;
  },

  async getMe(): Promise<UserInfo> {
    return fetchWithAuth<UserInfo>('/auth/me', {
      method: 'GET',
    });
  },

  async validateToken(): Promise<boolean> {
    try {
      const response = await fetchWithAuth<ValidateResponse>('/auth/validate', {
        method: 'GET',
      });
      return response.isValid;
    } catch {
      return false;
    }
  },

  logout(): void {
    removeToken();
  },

  async parseCode(code: string, language: string): Promise<ParserResponse> {
    return fetchWithAuth<ParserResponse>('/parser/parse', {
      method: 'POST',
      body: JSON.stringify({ Code: code, Language: language }),
    });
  },

  async validateCode(code: string, language: string): Promise<ValidationResponse> {
    return fetchWithAuth<ValidationResponse>('/parser/validate', {
      method: 'POST',
      body: JSON.stringify({ Code: code, Language: language }),
    });
  },

  async validateCodeSimple(code: string, language: string): Promise<SimpleValidationResponse> {
    return fetchWithAuth<SimpleValidationResponse>('/parser/validate/simple', {
      method: 'POST',
      body: JSON.stringify({ Code: code, Language: language }),
    });
  },

  async generateCode(representation: any, language: string): Promise<{ success: boolean; code?: string; error?: string }> {
    return fetchWithAuth<{ success: boolean; code?: string; error?: string }>('/parser/generate', {
      method: 'POST',
      body: JSON.stringify({ Representation: representation, Language: language }),
    });
  },
};

