import { useState, useEffect } from 'react';
import { useSearchParams } from 'react-router-dom';
import './LoginPage.css';
import { api } from '../services/api';
import { setToken } from '../utils/auth';

interface LoginPageProps {
  onLogin: (username: string) => void;
}

function LoginPage({ onLogin }: LoginPageProps) {
  const [searchParams] = useSearchParams();
  const [isRegisterMode, setIsRegisterMode] = useState(false);
  const [username, setUsername] = useState('');
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [name, setName] = useState('');
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    const mode = searchParams.get('mode');
    if (mode === 'register') {
      setIsRegisterMode(true);
    }
  }, [searchParams]);

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setLoading(true);

    try {
      const token = await api.login(username, password);
      setToken(token);

      // Получаем информацию о пользователе
      const userInfo = await api.getMe();
      onLogin(userInfo.name || username);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Произошла ошибка при входе');
    } finally {
      setLoading(false);
    }
  };

  const handleRegister = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setLoading(true);

    try {
      const token = await api.register(email, password, name || undefined, username || undefined);
      setToken(token);

      // Получаем информацию о пользователе
      const userInfo = await api.getMe();
      onLogin(userInfo.name || name || email);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Произошла ошибка при регистрации');
    } finally {
      setLoading(false);
    }
  };

  const handleSubmit = isRegisterMode ? handleRegister : handleLogin;

  return (
    <div className="login-page">
      <div className="login-container">
        <h1 className="login-title">
          {isRegisterMode ? 'Регистрация' : 'Авторизация'}
        </h1>
        {error && (
          <div className="error-message" style={{
            color: '#ef4444',
            marginBottom: '1rem',
            padding: '0.75rem',
            backgroundColor: '#fee2e2',
            borderRadius: '0.5rem',
            fontSize: '0.875rem'
          }}>
            {error}
          </div>
        )}
        <form onSubmit={handleSubmit} className="login-form">
          {isRegisterMode && (
            <div className="form-group">
              <label htmlFor="email">Email</label>
              <input
                type="email"
                id="email"
                value={email}
                onChange={(e) => setEmail(e.target.value)}
                placeholder="Введите email"
                required
                autoFocus
              />
            </div>
          )}
          {isRegisterMode && (
            <div className="form-group">
              <label htmlFor="name">Имя</label>
              <input
                type="text"
                id="name"
                value={name}
                onChange={(e) => setName(e.target.value)}
                placeholder="Введите имя (необязательно)"
              />
            </div>
          )}
          <div className="form-group">
            <label htmlFor="username">
              {isRegisterMode ? 'Логин (необязательно)' : 'Имя пользователя или Email'}
            </label>
            <input
              type="text"
              id="username"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              placeholder={isRegisterMode ? 'Введите логин (необязательно)' : 'Введите имя пользователя или email'}
              required={!isRegisterMode}
              autoFocus={!isRegisterMode}
            />
          </div>
          <div className="form-group">
            <label htmlFor="password">Пароль</label>
            <input
              type="password"
              id="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              placeholder="Введите пароль"
              required
            />
          </div>
          <button type="submit" className="login-button" disabled={loading}>
            {loading ? 'Загрузка...' : (isRegisterMode ? 'Зарегистрироваться' : 'Войти')}
          </button>
        </form>
        <div style={{ marginTop: '1rem', textAlign: 'center' }}>
          <button
            type="button"
            onClick={() => {
              setIsRegisterMode(!isRegisterMode);
              setError('');
              setUsername('');
              setEmail('');
              setPassword('');
              setName('');
            }}
            style={{
              background: 'none',
              border: 'none',
              color: '#3b82f6',
              cursor: 'pointer',
              textDecoration: 'underline',
              fontSize: '0.875rem'
            }}
          >
            {isRegisterMode ? 'Уже есть аккаунт? Войти' : 'Нет аккаунта? Зарегистрироваться'}
          </button>
        </div>
      </div>
    </div>
  );
}

export default LoginPage;



