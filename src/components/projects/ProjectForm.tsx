import { useState, useEffect } from 'react';
import { api } from '../../services/api';
import ReviewRulesEditor from './ReviewRulesEditor';
import './ProjectForm.css';

interface ProjectFormProps {
    userId: string;
    projectId?: string;
    onSave: () => void;
    onCancel: () => void;
}

export default function ProjectForm({ userId, projectId, onSave, onCancel }: ProjectFormProps) {
    const [name, setName] = useState('');
    const [description, setDescription] = useState('');
    const [visibility, setVisibility] = useState<string>('private');
    const [requiredReviewersRules, setRequiredReviewersRules] = useState<string>('');
    const [ownerId, setOwnerId] = useState<string>(userId);
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState<string | null>(null);

    useEffect(() => {
        if (projectId) {
            // Загрузка данных проекта для редактирования
            // TODO: Реализовать загрузку данных проекта
            loadProject(projectId);
        }
    }, [projectId]);

    const loadProject = async (id: string) => {
        setLoading(true);
        try {
            const project = await api.getProject(id);
            setName(project.name);
            setDescription(project.description || '');
            setVisibility(project.visibility || 'private');
            setRequiredReviewersRules(project.requiredReviewersRules || '');
            setOwnerId(project.ownerId);
        } catch (err) {
            setError(err instanceof Error ? err.message : 'Не удалось загрузить проект');
        } finally {
            setLoading(false);
        }
    };

    const handleSubmit = async (e: React.FormEvent) => {
        e.preventDefault();
        setError(null);
        setLoading(true);

        try {
            if (projectId) {
                await api.updateProject(projectId, {
                    name,
                    description: description.trim() || undefined,
                    ownerId,
                    visibility,
                    requiredReviewersRules: requiredReviewersRules || undefined
                });
            } else {
                await api.createProject({
                    name,
                    description: description.trim() || undefined,
                    ownerId: userId,
                    visibility,
                    requiredReviewersRules: requiredReviewersRules || undefined
                });
            }
            onSave();
        } catch (err) {
            setError(err instanceof Error ? err.message : 'Не удалось сохранить проект');
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="project-form-container">
            <h2>{projectId ? 'Редактирование проекта' : 'Создание проекта'}</h2>
            <form onSubmit={handleSubmit} className="project-form">
                {error && <div className="error-message">{error}</div>}

                <div className="form-group">
                    <label htmlFor="name">Название проекта *</label>
                    <input
                        type="text"
                        id="name"
                        value={name}
                        onChange={(e) => setName(e.target.value)}
                        required
                        placeholder="Введите название проекта"
                        disabled={loading}
                    />
                </div>

                <div className="form-group">
                    <label htmlFor="description">Описание</label>
                    <textarea
                        id="description"
                        value={description}
                        onChange={(e) => setDescription(e.target.value)}
                        placeholder="Введите описание проекта (необязательно)"
                        disabled={loading}
                        rows={5}
                    />
                    <small>Описание проекта поможет другим участникам понять его назначение</small>
                </div>

                <div className="form-group">
                    <label htmlFor="visibility">Видимость</label>
                    <select
                        id="visibility"
                        value={visibility}
                        onChange={(e) => setVisibility(e.target.value)}
                        disabled={loading}
                    >
                        <option value="private">Приватный</option>
                        <option value="public">Публичный</option>
                    </select>
                    <small>Приватный проект виден только участникам, публичный - всем пользователям</small>
                </div>

                <div className="form-group">
                    <label htmlFor="requiredReviewersRules">Правила ревью (необязательно)</label>
                    <ReviewRulesEditor
                        value={requiredReviewersRules}
                        onChange={setRequiredReviewersRules}
                        disabled={loading}
                    />
                </div>

                <div className="form-actions">
                    <button type="button" onClick={onCancel} disabled={loading} className="cancel-button">
                        Отмена
                    </button>
                    <button type="submit" disabled={loading} className="submit-button">
                        {loading ? 'Сохранение...' : (projectId ? 'Сохранить' : 'Создать')}
                    </button>
                </div>
            </form>
        </div>
    );
}

