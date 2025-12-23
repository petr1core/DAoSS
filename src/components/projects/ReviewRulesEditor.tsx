import { useState, useEffect, useMemo } from 'react';
import './ReviewRulesEditor.css';

interface ReviewRule {
  role: 'Owner' | 'Admin' | 'Reviewer';
  count: number;
}

interface ValidationError {
  type: 'duplicate' | 'multiple';
  message: string;
  mergeSuggestion?: { role: 'Owner' | 'Admin' | 'Reviewer'; count: number };
}

interface ReviewRulesEditorProps {
  value: string;
  onChange: (value: string) => void;
  disabled?: boolean;
}

export default function ReviewRulesEditor({ value, onChange, disabled }: ReviewRulesEditorProps) {
  const [rules, setRules] = useState<ReviewRule[]>([]);

  // Парсинг JSON при монтировании и изменении value
  useEffect(() => {
    try {
      if (!value || value.trim() === '') {
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

  // Сериализация в JSON при изменении правил
  const updateRules = (newRules: ReviewRule[]) => {
    setRules(newRules);
    if (newRules.length === 0) {
      onChange('');
      return;
    }
    const jsonRules = newRules.map(rule => ({
      Role: rule.role,
      Count: rule.count,
    }));
    onChange(JSON.stringify(jsonRules));
  };

  const handleAddRule = () => {
    if (rules.length < 5) {
      updateRules([...rules, { role: 'Admin', count: 1 }]);
    }
  };

  const handleRemoveRule = (index: number) => {
    const newRules = rules.filter((_, i) => i !== index);
    updateRules(newRules);
  };

  const handleRoleChange = (index: number, role: 'Owner' | 'Admin' | 'Reviewer') => {
    const newRules = [...rules];
    newRules[index].role = role;
    updateRules(newRules);
  };

  const handleCountChange = (index: number, count: number) => {
    const newRules = [...rules];
    newRules[index].count = Math.max(1, count);
    updateRules(newRules);
  };

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

  // Валидация правил на дублирующиеся роли
  const validationErrors = useMemo<Map<number, ValidationError>>(() => {
    const errors = new Map<number, ValidationError>();
    const roleGroups = new Map<'Owner' | 'Admin' | 'Reviewer', number[]>();

    // Группируем правила по ролям
    rules.forEach((rule, index) => {
      if (!roleGroups.has(rule.role)) {
        roleGroups.set(rule.role, []);
      }
      roleGroups.get(rule.role)!.push(index);
    });

    // Проверяем каждую группу
    roleGroups.forEach((indices, role) => {
      if (indices.length > 1) {
        // Есть несколько правил для одной роли
        const roleRules = indices.map(idx => rules[idx]);
        const totalCount = roleRules.reduce((sum, rule) => sum + rule.count, 0);
        
        // Проверяем, одинаковые ли количества
        const allSame = roleRules.every(rule => rule.count === roleRules[0].count);
        
        if (allSame) {
          // Все правила одинаковые - это дубликаты
          indices.forEach(index => {
            errors.set(index, {
              type: 'duplicate',
              message: `Дублирующееся правило. Для роли "${getRoleLabel(role)}" уже есть правило с таким же количеством.`
            });
          });
        } else {
          // Разные количества - предлагаем объединить
          indices.forEach(index => {
            errors.set(index, {
              type: 'multiple',
              message: `Для роли "${getRoleLabel(role)}" есть несколько правил. Рекомендуется объединить их в одно.`,
              mergeSuggestion: { role, count: totalCount }
            });
          });
        }
      }
    });

    return errors;
  }, [rules]);

  const handleMergeRules = (role: 'Owner' | 'Admin' | 'Reviewer', totalCount: number) => {
    const roleLabel = getRoleLabel(role);
    const confirmed = window.confirm(
      `Объединить все правила для роли "${roleLabel}" в одно правило с количеством ${totalCount}?`
    );
    
    if (confirmed) {
      // Удаляем все правила с этой ролью
      const newRules = rules.filter(rule => rule.role !== role);
      // Добавляем объединенное правило
      newRules.push({ role, count: totalCount });
      updateRules(newRules);
    }
  };

  return (
    <div className="review-rules-editor">
      <div className="review-rules-list">
        {rules.map((rule, index) => {
          const error = validationErrors.get(index);
          const hasError = error !== undefined;
          
          return (
            <div key={index} className="review-rule-wrapper">
              <div className={`review-rule-row ${hasError ? 'has-error' : ''}`}>
                <span className="rule-label">Правило {index + 1}:</span>
                <select
                  value={rule.role}
                  onChange={(e) => handleRoleChange(index, e.target.value as 'Owner' | 'Admin' | 'Reviewer')}
                  disabled={disabled}
                  className={`rule-role-select ${hasError ? 'error' : ''}`}
                >
                  <option value="Owner">Владелец</option>
                  <option value="Admin">Администратор</option>
                  <option value="Reviewer">Ревьюер</option>
                </select>
                <input
                  type="number"
                  value={rule.count}
                  onChange={(e) => handleCountChange(index, parseInt(e.target.value) || 1)}
                  min={1}
                  disabled={disabled}
                  className={`rule-count-input ${hasError ? 'error' : ''}`}
                  required
                />
                <button
                  type="button"
                  onClick={() => handleRemoveRule(index)}
                  disabled={disabled}
                  className="rule-remove-button"
                  title="Удалить правило"
                >
                  <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <polyline points="3 6 5 6 21 6"></polyline>
                    <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
                  </svg>
                </button>
              </div>
              {hasError && error && (
                <div className="rule-error-message">
                  <span className="error-text">{error.message}</span>
                  {error.mergeSuggestion && (
                    <button
                      type="button"
                      onClick={() => handleMergeRules(error.mergeSuggestion!.role, error.mergeSuggestion!.count)}
                      disabled={disabled}
                      className="merge-suggestion-button"
                    >
                      Объединить в {getRoleLabel(error.mergeSuggestion.role)}: {error.mergeSuggestion.count}
                    </button>
                  )}
                </div>
              )}
            </div>
          );
        })}
      </div>
      {rules.length < 5 && (
        <button
          type="button"
          onClick={handleAddRule}
          disabled={disabled}
          className="add-rule-button"
        >
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
            <line x1="12" y1="5" x2="12" y2="19"></line>
            <line x1="5" y1="12" x2="19" y2="12"></line>
          </svg>
          Добавить правило
        </button>
      )}
    </div>
  );
}

