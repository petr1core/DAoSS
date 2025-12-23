import { useState } from 'react';
import type { CreateSourceFileDto } from '../../services/api';
import './SourceFileForm.css';

interface SourceFileFormProps {
  onSubmit: (dto: CreateSourceFileDto) => Promise<void>;
  onCancel: () => void;
}

export default function SourceFileForm({ onSubmit, onCancel }: SourceFileFormProps) {
  const [path, setPath] = useState('');
  const [content, setContent] = useState('');
  const [message, setMessage] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);
    setLoading(true);

    try {
      await onSubmit({ path, content, message: message || undefined });
      setPath('');
      setContent('');
      setMessage('');
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось создать файл');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="source-file-form-container">
      <form onSubmit={handleSubmit} className="source-file-form">
        {error && <div className="error-message">{error}</div>}

        <div className="form-group">
          <label htmlFor="path">Путь к файлу *</label>
          <input
            type="text"
            id="path"
            value={path}
            onChange={(e) => setPath(e.target.value)}
            required
            placeholder="Например: src/main.cpp или main.pas"
            disabled={loading}
          />
          <small>Укажите путь к файлу относительно корня проекта</small>
        </div>

        <div className="form-group">
          <label htmlFor="content">Содержимое файла *</label>
          <textarea
            id="content"
            value={content}
            onChange={(e) => setContent(e.target.value)}
            required
            placeholder="Введите код файла..."
            disabled={loading}
            rows={15}
          />
        </div>

        <div className="form-group">
          <label htmlFor="message">Сообщение коммита (необязательно)</label>
          <input
            type="text"
            id="message"
            value={message}
            onChange={(e) => setMessage(e.target.value)}
            placeholder="Описание изменений"
            disabled={loading}
          />
        </div>

        <div className="form-actions">
          <button type="button" onClick={onCancel} disabled={loading} className="cancel-button">
            Отмена
          </button>
          <button type="submit" disabled={loading} className="submit-button">
            {loading ? 'Создание...' : 'Создать файл'}
          </button>
        </div>
      </form>
    </div>
  );
}

