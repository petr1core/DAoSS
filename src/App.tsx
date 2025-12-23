import { useState, useEffect } from 'react'
import { BrowserRouter, Routes, Route, Navigate, useLocation, useNavigate, Link } from 'react-router-dom'
import LoginPage from './components/LoginPage'
import UserPanel from './components/UserPanel'
import FlowchartEditor from './components/FlowchartEditorRefactored'
import HomePage from './components/HomePage'
import ProjectsListPage from './pages/ProjectsListPage'
import ProjectCreatePage from './pages/ProjectCreatePage'
import ProjectDetailsPage from './pages/ProjectDetailsPage'
import InvitationsPageWrapper from './pages/InvitationsPage'
import ProfilePage from './pages/ProfilePage'
import './App.css'
import { api } from './services/api'
import { getToken, removeToken } from './utils/auth'
import { initTheme, toggleTheme, getThemeToggleCooldown } from './utils/themeUtils'

// Компонент для защищенных маршрутов
function ProtectedRoute({ children, isAuthenticated }: { children: React.ReactNode; isAuthenticated: boolean }) {
  if (!isAuthenticated) {
    return <Navigate to="/" replace />
  }
  return <>{children}</>
}

// Компонент навигации
function Navigation({ username, isDark, onToggleTheme }: { username: string; isDark: boolean; onToggleTheme: () => void }) {
  const location = useLocation()
  const [themeCooldown, setThemeCooldown] = useState(0)

  useEffect(() => {
    if (themeCooldown > 0) {
      const timer = setTimeout(() => setThemeCooldown(themeCooldown - 1), 1000)
      return () => clearTimeout(timer)
    }
  }, [themeCooldown])

  const isActive = (path: string) => {
    if (path === '/') {
      return location.pathname === '/'
    }
    return location.pathname.startsWith(path)
  }

  const handleThemeToggle = () => {
    const cooldown = getThemeToggleCooldown()
    if (cooldown > 0) {
      setThemeCooldown(cooldown)
      return
    }
    onToggleTheme()
  }

  return (
    <nav className="main-navigation">
      <div className="nav-left">
        <Link
          to="/projects"
          className={`nav-button ${isActive('/projects') ? 'active' : ''}`}
        >
          Проекты
        </Link>
        <Link
          to="/invitations"
          className={`nav-button ${isActive('/invitations') ? 'active' : ''}`}
        >
          Приглашения
        </Link>
      </div>
      <div className="nav-right">
        <button
          className="theme-toggle-button"
          onClick={handleThemeToggle}
          title={themeCooldown > 0 ? `Подождите ${themeCooldown} сек.` : 'Переключить тему'}
          disabled={themeCooldown > 0}
        >
          {isDark ? (
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <circle cx="12" cy="12" r="5"></circle>
              <line x1="12" y1="1" x2="12" y2="3"></line>
              <line x1="12" y1="21" x2="12" y2="23"></line>
              <line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line>
              <line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line>
              <line x1="1" y1="12" x2="3" y2="12"></line>
              <line x1="21" y1="12" x2="23" y2="12"></line>
              <line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line>
              <line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line>
            </svg>
          ) : (
            <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path>
            </svg>
          )}
          {themeCooldown > 0 && <span className="cooldown-badge">{themeCooldown}</span>}
        </button>
        <span className="nav-username">{username}</span>
        <Link
          to="/profile"
          className={`nav-button ${isActive('/profile') ? 'active' : ''}`}
        >
          Личный кабинет
        </Link>
      </div>
    </nav>
  )
}

// Основной компонент приложения с роутингом
function AppContent() {
  const [isAuthenticated, setIsAuthenticated] = useState(false)
  const [username, setUsername] = useState('')
  const [isLoading, setIsLoading] = useState(true)
  const [isDarkTheme, setIsDarkTheme] = useState(false)
  const navigate = useNavigate()

  // Инициализация темы при загрузке
  useEffect(() => {
    const isDark = initTheme()
    setIsDarkTheme(isDark)
  }, [])

  const handleToggleTheme = () => {
    const result = toggleTheme()
    if (result.success) {
      setIsDarkTheme(result.isDark)
    }
  }

  useEffect(() => {
    const checkAuth = async () => {
      const token = getToken()

      if (!token) {
        setIsLoading(false)
        return
      }

      try {
        // Проверяем валидность токена с таймаутом
        // Оборачиваем validateToken в промис, который всегда разрешается
        const isValid = await Promise.race([
          api.validateToken().catch(() => false), // При ошибке возвращаем false
          new Promise<boolean>((resolve) =>
            setTimeout(() => resolve(false), 3000) // 3 секунды таймаут
          )
        ])

        if (isValid) {
          try {
            // Получаем информацию о пользователе
            const userInfo = await api.getMe()
            setUsername(userInfo.name || userInfo.email || 'Пользователь')
            setIsAuthenticated(true)
          } catch (error) {
            // Ошибка при получении информации о пользователе
            console.error('Failed to get user info:', error)
            removeToken()
            setIsAuthenticated(false)
          }
        } else {
          // Токен невалидный или истек таймаут, удаляем его
          removeToken()
          setIsAuthenticated(false)
        }
      } catch (error) {
        // Ошибка при проверке токена (например, Backend недоступен)
        console.warn('Auth check failed (Backend may be unavailable):', error)
        // Если Backend недоступен, просто считаем пользователя неавторизованным
        removeToken()
        setIsAuthenticated(false)
      } finally {
        setIsLoading(false)
      }
    }

    checkAuth()

    // Слушаем событие выхода из системы
    const handleLogoutEvent = () => {
      setIsAuthenticated(false)
      setUsername('')
    }

    window.addEventListener('auth:logout', handleLogoutEvent)

    return () => {
      window.removeEventListener('auth:logout', handleLogoutEvent)
    }
  }, [])

  const handleLogin = async (userName: string) => {
    setUsername(userName)
    setIsAuthenticated(true)
    try {
      const userInfo = await api.getMe()
      setUsername(userInfo.name || userInfo.email || 'Пользователь')
    } catch (err) {
      console.error('Failed to get user info:', err)
    }
    // Перенаправляем на страницу проектов после входа
    navigate('/projects')
  }

  const handleLogout = () => {
    removeToken()
    setIsAuthenticated(false)
    setUsername('')
    navigate('/')
  }

  if (isLoading) {
    return (
      <div style={{
        display: 'flex',
        justifyContent: 'center',
        alignItems: 'center',
        height: '100vh'
      }}>
        <p>Загрузка...</p>
      </div>
    )
  }

  return (
    <Routes>
      {/* Публичные маршруты */}
      <Route
        path="/"
        element={
          isAuthenticated ? (
            <Navigate to="/projects" replace />
          ) : (
            <HomePage />
          )
        }
      />
      <Route
        path="/login"
        element={
          isAuthenticated ? (
            <Navigate to="/projects" replace />
          ) : (
            <LoginPage onLogin={handleLogin} />
          )
        }
      />
      <Route
        path="/register"
        element={
          isAuthenticated ? (
            <Navigate to="/projects" replace />
          ) : (
            <Navigate to="/login?mode=register" replace />
          )
        }
      />

      {/* Защищенные маршруты */}
      <Route
        path="/projects"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} isDark={isDarkTheme} onToggleTheme={handleToggleTheme} />
            <ProjectsListPage />
          </ProtectedRoute>
        }
      />
      <Route
        path="/projects/new"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} isDark={isDarkTheme} onToggleTheme={handleToggleTheme} />
            <ProjectCreatePage />
          </ProtectedRoute>
        }
      />
      <Route
        path="/projects/:id"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} isDark={isDarkTheme} onToggleTheme={handleToggleTheme} />
            <ProjectDetailsPage />
          </ProtectedRoute>
        }
      />
      <Route
        path="/invitations"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} isDark={isDarkTheme} onToggleTheme={handleToggleTheme} />
            <InvitationsPageWrapper />
          </ProtectedRoute>
        }
      />
      <Route
        path="/profile"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} isDark={isDarkTheme} onToggleTheme={handleToggleTheme} />
            <ProfilePage />
          </ProtectedRoute>
        }
      />

      {/* Редактор блок-схем (если нужен отдельный маршрут) */}
      <Route
        path="/editor"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} isDark={isDarkTheme} onToggleTheme={handleToggleTheme} />
            <FlowchartEditor />
          </ProtectedRoute>
        }
      />

      {/* Редирект для неизвестных маршрутов */}
      <Route path="*" element={<Navigate to={isAuthenticated ? "/projects" : "/"} replace />} />
    </Routes>
  )
}

function App() {
  return (
    <BrowserRouter>
      <AppContent />
    </BrowserRouter>
  )
}

export default App
