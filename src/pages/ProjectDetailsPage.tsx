import { useParams, useNavigate } from 'react-router-dom';
import { useState, useEffect } from 'react';
import { api } from '../services/api';
import ProjectDetails from '../components/projects/ProjectDetails';
import './ProjectDetailsPage.css';

export default function ProjectDetailsPage() {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const [userId, setUserId] = useState<string>('');
  const [loading, setLoading] = useState(true);

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

  const handleEdit = () => {
    // Можно добавить отдельную страницу редактирования или модальное окно
    // Пока оставляем пустым, так как редактирование может быть реализовано внутри ProjectDetails
  };

  const handleDelete = () => {
    navigate('/projects');
  };

  const handleBack = () => {
    navigate('/projects');
  };

  if (loading) {
    return (
      <div className="project-details-page-container">
        <div className="loading">Загрузка...</div>
      </div>
    );
  }

  if (!userId || !id) {
    return (
      <div className="project-details-page-container">
        <div className="error">Не удалось загрузить информацию</div>
      </div>
    );
  }

  return (
    <div className="project-details-page-container">
      <ProjectDetails
        projectId={id}
        userId={userId}
        onEdit={handleEdit}
        onDelete={handleDelete}
        onBack={handleBack}
      />
    </div>
  );
}

