import { Link } from 'react-router-dom';
import './HomePage.css';

export default function HomePage() {
  return (
    <div className="home-page">
      <div className="home-container">
        <h1 className="home-title">DAoSS - Система автоматической генерации блок-схем</h1>
        <p className="home-description">
          Удобное средство для перевода кода в блок-схемы с автоматическим поддержанием документации в актуальном состоянии.
          При изменении кода формулы и блок-схемы в документации обновляются автоматически.
        </p>

        <div className="home-features">
          <div className="feature-card">
            <h3>Автоматическая генерация блок-схем</h3>
            <p>Переводите код в блок-схемы автоматически. Поддержка нескольких языков программирования (C/C++, Pascal)</p>
          </div>
          <div className="feature-card">
            <h3>Синхронизация кода и документации</h3>
            <p>Изменения в блок-схеме автоматически отображаются на код, поддерживая документацию в актуальном состоянии</p>
          </div>
          <div className="feature-card">
            <h3>Система рецензирования</h3>
            <p>Оставляйте комментарии и исправления блок-схем. Принимайте или отклоняйте замечания в процессе ревью</p>
          </div>
        </div>

        <div className="home-actions">
          <Link to="/login" className="home-button primary">
            Войти
          </Link>
          <Link to="/login?mode=register" className="home-button secondary">
            Зарегистрироваться
          </Link>
        </div>
      </div>
    </div>
  );
}

