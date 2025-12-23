import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { api } from '../services/api';
import { removeToken } from '../utils/auth';
import './ProfilePage.css';

export default function ProfilePage() {
  const navigate = useNavigate();
  const [userInfo, setUserInfo] = useState<{ name: string; email: string } | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadUserInfo();
  }, []);

  const loadUserInfo = async () => {
    try {
      const info = await api.getMe();
      setUserInfo({
        name: info.name || info.email || 'Пользователь',
        email: info.email,
      });
    } catch (err) {
      console.error('Failed to load user info:', err);
    } finally {
      setLoading(false);
    }
  };

  const handleLogout = () => {
    removeToken();
    window.dispatchEvent(new CustomEvent('auth:logout'));
    navigate('/');
  };

  if (loading) {
    return (
      <div className="profile-page-container">
        <div className="loading">Загрузка...</div>
      </div>
    );
  }

  return (
    <div className="profile-page-container">
      <div className="profile-content">
        <h1>Личный кабинет</h1>
        
        <div className="profile-info">
          <div className="info-section">
            <h2>Информация о пользователе</h2>
            <div className="info-item">
              <strong>Имя:</strong> {userInfo?.name || 'Не указано'}
            </div>
            <div className="info-item">
              <strong>Email:</strong> {userInfo?.email || 'Не указан'}
            </div>
          </div>
        </div>

        <div className="profile-actions">
          <button onClick={handleLogout} className="logout-button">
            Выйти
          </button>
        </div>
      </div>
    </div>
  );
}

