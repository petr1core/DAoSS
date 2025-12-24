import { useState, useEffect } from 'react';
import type { Review, UpdateReviewDto, ReviewItem, CreateReviewItemDto, UpdateReviewItemDto } from '../../services/api';
import { api } from '../../services/api';
import { parseAnchorRef } from '../../services/editorReviewService';
import ReviewItemEditor from './ReviewItemEditor';
import './ReviewDetails.css';

interface ReviewDetailsProps {
  projectId: string;
  reviewId: string;
  canManage: boolean;
  onBack: () => void;
  onUpdate: () => void;
}

export default function ReviewDetails({ projectId, reviewId, canManage, onBack, onUpdate }: ReviewDetailsProps) {
  const [review, setReview] = useState<Review | null>(null);
  const [items, setItems] = useState<ReviewItem[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [showAddItem, setShowAddItem] = useState(false);
  const [selectedItem, setSelectedItem] = useState<string | null>(null);
  const [targetFileName, setTargetFileName] = useState<string>('');
  const [showMenu, setShowMenu] = useState(false);

  useEffect(() => {
    loadReview();
    loadItems();
  }, [projectId, reviewId]);

  // Закрываем меню при клике вне его
  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      const target = event.target as HTMLElement;
      if (!target.closest('.menu-container')) {
        setShowMenu(false);
      }
    };

    if (showMenu) {
      document.addEventListener('mousedown', handleClickOutside);
      return () => {
        document.removeEventListener('mousedown', handleClickOutside);
      };
    }
  }, [showMenu]);

  const loadReview = async () => {
    try {
      const data = await api.getReview(projectId, reviewId);
      setReview(data);
      // Загружаем имя файла для targetId
      if (data.targetType === 'source_file_version') {
        loadTargetFileName(data.targetId);
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось загрузить ревью');
    } finally {
      setLoading(false);
    }
  };

  // Загружает имя файла по ID версии
  const loadTargetFileName = async (versionId: string) => {
    try {
      const files = await api.getSourceFiles(projectId);
      for (const file of files) {
        const versions = await api.getSourceFileVersions(projectId, file.id);
        const found = versions.find(v => v.id === versionId);
        if (found) {
          setTargetFileName(file.path);
          return;
        }
      }
    } catch (err) {
      console.error('Ошибка загрузки имени файла:', err);
    }
  };

  const loadItems = async () => {
    try {
      const data = await api.getReviewItems(projectId, reviewId);
      setItems(data);
    } catch (err) {
      console.error('Failed to load review items:', err);
    }
  };

  const handleUpdateStatus = async (status: 'pending' | 'approved' | 'changes_requested') => {
    const statusLabels: Record<string, string> = {
      'pending': 'Ожидает',
      'approved': 'Одобрено',
      'changes_requested': 'Требуются изменения'
    };
    
    if (!confirm(`Изменить статус на "${statusLabels[status]}"?`)) {
      return;
    }

    try {
      const dto: UpdateReviewDto = { status };
      await api.updateReview(projectId, reviewId, dto);
      loadReview();
      onUpdate();
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось обновить статус');
    }
  };

  const handleCreateItem = async (dto: CreateReviewItemDto) => {
    try {
      await api.createReviewItem(projectId, reviewId, dto);
      setShowAddItem(false);
      loadItems();
    } catch (err) {
      throw err;
    }
  };

  const handleUpdateItem = async (itemId: string, dto: UpdateReviewItemDto) => {
    try {
      await api.updateReviewItem(projectId, reviewId, itemId, dto);
      loadItems();
    } catch (err) {
      throw err;
    }
  };

  const handleDeleteItem = async (itemId: string) => {
    if (!confirm('Удалить элемент ревью?')) {
      return;
    }

    try {
      await api.deleteReviewItem(projectId, reviewId, itemId);
      if (selectedItem === itemId) {
        setSelectedItem(null);
      }
      loadItems();
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось удалить элемент');
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

  const getStatusLabel = (status: string) => {
    switch (status.toLowerCase()) {
      case 'pending':
        return 'Ожидает';
      case 'approved':
        return 'Одобрено';
      case 'changes_requested':
        return 'Требуются изменения';
      default:
        return status;
    }
  };

  const getStatusClass = (status: string) => {
    switch (status.toLowerCase()) {
      case 'pending':
        return 'status-pending';
      case 'approved':
        return 'status-approved';
      case 'changes_requested':
        return 'status-changes-requested';
      default:
        return '';
    }
  };

  // Получает отображаемый текст для anchorRef
  const getAnchorDisplayText = (anchorRef: string): string => {
    const data = parseAnchorRef(anchorRef);
    return data?.nodeText || anchorRef;
  };

  if (loading) {
    return <div className="review-details-container">Загрузка...</div>;
  }

  if (error || !review) {
    return (
      <div className="review-details-container">
        <div className="error">{error || 'Ревью не найдено'}</div>
        <button onClick={onBack} className="back-button">Назад</button>
      </div>
    );
  }

  return (
    <div className="review-details-container">
      <div className="review-details-header">
        <button onClick={onBack} className="back-button">← Назад</button>
        {canManage && (
          <div className="status-actions">
            {review.status !== 'approved' && (
              <button
                onClick={() => handleUpdateStatus('approved')}
                className="approve-button"
                title="Одобрить ревью"
              >
                Одобрить
              </button>
            )}
            {review.status !== 'changes_requested' && (
              <div className="menu-container">
                <button
                  onClick={() => setShowMenu(!showMenu)}
                  className="menu-button"
                  title="Дополнительные действия"
                >
                  <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                    <circle cx="12" cy="12" r="1"></circle>
                    <circle cx="12" cy="5" r="1"></circle>
                    <circle cx="12" cy="19" r="1"></circle>
                  </svg>
                </button>
                {showMenu && (
                  <div className="menu-dropdown">
                    <button
                      onClick={() => {
                        handleUpdateStatus('changes_requested');
                        setShowMenu(false);
                      }}
                      className="menu-item"
                    >
                      Требуются изменения
                    </button>
                  </div>
                )}
              </div>
            )}
          </div>
        )}
      </div>

      <div className="review-info">
        <h2>Ревью</h2>
        <div className="review-meta">
          <p><strong>Тип:</strong> {review.targetType === 'diagram_version' ? 'Версия диаграммы' : 'Версия файла'}</p>
          <p><strong>Файл:</strong> {targetFileName || 'Загрузка...'}</p>
          <p><strong>Статус:</strong>
            <span className={`status-badge ${getStatusClass(review.status)}`}>
              {getStatusLabel(review.status)}
            </span>
          </p>
          <p><strong>Создано:</strong> {formatDate(review.createdAt)}</p>
          {review.updatedAt && (
            <p><strong>Обновлено:</strong> {formatDate(review.updatedAt)}</p>
          )}
        </div>
      </div>

      <div className="review-items-section">
        <div className="section-header">
          <h3>Элементы ревью</h3>
          {canManage && (
            <button onClick={() => setShowAddItem(!showAddItem)} className="add-button">
              {showAddItem ? 'Отмена' : '+ Добавить элемент'}
            </button>
          )}
        </div>

        {showAddItem && canManage && (
          <ReviewItemEditor
            onSubmit={handleCreateItem}
            onCancel={() => setShowAddItem(false)}
          />
        )}

        {items.length === 0 ? (
          <div className="empty-state">Нет элементов ревью</div>
        ) : (
          <div className="review-items-list">
            {items.map((item) => (
              <div key={item.id} className="review-item-card">
                <div className="review-item-header">
                  <span className={`item-kind-badge kind-${item.kind}`}>
                    {item.kind === 'comment' ? 'Комментарий' : 'Проблема'}
                  </span>
                  <span className={`item-status-badge status-${item.status}`}>
                    {item.status === 'open' ? 'Открыто' : 'Решено'}
                  </span>
                </div>
                <div className="review-item-body">
                  <p><strong>Тип:</strong> {item.anchorType === 'code' ? 'Код' : 'Диаграмма'}</p>
                  <p><strong>Блок:</strong> {getAnchorDisplayText(item.anchorRef)}</p>
                  <p><strong>Комментарий:</strong> {item.body}</p>
                </div>
                {selectedItem === item.id ? (
                  <ReviewItemEditor
                    item={item}
                    reviewId={reviewId}
                    projectId={projectId}
                    onSubmit={(dto) => handleUpdateItem(item.id, dto)}
                    onCancel={() => setSelectedItem(null)}
                    onDelete={() => handleDeleteItem(item.id)}
                    canManage={canManage}
                  />
                ) : (
                  <div className="review-item-actions">
                    {canManage && (
                      <>
                        <button onClick={() => setSelectedItem(item.id)} className="edit-button">
                          Редактировать
                        </button>
                        <button onClick={() => handleDeleteItem(item.id)} className="delete-button">
                          Удалить
                        </button>
                      </>
                    )}
                  </div>
                )}
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
}

