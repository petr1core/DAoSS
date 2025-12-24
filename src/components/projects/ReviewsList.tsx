import { useState, useEffect } from 'react';
import type { Review, CreateReviewDto, SourceFile } from '../../services/api';
import { api } from '../../services/api';
import ReviewForm from './ReviewForm';
import ReviewDetails from './ReviewDetails';
import './ReviewsList.css';

interface ReviewsListProps {
  projectId: string;
  canManage: boolean;
}

// Кэш для хранения соответствия versionId -> имя файла
interface TargetNameCache {
  [versionId: string]: string;
}

export default function ReviewsList({ projectId, canManage }: ReviewsListProps) {
  const [reviews, setReviews] = useState<Review[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [showAddForm, setShowAddForm] = useState(false);
  const [selectedReview, setSelectedReview] = useState<string | null>(null);
  const [targetNames, setTargetNames] = useState<TargetNameCache>({});

  useEffect(() => {
    loadReviews();
  }, [projectId]);

  // Загружает имена файлов для всех targetId
  const loadTargetNames = async (reviewsList: Review[]) => {
    const newCache: TargetNameCache = { ...targetNames };
    const versionIds = reviewsList
      .filter(r => r.targetType === 'source_file_version')
      .map(r => r.targetId)
      .filter(id => !newCache[id]);

    if (versionIds.length === 0) return;

    try {
      // Загружаем все файлы проекта
      const files = await api.getSourceFiles(projectId);
      
      // Для каждого файла загружаем версии и ищем соответствие
      for (const file of files) {
        try {
          const versions = await api.getSourceFileVersions(projectId, file.id);
          for (const version of versions) {
            if (versionIds.includes(version.id)) {
              newCache[version.id] = file.path;
            }
          }
        } catch {
          // Игнорируем ошибки загрузки версий
        }
      }

      setTargetNames(newCache);
    } catch (err) {
      console.error('Ошибка загрузки имён файлов:', err);
    }
  };

  const loadReviews = async () => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.getReviews(projectId);
      setReviews(data);
      // Загружаем имена файлов для ревью
      loadTargetNames(data);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось загрузить ревью');
    } finally {
      setLoading(false);
    }
  };

  // Получает отображаемое имя для targetId
  const getTargetDisplayName = (review: Review): string => {
    if (review.targetType === 'source_file_version') {
      return targetNames[review.targetId] || 'Загрузка...';
    }
    return review.targetId; // Для diagram_version пока оставляем ID
  };

  const handleCreateReview = async (dto: CreateReviewDto) => {
    try {
      await api.createReview(projectId, dto);
      setShowAddForm(false);
      loadReviews();
    } catch (err) {
      throw err;
    }
  };

  const handleDeleteReview = async (reviewId: string) => {
    if (!confirm('Удалить ревью?')) {
      return;
    }

    try {
      await api.deleteReview(projectId, reviewId);
      if (selectedReview === reviewId) {
        setSelectedReview(null);
      }
      loadReviews();
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось удалить ревью');
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
    return <div className="reviews-list-container">Загрузка ревью...</div>;
  }

  if (error) {
    return (
      <div className="reviews-list-container">
        <div className="error">{error}</div>
        <button onClick={loadReviews}>Повторить</button>
      </div>
    );
  }

  if (selectedReview) {
    return (
      <ReviewDetails
        projectId={projectId}
        reviewId={selectedReview}
        canManage={canManage}
        onBack={() => setSelectedReview(null)}
        onUpdate={loadReviews}
      />
    );
  }

  return (
    <div className="reviews-list-container">
      <div className="reviews-header">
        <h2>Ревью проекта</h2>
        {canManage && (
          <button onClick={() => setShowAddForm(!showAddForm)} className="add-button">
            {showAddForm ? 'Отмена' : '+ Создать ревью'}
          </button>
        )}
      </div>

      {showAddForm && canManage && (
        <ReviewForm
          onSubmit={handleCreateReview}
          onCancel={() => setShowAddForm(false)}
        />
      )}

      {reviews.length === 0 ? (
        <div className="empty-state">Нет ревью</div>
      ) : (
        <div className="reviews-list">
          {reviews.map((review) => (
            <div key={review.id} className="review-card" onClick={() => setSelectedReview(review.id)}>
              <div className="review-card-header">
                <h3>
                  {review.targetType === 'diagram_version' ? 'Версия диаграммы' : 'Версия файла'}
                </h3>
                <span className={`status-badge ${getStatusClass(review.status)}`}>
                  {getStatusLabel(review.status)}
                </span>
              </div>
              <div className="review-card-info">
                <p><strong>Файл:</strong> {getTargetDisplayName(review)}</p>
                <p><strong>Создано:</strong> {formatDate(review.createdAt)}</p>
                {review.updatedAt && (
                  <p><strong>Обновлено:</strong> {formatDate(review.updatedAt)}</p>
                )}
              </div>
              {canManage && (
                <div className="review-card-actions">
                  <button
                    onClick={(e) => {
                      e.stopPropagation();
                      handleDeleteReview(review.id);
                    }}
                    className="delete-button"
                  >
                    Удалить
                  </button>
                </div>
              )}
            </div>
          ))}
        </div>
      )}
    </div>
  );
}

