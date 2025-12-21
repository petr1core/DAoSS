// Геометрические утилиты для работы с узлами и соединениями
import type { FlowchartNode, PortType } from '../types/flowchart';

/**
 * Возвращает позицию порта на узле
 */
export function getPortPosition(node: FlowchartNode, port: PortType): { x: number; y: number } {
    const positions = {
        top: { x: node.x + node.width / 2, y: node.y },
        right: { x: node.x + node.width, y: node.y + node.height / 2 },
        bottom: { x: node.x + node.width / 2, y: node.y + node.height },
        left: { x: node.x, y: node.y + node.height / 2 }
    };
    return positions[port] || positions.bottom;
}

/**
 * Вычисляет расстояние между двумя точками
 */
export function getDistance(x1: number, y1: number, x2: number, y2: number): number {
    return Math.sqrt(Math.pow(x2 - x1, 2) + Math.pow(y2 - y1, 2));
}

/**
 * Проверяет, находится ли точка внутри узла
 */
export function isPointInNode(x: number, y: number, node: FlowchartNode): boolean {
    return x >= node.x && 
           x <= node.x + node.width && 
           y >= node.y && 
           y <= node.y + node.height;
}

/**
 * Возвращает узел под указанными координатами
 */
export function getNodeAtPosition(
    x: number, 
    y: number, 
    nodes: FlowchartNode[]
): FlowchartNode | null {
    // Проверяем в обратном порядке (верхние узлы имеют приоритет)
    for (let i = nodes.length - 1; i >= 0; i--) {
        if (isPointInNode(x, y, nodes[i])) {
            return nodes[i];
        }
    }
    return null;
}

/**
 * Вычисляет путь для соединения между двумя портами
 */
export function calculateConnectionPath(
    fromNode: FlowchartNode,
    fromPort: PortType,
    toNode: FlowchartNode,
    toPort: PortType
): string {
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
    
    // Настраиваем контрольные точки в зависимости от портов
    if (fromPort === 'bottom') {
        cp1y += controlOffset;
    } else if (fromPort === 'top') {
        cp1y -= controlOffset;
    } else if (fromPort === 'right') {
        cp1x += controlOffset;
    } else if (fromPort === 'left') {
        cp1x -= controlOffset;
    }
    
    if (toPort === 'bottom') {
        cp2y += controlOffset;
    } else if (toPort === 'top') {
        cp2y -= controlOffset;
    } else if (toPort === 'right') {
        cp2x += controlOffset;
    } else if (toPort === 'left') {
        cp2x -= controlOffset;
    }
    
    return `M ${fromPos.x} ${fromPos.y} C ${cp1x} ${cp1y}, ${cp2x} ${cp2y}, ${toPos.x} ${toPos.y}`;
}


