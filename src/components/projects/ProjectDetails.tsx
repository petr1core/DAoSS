import { useState, useEffect } from 'react';
import { useSearchParams } from 'react-router-dom';
import type { Project, UpdateProjectDto } from '../../services/api';
import { api } from '../../services/api';
import MembersList from './MembersList';
import InvitationsList from './InvitationsList';
import ReviewsList from './ReviewsList';
import SourceFilesList from './SourceFilesList';
import ConfirmDialog from '../ui/ConfirmDialog';
import ReviewRulesEditor from './ReviewRulesEditor';
import ReviewRulesDisplay from './ReviewRulesDisplay';
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
  const [searchParams, setSearchParams] = useSearchParams();
  const tabFromUrl = searchParams.get('tab') as TabType | null;
  const [project, setProject] = useState<Project | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [activeTab, setActiveTab] = useState<TabType>(tabFromUrl && ['overview', 'members', 'invitations', 'reviews', 'files'].includes(tabFromUrl) ? tabFromUrl : 'overview');
  const [userRole, setUserRole] = useState<string | null>(null);
  const [deleteConfirm, setDeleteConfirm] = useState(false);
  const [ownerEmail, setOwnerEmail] = useState<string>('');
  const [languageName, setLanguageName] = useState<string>('');
  const [isEditing, setIsEditing] = useState(false);
  const [editName, setEditName] = useState('');
  const [editDescription, setEditDescription] = useState('');
  const [editVisibility, setEditVisibility] = useState('');
  const [editRequiredReviewersRules, setEditRequiredReviewersRules] = useState('');
  const [editLoading, setEditLoading] = useState(false);

  useEffect(() => {
    loadProject();
    loadUserRole();
  }, [projectId, userId]);

  useEffect(() => {
    if (project) {
      loadOwnerEmail();
      loadLanguageName();
      setEditName(project.name);
      setEditDescription(project.description || '');
      setEditVisibility(project.visibility);
      setEditRequiredReviewersRules(project.requiredReviewersRules || '');
    }
  }, [project]);

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

  const loadOwnerEmail = async () => {
    if (!project?.ownerId) return;
    try {
      // Пытаемся получить email через участников проекта
      const members = await api.getProjectMembers(projectId);
      const ownerMember = members.find(m => m.userId === project.ownerId);
      if (ownerMember?.email) {
        setOwnerEmail(ownerMember.email);
        return;
      }

      // Если не нашли в участниках, пытаемся через API пользователя
      try {
        const user = await api.getUser(project.ownerId);
        setOwnerEmail(user.email);
      } catch {
        // Если текущий пользователь - владелец, используем его email
        if (project.ownerId === userId) {
          try {
            const me = await api.getMe();
            setOwnerEmail(me.email);
            return;
          } catch {
            // Fallback to ID
          }
        }
        setOwnerEmail(project.ownerId); // Fallback to ID if user not found
      }
    } catch {
      // Если текущий пользователь - владелец, используем его email
      if (project.ownerId === userId) {
        try {
          const me = await api.getMe();
          setOwnerEmail(me.email);
          return;
        } catch {
          // Fallback to ID
        }
      }
      setOwnerEmail(project.ownerId); // Fallback to ID if user not found
    }
  };

  const loadLanguageName = async () => {
    if (!project?.defaultLanguageId || project.defaultLanguageId === '00000000-0000-0000-0000-000000000000') {
      setLanguageName('Не указан');
      return;
    }
    try {
      const language = await api.getLanguage(project.defaultLanguageId);
      setLanguageName(language.name);
    } catch {
      setLanguageName('Неизвестный язык');
    }
  };

  const handleDeleteClick = () => {
    setDeleteConfirm(true);
  };

  const handleDeleteConfirm = async () => {
    try {
      await api.deleteProject(projectId);
      setDeleteConfirm(false);
      onDelete();
    } catch (err) {
      setDeleteConfirm(false);
      // Показываем ошибку через alert, так как это критическая ошибка
      alert(err instanceof Error ? err.message : 'Не удалось удалить проект');
    }
  };

  const handleEditClick = () => {
    setIsEditing(true);
  };

  const handleEditCancel = () => {
    setIsEditing(false);
    if (project) {
      setEditName(project.name);
      setEditDescription(project.description || '');
      setEditVisibility(project.visibility);
      setEditRequiredReviewersRules(project.requiredReviewersRules || '');
    }
  };

  const handleEditSave = async () => {
    if (!project) return;

    setEditLoading(true);
    try {
      const dto: UpdateProjectDto = {
        name: editName,
        description: editDescription.trim() || undefined,
        ownerId: project.ownerId,
        visibility: editVisibility,
        defaultLanguageId: project.defaultLanguageId,
        requiredReviewersRules: editRequiredReviewersRules || undefined,
      };
      await api.updateProject(projectId, dto);
      setIsEditing(false);
      loadProject();
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось обновить проект');
    } finally {
      setEditLoading(false);
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
          {canEdit && !isEditing && (
            <button onClick={handleEditClick} className="edit-button">
              Редактировать
            </button>
          )}
          {canDelete && (
            <button onClick={handleDeleteClick} className="delete-button">
              Удалить
            </button>
          )}
        </div>
      </div>

      <div className="project-info">
        {isEditing ? (
          <div className="edit-form">
            <div className="form-group">
              <label htmlFor="edit-name">Название проекта *</label>
              <input
                id="edit-name"
                type="text"
                value={editName}
                onChange={(e) => setEditName(e.target.value)}
                disabled={editLoading}
                required
              />
            </div>
            <div className="form-group">
              <label htmlFor="edit-description">Описание</label>
              <textarea
                id="edit-description"
                value={editDescription}
                onChange={(e) => setEditDescription(e.target.value)}
                disabled={editLoading}
                rows={3}
              />
            </div>
            <div className="form-group">
              <label htmlFor="edit-visibility">Видимость *</label>
              <select
                id="edit-visibility"
                value={editVisibility}
                onChange={(e) => setEditVisibility(e.target.value)}
                disabled={editLoading}
              >
                <option value="private">Приватный</option>
                <option value="public">Публичный</option>
              </select>
            </div>
            <div className="form-group">
              <label htmlFor="edit-rules">Правила ревью (необязательно)</label>
              <ReviewRulesEditor
                value={editRequiredReviewersRules}
                onChange={setEditRequiredReviewersRules}
                disabled={editLoading}
              />
            </div>
            <div className="form-actions">
              <button
                type="button"
                onClick={handleEditCancel}
                disabled={editLoading}
                className="cancel-button"
              >
                Отмена
              </button>
              <button
                type="button"
                onClick={handleEditSave}
                disabled={editLoading || !editName.trim()}
                className="submit-button"
              >
                {editLoading ? 'Сохранение...' : 'Сохранить'}
              </button>
            </div>
          </div>
        ) : (
          <>
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
          </>
        )}
      </div>

      <div className="project-tabs">
        <button
          className={`tab ${activeTab === 'overview' ? 'active' : ''}`}
          onClick={() => {
            setActiveTab('overview');
            setSearchParams({ tab: 'overview' });
          }}
        >
          Обзор
        </button>
        <button
          className={`tab ${activeTab === 'members' ? 'active' : ''}`}
          onClick={() => {
            setActiveTab('members');
            setSearchParams({ tab: 'members' });
          }}
        >
          Участники
        </button>
        <button
          className={`tab ${activeTab === 'invitations' ? 'active' : ''}`}
          onClick={() => {
            setActiveTab('invitations');
            setSearchParams({ tab: 'invitations' });
          }}
        >
          Приглашения
        </button>
        <button
          className={`tab ${activeTab === 'reviews' ? 'active' : ''}`}
          onClick={() => {
            setActiveTab('reviews');
            setSearchParams({ tab: 'reviews' });
          }}
        >
          Ревью
        </button>
        <button
          className={`tab ${activeTab === 'files' ? 'active' : ''}`}
          onClick={() => {
            setActiveTab('files');
            setSearchParams({ tab: 'files' });
          }}
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
              <p><strong>Владелец:</strong> {ownerEmail || project.ownerId}</p>
              <p><strong>Язык по умолчанию:</strong> {languageName}</p>
            </div>
            {project.requiredReviewersRules && (
              <div className="info-section">
                <h3>Правила ревью</h3>
                <ReviewRulesDisplay value={project.requiredReviewersRules} />
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

      <ConfirmDialog
        isOpen={deleteConfirm}
        title="Удаление проекта"
        message="Вы уверены, что хотите удалить этот проект? Это действие нельзя отменить."
        onConfirm={handleDeleteConfirm}
        onCancel={() => setDeleteConfirm(false)}
        confirmText="Удалить"
        cancelText="Отмена"
        confirmButtonClass="delete-button"
      />
    </div>
  );
}


