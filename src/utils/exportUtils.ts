// Утилиты для экспорта блок-схемы (SVG, PNG)
import { getPortPosition } from './geometry';
import type { FlowchartNode, Connection } from '../types/flowchart';

/**
 * Экспортирует блок-схему в SVG
 */
export function exportToSVG(
    canvasWrapper: HTMLElement,
    nodes: FlowchartNode[],
    connections: Connection[]
): string {
    // Создаем SVG элемент
    const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    const bounds = calculateBounds(nodes);
    
    svg.setAttribute('width', (bounds.width + 100).toString());
    svg.setAttribute('height', (bounds.height + 100).toString());
    svg.setAttribute('xmlns', 'http://www.w3.org/2000/svg');
    
    // Добавляем стили
    const style = document.createElementNS('http://www.w3.org/2000/svg', 'style');
    style.textContent = `
        .flowchart-node { fill: #ffffff; stroke: #64748b; stroke-width: 2; }
        .node-start, .node-end { fill: #dcfce7; stroke: #16a34a; }
        .node-process { fill: #e9d5ff; stroke: #2563eb; }
        .node-process.function-node { fill: #bfdbfe; stroke: #3b82f6; }
        .node-process.function-node.function-prototype { fill: #e0e7ff; stroke: #818cf8; stroke-dasharray: 4,2; opacity: 0.9; }
        .node-process.main-function { fill: #fbbf24; stroke: #d97706; stroke-width: 3; }
        .node-decision { fill: #fef9c3; stroke: #d97706; }
        .node-input, .node-output { fill: #cffafe; stroke: #06b6d4; }
        .connection-line { stroke: #64748b; stroke-width: 2; fill: none; }
        .node-text { fill: #1e293b; font-family: Arial, sans-serif; font-size: 12px; text-anchor: middle; dominant-baseline: middle; }
    `;
    svg.appendChild(style);
    
    const padding = 50;
    const offsetX = -bounds.minX + padding;
    const offsetY = -bounds.minY + padding;
    
    // Создаем группу для соединений (чтобы они были под узлами)
    const connectionsGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    connectionsGroup.setAttribute('class', 'connections');
    
    // Рендерим соединения
    connections.forEach(conn => {
        const fromNode = nodes.find(n => n.id === conn.from);
        const toNode = nodes.find(n => n.id === conn.to);
        
        if (!fromNode || !toNode) return;
        
        const fromPort = conn.fromPort || 'bottom';
        const toPort = conn.toPort || 'top';
        
        const fromPos = getPortPosition(fromNode, fromPort);
        const toPos = getPortPosition(toNode, toPort);
        
        // Вычисляем контрольные точки для кривой Безье
        const dx = Math.abs(toPos.x - fromPos.x);
        const dy = Math.abs(toPos.y - fromPos.y);
        const controlOffset = Math.min(dx, dy) * 0.5;
        
        let cp1x = fromPos.x;
        let cp1y = fromPos.y;
        let cp2x = toPos.x;
        let cp2y = toPos.y;
        
        if (fromPort === 'bottom') {
            cp1y = fromPos.y + controlOffset;
        } else if (fromPort === 'top') {
            cp1y = fromPos.y - controlOffset;
        } else if (fromPort === 'right') {
            cp1x = fromPos.x + controlOffset;
        } else if (fromPort === 'left') {
            cp1x = fromPos.x - controlOffset;
        }
        
        if (toPort === 'top') {
            cp2y = toPos.y - controlOffset;
        } else if (toPort === 'bottom') {
            cp2y = toPos.y + controlOffset;
        } else if (toPort === 'right') {
            cp2x = toPos.x + controlOffset;
        } else if (toPort === 'left') {
            cp2x = toPos.x - controlOffset;
        }
        
        // Создаем SVG path для соединения
        const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
        const adjustedFromX = fromPos.x + offsetX;
        const adjustedFromY = fromPos.y + offsetY;
        const adjustedToX = toPos.x + offsetX;
        const adjustedToY = toPos.y + offsetY;
        const adjustedCp1x = cp1x + offsetX;
        const adjustedCp1y = cp1y + offsetY;
        const adjustedCp2x = cp2x + offsetX;
        const adjustedCp2y = cp2y + offsetY;
        
        path.setAttribute('d', `M ${adjustedFromX} ${adjustedFromY} C ${adjustedCp1x} ${adjustedCp1y}, ${adjustedCp2x} ${adjustedCp2y}, ${adjustedToX} ${adjustedToY}`);
        path.setAttribute('class', 'connection-line');
        path.setAttribute('fill', 'none');
        path.setAttribute('stroke', '#64748b');
        path.setAttribute('stroke-width', '3');
        
        // Создаем стрелку (маркер)
        const marker = document.createElementNS('http://www.w3.org/2000/svg', 'marker');
        const markerId = `arrowhead-${conn.id}`;
        marker.setAttribute('id', markerId);
        marker.setAttribute('markerWidth', '10');
        marker.setAttribute('markerHeight', '10');
        marker.setAttribute('refX', '9');
        marker.setAttribute('refY', '3');
        marker.setAttribute('orient', 'auto');
        marker.setAttribute('markerUnits', 'userSpaceOnUse');
        
        const arrowPolygon = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
        arrowPolygon.setAttribute('points', '0 0, 10 3, 0 6');
        arrowPolygon.setAttribute('fill', '#64748b');
        marker.appendChild(arrowPolygon);
        
        // Добавляем маркер в defs, если его еще нет
        let defs = svg.querySelector('defs');
        if (!defs) {
            defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
            svg.insertBefore(defs, svg.firstChild);
        }
        defs.appendChild(marker);
        
        path.setAttribute('marker-end', `url(#${markerId})`);
        connectionsGroup.appendChild(path);
        
        // Добавляем метку соединения, если есть
        if (conn.label) {
            const labelX = (adjustedFromX + adjustedToX) / 2;
            const labelY = (adjustedFromY + adjustedToY) / 2;
            
            const labelBg = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
            labelBg.setAttribute('x', (labelX - 30).toString());
            labelBg.setAttribute('y', (labelY - 8).toString());
            labelBg.setAttribute('width', '60');
            labelBg.setAttribute('height', '16');
            labelBg.setAttribute('fill', '#ffffff');
            labelBg.setAttribute('stroke', '#64748b');
            labelBg.setAttribute('stroke-width', '1');
            labelBg.setAttribute('rx', '2');
            
            const labelText = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            labelText.setAttribute('x', labelX.toString());
            labelText.setAttribute('y', labelY.toString());
            labelText.setAttribute('text-anchor', 'middle');
            labelText.setAttribute('dominant-baseline', 'middle');
            labelText.setAttribute('font-size', '12');
            labelText.setAttribute('fill', '#1e293b');
            labelText.setAttribute('font-weight', 'bold');
            labelText.textContent = conn.label;
            
            connectionsGroup.appendChild(labelBg);
            connectionsGroup.appendChild(labelText);
        }
    });
    
    svg.appendChild(connectionsGroup);
    
    // Рендерим узлы
    nodes.forEach(node => {
        const x = node.x + offsetX;
        const y = node.y + offsetY;
        
        let shape: SVGGraphicsElement;
        
        if (node.type === 'start' || node.type === 'end') {
            shape = document.createElementNS('http://www.w3.org/2000/svg', 'ellipse');
            shape.setAttribute('cx', (x + node.width / 2).toString());
            shape.setAttribute('cy', (y + node.height / 2).toString());
            shape.setAttribute('rx', (node.width / 2).toString());
            shape.setAttribute('ry', (node.height / 2).toString());
        } else if (node.type === 'decision') {
            shape = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
            const centerX = x + node.width / 2;
            const centerY = y + node.height / 2;
            const w = node.width / 2;
            const h = node.height / 2;
            shape.setAttribute('points', 
                `${centerX},${centerY - h} ` +
                `${centerX + w},${centerY} ` +
                `${centerX},${centerY + h} ` +
                `${centerX - w},${centerY}`
            );
        } else if (node.type === 'input' || node.type === 'output') {
            // Параллелограмм
            shape = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
            const offset = 10;
            shape.setAttribute('points', 
                `${x + offset},${y} ` +
                `${x + node.width},${y} ` +
                `${x + node.width - offset},${y + node.height} ` +
                `${x},${y + node.height}`
            );
        } else {
            shape = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
            shape.setAttribute('x', x.toString());
            shape.setAttribute('y', y.toString());
            shape.setAttribute('width', node.width.toString());
            shape.setAttribute('height', node.height.toString());
            shape.setAttribute('rx', '4');
        }
        
        // Добавляем классы в зависимости от типа узла и флагов
        let className = `flowchart-node node-${node.type}`;
        const nodeAny = node as any;
        if (nodeAny.isFunction) {
            className += ' function-node';
            if (nodeAny.isPrototype) {
                className += ' function-prototype';
            }
        }
        if (nodeAny.isMainFunction) {
            className += ' main-function';
        }
        shape.setAttribute('class', className);
        svg.appendChild(shape);
        
        // Добавляем текст с поддержкой многострочности
        const textLines = node.text.split('\n');
        const lineHeight = 14;
        const startY = y + node.height / 2 - ((textLines.length - 1) * lineHeight) / 2;
        
        textLines.forEach((line, index) => {
            const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            text.setAttribute('x', (x + node.width / 2).toString());
            text.setAttribute('y', (startY + index * lineHeight).toString());
            text.setAttribute('class', 'node-text');
            text.setAttribute('text-anchor', 'middle');
            text.setAttribute('dominant-baseline', 'middle');
            text.textContent = line;
            svg.appendChild(text);
        });
    });
    
    return new XMLSerializer().serializeToString(svg);
}

/**
 * Экспортирует блок-схему в PNG
 */
export async function exportToPNG(
    canvasWrapper: HTMLElement,
    nodes: FlowchartNode[],
    connections: Connection[],
    zoom: number = 1
): Promise<string> {
    // Создаем временный canvas
    const bounds = calculateBounds(nodes);
    const padding = 50;
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
    
    if (!ctx) throw new Error('Не удалось получить контекст canvas');
    
    // Устанавливаем размеры canvas с учетом zoom
    const canvasWidth = (bounds.width + padding * 2) * zoom;
    const canvasHeight = (bounds.height + padding * 2) * zoom;
    
    // Проверяем ограничения canvas (максимум ~16,777,216 пикселей)
    const maxCanvasSize = Math.sqrt(16777216); // ~4096
    if (canvasWidth > maxCanvasSize || canvasHeight > maxCanvasSize) {
        // Если слишком большой, уменьшаем zoom
        const scaleFactor = Math.min(maxCanvasSize / canvasWidth, maxCanvasSize / canvasHeight);
        zoom = zoom * scaleFactor * 0.95; // Немного уменьшаем для безопасности
    }
    
    canvas.width = (bounds.width + padding * 2) * zoom;
    canvas.height = (bounds.height + padding * 2) * zoom;
    
    // Применяем zoom
    ctx.scale(zoom, zoom);
    
    // Белый фон
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, canvas.width / zoom, canvas.height / zoom);
    
    // Смещение для учета bounds и padding
    const offsetX = -bounds.minX + padding;
    const offsetY = -bounds.minY + padding;
    
    // Рендерим соединения сначала (чтобы они были под узлами)
    ctx.save();
    ctx.translate(offsetX, offsetY);
    
    connections.forEach(conn => {
        const fromNode = nodes.find(n => n.id === conn.from);
        const toNode = nodes.find(n => n.id === conn.to);
        
        if (!fromNode || !toNode) return;
        
        const fromPort = conn.fromPort || 'bottom';
        const toPort = conn.toPort || 'top';
        
        const fromPos = getPortPosition(fromNode, fromPort);
        const toPos = getPortPosition(toNode, toPort);
        
        // Вычисляем контрольные точки для кривой Безье
        const dx = Math.abs(toPos.x - fromPos.x);
        const dy = Math.abs(toPos.y - fromPos.y);
        const controlOffset = Math.min(dx, dy) * 0.5;
        
        let cp1x = fromPos.x;
        let cp1y = fromPos.y;
        let cp2x = toPos.x;
        let cp2y = toPos.y;
        
        if (fromPort === 'bottom') {
            cp1y = fromPos.y + controlOffset;
        } else if (fromPort === 'top') {
            cp1y = fromPos.y - controlOffset;
        } else if (fromPort === 'right') {
            cp1x = fromPos.x + controlOffset;
        } else if (fromPort === 'left') {
            cp1x = fromPos.x - controlOffset;
        }
        
        if (toPort === 'top') {
            cp2y = toPos.y - controlOffset;
        } else if (toPort === 'bottom') {
            cp2y = toPos.y + controlOffset;
        } else if (toPort === 'right') {
            cp2x = toPos.x + controlOffset;
        } else if (toPort === 'left') {
            cp2x = toPos.x - controlOffset;
        }
        
        // Рисуем кривую Безье
        ctx.strokeStyle = '#64748b';
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.moveTo(fromPos.x, fromPos.y);
        ctx.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, toPos.x, toPos.y);
        ctx.stroke();
        
        // Рисуем стрелку на конце
        const angle = Math.atan2(toPos.y - cp2y, toPos.x - cp2x);
        const arrowLength = 10;
        const arrowWidth = 6;
        
        ctx.save();
        ctx.translate(toPos.x, toPos.y);
        ctx.rotate(angle);
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(-arrowLength, -arrowWidth / 2);
        ctx.lineTo(-arrowLength, arrowWidth / 2);
        ctx.closePath();
        ctx.fillStyle = '#64748b';
        ctx.fill();
        ctx.restore();
        
        // Рисуем метку соединения, если есть
        if (conn.label) {
            const labelX = (fromPos.x + toPos.x) / 2;
            const labelY = (fromPos.y + toPos.y) / 2;
            
            ctx.font = 'bold 12px Arial';
            const textMetrics = ctx.measureText(conn.label);
            const textWidth = textMetrics.width;
            const textHeight = 14;
            const padding = 4;
            
            // Фон для метки
            ctx.fillStyle = '#ffffff';
            ctx.fillRect(
                labelX - textWidth / 2 - padding,
                labelY - textHeight / 2 - padding / 2,
                textWidth + padding * 2,
                textHeight + padding
            );
            
            // Обводка
            ctx.strokeStyle = '#64748b';
            ctx.lineWidth = 1;
            ctx.strokeRect(
                labelX - textWidth / 2 - padding,
                labelY - textHeight / 2 - padding / 2,
                textWidth + padding * 2,
                textHeight + padding
            );
            
            // Текст
            ctx.fillStyle = '#1e293b';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText(conn.label, labelX, labelY);
        }
    });
    
    ctx.restore();
    
    // Рендерим узлы
    nodes.forEach(node => {
        const x = node.x + offsetX;
        const y = node.y + offsetY;
        
        // Получаем цвета в зависимости от типа узла
        let fillColor: string;
        let strokeColor: string;
        
        if (node.type === 'start' || node.type === 'end') {
            fillColor = '#dcfce7';
            strokeColor = '#16a34a';
        } else if (node.type === 'process') {
            // Проверяем специальные типы функций
            const nodeAny = node as any;
            if (nodeAny.isMainFunction) {
                fillColor = '#fbbf24'; // Жёлтый для main (градиент упрощён)
                strokeColor = '#d97706';
            } else if (nodeAny.isFunction) {
                if (nodeAny.isPrototype) {
                    fillColor = '#e0e7ff'; // Светло-синий/лавандовый для прототипов
                    strokeColor = '#818cf8';
                } else {
                    fillColor = '#bfdbfe'; // Синий для реализаций функций
                    strokeColor = '#3b82f6';
                }
            } else {
                fillColor = '#e9d5ff'; // Фиолетовый для обычных process
                strokeColor = '#2563eb';
            }
        } else if (node.type === 'decision') {
            fillColor = '#fef9c3';
            strokeColor = '#d97706';
        } else if (node.type === 'input' || node.type === 'output') {
            fillColor = '#cffafe'; // Бирюзовый для IO операций
            strokeColor = '#06b6d4';
        } else {
            fillColor = '#ffffff';
            strokeColor = '#64748b';
        }
        
        ctx.save();
        ctx.translate(x, y);
        
        // Рисуем форму узла
        if (node.type === 'start' || node.type === 'end') {
            // Эллипс
            ctx.beginPath();
            ctx.ellipse(
                node.width / 2,
                node.height / 2,
                node.width / 2,
                node.height / 2,
                0,
                0,
                2 * Math.PI
            );
            ctx.fillStyle = fillColor;
            ctx.fill();
            ctx.strokeStyle = strokeColor;
            ctx.lineWidth = 2;
            ctx.stroke();
        } else if (node.type === 'decision') {
            // Ромб
            ctx.beginPath();
            ctx.moveTo(node.width / 2, 0);
            ctx.lineTo(node.width, node.height / 2);
            ctx.lineTo(node.width / 2, node.height);
            ctx.lineTo(0, node.height / 2);
            ctx.closePath();
            ctx.fillStyle = fillColor;
            ctx.fill();
            ctx.strokeStyle = strokeColor;
            ctx.lineWidth = 2;
            ctx.stroke();
        } else if (node.type === 'input' || node.type === 'output') {
            // Параллелограмм
            const offset = 10;
            ctx.beginPath();
            ctx.moveTo(offset, 0);
            ctx.lineTo(node.width, 0);
            ctx.lineTo(node.width - offset, node.height);
            ctx.lineTo(0, node.height);
            ctx.closePath();
            ctx.fillStyle = fillColor;
            ctx.fill();
            ctx.strokeStyle = strokeColor;
            ctx.lineWidth = 2;
            ctx.stroke();
        } else {
            // Прямоугольник с закругленными углами
            const radius = 4;
            ctx.beginPath();
            ctx.moveTo(radius, 0);
            ctx.lineTo(node.width - radius, 0);
            ctx.quadraticCurveTo(node.width, 0, node.width, radius);
            ctx.lineTo(node.width, node.height - radius);
            ctx.quadraticCurveTo(node.width, node.height, node.width - radius, node.height);
            ctx.lineTo(radius, node.height);
            ctx.quadraticCurveTo(0, node.height, 0, node.height - radius);
            ctx.lineTo(0, radius);
            ctx.quadraticCurveTo(0, 0, radius, 0);
            ctx.closePath();
            ctx.fillStyle = fillColor;
            ctx.fill();
            ctx.strokeStyle = strokeColor;
            ctx.lineWidth = 2;
            ctx.stroke();
        }
        
        // Рисуем текст
        ctx.fillStyle = '#1e293b';
        ctx.font = '14px Arial';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        
        // Разбиваем текст на строки, если он слишком длинный
        const maxWidth = node.width - 20;
        const words = node.text.split(' ');
        const lines: string[] = [];
        let currentLine = '';
        
        words.forEach(word => {
            const testLine = currentLine + (currentLine ? ' ' : '') + word;
            const metrics = ctx.measureText(testLine);
            if (metrics.width > maxWidth && currentLine) {
                lines.push(currentLine);
                currentLine = word;
            } else {
                currentLine = testLine;
            }
        });
        if (currentLine) {
            lines.push(currentLine);
        }
        
        // Если текст не помещается, обрезаем
        const lineHeight = 16;
        const maxLines = Math.floor((node.height - 10) / lineHeight);
        const displayLines = lines.slice(0, maxLines);
        
        displayLines.forEach((line, index) => {
            const yPos = node.height / 2 + (index - (displayLines.length - 1) / 2) * lineHeight;
            ctx.fillText(line, node.width / 2, yPos);
        });
        
        ctx.restore();
    });
    
    return canvas.toDataURL('image/png');
}

/**
 * Вычисляет границы всех узлов
 */
function calculateBounds(
    nodes: Array<{ x: number; y: number; width: number; height: number }>
): { minX: number; minY: number; maxX: number; maxY: number; width: number; height: number } {
    if (nodes.length === 0) {
        return { minX: 0, minY: 0, maxX: 800, maxY: 600, width: 800, height: 600 };
    }
    
    let minX = Infinity;
    let minY = Infinity;
    let maxX = -Infinity;
    let maxY = -Infinity;
    
    nodes.forEach(node => {
        minX = Math.min(minX, node.x);
        minY = Math.min(minY, node.y);
        maxX = Math.max(maxX, node.x + node.width);
        maxY = Math.max(maxY, node.y + node.height);
    });
    
    return {
        minX,
        minY,
        maxX,
        maxY,
        width: maxX - minX,
        height: maxY - minY
    };
}

/**
 * Скачивает SVG файл
 */
export function downloadSVG(svgContent: string, filename: string = 'flowchart.svg'): void {
    const blob = new Blob([svgContent], { type: 'image/svg+xml' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
}

/**
 * Скачивает PNG файл
 */
export function downloadPNG(dataUrl: string, filename: string = 'flowchart.png'): void {
    const link = document.createElement('a');
    link.href = dataUrl;
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}


