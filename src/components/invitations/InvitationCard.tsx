import type { Invitation } from '../../services/api';
import './InvitationCard.css';

interface InvitationCardProps {
  invitation: Invitation;
  onAccept: () => void;
  onReject: () => void;
}

export default function InvitationCard({ invitation, onAccept, onReject }: InvitationCardProps) {
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

  const isExpired = new Date(invitation.expiresAt) < new Date();

  return (
    <div className={`invitation-card ${isExpired ? 'expired' : ''}`}>
      <div className="invitation-header">
        <h3>Приглашение в проект</h3>
        {isExpired && <span className="expired-badge">Истекло</span>}
      </div>

      <div className="invitation-info">
        <p><strong>Роль:</strong>
          <span className={`role-badge role-${invitation.role}`}>
            {invitation.role === 'admin' ? 'Администратор' : 'Ревьюер'}
          </span>
        </p>
        <p><strong>Срок действия:</strong> {formatDate(invitation.expiresAt)}</p>
        <p><strong>Отправлено:</strong> {formatDate(invitation.createdAt)}</p>
      </div>

      {!isExpired && (
        <div className="invitation-actions">
          <button onClick={onAccept} className="accept-button">
            Принять
          </button>
          <button onClick={onReject} className="reject-button">
            Отклонить
          </button>
        </div>
      )}
    </div>
  );
}


