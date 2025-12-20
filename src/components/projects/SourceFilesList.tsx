import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import type { SourceFile, CreateSourceFileDto } from '../../services/api';
import { api } from '../../services/api';
import SourceFileForm from './SourceFileForm';
import './SourceFilesList.css';

interface SourceFilesListProps {
  projectId: string;
  canManage: boolean;
}

export default function SourceFilesList({ projectId, canManage }: SourceFilesListProps) {
  const [files, setFiles] = useState<SourceFile[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [showAddForm, setShowAddForm] = useState(false);
  const navigate = useNavigate();

  useEffect(() => {
    loadFiles();
  }, [projectId]);

  const loadFiles = async () => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.getSourceFiles(projectId);
      setFiles(data);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось загрузить файлы');
    } finally {
      setLoading(false);
    }
  };

  const handleAddFile = async (dto: CreateSourceFileDto) => {
    try {
      await api.createSourceFile(projectId, dto);
      setShowAddForm(false);
      loadFiles();
    } catch (err) {
      throw err;
    }
  };

  const handleDownload = async (fileId: string, fileName: string) => {
    try {
      // Получаем версии файла
      const versions = await api.getSourceFileVersions(projectId, fileId);
      if (versions.length === 0) {
        alert('Файл не имеет версий');
        return;
      }

      // Находим последнюю версию (с максимальным versionIndex)
      const latestVersion = versions.reduce((latest, current) => 
        current.versionIndex > latest.versionIndex ? current : latest
      );

      // Получаем содержимое последней версии
      const version = await api.getSourceFileVersion(projectId, fileId, latestVersion.id);

      // Создаем Blob и скачиваем
      const blob = new Blob([version.content], { type: 'text/plain' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = fileName;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось скачать файл');
    }
  };

  const handleViewDiagram = (fileId: string) => {
    navigate(`/editor?projectId=${projectId}&fileId=${fileId}`);
  };

  const handleDelete = async (fileId: string) => {
    if (!confirm('Удалить файл? Это действие нельзя отменить.')) {
      return;
    }

    try {
      await api.deleteSourceFile(projectId, fileId);
      loadFiles();
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось удалить файл');
    }
  };

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

  const getFileName = (path: string) => {
    return path.split('/').pop() || path;
  };

  if (loading) {
    return <div className="source-files-list-container">Загрузка файлов...</div>;
  }

  if (error) {
    return (
      <div className="source-files-list-container">
        <div className="error">{error}</div>
        <button onClick={loadFiles} className="retry-button">Повторить</button>
      </div>
    );
  }

  return (
    <div className="source-files-list-container">
      <div className="section-header">
        <h2>Файлы проекта</h2>
        {canManage && (
          <button onClick={() => setShowAddForm(!showAddForm)} className="add-button">
            {showAddForm ? 'Отмена' : '+ Добавить файл'}
          </button>
        )}
      </div>

      {showAddForm && canManage && (
        <SourceFileForm
          onSubmit={handleAddFile}
          onCancel={() => setShowAddForm(false)}
        />
      )}

      {files.length === 0 ? (
        <div className="empty-state">
          <p>В проекте пока нет файлов</p>
          {canManage && (
            <button onClick={() => setShowAddForm(true)} className="add-button">
              Добавить первый файл
            </button>
          )}
        </div>
      ) : (
        <div className="source-files-list">
          {files.map((file) => (
            <div key={file.id} className="source-file-card">
              <div className="file-info">
                <h3 className="file-name">{getFileName(file.path)}</h3>
                <p className="file-path">{file.path}</p>
                <p className="file-meta">
                  <span>Создан: {formatDate(file.createdAt)}</span>
                </p>
              </div>
              <div className="file-actions">
                <button
                  onClick={() => handleViewDiagram(file.id)}
                  className="action-button view-button"
                  title="Просмотр диаграммы"
                >
                  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>
                    <circle cx="12" cy="12" r="3"></circle>
                  </svg>
                  Просмотр
                </button>
                <button
                  onClick={() => handleDownload(file.id, getFileName(file.path))}
                  className="action-button download-button"
                  title="Скачать файл"
                >
                  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path>
                    <polyline points="7 10 12 15 17 10"></polyline>
                    <line x1="12" y1="15" x2="12" y2="3"></line>
                  </svg>
                  Скачать
                </button>
                {canManage && (
                  <button
                    onClick={() => handleDelete(file.id)}
                    className="action-button delete-button"
                    title="Удалить файл"
                  >
                    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <polyline points="3 6 5 6 21 6"></polyline>
                      <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
                    </svg>
                    Удалить
                  </button>
                )}
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}

