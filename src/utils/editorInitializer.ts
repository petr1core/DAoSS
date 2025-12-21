// Утилиты для инициализации редактора
import type { FlowchartStore } from '../state/flowchartStore';
import { initTheme, toggleTheme } from './themeUtils';
import { showToast } from './toast';
import { readFileAsText, getLanguageFromFilename } from './fileUtils';

/**
 * Инициализирует обработчики для кнопок добавления узлов
 */
export function initializeToolbarButtons(
    onAddNode: (type: string) => void
): void {
    document.querySelectorAll('.tool-btn').forEach(btn => {
        // Удаляем старые обработчики
        const newBtn = btn.cloneNode(true);
        btn.parentNode?.replaceChild(newBtn, btn);
        
        // Добавляем новый обработчик
        if (newBtn instanceof HTMLElement && newBtn.dataset.type) {
            newBtn.addEventListener('click', (e) => {
                e.preventDefault();
                e.stopPropagation();
                onAddNode(newBtn.dataset.type!);
            });
        }
    });
}

/**
 * Инициализирует обработчики для кнопок зума
 */
export function initializeZoomButtons(
    onZoomIn: () => void,
    onZoomOut: () => void,
    getZoom: () => number
): void {
    const zoomInBtn = document.getElementById('zoom-in');
    const zoomOutBtn = document.getElementById('zoom-out');
    const zoomValue = document.getElementById('zoom-value');
    
    if (zoomInBtn) {
        zoomInBtn.onclick = () => {
            onZoomIn();
            if (zoomValue) zoomValue.textContent = Math.round(getZoom() * 100).toString();
        };
    }
    
    if (zoomOutBtn) {
        zoomOutBtn.onclick = () => {
            onZoomOut();
            if (zoomValue) zoomValue.textContent = Math.round(getZoom() * 100).toString();
        };
    }
    
    // Обновляем начальное значение
    if (zoomValue) zoomValue.textContent = Math.round(getZoom() * 100).toString();
}

/**
 * Инициализирует кнопку переключения темы
 */
export function initializeThemeToggle(
    onThemeToggle: () => void,
    isDark: boolean
): void {
    const themeToggleBtn = document.getElementById('theme-toggle');
    if (themeToggleBtn) {
        themeToggleBtn.onclick = () => {
            onThemeToggle();
            const textSpan = themeToggleBtn.querySelector('.theme-toggle-text');
            if (textSpan) {
                const html = document.documentElement;
                const newIsDark = html.classList.contains('dark');
                textSpan.textContent = newIsDark ? 'Тёмная' : 'Светлая';
            }
        };
        
        // Устанавливаем начальный текст
        const textSpan = themeToggleBtn.querySelector('.theme-toggle-text');
        if (textSpan) {
            textSpan.textContent = isDark ? 'Тёмная' : 'Светлая';
        }
    }
}

/**
 * Инициализирует обработчики для загрузки файлов
 */
export function initializeFileUpload(
    onFileLoad: (code: string, language: string) => void
): void {
    const fileUpload = document.getElementById('file-upload') as HTMLInputElement | null;
    if (fileUpload) {
        fileUpload.onchange = async (e) => {
            const file = (e.target as HTMLInputElement).files?.[0];
            if (file) {
                try {
                    const code = await readFileAsText(file);
                    const language = getLanguageFromFilename(file.name) || 'pascal';
                    
                    // Устанавливаем язык в селекте
                    const languageSelect = document.getElementById('language-select') as HTMLSelectElement | null;
                    if (languageSelect) {
                        languageSelect.value = language;
                    }
                    
                    // Устанавливаем код в редактор
                    const codeEditor = document.getElementById('code-editor') as HTMLTextAreaElement | null;
                    if (codeEditor) {
                        codeEditor.value = code;
                    }
                    
                    onFileLoad(code, language);
                    showToast('Файл загружен');
                } catch (error) {
                    showToast('Ошибка загрузки файла', 'error');
                    console.error('File load error:', error);
                }
            }
        };
    }
}

/**
 * Инициализирует обработчики для комментариев
 */
export function initializeComments(
    onAddComment: () => void
): void {
    const addCommentBtn = document.getElementById('add-comment-btn');
    const newComment = document.getElementById('new-comment') as HTMLTextAreaElement | null;
    
    if (addCommentBtn) {
        addCommentBtn.onclick = onAddComment;
    }
    
    if (newComment) {
        newComment.onkeydown = (e) => {
            if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) {
                e.preventDefault();
                onAddComment();
            }
        };
    }
}

/**
 * Инициализирует переключение вкладок
 */
export function initializeTabs(
    onTabSwitch?: (tabName: string) => void
): void {
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const tabName = (btn as HTMLElement).dataset.tab;
            if (!tabName) return;
            
            // Обновляем классы кнопок
            document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            
            // Обновляем классы контента
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
            const tabContent = document.getElementById(`tab-${tabName}`);
            if (tabContent) {
                tabContent.classList.add('active');
            }
            
            if (onTabSwitch) {
                onTabSwitch(tabName);
            }
        });
    });
}


