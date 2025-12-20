import { useState, useEffect } from 'react'
import { BrowserRouter, Routes, Route, Navigate, useLocation, useNavigate, Link } from 'react-router-dom'
import LoginPage from './components/LoginPage'
import HomePage from './components/HomePage'
import UserPanel from './components/UserPanel'
import FlowchartEditor from './components/FlowchartEditor'
import ProjectsListPage from './pages/ProjectsListPage'
import ProjectCreatePage from './pages/ProjectCreatePage'
import ProjectDetailsPage from './pages/ProjectDetailsPage'
import InvitationsPageWrapper from './pages/InvitationsPage'
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
function Navigation({ username, onLogout }: { username: string; onLogout: () => void }) {
  const location = useLocation()

  const isActive = (path: string) => {
    if (path === '/') {
      return location.pathname === '/'
    }
    return location.pathname.startsWith(path)
  }

  return (
    <>
      <UserPanel username={username} status="пользователь" onLogout={onLogout} />
      <nav className="main-navigation">
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
      </nav>
    </>
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
            <Navigation username={username} onLogout={handleLogout} />
            <ProjectsListPage />
          </ProtectedRoute>
        }
      />
      <Route
        path="/projects/new"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} onLogout={handleLogout} />
            <ProjectCreatePage />
          </ProtectedRoute>
        }
      />
      <Route
        path="/projects/:id"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} onLogout={handleLogout} />
            <ProjectDetailsPage />
          </ProtectedRoute>
        }
      />
      <Route
        path="/invitations"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} onLogout={handleLogout} />
            <InvitationsPageWrapper />
          </ProtectedRoute>
        }
      />

      {/* Редактор блок-схем (если нужен отдельный маршрут) */}
      <Route
        path="/editor"
        element={
          <ProtectedRoute isAuthenticated={isAuthenticated}>
            <Navigation username={username} onLogout={handleLogout} />
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
