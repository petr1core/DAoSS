import { useState } from 'react';
import './UserDisplay.css';

interface UserDisplayProps {
  userId: string;
  userName: string;
  userEmail: string;
}

export default function UserDisplay({ userId, userName, userEmail }: UserDisplayProps) {
  const [copied, setCopied] = useState(false);
  const [showTooltip, setShowTooltip] = useState(false);

  const handleCopy = async () => {
    try {
      await navigator.clipboard.writeText(userEmail);
      setCopied(true);
      setTimeout(() => {
        setCopied(false);
      }, 1000);
    } catch (err) {
      // Fallback для старых браузеров
      const textArea = document.createElement('textarea');
      textArea.value = userEmail;
      textArea.style.position = 'fixed';
      textArea.style.left = '-999999px';
      document.body.appendChild(textArea);
      textArea.select();
      try {
        document.execCommand('copy');
        setCopied(true);
        setTimeout(() => {
          setCopied(false);
        }, 1000);
      } catch (e) {
        console.error('Failed to copy email:', e);
      }
      document.body.removeChild(textArea);
    }
  };

  const displayName = userName || userId;

  return (
    <div className="user-display-container">
      <span className="user-name">{displayName}</span>
      <div
        className="user-email-icon-container"
        onMouseEnter={() => setShowTooltip(true)}
        onMouseLeave={() => setShowTooltip(false)}
        onClick={handleCopy}
        title={userEmail}
      >
        {copied ? (
          <svg
            className="user-icon check-icon"
            width="14"
            height="14"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            <polyline points="20 6 9 17 4 12"></polyline>
          </svg>
        ) : (
          <svg
            className="user-icon eye-icon"
            width="14"
            height="14"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
          >
            <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>
            <circle cx="12" cy="12" r="3"></circle>
          </svg>
        )}
        {showTooltip && !copied && (
          <div className="email-tooltip">{userEmail}</div>
        )}
      </div>
    </div>
  );
}

