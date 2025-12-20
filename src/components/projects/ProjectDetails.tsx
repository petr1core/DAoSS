import { useState, useEffect } from 'react';
import type { Project } from '../../services/api';
import { api } from '../../services/api';
import MembersList from './MembersList';
import InvitationsList from './InvitationsList';
import ReviewsList from './ReviewsList';
import SourceFilesList from './SourceFilesList';
import './ProjectDetails.css';

interface ProjectDetailsProps {
  projectId: string;
  userId: string;
  onEdit: () => void;
  onDelete: () => void;
  onBack: () => void;
}

type TabType = 'overview' | 'members' | 'invitations' | 'reviews' | 'files';

export default function ProjectDetails({
  projectId,
  userId,
  onEdit,
  onDelete,
  onBack
}: ProjectDetailsProps) {
  const [project, setProject] = useState<Project | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [activeTab, setActiveTab] = useState<TabType>('overview');
  const [userRole, setUserRole] = useState<string | null>(null);

  useEffect(() => {
    loadProject();
    loadUserRole();
  }, [projectId, userId]);

  const loadProject = async () => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.getProject(projectId);
      setProject(data);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось загрузить проект');
    } finally {
      setLoading(false);
    }
  };

  const loadUserRole = async () => {
    try {
      const member = await api.getProjectMember(projectId, userId);
      setUserRole(member.role);
    } catch {
      // Пользователь не является участником или произошла ошибка
      setUserRole(null);
    }
  };

  const handleDelete = async () => {
    if (!confirm('Вы уверены, что хотите удалить этот проект? Это действие нельзя отменить.')) {
      return;
    }

    try {
      await api.deleteProject(projectId);
      onDelete();
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось удалить проект');
    }
  };

  const canEdit = userRole === 'owner' || userRole === 'admin';
  const canDelete = userRole === 'owner';

  if (loading) {
    return (
      <div className="project-details-container">
        <div className="loading">Загрузка проекта...</div>
      </div>
    );
  }

  if (error || !project) {
    return (
      <div className="project-details-container">
        <div className="error">{error || 'Проект не найден'}</div>
        <button onClick={onBack} className="back-button">Назад</button>
      </div>
    );
  }

  const formatDate = (dateString: string) => {
    const date = new Date(dateString);
    return date.toLocaleDateString('ru-RU', {
      year: 'numeric',
      month: 'long',
      day: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
    });
  };

  return (
    <div className="project-details-container">
      <div className="project-details-header">
        <button onClick={onBack} className="back-button">← Назад</button>
        <div className="project-actions">
          {canEdit && (
            <button onClick={onEdit} className="edit-button">
              Редактировать
            </button>
          )}
          {canDelete && (
            <button onClick={handleDelete} className="delete-button">
              Удалить
            </button>
          )}
        </div>
      </div>

      <div className="project-info">
        <h1>{project.name}</h1>
        <p className="project-description">{project.description}</p>
        <div className="project-meta">
          <span className="meta-item">
            <strong>Видимость:</strong> {project.visibility === 'private' ? 'Приватный' : 'Публичный'}
          </span>
          <span className="meta-item">
            <strong>Создан:</strong> {formatDate(project.createdAt)}
          </span>
        </div>
      </div>

      <div className="project-tabs">
        <button
          className={`tab ${activeTab === 'overview' ? 'active' : ''}`}
          onClick={() => setActiveTab('overview')}
        >
          Обзор
        </button>
        <button
          className={`tab ${activeTab === 'members' ? 'active' : ''}`}
          onClick={() => setActiveTab('members')}
        >
          Участники
        </button>
        <button
          className={`tab ${activeTab === 'invitations' ? 'active' : ''}`}
          onClick={() => setActiveTab('invitations')}
        >
          Приглашения
        </button>
        <button
          className={`tab ${activeTab === 'reviews' ? 'active' : ''}`}
          onClick={() => setActiveTab('reviews')}
        >
          Ревью
        </button>
        <button
          className={`tab ${activeTab === 'files' ? 'active' : ''}`}
          onClick={() => setActiveTab('files')}
        >
          Файлы
        </button>
      </div>

      <div className="project-tab-content">
        {activeTab === 'overview' && (
          <div className="overview-tab">
            <h2>Информация о проекте</h2>
            <div className="info-section">
              <h3>Основная информация</h3>
              <p><strong>ID проекта:</strong> {project.id}</p>
              <p><strong>Владелец:</strong> {project.ownerId}</p>
              <p><strong>Язык по умолчанию:</strong> {project.defaultLanguageId}</p>
            </div>
            {project.requiredReviewersRules && (
              <div className="info-section">
                <h3>Правила ревью</h3>
                <pre className="json-preview">{project.requiredReviewersRules}</pre>
              </div>
            )}
          </div>
        )}

        {activeTab === 'members' && (
          <MembersList
            projectId={projectId}
            userId={userId}
            userRole={userRole}
            onUpdate={loadUserRole}
          />
        )}

        {activeTab === 'invitations' && (
          <InvitationsList
            projectId={projectId}
            canManage={canEdit}
          />
        )}

        {activeTab === 'reviews' && (
          <ReviewsList
            projectId={projectId}
            canManage={userRole === 'owner' || userRole === 'admin' || userRole === 'reviewer'}
          />
        )}

        {activeTab === 'files' && (
          <SourceFilesList
            projectId={projectId}
            canManage={canEdit}
          />
        )}
      </div>
    </div>
  );
}


