import { useState, useEffect } from 'react'
import { BrowserRouter, Routes, Route, Navigate, useLocation, useNavigate, Link } from 'react-router-dom'
import LoginPage from './components/LoginPage'
import UserPanel from './components/UserPanel'
import FlowchartEditor from './components/FlowchartEditorRefactored'
import './App.css'
import { api } from './services/api'
import { getToken, removeToken } from './utils/auth'

// Компонент для защищенных маршрутов
function ProtectedRoute({ children, isAuthenticated }: { children: React.ReactNode; isAuthenticated: boolean }) {
  if (!isAuthenticated) {
    return <Navigate to="/" replace />
  }
  return <>{children}</>
}

// Компонент навигации
function Navigation({ username }: { username: string }) {
  const location = useLocation()

  const isActive = (path: string) => {
    if (path === '/') {
      return location.pathname === '/'
    }
    return location.pathname.startsWith(path)
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
  const navigate = useNavigate()

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
            <Navigation username={username} />
            <ProjectsListPage />
          </ProtectedRoute>
        }
      />
      <Route
        path="/projects/new"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} />
            <ProjectCreatePage />
          </ProtectedRoute>
        }
      />
      <Route
        path="/projects/:id"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} />
            <ProjectDetailsPage />
          </ProtectedRoute>
        }
      />
      <Route
        path="/invitations"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} />
            <InvitationsPageWrapper />
          </ProtectedRoute>
        }
      />
      <Route
        path="/profile"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} />
            <ProfilePage />
          </ProtectedRoute>
        }
      />

      {/* Редактор блок-схем (если нужен отдельный маршрут) */}
      <Route
        path="/editor"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} />
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
