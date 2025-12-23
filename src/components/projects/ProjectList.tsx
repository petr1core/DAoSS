import { useState, useEffect } from 'react';
import type { Project } from '../../services/api';
import { api } from '../../services/api';
import ProjectCard from './ProjectCard';
import './ProjectList.css';

interface ProjectListProps {
  userId: string;
  onProjectClick: (projectId: string) => void;
  onCreateProject: () => void;
}

export default function ProjectList({ userId, onProjectClick, onCreateProject }: ProjectListProps) {
  const [projects, setProjects] = useState<Project[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    loadProjects();
  }, [userId]);

  const loadProjects = async () => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.getProjects(userId);
      setProjects(data);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось загрузить проекты');
    } finally {
      setLoading(false);
    }
  };

  if (loading) {
    return (
      <div className="project-list-container">
        <div className="loading">Загрузка проектов...</div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="project-list-container">
        <div className="error">{error}</div>
        <button onClick={loadProjects} className="retry-button">Повторить</button>
      </div>
    );
  }

  return (
    <div className="project-list-container">
      <div className="project-list-header">
        <h1>Мои проекты</h1>
        <button onClick={onCreateProject} className="create-project-button">
          + Создать проект
        </button>
      </div>

      {projects.length === 0 ? (
        <div className="empty-state">
          <p>У вас пока нет проектов</p>
          <button onClick={onCreateProject} className="create-project-button">
            Создать первый проект
          </button>
        </div>
      ) : (
        <div className="project-list">
          {projects.map((project) => (
            <ProjectCard
              key={project.id}
              project={project}
              onClick={() => onProjectClick(project.id)}
            />
          ))}
        </div>
      )}
    </div>
  );
}

