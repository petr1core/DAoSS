import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { api } from '../services/api';
import ProjectList from '../components/projects/ProjectList';
import './ProjectsListPage.css';

export default function ProjectsListPage() {
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

  const handleProjectClick = (projectId: string) => {
    navigate(`/projects/${projectId}`);
  };

  const handleCreateProject = () => {
    navigate('/projects/new');
  };

  if (loading) {
    return (
      <div className="projects-list-page-container">
        <div className="loading">Загрузка...</div>
      </div>
    );
  }

  if (!userId) {
    return (
      <div className="projects-list-page-container">
        <div className="error">Не удалось загрузить информацию о пользователе</div>
      </div>
    );
  }

  return (
    <div className="projects-list-page-container">
      <ProjectList
        userId={userId}
        onProjectClick={handleProjectClick}
        onCreateProject={handleCreateProject}
      />
    </div>
  );
}

