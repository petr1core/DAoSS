import { useEffect, useRef, useState } from 'react';
import { useSearchParams } from 'react-router-dom';
import { api } from '../services/api';
import '../../styles.css';
import './FlowchartEditor.css';

interface ParserNotification {
  type: 'error' | 'warning' | 'success';
  message: string;
}

function FlowchartEditor() {
  const editorRef = useRef<HTMLDivElement>(null);
  const scriptLoadedRef = useRef(false);
  const [searchParams] = useSearchParams();
  const [fileLoaded, setFileLoaded] = useState(false);
  const [fileName, setFileName] = useState<string | null>(null);
  const [notification, setNotification] = useState<ParserNotification | null>(null);
  const [isLoading, setIsLoading] = useState(false);

  // Загрузка файла из URL параметров
  useEffect(() => {
    const fileId = searchParams.get('fileId');
    const projectId = searchParams.get('projectId');

    if (fileId && projectId && !fileLoaded) {
      loadFileAndGenerateDiagram(projectId, fileId);
    }
  }, [searchParams, fileLoaded]);

  const loadFileAndGenerateDiagram = async (projectId: string, fileId: string) => {
    setIsLoading(true);
    setNotification(null);
    
    try {
      // Получаем информацию о файле
      const file = await api.getSourceFile(projectId, fileId);
      setFileName(file.path);

      // Получаем версии файла
      const versions = await api.getSourceFileVersions(projectId, fileId);
      if (versions.length === 0) {
        setNotification({ type: 'warning', message: 'Файл не имеет версий' });
        setIsLoading(false);
        return;
      }

      // Находим последнюю версию
      const latestVersion = versions.reduce((latest, current) => 
        current.versionIndex > latest.versionIndex ? current : latest
      );

      // Получаем содержимое последней версии
      const version = await api.getSourceFileVersion(projectId, fileId, latestVersion.id);
      const code = version.content;

      // Определяем язык по расширению файла
      const extension = file.path.split('.').pop()?.toLowerCase();
      let language = 'cpp';
      if (extension === 'py') language = 'python';
      else if (extension === 'js' || extension === 'ts') language = 'javascript';
      else if (extension === 'java') language = 'java';
      else if (extension === 'pas') language = 'pascal';

      // Пытаемся распарсить через API
      let parserSuccess = false;
      try {
        const parseResult = await api.parseCode(code, language);
        if (parseResult.success && parseResult.representation) {
          // Парсер успешно вернул результат
          parserSuccess = true;
          setNotification({ type: 'success', message: 'Код успешно распарсен' });
          
          // Загружаем код в редактор с результатом парсера
          const loadCode = () => {
            if ((window as any).loadCodeIntoEditor) {
              (window as any).loadCodeIntoEditor(code, { 
                autoGenerate: true,
                useApiParser: false // Уже распарсили
              });
            }
          };
          
          if ((window as any).loadCodeIntoEditor) {
            loadCode();
          } else {
            const checkInterval = setInterval(() => {
              if ((window as any).loadCodeIntoEditor) {
                loadCode();
                clearInterval(checkInterval);
              }
            }, 100);
            setTimeout(() => clearInterval(checkInterval), 5000);
          }
        }
      } catch (parseError) {
        console.warn('Ошибка API парсера:', parseError);
        setNotification({ 
          type: 'error', 
          message: `Парсер недоступен: ${parseError instanceof Error ? parseError.message : 'неизвестная ошибка'}. Используется локальная генерация.`
        });
      }

      // Если парсер не сработал, используем локальную генерацию
      if (!parserSuccess) {
        const loadCodeLocally = () => {
          if ((window as any).loadCodeIntoEditor) {
            (window as any).loadCodeIntoEditor(code, { 
              autoGenerate: true,
              useApiParser: false
            });
          }
        };
        
        if ((window as any).loadCodeIntoEditor) {
          loadCodeLocally();
        } else {
          const checkInterval = setInterval(() => {
            if ((window as any).loadCodeIntoEditor) {
              loadCodeLocally();
              clearInterval(checkInterval);
            }
          }, 100);
          setTimeout(() => clearInterval(checkInterval), 5000);
        }
      }

      setFileLoaded(true);
    } catch (error) {
      console.error('Ошибка при загрузке файла:', error);
      setNotification({ 
        type: 'error', 
        message: `Не удалось загрузить файл: ${error instanceof Error ? error.message : 'неизвестная ошибка'}`
      });
    } finally {
      setIsLoading(false);
    }
  };

  useEffect(() => {
    // Загружаем и инициализируем app.js только один раз
    if (scriptLoadedRef.current) {
      // Если скрипт уже загружен, просто инициализируем редактор
      if ((window as any).initializeEditor) {
        setTimeout(() => {
          (window as any).initializeEditor();
        }, 100);
      }
      return;
    }
    
    // Создаем контейнер для toast-уведомлений, если его нет
    if (!document.getElementById('toast-container')) {
      const toastContainer = document.createElement('div');
      toastContainer.id = 'toast-container';
      toastContainer.className = 'toast-container';
      document.body.appendChild(toastContainer);
    }
    
    const script = document.createElement('script');
    script.src = '/app.js';
    script.async = true;
    
    script.onload = () => {
      scriptLoadedRef.current = true;
      // Инициализируем редактор после загрузки скрипта и рендеринга компонента
      setTimeout(() => {
        if ((window as any).initializeEditor) {
          (window as any).initializeEditor();
        }
      }, 200);
    };
    
    document.body.appendChild(script);
    
    return () => {
      // Очистка при размонтировании
      if (script.parentNode) {
        script.parentNode.removeChild(script);
      }
    };
  }, []);

  const dismissNotification = () => {
    setNotification(null);
  };

  return (
    <div className="app-container" ref={editorRef}>
      {/* Уведомление о парсере */}
      {notification && (
        <div className={`parser-notification ${notification.type}`}>
          <div className="notification-content">
            {notification.type === 'error' && (
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <circle cx="12" cy="12" r="10"></circle>
                <line x1="15" y1="9" x2="9" y2="15"></line>
                <line x1="9" y1="9" x2="15" y2="15"></line>
              </svg>
            )}
            {notification.type === 'warning' && (
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"></path>
                <line x1="12" y1="9" x2="12" y2="13"></line>
                <line x1="12" y1="17" x2="12.01" y2="17"></line>
              </svg>
            )}
            {notification.type === 'success' && (
              <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path>
                <polyline points="22 4 12 14.01 9 11.01"></polyline>
              </svg>
            )}
            <span>{notification.message}</span>
          </div>
          <button className="notification-close" onClick={dismissNotification}>
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <line x1="18" y1="6" x2="6" y2="18"></line>
              <line x1="6" y1="6" x2="18" y2="18"></line>
            </svg>
          </button>
        </div>
      )}
      
      {/* Индикатор загрузки */}
      {isLoading && (
        <div className="loading-overlay">
          <div className="loading-spinner"></div>
          <span>Загрузка файла и генерация диаграммы...</span>
        </div>
      )}
      
      {/* Левая панель - Инструменты */}
      <aside className="sidebar-left">
        <h2>Панель инструментов</h2>
        
        <div className="toolbar-section">
          <h3>Добавить блок</h3>
          <div className="toolbar-buttons">
            <button className="tool-btn" data-type="start">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <circle cx="12" cy="12" r="10"></circle>
              </svg>
              <span>Начало/Конец</span>
            </button>
            <button className="tool-btn" data-type="process">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <rect x="3" y="3" width="18" height="18" rx="2"></rect>
              </svg>
              <span>Процесс</span>
            </button>
            <button className="tool-btn" data-type="decision">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M12 2 L22 12 L12 22 L2 12 Z"></path>
              </svg>
              <span>Условие</span>
            </button>
            <button className="tool-btn" data-type="input">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M12 5v14M19 12l-7 7-7-7"></path>
              </svg>
              <span>Ввод</span>
            </button>
            <button className="tool-btn" data-type="output">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M12 19V5M5 12l7-7 7 7"></path>
              </svg>
              <span>Вывод</span>
            </button>
          </div>
        </div>

        <div className="toolbar-section">
          <h3>Управление</h3>
          <button className="control-btn" id="zoom-in">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <circle cx="11" cy="11" r="8"></circle>
              <path d="m21 21-4.35-4.35M11 8v6M8 11h6"></path>
            </svg>
            Увеличить
          </button>
          <button className="control-btn" id="zoom-out">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <circle cx="11" cy="11" r="8"></circle>
              <path d="m21 21-4.35-4.35M8 11h6"></path>
            </svg>
            Уменьшить
          </button>
          <button className="control-btn" id="export-svg">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M7 10l5 5 5-5M12 15V3"></path>
            </svg>
            Экспорт SVG
          </button>
          <button className="control-btn" id="export-png">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M7 10l5 5 5-5M12 15V3"></path>
            </svg>
            Экспорт PNG
          </button>
        </div>
      </aside>

      {/* Центральная область - Canvas */}
      <main className="main-content">
        <header className="top-bar">
          <h1>
            Редактор блок-схем
            {fileName && <span className="file-name-indicator"> - {fileName}</span>}
          </h1>
          <div className="zoom-indicator">
            Масштаб: <span id="zoom-value">100</span>%
          </div>
        </header>
        <div className="canvas-wrapper" id="canvas-wrapper">
          <canvas id="connections-canvas"></canvas>
          <svg id="connections-svg"></svg>
          <div className="flowchart-canvas" id="flowchart-canvas"></div>
        </div>
      </main>

      {/* Правая панель */}
      <aside className="sidebar-right">
        <div className="tabs">
          <button className="tab-btn active" data-tab="info">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path>
              <path d="M14 2v6h6M16 13H8M16 17H8M10 9H8"></path>
            </svg>
          </button>
          <button className="tab-btn" data-tab="code">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="m18 16 4-4-4-4M6 8l-4 4 4 4M14.5 4l-5 16"></path>
            </svg>
          </button>
          <button className="tab-btn" data-tab="comments">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"></path>
            </svg>
          </button>
          <button className="tab-btn" data-tab="history">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8"></path>
              <path d="M3 3v5h5M12 7v5l4 2"></path>
            </svg>
          </button>
        </div>

        <div className="tab-content active" id="tab-info">
          <div className="empty-state" id="info-empty">
            <p>Выберите элемент блок-схемы для просмотра информации</p>
          </div>
          <div className="info-panel" id="info-panel" style={{ display: 'none', flexDirection: 'column' }}>
            <h3>Информация о блоке</h3>
            <div className="form-group">
              <label>Тип блока</label>
              <div className="node-type-badge" id="node-type-badge"></div>
            </div>
            <div className="form-group">
              <label htmlFor="node-id-input">ID блока</label>
              <input type="text" id="node-id-input" readOnly />
            </div>
            <div className="form-group">
              <label htmlFor="node-text-input">Текст блока</label>
              <textarea id="node-text-input" rows={6}></textarea>
            </div>
            <div className="form-group">
              <label htmlFor="node-x-input">Позиция X</label>
              <input type="text" id="node-x-input" readOnly />
            </div>
            <div className="form-group">
              <label htmlFor="node-y-input">Позиция Y</label>
              <input type="text" id="node-y-input" readOnly />
            </div>
            <div className="form-group">
              <label htmlFor="node-code-input">Фрагмент кода</label>
              <textarea id="node-code-input" rows={4} placeholder="Связанный фрагмент кода..."></textarea>
            </div>
            <button className="primary-btn" id="save-node-btn">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"></path>
                <polyline points="17 21 17 13 7 13 7 21"></polyline>
                <polyline points="7 3 7 8 15 8"></polyline>
              </svg>
              Сохранить изменения
            </button>
            <div className="info-hint">
              <p><strong>Подсказка:</strong></p>
              <ul>
                <li>Двойной клик на блоке для быстрого редактирования</li>
                <li>Перетаскивайте блоки для изменения позиции</li>
              </ul>
            </div>
          </div>
        </div>

        <div className="tab-content" id="tab-code">
          <h3>Загрузка исходного кода</h3>
          <p className="tab-description">Загрузите код или введите его вручную для автоматической генерации блок-схемы</p>
          <div className="code-controls">
            <label htmlFor="file-upload" className="file-upload-btn">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4M17 8l-5-5-5 5M12 3v12"></path>
              </svg>
              Загрузить файл
            </label>
            <input type="file" id="file-upload" accept=".cpp,.c,.java,.py,.js,.ts" style={{ display: 'none' }} />
            <button className="control-btn" id="load-example-btn">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="m18 16 4-4-4-4M6 8l-4 4 4 4M14.5 4l-5 16"></path>
              </svg>
              Загрузить пример
            </button>
          </div>
          <div className="form-group">
            <label htmlFor="code-editor">Исходный код:</label>
            <textarea id="code-editor" rows={10} placeholder="Вставьте или введите код здесь..."></textarea>
          </div>
          <button className="primary-btn" id="generate-btn">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="m18 16 4-4-4-4M6 8l-4 4 4 4M14.5 4l-5 16"></path>
            </svg>
            Сгенерировать блок-схему
          </button>
          <div className="info-hint">
            <p><strong>Поддерживаемые конструкции:</strong></p>
            <ul>
              <li>if, while, for - условия и циклы</li>
              <li>print, cout, console.log - вывод</li>
              <li>input, cin, scanf - ввод</li>
              <li>Прочие операторы - процессы</li>
            </ul>
          </div>
        </div>

        <div className="tab-content" id="tab-comments">
          <div className="empty-state" id="comments-empty">
            <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" style={{ opacity: 0.5, margin: '0 auto 1rem' }}>
              <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"></path>
            </svg>
            <p>Выберите элемент для просмотра и добавления комментариев</p>
          </div>
          <div className="comments-panel" id="comments-panel" style={{ display: 'none' }}>
            <h3>Комментарии</h3>
            <p className="selected-node-name">Блок: <span id="comment-node-name"></span></p>
            <div className="comments-list" id="comments-list"></div>
            <div className="comment-form">
              <textarea id="new-comment" rows={3} placeholder="Введите комментарий..."></textarea>
              <button className="primary-btn" id="add-comment-btn">
                <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                  <path d="m22 2-7 20-4-9-9-4Z"></path>
                  <path d="M22 2 11 13"></path>
                </svg>
                Добавить комментарий
              </button>
              <p className="hint-text">Ctrl + Enter для быстрой отправки</p>
            </div>
          </div>
        </div>

        <div className="tab-content" id="tab-history">
          <h3>История изменений</h3>
          <p className="history-count">Всего записей: <span id="history-count">0</span></p>
          <div className="history-list" id="history-list"></div>
          <div className="info-hint">
            <p><strong>Информация:</strong></p>
            <ul>
              <li>История сохраняется автоматически при изменениях</li>
              <li>Нажмите на кнопку восстановления для возврата к версии</li>
            </ul>
          </div>
        </div>
      </aside>
      
      {/* Toast Container */}
      <div id="toast-container" className="toast-container"></div>
    </div>
  );
}

export default FlowchartEditor;

