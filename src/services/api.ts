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
    ...(options.headers as Record<string, string> || {}),
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

  // Project methods
  async getProjects(userId: string): Promise<Project[]> {
    return fetchWithAuth<Project[]>(`/projects?ownerId=${userId}`, {
      method: 'GET',
    });
  },

  async getProject(projectId: string): Promise<Project> {
    return fetchWithAuth<Project>(`/projects/${projectId}`, {
      method: 'GET',
    });
  },

  async createProject(dto: CreateProjectDto): Promise<Project> {
    return fetchWithAuth<Project>('/projects', {
      method: 'POST',
      body: JSON.stringify({
        Name: dto.name,
        Description: dto.description?.trim() || null,
        OwnerId: dto.ownerId,
        Visibility: dto.visibility || 'private',
        DefaultLanguageId: dto.defaultLanguageId || '00000000-0000-0000-0000-000000000000',
        RequiredReviewersRules: dto.requiredReviewersRules || null,
      }),
    });
  },

  async updateProject(projectId: string, dto: UpdateProjectDto): Promise<Project> {
    return fetchWithAuth<Project>(`/projects/${projectId}`, {
      method: 'PUT',
      body: JSON.stringify({
        Name: dto.name,
        Description: dto.description?.trim() || null,
        OwnerId: dto.ownerId,
        Visibility: dto.visibility,
        DefaultLanguageId: dto.defaultLanguageId,
        RequiredReviewersRules: dto.requiredReviewersRules || null,
      }),
    });
  },

  async deleteProject(projectId: string): Promise<void> {
    return fetchWithAuth<void>(`/projects/${projectId}`, {
      method: 'DELETE',
    });
  },

  // Invitation methods
  async getUserInvitations(): Promise<Invitation[]> {
    return fetchWithAuth<Invitation[]>('/invitations', {
      method: 'GET',
    });
  },

  async getProjectInvitations(projectId: string, status?: string): Promise<Invitation[]> {
    const url = status
      ? `/projects/${projectId}/invitations?status=${status}`
      : `/projects/${projectId}/invitations`;
    return fetchWithAuth<Invitation[]>(url, {
      method: 'GET',
    });
  },

  async sendInvitation(projectId: string, dto: SendInvitationDto): Promise<Invitation> {
    return fetchWithAuth<Invitation>(`/projects/${projectId}/invitations`, {
      method: 'POST',
      body: JSON.stringify(dto),
    });
  },

  async acceptInvitation(invitationId: string): Promise<void> {
    return fetchWithAuth<void>(`/invitations/${invitationId}/accept`, {
      method: 'POST',
    });
  },

  async rejectInvitation(invitationId: string): Promise<void> {
    return fetchWithAuth<void>(`/invitations/${invitationId}/reject`, {
      method: 'POST',
    });
  },

  async cancelInvitation(projectId: string, invitationId: string): Promise<void> {
    return fetchWithAuth<void>(`/projects/${projectId}/invitations/${invitationId}`, {
      method: 'DELETE',
    });
  },

  // Member methods
  async getProjectMembers(projectId: string): Promise<ProjectMember[]> {
    return fetchWithAuth<ProjectMember[]>(`/projects/${projectId}/members`, {
      method: 'GET',
    });
  },

  async getProjectMember(projectId: string, userId: string): Promise<ProjectMember> {
    return fetchWithAuth<ProjectMember>(`/projects/${projectId}/members/${userId}`, {
      method: 'GET',
    });
  },

  async addProjectMember(projectId: string, dto: CreateMemberDto): Promise<ProjectMember> {
    return fetchWithAuth<ProjectMember>(`/projects/${projectId}/members`, {
      method: 'POST',
      body: JSON.stringify(dto),
    });
  },

  async updateMemberRole(projectId: string, memberUserId: string, dto: UpdateRoleDto): Promise<ProjectMember> {
    return fetchWithAuth<ProjectMember>(`/projects/${projectId}/members/${memberUserId}/role`, {
      method: 'PUT',
      body: JSON.stringify(dto),
    });
  },

  async removeProjectMember(projectId: string, memberUserId: string): Promise<void> {
    return fetchWithAuth<void>(`/projects/${projectId}/members/${memberUserId}`, {
      method: 'DELETE',
    });
  },

  // Review methods
  async getReviews(projectId: string): Promise<Review[]> {
    return fetchWithAuth<Review[]>(`/projects/${projectId}/reviews`, {
      method: 'GET',
    });
  },

  async getReview(projectId: string, reviewId: string): Promise<Review> {
    return fetchWithAuth<Review>(`/projects/${projectId}/reviews/${reviewId}`, {
      method: 'GET',
    });
  },

  async createReview(projectId: string, dto: CreateReviewDto): Promise<Review> {
    return fetchWithAuth<Review>(`/projects/${projectId}/reviews`, {
      method: 'POST',
      body: JSON.stringify(dto),
    });
  },

  async updateReview(projectId: string, reviewId: string, dto: UpdateReviewDto): Promise<Review> {
    return fetchWithAuth<Review>(`/projects/${projectId}/reviews/${reviewId}`, {
      method: 'PUT',
      body: JSON.stringify(dto),
    });
  },

  async deleteReview(projectId: string, reviewId: string): Promise<void> {
    return fetchWithAuth<void>(`/projects/${projectId}/reviews/${reviewId}`, {
      method: 'DELETE',
    });
  },

  async getReviewItems(projectId: string, reviewId: string): Promise<ReviewItem[]> {
    return fetchWithAuth<ReviewItem[]>(`/projects/${projectId}/reviews/${reviewId}/items`, {
      method: 'GET',
    });
  },

  async createReviewItem(projectId: string, reviewId: string, dto: CreateReviewItemDto): Promise<ReviewItem> {
    return fetchWithAuth<ReviewItem>(`/projects/${projectId}/reviews/${reviewId}/items`, {
      method: 'POST',
      body: JSON.stringify(dto),
    });
  },

  async updateReviewItem(projectId: string, reviewId: string, itemId: string, dto: UpdateReviewItemDto): Promise<ReviewItem> {
    return fetchWithAuth<ReviewItem>(`/projects/${projectId}/reviews/${reviewId}/items/${itemId}`, {
      method: 'PUT',
      body: JSON.stringify(dto),
    });
  },

  async deleteReviewItem(projectId: string, reviewId: string, itemId: string): Promise<void> {
    return fetchWithAuth<void>(`/projects/${projectId}/reviews/${reviewId}/items/${itemId}`, {
      method: 'DELETE',
    });
  },

  // Comment methods
  async getComments(projectId: string, reviewId: string, reviewItemId: string): Promise<Comment[]> {
    return fetchWithAuth<Comment[]>(`/projects/${projectId}/reviews/${reviewId}/items/${reviewItemId}/comments`, {
      method: 'GET',
    });
  },

  async createComment(projectId: string, reviewId: string, reviewItemId: string, dto: CreateCommentDto): Promise<Comment> {
    return fetchWithAuth<Comment>(`/projects/${projectId}/reviews/${reviewId}/items/${reviewItemId}/comments`, {
      method: 'POST',
      body: JSON.stringify(dto),
    });
  },

  async updateComment(projectId: string, reviewId: string, reviewItemId: string, commentId: string, dto: UpdateCommentDto): Promise<Comment> {
    return fetchWithAuth<Comment>(`/projects/${projectId}/reviews/${reviewId}/items/${reviewItemId}/comments/${commentId}`, {
      method: 'PUT',
      body: JSON.stringify(dto),
    });
  },

  async deleteComment(projectId: string, reviewId: string, reviewItemId: string, commentId: string): Promise<void> {
    return fetchWithAuth<void>(`/projects/${projectId}/reviews/${reviewId}/items/${reviewItemId}/comments/${commentId}`, {
      method: 'DELETE',
    });
  },

  // Source File methods
  async getSourceFiles(projectId: string): Promise<SourceFile[]> {
    return fetchWithAuth<SourceFile[]>(`/projects/${projectId}/source-files`, {
      method: 'GET',
    });
  },

  async getSourceFile(projectId: string, fileId: string): Promise<SourceFile> {
    return fetchWithAuth<SourceFile>(`/projects/${projectId}/source-files/${fileId}`, {
      method: 'GET',
    });
  },

  async createSourceFile(projectId: string, dto: CreateSourceFileDto): Promise<SourceFile> {
    return fetchWithAuth<SourceFile>(`/projects/${projectId}/source-files`, {
      method: 'POST',
      body: JSON.stringify({
        Path: dto.path,
        Content: dto.content,
        Message: dto.message,
      }),
    });
  },

  async deleteSourceFile(projectId: string, fileId: string): Promise<void> {
    return fetchWithAuth<void>(`/projects/${projectId}/source-files/${fileId}`, {
      method: 'DELETE',
    });
  },

  async getSourceFileVersions(projectId: string, fileId: string): Promise<SourceFileVersion[]> {
    return fetchWithAuth<SourceFileVersion[]>(`/projects/${projectId}/source-files/${fileId}/versions`, {
      method: 'GET',
    });
  },

  async getSourceFileVersion(projectId: string, fileId: string, versionId: string): Promise<SourceFileVersion> {
    return fetchWithAuth<SourceFileVersion>(`/projects/${projectId}/source-files/${fileId}/versions/${versionId}`, {
      method: 'GET',
    });
  },

  async createSourceFileVersion(projectId: string, fileId: string, dto: CreateSourceFileVersionDto): Promise<SourceFileVersion> {
    return fetchWithAuth<SourceFileVersion>(`/projects/${projectId}/source-files/${fileId}/versions`, {
      method: 'POST',
      body: JSON.stringify({
        Content: dto.content,
        Message: dto.message,
      }),
    });
  },

  // User methods
  async getUser(userId: string): Promise<User> {
    return fetchWithAuth<User>(`/users/${userId}`, {
      method: 'GET',
    });
  },

  async getUserByEmail(email: string): Promise<User> {
    return fetchWithAuth<User>(`/users/by-email/${encodeURIComponent(email)}`, {
      method: 'GET',
    });
  },

  // Language methods
  async getLanguage(languageId: string): Promise<Language> {
    return fetchWithAuth<Language>(`/languages/${languageId}`, {
      method: 'GET',
    });
  },

  async getLanguages(): Promise<Language[]> {
    return fetchWithAuth<Language[]>('/languages', {
      method: 'GET',
    });
  },
};

