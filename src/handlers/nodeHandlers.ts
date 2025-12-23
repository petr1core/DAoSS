// Обработчики для работы с узлами
import type { FlowchartNode, NodeType } from '../types/flowchart';
import { getDefaultText } from '../utils/nodeUtils';

/**
 * Создает новый узел
 */
export function createNode(
    type: NodeType | 'function' | 'main',
    nodes: FlowchartNode[],
    yPosition: number = 100,
    options?: {
        isFunction?: boolean;
        isMainFunction?: boolean;
    }
): FlowchartNode {
    // Для function и main используем process тип, но с флагами
    const actualType: NodeType = (type === 'function' || type === 'main') ? 'process' : type;
    
    const node: FlowchartNode = {
        id: `node-${Date.now()}`,
        type: actualType,
        x: 400,
        y: yPosition + nodes.length * 150,
        width: actualType === 'decision' ? 180 : (actualType === 'start' || actualType === 'end') ? 120 : 180,
        height: actualType === 'decision' ? 100 : (actualType === 'start' || actualType === 'end') ? 60 : 80,
        text: getDefaultText(type as NodeType),
        codeReference: '',
        comments: []
    };
    
    // Добавляем флаги через расширение объекта (так как TypeScript не позволяет добавлять произвольные свойства)
    if (options?.isFunction || type === 'function') {
        (node as any).isFunction = true;
    }
    if (options?.isMainFunction || type === 'main') {
        (node as any).isMainFunction = true;
    }
    
    return node;
}

/**
 * Начинает редактирование узла
 */
export function startEditingNode(
    nodeId: string,
    nodes: FlowchartNode[],
    onUpdate: (nodeId: string, updates: Partial<FlowchartNode>) => void
): void {
    const nodeEl = document.querySelector(`[data-id="${nodeId}"]`);
    if (!nodeEl) {
        console.warn('Node element not found for editing:', nodeId);
        return;
    }
    
    const textEl = nodeEl.querySelector('.node-text');
    if (!textEl) {
        console.warn('Text element not found in node:', nodeId);
        return;
    }
    
    const node = nodes.find(n => n.id === nodeId);
    if (!node) {
        console.warn('Node not found in state:', nodeId);
        return;
    }
    
    const textarea = document.createElement('textarea');
    textarea.className = 'node-edit';
    textarea.value = node.text || '';
    
    textEl.replaceWith(textarea);
    textarea.focus();
    textarea.select();
    
    const finishEdit = () => {
        if (!textarea.parentNode) return;
        
        onUpdate(nodeId, { text: textarea.value });
        
        const newTextEl = document.createElement('div');
        newTextEl.className = 'node-text';
        newTextEl.textContent = textarea.value;
        textarea.replaceWith(newTextEl);
    };
    
    const handleBlur = () => {
        finishEdit();
        textarea.removeEventListener('blur', handleBlur);
    };
    
    textarea.addEventListener('blur', handleBlur);
    textarea.addEventListener('keydown', (e) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            finishEdit();
        } else if (e.key === 'Escape') {
            e.preventDefault();
            const newTextEl = document.createElement('div');
            newTextEl.className = 'node-text';
            newTextEl.textContent = node.text;
            textarea.replaceWith(newTextEl);
        }
    });
}

/**
 * Начинает перетаскивание узла
 */
export function startDraggingNode(
    nodeId: string,
    nodes: FlowchartNode[],
    event: MouseEvent,
    zoom: number,
    wrapper: HTMLElement
): { nodeId: string; offset: { x: number; y: number } } | null {
    const node = nodes.find(n => n.id === nodeId);
    const nodeEl = document.querySelector(`[data-id="${nodeId}"]`);
    
    if (!node || !nodeEl) return null;
    
    nodeEl.classList.add('dragging');
    event.preventDefault();
    
    // Вычисляем offset относительно wrapper с учетом scroll и zoom
    const rect = wrapper.getBoundingClientRect();
    const nodeScreenX = node.x * zoom + rect.left - wrapper.scrollLeft;
    const nodeScreenY = node.y * zoom + rect.top - wrapper.scrollTop;
    
    return {
        nodeId,
        offset: {
            x: event.clientX - nodeScreenX,
            y: event.clientY - nodeScreenY
        }
    };
}

/**
 * Обновляет позицию узла при перетаскивании (использует прямую DOM-манипуляцию для производительности)
 */
export function updateNodePosition(
    nodeId: string,
    nodes: FlowchartNode[],
    event: MouseEvent,
    dragOffset: { x: number; y: number },
    zoom: number,
    wrapper: HTMLElement
): { x: number; y: number } {
    const node = nodes.find(n => n.id === nodeId);
    if (!node) return { x: 0, y: 0 };
    
    // Вычисляем новую позицию с учетом offset
    const rect = wrapper.getBoundingClientRect();
    // event.clientX/Y - это позиция мыши на экране
    // dragOffset - это разница между позицией мыши и позицией узла на момент начала перетаскивания
    // Вычитаем dragOffset, чтобы узел оставался под курсором в той же точке
    const newX = (event.clientX - rect.left - dragOffset.x + wrapper.scrollLeft) / zoom;
    const newY = (event.clientY - rect.top - dragOffset.y + wrapper.scrollTop) / zoom;
    
    // Обновляем позицию напрямую в DOM для плавного перетаскивания
    const nodeEl = document.querySelector(`[data-id="${nodeId}"]`) as HTMLElement;
    if (nodeEl) {
        nodeEl.style.left = `${newX}px`;
        nodeEl.style.top = `${newY}px`;
    }
    
    return { x: newX, y: newY };
}

/**
 * Завершает перетаскивание узла
 */
export function stopDraggingNode(nodeId: string | null): void {
    if (!nodeId) return;
    
    const nodeEl = document.querySelector(`[data-id="${nodeId}"]`);
    if (nodeEl) {
        nodeEl.classList.remove('dragging');
    }
}

