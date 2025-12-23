import { useState } from 'react';
import type { CreateReviewDto } from '../../services/api';
import './ReviewForm.css';

interface ReviewFormProps {
  onSubmit: (dto: CreateReviewDto) => Promise<void>;
  onCancel: () => void;
}

export default function ReviewForm({ onSubmit, onCancel }: ReviewFormProps) {
  const [targetType, setTargetType] = useState<'diagram_version' | 'source_file_version'>('diagram_version');
  const [targetId, setTargetId] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);
    setLoading(true);

    try {
      await onSubmit({ targetType, targetId });
      setTargetId('');
      setTargetType('diagram_version');
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось создать ревью');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="review-form-container">
      <form onSubmit={handleSubmit} className="review-form">
        {error && <div className="error-message">{error}</div>}

        <div className="form-group">
          <label htmlFor="targetType">Тип цели *</label>
          <select
            id="targetType"
            value={targetType}
            onChange={(e) => setTargetType(e.target.value as any)}
            disabled={loading}
          >
            <option value="diagram_version">Версия диаграммы</option>
            <option value="source_file_version">Версия файла</option>
          </select>
        </div>

        <div className="form-group">
          <label htmlFor="targetId">ID цели *</label>
          <input
            type="text"
            id="targetId"
            value={targetId}
            onChange={(e) => setTargetId(e.target.value)}
            required
            placeholder="Введите GUID цели"
            disabled={loading}
          />
        </div>

        <div className="form-actions">
          <button type="button" onClick={onCancel} disabled={loading} className="cancel-button">
            Отмена
          </button>
          <button type="submit" disabled={loading} className="submit-button">
            {loading ? 'Создание...' : 'Создать'}
          </button>
        </div>
      </form>
    </div>
  );
}

