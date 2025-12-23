import { useState, useEffect } from 'react';
import type { ProjectMember, CreateMemberDto, UpdateRoleDto } from '../../services/api';
import { api } from '../../services/api';
import MemberForm from './MemberForm';
import ConfirmDialog from '../ui/ConfirmDialog';
import UserDisplay from '../ui/UserDisplay';
import './MembersList.css';

interface MembersListProps {
  projectId: string;
  userId: string;
  userRole: string | null;
  onUpdate: () => void;
}

export default function MembersList({ projectId, userId, userRole, onUpdate }: MembersListProps) {
  const [members, setMembers] = useState<ProjectMember[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [showAddForm, setShowAddForm] = useState(false);
  const [roleChangeConfirm, setRoleChangeConfirm] = useState<{ isOpen: boolean; userId: string | null; newRole: 'admin' | 'reviewer' | null }>({ isOpen: false, userId: null, newRole: null });
  const [removeConfirm, setRemoveConfirm] = useState<{ isOpen: boolean; userId: string | null }>({ isOpen: false, userId: null });
  const [userData, setUserData] = useState<Record<string, { name: string; email: string }>>({});

  const canManage = userRole === 'owner' || userRole === 'admin';

  useEffect(() => {
    loadMembers();
  }, [projectId]);

  useEffect(() => {
    if (members.length > 0) {
      loadUserData();
    }
  }, [members]);

  const loadMembers = async () => {
    try {
      setLoading(true);
      setError(null);
      const data = await api.getProjectMembers(projectId);
      setMembers(data);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось загрузить участников');
    } finally {
      setLoading(false);
    }
  };

  const loadUserData = async () => {
    const data: Record<string, { name: string; email: string }> = {};
    for (const member of members) {
      if (member.email && member.name) {
        // Если данные уже есть в member, используем их
        data[member.userId] = { name: member.name, email: member.email };
      } else {
        try {
          const user = await api.getUser(member.userId);
          data[member.userId] = { 
            name: user.name || user.login || user.email || member.userId, 
            email: user.email 
          };
        } catch {
          // Fallback to ID if user not found
          data[member.userId] = { 
            name: member.userId, 
            email: member.email || member.userId 
          };
        }
      }
    }
    setUserData(data);
  };

  const handleAddMember = async (dto: CreateMemberDto) => {
    try {
      await api.addProjectMember(projectId, dto);
      setShowAddForm(false);
      loadMembers();
      onUpdate();
    } catch (err) {
      throw err;
    }
  };

  const handleUpdateRoleClick = (memberUserId: string, newRole: 'admin' | 'reviewer') => {
    setRoleChangeConfirm({ isOpen: true, userId: memberUserId, newRole });
  };

  const handleUpdateRoleConfirm = async () => {
    if (!roleChangeConfirm.userId || !roleChangeConfirm.newRole) return;

    try {
      const dto: UpdateRoleDto = { role: roleChangeConfirm.newRole };
      await api.updateMemberRole(projectId, roleChangeConfirm.userId, dto);
      setRoleChangeConfirm({ isOpen: false, userId: null, newRole: null });
      loadMembers();
      onUpdate();
    } catch (err) {
      setRoleChangeConfirm({ isOpen: false, userId: null, newRole: null });
      alert(err instanceof Error ? err.message : 'Не удалось изменить роль');
    }
  };

  const handleRemoveMemberClick = (memberUserId: string) => {
    setRemoveConfirm({ isOpen: true, userId: memberUserId });
  };

  const handleRemoveMemberConfirm = async () => {
    if (!removeConfirm.userId) return;

    try {
      await api.removeProjectMember(projectId, removeConfirm.userId);
      setRemoveConfirm({ isOpen: false, userId: null });
      loadMembers();
      onUpdate();
    } catch (err) {
      setRemoveConfirm({ isOpen: false, userId: null });
      alert(err instanceof Error ? err.message : 'Не удалось удалить участника');
    }
  };

  if (loading) {
    return <div className="members-list-container">Загрузка участников...</div>;
  }

  if (error) {
    return (
      <div className="members-list-container">
        <div className="error">{error}</div>
        <button onClick={loadMembers}>Повторить</button>
      </div>
    );
  }

  return (
    <div className="members-list-container">
      <div className="members-header">
        <h2>Участники проекта</h2>
        {canManage && (
          <button onClick={() => setShowAddForm(!showAddForm)} className="add-button">
            {showAddForm ? 'Отмена' : '+ Добавить участника'}
          </button>
        )}
      </div>

      {showAddForm && canManage && (
        <MemberForm
          onSubmit={handleAddMember}
          onCancel={() => setShowAddForm(false)}
        />
      )}

      {members.length === 0 ? (
        <div className="empty-state">Нет участников</div>
      ) : (
        <table className="members-table">
          <thead>
            <tr>
              <th>Пользователь</th>
              <th>Роль</th>
              {canManage && <th>Действия</th>}
            </tr>
          </thead>
          <tbody>
            {members.map((member) => {
              const userInfo = userData[member.userId];
              return (
              <tr key={member.userId}>
                <td>
                  {userInfo ? (
                    <UserDisplay
                      userId={member.userId}
                      userName={userInfo.name}
                      userEmail={userInfo.email}
                    />
                  ) : (
                    <span>Загрузка...</span>
                  )}
                </td>
                <td>
                  <span className={`role-badge role-${member.role}`}>
                    {member.role === 'owner' ? 'Владелец' :
                      member.role === 'admin' ? 'Администратор' : 'Ревьюер'}
                  </span>
                </td>
                {canManage && (
                  <td className="actions-cell">
                    {member.role !== 'owner' && (
                      <>
                        <select
                          value={member.role}
                          onChange={(e) => {
                            const newRole = e.target.value as 'admin' | 'reviewer';
                            if (newRole !== member.role) {
                              handleUpdateRoleClick(member.userId, newRole);
                            }
                          }}
                          className="role-select"
                        >
                          <option value="reviewer">Ревьюер</option>
                          <option value="admin">Администратор</option>
                          {userRole === 'owner' && <option value="owner">Владелец</option>}
                        </select>
                        <button
                          onClick={() => handleRemoveMemberClick(member.userId)}
                          className="remove-button"
                        >
                          Удалить
                        </button>
                      </>
                    )}
                    {member.role === 'owner' && <span className="no-actions">Нельзя изменить</span>}
                  </td>
                )}
              </tr>
            );
            })}
          </tbody>
        </table>
      )}

      <ConfirmDialog
        isOpen={roleChangeConfirm.isOpen}
        title="Изменение роли"
        message={`Изменить роль на "${roleChangeConfirm.newRole === 'admin' ? 'Администратор' : 'Ревьюер'}"?`}
        onConfirm={handleUpdateRoleConfirm}
        onCancel={() => setRoleChangeConfirm({ isOpen: false, userId: null, newRole: null })}
        confirmText="Изменить"
        cancelText="Отмена"
      />

      <ConfirmDialog
        isOpen={removeConfirm.isOpen}
        title="Удаление участника"
        message="Удалить участника из проекта?"
        onConfirm={handleRemoveMemberConfirm}
        onCancel={() => setRemoveConfirm({ isOpen: false, userId: null })}
        confirmText="Удалить"
        cancelText="Отмена"
        confirmButtonClass="delete-button"
      />
    </div>
  );
}

