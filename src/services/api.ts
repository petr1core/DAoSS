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
  
  return response.json();
}

async function fetchWithAuth<T>(url: string, options: RequestInit = {}): Promise<T> {
  const token = getToken();
  
  const headers: HeadersInit = {
    'Content-Type': 'application/json',
    ...options.headers,
  };
  
  if (token) {
    headers['Authorization'] = `Bearer ${token}`;
  }
  
  try {
    const response = await fetch(`${API_BASE_URL}${url}`, {
      ...options,
      headers,
    });
    
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
    throw error;
  }
}

async function fetchWithoutAuth<T>(url: string, options: RequestInit = {}): Promise<T> {
  const headers: HeadersInit = {
    'Content-Type': 'application/json',
    ...options.headers,
  };
  
  try {
    const response = await fetch(`${API_BASE_URL}${url}`, {
      ...options,
      headers,
    });
    
    return handleResponse<T>(response);
  } catch (error) {
    if (error instanceof TypeError && error.message.includes('fetch')) {
      throw new Error('Не удалось подключиться к серверу');
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
};

