// Рендеринг узлов
import type { FlowchartNode, PortType } from '../types/flowchart';

interface NodeRendererOptions {
    selectedNodeId: string | null;
    connectingFrom: string | null;
    connectingFromPort: PortType | null;
    zoom: number;
    onNodeClick?: (nodeId: string) => void;
    onNodeDoubleClick?: (nodeId: string) => void;
    onNodeDragStart?: (nodeId: string, event: MouseEvent) => void;
    onPortClick?: (nodeId: string, port: PortType, event: MouseEvent) => void;
    onPortConnect?: (fromNodeId: string, toNodeId: string, port: PortType) => void;
}

/**
 * Рендерит узел
 */
export function renderNode(
    node: FlowchartNode,
    container: HTMLElement,
    options: NodeRendererOptions
): HTMLElement | null {
    // Пропускаем скрытые узлы
    if ((node as any).hidden) {
        return null;
    }
    
    const {
        selectedNodeId,
        connectingFrom,
        zoom,
        onNodeClick,
        onNodeDoubleClick,
        onNodeDragStart,
        onPortClick,
        onPortConnect
    } = options;
    
    const nodeEl = document.createElement('div');
    nodeEl.className = `flowchart-node node-${node.type}`;
    if (selectedNodeId === node.id) {
        nodeEl.classList.add('selected');
    }
    if (connectingFrom) {
        nodeEl.classList.add('connecting');
    }
    
    nodeEl.style.left = `${node.x}px`;
    nodeEl.style.top = `${node.y}px`;
    nodeEl.style.width = `${node.width}px`;
    nodeEl.style.height = `${node.height}px`;
    nodeEl.dataset.id = node.id;
    
    nodeEl.innerHTML = `<div class="node-text">${node.text}</div>`;
    
    // Добавляем точки соединения
    const ports: PortType[] = ['top', 'right', 'bottom', 'left'];
    ports.forEach(port => {
        const point = document.createElement('div');
        point.className = 'node-connection-point';
        point.dataset.port = port;
        point.dataset.nodeId = node.id;
        
        const positions: Record<PortType, { left: string; top: string }> = {
            top: { left: '50%', top: '0' },
            right: { left: '100%', top: '50%' },
            bottom: { left: '50%', top: '100%' },
            left: { left: '0', top: '50%' }
        };
        
        const pos = positions[port];
        point.style.left = pos.left;
        point.style.top = pos.top;
        
        if (connectingFrom === node.id && options.connectingFromPort === port) {
            point.classList.add('connecting');
        }
        
        point.addEventListener('click', (e) => {
            e.stopPropagation();
            if (onPortClick) {
                onPortClick(node.id, port, e);
            }
        });
        
        nodeEl.appendChild(point);
    });
    
    // Обработчики событий
    if (onNodeClick) {
        nodeEl.addEventListener('click', (e) => {
            // Не срабатывает при клике на точку соединения
            if ((e.target as HTMLElement).classList.contains('node-connection-point')) {
                return;
            }
            e.stopPropagation();
            onNodeClick(node.id);
        });
    }
    
    if (onNodeDoubleClick) {
        nodeEl.addEventListener('dblclick', (e) => {
            e.stopPropagation();
            onNodeDoubleClick(node.id);
        });
    }
    
    if (onNodeDragStart) {
        nodeEl.addEventListener('mousedown', (e) => {
            const target = e.target as HTMLElement;
            if (target.tagName === 'TEXTAREA' || target.classList.contains('node-connection-point')) {
                return;
            }
            onNodeDragStart(node.id, e);
        });
    }
    
    container.appendChild(nodeEl);
    return nodeEl;
}

/**
 * Рендерит контролы для выбранного узла
 */
export function renderNodeControls(
    node: FlowchartNode,
    container: HTMLElement,
    options: {
        onConnect?: () => void;
        onDelete?: () => void;
    }
): HTMLElement {
    const existingControls = container.querySelector('.node-controls');
    if (existingControls) {
        existingControls.remove();
    }
    
    const controls = document.createElement('div');
    controls.className = 'node-controls';
    controls.style.left = `${node.x + node.width + 10}px`;
    controls.style.top = `${node.y}px`;
    
    controls.innerHTML = `
        <button class="node-control-btn" data-action="connect" title="Создать связь">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"></path>
                <path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"></path>
            </svg>
        </button>
        <button class="node-control-btn delete" data-action="delete" title="Удалить">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M3 6h18M19 6v14c0 1-1 2-2 2H7c-1 0-2-1-2-2V6M8 6V4c0-1 1-2 2-2h4c1 0 2 1 2 2v2"></path>
            </svg>
        </button>
    `;
    
    const connectBtn = controls.querySelector('[data-action="connect"]');
    const deleteBtn = controls.querySelector('[data-action="delete"]');
    
    if (connectBtn && options.onConnect) {
        connectBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            options.onConnect!();
        });
    }
    
    if (deleteBtn && options.onDelete) {
        deleteBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            options.onDelete!();
        });
    }
    
    container.appendChild(controls);
    return controls;
}

/**
 * Рендерит все узлы
 */
export function renderAllNodes(
    nodes: FlowchartNode[],
    container: HTMLElement,
    options: NodeRendererOptions & {
        onRenderControls?: (node: FlowchartNode, container: HTMLElement) => void;
        skipNodeId?: string; // ID узла, который не нужно перерисовывать (например, во время перетаскивания)
    }
): void {
    // Если есть узел, который нужно пропустить, просто удаляем остальные
    // и не трогаем пропускаемый узел
    if (options.skipNodeId) {
        const existingNode = container.querySelector(`[data-id="${options.skipNodeId}"]`);
        if (existingNode) {
            // Удаляем все узлы кроме пропускаемого
            Array.from(container.children).forEach(child => {
                if (child !== existingNode && child.classList.contains('flowchart-node')) {
                    child.remove();
                }
            });
            // Рендерим только новые узлы
            nodes.forEach(node => {
                if (node.id !== options.skipNodeId) {
                    const nodeEl = renderNode(node, container, options);
                    if (nodeEl && options.selectedNodeId === node.id && options.onRenderControls) {
                        options.onRenderControls(node, container);
                    }
                }
            });
            // Обновляем контролы для пропускаемого узла, если он выбран
            if (options.selectedNodeId === options.skipNodeId && options.onRenderControls) {
                const node = nodes.find(n => n.id === options.skipNodeId);
                if (node) {
                    options.onRenderControls(node, container);
                }
            }
            return;
        }
    }
    
    // Обычный рендеринг всех узлов
    container.innerHTML = '';
    
    nodes.forEach(node => {
        const nodeEl = renderNode(node, container, options);
        
        if (nodeEl && options.selectedNodeId === node.id && options.onRenderControls) {
            options.onRenderControls(node, container);
        }
    });
}

