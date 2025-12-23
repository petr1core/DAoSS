import { useState, useEffect } from 'react';
import type { ReviewItem, CreateReviewItemDto, UpdateReviewItemDto, Comment, CreateCommentDto, UpdateCommentDto } from '../../services/api';
import { api } from '../../services/api';
import './ReviewItemEditor.css';

interface ReviewItemEditorProps {
  item?: ReviewItem;
  reviewId?: string;
  projectId?: string;
  onSubmit: (dto: CreateReviewItemDto | UpdateReviewItemDto) => Promise<void>;
  onCancel: () => void;
  onDelete?: () => void;
  canManage?: boolean;
}

export default function ReviewItemEditor({
  item,
  reviewId,
  projectId,
  onSubmit,
  onCancel,
  onDelete,
  canManage = true
}: ReviewItemEditorProps) {
  const [kind, setKind] = useState<'comment' | 'issue'>('comment');
  const [anchorType, setAnchorType] = useState<'code' | 'diagram'>('code');
  const [anchorRef, setAnchorRef] = useState('');
  const [body, setBody] = useState('');
  const [status, setStatus] = useState<'open' | 'resolved'>('open');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [comments, setComments] = useState<Comment[]>([]);
  const [showComments, setShowComments] = useState(false);
  const [newComment, setNewComment] = useState('');

  useEffect(() => {
    if (item) {
      setKind(item.kind);
      setAnchorType(item.anchorType);
      setAnchorRef(item.anchorRef);
      setBody(item.body);
      setStatus(item.status as 'open' | 'resolved');
    }
  }, [item]);

  const loadComments = async () => {
    if (!item?.id || !reviewId || !projectId) return;
    try {
      const data = await api.getComments(projectId, reviewId, item.id);
      setComments(data);
    } catch (err) {
      console.error('Failed to load comments:', err);
    }
  };

  useEffect(() => {
    if (item?.id && reviewId && projectId && showComments) {
      loadComments();
    }
  }, [item?.id, reviewId, projectId, showComments]); // eslint-disable-line react-hooks/exhaustive-deps

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);
    setLoading(true);

    try {
      if (item) {
        await onSubmit({ body, status });
      } else {
        await onSubmit({ kind, anchorType, anchorRef, body });
      }
      if (!item) {
        setKind('comment');
        setAnchorType('code');
        setAnchorRef('');
        setBody('');
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось сохранить элемент');
    } finally {
      setLoading(false);
    }
  };

  const handleAddComment = async () => {
    if (!newComment.trim() || !item?.id || !reviewId || !projectId) return;

    try {
      const dto: CreateCommentDto = { body: newComment.trim() };
      await api.createComment(projectId, reviewId, item.id, dto);
      setNewComment('');
      await loadComments();
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось добавить комментарий');
    }
  };

  const handleUpdateComment = async (commentId: string, body: string) => {
    if (!item?.id || !reviewId || !projectId) return;

    try {
      const dto: UpdateCommentDto = { body };
      await api.updateComment(projectId, reviewId, item.id, commentId, dto);
      loadComments();
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось обновить комментарий');
    }
  };

  const handleDeleteComment = async (commentId: string) => {
    if (!item?.id || !reviewId || !projectId) return;
    if (!confirm('Удалить комментарий?')) return;

    try {
      await api.deleteComment(projectId, reviewId, item.id, commentId);
      loadComments();
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось удалить комментарий');
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

  return (
    <div className="review-item-editor-container">
      <form onSubmit={handleSubmit} className="review-item-form">
        {error && <div className="error-message">{error}</div>}

        {!item && (
          <>
            <div className="form-group">
              <label htmlFor="kind">Тип *</label>
              <select
                id="kind"
                value={kind}
                onChange={(e) => setKind(e.target.value as any)}
                disabled={loading}
              >
                <option value="comment">Комментарий</option>
                <option value="issue">Проблема</option>
              </select>
            </div>

            <div className="form-group">
              <label htmlFor="anchorType">Тип якоря *</label>
              <select
                id="anchorType"
                value={anchorType}
                onChange={(e) => setAnchorType(e.target.value as any)}
                disabled={loading}
              >
                <option value="code">Код</option>
                <option value="diagram">Диаграмма</option>
              </select>
            </div>

            <div className="form-group">
              <label htmlFor="anchorRef">Ссылка на якорь *</label>
              <input
                type="text"
                id="anchorRef"
                value={anchorRef}
                onChange={(e) => setAnchorRef(e.target.value)}
                required
                disabled={loading}
              />
            </div>
          </>
        )}

        <div className="form-group">
          <label htmlFor="body">Текст *</label>
          <textarea
            id="body"
            value={body}
            onChange={(e) => setBody(e.target.value)}
            required
            rows={4}
            disabled={loading}
          />
        </div>

        {item && (
          <div className="form-group">
            <label htmlFor="status">Статус</label>
            <select
              id="status"
              value={status}
              onChange={(e) => setStatus(e.target.value as any)}
              disabled={loading}
            >
              <option value="open">Открыто</option>
              <option value="resolved">Решено</option>
            </select>
          </div>
        )}

        <div className="form-actions">
          {item && onDelete && canManage && (
            <button type="button" onClick={onDelete} className="delete-button">
              Удалить
            </button>
          )}
          <button type="button" onClick={onCancel} disabled={loading} className="cancel-button">
            Отмена
          </button>
          <button type="submit" disabled={loading} className="submit-button">
            {loading ? 'Сохранение...' : item ? 'Сохранить' : 'Создать'}
          </button>
        </div>
      </form>

      {item && item.id && reviewId && projectId && (
        <div className="comments-section">
          <button
            onClick={() => {
              setShowComments(!showComments);
              if (!showComments && comments.length === 0) {
                loadComments();
              }
            }}
            className="toggle-comments-button"
          >
            {showComments ? 'Скрыть' : 'Показать'} комментарии ({comments.length})
          </button>

          {showComments && (
            <div className="comments-list">
              {comments.map((comment) => (
                <div key={comment.id} className="comment-card">
                  <div className="comment-header">
                    <span className="comment-author">Автор: {comment.authorId}</span>
                    <span className="comment-date">{formatDate(comment.createdAt)}</span>
                  </div>
                  <div className="comment-body">{comment.body}</div>
                  {canManage && (
                    <div className="comment-actions">
                      <button
                        onClick={() => {
                          const newBody = prompt('Новый текст комментария:', comment.body);
                          if (newBody !== null && newBody.trim()) {
                            handleUpdateComment(comment.id, newBody);
                          }
                        }}
                        className="edit-comment-button"
                      >
                        Редактировать
                      </button>
                      <button
                        onClick={() => handleDeleteComment(comment.id)}
                        className="delete-comment-button"
                      >
                        Удалить
                      </button>
                    </div>
                  )}
                </div>
              ))}

              {canManage && (
                <div className="add-comment-section">
                  <textarea
                    value={newComment}
                    onChange={(e) => setNewComment(e.target.value)}
                    placeholder="Добавить комментарий..."
                    rows={3}
                    className="comment-input"
                  />
                  <button
                    onClick={handleAddComment}
                    className="add-comment-button"
                    disabled={!newComment.trim()}
                  >
                    Добавить комментарий
                  </button>
                </div>
              )}
            </div>
          )}
        </div>
      )}
    </div>
  );
}

