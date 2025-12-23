import { useState, useEffect } from 'react';
import type { Invitation } from '../../services/api';
import { api } from '../../services/api';
import InvitationCard from './InvitationCard';
import './InvitationsPage.css';

export default function InvitationsPage() {
  const [invitations, setInvitations] = useState<Invitation[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    loadInvitations();
  }, []);

  const loadInvitations = async () => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.getUserInvitations();
      // Фильтруем только pending приглашения
      setInvitations(data.filter(inv => inv.status.toLowerCase() === 'pending'));
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось загрузить приглашения');
    } finally {
      setLoading(false);
    }
  };

  const handleAccept = async (invitationId: string) => {
    try {
      await api.acceptInvitation(invitationId);
      loadInvitations();
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось принять приглашение');
    }
  };

  const handleReject = async (invitationId: string) => {
    try {
      await api.rejectInvitation(invitationId);
      loadInvitations();
    } catch (err) {
      alert(err instanceof Error ? err.message : 'Не удалось отклонить приглашение');
    }
  };

  if (loading) {
    return (
      <div className="invitations-page-container">
        <div className="loading">Загрузка приглашений...</div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="invitations-page-container">
        <div className="error">{error}</div>
        <button onClick={loadInvitations}>Повторить</button>
      </div>
    );
  }

  return (
    <div className="invitations-page-container">
      <h1>Мои приглашения</h1>

      {invitations.length === 0 ? (
        <div className="empty-state">
          <p>У вас нет новых приглашений</p>
        </div>
      ) : (
        <div className="invitations-list">
          {invitations.map((invitation) => (
            <InvitationCard
              key={invitation.id}
              invitation={invitation}
              onAccept={() => handleAccept(invitation.id)}
              onReject={() => handleReject(invitation.id)}
            />
          ))}
        </div>
      )}
    </div>
  );
}

