// Рендеринг соединений
import type { FlowchartNode, Connection } from '../types/flowchart';
import { getPortPosition, calculateConnectionPath } from '../utils/geometry';

interface ConnectionRendererOptions {
    zoom: number;
    selectedConnectionId: string | null;
    onConnectionClick?: (connectionId: string) => void;
}

/**
 * Рендерит соединения на canvas и SVG
 */
export function renderConnections(
    nodes: FlowchartNode[],
    connections: Connection[],
    canvas: HTMLCanvasElement,
    svg: SVGElement,
    wrapper: HTMLElement,
    options: ConnectionRendererOptions
): void {
    const { zoom, selectedConnectionId, onConnectionClick } = options;
    
    // Устанавливаем размеры
    canvas.width = wrapper.scrollWidth;
    canvas.height = wrapper.scrollHeight;
    
    const svgWidth = wrapper.scrollWidth;
    const svgHeight = wrapper.scrollHeight;
    svg.setAttribute('width', svgWidth.toString());
    svg.setAttribute('height', svgHeight.toString());
    svg.removeAttribute('viewBox');
    svg.removeAttribute('preserveAspectRatio');
    
    const ctx = canvas.getContext('2d');
    if (!ctx) return;
    
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    svg.innerHTML = '';
    
    // Создаем стрелку для маркера
    const defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
    const marker = document.createElementNS('http://www.w3.org/2000/svg', 'marker');
    const arrowSize = 10 * zoom;
    marker.setAttribute('id', 'arrowhead');
    marker.setAttribute('markerWidth', arrowSize.toString());
    marker.setAttribute('markerHeight', arrowSize.toString());
    marker.setAttribute('refX', (arrowSize * 0.9).toString());
    marker.setAttribute('refY', (arrowSize * 0.3).toString());
    marker.setAttribute('orient', 'auto');
    marker.setAttribute('markerUnits', 'userSpaceOnUse');
    
    const polygon = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
    polygon.setAttribute('points', `0 0, ${arrowSize} ${arrowSize * 0.3}, 0 ${arrowSize * 0.6}`);
    polygon.setAttribute('fill', selectedConnectionId ? '#3b82f6' : '#64748b');
    marker.appendChild(polygon);
    defs.appendChild(marker);
    svg.appendChild(defs);
    
    ctx.save();
    ctx.scale(zoom, zoom);
    
    // Рисуем соединения
    connections.forEach(conn => {
        const fromNode = nodes.find(n => n.id === conn.from);
        const toNode = nodes.find(n => n.id === conn.to);
        
        if (!fromNode || !toNode) return;
        
        const fromPort = getPortPosition(fromNode, conn.fromPort || 'bottom');
        const toPort = getPortPosition(toNode, conn.toPort || 'top');
        
        const fromX = fromPort.x;
        const fromY = fromPort.y;
        const toX = toPort.x;
        const toY = toPort.y;
        
        // Вычисляем контрольные точки для кривой Безье
        const dx = Math.abs(toX - fromX);
        const dy = Math.abs(toY - fromY);
        const controlOffset = Math.min(dx, dy) * 0.5;
        
        let cp1x = fromX;
        let cp1y = fromY;
        let cp2x = toX;
        let cp2y = toY;
        
        if (conn.fromPort === 'bottom') {
            cp1y = fromY + controlOffset;
        } else if (conn.fromPort === 'top') {
            cp1y = fromY - controlOffset;
        } else if (conn.fromPort === 'right') {
            cp1x = fromX + controlOffset;
        } else if (conn.fromPort === 'left') {
            cp1x = fromX - controlOffset;
        }
        
        if (conn.toPort === 'top') {
            cp2y = toY - controlOffset;
        } else if (conn.toPort === 'bottom') {
            cp2y = toY + controlOffset;
        } else if (conn.toPort === 'right') {
            cp2x = toX + controlOffset;
        } else if (conn.toPort === 'left') {
            cp2x = toX - controlOffset;
        }
        
        // Создаем SVG path
        const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
        const scaledFromX = fromX * zoom;
        const scaledFromY = fromY * zoom;
        const scaledToX = toX * zoom;
        const scaledToY = toY * zoom;
        const scaledCp1x = cp1x * zoom;
        const scaledCp1y = cp1y * zoom;
        const scaledCp2x = cp2x * zoom;
        const scaledCp2y = cp2y * zoom;
        
        const d = `M ${scaledFromX} ${scaledFromY} C ${scaledCp1x} ${scaledCp1y}, ${scaledCp2x} ${scaledCp2y}, ${scaledToX} ${scaledToY}`;
        path.setAttribute('d', d);
        path.setAttribute('class', `connection-line ${selectedConnectionId === conn.id ? 'selected' : ''}`);
        path.setAttribute('data-connection-id', conn.id);
        path.setAttribute('fill', 'none');
        const strokeColor = selectedConnectionId === conn.id ? '#3b82f6' : '#64748b';
        path.setAttribute('stroke', strokeColor);
        path.setAttribute('stroke-width', (3 * zoom).toString());
        path.setAttribute('marker-end', 'url(#arrowhead)');
        path.style.cursor = 'pointer';
        path.style.pointerEvents = 'stroke';
        
        if (onConnectionClick) {
            path.addEventListener('click', (e) => {
                e.stopPropagation();
                onConnectionClick(conn.id);
            });
        }
        
        svg.appendChild(path);
        
        // Добавляем метку на соединение, если она есть
        if (conn.label) {
            const labelX = (scaledFromX + scaledToX) / 2;
            const labelY = (scaledFromY + scaledToY) / 2;
            const fontSize = 12 * zoom;
            const textWidth = conn.label.length * fontSize * 0.6;
            const textHeight = fontSize * 1.2;
            const padding = 4 * zoom;
            
            // Создаем фон для метки
            const labelBg = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
            labelBg.setAttribute('x', (labelX - textWidth / 2 - padding).toString());
            labelBg.setAttribute('y', (labelY - textHeight / 2 - padding / 2).toString());
            labelBg.setAttribute('width', (textWidth + padding * 2).toString());
            labelBg.setAttribute('height', (textHeight + padding).toString());
            labelBg.setAttribute('fill', '#ffffff');
            labelBg.setAttribute('stroke', '#64748b');
            labelBg.setAttribute('stroke-width', (1 * zoom).toString());
            labelBg.setAttribute('rx', (2 * zoom).toString());
            
            const labelText = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            labelText.textContent = conn.label;
            labelText.setAttribute('x', labelX.toString());
            labelText.setAttribute('y', labelY.toString());
            labelText.setAttribute('text-anchor', 'middle');
            labelText.setAttribute('dominant-baseline', 'middle');
            labelText.setAttribute('font-size', fontSize.toString());
            labelText.setAttribute('fill', '#1e293b');
            labelText.setAttribute('font-weight', 'bold');
            
            svg.appendChild(labelBg);
            svg.appendChild(labelText);
        }
    });
    
    ctx.restore();
}

/**
 * Рендерит временную линию соединения при перетаскивании
 */
export function renderTemporaryConnection(
    canvas: HTMLCanvasElement,
    fromNode: FlowchartNode,
    fromPort: 'top' | 'right' | 'bottom' | 'left',
    mouseX: number,
    mouseY: number,
    zoom: number
): void {
    const ctx = canvas.getContext('2d');
    if (!ctx) return;
    
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    const fromPos = getPortPosition(fromNode, fromPort);
    const mouseXScaled = mouseX / zoom;
    const mouseYScaled = mouseY / zoom;
    
    ctx.save();
    ctx.scale(zoom, zoom);
    
    ctx.strokeStyle = '#3b82f6';
    ctx.lineWidth = 2;
    ctx.setLineDash([5, 5]);
    ctx.beginPath();
    ctx.moveTo(fromPos.x, fromPos.y);
    
    const dx = Math.abs(mouseXScaled - fromPos.x);
    const dy = Math.abs(mouseYScaled - fromPos.y);
    const controlOffset = Math.min(dx, dy) * 0.5;
    
    let cp1x = fromPos.x;
    let cp1y = fromPos.y;
    
    if (fromPort === 'bottom') {
        cp1y = fromPos.y + controlOffset;
    } else if (fromPort === 'top') {
        cp1y = fromPos.y - controlOffset;
    } else if (fromPort === 'right') {
        cp1x = fromPos.x + controlOffset;
    } else if (fromPort === 'left') {
        cp1x = fromPos.x - controlOffset;
    }
    
    ctx.bezierCurveTo(cp1x, cp1y, mouseXScaled, mouseYScaled, mouseXScaled, mouseYScaled);
    ctx.stroke();
    ctx.setLineDash([]);
    
    ctx.restore();
}

/**
 * Рендерит контролы для выбранного соединения
 */
export function renderConnectionControls(
    connectionId: string | null,
    connection: Connection | null,
    fromNode: FlowchartNode | null,
    toNode: FlowchartNode | null,
    container: HTMLElement,
    onDelete?: () => void
): void {
    const existingControls = container.querySelector('.connection-controls');
    if (existingControls) {
        existingControls.remove();
    }
    
    if (!connectionId || !connection || !fromNode || !toNode) {
        return;
    }
    
    const fromPort = getPortPosition(fromNode, connection.fromPort || 'bottom');
    const toPort = getPortPosition(toNode, connection.toPort || 'top');
    const midX = (fromPort.x + toPort.x) / 2;
    const midY = (fromPort.y + toPort.y) / 2;
    
    const controls = document.createElement('div');
    controls.className = 'connection-controls';
    controls.style.left = `${midX + 20}px`;
    controls.style.top = `${midY - 20}px`;
    
    controls.innerHTML = `
        <button class="node-control-btn delete" data-action="delete-connection" title="Удалить связь">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M3 6h18M19 6v14c0 1-1 2-2 2H7c-1 0-2-1-2-2V6M8 6V4c0-1 1-2 2-2h4c1 0 2 1 2 2v2"></path>
            </svg>
        </button>
    `;
    
    if (onDelete) {
        controls.addEventListener('click', (e) => {
            e.stopPropagation();
            const action = (e.target as HTMLElement).closest('[data-action]')?.getAttribute('data-action');
            if (action === 'delete-connection') {
                onDelete();
            }
        });
    }
    
    container.appendChild(controls);
}

