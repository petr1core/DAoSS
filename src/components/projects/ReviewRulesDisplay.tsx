import { useState, useEffect } from 'react';
import './ReviewRulesDisplay.css';

interface ReviewRule {
  role: 'Owner' | 'Admin' | 'Reviewer';
  count: number;
}

interface ReviewRulesDisplayProps {
  value: string | null | undefined;
}

export default function ReviewRulesDisplay({ value }: ReviewRulesDisplayProps) {
  const [rules, setRules] = useState<ReviewRule[]>([]);

  // Парсинг JSON при монтировании и изменении value
  useEffect(() => {
    try {
      if (!value || (typeof value === 'string' && value.trim() === '')) {
        setRules([]);
        return;
      }
      const parsed = JSON.parse(value);
      if (Array.isArray(parsed)) {
        const normalizedRules: ReviewRule[] = parsed.map((rule: any) => ({
          role: rule.Role || rule.role || 'Admin',
          count: rule.Count !== undefined ? Number(rule.Count) : (rule.count !== undefined ? Number(rule.count) : 1),
        })).filter((rule: ReviewRule) => rule.count >= 1);
        setRules(normalizedRules);
      } else {
        setRules([]);
      }
    } catch {
      setRules([]);
    }
  }, [value]);

  const getRoleLabel = (role: 'Owner' | 'Admin' | 'Reviewer'): string => {
    switch (role) {
      case 'Owner':
        return 'Владелец';
      case 'Admin':
        return 'Администратор';
      case 'Reviewer':
        return 'Ревьюер';
      default:
        return role;
    }
  };

  const getRoleIcon = (role: 'Owner' | 'Admin' | 'Reviewer'): string => {
    switch (role) {
      case 'Owner':
        return '👑';
      case 'Admin':
        return '⚙️';
      case 'Reviewer':
        return '👁️';
      default:
        return '•';
    }
  };

  if (!value || (typeof value === 'string' && value.trim() === '') || rules.length === 0) {
    return null;
  }

  return (
    <div className="review-rules-display">
      <div className="review-rules-cards">
        {rules.map((rule, index) => (
          <div key={index} className="review-rule-card">
            <div className="rule-icon">{getRoleIcon(rule.role)}</div>
            <div className="rule-content">
              <div className="rule-role">{getRoleLabel(rule.role)}</div>
              <div className="rule-count">
                требуется {rule.count} {rule.count === 1 ? 'ревьюер' : rule.count < 5 ? 'ревьюера' : 'ревьюеров'}
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}

