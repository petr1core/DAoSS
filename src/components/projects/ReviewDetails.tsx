import { useState, useEffect } from 'react';
import type { Review, UpdateReviewDto, ReviewItem, CreateReviewItemDto, UpdateReviewItemDto, Comment, CreateCommentDto, UpdateCommentDto } from '../../services/api';
import { api } from '../../services/api';
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

  useEffect(() => {
    loadReview();
    loadItems();
  }, [projectId, reviewId]);

  const loadReview = async () => {
    try {
      const data = await api.getReview(projectId, reviewId);
      setReview(data);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось загрузить ревью');
    } finally {
      setLoading(false);
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

  const handleUpdateStatus = async (status: 'approved' | 'changes_requested') => {
    if (!confirm(`Изменить статус на "${status === 'approved' ? 'Одобрено' : 'Требуются изменения'}"?`)) {
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
        {canManage && review.status === 'pending' && (
          <div className="status-actions">
            <button
              onClick={() => handleUpdateStatus('approved')}
              className="approve-button"
            >
              Одобрить
            </button>
            <button
              onClick={() => handleUpdateStatus('changes_requested')}
              className="request-changes-button"
            >
              Требуются изменения
            </button>
          </div>
        )}
      </div>

      <div className="review-info">
        <h2>Ревью</h2>
        <div className="review-meta">
          <p><strong>Тип:</strong> {review.targetType === 'diagram_version' ? 'Версия диаграммы' : 'Версия файла'}</p>
          <p><strong>ID цели:</strong> {review.targetId}</p>
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
                  <p><strong>Тип якоря:</strong> {item.anchorType === 'code' ? 'Код' : 'Диаграмма'}</p>
                  <p><strong>Ссылка:</strong> {item.anchorRef}</p>
                  <p><strong>Текст:</strong> {item.body}</p>
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

