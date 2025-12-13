import './UserPanel.css';

interface UserPanelProps {
  username: string;
  status: string;
  onLogout: () => void;
}

function UserPanel({ username, status, onLogout }: UserPanelProps) {
  return (
    <div className="user-panel">
      <div className="user-info">
        <span className="user-name">{username}</span>
        <span className="user-status">{status}</span>
      </div>
      <button 
        onClick={onLogout}
        className="logout-button"
        style={{
          padding: '0.5rem 1rem',
          backgroundColor: '#ef4444',
          color: 'white',
          border: 'none',
          borderRadius: '0.375rem',
          cursor: 'pointer',
          fontSize: '0.875rem',
          fontWeight: '500',
          transition: 'background-color 0.2s'
        }}
        onMouseOver={(e) => e.currentTarget.style.backgroundColor = '#dc2626'}
        onMouseOut={(e) => e.currentTarget.style.backgroundColor = '#ef4444'}
      >
        Выйти
      </button>
    </div>
  );
}

export default UserPanel;



