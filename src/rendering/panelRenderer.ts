// Рендеринг панелей (информация, комментарии, история)
import type { FlowchartNode, HistoryEntry } from '../types/flowchart';
import { getTypeLabel } from '../utils/nodeUtils';
import { formatDate, getTimeAgo } from '../utils/date';

/**
 * Рендерит панель информации о выбранном узле
 */
export function renderInfoPanel(
    node: FlowchartNode | null,
    emptyStateEl: HTMLElement | null,
    infoPanelEl: HTMLElement | null
): void {
    if (!emptyStateEl || !infoPanelEl) {
        console.warn('Info panel elements not found');
        return;
    }
    
    if (!node) {
        emptyStateEl.style.display = 'flex';
        infoPanelEl.style.display = 'none';
        return;
    }
    
    emptyStateEl.style.display = 'none';
    infoPanelEl.style.display = 'flex';
    
    const badge = document.getElementById('node-type-badge');
    if (badge) {
        badge.textContent = getTypeLabel(node.type);
        badge.className = `node-type-badge badge-${node.type}`;
    }
    
    const idInput = document.getElementById('node-id-input') as HTMLInputElement | null;
    const textInput = document.getElementById('node-text-input') as HTMLTextAreaElement | null;
    const xInput = document.getElementById('node-x-input') as HTMLInputElement | null;
    const yInput = document.getElementById('node-y-input') as HTMLInputElement | null;
    const codeInput = document.getElementById('node-code-input') as HTMLTextAreaElement | null;
    
    if (idInput) idInput.value = node.id || '';
    if (textInput) textInput.value = node.text || '';
    if (xInput) xInput.value = Math.round(node.x || 0).toString();
    if (yInput) yInput.value = Math.round(node.y || 0).toString();
    
    // Для функций показываем тело функции, для остальных - codeReference
    const isFunction = (node as any).isFunction === true;
    if (codeInput) {
        if (isFunction && node.codeReference) {
            // Для функций показываем полное тело функции
            codeInput.value = node.codeReference;
            codeInput.rows = Math.max(10, node.codeReference.split('\n').length + 2);
            codeInput.readOnly = true; // Тело функции только для чтения
            codeInput.placeholder = 'Тело функции (только для чтения)';
        } else {
            // Для остальных нод показываем codeReference как обычно
            codeInput.value = node.codeReference || '';
            codeInput.rows = 4;
            codeInput.readOnly = false;
            codeInput.placeholder = 'Связанный фрагмент кода...';
        }
    }
}

/**
 * Рендерит панель комментариев
 */
export function renderComments(
    node: FlowchartNode | null,
    emptyStateEl: HTMLElement | null,
    commentsPanelEl: HTMLElement | null
): void {
    if (!emptyStateEl || !commentsPanelEl) return;
    
    if (!node) {
        emptyStateEl.style.display = 'flex';
        commentsPanelEl.style.display = 'none';
        return;
    }
    
    emptyStateEl.style.display = 'none';
    commentsPanelEl.style.display = 'flex';
    
    const nodeName = document.getElementById('comment-node-name');
    if (nodeName) nodeName.textContent = node.text;
    
    const commentsList = document.getElementById('comments-list');
    if (!commentsList) return;
    
    if (!node.comments || node.comments.length === 0) {
        commentsList.innerHTML = '<div class="empty-state"><p style="font-size: 0.875rem;">Комментариев пока нет<br><small>Добавьте первый комментарий ниже</small></p></div>';
    } else {
        commentsList.innerHTML = node.comments.map(comment => `
            <div class="comment-item">
                <div class="comment-header">
                    <div class="comment-author">${(comment as any).author || 'Аноним'}</div>
                    <div class="comment-date">${formatDate(comment.timestamp)}</div>
                </div>
                <div class="comment-text">${comment.text}</div>
            </div>
        `).join('');
    }
}

/**
 * Рендерит панель истории
 */
export function renderHistory(
    history: HistoryEntry[],
    historyListEl: HTMLElement | null,
    historyCountEl: HTMLElement | null,
    onRestore?: (entry: HistoryEntry) => void
): void {
    if (historyCountEl) {
        historyCountEl.textContent = history.length.toString();
    }
    if (!historyListEl) return;
    
    if (history.length === 0) {
        historyListEl.innerHTML = '<div class="empty-state"><p>История изменений пуста</p></div>';
        return;
    }
    
    historyListEl.innerHTML = [...history].reverse().map((entry, index) => `
        <div class="history-item">
            <div class="history-header">
                <div class="history-description">${entry.description}</div>
                <div class="history-time">${getTimeAgo(entry.timestamp)}</div>
            </div>
            <div class="history-meta">
                <span class="history-nodes">${entry.nodes.length} блоков</span>
                <span class="history-connections">${entry.connections.length} связей</span>
            </div>
            ${onRestore ? `
                <button class="history-restore-btn" data-history-id="${entry.id}">
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <path d="M1 4v6h6M23 20v-6h-6M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path>
                    </svg>
                    Восстановить
                </button>
            ` : ''}
        </div>
    `).join('');
    
    // Добавляем обработчики для кнопок восстановления
    if (onRestore) {
        historyListEl.querySelectorAll('.history-restore-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                const historyId = btn.getAttribute('data-history-id');
                const entry = history.find(e => e.id === historyId);
                if (entry) {
                    onRestore(entry);
                }
            });
        });
    }
}


