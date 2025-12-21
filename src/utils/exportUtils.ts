// Утилиты для экспорта блок-схемы (SVG, PNG)

/**
 * Экспортирует блок-схему в SVG
 */
export function exportToSVG(
    canvasWrapper: HTMLElement,
    nodes: Array<{ id: string; x: number; y: number; width: number; height: number; text: string; type: string }>,
    connections: Array<{ from: string; to: string; fromPort: string; toPort: string; label?: string }>
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
        .node-start, .node-end { fill: #22c55e; stroke: #16a34a; }
        .node-process { fill: #3b82f6; stroke: #2563eb; }
        .node-decision { fill: #f59e0b; stroke: #d97706; }
        .node-input, .node-output { fill: #8b5cf6; stroke: #7c3aed; }
        .connection-line { stroke: #64748b; stroke-width: 2; fill: none; }
        .node-text { fill: #1e293b; font-family: Arial, sans-serif; font-size: 12px; text-anchor: middle; dominant-baseline: middle; }
    `;
    svg.appendChild(style);
    
    // Рендерим узлы
    nodes.forEach(node => {
        let shape: SVGGraphicsElement;
        
        if (node.type === 'start' || node.type === 'end') {
            shape = document.createElementNS('http://www.w3.org/2000/svg', 'ellipse');
            shape.setAttribute('cx', (node.x + node.width / 2 - bounds.minX + 50).toString());
            shape.setAttribute('cy', (node.y + node.height / 2 - bounds.minY + 50).toString());
            shape.setAttribute('rx', (node.width / 2).toString());
            shape.setAttribute('ry', (node.height / 2).toString());
        } else if (node.type === 'decision') {
            shape = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
            const centerX = node.x + node.width / 2 - bounds.minX + 50;
            const centerY = node.y + node.height / 2 - bounds.minY + 50;
            const w = node.width / 2;
            const h = node.height / 2;
            shape.setAttribute('points', 
                `${centerX},${centerY - h} ` +
                `${centerX + w},${centerY} ` +
                `${centerX},${centerY + h} ` +
                `${centerX - w},${centerY}`
            );
        } else {
            shape = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
            shape.setAttribute('x', (node.x - bounds.minX + 50).toString());
            shape.setAttribute('y', (node.y - bounds.minY + 50).toString());
            shape.setAttribute('width', node.width.toString());
            shape.setAttribute('height', node.height.toString());
            shape.setAttribute('rx', '4');
        }
        
        shape.setAttribute('class', `flowchart-node node-${node.type}`);
        svg.appendChild(shape);
        
        // Добавляем текст
        const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        text.setAttribute('x', (node.x + node.width / 2 - bounds.minX + 50).toString());
        text.setAttribute('y', (node.y + node.height / 2 - bounds.minY + 50).toString());
        text.setAttribute('class', 'node-text');
        text.textContent = node.text;
        svg.appendChild(text);
    });
    
    // TODO: Добавить рендеринг соединений
    
    return new XMLSerializer().serializeToString(svg);
}

/**
 * Экспортирует блок-схему в PNG
 */
export async function exportToPNG(
    canvasWrapper: HTMLElement,
    nodes: Array<{ id: string; x: number; y: number; width: number; height: number; text: string; type: string }>,
    connections: Array<{ from: string; to: string }>,
    zoom: number = 1
): Promise<string> {
    // Создаем временный canvas
    const bounds = calculateBounds(nodes);
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
    
    if (!ctx) throw new Error('Не удалось получить контекст canvas');
    
    canvas.width = (bounds.width + 100) * zoom;
    canvas.height = (bounds.height + 100) * zoom;
    
    ctx.scale(zoom, zoom);
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, canvas.width / zoom, canvas.height / zoom);
    
    // Рендерим узлы и соединения
    // TODO: Реализовать полный рендеринг
    
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

