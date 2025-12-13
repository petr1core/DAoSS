import { useState, useEffect } from 'react'
import LoginPage from './components/LoginPage'
import UserPanel from './components/UserPanel'
import FlowchartEditor from './components/FlowchartEditor'
import './App.css'
import { api } from './services/api'
import { getToken, removeToken } from './utils/auth'

function App() {
  const [isAuthenticated, setIsAuthenticated] = useState(false)
  const [username, setUsername] = useState('')
  const [isLoading, setIsLoading] = useState(true)

  useEffect(() => {
    const checkAuth = async () => {
      const token = getToken()
      
      if (!token) {
        setIsLoading(false)
        return
      }

      try {
        // Проверяем валидность токена
        const isValid = await api.validateToken()
        
        if (isValid) {
          // Получаем информацию о пользователе
          const userInfo = await api.getMe()
          setUsername(userInfo.name || userInfo.email || 'Пользователь')
          setIsAuthenticated(true)
        } else {
          // Токен невалидный, удаляем его
          removeToken()
          setIsAuthenticated(false)
        }
      } catch (error) {
        // Ошибка при проверке токена, удаляем его
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

  const handleLogin = (userName: string) => {
    setUsername(userName)
    setIsAuthenticated(true)
  }

  const handleLogout = () => {
    removeToken()
    setIsAuthenticated(false)
    setUsername('')
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

  if (!isAuthenticated) {
    return <LoginPage onLogin={handleLogin} />
  }

  return (
    <>
      <UserPanel username={username} status="пользователь" onLogout={handleLogout} />
      <FlowchartEditor />
    </>
  )
}

export default App
