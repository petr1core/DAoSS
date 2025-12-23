import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { api } from '../services/api';
import ProjectForm from '../components/projects/ProjectForm';
import './ProjectCreatePage.css';

export default function ProjectCreatePage() {
  const [userId, setUserId] = useState<string>('');
  const [loading, setLoading] = useState(true);
  const navigate = useNavigate();

  useEffect(() => {
    loadUserInfo();
  }, []);

  const loadUserInfo = async () => {
    try {
      const userInfo = await api.getMe();
      setUserId(userInfo.sub);
    } catch (err) {
      console.error('Failed to load user info:', err);
    } finally {
      setLoading(false);
    }
  };

  const handleSave = () => {
    navigate('/projects');
  };

  const handleCancel = () => {
    navigate('/projects');
  };

  if (loading) {
    return (
      <div className="project-create-page-container">
        <div className="loading">Загрузка...</div>
      </div>
    );
  }

  if (!userId) {
    return (
      <div className="project-create-page-container">
        <div className="error">Не удалось загрузить информацию о пользователе</div>
      </div>
    );
  }

  return (
    <div className="project-create-page-container">
      <ProjectForm
        userId={userId}
        onSave={handleSave}
        onCancel={handleCancel}
      />
    </div>
  );
}

